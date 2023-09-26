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

        std::string src = "";
        if (args.contains("src"))
        {
            src = args["src"].get<std::string>();
        }
        else
        {
            ers::error(ConfigError(ERS_HERE, "src is mandatory to create a new transfer"));
            return;
        }

        if (src == m_name)
        {
            std::set<std::string> dests = {};
            std::set<std::filesystem::path> files = {};

            if (args.contains("dests") && args.contains("files"))
            {
                dests = args["dests"].get<std::set<std::string>>();
                files = args["files"].get<std::set<std::filesystem::path>>();
            }
            else
            {
                ers::error(ConfigError(ERS_HERE, "dests and files are mandatory to create a new transfer"));
                return;
            }

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
        if (args.contains("transfer_id"))
        {
            m_client->start_transfer(args["transfer_id"].get<std::string>());
        }
        else
        {
            ers::error(ConfigError(ERS_HERE, "transfer_id is mandatory to start a transfer"));
        }
    }
    void
    SNBFileTransfer::do_tr_pause(const nlohmann::json &args)
    {
        if (args.contains("transfer_id"))
        {
            m_client->pause_transfer(args["transfer_id"].get<std::string>());
        }
        else
        {
            ers::error(ConfigError(ERS_HERE, "transfer_id is mandatory to pause a transfer"));
        }
    }
    void
    SNBFileTransfer::do_tr_resume(const nlohmann::json &args)
    {
        if (args.contains("transfer_id"))
        {
            m_client->resume_transfer(args["transfer_id"].get<std::string>());
        }
        else
        {
            ers::error(ConfigError(ERS_HERE, "transfer_id is mandatory to resume a transfer"));
        }
    }
    void
    SNBFileTransfer::do_tr_cancel(const nlohmann::json &args)
    {
        if (args.contains("transfer_id"))
        {
            m_client->cancel_transfer(args["transfer_id"].get<std::string>());
        }
        else
        {
            ers::error(ConfigError(ERS_HERE, "transfer_id is mandatory to cancel a transfer"));
        }
    }

    void
    SNBFileTransfer::init(const nlohmann::json &args)
    {
        (void)args;
    }

    void
    SNBFileTransfer::do_conf(const nlohmann::json &args)
    {
        if (args.contains("client_ip") && args.contains("work_dir") && args.contains("connection_prefix") && args.contains("timeout_send") && args.contains("timeout_receive"))
        {
            m_client = std::make_shared<TransferClient>(IPFormat(args["client_ip"].get<std::string>()), m_name, args["work_dir"].get<std::filesystem::path>(), args["connection_prefix"].get<std::string>(), args["timeout_send"].get<int>(), args["timeout_receive"].get<int>());
            m_thread = std::make_unique<dunedaq::utilities::WorkerThread>([&](std::atomic<bool> &running)
                                                                          { m_client->do_work(running); });
        }
        else
        {
            ers::error(ConfigError(ERS_HERE, "client_ip, work_dir, connection_prefix, timeout_send and timeout_receive are mandatory to configure a TransferClient"));
        }
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
        m_client->lookups_connections();
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