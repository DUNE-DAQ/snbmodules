/**
 * @file SNBTransferBookkeeper.cpp SNBTransferBookkeeper Bookkeeper module retriving transfers informations from SNBFileTransfer clients.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "SNBTransferBookkeeper.hpp"
#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/cmd/Nljs.hpp"

#include <string>
#include <set>
#include <memory>

namespace dunedaq::snbmodules
{

    SNBTransferBookkeeper::SNBTransferBookkeeper(const std::string &name)
        : DAQModule(name)
    {
        register_command("conf", &SNBTransferBookkeeper::do_conf);
        register_command("scrap", &SNBTransferBookkeeper::do_scrap);
        register_command("start", &SNBTransferBookkeeper::do_start);
        register_command("stop", &SNBTransferBookkeeper::do_stop);
        register_command("info", &SNBTransferBookkeeper::do_info);

        register_command("new_transfer", &SNBTransferBookkeeper::do_tr_new);
        register_command("start_transfer", &SNBTransferBookkeeper::do_tr_start);
        register_command("pause_transfer", &SNBTransferBookkeeper::do_tr_pause);
        register_command("resume_transfer", &SNBTransferBookkeeper::do_tr_resume);
        register_command("stop-transfer", &SNBTransferBookkeeper::do_tr_stop);

        m_name = name;
    }

    void
    SNBTransferBookkeeper::do_tr_new(const nlohmann::json &args)
    {
        if (args.contains("dests") && args.contains("files") && args.contains("src") && args.contains("protocol"))
        {
            auto dests = args["dests"].get<std::set<std::string>>();
            auto files = args["files"].get<std::set<std::filesystem::path>>();
            auto src = args["src"].get<std::string>();

            m_bookkeeper->create_new_transfer(args["protocol"].get<std::string>(), src, dests, files);
        }
        else
        {
            ers::error(ErrorConfigError(ERS_HERE, "dests, files, src and protocol are mandatory to create a new transfer"));
        }
    }

    void
    SNBTransferBookkeeper::do_tr_start(const nlohmann::json &args)
    {
        (void)args;
    }

    void
    SNBTransferBookkeeper::do_tr_pause(const nlohmann::json &args)
    {
        (void)args;
    }

    void
    SNBTransferBookkeeper::do_tr_resume(const nlohmann::json &args)
    {
        (void)args;
    }

    void
    SNBTransferBookkeeper::do_tr_stop(const nlohmann::json &args)
    {
        (void)args;
    }

    void
    SNBTransferBookkeeper::do_info(const nlohmann::json &args)
    {
        (void)args;
        m_bookkeeper->request_update_metadata(true);
    }

    void
    SNBTransferBookkeeper::init(const nlohmann::json &args)
    {
        (void)args;
    }

    void
    SNBTransferBookkeeper::do_conf(const nlohmann::json &args)
    {
        if (args.contains("bookkeeper_ip") && args.contains("bookkeeper_log_path") && args.contains("refresh_rate") && args.contains("connection_prefix") && args.contains("timeout_send") && args.contains("timeout_receive"))
        {
            m_bookkeeper = std::make_shared<Bookkeeper>(IPFormat(args["bookkeeper_ip"].get<std::string>()), m_name, args["bookkeeper_log_path"].get<std::string>(), args["refresh_rate"].get<int>(), args["connection_prefix"].get<std::string>(), args["timeout_send"].get<int>(), args["timeout_receive"].get<int>());
            m_bookkeeper->lookups_connections();
            m_thread = std::make_unique<dunedaq::utilities::WorkerThread>([&](std::atomic<bool> &running)
                                                                          { m_bookkeeper->do_work(running); });
        }
        else
        {
            ers::error(ErrorConfigError(ERS_HERE, "bookkeeper_ip, bookkeeper_log_path, refresh_rate, connection_prefix, timeout_send and timeout_receive are mandatory to configure the bookkeeper"));
        }
    }

    void
    SNBTransferBookkeeper::do_scrap(const nlohmann::json &args)
    {
        (void)args;
        m_bookkeeper.reset();
        m_thread.reset();
    }

    void
    SNBTransferBookkeeper::do_start(const nlohmann::json &args)
    {
        (void)args;
        m_thread->start_working_thread();
    }

    void
    SNBTransferBookkeeper::do_stop(const nlohmann::json &args)
    {
        (void)args;
        m_thread->stop_working_thread();
    }

} // namespace dunedaq::snbmodules

DEFINE_DUNE_DAQ_MODULE(dunedaq::snbmodules::SNBTransferBookkeeper)