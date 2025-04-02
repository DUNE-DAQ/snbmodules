/**
 * @file SNBFileTransfer.cpp Module to transfer files between SNBFileTransfer modules.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "SNBFileTransfer.hpp"

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
    SNBFileTransfer::init(std::shared_ptr<dunedaq::appfwk::ConfigurationManager> mcfg)
    {
      auto mdal = mcfg->get_dal<appmodel::SNBFileTransfer>(get_name());
      if (!mdal) {
	throw appfwk::CommandFailed(ERS_HERE, "init", get_name(), "Unable to retrieve configuration object");
      }

      m_snbft_conf = mdal->get_configuration();
    }

    void
    SNBFileTransfer::do_conf(const data_t& /*payload*/)
    {
      TLOG() << "FAB " << __LINE__ << " " << m_snbft_conf->get_work_dir();
      m_client = std::make_shared<TransferClient>(IPFormat(m_snbft_conf->get_client_ip()), m_name, m_snbft_conf->get_work_dir(), m_snbft_conf->get_connection_prefix(), m_snbft_conf->get_timeout_send(), m_snbft_conf->get_timeout_receive());
      m_thread = std::make_unique<dunedaq::utilities::WorkerThread>([&](std::atomic<bool> &running)
                                                                    { m_client->do_work(running); });
    }

    void
    SNBFileTransfer::do_scrap(const data_t& /*payload*/)
    {
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
    SNBFileTransfer::do_start(const data_t& /*payload*/)
    {
        m_client->lookups_connections();
        m_thread->start_working_thread();
    }

    void
    SNBFileTransfer::do_stop(const data_t& /*payload*/)
    {
        m_thread->stop_working_thread();
    }

} // namespace dunedaq::snbmodules

DEFINE_DUNE_DAQ_MODULE(dunedaq::snbmodules::SNBFileTransfer)
