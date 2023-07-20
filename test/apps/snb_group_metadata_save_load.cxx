
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
        metadata.generate_metadata_file();

        // Test if file exist in current directory
        if (std::filesystem::exists("./group1" + GroupMetadata::m_file_extension))
        {
            std::cout << "File exists" << std::endl;
        }
        else
        {
            std::cout << "File does not exist" << std::endl;
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
        std::cerr << e.what() << '\n';
        return 1;
    }
}