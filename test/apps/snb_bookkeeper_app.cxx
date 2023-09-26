/**
 * @file snb_bookkeeper_app.cxx Test app full bookkeeper implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/bookkeeper.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <cassert>
#include <stdexcept>

using namespace dunedaq::snbmodules;

int main()
{
    Bookkeeper b(IPFormat("localhost", 5001), "bookkeeper");
    b.add_connection(IPFormat("localhost:5001"), "snbmodules_bookkeeper_notification", "notification_t", false);
    b.add_connection(IPFormat("localhost:5002"), "snbmodules_client0_notification", "notification_t", true);
    b.add_connection(IPFormat("localhost:5003"), "snbmodules_client1_notification", "notification_t", true);
    b.init_connection_interface("snbmodules", false, IPFormat("localhost:5000"));
    b.start();

    return 0;
}