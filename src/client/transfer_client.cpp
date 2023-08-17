
#include "snbmodules/transfer_client.hpp"

namespace dunedaq::snbmodules
{
    TransferClient::TransferClient(const IPFormat &listening_ip, const std::string &client_id, const std::filesystem::path &listening_dir, const std::string &connection_prefix /*= "snbmodules"*/, int timeout_send /*= 10*/, int timeout_receive /*= 100*/)
        : NotificationInterface(connection_prefix, timeout_send, timeout_receive),
          m_listening_ip(listening_ip),
          m_client_id(client_id)
    {
        // remove all occurences of ./ in the file path
        std::string file_path_str = listening_dir.string();
        std::string x = "./";

        size_t pos = 0;
        while (1)
        {
            pos = file_path_str.find(x, pos);
            if (pos == std::string::npos)
            {
                break;
            }

            file_path_str.replace(pos, x.length(), "");
        }
        m_listening_dir = std::filesystem::absolute(file_path_str);
        std::filesystem::create_directories(m_listening_dir);
    }

    TransferClient::~TransferClient()
    {
        for (auto &session : m_sessions)
        {
            delete session.second;
        }
    }

    bool TransferClient::start(int timeout)
    {

        while (1)
        {
            // When starting, client wait for notification from bookkeeper.
            std::optional<NotificationData> msg = listen_for_notification(get_my_conn(), "", timeout);
            if (msg.has_value())
            {
                action_on_receive_notification(msg.value());
            }
            else
            {
                TLOG() << "debug : no notification received, timeout";
                return false;
            }

            // print status of sessions
            for (const auto &session : m_sessions)
            {
                TLOG() << session.second->to_string();
            }
        }

        return true;
    }

    bool TransferClient::do_work(std::atomic<bool> &running_flag)
    {

        while (running_flag.load())
        {
            // When starting, client wait for notification from bookkeeper.
            std::optional<NotificationData> msg = listen_for_notification(get_my_conn());
            if (msg.has_value())
            {
                action_on_receive_notification(msg.value());

                // print status of sessions
                for (const auto &session : m_sessions)
                {
                    TLOG() << session.second->to_string();
                }
            }
        }

        return true;
    }

    void TransferClient::create_new_transfer(const std::string &transfer_id, const std::string &protocol, const std::set<std::string> &dest_clients, const std::set<std::filesystem::path> &files, const nlohmann::json &protocol_options /*= nlohmann::json()*/)
    {

        std::string session_name = generate_session_id(transfer_id);

        // Checking if transfer already exists
        auto ses = get_sessions();
        if (ses.find(session_name) != ses.end())
        {
            TLOG() << "debug : transfer " << transfer_id << " already exists !";
            return;
        }

        TLOG() << "debug : creating new transfer with protocol " << protocol;

        std::optional<e_protocol_type> _protocol = magic_enum::enum_cast<e_protocol_type>(protocol);
        if (!_protocol.has_value())
        {
            ers::error(InvalidProtocolError(ERS_HERE, get_client_id(), protocol));
            return;
        }
        // Initialize transfer

        GroupMetadata group_transfer(transfer_id, session_name, m_listening_ip, _protocol.value(), protocol_options);

        for (const auto &file : files)
        {
            // Check if file exists
            if (!std::filesystem::exists(file))
            {
                ers::warning(FileForTransferNotExistError(ERS_HERE, get_client_id(), file));
                continue;
            }
            else
            {
                group_transfer.add_expected_file(file);
                group_transfer.add_file(std::move(create_metadata_from_file(file)));
            }
        }

        if (group_transfer.get_transfers_meta().empty())
        {
            ers::error(FileForTransferNotExistError(ERS_HERE, get_client_id(), "All files"));
            return;
        }

        // Sending to bookkeepers to update preparing state
        for (auto &bk : get_bookkeepers_conn())
        {
            send_notification(e_notification_type::GROUP_METADATA, session_name, bk, bk, group_transfer.export_to_string());
        }

        // Create local session, can take time depending on protocol
        TransferSession *s = create_session(std::move(group_transfer), e_session_type::Uploader, session_name, get_listening_dir().append(transfer_id), m_listening_ip, dest_clients);

        // Notify clients and sending group metadata
        for (const auto &client : dest_clients)
        {
            std::string new_session_name = generate_session_id(transfer_id, client);
            TLOG() << "debug : notifying client " << client;
            send_notification(e_notification_type::NEW_TRANSFER, get_client_id(), new_session_name, client, s->get_transfer_options().export_to_string());

            // Send transfer metadata
            for (auto &file : s->get_transfer_options().get_transfers_meta())
            {
                send_notification(e_notification_type::TRANSFER_METADATA, get_client_id(), new_session_name, client, file.export_to_string());
            }
        }
    }

