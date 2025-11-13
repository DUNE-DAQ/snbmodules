/**
 * @file SNBRequestHandlerModel.hpp SNB customizations for request handling
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef DATAHANDLINGLIBS_INCLUDE_DATAHANDLINGLIBS_MODELS_SNBRequestHandlerModel_HPP_
#define DATAHANDLINGLIBS_INCLUDE_DATAHANDLINGLIBS_MODELS_SNBRequestHandlerModel_HPP_

#include "datahandlinglibs/DataHandlingIssues.hpp"
#include "datahandlinglibs/concepts/RequestHandlerConcept.hpp"
#include "datahandlinglibs/utils/BufferedFileWriter.hpp"
#include "utilities/ReusableThread.hpp"

#include "datahandlinglibs/opmon/datahandling_info.pb.h"

#include "appmodel/DataHandlerConf.hpp"
#include "appmodel/DataHandlerModule.hpp"
#include "appmodel/DataRecorderConf.hpp"
#include "appmodel/LatencyBuffer.hpp"
#include "appmodel/RequestHandler.hpp"
#include "confmodel/Connection.hpp"
#include "confmodel/DaqModule.hpp"

#include "daqdataformats/Types.hpp"
#include "datahandlinglibs/FrameErrorRegistry.hpp"
#include "datahandlinglibs/ReadoutLogging.hpp"
#include "dfmessages/DataRequest.hpp"
#include "dfmessages/Fragment_serialization.hpp"
#include "logging/Logging.hpp"

#include <boost/asio.hpp>

#include <daqdataformats/FragmentHeader.hpp>
#include <folly/concurrency/UnboundedQueue.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <future>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using dunedaq::datahandlinglibs::logging::TLVL_HOUSEKEEPING;
using dunedaq::datahandlinglibs::logging::TLVL_QUEUE_PUSH;
using dunedaq::datahandlinglibs::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace snbmodules {

// This function takes the type returned by the begin() and end()
// functions in a ReadoutType object and returns the timestamp that
// should be used to determine whether the item pointed to by the
// iterator is within the readout window. The "frame" type
// ReadoutTypes all have a get_timestamp() function on the type
// pointed to by the iterator (eg, WIBFrame), so we make this
// "default" function return that. For other types, eg, those coming
// from DS, there isn't a get_timestamp() function, so a different
// template specialization is used in the appropriate package (eg,
// trigger)

template<class T>
uint64_t
get_frame_iterator_timestamp(T iter)
{
  return iter->get_timestamp();
}

template<class ReadoutType, class LatencyBufferType>
class SNBRequestHandlerModel : public datahandlinglibs::RequestHandlerConcept<ReadoutType, LatencyBufferType>
{
public:
  // Using shorter typenames
  using RDT = ReadoutType;
  using LBT = LatencyBufferType;

  using RequestResult =
    typename dunedaq::datahandlinglibs::RequestHandlerConcept<ReadoutType, LatencyBufferType>::RequestResult;
  using ResultCode =
    typename dunedaq::datahandlinglibs::RequestHandlerConcept<ReadoutType, LatencyBufferType>::ResultCode;

  // Explicit constructor with binding LB and error registry
  explicit SNBRequestHandlerModel(std::shared_ptr<LatencyBufferType>& latency_buffer,
                                  std::unique_ptr<datahandlinglibs::FrameErrorRegistry>& error_registry)
    : m_latency_buffer(latency_buffer)
    , m_recording_thread(0)
    , m_cleanup_thread(0)
    , m_waiting_requests()
    , m_waiting_requests_lock()
    , m_error_registry(error_registry)
    , m_buffer_capacity(0)
    , m_pop_counter{ 0 }
    , m_pop_reqs(0)
    , m_pops_count(0)
    , m_occupancy(0)
  //, m_response_time_log()
  //, m_response_time_log_lock()
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "SNBRequestHandlerModel created...";
  }

  // A struct that combines a data request, with the number of times it was issued internally
  struct RequestElement
  {
    RequestElement(const dfmessages::DataRequest& data_request,
                   const std::chrono::time_point<std::chrono::high_resolution_clock>& tp_value)
      : request(data_request)
      , start_time(tp_value)
    {
    }

    dfmessages::DataRequest request;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
  };

  // Default configuration mechanism
  void conf(const dunedaq::appmodel::DataHandlerModule*);

  // Default un-configure mechanism
  void scrap(const appfwk::DAQModule::CommandData_t& /*args*/) override;

  // Default start mechanism
  void start(const appfwk::DAQModule::CommandData_t& /*args*/);

  // Default stop mechanism
  void stop(const appfwk::DAQModule::CommandData_t& /*args*/);

  // Raw data recording implementation
  void record(const appfwk::DAQModule::CommandData_t& args) override;

  // A function that determines if a cleanup request should be issued based on LB occupancy
  void cleanup_check() override;

  // Periodic data transmission method invoked at configurable interval
  virtual void periodic_data_transmission() override;

  // Implementation of default request handling. (boost::asio post to a thread pool)
  void issue_request(dfmessages::DataRequest datarequest, bool is_retry = false) override;

  // Opmon get_info implementation
  // void get_info(opmonlib::InfoCollector& ci, int /*level*/) override;

  virtual dunedaq::daqdataformats::timestamp_t get_cutoff_timestamp() { return 0; }
  virtual bool supports_cutoff_timestamp() { return false; }

  // Resets last known/processed DAQ timestamp
  void reset_oldest_time() { m_oldest_timestamp.store(0); }

  // Returns last processed ReadoutTyped element's DAQ timestamp
  std::uint64_t get_oldest_time() override { return m_oldest_timestamp.load(); } // NOLINT(build/unsigned)

