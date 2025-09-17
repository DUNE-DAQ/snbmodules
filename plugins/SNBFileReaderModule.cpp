/**
 * @file SNBFileReaderModule.cpp SNBFileReaderModule class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "SNBFileReaderModule.hpp"

// #include "appfwk/app/Nljs.hpp"
// #include "appfwk/cmd/Nljs.hpp"
#include "logging/Logging.hpp"

#include "datahandlinglibs/DataHandlingIssues.hpp"
#include "datahandlinglibs/ReadoutLogging.hpp"
// #include "snbmodules/sourceemulatorconfig/Nljs.hpp"
#include "appmodel/DataReaderModule.hpp"
#include "snbmodules/FileSourceModel.hpp"

// #include "fdreadoutlibs/DUNEWIBSuperChunkTypeAdapter.hpp"
#include "fdreadoutlibs/CRTBernTypeAdapter.hpp"
#include "fdreadoutlibs/CRTGrenobleTypeAdapter.hpp"
#include "fdreadoutlibs/DAPHNEStreamSuperChunkTypeAdapter.hpp"
#include "fdreadoutlibs/DAPHNESuperChunkTypeAdapter.hpp"
#include "fdreadoutlibs/DUNEWIBEthTypeAdapter.hpp"
#include "fdreadoutlibs/TDEEthTypeAdapter.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace dunedaq::datahandlinglibs::logging;

namespace dunedaq {

// DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::DUNEWIBSuperChunkTypeAdapter, "WIB2Frame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::DUNEWIBEthTypeAdapter, "WIBEthFrame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::DAPHNESuperChunkTypeAdapter, "PDSFrame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::DAPHNEStreamSuperChunkTypeAdapter, "PDSStreamFrame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::TDEEthTypeAdapter, "TDEEthFrame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::CRTBernTypeAdapter, "CRTBernFrame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::CRTGrenobleTypeAdapter, "CRTGrenobleFrame")

namespace fdreadoutmodules {

SNBFileReaderModule::SNBFileReaderModule(const std::string& name)
  : DAQModule(name)
  , FileReaderBase(name)
{
  inherited_mod::register_command("conf", &inherited_fcr::do_conf);
  inherited_mod::register_command("scrap", &inherited_fcr::do_scrap);
  inherited_mod::register_command("start", &inherited_fcr::do_start);
  inherited_mod::register_command("stop_trigger_sources", &inherited_fcr::do_stop);
}

void
SNBFileReaderModule::init(std::shared_ptr<appfwk::ConfigurationManager> cfg)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  inherited_fcr::init(cfg);
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

std::shared_ptr<snbmodules::FileSourceConcept>
SNBFileReaderModule::create_source_emulator(std::string q_id, std::atomic<bool>& run_marker)
{
  //! Values suitable to emulation

  static constexpr int daphnestream_time_tick_diff =
    fdreadoutlibs::types::DAPHNEStreamSuperChunkTypeAdapter::expected_tick_difference;
  static constexpr double daphnestream_rate_khz =
    62500. / daphnestream_time_tick_diff / fdreadoutlibs::types::kDAPHNEStreamNumFrames;

  static constexpr int daphne_time_tick_diff =
    fdreadoutlibs::types::DAPHNESuperChunkTypeAdapter::expected_tick_difference;
  static constexpr double daphne_rate_khz = 62500. / daphne_time_tick_diff / fdreadoutlibs::types::kDAPHNENumFrames;

  static constexpr int wibeth_time_tick_diff = fdreadoutlibs::types::DUNEWIBEthTypeAdapter::expected_tick_difference;
  static constexpr double wibeth_rate_khz = 62500. / wibeth_time_tick_diff;

  static constexpr int tdeeth_time_tick_diff = fdreadoutlibs::types::TDEEthTypeAdapter::expected_tick_difference;
  static constexpr double tdeeth_rate_khz = 62500. / tdeeth_time_tick_diff;
  static constexpr double crtbern_rate_khz = 100;
  static constexpr double crtgrenoble_rate_khz = 100;

  auto datatypes = dunedaq::iomanager::IOManager::get()->get_datatypes(q_id);
  if (datatypes.size() != 1) {
    ers::error(dunedaq::datahandlinglibs::GenericConfigurationError(
      ERS_HERE, "Multiple output data types specified! Expected only a single type!"));
  }
  std::string raw_dt{ *datatypes.begin() };
  TLOG() << "Choosing specialization for SourceEmulator with raw_input"
         << " [uid:" << q_id << " , data_type:" << raw_dt << ']';

  // IF WIBETH
  if (raw_dt.find("WIBEthFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake wibeth link";
    auto source_emu_model = std::make_shared<snbmodules::FileSourceModel<fdreadoutlibs::types::DUNEWIBEthTypeAdapter>>(
      q_id, run_marker,  wibeth_rate_khz);
    register_node(q_id, source_emu_model);
    return source_emu_model;
  }

  // IF PDS
  if (raw_dt.find("PDSFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake pds link";
    auto source_emu_model =
      std::make_shared<snbmodules::FileSourceModel<fdreadoutlibs::types::DAPHNESuperChunkTypeAdapter>>(
        q_id, run_marker,  daphne_rate_khz);
    register_node(q_id, source_emu_model);
    return source_emu_model;
  }

  // IF PDSStream
  if (raw_dt.find("PDSStreamFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake pds stream link";
    auto source_emu_model =
      std::make_shared<snbmodules::FileSourceModel<fdreadoutlibs::types::DAPHNEStreamSuperChunkTypeAdapter>>(
        q_id, run_marker,  daphnestream_rate_khz);
    register_node(q_id, source_emu_model);
    return source_emu_model;
  }

  // IF TDEEth
  if (raw_dt.find("TDEEthFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake tde link";
    auto source_emu_model = std::make_shared<snbmodules::FileSourceModel<fdreadoutlibs::types::TDEEthTypeAdapter>>(
      q_id, run_marker,  tdeeth_rate_khz);
    register_node(q_id, source_emu_model);
    return source_emu_model;
  }

  // IF CRTBern
  if (raw_dt.find("CRTBernFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake crt bern link";
    auto source_emu_model = std::make_shared<snbmodules::FileSourceModel<fdreadoutlibs::types::CRTBernTypeAdapter>>(
      q_id, run_marker,  crtbern_rate_khz);
    register_node(q_id, source_emu_model);
    return source_emu_model;
  }

  // IF CRTGrenoble
  if (raw_dt.find("CRTGrenobleFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake crt grenoble link";
    auto source_emu_model = std::make_shared<snbmodules::FileSourceModel<fdreadoutlibs::types::CRTGrenobleTypeAdapter>>(
      q_id, run_marker,  crtgrenoble_rate_khz);
    register_node(q_id, source_emu_model);
    return source_emu_model;
  }

  return nullptr;
}

} // namespace fdreadoutmodules
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::fdreadoutmodules::SNBFileReaderModule)
