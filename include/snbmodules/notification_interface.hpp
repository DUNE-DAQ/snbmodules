
#ifndef SNBMODULES_INCLUDE_SNBMODULES_NOTIFICATION_INTERFACE_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_NOTIFICATION_INTERFACE_HPP_

#include "iomanager/IOManager.hpp"
#include "iomanager/network/ConfigClient.hpp"
#include "iomanager/network/NetworkIssues.hpp"

#include "snbmodules/common/notification_enum.hpp"
#include "snbmodules/ip_format.hpp"
#include "snbmodules/tools/magic_enum.hpp"
#include "snbmodules/iomanager_wrapper.hpp"

#include <string>
#include <set>
#include <vector>
#include <utility>

namespace dunedaq
{
    namespace snbmodules
    {
        /// @brief NotificationData class, represent a notification
        struct NotificationData
        {
            /// @brief Source ID
            std::string m_source_id;
            /// @brief Target ID
            std::string m_target_id;
            /// @brief Notification type
            std::string m_notification;
            /// @brief Data of the notification, can be empty
            std::string m_data;

            NotificationData() = default;
            NotificationData(std::string source_id, std::string target_id, std::string notification, std::string data = "")
                : m_source_id(std::move(source_id)), m_target_id(std::move(target_id)), m_notification(std::move(notification)), m_data(std::move(data))
            {
            }
            NotificationData(std::string source_id, std::string target_id, e_notification_type notification, std::string data = "")
                : m_source_id(std::move(source_id)), m_target_id(std::move(target_id)), m_notification(magic_enum::enum_name(notification)), m_data(std::move(data))
            {
            }
            virtual ~NotificationData() = default;
            NotificationData(NotificationData const &) = default;
            NotificationData &operator=(NotificationData const &) = default;
            NotificationData(NotificationData &&) = default;
            NotificationData &operator=(NotificationData &&) = default;

            DUNE_DAQ_SERIALIZE(NotificationData, m_source_id, m_target_id, m_notification, m_data);
        };

        class NotificationInterface
        {

        public:
            explicit NotificationInterface(std::string connection_prefix = "snbmodules", int timeout_send = 10, int timeout_receive = 100)
                : m_timeout_send(timeout_send),
                  m_timeout_receive(timeout_receive),
                  m_connection_prefix(std::move(connection_prefix))
            {
            }

            NotificationInterface(std::vector<std::string> bk_conn, std::set<std::string> client_conn, const std::string &connection_prefix = "snbmodules", int timeout_send = 10, int timeout_receive = 100)
                : m_timeout_send(timeout_send),
                  m_timeout_receive(timeout_receive),
                  m_connection_prefix(connection_prefix)
            {
                m_bookkeepers_conn = std::move(bk_conn);
                m_clients_conn = std::move(client_conn);
            }
            virtual ~NotificationInterface() = default;

            /// @brief Listen for a notification
            /// @param id  ID of the connection
            /// @param expected_from Expected source of the notification, if empty, accept any source
            /// @param timeout Timeout for receiving the notification in ms, if -1, use the default timeout
            /// @return std::optional<NotificationData> Notification received or not
            std::optional<NotificationData> listen_for_notification(const std::string &id, const std::string &expected_from = "", int timeout = -1, int tries = -1);

            /// @brief Send a notification during m_timeout_send ms
            /// @param notif   Type of the notification
            /// @param src  name of the source or uuid of the connection
            /// @param dst  name of the destination or uuid of the connection
            /// @param id_conn ID of the connection
            /// @param data Data of the notification (optional)
            /// @return
            bool send_notification(const e_notification_type &notif, const std::string &src, const std::string &dst, const std::string &id_conn, const std::string &data = "", int tries = -1);

            /// @brief Action to do when receiving a notification
            /// @param notif Notification received
            /// @return true if the notification was handled, false otherwise
            virtual bool action_on_receive_notification(NotificationData notif) = 0;

            /// @brief Get the list of every connections, must have the prefix first in the name of the connection
            /// and the name of the connection must be in the format: prefix.*bookkeeper.* or prefix.*client.*
            void lookups_connections()
            {

                iomanager::ConnectionId id = {m_connection_prefix + ".*bookkeeper.*", "notification_t", ""};

                try
                {
                    iomanager::ConnectionResponse result = IOManagerWrapper::get().lookups_connection(id, false);
                    for (const auto &conn : result.connections)
                    {
                        m_bookkeepers_conn.push_back(conn.uid);
                    }
                }
                catch (...) // Ignore if no connection found
                {
                }
                try
                {
                    id = {m_connection_prefix + ".*client.*", "notification_t", ""};
                    iomanager::ConnectionResponse result = IOManagerWrapper::get().lookups_connection(id, false);
                    for (const auto &conn : result.connections)
                    {
                        m_clients_conn.insert(conn.uid);
                    }
                }
                catch (...) // Ignore if no connection found
                {
                }
            }

            /// @brief Add a conection to the list of connections
            /// @param ip IP of the connection
            /// @param connection_name Name of the connection
            /// @param data_type Type of the data
            /// @param is_client Is the connection a client
            virtual inline void add_connection(const IPFormat &ip, const std::string &connection_name, const std::string &data_type, bool is_client)
            {
                IOManagerWrapper::get().add_connection(ip, connection_name, data_type);

                if (is_client)
                {
                    m_clients_conn.insert(connection_name);
                }
                else
                {
                    m_bookkeepers_conn.push_back(connection_name);
                }
            }

            /// @brief Init the connection interface,
            /// Only used for standalone application
            /// @param session_name Name of the session
            /// @param use_connectivity_service Use the connectivity service
            /// @param ip IP of the connectivity service
            void init_connection_interface(std::string session_name = "SNBMODULES", bool use_connectivity_service = false, const IPFormat &ip = IPFormat("localhost", 5000))
            {
                IOManagerWrapper::get().init_connection_interface(std::move(session_name), use_connectivity_service, ip);
            }

            // Getters
            inline const std::vector<std::string> &get_bookkeepers_conn() const { return m_bookkeepers_conn; }
            inline const std::set<std::string> &get_clients_conn() const { return m_clients_conn; }

        private:
            /// @brief List of bookkeepers connections
            std::vector<std::string> m_bookkeepers_conn;
            /// @brief List of clients connections
            std::set<std::string> m_clients_conn;

            /// @brief Timeout for sending a notification in ms
            int m_timeout_send = 10;
            /// @brief Timeout for receiving a notification in ms
            int m_timeout_receive = 100;
            /// @brief Prefix for the connection name
            std::string m_connection_prefix = "snbmodules";

            int m_max_tries = 3;
        };

    } // namespace snbmodules

    // Must be in dunedaq namespace only
    DUNE_DAQ_SERIALIZABLE(snbmodules::NotificationData, "notification_t");

} // namespace dunedaq

#endif // SNBMODULES_INCLUDE_SNBMODULES_NOTIFICATION_INTERFACE_HPP_