protected:
  // An inline helper function that creates a fragment header based on a data request
  inline daqdataformats::FragmentHeader create_fragment_header(const dfmessages::DataRequest& dr)
  {
    daqdataformats::FragmentHeader fh;
    fh.size = sizeof(fh);
    fh.trigger_number = dr.trigger_number;
    fh.trigger_timestamp = dr.trigger_timestamp;
    fh.window_begin = dr.request_information.window_begin;
    fh.window_end = dr.request_information.window_end;
    fh.run_number = dr.run_number;
    fh.fragment_type = static_cast<daqdataformats::fragment_type_t>(ReadoutType::fragment_type);
    fh.sequence_number = dr.sequence_number;
    fh.detector_id = m_detid;
    fh.element_id = { m_sourceid.subsystem, m_sourceid.id };
    return fh;
  }

  // Helper function that creates and empty fragment.
  std::unique_ptr<daqdataformats::Fragment> create_empty_fragment(const dfmessages::DataRequest& dr);

  // An inline helper function that merges a set of byte arrays into a destination array
  inline void dump_to_buffer(const void* data,
                             std::size_t size,
                             void* buffer,
                             uint32_t buffer_pos, // NOLINT(build/unsigned)
                             const std::size_t& buffer_size)
  {
    auto bytes_to_copy = size;
    while (bytes_to_copy > 0) {
      auto n = std::min(bytes_to_copy, buffer_size - buffer_pos);
      std::memcpy(static_cast<char*>(buffer) + buffer_pos, static_cast<const char*>(data), n);
      buffer_pos += n;
      bytes_to_copy -= n;
      if (buffer_pos == buffer_size) {
        buffer_pos = 0;
      }
    }
  }

  // Cleanup thread's work function. Runs the cleanup() routine
  void periodic_cleanups();

  // Periodic data transmission thread's work function. Runs the periodic_data_transmission() routine
  void periodic_data_transmissions();

  // LB cleanup implementation
  void cleanup();

  // Function that checks delayed requests that are waiting for not yet present data in LB
  void check_waiting_requests();

  // Function that gathers fragment pieces from LB
  std::vector<std::pair<void*, size_t>> get_fragment_pieces(uint64_t start_win_ts,
                                                            uint64_t end_win_ts,
                                                            RequestResult& rres);

  // Override data_request functionality
  RequestResult data_request(dfmessages::DataRequest dr) override;

  // operational monitoring
  virtual void generate_opmon_data() override;

  // Data access (LB)
  std::shared_ptr<LatencyBufferType>& m_latency_buffer;

  // Data recording
  datahandlinglibs::BufferedFileWriter<> m_buffered_writer;
  utilities::ReusableThread m_recording_thread;

  utilities::ReusableThread m_cleanup_thread;
  utilities::ReusableThread m_periodic_transmission_thread;

  // Bookkeeping of OOB requests
  std::map<dfmessages::DataRequest, int> m_request_counter;

  // Requests
  std::mutex m_cv_mutex;
  std::condition_variable m_cv;
  std::atomic<bool> m_cleanup_requested = false;
  std::atomic<int> m_requests_running = 0;
  std::vector<RequestElement> m_waiting_requests;
  std::mutex m_waiting_requests_lock;

  // Data extractor threads pool and corresponding requests
  std::unique_ptr<boost::asio::thread_pool> m_request_handler_thread_pool;
  size_t m_num_request_handling_threads = 0;

  // Error registry
  std::unique_ptr<datahandlinglibs::FrameErrorRegistry>& m_error_registry;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_t0;

  // The run marker
  std::atomic<bool> m_run_marker = false;
  // Threads and handles
  std::thread m_waiting_queue_thread;
  std::atomic<bool> m_recording = false;
  std::atomic<uint64_t> m_next_timestamp_to_record = std::numeric_limits<uint64_t>::max(); // NOLINT (build/unsigned)

  // Configuration
  bool m_configured;
  std::mutex m_pop_list_mutex;
  std::set<uint64_t> m_pop_list;
  size_t m_buffer_capacity;
  daqdataformats::SourceID m_sourceid;
  uint16_t m_detid;
  std::string m_output_file;
  size_t m_stream_buffer_size = 0;
  bool m_recording_configured = false;
  bool m_warn_on_timeout = true;         // Whether to warn when a request times out
  bool m_warn_about_empty_buffer = true; // Whether to warn about an empty buffer when processing a request
  uint32_t m_periodic_data_transmission_ms = 0;
  std::vector<std::string> m_frag_out_conn_ids;

  // Stats
  std::atomic<int> m_pop_counter;
  std::atomic<int> m_num_buffer_cleanups{ 0 };
  std::atomic<int> m_pop_reqs;
  std::atomic<int> m_pops_count;
  std::atomic<int> m_occupancy;
  std::atomic<int> m_num_requests_found{ 0 };
  std::atomic<int> m_num_requests_bad{ 0 };
  std::atomic<int> m_num_requests_old_window{ 0 };
  std::atomic<int> m_num_requests_delayed{ 0 };
  std::atomic<int> m_num_requests_uncategorized{ 0 };
  std::atomic<int> m_num_requests_timed_out{ 0 };
  std::atomic<int> m_handled_requests{ 0 };
  std::atomic<int> m_response_time_acc{ 0 };
  std::atomic<int> m_response_time_min{ std::numeric_limits<int>::max() };
  std::atomic<int> m_response_time_max{ 0 };
  std::atomic<int> m_payloads_written{ 0 };
  std::atomic<int> m_bytes_written{ 0 };
  std::atomic<uint64_t> m_num_periodic_sent{ 0 };        // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_num_periodic_send_failed{ 0 }; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_oldest_timestamp{ 0 };         // NOLINT(build/unsigned)

  // std::atomic<int> m_avg_req_count{ 0 }; // for opmon, later
  // std::atomic<int> m_avg_resp_time{ 0 };
  // Request response time log (kept for debugging if needed)
  // std::deque<std::pair<int, int>> m_response_time_log;
  // std::mutex m_response_time_log_lock;

  int m_fragment_send_timeout_ms;

private:
  int m_request_timeout_ms;
};

} // namespace snbmodules
} // namespace dunedaq

// Declarations
#include "detail/SNBRequestHandlerModel.hxx"

#endif // DATAHANDLINGLIBS_INCLUDE_DATAHANDLINGLIBS_MODELS_SNBRequestHandlerModel_HPP_
