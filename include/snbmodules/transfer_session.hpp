
#ifndef SNBMODULES_INCLUDE_SNBMODULES_SESSION_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_SESSION_HPP_

// for fork (TODO : usefull ?)
#include <sys/prctl.h>
#include <sys/wait.h>
#include <fstream>

#include "snbmodules/group_metadata.hpp"
#include "snbmodules/transfer_metadata.hpp"
#include "snbmodules/common/protocols_enum.hpp"
#include "snbmodules/notification_interface.hpp" // for notification data

#include "snbmodules/tools/natural_sort.hpp" // convert enum to string and reverse

// protocols
#include "snbmodules/interfaces/transfer_interface_abstract.hpp"
#include "snbmodules/interfaces/transfer_interface_bittorrent.hpp"
#include "snbmodules/interfaces/transfer_interface_SCP.hpp"
#include "snbmodules/interfaces/transfer_interface_RClone.hpp"

namespace dunedaq::snbmodules
{
    /// @brief Different type of session
    enum e_session_type
    {
        /// @brief TransferSession used to download files from uploaders client
        Downloader,
        /// @brief TransferSession used to upload files to downloaders client
        Uploader,
    };

    /// @brief TransferSession class contained in a client, is a wrapper for a transfer.
    /// extend notification interface because the session can communicate with bookkeeper via client initialized connection.
    class TransferSession : public NotificationInterface
    {

    public:
        /// Update the metadata of the session to the bookkeeper
        /// @return true if success
        bool update_metadatas_to_bookkeeper();

        /// Update the metadata of the given transfer metadata to the bookkeeper
        /// @return true if success
        bool update_metadata_to_bookkeeper(TransferMetadata *f_meta);

        /// @brief Usefull to check if the session is a downloader
        /// @return true if the session is a downloader
        bool is_downloader() { return m_type == e_session_type::Downloader; }

        /// @brief Usefull to check if the session is a uploader
        /// @return true if the session is an uploader
        bool is_uploader() { return m_type == e_session_type::Uploader; }

        /// @brief equal operator, used to compare sessions in a set (client)
        /// @param other other session
        /// @return is equal ?
        bool operator==(TransferSession const &other) const { return m_session_id == other.m_session_id; }

        /// @brief Operator <, used to compare sessions in a set (client)
        /// @param other other session
        /// @return is less than ?
        bool operator<(TransferSession const &other) const { return SI::natural::compare<std::string>(m_session_id, other.m_session_id); }

        /// @brief Constructor
        /// @param transfer_options group metadata
        /// @param type type of session
        /// @param id unique identifier of the session
        /// @param ip ip of the client (TODO : useless ?)
        TransferSession(GroupMetadata *transfer_options, e_session_type type, std::string id, IPFormat ip, std::filesystem::path work_dir, std::vector<std::string> bk_conn = std::vector<std::string>(), std::set<std::string> client_conn = std::set<std::string>());

        /// @brief Destructor
        /// Kill all threads created by the session (TODO : useless ?)
        virtual ~TransferSession();

        // Getters
        inline GroupMetadata get_transfer_options() const { return m_transfer_options; }
        inline TransferInterfaceAbstract *get_transfer_interface() { return m_transfer_interface; }
        inline std::filesystem::path get_work_dir() const { return m_work_dir; }
        inline std::set<std::string> get_target_clients() const { return m_target_clients; }
        inline IPFormat get_ip() const { return m_ip; }
        std::string get_session_id() { return m_session_id; }
        std::string to_string();

        // Setters
        void set_target_clients(std::set<std::string> clients) { m_target_clients = clients; }

        // Interface for the transfer, TODO: add notifications : DO WE REALLY WANT THAT ?
        void add_file(TransferMetadata *fmeta)
        {
            m_transfer_options.add_file(fmeta);
            update_metadata_to_bookkeeper(fmeta);
        }

        bool pause_file(TransferMetadata *f_meta, bool is_multiple = false);
        bool resume_file(TransferMetadata *f_meta, bool is_multiple = false);
        bool hash_file(TransferMetadata *f_meta, bool is_multiple = false);
        bool cancel_file(TransferMetadata *f_meta, bool is_multiple = false);

        bool pause_all();
        bool resume_all();
        bool cancel_all();

        /// @brief Start the session by downloading or uploading files depending on the type of session TODO : separate thread ?
        bool start_file(TransferMetadata *f_meta);
        bool start_all();

        // Downloaders only
        bool download_all(std::filesystem::path dest);
        bool download_file(TransferMetadata *f_meta, std::filesystem::path dest, bool is_multiple = false);

        // Uploaders only
        bool upload_all();
        bool upload_file(TransferMetadata *f_meta, bool is_multiple = false);

    private:
        /// @brief Type of session, uploader or downloader.
        /// Used to block access to some functions
        e_session_type m_type;

        /// @brief Unique identifier of the session,
        /// used to identify the session in the client and to target notification
        std::string m_session_id;

        /// @brief Ip of the client
        /// TODO : useless ? the session cannot have a unique connection
        IPFormat m_ip;

        /// @brief A session must contain a transfer matadata.
        /// The session is basically a transfer wrapper
        GroupMetadata m_transfer_options;

        /// @brief Abstract interface can reference to any protocol used to transfer data
        TransferInterfaceAbstract *m_transfer_interface;

        /// @brief List of threads created by the session
        /// TODO : useless ? should be handle in client
        std::vector<pid_t> m_threads;

        /// @brief Dir where the session write files
        std::filesystem::path m_work_dir;

        /// @brief clients that dowload the files, only used by uploader
        std::set<std::string> m_target_clients;

        /// @brief check if the session is initialized to send metadata to bookkeeper
        bool m_is_init = false;

        /// @brief handle actions to be taken when a notification is received.
        /// The notification is passed as a parameter by the client because only 1 connection is opened
        /// (not possible to open connection after configuration)
        /// @param notif contain everything we need to know about the notification
        bool action_on_receive_notification(NotificationData notif) override;

        /// @brief Send a notification to every targets of the session, generally for the uploader
        /// @param type type of notification to send
        /// @param data data to send, default empty
        /// @return true if success
        bool send_notification_to_targets(e_notification_type type, std::string data = "");
    };

} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_TRANSFERSESSION_HPP_