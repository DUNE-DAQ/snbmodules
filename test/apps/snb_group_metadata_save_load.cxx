/**
 * @file snb_group_metadata_save_load.cxx Test app to save and load group metadatas
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/group_metadata.hpp"

#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <cassert>

using namespace dunedaq::snbmodules;

int main()
{
    try
    {
        // Create a GroupMetdata file
        nlohmann::json transfer_options = R"(
            {
                "option1" : "5010",
                "option2" : -1
            }
        )"_json;

        GroupMetadata metadata("group1", "source1", IPFormat("192.168.0.107", 42100), e_protocol_type::dummy, transfer_options);
        metadata.generate_metadata_file(".");

        // Test if file exist in current directory
        if (std::filesystem::exists("./group1" + GroupMetadata::m_file_extension))
        {
            TLOG() << "File exists";
        }
        else
        {
            TLOG() << "File does not exist";
            return 1;
        }

        // Create a TransferMetadata object from file saved before and compare it with the original one
        GroupMetadata metadata2("./group1" + GroupMetadata::m_file_extension);
        assert(metadata == metadata2);

        TLOG() << "Test passed";

        // Delete file
        std::filesystem::remove("./group1" + GroupMetadata::m_file_extension);

        return 0;
    }
    catch (const std::exception &e)
    {
        TLOG() << e.what();
        return 1;
    }
}