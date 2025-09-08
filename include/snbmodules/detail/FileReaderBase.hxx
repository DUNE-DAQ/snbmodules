
#include "datahandlinglibs/ReadoutLogging.hpp"
#include "datahandlinglibs/DataHandlingIssues.hpp"

namespace dunedaq {
namespace snbmodules {

FileReaderBase::FileReaderBase(const std::string& name)
  : m_configured(false)
  , m_name(name)
  , m_run_marker{ false }
{
}

void
FileReaderBase::init(std::shared_ptr<appfwk::ConfigurationManager> cfg)
{
  m_cfg = cfg;
  TLOG_DEBUG(dunedaq::datahandlinglibs::logging::TLVL_ENTER_EXIT_METHODS) << get_fcr_name() << ": Entering init() method";
  //auto ini = args.get<appfwk::app::ModInit>();
  auto ini = cfg->get_dal<appmodel::DataReaderModule>(m_name);
  if (ini != nullptr && ini->get_configuration()->get_emulation_mode()) {

    for (auto qi : ini->get_outputs()) {
      
      try {
        if (m_source_emus.find(qi->UID()) != m_source_emus.end()) {
          TLOG() << get_fcr_name() << "Same queue instance used twice";
          throw datahandlinglibs::FailedFakeCardInitialization(ERS_HERE, get_fcr_name(), "");
        }
        m_source_emus[qi->UID()] = create_source_emulator(qi->UID(), m_run_marker);
        if (m_source_emus[qi->UID()].get() == nullptr) {
          TLOG() << get_fcr_name() << "Source emulator could not be created";
          throw datahandlinglibs::FailedFakeCardInitialization(ERS_HERE, get_fcr_name(), "");
        }
        // m_source_emus[qi->UID()]->init(cfg);
        m_source_emus[qi->UID()]->set_sender(qi->UID());
      } catch (const ers::Issue& excpt) {
        throw datahandlinglibs::ResourceQueueError(ERS_HERE, qi->UID(), get_fcr_name(), excpt);
      }
    }
  }
  TLOG_DEBUG(dunedaq::datahandlinglibs::logging::TLVL_ENTER_EXIT_METHODS) << get_fcr_name() << ": Exiting init() method";
}

void
FileReaderBase::do_conf(const appfwk::DAQModule::CommandData_t& /*args*/)
{
  TLOG_DEBUG(dunedaq::datahandlinglibs::logging::TLVL_ENTER_EXIT_METHODS) << get_fcr_name() << ": Entering do_conf() method";

  if (m_configured) {
    TLOG_DEBUG(dunedaq::datahandlinglibs::logging::TLVL_WORK_STEPS) << "This module is already configured!";
  } else {
      auto cfg = m_cfg->get_dal<appmodel::DataReaderModule>(get_fcr_name());

    std::map<uint32_t, const confmodel::DetectorStream*> streams;
    for (const auto & det_connections : cfg->get_connections()) {
      	    
      for (const auto& stream : det_connections->streams()) {
        streams[stream->get_source_id()] = stream;
      }
    }

    for (const auto& qi : cfg->get_outputs()) {
      auto q_with_id = qi->cast<confmodel::QueueWithSourceId>();
      if (q_with_id == nullptr) {
        throw datahandlinglibs::FailedFakeCardInitialization(ERS_HERE, get_fcr_name(), "Queue is not of type QueueWithSourceId");
      }  
      if (m_source_emus.find(q_with_id->UID()) == m_source_emus.end()) {
       TLOG() << "Cannot find queue: " <<  q_with_id->UID() << std::endl;
        throw datahandlinglibs::GenericConfigurationError(ERS_HERE, "Cannot find queue: " + q_with_id->UID());
      }
      if (m_source_emus[q_with_id->UID()]->is_configured()) {
        TLOG() << "Emulator for queue name " << q_with_id->UID() << " was already configured";
        throw datahandlinglibs::GenericConfigurationError(ERS_HERE, "Emulator configured twice: " + q_with_id->UID());
      }
      m_source_emus[q_with_id->UID()]->conf(streams[q_with_id->get_source_id()],
                                            cfg->get_configuration()->get_snb_conf());
    }
    for (auto& [name, emu] : m_source_emus) {
      if (!emu->is_configured()) {
        throw datahandlinglibs::GenericConfigurationError(ERS_HERE, "Not all links were configured");
      }
    }

    // Mark configured
    m_configured = true;
  }

  TLOG_DEBUG(dunedaq::datahandlinglibs::logging::TLVL_ENTER_EXIT_METHODS) << get_fcr_name() << ": Exiting do_conf() method";
}

void
FileReaderBase::do_scrap(const appfwk::DAQModule::CommandData_t& args)
{
  TLOG_DEBUG(dunedaq::datahandlinglibs::logging::TLVL_ENTER_EXIT_METHODS) << get_fcr_name() << ": Entering do_scrap() method";

  for (auto& [name, emu] : m_source_emus) {
    emu->scrap(args);
  }

  m_configured = false;

  TLOG_DEBUG(dunedaq::datahandlinglibs::logging::TLVL_ENTER_EXIT_METHODS) << get_fcr_name() << ": Exiting do_scrap() method";
}
void
FileReaderBase::do_start(const appfwk::DAQModule::CommandData_t& args)
{
  TLOG_DEBUG(dunedaq::datahandlinglibs::logging::TLVL_ENTER_EXIT_METHODS) << get_fcr_name() << ": Entering do_start() method";

  m_run_marker.store(true);

  for (auto& [name, emu] : m_source_emus) {
    emu->start(args);
  }

  TLOG_DEBUG(dunedaq::datahandlinglibs::logging::TLVL_ENTER_EXIT_METHODS) << get_fcr_name() << ": Exiting do_start() method";
}

void
FileReaderBase::do_stop(const appfwk::DAQModule::CommandData_t& args)
{
  TLOG_DEBUG(dunedaq::datahandlinglibs::logging::TLVL_ENTER_EXIT_METHODS) << get_fcr_name() << ": Entering do_stop() method";

  m_run_marker = false;

  for (auto& [name, emu] : m_source_emus) {
    emu->stop(args);
  }

  TLOG_DEBUG(dunedaq::datahandlinglibs::logging::TLVL_ENTER_EXIT_METHODS) << get_fcr_name() << ": Exiting do_stop() method";
}

} // namespace datahandlinglibs
} // namespace dunedaq

