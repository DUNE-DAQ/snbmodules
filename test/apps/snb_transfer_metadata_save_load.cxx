/**
 * @file snb_transfer_metadata_save_load.cxx Test app to save and load transfer metadatas
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/transfer_metadata.hpp"

#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <cassert>

using namespace dunedaq::snbmodules;

int main()
{
    try
    {
        // Create a TransferMetadata file
        TransferMetadata transfer_metadata("./test.txt", 100, IPFormat("192.168.0.107", 42100), "QOSFCBZAPOQDBSOsdonfsnodfnossnof4516", IPFormat("192.168.0.106", 999), "group1", 50, e_status::WAITING);
        transfer_metadata.generate_metadata_file(".");

        // Test if file exist in current directory
        if (std::filesystem::exists("./test.txt" + TransferMetadata::m_file_extension))
        {
            TLOG() << "File exists";
        }
        else
        {
            TLOG() << "File does not exist";
            return 1;
        }

        // Create a TransferMetadata object from file saved before and compare it with the original one
        TransferMetadata transfer_metadata2("./test.txt" + TransferMetadata::m_file_extension);
        assert(transfer_metadata == transfer_metadata2);

        TLOG() << "Test passed";

        // Delete file
        std::filesystem::remove("./test.txt" + TransferMetadata::m_file_extension);

        return 0;
    }
    catch (const std::exception &e)
    {
        TLOG() << e.what();
        return 1;
    }
}