
#include "snbmodules/transfer_client.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <cassert>
#include <stdexcept>

using namespace dunedaq::snbmodules;

int main()
{

    try
    {
        // Test Constructor
        std::string ip = "192.168.0.107";
        int port = 42100;
        std::string client_id = "client1";
        std::filesystem::path listening_dir = "test";

        TransferClient client(IPFormat(ip, port), client_id, listening_dir);

        assert(client.get_ip().get_ip() == ip);
        assert(client.get_ip().get_port() == port);
        assert(client.get_client_id() == client_id);
        assert(client.get_listening_dir() == std::filesystem::absolute(listening_dir));

        // Test setters/getters
        client.set_ip("192.168.0.106");
        client.set_port(42101);
        client.set_client_id("client2");
        client.set_listening_dir("test2");

        assert(client.get_ip().get_ip() == "192.168.0.106");
        assert(client.get_ip().get_port() == 42101);
        assert(client.get_client_id() == "client2");
        assert(client.get_listening_dir() == std::filesystem::absolute("test2"));

        TLOG() << "Test passed";

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}