
#include "snbmodules/transfer_client.hpp"

namespace dunedaq::snbmodules
{
    TransferClient::TransferClient(IPFormat listening_ip, std::string client_id, std::filesystem::path listening_dir, std::string connection_prefix, int timeout_send, int timeout_receive)
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
        for (auto &s : m_sessions)
        {
            delete s.second;
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
            for (auto &session : m_sessions)
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
                for (auto &session : m_sessions)
                {
                    TLOG() << session.second->to_string();
                }
            }
        }

        return true;
    }

    void TransferClient::create_new_transfer(std::string transfer_id, std::string protocol, std::set<std::string> dest_clients, std::set<std::filesystem::path> files, const nlohmann::json &protocol_options)
    {

        std::string session_name = get_client_id() + "_ses" + transfer_id;

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

        for (auto file : files)
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
                group_transfer.add_file(create_metadata_from_file(file));
            }
        }

        if (group_transfer.get_transfers_meta().empty())
        {
            ers::error(FileForTransferNotExistError(ERS_HERE, get_client_id(), "All files"));
            return;
        }

        // Sending to bookkeepers to update preparing state
        for (auto bk : get_bookkeepers_conn())
        {
            send_notification(e_notification_type::GROUP_METADATA, session_name, bk, bk, group_transfer.export_to_string());
        }

        // Create local session, can take time depending on protocol
        create_session(&group_transfer, e_session_type::Uploader, session_name, get_listening_dir().append(transfer_id), m_listening_ip, dest_clients);

        // Notify clients and sending group metadata
        for (auto client : dest_clients)
        {
            std::string new_session_name = client + "_ses" + transfer_id;
            TLOG() << "debug : notifying client " << client;
            send_notification(e_notification_type::NEW_TRANSFER, get_client_id(), new_session_name, client, group_transfer.export_to_string());

            // Send transfer metadata
            for (auto file : group_transfer.get_transfers_meta())
            {
                send_notification(e_notification_type::TRANSFER_METADATA, get_client_id(), new_session_name, client, file->export_to_string());
            }
        }
    }

    void TransferClient::start_transfer(std::string transfer_id)
    {

        TransferSession *session = get_session(transfer_id);

        if (session == nullptr)
        {
            ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), transfer_id));
            return;
        }

        session->start_all();
    }

    void TransferClient::pause_transfer(std::string transfer_id)
    {

        TransferSession *session = get_session(transfer_id);

        if (session == nullptr)
        {
            ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), transfer_id));
            return;
        }

        session->pause_all();
    }
    void TransferClient::resume_transfer(std::string transfer_id)
    {

        TransferSession *session = get_session(transfer_id);

        if (session == nullptr)
        {
            ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), transfer_id));
            return;
        }

        session->resume_all();
    }

    void TransferClient::cancel_transfer(std::string transfer_id)
    {

        TransferSession *session = get_session(transfer_id);

        if (session == nullptr)
        {
            ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), transfer_id));
            return;
        }

        session->cancel_all();
    }

    TransferSession *TransferClient::get_session(std::string transfer_id)
    {
        return get_sessions()[get_client_id() + "_ses" + transfer_id];
    }

    // TODO ip useless ?
    TransferSession *TransferClient::create_session(GroupMetadata *transfer_options, e_session_type type, std::string id, std::filesystem::path work_dir, IPFormat ip, std::set<std::string> dest_clients)
    {

        if (ip.is_default())
        {
            ip = get_ip();
        }

        TransferSession *new_session = new TransferSession(transfer_options, type, id, ip, work_dir, get_bookkeepers_conn(), get_clients_conn());
        new_session->set_target_clients(dest_clients);
        add_sessions(new_session);

        TLOG() << "debug : session created " << magic_enum::enum_name(type);
        return new_session;
    }

    void TransferClient::share_available_files(std::set<std::filesystem::path> *to_share, std::string dest)
    {

        for (std::filesystem::path f : *to_share)
        {
            TLOG() << "debug : Sharing " << f.filename();
            TransferMetadata *fmeta = create_metadata_from_file(f);
            m_available_files.insert({f.string(), *fmeta});
            this->send_notification(e_notification_type::TRANSFER_METADATA, get_client_id(), dest, dest, fmeta->export_to_string());
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
            scan_available_files(&to_share);
            share_available_files(&to_share, notif.m_source_id);
            send_notification(e_notification_type::TRANSFER_METADATA, get_client_id(), notif.m_source_id, notif.m_source_id, "end");
            break;
        }

        case e_notification_type::NEW_TRANSFER:
        {
            GroupMetadata *metadata = new GroupMetadata(notif.m_data, false);
            e_session_type type = Downloader;

            // If file is available, create a downloader session
            for (auto f : metadata->get_expected_files())
            {
                if (m_available_files.find(f) != m_available_files.end())
                {
                    type = Uploader;
                    break;
                }
            }

            TLOG() << "debug : creating session " << notif.m_target_id << " type " << magic_enum::enum_name(type);
            create_session(metadata, type, notif.m_target_id, get_listening_dir().append(metadata->get_group_id()));
            break;
        }

        case e_notification_type::TRANSFER_METADATA:
        {
            TransferMetadata *fmeta = new TransferMetadata(notif.m_data, false);

            if (get_sessions()[notif.m_target_id] != nullptr)
            {
                fmeta->set_dest(get_sessions()[notif.m_target_id]->get_ip());
                get_sessions()[notif.m_target_id]->add_file(fmeta);
            }
            else
                ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));
            break;
            break;
        }

        case e_notification_type::START_TRANSFER:
        {
            TLOG() << "debug : starting transfer " << notif.m_target_id;
            if (get_sessions()[notif.m_target_id] != nullptr)
            {
                if (notif.m_data == "")
                    get_sessions()[notif.m_target_id]->start_all();
                else
                    get_sessions()[notif.m_target_id]->start_file(get_sessions()[notif.m_target_id]->get_transfer_options().get_transfer_meta_from_file_path(notif.m_data));
            }
            else
                ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));
            break;
        }

        case e_notification_type::PAUSE_TRANSFER:
        {
            TLOG() << "debug : pausing transfer " << notif.m_target_id;
            if (get_sessions()[notif.m_target_id] != nullptr)
            {
                if (notif.m_data == "")
                    get_sessions()[notif.m_target_id]->pause_all();
                else
                    get_sessions()[notif.m_target_id]->pause_file(get_sessions()[notif.m_target_id]->get_transfer_options().get_transfer_meta_from_file_path(notif.m_data));
            }
            else
                ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));
            break;
            break;
        }

        case e_notification_type::RESUME_TRANSFER:
        {
            TLOG() << "debug : resuming transfer " << notif.m_target_id;
            if (get_sessions()[notif.m_target_id] != nullptr)
            {
                if (notif.m_data == "")
                    get_sessions()[notif.m_target_id]->resume_all();
                else
                    get_sessions()[notif.m_target_id]->resume_file(get_sessions()[notif.m_target_id]->get_transfer_options().get_transfer_meta_from_file_path(notif.m_data));
            }
            else
                ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));
            break;
        }

        case e_notification_type::CANCEL_TRANSFER:
        {
            TLOG() << "debug : cancelling transfer " << notif.m_target_id;
            if (get_sessions()[notif.m_target_id] != nullptr)
            {
                if (notif.m_data == "")
                    get_sessions()[notif.m_target_id]->cancel_all();
                else
                    get_sessions()[notif.m_target_id]->cancel_file(get_sessions()[notif.m_target_id]->get_transfer_options().get_transfer_meta_from_file_path(notif.m_data));
            }
            else
                ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));
            break;
            break;
        }

        case e_notification_type::UPDATE_REQUEST:
        {
            TLOG() << "debug : updating grp transfer for " << notif.m_target_id;
            if (get_sessions()[notif.m_target_id] != nullptr)
            {
                if (notif.m_data == "")
                    get_sessions()[notif.m_target_id]->update_metadatas_to_bookkeeper();
                else
                    get_sessions()[notif.m_target_id]->update_metadata_to_bookkeeper(get_sessions()[notif.m_target_id]->get_transfer_options().get_transfer_meta_from_file_path(notif.m_data));
            }
            else
                ers::warning(SessionIDNotFoundInClientError(ERS_HERE, get_client_id(), notif.m_target_id));
            break;
            break;
        }

        default:
            ers::warning(NotHandledNotificationError(ERS_HERE, get_client_id(), notif.m_source_id, notif.m_notification));
        }
        return true;
    }

    void TransferClient::add_sessions(TransferSession *session)
    {
        m_sessions[session->get_session_id()] = session;
    }

    void TransferClient::remove_session(std::string session_id)
    {
        m_sessions.erase(session_id);
    }

    std::string TransferClient::generate_session_id()
    {
        std::string id = "";

        id += this->get_client_id();
        id += "_";
        id += this->get_sessions().size();

        return id;
    }

    void TransferClient::scan_available_files(std::set<std::filesystem::path> *previous_scan, std::filesystem::path folder, bool nested)
    {
        if (folder.empty())
        {
            folder = this->get_listening_dir();
        }

        TLOG() << "debug : scanning files in " << folder;

        // First scan group metadata files
        for (const auto &entry : std::filesystem::directory_iterator(folder))
        {
            if (entry.is_regular_file() && entry.path().extension() == GroupMetadata::m_file_extension)
            {
                if (previous_scan->insert(entry.path()).second)
                {
                    TLOG() << "debug : found new file " << entry.path();

                    GroupMetadata *metadata = new GroupMetadata(entry.path());
                    bool already_active = false;

                    for (auto s : this->get_sessions())
                    {
                        if (s.second->get_transfer_options() == *metadata)
                        {
                            already_active = true;
                            break;
                        }
                    }

                    if (!already_active)
                    {
                        create_session(metadata, Uploader, generate_session_id(), get_listening_dir().append("ses" + m_sessions.size()));
                    }
                    else
                    {
                        delete metadata;
                    }
                }
            }
            else if (entry.is_directory())
            {
                TLOG() << "debug : found directory " << entry.path();
                if (nested)
                    scan_available_files(previous_scan, entry.path(), nested);
            }
        }

        for (const auto &entry : std::filesystem::directory_iterator(folder))
        {
            if (entry.is_regular_file() && entry.path().extension() != GroupMetadata::m_file_extension)
            {
                if (previous_scan->insert(entry.path()).second)
                {
                    TLOG() << "debug : found new file " << entry.path();
                    if (entry.path().extension() == TransferMetadata::m_file_extension)
                    {
                        TransferMetadata *metadata = new TransferMetadata(entry.path());

                        bool wanted = false;
                        for (auto s : this->get_sessions())
                        {
                            if (s.second->get_transfer_options().get_expected_files().find(metadata->get_file_name()) != s.second->get_transfer_options().get_expected_files().end())
                            {
                                s.second->add_file(metadata);
                                wanted = true;
                                break;
                            }
                        }
                        if (!wanted)
                        {
                            // If not wanted yet (or never), delete the file from the scan list to find it again later
                            TLOG() << "debug : " << entry.path() << " not wanted yet, deleting from scan list";
                            previous_scan->erase(entry.path());
                            delete metadata;
                        }
                    }
                }
            }
            else if (entry.is_directory())
            {
                TLOG() << "debug : found directory " << entry.path();
                if (nested)
                    scan_available_files(previous_scan, entry.path(), nested);
            }
        }
    }

    TransferMetadata *TransferClient::create_metadata_from_file(std::filesystem::path src)
    {
        return new TransferMetadata(src, std::filesystem::file_size(src), this->get_ip());
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