    void TransferClient::start_transfer(const std::string &transfer_id)
    {

        TransferSession *session = get_session(transfer_id);

        if (session == nullptr)
        {
            return;
        }

        session->start_all();
    }

    void TransferClient::pause_transfer(const std::string &transfer_id)
    {

        TransferSession *session = get_session(transfer_id);

        if (session == nullptr)
        {
            return;
        }

        session->pause_all();
    }
    void TransferClient::resume_transfer(const std::string &transfer_id)
    {

        TransferSession *session = get_session(transfer_id);

        if (session == nullptr)
        {
            return;
        }

        session->resume_all();
    }

    void TransferClient::cancel_transfer(const std::string &transfer_id)
    {

        TransferSession *session = get_session(transfer_id);

        if (session == nullptr)
        {
            return;
        }

        session->cancel_all();
    }

    TransferSession *TransferClient::get_session(std::string transfer_id)
    {
        if (transfer_id.find("ses") == std::string::npos)
        {
            transfer_id = generate_session_id(transfer_id);
        }

        auto sessions_ref = get_sessions();
        if (sessions_ref.find(transfer_id) == sessions_ref.end())
        {
            ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), transfer_id));
            return nullptr;
        }

        return get_sessions().at(transfer_id);
    }

    // TODO ip useless ?
    TransferSession *TransferClient::create_session(GroupMetadata transfer_options, e_session_type type, std::string id, const std::filesystem::path &work_dir, IPFormat ip /*= IPFormat()*/, const std::set<std::string> &dest_clients /*= std::set<std::string>()*/)
    {
        if (ip.is_default())
        {
            ip = get_ip();
        }

        TransferSession *new_session = new TransferSession(std::move(transfer_options), type, std::move(id), std::move(ip), work_dir, get_bookkeepers_conn(), get_clients_conn());
        new_session->set_target_clients(dest_clients);

        TLOG() << "debug : session created " << magic_enum::enum_name(type);
        return add_session(new_session);
    }

    void TransferClient::share_available_files(const std::set<std::filesystem::path> &to_share, const std::string &dest)
    {

        for (const std::filesystem::path &f : to_share)
        {
            TLOG() << "debug : Sharing " << f.filename();
            TransferMetadata fmeta = create_metadata_from_file(f);
            m_available_files.insert({f.string(), std::move(fmeta)});
            send_notification(e_notification_type::TRANSFER_METADATA, get_client_id(), dest, dest, m_available_files.at(f.string()).export_to_string());
        }
    }

    bool TransferClient::action_on_receive_notification(NotificationData notif)
    {
        TLOG() << "debug : Action on request " << notif.m_notification << " for " << notif.m_target_id;

        if (notif.m_target_id.find(get_client_id()) == std::string::npos && notif.m_target_id != "all")
        {
            ers::warning(NotificationWrongDestinationError(ERS_HERE, get_client_id(), notif.m_source_id, notif.m_target_id));
            return false;
        }

        // Use enum cast for converting string to enum, still working with older clients and user readable
        auto action = magic_enum::enum_cast<e_notification_type>(notif.m_notification);

        if (action.has_value() == false)
        {
            ers::warning(InvalidNotificationReceivedError(ERS_HERE, get_client_id(), notif.m_source_id, notif.m_notification));
        }

        switch (action.value())
        {

        case e_notification_type::CONNECTION_REQUEST:
        {
            TLOG() << "debug : receive connection request, sending available files";
            std::set<std::filesystem::path> to_share;
            scan_available_files(to_share, true);
            share_available_files(to_share, notif.m_source_id);
            send_notification(e_notification_type::TRANSFER_METADATA, get_client_id(), notif.m_source_id, notif.m_source_id, "end");
            break;
        }

        case e_notification_type::NEW_TRANSFER:
        {
            GroupMetadata metadata(notif.m_data, false);
            e_session_type type = Downloader;

            // If file is available, create a downloader session
            for (const auto &f : metadata.get_expected_files())
            {
                if (m_available_files.find(f) != m_available_files.end())
                {
                    type = Uploader;
                    break;
                }
            }

            TLOG() << "debug : creating session " << notif.m_target_id << " type " << magic_enum::enum_name(type);
            std::string group_id_tmp = metadata.get_group_id();
            create_session(std::move(metadata), type, notif.m_target_id, get_listening_dir().append(group_id_tmp));
            break;
        }

        case e_notification_type::TRANSFER_METADATA:
        {
            TransferMetadata fmeta = TransferMetadata(notif.m_data, false);
            auto &sessions_ref = get_sessions();

            // print sessions names
            for (auto &s : sessions_ref)
            {
                TLOG() << "debug : session " << s.first;
                if (s.first.find(notif.m_target_id) != std::string::npos)
                {
                    TLOG() << "debug : session found";
                    fmeta.set_dest(s.second->get_ip());
                    s.second->add_file(std::move(fmeta));
                    return true;
                }
            }

            ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));

            break;
        }

        case e_notification_type::START_TRANSFER:
        {
            TLOG() << "debug : starting transfer " << notif.m_target_id;
            auto sessions_ref = get_sessions();
            if (sessions_ref.find(notif.m_target_id) != sessions_ref.end())
            {
                if (notif.m_data == "")
                {
                    get_session(notif.m_target_id)->start_all();
                }
                else
                {
                    get_session(notif.m_target_id)->start_file(get_session(notif.m_target_id)->get_transfer_options().get_transfer_meta_from_file_path(notif.m_data));
                }
            }
            else
            {
                ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));
            }
            break;
        }

        case e_notification_type::PAUSE_TRANSFER:
        {
            TLOG() << "debug : pausing transfer " << notif.m_target_id;
            auto sessions_ref = get_sessions();
            if (sessions_ref.find(notif.m_target_id) != sessions_ref.end())
            {
                if (notif.m_data == "")
                {
                    get_session(notif.m_target_id)->pause_all();
                }
                else
                {
                    get_session(notif.m_target_id)->pause_file(get_session(notif.m_target_id)->get_transfer_options().get_transfer_meta_from_file_path(notif.m_data));
                }
            }
            else
            {
                ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));
            }
            break;
            break;
        }

        case e_notification_type::RESUME_TRANSFER:
        {
            TLOG() << "debug : resuming transfer " << notif.m_target_id;
            auto sessions_ref = get_sessions();
            if (sessions_ref.find(notif.m_target_id) != sessions_ref.end())
            {
                if (notif.m_data == "")
                {
                    get_session(notif.m_target_id)->resume_all();
                }
                else
                {
                    get_session(notif.m_target_id)->resume_file(get_session(notif.m_target_id)->get_transfer_options().get_transfer_meta_from_file_path(notif.m_data));
                }
            }
            else
            {
                ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));
            }
            break;
        }

        case e_notification_type::CANCEL_TRANSFER:
        {
            TLOG() << "debug : cancelling transfer " << notif.m_target_id;
            auto sessions_ref = get_sessions();
            if (sessions_ref.find(notif.m_target_id) != sessions_ref.end())
            {
                if (notif.m_data == "")
                {
                    get_session(notif.m_target_id)->cancel_all();
                }
                else
                {
                    get_session(notif.m_target_id)->cancel_file(get_session(notif.m_target_id)->get_transfer_options().get_transfer_meta_from_file_path(notif.m_data));
                }
            }
            else
            {
                ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));
            }
            break;
            break;
        }

        case e_notification_type::UPDATE_REQUEST:
        {
            TLOG() << "debug : updating grp transfer for " << notif.m_target_id;
            auto sessions_ref = get_sessions();
            if (sessions_ref.find(notif.m_target_id) != sessions_ref.end())
            {
                if (notif.m_data == "")
                {
                    get_session(notif.m_target_id)->update_metadatas_to_bookkeeper();
                }
                else
                {
                    get_session(notif.m_target_id)->update_metadata_to_bookkeeper(get_session(notif.m_target_id)->get_transfer_options().get_transfer_meta_from_file_path(notif.m_data));
                }
            }
            else
            {
                ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));
            }
            break;
            break;
        }

        default:
            ers::warning(NotHandledNotificationError(ERS_HERE, get_client_id(), notif.m_source_id, notif.m_notification));
        }
        return true;
    }

    TransferSession *TransferClient::add_session(TransferSession *session)
    {
        m_sessions[session->get_session_id()] = session;
        return m_sessions[session->get_session_id()];
    }

    void TransferClient::remove_session(const std::string &session_id)
    {
        TransferSession *ses = get_session(session_id);
        m_sessions.erase(session_id);
        delete ses;
    }

    std::string TransferClient::generate_session_id(const std::string &transferid, const std::string &dest_id /*= ""*/)
    {
        std::string id = "";

        id += dest_id == "" ? get_client_id() : dest_id;
        id += "_ses";
        id += transferid;

        return id;
    }

    void TransferClient::scan_available_files(std::set<std::filesystem::path> &previous_scan, bool nested, std::filesystem::path folder)
    {
        if (folder.empty())
        {
            folder = get_listening_dir();
        }

        TLOG() << "debug : scanning files in " << folder;

        // First scan group metadata files
        for (const auto &entry : std::filesystem::directory_iterator(folder))
        {
            if (entry.is_regular_file() && entry.path().extension() == GroupMetadata::m_file_extension)
            {
                if (previous_scan.insert(entry.path()).second)
                {
                    TLOG() << "debug : found new file " << entry.path();

                    GroupMetadata metadata = GroupMetadata(entry.path());
                    bool already_active = false;

                    for (auto &s : get_sessions())
                    {
                        if (s.second->get_transfer_options() == metadata)
                        {
                            already_active = true;
                            break;
                        }
                    }

                    if (!already_active)
                    {
                        std::string group_id_tmp = metadata.get_group_id();
                        create_session(std::move(metadata), Uploader, generate_session_id(group_id_tmp), get_listening_dir().append("ses" + m_sessions.size()));
                    }
                }
            }
        }

        for (const auto &entry : std::filesystem::directory_iterator(folder))
        {
            if (entry.is_regular_file() && entry.path().extension() != GroupMetadata::m_file_extension)
            {
                if (previous_scan.insert(entry.path()).second)
                {
                    TLOG() << "debug : found new file " << entry.path();
                    if (entry.path().extension() == TransferMetadata::m_file_extension)
                    {
                        TransferMetadata metadata = TransferMetadata(entry.path());

                        bool wanted = false;
                        for (auto &s : get_sessions())
                        {
                            auto expect_ref = s.second->get_transfer_options().get_expected_files();
                            if (expect_ref.find(metadata.get_file_name()) != expect_ref.end())
                            {
                                s.second->add_file(std::move(metadata));
                                wanted = true;
                                break;
                            }
                        }
                        if (!wanted)
                        {
                            // If not wanted yet (or never), delete the file from the scan list to find it again later
                            TLOG() << "debug : " << entry.path() << " not wanted yet, deleting from scan list";
                            previous_scan.erase(entry.path());
                        }
                    }
                }
            }
            else if (entry.is_directory())
            {
                TLOG() << "debug : found directory " << entry.path();
                if (nested)
                {
                    scan_available_files(previous_scan, nested, entry.path());
                }
            }
        }
    }

    TransferMetadata TransferClient::create_metadata_from_file(const std::filesystem::path &src)
    {
        return TransferMetadata(src, std::filesystem::file_size(src), get_ip());
    }

    std::string TransferClient::get_my_conn()
    {
        if (m_my_conn.empty())
        {
            for (std::string c : get_clients_conn())
            {
                if (c.find(get_client_id()) != std::string::npos)
                {
                    m_my_conn = c;
                    break;
                }
            }

            if (m_my_conn.empty())
            {
                ers::warning(ConnectionNotFoundError(ERS_HERE, get_client_id()));
            }
        }
        return m_my_conn;
    }
} // namespace dunedaq::snbmodules