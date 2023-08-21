/**
 * @file snb_transfer_client_app.cxx Test app of clients interraction
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/transfer_client.hpp"
#include "snbmodules/transfer_session.hpp"
#include "snbmodules/ip_format.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <cassert>
#include <stdexcept>

using namespace dunedaq::snbmodules;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        TLOG() << "Usage: ./client <number>";
        return 1;
    }

    std::string number = argv[1]; // NOLINT

    int base_port = 5002;
    IPFormat ip = IPFormat("localhost", base_port + std::stoi(number));

    TransferClient transfer_client(ip, "client" + number, "listen");
    transfer_client.add_connection(IPFormat("localhost:5001"), "snbmodules_bookkeeper_notification", "notification_t", false);
    transfer_client.add_connection(ip, "snbmodules_client" + number + "_notification", "notification_t", true);
    transfer_client.init_connection_interface("snbmodules", false, IPFormat("localhost:5000"));
    transfer_client.start(100000000);

    return 0;
}