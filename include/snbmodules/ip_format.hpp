#ifndef SNBMODULES_INCLUDE_SNBMODULES_IPFORMAT_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_IPFORMAT_HPP_

#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <filesystem>
#include <string>
#include <stdexcept>
#include <iostream>
#include <stdexcept>
#include <string.h>

#include "snbmodules/tools/natural_sort.hpp"
namespace dunedaq::snbmodules
{
    /// @brief Class that represents an IP address and a port
    /// TODO: should be replaced by something better ?
    class IPFormat
    {
    private:
        /// @brief IP address
        std::string ip = "0.0.0.0";
        /// @brief Port
        int port = 0;

    public:
        /// @brief  Defualt = operator
        IPFormat &operator=(IPFormat const &) = default;

        /// @brief  == operator
        bool operator==(IPFormat const &o) const
        {
            return ip == o.ip && port == o.port;
        }

        /// @brief  < operator
        bool operator<(IPFormat const &o) const
        {
            return SI::natural::compare<std::string>(ip, o.ip) || (ip == o.ip && port < o.port);
        }

        /// @brief  Get the IP address and the port in the format "ip:port"
        /// @return  The IP address and the port in the format "ip:port"
        std::string get_ip_port() { return this->ip + ":" + std::to_string(this->port); }

        /// @brief Check if the IP address and the port are set to the default values
        /// @return True if the IP address and the port are set to the default values, false otherwise
        inline bool is_default() { return ip == "0.0.0.0" && port == 0; }

        // Setters
        void set_port(int port)
        {

            if (port < 0 || port > 65535)
            {
                throw std::invalid_argument("Port must be between 0 and 65535");
            }

            this->port = port;
        }

#ifndef _MSC_VER
        inline char *strtok_s(char *s, const char *delim, char **context)
        {
            return strtok_r(s, delim, context);
        }
#endif // !_MSC_VER

        /// @brief  Set the IP address, must be called after set_port() if the port is specified in the IP address ex 0.0.0.0:1234
        void set_ip(std::string ip)
        {
            if (ip.empty())
            {
                throw std::invalid_argument("IP address cannot be empty");
            }

            // Splitting the string IP:PORT or IP
            std::vector<std::string> ip_port_pair;
            char *next_token = NULL;

            char *token = strtok_s(const_cast<char *>(ip.c_str()), ":", &next_token);
            while (token != nullptr)
            {
                ip_port_pair.push_back(std::string(token));
                token = strtok_s(nullptr, ":", &next_token);
            }
            if (ip_port_pair.size() != 1 && ip_port_pair.size() != 2)
            {
                throw std::invalid_argument("Invalid IP address format (IP:PORT or IP)");
            }

            // TODO : Check if the IPv4 address is valid

            // set values
            if (ip_port_pair.size() == 2)
            {
                set_port(std::stoi(ip_port_pair[1]));
            }

            this->ip = ip_port_pair[0];
        }

        // Getters
        std::string get_ip() { return this->ip; }
        int get_port() { return this->port; }

        // Constructors
        /// @brief  Constructor that set the IP address and the port
        /// @param ip  IP address
        /// @param port  Port
        IPFormat(std::string ip = "0.0.0.0", int port = 0)
        {
            set_port(port);
            set_ip(ip);
        }

        /// @brief  Copy constructor
        IPFormat(const IPFormat &o)
        {
            set_port(o.port);
            set_ip(o.ip);
        }
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_IPFORMAT_HPP_