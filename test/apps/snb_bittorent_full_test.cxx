
#include "snbmodules/transfer_client.hpp"
#include "snbmodules/common/protocols_enum.hpp"
#include "snbmodules/bookkeeper.hpp"

#include "utilities/WorkerThread.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <boost/iostreams/device/mapped_file.hpp>

using namespace dunedaq::snbmodules;
namespace io = boost::iostreams;

int main()
{

    try
    {
        // Important note : libtorrent does not support localhost ip addresses, he cannot connect to himself

        // Create clients
        std::string ip0 = "192.168.0.105:5009";
        std::string ip1 = "192.168.0.106:5010";

        TransferClient c0(IPFormat(ip0), "client0", "./client0");
        TransferClient c1(IPFormat(ip1), "client1", "./client1");

        // Initialize connections
        c0.add_connection(IPFormat(ip0), "client0", "notification_t", true);
        c0.add_connection(IPFormat(ip1), "client1", "notification_t", true);
        c0.init_connection_interface();

        // Start client0 in a thread ( listening to notifications, Dowloader )
        dunedaq::utilities::WorkerThread thread(std::bind(&TransferClient::do_work, &c0, std::placeholders::_1));
        thread.start_working_thread();

        // Create file to transfer
        std::string file_name = "client1/test.txt";
        std::ofstream file(file_name);
        for (int i = 0; i < 100000; i++)
            file << "Hello World " << i << "!";
        file.close();

        nlohmann::json transfer_options = R"(
            {
                "port" : "5010",
                "rate_limit" : -1
            }
        )"_json;

        // Create transfer with client1 as uploader and client0 as downloader
        c1.create_new_transfer("transfer0", "BITTORRENT", {c0.get_client_id()}, {file_name}, transfer_options);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        c1.get_session("transfer0")->start_all();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // note that if input file is too small, the transfer will be completed before the pause and Warning will be printed
        c0.get_session("transfer0")->pause_all();
        std::this_thread::sleep_for(std::chrono::seconds(5));

        c0.get_session("transfer0")->resume_all();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        thread.stop_working_thread();

        // Checking if file was transferred
        io::mapped_file_source f1(file_name);
        io::mapped_file_source f2("./client0/transfer0/test.txt");

        if (f1.size() == f2.size() && std::equal(f1.data(), f1.data() + f1.size(), f2.data()))
            TLOG() << "The files are equal";
        else
            TLOG() << "The files are not equal";

        // Clean files
        std::filesystem::remove_all("client0");
        std::filesystem::remove_all("client1");
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}