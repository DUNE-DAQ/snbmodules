

#ifndef SNBMODULES_INCLUDE_SNBMODULES_INTERFACES_TRANSFER_INTERFACE_BITTORRENT_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_INTERFACES_TRANSFER_INTERFACE_BITTORRENT_HPP_

#include "snbmodules/interfaces/transfer_interface_abstract.hpp"
#include "utilities/WorkerThread.hpp"

#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/torrent_status.hpp"
#include "libtorrent/error_code.hpp"
#include "libtorrent/create_torrent.hpp"

#include "libtorrent/config.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/announce_entry.hpp"
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/session_params.hpp"
#include "libtorrent/identify_client.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/ip_filter.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/peer_info.hpp"
#include "libtorrent/bdecode.hpp"
#include "libtorrent/add_torrent_params.hpp"
#include "libtorrent/time.hpp"
#include "libtorrent/read_resume_data.hpp"
#include "libtorrent/write_resume_data.hpp"
#include "libtorrent/string_view.hpp"
#include "libtorrent/disk_interface.hpp"   // for open_file_state
#include "libtorrent/disabled_disk_io.hpp" // for disabled_disk_io_constructor
#include "libtorrent/load_torrent.hpp"

#include <termios.h>
#include <sys/ioctl.h>
#include <csignal>
#include <utility>
#include <dirent.h>
#include <filesystem>

#include <cstdio>  // for snprintf
#include <cstdlib> // for atoi
#include <cstring>
#include <deque>
#include <fstream>
#include <regex>
#include <algorithm> // for min()/max()

#include <string>
#include <vector>
#include <map>

namespace dunedaq::snbmodules
{
    using clk = std::chrono::steady_clock;
    using lt::seconds;
    using std::chrono::duration_cast;

    struct session_state_t
    {
        // torrent_view &view;
        // session_view &ses_view;
        std::deque<std::string> events;
        std::vector<lt::peer_info> peers;
        std::vector<std::int64_t> file_progress;
        std::vector<lt::partial_piece_info> download_queue;
        std::vector<lt::block_info> download_queue_block_info;
        std::vector<int> piece_availability;
        std::vector<lt::announce_entry> trackers;

        void clear()
        {
            peers.clear();
            file_progress.clear();
            download_queue.clear();
            download_queue_block_info.clear();
            piece_availability.clear();
            trackers.clear();
        }
    };

    class TransferInterfaceBittorrent : public TransferInterfaceAbstract
    {

    public:
        TransferInterfaceBittorrent(GroupMetadata &config, bool is_client, std::filesystem::path work_dir, IPFormat listening_ip);
        ~TransferInterfaceBittorrent();

        void generate_torrents_files(const std::filesystem::path &dest, const std::string &tracker);
        std::filesystem::path get_work_dir() { return m_work_dir; }

        bool upload_file(TransferMetadata &f_meta) override;
        bool download_file(TransferMetadata &f_meta, std::filesystem::path dest) override;
        bool pause_file(TransferMetadata &f_meta) override;
        bool resume_file(TransferMetadata &f_meta) override;
        bool hash_file(TransferMetadata &f_meta) override;
        bool cancel_file(TransferMetadata &f_meta) override;

    private:
        lt::session ses;
        bool m_is_client;
        int m_torrent_num = 0;
        int m_peer_num = 0;
        int m_paused = 0;
        bool save_on_exit = true;
        std::filesystem::path m_work_dir;
        IPFormat m_listening_ip;
        session_state_t session_state;
        std::map<std::string, TransferMetadata *> m_filename_to_metadata;
        int m_rate_limit = -1;

        // bool print_ip = true;
        // bool print_peaks = true;
        // bool print_local_ip = true;
        // bool print_timers = false;
        // bool print_block = false;
        // bool print_fails = false;
        // bool print_send_bufs = true;
        // bool print_connecting_peers = true;

        // bool print_peers = true;
        // bool print_peers_legend = true;

        // return the name of a torrent status enum
        char const *state(lt::torrent_status::state_t s);

        bool add_magnet(lt::string_view uri, std::filesystem::path dest);
        // return magnet url
        std::string add_torrent(std::string torrent, std::filesystem::path dest);

        void set_torrent_params(lt::add_torrent_params &p, std::filesystem::path dest);
        lt::session_params set_settings(IPFormat listen_interface, std::string listen_port);

        static std::vector<char> load_file(std::string const &filename);
        static std::string branch_path(std::string const &f);
        // int print_peer_info(std::string &out, std::vector<lt::peer_info> const &peers, int max_lines);
        // int print_peer_legend(std::string &out, int max_lines);
        // std::string print_endpoint(lt::tcp::endpoint const &ep);

        // do not include files and folders whose
        // name starts with a .
        static bool file_filter(std::string const &f);

        bool make_torrent(std::filesystem::path full_path, int piece_size, std::string tracker, std::string outfile);

        // Threading
        dunedaq::utilities::WorkerThread m_thread;
        void do_work(std::atomic<bool> &);
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_INTERFACES_TRANSFER_INTERFACE_BITTORRENT_HPP_
