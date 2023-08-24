/**
 * @file notification_interface.cpp NotificationData class, NotificationInterface class, interface used by clients or bookkeepers to send/receive notifications
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/notification_interface.hpp"

#include "snbmodules/tools/magic_enum.hpp"
#include "snbmodules/common/errors_declaration.hpp"

#include <iostream>
#include <string>
#include <utility>

namespace dunedaq::snbmodules
{

    std::optional<NotificationData> NotificationInterface::listen_for_notification(const std::string &id, const std::string &expected_from /*= ""*/, int timeout /*= -1*/, int tries /*= -1*/)
    { // NOLINT
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
                tries--;
                return listen_for_notification(id, expected_from, timeout, tries);
            }
        }
        return msg;
    }

    bool NotificationInterface::send_notification(const e_notification_type &notif, const std::string &src, const std::string &dst, const std::string &id_conn, const std::string &data, int tries)
    {
        // Default value for tries
        if (tries == -1)
        {
            tries = m_max_tries;
        }

        // find connection with dst in it
        std::string real_conn_id = id_conn;
        for (const auto &conn : m_bookkeepers_conn)
        {
            if (conn.find(id_conn) != std::string::npos)
            {
                real_conn_id = conn;
                break;
            }
        }
        for (const auto &conn : m_clients_conn)
        {
            if (conn.find(id_conn) != std::string::npos)
            {
                real_conn_id = conn;
                break;
            }
        }

        TLOG() << "debug : Sending request " << magic_enum::enum_name(notif) << " to " << dst << " via " << real_conn_id;

        NotificationData notif_data(src, dst, static_cast<std::string>(magic_enum::enum_name(notif)), data);

        bool result = iomanager::IOManager::get()
                          ->get_sender<NotificationData>(real_conn_id)
                          ->try_send(std::move(notif_data), std::chrono::milliseconds(m_timeout_send));

        if (result == false)
        {
            ers::error(NotificationSendError(ERS_HERE, real_conn_id));
            if (tries <= 1)
            {
                return false;
            }

            // wait
            std::this_thread::sleep_for(std::chrono::milliseconds(m_timeout_send));
            TLOG() << "debug : Retrying send notification";
            tries--;
            return send_notification(notif, src, dst, real_conn_id, data, tries);
        }

        return result;
    }
} // namespace dunedaq::snbmodules