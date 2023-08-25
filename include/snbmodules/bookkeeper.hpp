/**
 * @file bookkeeper.hpp Bookkeeper class retriving informations from clients
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_INCLUDE_SNBMODULES_BOOKKEEPER_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_BOOKKEEPER_HPP_

#include "snbmodules/group_metadata.hpp"
#include "snbmodules/common/protocols_enum.hpp"
#include "snbmodules/ip_format.hpp"
#include "snbmodules/notification_interface.hpp"
#include "snbmodules/common/status_enum.hpp"

// errors handling
#include "snbmodules/common/errors_declaration.hpp"
#include "logging/Logging.hpp"

#include <string>
#include <algorithm>
#include <chrono>
#include <map>
#include <set>
#include <vector>
#include <utility>

namespace dunedaq::snbmodules
{
    class Bookkeeper : public NotificationInterface
    {

    public:
        /// @brief Constructor with params
        Bookkeeper(const IPFormat &listening_ip, std::string bookkeeper_id, std::string file_log_path = "", int refresh_rate = 5, std::string connection_prefix = "snbmodules", int timeout_send = 10, int timeout_receive = 100)
            : NotificationInterface(std::move(connection_prefix), timeout_send, timeout_receive)
        {
            set_bookkeeper_id(std::move(bookkeeper_id));
            set_ip(listening_ip);
            m_file_log_path = std::move(file_log_path);
            m_refresh_rate = refresh_rate;
            display_information();
        }

        /// @brief Destructor
        ~Bookkeeper()
        {
            for (auto &grp_transfer : m_grp_transfers)
            {
                delete grp_transfer.second;
            }

            for (auto &transfer : m_transfers)
            {
                for (auto &file : transfer.second)
                {
                    delete file;
                }
            }
        }

        /// @brief Start the bookkeeper, Only used for stand alone application
        /// @return  False if the bookkeeper did not stopped correctly
        bool start();

        /// @brief Start the bookkeeper thread to receive notifications
        /// @param running_flag stop Flag to stop the thread
        void do_work(std::atomic<bool> &running_flag);

        bool action_on_receive_notification(NotificationData notif) override;

        /// @brief Do action depending on the input, Used for stand alone application TODO: remake
        /// @param input Input from the user
        void input_action(char input);

        [[deprecated("Now only the uploader can create a new transfer")]] void create_new_transfer(const std::string &protocol, const std::string &src, const std::set<std::string> &dests, const std::set<std::filesystem::path> &files, const nlohmann::json &protocol_options = nlohmann::json());

        /// @brief Display the information of the bookkeeper either on the normal log or on a specific file depending on the value of m_file_log_path
        void display_information();

        /// @brief Request the update of the metadata from every known clients to the bookkeeper.
        /// Only get from group transfers that are in a dynamic state (downloading, checking, uploading...)
        /// @param force Force the update of the metadata even if the transfer is not in a dynamic state
        void request_update_metadata(bool force = false);

        // Setters
        inline void set_bookkeeper_id(std::string bookkeeper_id) { m_bookkeeper_id = std::move(bookkeeper_id); }
        inline void set_ip(const IPFormat &ip) { m_ip = ip; }
        void add_update_transfer(const std::string &client_id, const std::string &data);
        void add_update_grp_transfer(GroupMetadata *grp_transfers);

        // Getters
        inline std::string get_bookkeeper_id() const { return m_bookkeeper_id; }
        inline IPFormat get_ip() const { return m_ip; }
        inline std::map<std::string, GroupMetadata *> &get_grp_transfers() { return m_grp_transfers; }
        inline const std::map<std::string, GroupMetadata *> &get_grp_transfers() const { return m_grp_transfers; }
        inline std::map<std::string, std::vector<TransferMetadata *>> &get_transfers() { return m_transfers; }
        inline const std::map<std::string, std::vector<TransferMetadata *>> &get_transfers() const { return m_transfers; }

    private:
        /// @brief Unique identifier for the bookkeeper
        std::string m_bookkeeper_id;

        /// @brief IP address of the bookkeeper
        IPFormat m_ip;

        /// @brief map of group_id -> group_transfers
        std::map<std::string, GroupMetadata *> m_grp_transfers;

        /// @brief map of grp_transfer_id -> clients_id
        std::map<std::string, std::set<std::string>> m_clients_per_grp_transfer;

        /// @brief Map of files/current transfers, client_id -> set of transfers
        std::map<std::string, std::vector<TransferMetadata *>> m_transfers;

        /// @brief should the information pannel of transfers be displayed on the normal log or a specific file
        std::string m_file_log_path = "";

        /// @brief Refresh rate of the information pannel of transfers in seconds
        int m_refresh_rate = 5;

        /// @brief Send a notification to a clients id or connection to get available files
        /// @param client client id or connection name
        void request_connection_and_available_files(const std::string &client);

        /// @brief Start a new transfer
        [[deprecated("Now only the uploader can start a transfer")]] void start_transfers(const std::string &transfer_id);

        /// @brief Usefull convertion from session id to client id
        /// @param session_name session id
        /// @return client id
        std::string get_client_name_from_session_name(const std::string &session_name) const
        {
            return session_name.substr(0, session_name.find("_"));
        }
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_BOOKKEEPER_HPP_