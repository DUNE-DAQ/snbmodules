/**
 * @file SNBDataHandlerModule.hpp SNB readout
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef SNBMODULES_PLUGINS_SNBDATAHANDLERMODULE_HPP_
#define SNBMODULES_PLUGINS_SNBDATAHANDLERMODULE_HPP_

#include "appfwk/DAQModule.hpp"

#include "datahandlinglibs/RawDataHandlerBase.hpp"

#include <string>

namespace dunedaq {
namespace snbmodules {

class SNBDataHandlerModule
  : public dunedaq::appfwk::DAQModule
  , public dunedaq::datahandlinglibs::RawDataHandlerBase
{
public:
  using inherited_dlh = dunedaq::datahandlinglibs::RawDataHandlerBase;
  using inherited_mod = dunedaq::appfwk::DAQModule;
  /**
   * @brief SNBDataHandlerModule Constructor
   * @param name Instance name for this SNBDataHandlerModule instance
   */
  explicit SNBDataHandlerModule(const std::string& name);

  SNBDataHandlerModule(const SNBDataHandlerModule&) = delete; ///< SNBDataHandlerModule is not copy-constructible
  SNBDataHandlerModule& operator=(const SNBDataHandlerModule&) =
    delete;                                                         ///< SNBDataHandlerModule is not copy-assignable
  SNBDataHandlerModule(SNBDataHandlerModule&&) = delete;            ///< SNBDataHandlerModule is not move-constructible
  SNBDataHandlerModule& operator=(SNBDataHandlerModule&&) = delete; ///< SNBDataHandlerModule is not move-assignable

  void init(std::shared_ptr<appfwk::ConfigurationManager> cfg) override;

  std::shared_ptr<datahandlinglibs::DataHandlingConcept> create_readout(const appmodel::DataHandlerModule* modconf,
                                                                        std::atomic<bool>& run_marker) override;

protected:
  void generate_opmon_data() override;
};

} // namespace fdreadoutmodules
} // namespace dunedaq

#endif // SNBMODULES_PLUGINS_SNBDATAHANDLERMODULE_HPP_
