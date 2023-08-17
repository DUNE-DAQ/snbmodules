
#ifndef SNBMODULES_INCLUDE_SNBMODULES_IOMANAGER_WRAPPER_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_IOMANAGER_WRAPPER_HPP_

#include "snbmodules/common/notification_enum.hpp"
#include "snbmodules/ip_format.hpp"

#include "iomanager/IOManager.hpp"
#include "iomanager/network/ConfigClient.hpp"
#include "iomanager/network/NetworkIssues.hpp"

#include <string>

namespace dunedaq::snbmodules
{
    // Singleton
    class IOManagerWrapper
    {

    public:
        static IOManagerWrapper &get()
        {
            static IOManagerWrapper instance;
            return instance;
        }

        // not cloneable
        IOManagerWrapper(IOManagerWrapper &other) = delete;
        // not assignable
        void operator=(const IOManagerWrapper &) = delete;

        /// @brief init IOManager instance,
        /// Only used for stand alone application
        /// @param use_connectivity_service if true, use the connectivity service
        /// @param ip ip of the connectivity service
        void init_connection_interface(const std::string &session_name, bool use_connectivity_service = false, const IPFormat &ip = IPFormat("localhost", 5000));

        /// @brief lookups a connection with the given pattern
        /// @param conn_id regex pattern of the connection
        /// @param restrict_single if true, only one connection is allowed with the given pattern
        /// @return connection response
        iomanager::ConnectionResponse lookups_connection(const iomanager::ConnectionId &conn_id, bool restrict_single);

        /// @brief add a connection to the list of connections,
        /// Only used for stand alone application
        /// @param ip ip of the connection
        /// @param id name of the connection
        /// @param data_type data type of the connection data
        void add_connection(const IPFormat &ip, std::string id, std::string data_type);

    protected:
        /// @brief List of connections
        iomanager::connection::Connections_t m_connections;
        /// @brief List of queues
        iomanager::connection::Queues_t m_queues;

    private:
        IOManagerWrapper() = default;
    };

} // namespace dunedaq::snbmodules

#endif // SNBMODULES_INCLUDE_SNBMODULES_IOMANAGER_WRAPPER_HPP_