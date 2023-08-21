/**
 * @file snb_session_test.cxx Test app of session transfers
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/transfer_client.hpp"
#include "snbmodules/common/protocols_enum.hpp"
#include "snbmodules/bookkeeper.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <cassert>
#include <stdexcept>
#include <utility>

using namespace dunedaq::snbmodules;

int main()
{

    try
    {

        // Create clients
        IPFormat ip1("127.0.0.1:42100");
        IPFormat ip2("127.0.0.2:42100");

        TransferClient client1(ip1, "client1", "./client");
        TransferClient client2(ip2, "client2", "./client");

        // Create new group transfer with 2 files and 1 expected file
        GroupMetadata transfer_options("group1", "client1", ip1, e_protocol_type::dummy);
        transfer_options.add_expected_file("test.txt");
        transfer_options.add_expected_file("test2.txt");
        transfer_options.add_file(std::move(TransferMetadata("test.txt", 100, ip1)));

        // Create sessions in each client
        TransferSession *ses1 = client1.create_session(transfer_options, e_session_type::Uploader, "session1", "./listen/s1");
        TransferSession *ses2 = client2.create_session(transfer_options, e_session_type::Downloader, "session2", "./listen/s2");

        // Upload the first file
        ses1->upload_all();
        ses2->download_all("./listen");
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Add the second file
        ses1->add_file(std::move(TransferMetadata("test2.txt", 50, ip1)));

        // Upload the second file
        ses1->upload_all();
        ses2->download_all("./listen");
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Remove the files
        std::filesystem::remove_all("./listen");

        TLOG() << "Test passed";

        return 0;
    }
    catch (const std::exception &e)
    {
        TLOG() << e.what();
        return 1;
    }
}