/**
 * @file bookkeeper.cpp Bookkeeper class retriving informations from clients
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/bookkeeper.hpp"

#include <string>
#include <set>
#include <vector>
#include <utility>

namespace dunedaq::snbmodules
{
    // TODO Aug-14-2022 Leo Joly leo.vincent.andre.joly@cern.ch : Obsolete, is this needed anymore ?
    void Bookkeeper::create_new_transfer(const std::string &protocol, const std::string &src, const std::set<std::string> &dests, const std::set<std::filesystem::path> &files, const nlohmann::json &protocol_options)
    {
        // suppress warnings
        (void)protocol;
        (void)src;
        (void)dests;
        (void)files;
        (void)protocol_options;

        // TLOG() << "debug : creating new transfer with protocol " << protocol;

        // // Check protocol
        // std::optional<protocol_type::e_protocol_type> _protocol = protocol_type::string_to_protocols(protocol);
        // if (!_protocol.has_value())
        // {
        //     TLOG() << "debug : invalid protocol !";
        //     return;
        // }

        // // Check files and found metadata
        // std::set<TransferMetadata *> files_meta;
        // auto list = get_m_transfers().at(src);
        // int nb_files = 0;

        // for (std::filesystem::path file : files)
        // {
        //     bool found = false;
        //     for (auto filemeta : list)
        //     {
        //         if (filemeta.get_file_name() == file)
        //         {
        //             files_meta.emplace(&filemeta);
        //             found = true;
        //             nb_files++;
        //             break;
        //         }
        //     }
        //     if (!found)
        //     {
        //         TLOG() << "debug : file " << file << " not found ! (ignoring)";
        //     }
        // }

        // if (nb_files == 0)
        // {
        //     TLOG() << "debug : no file found ! exiting transfer creation.";
        //     return;
        // }

        // // Create transfer
        // GroupMetadata transfer("transfer" + std::to_string(transfers.size()), _protocol.value(), files_meta);
        // transfers_link[transfer.get_group_id()] = src;
        // transfers.emplace(transfer);

        // // Notify clients
        // for (auto client : src)
        // {
        //     std::string session_name = client + "_ses" + transfer.get_group_id();

        //     TLOG() << "debug : notifying src client " << client;
        //     send_notification(notification_type::e_notification_type::NEW_TRANSFER, get_bookkeeper_id(), session_name, client, 1000, transfer.export_to_string());
        // }
    }

    // Only for stand alone application
    void Bookkeeper::input_action(char input)
    {
        switch (input)
        {
        case 'q':
        {
            TLOG() << "Exiting...";
            exit(0);
            break;
        }
        case 'd':
        {
            display_information();
            break;
        }
        case 'n':
        {
            TLOG() << "Creating new transfer ...";
            TLOG() << "Choose protocol in the list";
            for (int enum_i = protocol_type::e_protocol_type::BITTORRENT; enum_i != protocol_type::e_protocol_type::dummy; enum_i++)
            {
                TLOG() << enum_i << " - " << protocol_type::protocols_to_string(static_cast<protocol_type::e_protocol_type>(enum_i));
            }
            int protocol = -1;
            std::cin >> protocol;
            // Check input
            if (protocol < 0 || protocol > protocol_type::e_protocol_type::dummy)
            {
                TLOG() << "Invalid protocol";
                break;
            }

            TLOG() << "Choose clients (q when finished)";
            std::set<std::string> choosen_clients;
            while (true)
            {
                std::string client;
                std::cin >> client;
                if (client == "q")
                {
                    break;
                }
                // Check input
                if (std::find(get_clients_conn().begin(), get_clients_conn().end(), client) == get_clients_conn().end())
                {
                    TLOG() << "Invalid client";
                    break;
                }

                choosen_clients.emplace(client);
            }
            if (choosen_clients.empty())
            {
                TLOG() << "No client selected";
                break;
            }
            if (choosen_clients.size() < 2)
            {
                TLOG() << "At least 2 clients are required";
                break;
            }

            TLOG() << "Choose file to transmit (q when finished)";
            std::set<TransferMetadata *> choosen_files;
            while (true)
            {
                uint64_t initial_size = choosen_files.size();
                std::string file;
                std::cin >> file;
                if (file == "q")
                {
                    break;
                }

                for (const std::string &client : choosen_clients)
                {
                    // Check input
                    auto &list = get_transfers().at(client);
                    bool found = false;
                    for (TransferMetadata *filemeta : list)
                    {
                        if (filemeta->get_file_name() == file)
                        {
                            choosen_files.emplace(filemeta);
                            found = true;
                            break;
                        }
                    }
                    if (found == true)
                    {
                        break;
                    }
                }
                if (initial_size == choosen_files.size())
                {
                    TLOG() << "Invalid file";
                    break;
                }
            }
            if (choosen_files.empty())
            {
                TLOG() << "No file selected";
                break;
            }

            // create_new_transfer(protocol_type::protocols_to_string(protocol_type::string_to_protocols(protocol).value()), choosen_clients, choosen_files);
            break;
        }

        case 's':
        {
            TLOG() << "Choose transfers to start ... ";
            std::set<std::string> choosen_transfers;
            while (true)
            {
                std::string input;
                std::cin >> input;
                if (input == "q")
                {
                    break;
                }
                // Check input
                if (m_clients_per_grp_transfer.find(input) == m_clients_per_grp_transfer.end())
                {
                    TLOG() << "Invalid transfer";
                    break;
                }
                choosen_transfers.emplace(input);
            }
            if (choosen_transfers.size() == 0)
            {
                TLOG() << "No transfer selected";
                break;
            }

            for (const auto &transfer : choosen_transfers)
            {
                start_transfers(transfer);
            }
            break;
        }

        default:
            TLOG() << "Unknown command";
            break;
        }
    }

    void Bookkeeper::start_transfers(const std::string &transfer_id)
    {
        TLOG() << "Starting transfer " << transfer_id;

        if (m_clients_per_grp_transfer.find(transfer_id) != m_clients_per_grp_transfer.end())
        {
            for (const std::string &client : m_clients_per_grp_transfer[transfer_id])
            {
                std::string session_name = client;
                session_name += "_ses";
                session_name += transfer_id;
                send_notification(notification_type::e_notification_type::START_TRANSFER, get_bookkeeper_id(), session_name, client);
            }
        }
        else
        {
            ers::warning(InvalidGroupTransferIDError(ERS_HERE, transfer_id, get_bookkeeper_id()));
        }
    }

    void Bookkeeper::do_work(std::atomic<bool> &running_flag)
    {
        // Just one request on startup, after that the clients will have to send by themself
        for (const std::string &client : get_clients_conn())
        {
            request_connection_and_available_files(client);
        }

        auto time_point = std::chrono::high_resolution_clock::now();

        while (running_flag.load())
        {

            std::optional<NotificationData> msg = listen_for_notification(get_bookkeepers_conn().front());
            if (msg.has_value())
            {
                action_on_receive_notification(msg.value());
            }

            // check alives clients and available files
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Auto update metadata every 2 seconds
            if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - time_point).count() >= m_refresh_rate)
            {
                time_point = std::chrono::high_resolution_clock::now();
                request_update_metadata();
                display_information();
            }
        }
    }

    bool Bookkeeper::start()
    {
        auto time_point = std::chrono::high_resolution_clock::now();

        // Just one request on startup, after that the clients will have to send by themself
        for (const std::string &client : get_clients_conn())
        {
            request_connection_and_available_files(client);
        }

        while (true)
        {
            std::optional<NotificationData> msg = listen_for_notification(get_bookkeepers_conn().front());
            if (msg.has_value())
            {
                action_on_receive_notification(msg.value());
            }

            std::string input;
            getline(std::cin, input);
            if (input.empty() == false)
            {
                input_action(input[0]);
            }

            // check alives clients and available files
            for (const std::string &client : get_clients_conn())
            {
                if (m_transfers.find(client) != m_transfers.end())
                {
                    // already known client
                    continue;
                }

                request_connection_and_available_files(client);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Auto update metadata every 2 seconds
            if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - time_point).count() >= 5)
            {
                time_point = std::chrono::high_resolution_clock::now();
                request_update_metadata();
                display_information();
            }
        }
    }

    void Bookkeeper::request_connection_and_available_files(const std::string &client)
    {
        // send connection request to client
        send_notification(notification_type::e_notification_type::CONNECTION_REQUEST, get_bookkeeper_id(), client, client, get_bookkeeper_id(), 1);

        // Listen to receive connection response and available files
        // auto msg = listen_for_notification(get_bookkeepers_conn().front(), client);

        // while (msg.has_value() && msg.value().m_notification != notification_type::notification_to_string(notification_type::e_notification_type::CONNECTION_REQUEST))
        // {
        //     action_on_receive_notification(msg.value());
        //     msg = listen_for_notification(get_bookkeepers_conn().front(), client);
        // }
    }

    void Bookkeeper::request_update_metadata(bool force)
    {
        for (const auto &[id, g] : get_grp_transfers())
        {
            // Only request for dynamic status
            if (g->get_group_status() == status_type::e_status::DOWNLOADING ||
                g->get_group_status() == status_type::e_status::CHECKING ||
                g->get_group_status() == status_type::e_status::UPLOADING ||
                g->get_group_status() == status_type::e_status::HASHING ||
                force)
            {

                for (const std::string &session : m_clients_per_grp_transfer[g->get_group_id()])
                {
                    send_notification(notification_type::e_notification_type::UPDATE_REQUEST, get_bookkeeper_id(), session, get_client_name_from_session_name(session));
                }
            }
        }
    }

    bool Bookkeeper::action_on_receive_notification(NotificationData notif)
    {
        TLOG() << "debug : Action on request " << notif.m_notification << " for " << notif.m_target_id;

        if (notif.m_target_id.find(get_bookkeeper_id()) == std::string::npos && notif.m_target_id != "all")
        {
            ers::warning(NotificationWrongDestinationError(ERS_HERE, get_bookkeeper_id(), notif.m_source_id, notif.m_target_id));
            return false;
        }

        // Use enum cast for converting string to enum, still working with older clients and user readable
        auto action = notification_type::string_to_notification(notif.m_notification);

        if (action.has_value() == false)
        {
            ers::warning(InvalidNotificationReceivedError(ERS_HERE, get_bookkeeper_id(), notif.m_source_id, notif.m_notification));
        }

        switch (action.value())
        {
        case notification_type::e_notification_type::TRANSFER_METADATA:
        {
            if (notif.m_data == "end")
            {
                // Create entry in the map in case no files
                m_transfers[notif.m_source_id];
                break;
            }

            // Store it
            add_update_transfer(notif.m_source_id, notif.m_data);
            break;
        }

        case notification_type::e_notification_type::TRANSFER_ERROR:
        case notification_type::e_notification_type::GROUP_METADATA:
        {
            // Loading the data and cnovert to a proper transfer metadata object
            auto *tmeta = new GroupMetadata(notif.m_data, false);

            // Store it
            m_clients_per_grp_transfer[tmeta->get_group_id()].insert(notif.m_source_id);
            add_update_grp_transfer(tmeta);
            break;
        }

        default:
            ers::warning(NotHandledNotificationError(ERS_HERE, get_bookkeeper_id(), notif.m_source_id, notif.m_notification));
        }
        return true;
    }

    void Bookkeeper::display_information()
    {
        std::ostream *output = nullptr;
        std::ostream *output_line_log = nullptr;
        std::string sep = ";";

        if (m_file_log_path != "")
        {
            // open file
            output = new std::ofstream();
            output_line_log = new std::ofstream();
            dynamic_cast<std::ofstream *>(output)->open(m_file_log_path + get_bookkeeper_id() + ".log", std::fstream::out);
            dynamic_cast<std::ofstream *>(output_line_log)->open(m_file_log_path + get_bookkeeper_id() + "_line.csv", std::fstream::app | std::fstream::out);
            // clear file
            dynamic_cast<std::ofstream *>(output)->clear();
            TLOG() << "debug : output log wroten "
                   << m_file_log_path << get_bookkeeper_id() << ".log\t"
                   << m_file_log_path << get_bookkeeper_id() << "_line.csv";

            // if csv file empty, write header
            if (dynamic_cast<std::ofstream *>(output_line_log)->tellp() == 0)
            {
                *output_line_log << "time" << sep
                                 << "file_name" << sep
                                 << "file_full_path" << sep
                                 << "group_id" << sep
                                 << "src_ip" << sep
                                 << "size" << sep

                                 << "dest_ip" << sep
                                 << "start_time" << sep
                                 << "duration" << sep
                                 << "progress" << sep
                                 << "speed" << sep
                                 << "state" << sep

                                 << "end_time" << sep
                                 << "error" << sep
                                 << std::endl;
            }
        }
        else
        {
            output = &std::cout;
        }

        *output << "***** Bookkeeper " << get_bookkeeper_id() << " " + get_ip().get_ip_port() << " informations display *****" << std::endl;
        // *output << "q: quit, d : display info, n : new transfer, s : start transfer" << std::endl;
        *output << "Connected clients :" << std::endl;

        for (const auto &client : get_transfers())
        {
            bool is_session = false;
            // If it's a session
            if (client.first.find("ses") != std::string::npos)
            {
                *output << "\t* Session " << client.first << " is active" << std::endl;
                is_session = true;
            }
            else
            {
                *output << "> Client " << client.first << " is connected" << std::endl;
            }

            // print for each file the status
            for (const auto &file : client.second)
            {
                if (m_file_log_path != "")
                {
                    *output_line_log << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() << sep
                                     << file->get_file_name() << sep
                                     << file->get_file_path() << sep
                                     << file->get_group_id() << sep
                                     << file->get_src().get_ip_port() << sep
                                     << file->get_size() << sep

                                     << file->get_dest().get_ip_port() << sep
                                     << file->get_start_time_str() << sep
                                     << file->get_total_duration_ms() << sep
                                     << file->get_progress() << sep
                                     << file->get_transmission_speed() << sep
                                     << status_type::status_to_string(file->get_status()) << sep

                                     << file->get_end_time_str() << sep
                                     << file->get_error_code() << sep

                                     << std::endl;
                }

                if (is_session)
                {
                    *output << "\t\t - ";
                }
                else
                {
                    *output << "\t - ";
                }

                if (is_session)
                    *output << file->get_file_name() << "\t"
                            << file->get_size() << " bytes\tfrom "
                            << file->get_src().get_ip_port() << "\t"
                            // << file->get_dest().get_ip_port() << "\t"
                            << status_type::status_to_string(file->get_status()) << "\t"
                            << file->get_progress() << "%\t"
                            << (file->get_transmission_speed() == 0 ? "-" : std::to_string(file->get_transmission_speed())) << "Bi/s\t"
                            << file->get_start_time_str() << "\t"
                            << file->get_total_duration_ms() << "ms\t"
                            << file->get_end_time_str() << "\t"
                            << file->get_error_code() << "\t"
                            << std::endl;
                else
                    *output << "Available file "
                            << file->get_file_path() << "\t"
                            << file->get_size() << " bytes\tfrom "
                            << file->get_src().get_ip_port() << "\t"
                            << std::endl;
            }
        }

        *output << std::endl
                << "Active Transfers :" << std::endl;

        *output << "Group ID\t"
                << "Protocol\t"
                << "Src\t"
                << "IP\t"
                << "Status\t"
                << std::endl;

        for (const auto &[id, g] : get_grp_transfers())
        {
            *output << g->get_group_id() << "\t"
                    << protocol_type::protocols_to_string(g->get_protocol()) << "\t"
                    << g->get_source_id() << "\t"
                    << g->get_source_ip().get_ip_port() << "\t"
                    << status_type::status_to_string(g->get_group_status()) << "\t"
                    << std::endl;

            for (const TransferMetadata &fmeta : g->get_transfers_meta())
            {
                *output << "\t- "
                        << fmeta.get_file_name() << "\t"
                        << fmeta.get_src().get_ip_port() << " to "
                        << fmeta.get_dest().get_ip_port() << "\t"
                        << status_type::status_to_string(fmeta.get_status()) << "\t"
                        << std::endl;
            }

            for (const std::string &f : g->get_expected_files())
            {
                *output << "\t- "
                        << f << "\t"
                        << "Expected"
                        << std::endl;
            }
        }

        if (m_file_log_path != "")
        {
            dynamic_cast<std::ofstream *>(output)->close();
            dynamic_cast<std::ofstream *>(output_line_log)->close();
        }
        else
        {
            output->flush();
        }
    }

    void Bookkeeper::add_update_transfer(const std::string &client_id, const std::string &data)
    {
        // Loading the data and convert to a proper transfer metadata object
        TransferMetadata file = TransferMetadata(data, false);

        std::vector<TransferMetadata *> &tr_vector = m_transfers[client_id];
        for (TransferMetadata *tr : tr_vector)
        {
            if (*tr == file)
            {
                // Already inserted, simply update the one already present
                tr->from_string(data);
                return;
            }
        }

        std::string group_id_tmp = file.get_group_id();

        // Check if transfer already exist in a group transfer
        if (m_grp_transfers.find(group_id_tmp) != m_grp_transfers.end())
        {
            m_grp_transfers[group_id_tmp]->add_file(file);
            m_transfers[client_id].push_back(&m_grp_transfers[group_id_tmp]->get_transfers_meta().back());
        }
    }

    void Bookkeeper::add_update_grp_transfer(GroupMetadata *grp_transfers)
    {
        if (m_grp_transfers.find(grp_transfers->get_group_id()) != m_grp_transfers.end())
        {
            // Already inserted, copy old values
            grp_transfers->set_transfers_meta(m_grp_transfers[grp_transfers->get_group_id()]->get_transfers_meta());
            grp_transfers->set_expected_files(m_grp_transfers[grp_transfers->get_group_id()]->get_expected_files());
        }
        m_grp_transfers[grp_transfers->get_group_id()] = grp_transfers;
    }
} // namespace dunedaq::snbmodules