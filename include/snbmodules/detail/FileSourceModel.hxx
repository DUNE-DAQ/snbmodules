// Declarations for FileSourceModel

#include "datahandlinglibs/DataHandlingIssues.hpp"
#include "datahandlinglibs/ReadoutLogging.hpp"

using dunedaq::datahandlinglibs::CannotWriteToQueue;
using dunedaq::datahandlinglibs::ConfigurationError;
using dunedaq::datahandlinglibs::logging::TLVL_TAKE_NOTE;
using dunedaq::datahandlinglibs::logging::TLVL_WORK_STEPS;
using dunedaq::datahandlinglibs::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace snbmodules {

template<class ReadoutType>
void
FileSourceModel<ReadoutType>::set_sender(const std::string& conn_name)
{
  if (!m_sender_is_set) {
    m_raw_data_sender = get_iom_sender<ReadoutType>(conn_name);
    m_sender_is_set = true;
  } else {
    // ers::error();
  }
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
    m_raw_sender_timeout_ms = std::chrono::milliseconds(1);

    m_sourceid.id = link_conf->get_source_id();
    m_sourceid.subsystem = ReadoutType::subsystem;

    m_crateid = link_conf->get_geo_id()->get_crate_id();
    m_slotid = link_conf->get_geo_id()->get_slot_id();
    m_linkid = link_conf->get_geo_id()->get_stream_id();

    m_t0_now = file_params->get_set_t0();
    try {
      m_file_reader = std::make_unique<datahandlinglibs::BufferedFileReader<ReadoutType>>(
        file_params->get_data_file_name(),
        file_params->get_input_buffer_size(),
        file_params->get_file_compression_algorithm());
    } catch (const ers::Issue& ex) {
      ers::fatal(ex);
      throw ConfigurationError(ERS_HERE, m_sourceid, "", ex);
    }

    m_is_configured = true;
  }
  // Configure thread:
  m_producer_thread.set_name("fileread", m_sourceid.id);
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
  TLOG_DEBUG(TLVL_WORK_STEPS) << "Data generation thread " << m_this_link_number << " started";

  // pthread_setname_np(pthread_self(), get_name().c_str());

  ReadoutType elem;
  while (m_run_marker.load()) {
    auto read_sts = m_file_reader->read(elem);

    while (!read_sts) {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "No elements to read from buffer! Sleeping...";
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      read_sts = m_file_reader->read(elem);
    }

    // set the initial timestamp to a configured value, otherwise just use the timestamp from the header
    uint64_t ts_0 = elem.get_timestamp(); // NOLINT(build/unsigned)
    if (m_t0_now) {
      auto time_now = std::chrono::system_clock::now().time_since_epoch();
      uint64_t current_time = // NOLINT (build/unsigned)
        std::chrono::duration_cast<std::chrono::microseconds>(time_now).count();
      // FIXME: where do I get the clockspeed from?
      // ts_0 = (m_conf.clock_speed_hz / 100000) * current_time;
      ts_0 = 625 * current_time / 10;
    }
    TLOG_DEBUG(TLVL_BOOKKEEPING) << "Using first timestamp: " << ts_0;
    uint64_t timestamp = ts_0; // NOLINT(build/unsigned)

    // Fake timestamp
    elem.fake_timestamps(timestamp, m_time_tick_diff);

    // Fake geoid
    elem.fake_geoid(m_crateid, m_slotid, m_linkid);

    // send it
    bool send_successful = false;
    while (!send_successful && m_run_marker.load()) {
      try {
        ReadoutType elem_copy(elem);
        m_raw_data_sender->send(std::move(elem_copy), m_raw_sender_timeout_ms);
        send_successful = true;
      } catch (ers::Issue& excpt) {
        ers::warning(CannotWriteToQueue(ERS_HERE, m_sourceid, "raw data input queue", excpt));
        // std::runtime_error("Queue timed out...");
      }
    }

    // Count packet and limit rate if needed.
    ++m_packet_count;
    ++m_packet_count_tot;

    m_rate_limiter->limit();
  }
  TLOG_DEBUG(TLVL_WORK_STEPS) << "Data generation thread " << m_sourceid.to_string() << " finished";
}

} // namespace snbmodules
} // namespace dunedaq
