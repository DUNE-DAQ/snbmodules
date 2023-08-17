

#ifndef SNBMODULES_INCLUDE_SNBMODULES_TRANSFER_CLIENT_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_TRANSFER_CLIENT_HPP_

#include "snbmodules/transfer_session.hpp"
#include "snbmodules/ip_format.hpp"
#include "snbmodules/tools/magic_enum.hpp"
#include "snbmodules/common/notification_enum.hpp"
#include "snbmodules/notification_interface.hpp"
#include "snbmodules/iomanager_wrapper.hpp"

// errors handling
#include "snbmodules/common/errors_declaration.hpp"
#include "logging/Logging.hpp"

#include <unistd.h>
#include <string>
#include <filesystem>
#include <set>
#include <iostream>
#include <map>
#include <utility>

namespace dunedaq::snbmodules
{
    /// @brief TransferClient class, represent a client that can create session and communicate with Bookkeepers.
    /// extend NotificationInterface because the client can communicate and need to initialize the connection.
    class TransferClient : public NotificationInterface
    {

    public:
        /// @brief TransferClient constructor
        /// @param listening_ip  Listening IP address
        /// @param port Listening port
        /// @param client_id Client ID
        /// @param listening_dir Listening directory (where the client will save incoming files and files to share with bookkeepers)
        TransferClient(const IPFormat &listening_ip, const std::string &client_id, const std::filesystem::path &listening_dir, const std::string &connection_prefix = "snbmodules", int timeout_send = 10, int timeout_receive = 100);

        ~TransferClient();

        /// @brief Start the client in curent thread
        /// @param timeout  Timeout in seconds, after this time the client will stop
        /// @return  False if the client did not stopped correctly
        bool start(int timeout);

        /// @brief Start function to use in a thread
        /// @param running_flag Flag to stop the client
        /// @return False if the client did not stopped correctly
        bool do_work(std::atomic<bool> &running_flag);

        /// @brief Create a new transfer
        /// @param transfer_id ID of the transfer
        /// @param protocol Protocol to use
        /// @param dest_clients Set of destination clients
        /// @param files Set of files to transfer
        /// @param protocol_options Protocol options
        void create_new_transfer(const std::string &transfer_id, const std::string &protocol, const std::set<std::string> &dest_clients, const std::set<std::filesystem::path> &files, const nlohmann::json &protocol_options = nlohmann::json());

        /// @brief Start, pause, resume or cancel a transfer
        /// @param transfer_id ID of the transfer to start, pause, resume or cancel
        void start_transfer(const std::string &transfer_id);
        void pause_transfer(const std::string &transfer_id);
        void resume_transfer(const std::string &transfer_id);
        void cancel_transfer(const std::string &transfer_id);

        /// @brief Create a new session, you can precise the IP address of the session forcing to use a
        /// specific network interface and different port that the client
        /// @param transfer_options Transfer options for the protocol
        /// @param type Type of the session (upload or download)
        /// @param id ID of the session
        /// @param listening_ip Listening IP\:PORT address of the session
        /// @return Pointer to the new session
        TransferSession *create_session(GroupMetadata transfer_options, e_session_type type, std::string id, const std::filesystem::path &work_dir, IPFormat ip = IPFormat(), const std::set<std::string> &dest_clients = std::set<std::string>());

        /// @brief Scan available files in the listening directory
        /// @param previous_scan Set of files already scanned
        /// @param folder Folder to scan
        /// @param nested True if the function is called recursively
        void scan_available_files(std::set<std::filesystem::path> &previous_scan, bool nested = false, std::filesystem::path folder = std::filesystem::path());

        // Getters
        inline IPFormat get_ip() const { return m_listening_ip; }
        inline std::string get_client_id() const { return m_client_id; }
        inline std::filesystem::path get_listening_dir() const { return m_listening_dir; }
        TransferSession *get_session(std::string transfer_id);
        inline std::map<std::string, TransferSession *> &get_sessions() { return m_sessions; }
        inline const std::map<std::string, TransferSession *> &get_sessions() const { return m_sessions; }
        std::string get_my_conn();

        // Setters
        inline void set_ip(std::string ip) { m_listening_ip.set_ip(std::move(ip)); }
        inline void set_port(int port) { m_listening_ip.set_port(port); }
        inline void set_client_id(std::string client_id) { m_client_id = std::move(client_id); }
        inline void set_listening_dir(const std::filesystem::path &listening_dir)
        {
            // remove all occurences of ./ in the file path
            std::string file_path_str = listening_dir.string();
            std::string x = "./";

            size_t pos = 0;
            while (true)
            {
                pos = file_path_str.find(x, pos);
                if (pos == std::string::npos)
                {
                    break;
                }

                file_path_str.replace(pos, x.length(), "");
            }

            m_listening_dir = std::filesystem::absolute(file_path_str);
        }

    private:
        /// @brief IP address of the client
        IPFormat m_listening_ip;

        /// @brief TransferClient ID, unique identifier of the client
        std::string m_client_id;

        /// @brief Listening directory, directory where the client will listen for incoming files and files to share
        std::filesystem::path m_listening_dir;

        /// @brief Map of active sessions (key = session ID, value = session)
        std::map<std::string, TransferSession *> m_sessions;

        /// @brief Map of available files (key = file path, value = file metadata)
        std::map<std::string, TransferMetadata> m_available_files;

        /// @brief Connection uuid of the client, retrieved using the notification interface and calling get_my_conn()
        std::string m_my_conn = "";

        /// @brief Function to generate session ID
        std::string generate_session_id(const std::string &transferid, const std::string &dest = "");

        /// @brief Add a session to the client
        /// @param session  Session to add
        TransferSession *add_session(TransferSession *session);

        /// @brief Remove a session from the client
        /// @param session_id  ID of the session to remove
        void remove_session(const std::string &session_id);

        /// @brief Action to do when the client receive a notification
        /// @param notif Notification received
        /// @return True if the notification was handled
        bool action_on_receive_notification(NotificationData notif) override;

        /// @brief Share available files (in m_listening_dir)
        /// @param to_share Set of files to share
        /// @param dest Destination of the files
        void share_available_files(const std::set<std::filesystem::path> &to_share, const std::string &dest);

        /// @brief Create a metadata from a file
        /// @param src Path of the file
        /// @return Metadata of the file
        TransferMetadata create_metadata_from_file(const std::filesystem::path &src);
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_TRANSFER_CLIENT_HPP_