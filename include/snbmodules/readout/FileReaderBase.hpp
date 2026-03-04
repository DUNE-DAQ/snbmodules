/**
 * @file FileReaderBase.hpp Generate payloads from input file
 * Generates user payloads at a given rate, from raw binary data files.
 * This implementation is purely software based, no I/O devices and tools
 * are needed to use this module.
 *
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef SNBMODULES_INCLUDE_SNBMODULES_FILEREADERBASE_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_FILEREADERBASE_HPP_

// package
#include "snbmodules/readout/FileSourceConcept.hpp"

#include "appmodel/DataReaderConf.hpp"
#include "appmodel/DataReaderModule.hpp"
#include "appmodel/DataMoveCallbackConf.hpp"
#include "confmodel/Connection.hpp"
#include "confmodel/DaqModule.hpp"
#include "confmodel/DetDataSender.hpp"
#include "confmodel/DetectorStream.hpp"
#include "confmodel/DetectorToDaqConnection.hpp"
#include "confmodel/QueueWithSourceId.hpp"

#include "appfwk/ConfigurationManager.hpp"
#include "appfwk/DAQModule.hpp"

#include "utilities/ReusableThread.hpp"

#include "rcif/cmd/Nljs.hpp"

// std
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace snbmodules {

class FileReaderBase
{
public:
  /**
   * @brief FileReaderBase Constructor
   * @param name Instance name for this FileReaderBase instance
   */
  explicit FileReaderBase(const std::string& name);
  virtual ~FileReaderBase() {}

  FileReaderBase(const FileReaderBase&) = delete;            ///< FileReaderBase is not copy-constructible
  FileReaderBase& operator=(const FileReaderBase&) = delete; ///< FileReaderBase is not copy-assignable
  FileReaderBase(FileReaderBase&&) = delete;                 ///< FileReaderBase is not move-constructible
  FileReaderBase& operator=(FileReaderBase&&) = delete;      ///< FileReaderBase is not move-assignable

  void init(std::shared_ptr<appfwk::ConfigurationManager> cfg);

  // To be implemented by final module
  virtual std::shared_ptr<snbmodules::FileSourceConcept> create_source_emulator(const appmodel::DataMoveCallbackConf* cb_conf,
                                                                                std::atomic<bool>& run_marker) = 0;

  // Commands
  void do_conf(const appfwk::DAQModule::CommandData_t& /*args*/);
  void do_scrap(const appfwk::DAQModule::CommandData_t& /*args*/);
  void do_start(const appfwk::DAQModule::CommandData_t& /*args*/);
  void do_stop(const appfwk::DAQModule::CommandData_t& /*args*/);

  std::string get_fcr_name() { return m_name; }

private:
  // Configuration
  bool m_configured;
  std::string m_name;
  std::shared_ptr<appfwk::ConfigurationManager> m_cfg;

  std::map<std::string, std::shared_ptr<snbmodules::FileSourceConcept>> m_source_emus;

  // Threading
  std::atomic<bool> m_run_marker;
};

} // namespace snbmodules
} // namespace dunedaq

// Declarations
#include "detail/FileReaderBase.hxx"

#endif // SNBMODULES_INCLUDE_SNBMODULES_FILEREADERBASE_HPP_
