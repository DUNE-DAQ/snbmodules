/**
 * @file PreconfiguredTriggerModule.cpp Implementation of a Preconfigured Trigger
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "PreconfiguredTriggerModule.hpp"

#include "appmodel/PreconfiguredTriggerModule.hpp"
#include "appmodel/PreconfiguredTriggerModuleComponent.hpp"
#include "appmodel/PreconfiguredTriggerModuleConf.hpp"
#include "appmodel/SourceIDConf.hpp"

namespace dunedaq::snbmodules {
PreconfiguredTriggerModule::PreconfiguredTriggerModule(const std::string& module_name)
  : appfwk::DAQModule(module_name)
{
  register_command("start", &PreconfiguredTriggerModule::do_start);
  register_command("enable_triggers", &PreconfiguredTriggerModule::do_send_decision);
}

void
PreconfiguredTriggerModule::init(std::shared_ptr<appfwk::ConfigurationManager> cfg)
{
  auto mdal = cfg->get_dal<appmodel::PreconfiguredTriggerModule>(get_name());

  if (!mdal) {
    throw appfwk::CommandFailed(ERS_HERE, "init", get_name(), "Unable to retrieve configuration object");
  }

  auto iom = iomanager::IOManager::get();
  for (auto con : mdal->get_outputs()) {
    if (con->get_data_type() == datatype_to_string<dfmessages::TriggerDecision>()) {
      m_trigger_decision_sender = iom->get_sender<dfmessages::TriggerDecision>(con->UID());
    }
  }

  if (m_trigger_decision_sender == nullptr) {
    throw appfwk::MissingConnection(ERS_HERE, get_name(), "TriggerDecision", "output");
  }

  auto config = mdal->get_configuration();
  m_decision.trigger_timestamp = config->get_trigger_timestamp();
  m_decision.trigger_type = config->get_trigger_type();
  m_decision.readout_type = static_cast<dfmessages::ReadoutType>(config->get_readout_type());

  for (auto& comp : mdal->get_components()) {
    daqdataformats::SourceID sid(daqdataformats::SourceID::string_to_subsystem(comp->get_source_id()->get_subsystem()),
                                 comp->get_source_id()->get_sid());
    m_decision.components.emplace_back(sid, comp->get_time_start(), comp->get_time_end());
  }
}
void
PreconfiguredTriggerModule::do_start(const CommandData_t& cmd)
{
  m_run_number = cmd.at("run").get<daqdataformats::run_number_t>();
  m_last_trigger_number = 0;
}

void
PreconfiguredTriggerModule::do_send_decision(const CommandData_t& /*cmd*/)
{
  dfmessages::TriggerDecision decision_copy(m_decision);
  decision_copy.run_number = m_run_number;
  decision_copy.trigger_number = ++m_last_trigger_number;
  m_trigger_decision_sender->send(std::move(decision_copy), iomanager::Sender::s_block);
}

} // namespace dunedaq::snbmodules

DEFINE_DUNE_DAQ_MODULE(dunedaq::snbmodules::PreconfiguredTriggerModule)
