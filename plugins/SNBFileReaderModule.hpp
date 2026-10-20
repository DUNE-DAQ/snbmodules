/**
 * @file SNBFileReaderModule.hpp FarDetector FakeCardReader
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef SNBMODULES_PLUGINS_SNBFILEREADERMODULE_HPP_
#define SNBMODULES_PLUGINS_SNBFILEREADERMODULE_HPP_

// #include "appfwk/cmd/Nljs.hpp"
// #include "appfwk/app/Nljs.hpp"
// #include "appfwk/cmd/Structs.hpp"

#include "appfwk/ConfigurationManager.hpp"
#include "appfwk/DAQModule.hpp"

#include "appmodel/DataMoveCallbackConf.hpp"
#include "snbmodules/readout/FileReaderBase.hpp"

#include <memory>
#include <string>

namespace dunedaq::snbmodules {

class SNBFileReaderModule
  : public dunedaq::appfwk::DAQModule
  , public dunedaq::snbmodules::FileReaderBase
{
public:
  using inherited_fcr = dunedaq::snbmodules::FileReaderBase;
  using inherited_mod = dunedaq::appfwk::DAQModule;
  /**
   * @brief SNBFileReaderModule Constructor
   * @param name Instance name for this SNBFileReaderModule instance
   */
  explicit SNBFileReaderModule(const std::string& name);

  SNBFileReaderModule(const SNBFileReaderModule&) = delete; ///< SNBFileReaderModule is not copy-constructible
  SNBFileReaderModule& operator=(const SNBFileReaderModule&) = delete; ///< SNBFileReaderModule is not copy-assignable
  SNBFileReaderModule(SNBFileReaderModule&&) = delete;            ///< SNBFileReaderModule is not move-constructible
  SNBFileReaderModule& operator=(SNBFileReaderModule&&) = delete; ///< SNBFileReaderModule is not move-assignable

  void init(std::shared_ptr<appfwk::ConfigurationManager> cfg) override;

  std::shared_ptr<snbmodules::FileSourceConcept> create_source_emulator(const appmodel::DataMoveCallbackConf* cb_conf,
                                                                        std::atomic<bool>& run_marker) override;
};

} // namespace dunedaq::snbmodules

#endif // SNBMODULES_PLUGINS_SNBFILEREADERMODULE_HPP_
