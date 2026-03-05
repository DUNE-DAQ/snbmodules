// Declarations for FileSourceModel

#include "datahandlinglibs/DataHandlingIssues.hpp"
#include "datahandlinglibs/ReadoutLogging.hpp"
#include "datahandlinglibs/DataMoveCallbackRegistry.hpp"

using dunedaq::datahandlinglibs::CannotWriteToQueue;
using dunedaq::datahandlinglibs::ConfigurationError;
using dunedaq::datahandlinglibs::logging::TLVL_BOOKKEEPING;
using dunedaq::datahandlinglibs::logging::TLVL_TAKE_NOTE;
using dunedaq::datahandlinglibs::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace snbmodules {

template<class ReadoutType>
void
FileSourceModel<ReadoutType>::set_sender(const appmodel::DataMoveCallbackConf* sink)
{
  m_raw_sender_conf = sink;
}

template<class ReadoutType>
void
FileSourceModel<ReadoutType>::conf(const confmodel::DetectorStream* link_conf,
                                   const appmodel::SNBFileSourceParameters* file_params)
{
  if (m_is_configured) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "This emulator is already configured!";
  } else {
    // m_conf = args.get<module_conf_t>();
    // m_link_conf = link_conf.get<link_conf_t>();
    m_raw_sender_timeout_ms = std::chrono::milliseconds(1000);

    m_sourceid.id = link_conf->get_source_id();
    m_sourceid.subsystem = ReadoutType::subsystem;

    m_file_names = file_params->get_data_files();
    m_file_iterator = m_file_names.begin();

    m_input_buffer_size = file_params->get_input_buffer_size();
    m_compression_algorithm = file_params->get_file_compression_algorithm();

    open_next_file();

    m_is_configured = true;
  }
  // Configure thread:
  m_producer_thread.set_name("fileread", m_sourceid.id);
}

template<class ReadoutType>
void
FileSourceModel<ReadoutType>::open_next_file()
{
  if (m_file_iterator == m_file_names.end()) {
    if (m_file_reader) {
      m_file_reader->close();
    }
    m_file_reader.reset();
    m_is_configured = false;
    return;
  }
  try {
    m_file_reader = std::make_unique<datahandlinglibs::BufferedFileReader<ReadoutType>>(
      *m_file_iterator, m_input_buffer_size, m_compression_algorithm);
    ++m_file_iterator;
  } catch (const ers::Issue& ex) {
    ers::fatal(ex);
    throw ConfigurationError(ERS_HERE, m_sourceid, "", ex);
  }
}

template<class ReadoutType>
void
FileSourceModel<ReadoutType>::start(const appfwk::DAQModule::CommandData_t& /*args*/)
{
  m_packet_count_tot = 0;
  TLOG_DEBUG(TLVL_WORK_STEPS) << "Starting threads...";
  // FIXME: don't know where to take the slowdown from... m_rate_limiter = std::make_unique<RateLimiter>(m_rate_khz /
  // m_link_conf.slowdown);
  m_rate_limiter = std::make_unique<datahandlinglibs::RateLimiter>(m_rate_khz);
  // m_stats_thread.set_work(&FileSourceModel<ReadoutType>::run_stats, this);
  m_producer_thread.set_work(&FileSourceModel<ReadoutType>::run_produce, this);
}

template<class ReadoutType>
void
FileSourceModel<ReadoutType>::stop(const appfwk::DAQModule::CommandData_t& /*args*/)
{
  while (!m_producer_thread.get_readiness()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

template<class ReadoutType>
void
FileSourceModel<ReadoutType>::generate_opmon_data()
{
  datahandlinglibs::opmon::DataSourceInfo info;
  info.set_sum_packets(m_packet_count_tot.load());
  info.set_num_packets(m_packet_count.exchange(0));

  this->publish(std::move(info));
}

template<class ReadoutType>
void
FileSourceModel<ReadoutType>::run_produce()
{
  TLOG_DEBUG(TLVL_WORK_STEPS) << "Data generation thread " << m_sourceid.to_string() << " started";

  // pthread_setname_np(pthread_self(), get_name().c_str());

  ReadoutType elem;
  while (m_run_marker.load()) {
    auto read_sts = m_file_reader->read(elem);

    if (!read_sts) {
      TLOG_DEBUG(TLVL_BOOKKEEPING) << "Failed to read, moving to next file";
      open_next_file();
      if (!m_is_configured) {
        break;
      }
      continue;
    }

    TLOG_DEBUG(TLVL_BOOKKEEPING) << "Read element with timestamp " << elem.get_timestamp() << " from file";

    // send it
    while (m_raw_data_callback == nullptr && m_run_marker.load()) {
      m_raw_data_callback =
        datahandlinglibs::DataMoveCallbackRegistry::get()->get_callback<ReadoutType>(m_raw_sender_conf);
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Sender not set yet, waiting...";
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!m_run_marker.load()) {
      break;
    }
    (*m_raw_data_callback)(std::move(elem));

    // Count packet and limit rate if needed.
    ++m_packet_count;
    ++m_packet_count_tot;

    m_rate_limiter->limit();
  }
  TLOG_DEBUG(TLVL_WORK_STEPS) << "Data generation thread " << m_sourceid.to_string() << " finished";
}

} // namespace snbmodules
} // namespace dunedaq
