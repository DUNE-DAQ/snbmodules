/**
 * @file iomanager_wrapper.cpp IOManagerWrapper class definition : wrapper of the IOManager to adapt it to the snbmodules
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/iomanager_wrapper.hpp"

#include <string>
#include <utility>

namespace dunedaq::snbmodules
{

    void IOManagerWrapper::init_connection_interface(const std::string &session_name, bool use_connectivity_service, const IPFormat &ip)
    {
        dunedaq::logging::Logging::setup();

        setenv("DUNEDAQ_SESSION", session_name.c_str(), 0);

        if (use_connectivity_service)
        {
            setenv("CONNECTION_SERVER", ip.get_ip().c_str(), 1);
            setenv("CONNECTION_PORT", std::to_string(ip.get_port()).c_str(), 1);
        }

        iomanager::IOManager::get()->configure(m_queues, m_connections, use_connectivity_service, std::chrono::milliseconds(100));
    }

    void IOManagerWrapper::add_connection(const IPFormat &ip, std::string id, std::string data_type)
    {
        iomanager::Connection conn = iomanager::Connection{iomanager::ConnectionId{std::move(id), std::move(data_type)},
                                                           "tcp://" + ip.get_ip() + ":" + (std::to_string(ip.get_port()) == "0" ? "*" : std::to_string(ip.get_port())),
                                                           iomanager::ConnectionType::kSendRecv};

        TLOG() << "debug : Added connection " << conn.id.uid << " uri: " << conn.uri;
        m_connections.emplace_back(std::move(conn));
    }

    iomanager::ConnectionResponse IOManagerWrapper::lookups_connection(iomanager::ConnectionId const &conn_id, bool restrict_single)
    {
        return iomanager::NetworkManager::get().get_connections(conn_id, restrict_single);
    }
} // namespace dunedaq::snbmodules