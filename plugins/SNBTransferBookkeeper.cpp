/**
 * @file SNBTransferBookkeeper.cpp SNBTransferBookkeeper Bookkeeper module retriving transfers informations from SNBFileTransfer clients.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "SNBTransferBookkeeper.hpp"
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
        //register_command("info", &SNBTransferBookkeeper::do_info);

        m_name = name;
    }

  //void
  //SNBTransferBookkeeper::do_info(const nlohmann::json &args)
  //{
  //    (void)args;
  //    m_bookkeeper->request_update_metadata(true);
  //}

    void
    SNBTransferBookkeeper::init(std::shared_ptr<dunedaq::appfwk::ModuleConfiguration> mcfg)
    {
      auto mdal = mcfg->module<appmodel::SNBTransferBookkeeper>(get_name());
      if (!mdal) {
	throw appfwk::CommandFailed(ERS_HERE, "init", get_name(), "Unable to retrieve configuration object");
      }

      m_snbbk_conf = mdal->get_configuration();
    }

    void
    SNBTransferBookkeeper::do_conf(const data_t& /*payload*/)
    {
      m_bookkeeper = std::make_shared<Bookkeeper>(IPFormat(m_snbbk_conf->get_bookkeeper_ip()), m_name, m_snbbk_conf->get_bookkeeper_log_path(), m_snbbk_conf->get_refresh_rate(), m_snbbk_conf->get_connection_prefix(), m_snbbk_conf->get_timeout_send(), m_snbbk_conf->get_timeout_receive());
      m_thread = std::make_unique<dunedaq::utilities::WorkerThread>([&](std::atomic<bool> &running)
                                                                    { m_bookkeeper->do_work(running); });
    }

    void
    SNBTransferBookkeeper::do_scrap(const data_t& /*payload*/)
    {
        m_bookkeeper.reset();
        m_thread.reset();
    }

    void
    SNBTransferBookkeeper::do_start(const data_t& /*payload*/)
    {
        m_bookkeeper->lookups_connections();
        m_thread->start_working_thread();
    }

    void
    SNBTransferBookkeeper::do_stop(const data_t& /*payload*/)
    {
        m_thread->stop_working_thread();
    }

} // namespace dunedaq::snbmodules

DEFINE_DUNE_DAQ_MODULE(dunedaq::snbmodules::SNBTransferBookkeeper)
