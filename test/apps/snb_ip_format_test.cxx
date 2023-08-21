/**
 * @file snb_ip_format_test.cxx Test app of IP format class
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/ip_format.hpp"
#include "logging/Logging.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace dunedaq::snbmodules;

int main()
{

    try
    {
        // Construcor test
        IPFormat ip("192.168.0.107", 42100);

        assert(ip.get_ip() == "192.168.0.107" && ip.get_port() == 42100);

        // Setters/Getters test
        ip.set_ip("192.168.0.110");
        ip.set_port(42101);

        assert(ip.get_ip() == "192.168.0.110" && ip.get_port() == 42101);

        // get_ip_port test
        assert(ip.get_ip_port() == "192.168.0.110:42101");

        // Copy constructor test
        IPFormat ip2(ip);

        // == operator test
        assert(ip == ip2);
        ip2.set_ip("192.515.0.110");
        assert(!(ip == ip2));

        TLOG() << "IPFormat tests passed";

        return 0;
    }
    catch (const std::exception &e)
    {
        TLOG() << e.what();
        return 1;
    }
}