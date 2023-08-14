
#include "snbmodules/notification_interface.hpp"

#include "snbmodules/tools/magic_enum.hpp"
#include "snbmodules/common/errors_declaration.hpp"

#include <iostream>
#include <string>

namespace dunedaq::snbmodules
{

    std::optional<NotificationData> NotificationInterface::listen_for_notification(std::string id, std::string expected_from, int timeout, int tries)
    {
        // Default value for tries
        if (tries == -1)
        {
            tries = m_max_tries;
        }
        // TLOG() << "debug : Listening for request from " << id;

        if (timeout == -1)
        {
            timeout = m_timeout_receive;
        }

        std::optional<NotificationData> msg = iomanager::IOManager::get()
                                                  ->get_receiver<NotificationData>(id)
                                                  ->try_receive(std::chrono::milliseconds(timeout));

        if (msg.has_value())
        {
            TLOG() << "debug : Received request " << msg->m_notification << " for " << msg->m_target_id;

            if (expected_from != "" && expected_from.find(msg.value().m_source_id) == std::string::npos)
            {
                TLOG() << "debug : Received request from " << msg->m_source_id << " but expected from " << expected_from << " ignoring";
                if (tries <= 1)
                {
                    return std::nullopt;
                }
                return listen_for_notification(id, expected_from, timeout, --tries);
            }
        }
        return msg;
    }

    bool NotificationInterface::send_notification(e_notification_type notif, std::string src, std::string dst, std::string id_conn, std::string data, int tries)
    {
        // Default value for tries
        if (tries == -1)
        {
            tries = m_max_tries;
        }

        // find connection with dst in it
        for (auto conn : m_bookkeepers_conn)
        {
            if (conn.find(id_conn) != std::string::npos)
            {
                id_conn = conn;
                break;
            }
        }
        for (auto conn : m_clients_conn)
        {
            if (conn.find(id_conn) != std::string::npos)
            {
                id_conn = conn;
                break;
            }
        }

        TLOG() << "debug : Sending request " << magic_enum::enum_name(notif) << " to " << dst << " via " << id_conn;

        NotificationData notif_data(src, dst, static_cast<std::string>(magic_enum::enum_name(notif)), data);

        bool result = iomanager::IOManager::get()
                          ->get_sender<NotificationData>(id_conn)
                          ->try_send(std::move(notif_data), std::chrono::milliseconds(m_timeout_send));

        if (result == false)
        {
            ers::error(NotificationSendError(ERS_HERE, id_conn));
            if (tries <= 1)
            {
                return false;
            }

            // wait
            std::this_thread::sleep_for(std::chrono::milliseconds(m_timeout_send));
            TLOG() << "debug : Retrying send notification";
            return send_notification(notif, src, dst, id_conn, data, --tries);
        }

        return result;
    }
} // namespace dunedaq::snbmodules