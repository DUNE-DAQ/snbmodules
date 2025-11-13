/**
 * @file SNBDataHandlerModule.cpp SNBDataHandlerModule class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "SNBDataHandlerModule.hpp"

#include "datahandlinglibs/opmon/datahandling_info.pb.h"

#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"

#include "snbmodules/readout/SNBRequestHandlerModel.hpp"

#include "datahandlinglibs/DataHandlingIssues.hpp"
#include "datahandlinglibs/ReadoutLogging.hpp"
#include "datahandlinglibs/concepts/DataHandlingConcept.hpp"
#include "snbmodules/readout/SNBDataHandlingModel.hpp"
#include "datahandlinglibs/models/FixedRateQueueModel.hpp"

#include "fdreadoutlibs/DAPHNEEthTypeAdapter.hpp"
#include "fdreadoutlibs/DAPHNEStreamSuperChunkTypeAdapter.hpp"
#include "fdreadoutlibs/DAPHNESuperChunkTypeAdapter.hpp"
#include "fdreadoutlibs/DUNEWIBEthTypeAdapter.hpp"
#include "fdreadoutlibs/TDEEthTypeAdapter.hpp"

#include "fdreadoutlibs/crt/CRTBernFrameProcessor.hpp"
#include "fdreadoutlibs/crt/CRTGrenobleFrameProcessor.hpp"
#include "fdreadoutlibs/daphne/DAPHNEFrameProcessor.hpp"
#include "fdreadoutlibs/daphne/DAPHNEStreamFrameProcessor.hpp"
#include "fdreadoutlibs/daphneeth/DAPHNEEthFrameProcessor.hpp"
#include "fdreadoutlibs/tde/TDEEthFrameProcessor.hpp"
#include "fdreadoutlibs/wibeth/WIBEthFrameProcessor.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace dunedaq::datahandlinglibs::logging;

namespace dunedaq {

DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::DUNEWIBEthTypeAdapter, "WIBEthFrame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::DAPHNESuperChunkTypeAdapter, "PDSFrame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::DAPHNEStreamSuperChunkTypeAdapter, "PDSStreamFrame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::TDEEthTypeAdapter, "TDEEthFrame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::CRTBernTypeAdapter, "CRTBernFrame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::CRTGrenobleTypeAdapter, "CRTGrenobleFrame")
DUNE_DAQ_TYPESTRING(dunedaq::fdreadoutlibs::types::DAPHNEEthTypeAdapter, "DAPHNEEthFrame")

namespace snbmodules {

SNBDataHandlerModule::SNBDataHandlerModule(const std::string& name)
  : DAQModule(name)
  , RawDataHandlerBase(name)
{

  inherited_mod::register_command("conf", &inherited_dlh::do_conf);
  inherited_mod::register_command("scrap", &inherited_dlh::do_scrap);
  inherited_mod::register_command("start", &inherited_dlh::do_start);
  inherited_mod::register_command("stop_trigger_sources", &inherited_dlh::do_stop);
  inherited_mod::register_command("record", &inherited_dlh::do_record);
}

void
SNBDataHandlerModule::init(std::shared_ptr<appfwk::ConfigurationManager> cfg)
{

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  inherited_dlh::init(cfg);
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
SNBDataHandlerModule::generate_opmon_data()
{
}

std::shared_ptr<datahandlinglibs::DataHandlingConcept>
SNBDataHandlerModule::create_readout(const appmodel::DataHandlerModule* modconf, std::atomic<bool>& run_marker)
{
  namespace rol = dunedaq::datahandlinglibs;
  namespace fdl = dunedaq::fdreadoutlibs;
  namespace fdt = dunedaq::fdreadoutlibs::types;

  // Acquire DataType
  std::string raw_dt = modconf->get_module_configuration()->get_input_data_type();
  TLOG() << "Choosing specializations for DataHandlingModel with data_type:" << raw_dt << ']';

  // IF WIBEth
  if (raw_dt.find("WIBEthFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for an Ethernet DUNE-WIB";
    auto readout_model = std::make_shared<SNBDataHandlingModel<
      fdt::DUNEWIBEthTypeAdapter,
      SNBRequestHandlerModel<fdt::DUNEWIBEthTypeAdapter, rol::FixedRateQueueModel<fdt::DUNEWIBEthTypeAdapter>>,
      rol::FixedRateQueueModel<fdt::DUNEWIBEthTypeAdapter>,
      fdl::WIBEthFrameProcessor>>(run_marker);
    register_node("WIBEthFrameProcessor", readout_model);
    readout_model->init(modconf);
    return readout_model;
  }

  // IF CRTBern
  if (raw_dt.find("CRTBernFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a CRTBern";
    auto readout_model = std::make_shared<SNBDataHandlingModel<
      fdt::CRTBernTypeAdapter,
      SNBRequestHandlerModel<fdt::CRTBernTypeAdapter, rol::FixedRateQueueModel<fdt::CRTBernTypeAdapter>>,
      rol::FixedRateQueueModel<fdt::CRTBernTypeAdapter>,
      fdl::CRTBernFrameProcessor>>(run_marker);
    register_node("CRTBernFrameProcessor", readout_model);
    readout_model->init(modconf);
    return readout_model;
  }

  // IF CRTGrenoble
  if (raw_dt.find("CRTGrenobleFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a CRTGrenoble";
    auto readout_model = std::make_shared<SNBDataHandlingModel<
      fdt::CRTGrenobleTypeAdapter,
      SNBRequestHandlerModel<fdt::CRTGrenobleTypeAdapter, rol::FixedRateQueueModel<fdt::CRTGrenobleTypeAdapter>>,
      rol::FixedRateQueueModel<fdt::CRTGrenobleTypeAdapter>,
      fdl::CRTGrenobleFrameProcessor>>(run_marker);
    register_node("CRTGrenobleFrameProcessor", readout_model);
    readout_model->init(modconf);
    return readout_model;
  }

  // IF TDEEth
  if (raw_dt.find("TDEEthFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for an Ethernet TDEEth";
    auto readout_model = std::make_shared<SNBDataHandlingModel<
      fdt::TDEEthTypeAdapter,
      SNBRequestHandlerModel<fdt::TDEEthTypeAdapter, rol::FixedRateQueueModel<fdt::TDEEthTypeAdapter>>,
      rol::FixedRateQueueModel<fdt::TDEEthTypeAdapter>,
      fdl::TDEEthFrameProcessor>>(run_marker);
    register_node("TDEEthFrameProcessor", readout_model);
    readout_model->init(modconf);
    return readout_model;
  }

  // IF PDS Frame using skiplist
  if (raw_dt.find("PDSFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a PDS DAPHNE using SkipList LB";
    auto readout_model = std::make_shared<
      SNBDataHandlingModel<fdt::DAPHNESuperChunkTypeAdapter,
                             SNBRequestHandlerModel<fdt::DAPHNESuperChunkTypeAdapter,
                                                    rol::FixedRateQueueModel<fdt::DAPHNESuperChunkTypeAdapter>>,
                             rol::FixedRateQueueModel<fdt::DAPHNESuperChunkTypeAdapter>,
                             fdl::DAPHNEFrameProcessor>>(run_marker);
    register_node("PDSFrameProcessor", readout_model);
    readout_model->init(modconf);
    return readout_model;
  }

  // IF PDS Frame using skiplist
  if (raw_dt.find("DAPHNEEthFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a PDS DAPHNE Ethernet using SkipList LB";
    auto readout_model = std::make_shared<SNBDataHandlingModel<
      fdt::DAPHNEEthTypeAdapter,
      SNBRequestHandlerModel<fdt::DAPHNEEthTypeAdapter, rol::FixedRateQueueModel<fdt::DAPHNEEthTypeAdapter>>,
      rol::FixedRateQueueModel<fdt::DAPHNEEthTypeAdapter>,
      fdl::DAPHNEEthFrameProcessor>>(run_marker);
    register_node("DAPHNEEthFrameProcessor", readout_model);
    readout_model->init(modconf);
    return readout_model;
  }

  // IF PDS Stream Frame using SPSC LB
  if (raw_dt.find("PDSStreamFrame") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a PDS DAPHNE stream mode using BinarySearchQueue LB";
    auto readout_model = std::make_shared<
      SNBDataHandlingModel<fdt::DAPHNEStreamSuperChunkTypeAdapter,
                             SNBRequestHandlerModel<fdt::DAPHNEStreamSuperChunkTypeAdapter,
                                                    rol::FixedRateQueueModel<fdt::DAPHNEStreamSuperChunkTypeAdapter>>,
                             rol::FixedRateQueueModel<fdt::DAPHNEStreamSuperChunkTypeAdapter>,
                             fdl::DAPHNEStreamFrameProcessor>>(run_marker);
    register_node("PDSStreamFrameProcessor", readout_model);
    readout_model->init(modconf);
    return readout_model;
  }

  return nullptr;
}

} // namespace snbmodules
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::snbmodules::SNBDataHandlerModule)
