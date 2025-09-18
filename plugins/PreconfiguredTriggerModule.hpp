/**
 * @file PreconfiguredTriggerModule.hpp Declarations for Preconfigured Trigger Module
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_PLUGINS_PRECONFIGUREDTRIGGERMODULE_HPP_
#define SNBMODULES_PLUGINS_PRECONFIGUREDTRIGGERMODULE_HPP_

#include "appfwk/DAQModule.hpp"
#include "dfmessages/TriggerDecision.hpp"
#include "dfmessages/Types.hpp"
#include "iomanager/IOManager.hpp"

namespace dunedaq::snbmodules {
class PreconfiguredTriggerModule : public appfwk::DAQModule
{
public:
  explicit PreconfiguredTriggerModule(const std::string& module_name);
  PreconfiguredTriggerModule(const PreconfiguredTriggerModule&) =
    delete; ///< PreconfiguredTriggerModule is not copy-constructible
  PreconfiguredTriggerModule& operator=(const PreconfiguredTriggerModule&) =
    delete; ///< PreconfiguredTriggerModule is not copy-assignable
  PreconfiguredTriggerModule(PreconfiguredTriggerModule&&) =
    delete; ///< PreconfiguredTriggerModule is not move-constructible
  PreconfiguredTriggerModule& operator=(PreconfiguredTriggerModule&&) =
    delete; ///< PreconfiguredTriggerModule is not move-assignable

  void init(std::shared_ptr<appfwk::ConfigurationManager> cfg) override;

  void do_start(const CommandData_t& cmd);

  void do_send_decision(const CommandData_t& cmd);

private:
  dfmessages::run_number_t m_run_number;
  dfmessages::trigger_number_t m_last_trigger_number;
  dfmessages::TriggerDecision m_decision;
  std::shared_ptr<iomanager::SenderConcept<dfmessages::TriggerDecision>> m_trigger_decision_sender;
};
} // namespace dunedaq::snbmodules

#endif // SNBMODULES_PLUGINS_PRECONFIGUREDTRIGGERMODULE_HPP_
