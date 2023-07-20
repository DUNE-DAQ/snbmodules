
#include "snbmodules/transfer_client.hpp"
#include "snbmodules/common/protocols_enum.hpp"
#include "snbmodules/bookkeeper.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <cassert>
#include <stdexcept>
#include <set>

using namespace dunedaq::snbmodules;

int main()
{

    try
    {
        // might not be usefull anynmore, allow a client to load metadata files from a folder and add them to group transfers

        // Create a client
        IPFormat ip1("127.0.0.1:42100");
        TransferClient client1(ip1, "client1", "./client1");

        // Create transfer with dummy protocol
        GroupMetadata transfer_options("transfer1", "client1", ip1, e_protocol_type::dummy);

        // add non existing file to the transfer
        transfer_options.add_expected_file("file1");
        TransferMetadata file("file1", 100, ip1);
        transfer_options.add_file(&file);

        // write metadata file in folder
        file.generate_metadata_file("./client1");

        // scan available files and metadata files
        std::set<std::filesystem::path> files = std::set<std::filesystem::path>();
        client1.scan_available_files(&files); // file metadata alone without transfermetadata expecting them are ignored

        // create a metadata group transfer and scan it
        transfer_options.generate_metadata_file("./client1");
        client1.scan_available_files(&files);

        // print result, expecting 2 files
        TLOG() << "Available files: ";
        for (auto &file : files)
        {
            TLOG() << file;
        }

        // Clean up
        std::filesystem::remove_all("./client1");

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}