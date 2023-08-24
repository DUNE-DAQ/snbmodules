
/**
 * @file transfer_session.cpp TransferSession class, wrapper to get access to the transfer interface and control states of transfers
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/transfer_session.hpp"

#include <string>
#include <set>
#include <vector>
#include <utility>
#include <memory>

namespace dunedaq::snbmodules
{

    TransferSession::TransferSession(const GroupMetadata &transfer_options, e_session_type type, std::string id, const IPFormat &ip, std::filesystem::path work_dir, std::vector<std::string> bk_conn /*= std::vector<std::string>()*/, std::set<std::string> client_conn /*= std::set<std::string>()*/)
        : NotificationInterface(std::move(bk_conn), std::move(client_conn)),
          m_type(type),
          m_session_id(std::move(id)),
          m_ip(ip),
          m_transfer_options(transfer_options),
          //   m_threads(std::vector<pid_t>()),
          m_work_dir(std::move(work_dir))
    {
        std::filesystem::create_directories(m_work_dir);

        // Init transfer interface with the right protocol
        switch (m_transfer_options.get_protocol())
        {
        case BITTORRENT:

            // check if port is set
            if (!m_transfer_options.get_protocol_options().contains("port"))
            {
                ers::fatal(ConfigError(ERS_HERE, "port is mandatory in Bittorrent protocol options"));
                return;
            }

            m_transfer_interface = std::make_unique<TransferInterfaceBittorrent>(m_transfer_options, type == e_session_type::Downloader, get_work_dir(), get_ip());

            // Generate torrent files and magnet links
            if (type == e_session_type::Uploader)
            {
                TLOG() << "Generating torrent files...";
                dynamic_cast<TransferInterfaceBittorrent &>(*m_transfer_interface).generate_torrents_files(m_work_dir, "");

                for (TransferMetadata &f_meta : m_transfer_options.get_transfers_meta())
                {
                    TLOG() << "Writing magnet link data into transfer Metadata " << get_work_dir().append(f_meta.get_file_name() + ".torrent");
                    lt::error_code ec;
                    lt::torrent_info t(get_work_dir().append(f_meta.get_file_name() + ".torrent").string(), ec);

                    if (ec)
                    {
                        ers::error(BittorrentInvalidTorrentFileError(ERS_HERE, ec.message()));
                    }

                    TLOG() << "Magnet link: " << lt::make_magnet_uri(t);
                    f_meta.set_magnet_link(lt::make_magnet_uri(t) + "&x.pe=" + get_ip().get_ip() + ":" + m_transfer_options.get_protocol_options()["port"].get<std::string>());
                }
            }
            break;

        case SCP:
            m_transfer_interface = std::make_unique<TransferInterfaceSCP>(m_transfer_options, type == e_session_type::Uploader);
            break;

        case RCLONE:
        {
            m_transfer_interface = std::make_unique<TransferInterfaceRClone>(m_transfer_options, get_work_dir());
            break;
        }

        case dummy:
            m_transfer_interface = std::make_unique<TransferInterfaceDummy>(m_transfer_options);
            break;

        default:
            ers::error(InvalidProtocolError(ERS_HERE, get_session_id(), static_cast<std::string>(magic_enum::enum_name(m_transfer_options.get_protocol()))));
            break;
        }

        TLOG() << "debug : Transfer session " << get_session_id() << " created";
        update_metadatas_to_bookkeeper();
    }

    TransferSession::~TransferSession()
    {
        // TLOG() << "Reaping children";
        // for (pid_t pid : m_threads)
        // {
        //     TLOG() << "Killing pid " << pid;
        //     kill(pid, SIGINT);
        //     kill(pid, SIGKILL);
        // }
        // for (pid_t pid : m_threads)
        // {
        //     TLOG() << "Waiting for pid " << pid;
        //     siginfo_t status;
        //     auto sts = waitid(P_PID, pid, &status, WEXITED);

        //     TLOG() << "Forked process " << pid << " exited with status " << status.si_status << " (wait status " << sts
        //            << ")";
        // }

        TLOG() << "DONE CLOSING SESSION " << get_session_id();
    }

    bool TransferSession::action_on_receive_notification(NotificationData notif)
    {
        (void)notif;
        // TODO : now in client
        return true;
    }

    std::string TransferSession::to_string() const
    {
        std::string str;
        str += "Session " + get_session_id() + " ";
        str += "type " + static_cast<std::string>(magic_enum::enum_name(m_type)) + " ";
        str += "listening on " + m_ip.get_ip_port() + "\n";

        str += m_transfer_options.to_string();

        return str;
    }

    bool TransferSession::update_metadatas_to_bookkeeper()
    {
        bool result = true;
        for (const std::string &bk : get_bookkeepers_conn())
        {
            result = result && send_notification(e_notification_type::GROUP_METADATA, get_session_id(), bk, bk, get_transfer_options().export_to_string());
        }

        for (TransferMetadata &f_meta : m_transfer_options.get_transfers_meta())
        {
            result = result && update_metadata_to_bookkeeper(f_meta);
        }

        return result;
    }

    bool TransferSession::update_metadata_to_bookkeeper(TransferMetadata &f_meta)
    {
        bool result = true;
        for (const std::string &bk : get_bookkeepers_conn())
        {
            result = result && send_notification(e_notification_type::TRANSFER_METADATA, get_session_id(), bk, bk, f_meta.export_to_string_partial(false));
        }
        return result;
    }

    bool TransferSession::send_notification_to_targets(e_notification_type type, const std::string &data /*= ""*/)
    {
        bool result = true;
        for (const std::string &client : get_target_clients())
        {
            std::string session_name = client + "_ses" + m_transfer_options.get_group_id();
            result &= send_notification(type, get_session_id(), session_name, client, data);
        }
        return result;
    }

    bool TransferSession::start_file(TransferMetadata &f_meta)
    {
        bool res = false;
        if (is_downloader())
        {
            res = download_file(f_meta, m_work_dir);
        }
        else if (is_uploader())
        {
            res = upload_file(f_meta);
        }
        else
        {
            ers::error(SessionTypeNotSupportedError(ERS_HERE, get_session_id()));
        }

        return res;
    }

    bool TransferSession::pause_file(TransferMetadata &f_meta, bool is_multiple)
    {
        if (f_meta.get_status() != e_status::DOWNLOADING && f_meta.get_status() != e_status::UPLOADING)
        {
            ers::warning(SessionWrongStateTransitionError(ERS_HERE, get_session_id(), f_meta.get_file_name(), static_cast<std::string>(magic_enum::enum_name(f_meta.get_status())), static_cast<std::string>(magic_enum::enum_name(e_status::PAUSED))));
            return false;
        }

        bool res = m_transfer_interface->pause_file(f_meta);
        if (res)
        {
            f_meta.set_status(e_status::PAUSED);
        }
        else
        {
            f_meta.set_status(e_status::ERROR);
        }

        if (!is_multiple)
        {
            send_notification_to_targets(e_notification_type::PAUSE_TRANSFER, f_meta.get_file_path());
            update_metadata_to_bookkeeper(f_meta);
        }

        return res;
    }

    bool TransferSession::resume_file(TransferMetadata &f_meta, bool is_multiple)
    {
        if (f_meta.get_status() != e_status::PAUSED)
        {
            ers::warning(SessionWrongStateTransitionError(ERS_HERE, get_session_id(), f_meta.get_file_name(), static_cast<std::string>(magic_enum::enum_name(f_meta.get_status())), "RESUMING"));
            return false;
        }

        bool res = m_transfer_interface->resume_file(f_meta);
        if (res)
        {
            if (is_downloader())
            {
                f_meta.set_status(e_status::DOWNLOADING);
            }
            else if (is_uploader())
            {
                f_meta.set_status(e_status::UPLOADING);
            }
        }
        else
        {
            f_meta.set_status(e_status::ERROR);
        }

        if (!is_multiple)
        {
            send_notification_to_targets(e_notification_type::RESUME_TRANSFER, f_meta.get_file_path());
            update_metadata_to_bookkeeper(f_meta);
        }

        return res;
    }

    bool TransferSession::hash_file(TransferMetadata &f_meta, bool is_multiple)
    {
        if (f_meta.get_status() != e_status::FINISHED)
        {
            ers::warning(SessionWrongStateTransitionError(ERS_HERE, get_session_id(), f_meta.get_file_name(), static_cast<std::string>(magic_enum::enum_name(f_meta.get_status())), static_cast<std::string>(magic_enum::enum_name(e_status::HASHING))));
            return false;
        }

        bool res = m_transfer_interface->hash_file(f_meta);
        if (res)
        {
            f_meta.set_status(e_status::HASHING);
        }
        else
        {
            f_meta.set_status(e_status::ERROR);
        }

        if (!is_multiple)
        {
            // send_notification_to_targets(e_notification_type::HASH_TRANSFER, f_meta.get_file_path());
            update_metadata_to_bookkeeper(f_meta);
        }
        return res;
    }

    bool TransferSession::cancel_file(TransferMetadata &f_meta, bool is_multiple)
    {
        if (f_meta.get_status() == e_status::FINISHED || f_meta.get_status() == e_status::CANCELLED)
        {
            ers::warning(SessionWrongStateTransitionError(ERS_HERE, get_session_id(), f_meta.get_file_name(), static_cast<std::string>(magic_enum::enum_name(f_meta.get_status())), static_cast<std::string>(magic_enum::enum_name(e_status::CANCELLED))));
            return false;
        }

        bool res = m_transfer_interface->cancel_file(f_meta);
        if (res)
        {
            f_meta.set_status(e_status::CANCELLED);
        }
        else
        {
            f_meta.set_status(e_status::ERROR);
        }

        if (!is_multiple)
        {
            send_notification_to_targets(e_notification_type::CANCEL_TRANSFER, f_meta.get_file_path());
            update_metadata_to_bookkeeper(f_meta);
        }
        return res;
    }

    bool TransferSession::upload_file(TransferMetadata &f_meta, bool is_multiple)
    {
        if (m_type != e_session_type::Uploader)
        {
            ers::warning(SessionAccessToIncorrectActionError(ERS_HERE, get_session_id(), "upload_file"));
        }

        if (f_meta.get_status() != e_status::WAITING)
        {
            ers::warning(SessionWrongStateTransitionError(ERS_HERE, get_session_id(), f_meta.get_file_name(), static_cast<std::string>(magic_enum::enum_name(f_meta.get_status())), static_cast<std::string>(magic_enum::enum_name(e_status::UPLOADING))));
            return false;
        }

        bool res = m_transfer_interface->upload_file(f_meta);
        if (res)
        {
            f_meta.set_status(e_status::UPLOADING);
        }
        else
        {
            f_meta.set_status(e_status::ERROR);
        }

        if (!is_multiple)
        {
            send_notification_to_targets(e_notification_type::START_TRANSFER, f_meta.get_file_path());
            update_metadata_to_bookkeeper(f_meta);
        }
        return res;
    }

    bool TransferSession::download_file(TransferMetadata &f_meta, std::filesystem::path dest, bool is_multiple)
    {
        if (m_type != e_session_type::Downloader)
        {
            ers::warning(SessionAccessToIncorrectActionError(ERS_HERE, get_session_id(), "download_file"));
        }

        if (f_meta.get_status() != e_status::WAITING)
        {
            ers::warning(SessionWrongStateTransitionError(ERS_HERE, get_session_id(), f_meta.get_file_name(), static_cast<std::string>(magic_enum::enum_name(f_meta.get_status())), static_cast<std::string>(magic_enum::enum_name(e_status::DOWNLOADING))));
            return false;
        }

        // wait for the uploader to be ready
        std::this_thread::sleep_for(std::chrono::seconds(1));

        bool res = m_transfer_interface->download_file(f_meta, std::move(dest));
        if (res)
        {
            f_meta.set_status(e_status::DOWNLOADING);
        }
        else
        {
            f_meta.set_status(e_status::ERROR);
        }
        if (!is_multiple)
        {
            update_metadata_to_bookkeeper(f_meta);
        }
        return res;
    }

    bool TransferSession::start_all()
    {
        if (is_downloader())
        {
            return download_all(m_work_dir);
        }
        else if (is_uploader())
        {
            return upload_all();
        }
        else
        {
            ers::error(SessionTypeNotSupportedError(ERS_HERE, get_session_id()));
            return false;
        }
    }

    bool TransferSession::pause_all()
    {

        send_notification_to_targets(e_notification_type::PAUSE_TRANSFER);

        // wait 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));

        bool result = true;
        for (auto file : m_transfer_options.get_transfers_meta())
        {
            result = result && pause_file(file, true);
        }

        update_metadatas_to_bookkeeper();
        return result;
    }

    bool TransferSession::resume_all()
    {
        bool result = true;
        for (auto file : m_transfer_options.get_transfers_meta())
        {
            result = result && resume_file(file, true);
        }

        // wait 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));

        send_notification_to_targets(e_notification_type::RESUME_TRANSFER);
        update_metadatas_to_bookkeeper();
        return result;
    }

    bool TransferSession::cancel_all()
    {
        bool result = true;
        for (auto file : m_transfer_options.get_transfers_meta())
        {
            result = result && cancel_file(file, true);
        }

        send_notification_to_targets(e_notification_type::CANCEL_TRANSFER);
        update_metadatas_to_bookkeeper();
        return result;
    }

    // Downloaders only
    bool TransferSession::download_all(const std::filesystem::path &dest)
    {
        if (m_type != e_session_type::Downloader)
        {
            ers::warning(SessionAccessToIncorrectActionError(ERS_HERE, get_session_id(), "download_all"));
        }

        bool result = true;
        for (auto &file : m_transfer_options.get_transfers_meta())
        {
            result = result && download_file(file, dest, true);
        }
        update_metadatas_to_bookkeeper();
        return result;
    }

    // Uploaders only
    bool TransferSession::upload_all()
    {
        if (m_type != e_session_type::Uploader)
        {
            ers::warning(SessionAccessToIncorrectActionError(ERS_HERE, get_session_id(), "upload_all"));
        }

        bool result = true;
        for (auto &file : m_transfer_options.get_transfers_meta())
        {
            result = result && upload_file(file, true);
        }
        send_notification_to_targets(e_notification_type::START_TRANSFER);
        update_metadatas_to_bookkeeper();
        return result;
    }

} // namespace dunedaq::snbmodules