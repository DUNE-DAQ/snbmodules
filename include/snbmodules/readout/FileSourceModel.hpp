/**
 * @file FileSourceModel.hpp Emulates a source with given raw type
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef snbmodules_INCLUDE_snbmodules_MODELS_FileSourceModel_HPP_
#define snbmodules_INCLUDE_snbmodules_MODELS_FileSourceModel_HPP_

#include "iomanager/IOManager.hpp"
#include "iomanager/Sender.hpp"

#include "logging/Logging.hpp"

#include "confmodel/DetectorStream.hpp"
#include "confmodel/GeoId.hpp"

#include "datahandlinglibs/DataHandlingIssues.hpp"
#include "datahandlinglibs/utils/BufferedFileReader.hpp"
#include "datahandlinglibs/utils/RateLimiter.hpp"
#include "snbmodules/readout/FileSourceConcept.hpp"
#include "utilities/ReusableThread.hpp"

#include "datahandlinglibs/opmon/datahandling_info.pb.h"

#include "unistd.h"
#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

using dunedaq::datahandlinglibs::logging::TLVL_TAKE_NOTE;
using dunedaq::datahandlinglibs::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace snbmodules {

template<class ReadoutType>
class FileSourceModel : public FileSourceConcept
{
public:
  explicit FileSourceModel(std::string name, std::atomic<bool>& run_marker, double rate_khz)
    : m_run_marker(run_marker)
    , m_name(name)
    , m_rate_khz(rate_khz)
    , m_packet_count{ 0 }
    , m_raw_sender_timeout_ms(0)
    , m_raw_data_sender(nullptr)
    , m_producer_thread(0)
  {
  }

  // void init(const appfwk::DAQModule::CommandData_t& /*args*/) {}
  void set_sender(const std::string& conn_name);

  void conf(const confmodel::DetectorStream* stream_conf, const appmodel::SNBFileSourceParameters* file_params);
  void scrap(const appfwk::DAQModule::CommandData_t& /*args*/)
  {
    if (m_file_reader != nullptr) {
      m_file_reader->close();
    }
    m_file_reader.reset();
    m_is_configured = false;
  }
  bool is_configured() override { return m_is_configured; }

  void start(const appfwk::DAQModule::CommandData_t& /*args*/);
  void stop(const appfwk::DAQModule::CommandData_t& /*args*/);
  //  void get_info(opmonlib::InfoCollector& ci, int /*level*/);

protected:
  // The data emulator function that the worker thread runs
  void run_produce();
  virtual void generate_opmon_data() override;

  void open_next_file();

private:
  // Constuctor params
  std::atomic<bool>& m_run_marker;

  // CONFIGURATION
  std::string m_name;
  bool m_is_configured = false;
  double m_rate_khz;

  std::vector<std::string> m_file_names;
  std::vector<std::string>::const_iterator m_file_iterator;
  uint32_t m_input_buffer_size;
  std::string m_compression_algorithm;

  daqdataformats::SourceID m_sourceid;

  // STATS
  std::atomic<int> m_packet_count{ 0 };
  std::atomic<int> m_packet_count_tot{ 0 };

  // sourceemulatorconfig::Conf m_cfg;

  // RAW SENDER
  std::chrono::milliseconds m_raw_sender_timeout_ms;
  using raw_sender_ct = iomanager::SenderConcept<ReadoutType>;
  std::shared_ptr<raw_sender_ct> m_raw_data_sender;

  bool m_sender_is_set = false;
  // using module_conf_t = dunedaq::snbmodules::sourceemulatorconfig::Conf;
  // module_conf_t m_conf;
  // using link_conf_t = dunedaq::snbmodules::sourceemulatorconfig::LinkConfiguration;
  // link_conf_t m_link_conf;

  std::unique_ptr<dunedaq::datahandlinglibs::RateLimiter> m_rate_limiter;
  std::unique_ptr<dunedaq::datahandlinglibs::BufferedFileReader<ReadoutType>> m_file_reader;

  utilities::ReusableThread m_producer_thread;
};

} // namespace snbmodules
} // namespace dunedaq

// Declarations
#include "detail/FileSourceModel.hxx"

#endif // snbmodules_INCLUDE_snbmodules_MODELS_FileSourceModel_HPP_
