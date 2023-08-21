/**
 * @file snb_notfication_test.cxx Test app of notification transfers
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/transfer_client.hpp"
#include "snbmodules/transfer_session.hpp"
#include "snbmodules/bookkeeper.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <unistd.h>
#include <sys/types.h>

using namespace dunedaq::snbmodules;

int main()
{

    try
    {
        int n_apps = 4;
        int success_counter = 0;
        int starting_port = 50000;

        std::vector<pid_t> pids;

        for (int i = 0; i < n_apps; i++)
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                // Bookkeeper process
                if (i == n_apps - 1)
                {
                    Bookkeeper bookkeeper(IPFormat("localhost", starting_port + i), "bookkeeper");
                    // Add every client connection
                    for (int j = 0; j < n_apps - 1; j++)
                    {
                        bookkeeper.add_connection(IPFormat("localhost", starting_port + j), "client" + std::to_string(j), "notification_t", true);
                    }

                    bookkeeper.init_connection_interface();
                    // Receive every connection request
                    for (int j = 0; j < n_apps - 1; j++)
                    {
                        assert(bookkeeper.listen_for_notification("client" + std::to_string(j)).has_value() == true);
                        // wait 100ms
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
                // Clients process
                else
                {
                    // Add a unique connection and send a connection request
                    TransferClient client(IPFormat("localhost", starting_port + i), "client" + std::to_string(i), "./client");
                    client.add_connection(client.get_ip(), "client" + std::to_string(i), "notification_t", true);
                    client.init_connection_interface();
                    assert(client.send_notification(e_notification_type::CONNECTION_REQUEST, client.get_client_id(), "bookkeeper", "client" + std::to_string(i)) == true);
                }
                return 0;
            }
            else
            {
                // Parent process
                pids.push_back(pid);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        std::filesystem::remove("./client");

        for (auto pid : pids)
        {
            TLOG() << "Sending SIGINT to " << pid;
            kill(pid, SIGINT);
        }
        for (auto pid : pids)
        {
            TLOG() << "Sending SIGKILL to " << pid;
            kill(pid, SIGKILL);
        }
        for (auto pid : pids)
        {
            TLOG() << "Waiting for app " << pid;
            siginfo_t status;
            auto sts = waitid(P_PID, pid, &status, WEXITED);
            TLOG() << "Forked process " << pid << " exited with status " << status.si_status << " (wait status " << sts
                   << ")";
            if (status.si_status == 2)
            {
                success_counter++;
            }
            else
            {
                TLOG() << "App " << pid << " failed";
            }
        }

        if (success_counter == n_apps)
        {
            TLOG() << "Test passed";
        }
        else
        {
            TLOG() << "Test failed";
        }
    }
    catch (const std::exception &e)
    {
        TLOG() << e.what();
        return 1;
    }

    return 0;
} // NOLINT