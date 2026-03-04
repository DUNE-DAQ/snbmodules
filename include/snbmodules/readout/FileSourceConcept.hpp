/**
 * @file FileSourceConcept.hpp FileSourceConcept for constructors and
 * forwarding command args.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef snbmodules_INCLUDE_snbmodules_CONCEPTS_FileSourceConcept_HPP_
#define snbmodules_INCLUDE_snbmodules_CONCEPTS_FileSourceConcept_HPP_

#include "appmodel/SNBFileSourceParameters.hpp"
#include "confmodel/DetectorStream.hpp"
#include "datahandlinglibs/utils/RateLimiter.hpp"
#include "opmonlib/MonitorableObject.hpp"
#include "appmodel/DataMoveCallbackConf.hpp"

#include <map>
#include <string>
#include <thread>

namespace dunedaq {
namespace snbmodules {

class FileSourceConcept : public opmonlib::MonitorableObject
{
public:
  FileSourceConcept() {}

  virtual ~FileSourceConcept() {}
  FileSourceConcept(const FileSourceConcept&) = delete;            ///< FileSourceConcept is not copy-constructible
  FileSourceConcept& operator=(const FileSourceConcept&) = delete; ///< FileSourceConcept is not copy-assginable
  FileSourceConcept(FileSourceConcept&&) = delete;                 ///< FileSourceConcept is not move-constructible
  FileSourceConcept& operator=(FileSourceConcept&&) = delete;      ///< FileSourceConcept is not move-assignable

  virtual void set_sender(const appmodel::DataMoveCallbackConf* /*sink_name*/) = 0;
  virtual void conf(const confmodel::DetectorStream* conf, const appmodel::SNBFileSourceParameters* emu_conf) = 0;
  virtual void start(const appfwk::DAQModule::CommandData_t& /*args*/) = 0;
  virtual void stop(const appfwk::DAQModule::CommandData_t& /*args*/) = 0;
  virtual void scrap(const appfwk::DAQModule::CommandData_t& /*args*/) = 0;
  virtual bool is_configured() = 0;

private:
};

} // namespace snbmodules
} // namespace dunedaq

#endif // snbmodules_INCLUDE_snbmodules_CONCEPTS_FileSourceConcept_HPP_
