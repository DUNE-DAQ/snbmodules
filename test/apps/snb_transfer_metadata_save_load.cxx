
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
        transfer_metadata.generate_metadata_file();

        // Test if file exist in current directory
        if (std::filesystem::exists("./test.txt" + TransferMetadata::m_file_extension))
        {
            std::cout << "File exists" << std::endl;
        }
        else
        {
            std::cout << "File does not exist" << std::endl;
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
        std::cerr << e.what() << '\n';
        return 1;
    }
}