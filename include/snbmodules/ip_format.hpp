/**
 * @file ip_format.hpp IPFormat very simple class to represent an IP address and a port
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_INCLUDE_SNBMODULES_IP_FORMAT_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_IP_FORMAT_HPP_

// #define _CRT_SECURE_NO_WARNINGS

#include <filesystem>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstring>
#include <utility>

namespace dunedaq::snbmodules
{
    /// @brief Class that represents an IP address and a port
    /// TODO: should be replaced by something better ?
    class IPFormat
    {

    public:
        // Constructors
        /// @brief  Constructor that set the IP address and the port
        /// @param ip  IP address
        /// @param port  Port
        explicit IPFormat(const std::string &ip = "0.0.0.0", int port = 0)
        {
            set_port(port);
            set_ip(ip);
        }

        /// @brief  Copy constructor
        IPFormat(const IPFormat &o)
        {
            set_port(o.m_port);
            set_ip(o.m_ip);
        }

        /// @brief  Defualt = operator
        IPFormat &operator=(IPFormat const &) = default;

        /// @brief  == operator
        bool operator==(IPFormat const &o) const
        {
            return m_ip == o.m_ip && m_port == o.m_port;
        }

        /// @brief  < operator
        bool operator<(IPFormat const &o) const
        {
            return m_ip.compare(o.m_ip) || (m_ip == o.m_ip && m_port < o.m_port);
        }

        /// @brief  Get the IP address and the port in the format "ip:port"
        /// @return  The IP address and the port in the format "ip:port"
        std::string get_ip_port() const { return m_ip + ":" + std::to_string(m_port); }

        /// @brief Check if the IP address and the port are set to the default values
        /// @return True if the IP address and the port are set to the default values, false otherwise
        inline bool is_default() const { return m_ip == "0.0.0.0" && m_port == 0; }

        // Setters
        void set_port(int port)
        {

            if (port < 0 || port > 65535)
            {
                throw std::invalid_argument("Port must be between 0 and 65535");
            }

            m_port = port;
        }

#ifndef _MSC_VER
        inline char *strtok_s(char *s, const char *delim, char **context)
        {
            return strtok_r(s, delim, context);
        }
#endif // !_MSC_VER

        /// @brief  Set the IP address, must be called after set_port() if the port is specified in the IP address ex 0.0.0.0:1234
        void set_ip(const std::string &ip)
        {
            if (ip.empty())
            {
                throw std::invalid_argument("IP address cannot be empty");
            }

            // Splitting the string IP:PORT or IP
            std::vector<std::string> ip_port_pair;
            char *next_token = nullptr;

            char *ip_char = new char[ip.length() + 1];
            strcpy(ip_char, ip.c_str());

            char *token = strtok_s(ip_char, ":", &next_token);
            while (token != nullptr)
            {
                ip_port_pair.emplace_back(std::string(token));
                token = strtok_s(nullptr, ":", &next_token);
            }

            delete[] ip_char;

            if (ip_port_pair.size() != 1 && ip_port_pair.size() != 2)
            {
                throw std::invalid_argument("Invalid IP address format (IP:PORT or IP)");
            }

            // TODO Aug-14-2022 Leo Joly leo.vincent.andre.joly@cern.ch : Check if the IPv4 address is valid

            // set values
            if (ip_port_pair.size() == 2)
            {
                set_port(std::stoi(ip_port_pair[1]));
            }

            m_ip = ip_port_pair[0];
        }

        // Getters
        std::string get_ip() const { return m_ip; }
        int get_port() const { return m_port; }

    private:
        /// @brief IP address
        std::string m_ip = "0.0.0.0";
        /// @brief Port
        int m_port = 0;
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_IP_FORMAT_HPP_