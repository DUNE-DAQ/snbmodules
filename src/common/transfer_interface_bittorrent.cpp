/**
 * @file transfer_interface_bittorrent.cpp TransferInterfaceRClone protocol class for a Bittorrent transfer
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "snbmodules/interfaces/transfer_interface_bittorrent.hpp"

#include <vector>
#include <utility>
#include <string>

namespace dunedaq::snbmodules
{

    // return the name of a torrent status enum
    char const *TransferInterfaceBittorrent::state(lt::torrent_status::state_t s)
    {
        switch (s)
        {
        case lt::torrent_status::checking_files:
            return "checking";
        case lt::torrent_status::downloading_metadata:
            return "dl metadata";
        case lt::torrent_status::downloading:
            return "downloading";
        case lt::torrent_status::finished:
            return "finished";
        case lt::torrent_status::seeding:
            return "seeding";
        case lt::torrent_status::checking_resume_data:
            return "checking resume";
        default:
            return "<>";
        }
    }

    TransferInterfaceBittorrent::TransferInterfaceBittorrent(GroupMetadata &config, bool is_client, std::filesystem::path work_dir, const IPFormat &listening_ip)
        : TransferInterfaceAbstract(config),
          ses(set_settings(listening_ip, config.get_protocol_options()["port"].get<std::string>())),
          m_is_client(is_client),
          m_listening_ip(listening_ip),
          m_thread([&](std::atomic<bool> &running)
                   { this->do_work(running); })
    {
        m_work_dir = std::move(work_dir);
        m_thread.start_working_thread();

        if (config.get_protocol_options().contains("rate_limit"))
        {
            m_rate_limit = config.get_protocol_options()["rate_limit"].get<int>();
        }
    }

    TransferInterfaceBittorrent::~TransferInterfaceBittorrent()
    {
        m_thread.stop_working_thread();
    }

    void TransferInterfaceBittorrent::do_work(std::atomic<bool> &running_flag)
    try
    {
        bool m_done = false;
        int finished_torrents = 0;
        // lt::torrent_handle h;

        FILE *log_file = std::fopen(get_work_dir().append("bittorrent.log").c_str(), "w+");

        TLOG() << "debug : Starting bittorent work on " << m_listening_ip.get_ip_port();

        while (running_flag.load() || save_on_exit)
        {

            // Save before exit
            if (save_on_exit && !running_flag.load())
            {
                auto const handles = ses.get_torrents();
                for (const auto &h : handles)
                {
                    h.save_resume_data(lt::torrent_handle::only_if_modified | lt::torrent_handle::save_info_dict);
                }
                m_done = true;
                goto done;
            }

            std::vector<lt::alert *> alerts;
            ses.pop_alerts(&alerts);

            for (lt::alert const *a : alerts)
            {
                static auto const first_ts = a->timestamp();

                if (log_file)
                {
                    std::fprintf(log_file, "[%ld] %s\n", static_cast<std::int64_t>(duration_cast<std::chrono::milliseconds>(a->timestamp() - first_ts).count()), a->message().c_str());
                }

                if (auto at = lt::alert_cast<lt::add_torrent_alert>(a))
                {
                    // h = at->handle;
                    m_torrent_num++;
                    TLOG() << "debug : Added torrent " << at->torrent_name();
                }

                if (auto p = lt::alert_cast<lt::torrent_removed_alert>(a))
                {
                    lt::torrent_handle h = p->handle;
                }

                if (auto p = lt::alert_cast<lt::torrent_paused_alert>(a))
                {
                    lt::torrent_handle h = p->handle;
                    h.save_resume_data(lt::torrent_handle::save_info_dict);
                }

                if (auto *p = lt::alert_cast<lt::tracker_list_alert>(a))
                {
                    (void)p;
                    // if (h == p->handle)
                    // {
                    //     session_state.trackers = std::move(p->trackers);
                    // }
                }

                // if (auto *p = lt::alert_cast<lt::file_progress_alert>(a))
                // {
                //     m_filename_to_metadata[h.torrent_file()->name()]->set_bytes_transferred(h.status().total_payload_download);
                //     TLOG() << "debug : Progress: " << h.status().total_payload_download;
                // }

                // if we receive the finished alert or an error, we're done
                if (auto p = lt::alert_cast<lt::torrent_finished_alert>(a))
                {
                    TLOG() << "debug : Torrent finished " << p->torrent_name();
                    finished_torrents++;

                    p->handle.save_resume_data(lt::torrent_handle::only_if_modified | lt::torrent_handle::save_info_dict);

                    m_filename_to_metadata[p->torrent_name()]->set_status(status_type::e_status::FINISHED);
                    m_filename_to_metadata[p->torrent_name()]->set_bytes_transferred(m_filename_to_metadata[p->torrent_name()]->get_size());

                    if (finished_torrents == m_torrent_num && m_is_client)
                    {
                        m_done = true;
                    }
                }
                if (auto p = lt::alert_cast<lt::torrent_error_alert>(a))
                {
                    ers::error(BittorrentError(ERS_HERE, p->error.message()));

                    finished_torrents++;
                    if (finished_torrents == m_torrent_num && m_is_client)
                    {
                        m_done = true;
                    }

                    p->handle.save_resume_data(lt::torrent_handle::only_if_modified | lt::torrent_handle::save_info_dict);
                }

                // when resume data is ready, save it
                if (const auto *rd = lt::alert_cast<lt::save_resume_data_alert>(a))
                {
                    std::ofstream of(get_work_dir().append(".resume_file_" + rd->params.name), std::ios_base::binary);
                    of.unsetf(std::ios_base::skipws);
                    auto const b = write_resume_data_buf(rd->params);
                    of.write(b.data(), static_cast<int>(b.size()));
                    if (m_done)
                    {
                        goto done;
                    }
                }

                if (auto e = lt::alert_cast<lt::save_resume_data_failed_alert>(a))
                {
                    ers::warning(BittorrentSaveResumeFileError(ERS_HERE, e->message()));
                    if (m_done)
                    {
                        goto done;
                    }
                }

                if (lt::alert_cast<lt::peer_connect_alert>(a))
                {
                    m_peer_num++;
                    m_done = false;
                }

                if (auto e = lt::alert_cast<lt::peer_error_alert>(a))
                {
                    m_peer_num--;
                    // std::this_thread::sleep_for(std::chrono::seconds(1));
                    // Try to reconnect
                    // lt::error_code ec;
                    // h.connect_peer(lt::tcp::endpoint(boost::asio::ip::make_address(config->get_source_ip().get_ip(), ec), std::uint16_t(config->get_source_ip().get_port())));
                    ers::warning(BittorrentPeerDisconnectedError(ERS_HERE, e->message()));
                    // f_meta.set_error_code("peer error: " + a->message());

                    if (m_peer_num == 0 && m_paused == 0 && !m_is_client)
                    {
                        m_done = true;
                    }
                }

                if (auto e = lt::alert_cast<lt::peer_disconnected_alert>(a))
                {
                    m_peer_num--;
                    // wait
                    // std::this_thread::sleep_for(std::chrono::seconds(1));
                    // Try to reconnect
                    // lt::error_code ec;
                    // h.connect_peer(lt::tcp::endpoint(boost::asio::ip::make_address(config->get_source_ip().get_ip(), ec), std::uint16_t(config->get_source_ip().get_port())));
                    ers::warning(BittorrentPeerDisconnectedError(ERS_HERE, e->message()));
                    if (m_peer_num <= 0 && m_paused == 0 && !m_is_client)
                    {
                        m_done = true;
                    }
                }

                if (auto st = lt::alert_cast<lt::state_update_alert>(a))
                {
                    // TLOG() << "debug : State update alert";
                    if (st->status.empty())
                    {
                        continue;
                    }

                    for (uint64_t i = 0; i < st->status.size(); i++)
                    {
                        lt::torrent_status const &s = st->status[i];

                        if (m_filename_to_metadata[s.name]->get_status() != status_type::e_status::PAUSED)
                        {

                            switch (s.state)
                            {
                            case lt::torrent_status::checking_files:
                                m_filename_to_metadata[s.name]->set_status(status_type::e_status::CHECKING);
                                break;
                            case lt::torrent_status::downloading_metadata:
                                m_filename_to_metadata[s.name]->set_status(status_type::e_status::PREPARING);
                                break;
                            case lt::torrent_status::downloading:
                                m_filename_to_metadata[s.name]->set_status(status_type::e_status::DOWNLOADING);
                                break;
                            case lt::torrent_status::finished:
                                m_filename_to_metadata[s.name]->set_status(status_type::e_status::FINISHED);
                                break;
                            case lt::torrent_status::seeding:
                                if (m_is_client)
                                {
                                    m_filename_to_metadata[s.name]->set_status(status_type::e_status::FINISHED);
                                }
                                else
                                {
                                    m_filename_to_metadata[s.name]->set_status(status_type::e_status::UPLOADING);
                                }
                                break;
                            case lt::torrent_status::checking_resume_data:
                                m_filename_to_metadata[s.name]->set_status(status_type::e_status::CHECKING);
                                break;
                            default:
                                break;
                            }

                            // if (s.num_peers == 0)
                            // {
                            //     m_filename_to_metadata[s.name].set_status(status_type::e_status::WAITING);
                            // }
                        }

                        m_filename_to_metadata[s.name]->set_bytes_transferred(s.total_done);

                        TLOG() << "is_client " << m_is_client
                               << " [" << i << "]" << s.name << " " << state(s.state) << ' '
                               << (s.download_payload_rate / 1000) << " kB/s "
                               << (s.total_done / 1000) << " kB ("
                               << (s.progress_ppm / 10000) << "%) "
                               << s.current_tracker << " " << static_cast<std::int64_t>(duration_cast<seconds>(s.next_announce).count()) << "s ("
                               << s.num_peers << " peers) "
                               << "\n";

                        // h.post_trackers();

                        // for (lt::announce_entry const &ae : session_state.trackers)
                        // {
                        //     std::cout << ae.url << " ";
                        //     if (ae.verified)
                        //     {
                        //         std::cout << "OK ";
                        //     }
                        //     else
                        //     {
                        //         std::cout << "-- ";
                        //     }
                        //     std::cout << ae.tier << "\n";
                        // }
                        // auto &peers = client_state.peers;
                        // if (print_peers && !peers.empty())
                        // {
                        //     using lt::peer_info;
                        //     // sort connecting towards the bottom of the list, and by peer_id
                        //     // otherwise, to keep the list as stable as possible
                        //     std::sort(peers.begin(), peers.end(), [](peer_info const &lhs, peer_info const &rhs)
                        //               {
                        //         {
                        //             bool const l = bool(lhs.flags & peer_info::connecting);
                        //             bool const r = bool(rhs.flags & peer_info::connecting);
                        //             if (l != r) return l < r;
                        //         }

                        //         {
                        //             bool const l = bool(lhs.flags & peer_info::handshake);
                        //             bool const r = bool(rhs.flags & peer_info::handshake);
                        //             if (l != r) return l < r;
                        //         }

                        //         return lhs.pid < rhs.pid; });

                        //     print_peer_info(out, peers, 10);
                        //     if (print_peers_legend)
                        //     {
                        //         print_peer_legend(out, 10);
                        //     }
                        // }

                        // if (print_trackers)
                        // {
                        //     snprintf(str, sizeof(str), "next_announce: %4" PRId64 " | current tracker: %s\x1b[K\n", std::int64_t(duration_cast<seconds>(s.next_announce).count()), s.current_tracker.c_str());
                        //     out += str;
                        //     pos += 1;
                        //     h.post_trackers();
                        //     for (lt::announce_entry const &ae : client_state.trackers)
                        //     {
                        //         std::snprintf(str, sizeof(str), "%2d %-55s %s\x1b[K\n", ae.tier, ae.url.c_str(), ae.verified ? "OK " : "-  ");
                        //         out += str;
                        //         pos += 1;
                        //         int idx = 0;
                        //         for (auto const &ep : ae.endpoints)
                        //         {
                        //             ++idx;
                        //             if (pos + 1 >= terminal_height)
                        //             {
                        //                 break;
                        //             }
                        //             if (!ep.enabled)
                        //             {
                        //                 continue;
                        //             }
                        //             for (lt::protocol_version const v : {lt::protocol_version::V1, lt::protocol_version::V2})
                        //             {
                        //                 if (!s.info_hashes.has(v))
                        //                 {
                        //                     continue;
                        //                 }
                        //                 auto const &av = ep.info_hashes[v];

                        //                 std::snprintf(str, sizeof(str), "  [%2d] %s fails: %-3d (%-3d) %s %5d \"%s\" %s\x1b[K\n", idx, v == lt::protocol_version::V1 ? "v1" : "v2", av.fails, ae.fail_limit, to_string(int(total_seconds(av.next_announce - now)), 8).c_str(), av.min_announce > now ? int(total_seconds(av.min_announce - now)) : 0, av.last_error ? av.last_error.message().c_str() : "", av.message.c_str());
                        //                 out += str;
                        //                 pos += 1;
                        //                 // we only need to show this error once, not for every
                        //                 // endpoint
                        //                 if (av.last_error == boost::asio::error::host_not_found)
                        //                 {
                        //                     goto tracker_done;
                        //                 }
                        //             }
                        //         }
                        //     tracker_done:

                        //         if (pos + 1 >= terminal_height)
                        //         {
                        //             break;
                        //         }
                        //     }
                        // }
                    }
                    // std::cout << "\x1b[K";
                    // std::cout.flush();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            // ask the session to post a state_update_alert, to update our
            // state output for the torrent
            ses.post_torrent_updates();
            ses.post_session_stats();

            // save resume data once every 30 seconds
            // if (clk::now() - last_save_resume > std::chrono::seconds(30))
            // {
            //     h.save_resume_data(lt::torrent_handle::only_if_modified | lt::torrent_handle::save_info_dict);
            //     last_save_resume = clk::now();
            // }
        }
    done:

        for (auto &[k, s] : m_filename_to_metadata)
        {
            if (!m_is_client)
            {
                s->set_status(status_type::e_status::FINISHED);
                // deleting torrents files
                std::filesystem::remove(get_work_dir().append(k + ".torrent"));
            }
            else if (s->get_status() != status_type::e_status::FINISHED)
            {
                s->set_status(status_type::e_status::ERROR);
                s->set_error_code("Transfer interrupted");
            }
        }

        TLOG() << "\nBittorent session done, shutting down";
        if (log_file)
        {
            std::fclose(log_file);
        }
        return;
    }
    catch (std::exception &e)
    {
        // TODO: handle error
        std::cerr << "Error: " << e.what() << std::endl;
    }

    bool TransferInterfaceBittorrent::add_magnet(lt::string_view uri, const std::filesystem::path &dest)
    try
    {
        TLOG() << "debug : loading parameters from magnet " << uri.to_string();
        lt::error_code ec;
        lt::add_torrent_params p = lt::parse_magnet_uri(uri.to_string(), ec);

        if (ec)
        {
            ers::error(BittorrentInvalidMagnetLinkError(ERS_HERE, uri.to_string()));
            return false;
        }

        // std::vector<char> resume_data;
        // if (load_file(resume_file(p.info_hashes, session_number), resume_data))
        // {
        //     p = lt::read_resume_data(resume_data, ec);
        //     if (ec)
        //     {
        //         std::printf("  failed to load resume data: %s\n", ec.message().c_str());
        //     }
        // }

        set_torrent_params(p, dest);

        TLOG() << "debug : adding torrent";
        ses.async_add_torrent(std::move(p));
        return true;
    }
    catch (lt::system_error const &e)
    {
        ers::error(BittorrentInvalidMagnetLinkError(ERS_HERE, e.code().message()));
        return false;
    }

    // return magnet url
    std::string TransferInterfaceBittorrent::add_torrent(const std::string &torrent, const std::filesystem::path &dest)
    try
    {
        using lt::storage_mode_t;

        TLOG() << "debug : [" << m_torrent_num << "] " << torrent;

        // lt::error_code ec;
        lt::add_torrent_params p = lt::load_torrent_file(torrent);

        // std::vector<char> resume_data;
        // if (load_file(resume_file(atp.info_hashes, session_number), resume_data))
        // {
        //     lt::add_torrent_params rd = lt::read_resume_data(resume_data, ec);
        //     if (ec)
        //     {
        //         std::printf("  failed to load resume data: %s\n", ec.message().c_str());
        //     }
        //     else
        //     {
        //         atp = rd;
        //     }
        // }

        set_torrent_params(p, dest);
        std::string magnet = lt::make_magnet_uri(p);

        ses.async_add_torrent(std::move(p));

        return magnet;
    }
    catch (lt::system_error const &e)
    {
        ers::error(BittorrentInvalidTorrentFileError(ERS_HERE, e.code().message()));
        return "";
    }

    void TransferInterfaceBittorrent::set_torrent_params(lt::add_torrent_params &p, const std::filesystem::path &dest)
    {
        TLOG() << "debug : setting torrent parameters";

        bool seed_mode = !m_is_client;
        bool super_seeding = false;
        bool upload_mode = !m_is_client;

        bool sequential_mode = true;
        int max_connections_per_torrent = 100;
        std::string save_path = dest;
        // limits in bytes per seconds hqndle by session ?
        int torrent_upload_limit = -1;
        int torrent_download_limit = m_rate_limit;

        p.max_connections = max_connections_per_torrent;
        p.max_uploads = -1;
        p.upload_limit = torrent_upload_limit;
        p.download_limit = torrent_download_limit;
        // atp.flags &= ~lt::torrent_flags::duplicate_is_error;

        // not auto managed
        p.flags &= ~lt::torrent_flags::paused;
        p.flags &= ~lt::torrent_flags::auto_managed;

        if (super_seeding)
        {
            p.flags |= lt::torrent_flags::super_seeding;
        }
        else
        {
            p.flags &= ~lt::torrent_flags::super_seeding;
        }

        if (seed_mode)
        {
            p.flags |= lt::torrent_flags::seed_mode;
        }
        else
        {
            p.flags &= ~lt::torrent_flags::seed_mode;
        }

        if (upload_mode)
        {
            p.flags |= lt::torrent_flags::upload_mode;
        }
        else
        {
            p.flags &= ~lt::torrent_flags::upload_mode;
        }

        if (sequential_mode)
        {
            p.flags |= lt::torrent_flags::sequential_download;
        }
        else
        {
            p.flags &= ~lt::torrent_flags::sequential_download;
        }

        p.save_path = save_path;
        p.storage_mode = lt::storage_mode_allocate;
    }

    lt::session_params TransferInterfaceBittorrent::set_settings(const IPFormat &listen_interface, const std::string &listen_port)
    {
        lt::session_params sp;
        auto &p = sp.settings;

        sp.disk_io_constructor = lt::default_disk_io_constructor;

        int block_size = 1024 * 1024;
        std::string outgoing_interface = listen_interface.get_ip();
        std::string listen_interfaces = listen_interface.get_ip() + ":" + listen_port;

        p.set_bool(lt::settings_pack::enable_dht, false);
        p.set_int(lt::settings_pack::auto_manage_interval, 60);
        p.set_int(lt::settings_pack::auto_manage_startup, 1);
        p.set_int(lt::settings_pack::min_reconnect_time, 1);
        p.set_int(lt::settings_pack::max_failcount, 10);

        // p.set_str(lt::settings_pack::dht_bootstrap_nodes, "192.168.0.105:5001,192.168.0.106:5002");
        // p.set_bool(lt::settings_pack::use_dht_as_fallback, false);
        // p.set_bool(lt::settings_pack::dht_prefer_verified_node_ids, false);
        // p.set_bool(lt::settings_pack::dht_restrict_routing_ips, false);
        // p.set_bool(lt::settings_pack::dht_restrict_search_ips, false);

        p.set_str(lt::settings_pack::outgoing_interfaces, outgoing_interface);
        p.set_bool(lt::settings_pack::strict_end_game_mode, false);
        // p.set_bool(lt::settings_pack::low_prio_disk, false);
        p.set_bool(lt::settings_pack::smooth_connects, false);
        p.set_bool(lt::settings_pack::allow_multiple_connections_per_ip, true);
        p.set_bool(lt::settings_pack::announce_to_all_tiers, true);
        p.set_bool(lt::settings_pack::announce_to_all_trackers, true);
        p.set_bool(lt::settings_pack::auto_sequential, true);
        // p.set_bool(lt::settings_pack::coalesce_reads, true);
        // p.set_bool(lt::settings_pack::coalesce_writes, true);
        // p.set_bool(lt::settings_pack::contiguous_recv_buffer, true);
        p.set_bool(lt::settings_pack::incoming_starts_queued_torrents, true);

        p.set_bool(lt::settings_pack::enable_incoming_tcp, true);
        p.set_bool(lt::settings_pack::enable_outgoing_tcp, true);
        p.set_bool(lt::settings_pack::enable_incoming_utp, false);
        p.set_bool(lt::settings_pack::enable_outgoing_utp, false);
        p.set_bool(lt::settings_pack::enable_lsd, false);
        p.set_bool(lt::settings_pack::enable_natpmp, false);
        p.set_bool(lt::settings_pack::enable_upnp, false);
        p.set_bool(lt::settings_pack::prefer_rc4, false);
        p.set_bool(lt::settings_pack::prefer_udp_trackers, true);
        p.set_bool(lt::settings_pack::rate_limit_ip_overhead, false);
        // p.set_bool(lt::settings_pack::rate_limit_utp, false);

        p.set_int(lt::settings_pack::aio_threads, 1);
        // p.set_int(lt::settings_pack::network_threads, 1);
        p.set_int(lt::settings_pack::hashing_threads, 1);
        p.set_int(lt::settings_pack::disk_io_read_mode, 3);
        p.set_int(lt::settings_pack::disk_io_write_mode, 3);
        p.set_int(lt::settings_pack::allowed_enc_level, 3);
        p.set_int(lt::settings_pack::allowed_fast_set_size, 5);
        p.set_int(lt::settings_pack::seed_choking_algorithm, 1);
        p.set_int(lt::settings_pack::choking_algorithm, 0);
        p.set_int(lt::settings_pack::in_enc_policy, 2);
        p.set_int(lt::settings_pack::out_enc_policy, 2);
        p.set_int(lt::settings_pack::mixed_mode_algorithm, 0);
        p.set_int(lt::settings_pack::suggest_mode, 0);

        p.set_int(lt::settings_pack::close_file_interval, 0);
        p.set_int(lt::settings_pack::inactivity_timeout, 10);
        p.set_int(lt::settings_pack::request_queue_time, 50);
        p.set_int(lt::settings_pack::peer_timeout, 20);
        p.set_int(lt::settings_pack::request_timeout, 10);
        p.set_int(lt::settings_pack::predictive_piece_announce, 20);
        p.set_int(lt::settings_pack::whole_pieces_threshold, 20);
        p.set_int(lt::settings_pack::mmap_file_size_cutoff, 0);

        // limits in bytes per seconds
        p.set_int(lt::settings_pack::upload_rate_limit, 0);
        p.set_int(lt::settings_pack::download_rate_limit, 0);
        // p.set_int(lt::settings_pack::local_download_rate_limit, 0);
        // p.set_int(lt::settings_pack::local_upload_rate_limit, 0);
        p.set_int(lt::settings_pack::unchoke_slots_limit, -1);
        p.set_int(lt::settings_pack::max_failcount, 3);
        p.set_int(lt::settings_pack::max_http_recv_buffer_size, 1024 * 1024 * 8);
        p.set_int(lt::settings_pack::max_rejects, 20);
        p.set_int(lt::settings_pack::max_queued_disk_bytes, 1024 * 1024 * 1024);

        p.set_int(lt::settings_pack::read_cache_line_size, 512);
        // p.set_int(lt::settings_pack::cache_buffer_chunk_size, 512);
        // p.set_int(lt::settings_pack::cache_expiry, 400);
        p.set_int(lt::settings_pack::cache_size_volatile, 128);
        p.set_int(lt::settings_pack::checking_mem_usage, 1024);
        // p.set_bool(lt::settings_pack::use_read_cache, true);
        // p.set_bool(lt::settings_pack::use_write_cache, true);
        // p.set_bool(lt::settings_pack::use_disk_read_ahead, true);
        p.set_bool(lt::settings_pack::use_parole_mode, false);
        // p.set_bool(lt::settings_pack::guided_read_cache, true);
        // p.set_bool(lt::settings_pack::volatile_read_cache, false);
        p.set_int(lt::settings_pack::tracker_completion_timeout, 30);
        p.set_int(lt::settings_pack::tracker_receive_timeout, 30);
        p.set_int(lt::settings_pack::stop_tracker_timeout, 30);
        p.set_int(lt::settings_pack::tracker_backoff, 250);
        p.set_int(lt::settings_pack::tracker_maximum_response_length, 1024 * 1024 * 8);
        p.set_bool(lt::settings_pack::validate_https_trackers, false);
        p.set_int(lt::settings_pack::alert_mask, lt::alert_category::all);

        p.set_str(lt::settings_pack::listen_interfaces, listen_interfaces);

        if (m_is_client)
        {
            // p.set_str(lt::settings_pack::user_agent, "DuneTorrentClient");
            p.set_bool(lt::settings_pack::piece_extent_affinity, true);
            p.set_bool(lt::settings_pack::seeding_outgoing_connections, false);

            p.set_int(lt::settings_pack::tick_interval, 500);
            p.set_int(lt::settings_pack::torrent_connect_boost, 255);

            p.set_int(lt::settings_pack::connection_speed, 0);
            p.set_int(lt::settings_pack::active_seeds, 0);
            p.set_int(lt::settings_pack::active_downloads, 10);
            p.set_int(lt::settings_pack::active_checking, 10);
            p.set_int(lt::settings_pack::active_limit, 10);
            p.set_int(lt::settings_pack::active_tracker_limit, 10);
            p.set_int(lt::settings_pack::connections_limit, 10);
            // p.set_int(lt::settings_pack::half_open_limit, 500);
            p.set_int(lt::settings_pack::file_pool_size, 10);
            p.set_int(lt::settings_pack::listen_queue_size, 10);
            p.set_int(lt::settings_pack::max_allowed_in_request_queue, 50000);
            p.set_int(lt::settings_pack::max_out_request_queue, 50000);
            p.set_int(lt::settings_pack::dht_upload_rate_limit, block_size * 10);

            p.set_int(lt::settings_pack::write_cache_line_size, 512);
            // p.set_int(lt::settings_pack::cache_size, 1024 * 1);
            // p.set_bool(lt::settings_pack::use_disk_cache_pool, true);
            // p.set_bool(lt::settings_pack::allow_partial_disk_writes, false);

            p.set_int(lt::settings_pack::send_buffer_watermark_factor, 50);
            p.set_int(lt::settings_pack::send_buffer_low_watermark, 1024 * 10);
            p.set_int(lt::settings_pack::send_buffer_watermark, 1024 * 500);
            p.set_int(lt::settings_pack::send_socket_buffer_size, 1024 * 512);
            p.set_int(lt::settings_pack::recv_socket_buffer_size, 1024 * 1024 * 1024);

            p.set_bool(lt::settings_pack::no_atime_storage, true);
            p.set_bool(lt::settings_pack::enable_set_file_valid_data, false);
            p.set_int(lt::settings_pack::disk_write_mode, 1);
            p.set_bool(lt::settings_pack::disable_hash_checks, false);
        }
        else
        {

            // p.set_str(lt::settings_pack::user_agent, "DuneTorrentSeeder");
            p.set_bool(lt::settings_pack::piece_extent_affinity, false);
            p.set_bool(lt::settings_pack::seeding_outgoing_connections, true);

            p.set_int(lt::settings_pack::tick_interval, 150);
            p.set_int(lt::settings_pack::torrent_connect_boost, 255);

            p.set_int(lt::settings_pack::connection_speed, 200);
            p.set_int(lt::settings_pack::active_seeds, 1000);
            p.set_int(lt::settings_pack::active_downloads, 0);
            p.set_int(lt::settings_pack::active_checking, 1000);
            p.set_int(lt::settings_pack::active_limit, 2000);
            p.set_int(lt::settings_pack::active_tracker_limit, 2000);
            p.set_int(lt::settings_pack::connections_limit, 8000);
            // p.set_int(lt::settings_pack::half_open_limit, 50);
            p.set_int(lt::settings_pack::file_pool_size, 20);
            p.set_int(lt::settings_pack::listen_queue_size, 3000);
            p.set_int(lt::settings_pack::max_allowed_in_request_queue, 2000);
            p.set_int(lt::settings_pack::max_out_request_queue, 2000);
            p.set_int(lt::settings_pack::dht_upload_rate_limit, block_size * 8);

            p.set_int(lt::settings_pack::write_cache_line_size, 128);
            // p.set_int(lt::settings_pack::cache_size, 1024 * 50);
            // p.set_bool(lt::settings_pack::use_disk_cache_pool, false);
            // p.set_bool(lt::settings_pack::allow_partial_disk_writes, false);

            p.set_int(lt::settings_pack::send_buffer_watermark_factor, 150);
            p.set_int(lt::settings_pack::send_buffer_low_watermark, 1024 * 10);
            p.set_int(lt::settings_pack::send_buffer_watermark, 1024 * 1024 * 1024);
            p.set_int(lt::settings_pack::send_socket_buffer_size, 1024 * 1024 * 1024);
            p.set_int(lt::settings_pack::recv_socket_buffer_size, 1024 * 512);

            // p.set_bool(lt::settings_pack::no_atime_storage, true);
            // p.set_bool(lt::settings_pack::enable_set_file_valid_data, false);
            // p.set_int(lt::settings_pack::disk_write_mode, 1);
            // p.set_bool(lt::settings_pack::disable_hash_checks, false);
        }

        return sp;
    }

    std::vector<char> TransferInterfaceBittorrent::load_file(std::string const &filename)
    {
        std::fstream in;
        in.exceptions(std::ifstream::failbit);
        in.open(filename.c_str(), std::ios_base::in | std::ios_base::binary);
        in.seekg(0, std::ios_base::end);
        size_t const size = static_cast<size_t>(in.tellg());
        in.seekg(0, std::ios_base::beg);
        std::vector<char> ret(size);
        in.read(ret.data(), static_cast<int>(ret.size()));
        return ret;
    }

    std::string TransferInterfaceBittorrent::branch_path(std::string const &f)
    {
        if (f.empty())
        {
            return f;
        }

        if (f == "/")
        {
            return "";
        }

        auto len = f.size();
        // if the last character is / or \ ignore it
        if (f[len - 1] == '/' || f[len - 1] == '\\')
        {
            --len;
        }
        while (len > 0)
        {
            --len;
            if (f[len] == '/' || f[len] == '\\')
            {
                break;
            }
        }

        if (f[len] == '/' || f[len] == '\\')
        {
            ++len;
        }
        return std::string(f.c_str(), len);
    }

    // do not include files and folders whose
    // name starts with a .
    bool TransferInterfaceBittorrent::file_filter(std::string const &f)
    {
        if (f.empty())
        {
            return false;
        }

        char const *first = f.c_str();
        char const *sep = strrchr(first, '/');

        // if there is no parent path, just set 'sep'
        // to point to the filename.
        // if there is a parent path, skip the '/' character
        if (sep == nullptr)
        {
            sep = first;
        }
        else
        {
            ++sep; // NOLINT
        }

        // return false if the first character of the filename is a .
        if (sep[0] == '.') // NOLINT
        {
            return false;
        }

        return true;
    }

    bool TransferInterfaceBittorrent::make_torrent(std::filesystem::path full_path, int piece_size, const std::string &tracker, const std::string &outfile)
    try
    {
        std::string creator_str = "libtorrent";
        std::string comment_str;

        std::vector<std::string> web_seeds;
        std::vector<std::string> trackers;
        std::vector<std::string> collections;
        std::vector<lt::sha1_hash> similar;
        lt::create_flags_t flags = {};
        std::string root_cert;

        flags |= lt::create_torrent::v2_only;
        flags |= lt::create_torrent::modification_time;

        lt::file_storage fs;
        if (full_path.is_relative())
        {
            full_path = std::filesystem::absolute(full_path);
        }

        lt::add_files(fs, full_path.string(), file_filter, flags);
        if (fs.num_files() == 0)
        {
            std::cerr << "no files specified.\n";
            return true;
        }

        lt::create_torrent t(fs, piece_size, flags);
        int tier = 0;
        for (std::string const &tr : trackers)
        {
            if (tr == "-")
            {
                ++tier;
            }
            else
            {
                t.add_tracker(tr, tier);
            }
        }

        t.add_tracker(tracker);
        t.set_priv(false);

        auto const num = t.num_pieces();
        lt::set_piece_hashes(t, branch_path(full_path), [num](lt::piece_index_t const p)
                             { std::cerr << "\r" << p << "/" << num; });

        std::cerr << "\n";
        t.set_creator(creator_str.c_str());
        if (!comment_str.empty())
        {
            t.set_comment(comment_str.c_str());
        }

        // create the torrent and print it to stdout
        std::vector<char> torrent;

        // Silent the output for the logs
        // std::cout.setstate(std::ios_base::failbit);
        lt::bencode(back_inserter(torrent), t.generate());
        // std::cout.clear();

        if (!outfile.empty())
        {
            std::fstream out;
            out.exceptions(std::ifstream::failbit);
            out.open(outfile.c_str(), std::ios_base::out | std::ios_base::binary);
            out.write(torrent.data(), static_cast<int>(torrent.size()));
        }
        else
        {
            // TODO Aug-14-2022 Leo Joly leo.vincent.andre.joly@cern.ch : Add error code
            // std::cout.write(torrent.data(), int(torrent.size()));
            return false;
        }

        return true;
    }
    catch (std::exception &e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return false;
    }

    void TransferInterfaceBittorrent::generate_torrents_files(const std::filesystem::path &dest, const std::string &tracker)
    {
        for (const auto &f_meta : get_transfer_options().get_transfers_meta())
        {
            std::filesystem::path tmp = dest;
            make_torrent(f_meta->get_file_path(), static_cast<int>(pow(2, 23)), tracker, tmp.append(f_meta->get_file_name() + ".torrent").string());
        }
    }

    bool TransferInterfaceBittorrent::upload_file(TransferMetadata &f_meta)
    {
        TLOG() << "debug : uploading " << f_meta.get_file_name();

        if (add_torrent(get_work_dir().append(f_meta.get_file_name() + ".torrent"), f_meta.get_file_path().remove_filename()) == "")
        {
            f_meta.set_error_code("failed to add torrent to session");
            return false;
        }

        m_filename_to_metadata[f_meta.get_file_name()] = &f_meta;
        return true;
    }

    bool TransferInterfaceBittorrent::download_file(TransferMetadata &f_meta, std::filesystem::path dest)
    {
        TLOG() << "debug : starting download " << f_meta.get_file_name();

        // need to add before adding magnet because can instant access after adding magnet
        m_filename_to_metadata[f_meta.get_file_name()] = &f_meta;

        if (add_magnet(f_meta.get_magnet_link(), dest))
        {
            TLOG() << "debug : added magnet passed ";
        }
        else
        {
            // erasing from map because we failed to add magnet
            m_filename_to_metadata.erase(f_meta.get_file_name());
            f_meta.set_error_code("failed to add magnet link to session");
            return false;
        }

        // add_torrent(get_work_dir().append(f_meta.get_file_name() + ".torrent"), get_work_dir().append(".."));
        return true;
    }

    bool TransferInterfaceBittorrent::pause_file(TransferMetadata &f_meta)
    {
        auto handles = ses.get_torrents();
        for (const auto &h : handles)
        {
            if (h.torrent_file()->name() == f_meta.get_file_name())
            {
                m_paused++;
                h.pause(lt::torrent_handle::graceful_pause);
                TLOG() << "debug : pausing " << f_meta.get_file_name() << " and saving pause data in " << get_work_dir().string() << "/.resume_file_" << f_meta.get_file_name();
                break;
            }
        }

        return true;
    }

    bool TransferInterfaceBittorrent::resume_file(TransferMetadata &f_meta)
    {

        bool found = false;
        auto const handles = ses.get_torrents();
        for (const auto &h : handles)
        {
            if (h.torrent_file()->name() == f_meta.get_file_name())
            {
                m_paused--;
                h.resume();
                // lt::error_code ec;
                // TODO
                // h.connect_peer(lt::tcp::endpoint(boost::asio::ip::make_address("192.168.0.106", ec), std::uint16_t(5010)));
                // if (!peer.empty())
                // {
                //     auto port = peer.find_last_of(':');
                //     if (port != std::string::npos)
                //     {
                //         peer[port++] = '\0';
                //         char const* ip = peer.data();
                //         int const peer_port = atoi(peer.data() + port);
                //         error_code ec;
                //         if (peer_port > 0)
                //         {
                //             h.connect_peer(tcp::endpoint(asio::ip::make_address(ip, ec), std::uint16_t(peer_port)));
                //         }
                //     }
                // }
                found = true;
                break;
            }
        }

        if (!found)
        {
            // load resume data from disk and pass it in as we add the magnet link
            auto buf = load_file(get_work_dir().append(".resume_file" + f_meta.get_file_name()));
            lt::add_torrent_params atp;

            if (buf.size())
            {
                atp = lt::read_resume_data(buf);
            }
            else
            {
                ers::error(BittorrentLoadResumeFileError(ERS_HERE, f_meta.get_file_name()));
                f_meta.set_error_code("failed to load resume data");
                return false;
            }

            m_filename_to_metadata[f_meta.get_file_name()] = &f_meta;
            ses.async_add_torrent(std::move(atp));
        }

        return true;
    }

    bool TransferInterfaceBittorrent::cancel_file(TransferMetadata &f_meta)
    {
        auto const handles = ses.get_torrents();
        for (const auto &h : handles)
        {
            if (h.torrent_file()->name() == f_meta.get_file_name())
            {
                // Remove torrent from session
                ses.remove_torrent(h);
                break;
            }
        }

        // wait for the session to remove the torrent
        std::this_thread::sleep_for(std::chrono::seconds(1));
        m_filename_to_metadata.erase(f_meta.get_file_name());

        // remove resume data
        std::filesystem::remove(get_work_dir().append(".resume_file" + f_meta.get_file_name()));

        // remove torrent file if uploader or file if downloader
        if (!m_is_client)
        {
            std::filesystem::remove(get_work_dir().append(f_meta.get_file_name() + ".torrent"));
        }
        else
        {
            std::filesystem::remove(get_work_dir().append(f_meta.get_file_name()));
        }

        return true;
    }

    // TODO necessary ?
    bool TransferInterfaceBittorrent::hash_file(TransferMetadata &f_meta)
    {
        (void)f_meta;
        return true;
    }

    // Custom disk IO interface
    // struct temp_storage
    // {
    //     explicit temp_storage(lt::file_storage const &fs) : m_files(fs) {}

    //     lt::span<char const> readv(lt::peer_request const r, lt::storage_error &ec) const
    //     {
    //         auto const i = m_file_data.find(r.piece);
    //         if (i == m_file_data.end())
    //         {
    //             ec.operation = lt::operation_t::file_read;
    //             ec.ec = boost::asio::error::eof;
    //             return {};
    //         }
    //         if (int(i->second.size()) <= r.start)
    //         {
    //             ec.operation = lt::operation_t::file_read;
    //             ec.ec = boost::asio::error::eof;
    //             return {};
    //         }
    //         return {i->second.data() + r.start, std::min(r.length, int(i->second.size()) - r.start)};
    //     }
    //     void writev(lt::span<char const> const b, lt::piece_index_t const piece, int const offset)
    //     {
    //         auto &data = m_file_data[piece];
    //         if (data.empty())
    //         {
    //             // allocate the whole piece, otherwise we'll invalidate the pointers
    //             // we have returned back to libtorrent
    //             int const size = piece_size(piece);
    //             data.resize(std::size_t(size));
    //         }
    //         TORRENT_ASSERT(offset + b.size() <= int(data.size()));
    //         std::memcpy(data.data() + offset, b.data(), std::size_t(b.size()));
    //     }
    //     lt::sha1_hash hash(lt::piece_index_t const piece, lt::span<lt::sha256_hash> const block_hashes, lt::storage_error &ec) const
    //     {
    //         auto const i = m_file_data.find(piece);
    //         if (i == m_file_data.end())
    //         {
    //             ec.operation = lt::operation_t::file_read;
    //             ec.ec = boost::asio::error::eof;
    //             return {};
    //         }
    //         if (!block_hashes.empty())
    //         {
    //             int const piece_size2 = m_files.piece_size2(piece);
    //             int const blocks_in_piece2 = m_files.blocks_in_piece2(piece);
    //             char const *buf = i->second.data();
    //             std::int64_t offset = 0;
    //             for (int k = 0; k < blocks_in_piece2; ++k)
    //             {
    //                 lt::hasher256 h2;
    //                 std::ptrdiff_t const len2 = std::min(lt::default_block_size, int(piece_size2 - offset));
    //                 h2.update({buf, len2});
    //                 buf += len2;
    //                 offset += len2;
    //                 block_hashes[k] = h2.final();
    //             }
    //         }
    //         return lt::hasher(i->second).final();
    //     }
    //     lt::sha256_hash hash2(lt::piece_index_t const piece, int const offset, lt::storage_error &ec)
    //     {
    //         auto const i = m_file_data.find(piece);
    //         if (i == m_file_data.end())
    //         {
    //             ec.operation = lt::operation_t::file_read;
    //             ec.ec = boost::asio::error::eof;
    //             return {};
    //         }

    //         int const piece_size = m_files.piece_size2(piece);

    //         std::ptrdiff_t const len = std::min(lt::default_block_size, piece_size - offset);

    //         lt::span<char const> b = {i->second.data() + offset, len};
    //         return lt::hasher256(b).final();
    //     }

    // private:
    //     int piece_size(lt::piece_index_t piece) const
    //     {
    //         int const num_pieces = static_cast<int>((m_files.total_size() + m_files.piece_length() - 1) / m_files.piece_length());
    //         return static_cast<int>(piece) < num_pieces - 1
    //                    ? m_files.piece_length()
    //                    : static_cast<int>(m_files.total_size() - std::int64_t(num_pieces - 1) * m_files.piece_length());
    //     }

    //     lt::file_storage const &m_files;
    //     std::map<lt::piece_index_t, std::vector<char>> m_file_data;
    // };

    // lt::storage_index_t pop(std::vector<lt::storage_index_t> &q)
    // {
    //     TORRENT_ASSERT(!q.empty());
    //     lt::storage_index_t const ret = q.back();
    //     q.pop_back();
    //     return ret;
    // }

    // struct temp_disk_io final : lt::disk_interface, lt::buffer_allocator_interface
    // {
    //     explicit temp_disk_io(lt::io_context &ioc) : m_ioc(ioc) {}

    //     void settings_updated() override {}

    //     lt::storage_holder new_torrent(lt::storage_params const &params, std::shared_ptr<void> const &) override
    //     {
    //         lt::storage_index_t const idx = m_free_slots.empty()
    //                                             ? m_torrents.end_index()
    //                                             : pop(m_free_slots);
    //         auto storage = std::make_unique<temp_storage>(params.files);
    //         if (idx == m_torrents.end_index())
    //         {
    //             m_torrents.emplace_back(std::move(storage));
    //         }
    //         else
    //         {
    //             m_torrents[idx] = std::move(storage);
    //         }
    //         return lt::storage_holder(idx, *this);
    //     }

    //     void remove_torrent(lt::storage_index_t const idx) override
    //     {
    //         m_torrents[idx].reset();
    //         m_free_slots.push_back(idx);
    //     }

    //     void abort(bool) override {}

    //     void async_read(lt::storage_index_t storage, lt::peer_request const &r, std::function<void(lt::disk_buffer_holder block, lt::storage_error const &se)> handler, lt::disk_job_flags_t) override
    //     {
    //         // this buffer is owned by the storage. It will remain valid for as
    //         // long as the torrent remains in the session. We don't need any lifetime
    //         // management of it.
    //         lt::storage_error error;
    //         lt::span<char const> b = m_torrents[storage]->readv(r, error);

    //         post(m_ioc, [handler, error, b, this]
    //              { handler(lt::disk_buffer_holder(*this, const_cast<char *>(b.data()), int(b.size())), error); });
    //     }

    //     bool async_write(lt::storage_index_t storage, lt::peer_request const &r, char const *buf, std::shared_ptr<lt::disk_observer>, std::function<void(lt::storage_error const &)> handler, lt::disk_job_flags_t) override
    //     {
    //         lt::span<char const> const b = {buf, r.length};

    //         m_torrents[storage]->writev(b, r.piece, r.start);

    //         post(m_ioc, [=]
    //              { handler(lt::storage_error()); });
    //         return false;
    //     }

    //     void async_hash(lt::storage_index_t storage, lt::piece_index_t const piece, lt::span<lt::sha256_hash> block_hashes, lt::disk_job_flags_t, std::function<void(lt::piece_index_t, lt::sha1_hash const &, lt::storage_error const &)> handler) override
    //     {
    //         lt::storage_error error;
    //         lt::sha1_hash const hash = m_torrents[storage]->hash(piece, block_hashes, error);
    //         post(m_ioc, [=]
    //              { handler(piece, hash, error); });
    //     }

    //     void async_hash2(lt::storage_index_t storage, lt::piece_index_t const piece, int const offset, lt::disk_job_flags_t, std::function<void(lt::piece_index_t, lt::sha256_hash const &, lt::storage_error const &)> handler) override
    //     {
    //         lt::storage_error error;
    //         lt::sha256_hash const hash = m_torrents[storage]->hash2(piece, offset, error);
    //         post(m_ioc, [=]
    //              { handler(piece, hash, error); });
    //     }

    //     void async_move_storage(lt::storage_index_t, std::string p, lt::move_flags_t, std::function<void(lt::status_t, std::string const &, lt::storage_error const &)> handler) override
    //     {
    //         post(m_ioc, [=]
    //              { handler(lt::status_t::fatal_disk_error, p, lt::storage_error(lt::error_code(boost::system::errc::operation_not_supported, lt::system_category()))); });
    //     }

    //     void async_release_files(lt::storage_index_t, std::function<void()>) override {}

    //     void async_delete_files(lt::storage_index_t, lt::remove_flags_t, std::function<void(lt::storage_error const &)> handler) override
    //     {
    //         post(m_ioc, [=]
    //              { handler(lt::storage_error()); });
    //     }

    //     void async_check_files(lt::storage_index_t, lt::add_torrent_params const *, lt::aux::vector<std::string, lt::file_index_t>, std::function<void(lt::status_t, lt::storage_error const &)> handler) override
    //     {
    //         post(m_ioc, [=]
    //              { handler(lt::status_t::no_error, lt::storage_error()); });
    //     }

    //     void async_rename_file(lt::storage_index_t, lt::file_index_t const idx, std::string const name, std::function<void(std::string const &, lt::file_index_t, lt::storage_error const &)> handler) override
    //     {
    //         post(m_ioc, [=]
    //              { handler(name, idx, lt::storage_error()); });
    //     }

    //     void async_stop_torrent(lt::storage_index_t, std::function<void()> handler) override
    //     {
    //         post(m_ioc, handler);
    //     }

    //     void async_set_file_priority(lt::storage_index_t, lt::aux::vector<lt::download_priority_t, lt::file_index_t> prio, std::function<void(lt::storage_error const &, lt::aux::vector<lt::download_priority_t, lt::file_index_t>)> handler) override
    //     {
    //         post(m_ioc, [=]
    //              { handler(lt::storage_error(lt::error_code(
    //                            boost::system::errc::operation_not_supported, lt::system_category())),
    //                        std::move(prio)); });
    //     }

    //     void async_clear_piece(lt::storage_index_t, lt::piece_index_t index, std::function<void(lt::piece_index_t)> handler) override
    //     {
    //         post(m_ioc, [=]
    //              { handler(index); });
    //     }

    //     // implements buffer_allocator_interface
    //     void free_disk_buffer(char *) override
    //     {
    //         // never free any buffer. We only return buffers owned by the storage
    //         // object
    //     }

    //     void update_stats_counters(lt::counters &) const override {}

    //     std::vector<lt::open_file_state> get_status(lt::storage_index_t) const override
    //     {
    //         return {};
    //     }

    //     void submit_jobs() override {}

    // private:
    //     lt::aux::vector<std::shared_ptr<temp_storage>, lt::storage_index_t> m_torrents;

    //     // slots that are unused in the m_torrents vector
    //     std::vector<lt::storage_index_t> m_free_slots;

    //     // callbacks are posted on this
    //     lt::io_context &m_ioc;
    // };

    // std::unique_ptr<lt::disk_interface> temp_disk_constructor(
    //     lt::io_context &ioc, lt::settings_interface const &, lt::counters &)
    // {
    //     return std::make_unique<temp_disk_io>(ioc);
    // }

} // namespace dunedaq::snbmodules
