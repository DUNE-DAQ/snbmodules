/**
 * @file SNBFileTransfer.cpp Module to transfer files between SNBFileTransfer modules.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "SNBFileTransfer.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "appfwk/cmd/Nljs.hpp"

#include <string>
#include <set>
#include <memory>

namespace dunedaq::snbmodules
{

    SNBFileTransfer::SNBFileTransfer(const std::string &name)
        : DAQModule(name)
    {
        register_command("conf", &SNBFileTransfer::do_conf);
        register_command("scrap", &SNBFileTransfer::do_scrap);
        register_command("start", &SNBFileTransfer::do_start);
        register_command("stop", &SNBFileTransfer::do_stop);

        register_command("new_transfer", &SNBFileTransfer::do_tr_new);
        register_command("start_transfer", &SNBFileTransfer::do_tr_start);
        register_command("pause_transfer", &SNBFileTransfer::do_tr_pause);
        register_command("resume_transfer", &SNBFileTransfer::do_tr_resume);
        register_command("cancel_transfer", &SNBFileTransfer::do_tr_cancel);

        m_name = name;
    }

    void
    SNBFileTransfer::do_tr_new(const nlohmann::json &args)
    {
        TLOG() << "debug : New transfer request !";
        auto src = args["src"].get<std::string>();

        if (src == m_name)
        {
            auto dests = args["dests"].get<std::set<std::string>>();
            auto files = args["files"].get<std::set<std::filesystem::path>>();

            m_client->create_new_transfer(args["transfer_id"].get<std::string>(), args["protocol"].get<std::string>(), dests, files, args["protocol_args"]);
        }
        else
        {
            ers::error(InvalidSourceCommandRequestError(ERS_HERE, "New Transfer"));
        }
    }
    void
    SNBFileTransfer::do_tr_start(const nlohmann::json &args)
    {
        m_client->start_transfer(args["transfer_id"].get<std::string>());
    }
    void
    SNBFileTransfer::do_tr_pause(const nlohmann::json &args)
    {
        m_client->pause_transfer(args["transfer_id"].get<std::string>());
    }
    void
    SNBFileTransfer::do_tr_resume(const nlohmann::json &args)
    {
        m_client->resume_transfer(args["transfer_id"].get<std::string>());
    }
    void
    SNBFileTransfer::do_tr_cancel(const nlohmann::json &args)
    {
        m_client->cancel_transfer(args["transfer_id"].get<std::string>());
    }

    void
    SNBFileTransfer::init(const nlohmann::json &args)
    {
        (void)args;
    }

    void
    SNBFileTransfer::do_conf(const nlohmann::json &args)
    {
        m_client = std::make_shared<TransferClient>(IPFormat(args["client_ip"].get<std::string>()), m_name, args["work_dir"].get<std::filesystem::path>(), args["connection_prefix"].get<std::string>(), args["timeout_send"].get<int>(), args["timeout_receive"].get<int>());

        m_client->lookups_connections();

        m_thread = std::make_unique<dunedaq::utilities::WorkerThread>([&](std::atomic<bool> &running)
                                                                      { m_client->do_work(running); });
    }

    void
    SNBFileTransfer::do_scrap(const nlohmann::json &args)
    {
        (void)args;
        if (m_thread->thread_running())
        {
            m_thread->stop_working_thread();
            // wait for thread to stop
            while (m_thread->thread_running())
                ;
            m_thread.reset();
        }

        m_client.reset();
    }

    void
    SNBFileTransfer::do_start(const nlohmann::json &args)
    {
        (void)args;
        m_thread->start_working_thread();
    }

    void
    SNBFileTransfer::do_stop(const nlohmann::json &args)
    {
        (void)args;
        m_thread->stop_working_thread();
    }

} // namespace dunedaq::snbmodules

DEFINE_DUNE_DAQ_MODULE(dunedaq::snbmodules::SNBFileTransfer)