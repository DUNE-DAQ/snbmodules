/**
 * @file SNBDataHandlingModel.hpp Glue between data source, payload raw processor,
 * latency buffer and request handler.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef SNBMODULES_INCLUDE_SNBMODULES_READOUT_SNBDATAHANDLINGMODEL_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_READOUT_SNBDATAHANDLINGMODEL_HPP_

#include "confmodel/DaqModule.hpp"
#include "confmodel/Connection.hpp"
#include "appmodel/DataHandlerModule.hpp"
#include "appmodel/DataHandlerConf.hpp"
#include "appmodel/RequestHandler.hpp"
#include "appmodel/LatencyBuffer.hpp"
#include "appmodel/DataProcessor.hpp"

#include "datahandlinglibs/opmon/datahandling_info.pb.h"

#include "iomanager/IOManager.hpp"
#include "iomanager/Sender.hpp"
#include "iomanager/Receiver.hpp"

#include "logging/Logging.hpp"

#include "daqdataformats/ComponentRequest.hpp"
#include "daqdataformats/Fragment.hpp"

#include "dfmessages/DataRequest.hpp"
#include "dfmessages/TimeSync.hpp"

#include "datahandlinglibs/ReadoutLogging.hpp"
#include "datahandlinglibs/concepts/DataHandlingConcept.hpp"
#include "appmodel/DataHandlerModule.hpp"

#include "datahandlinglibs/FrameErrorRegistry.hpp"

#include "datahandlinglibs/concepts/LatencyBufferConcept.hpp"
#include "datahandlinglibs/concepts/RawDataProcessorConcept.hpp"
#include "datahandlinglibs/concepts/RequestHandlerConcept.hpp"

#include "datahandlinglibs/DataHandlingIssues.hpp"
#include "utilities/ReusableThread.hpp"

#include <folly/coro/Baton.h>
#include <folly/coro/Task.h>
#include <folly/futures/ThreadWheelTimekeeper.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using dunedaq::datahandlinglibs::logging::TLVL_QUEUE_POP;
using dunedaq::datahandlinglibs::logging::TLVL_TAKE_NOTE;
using dunedaq::datahandlinglibs::logging::TLVL_TIME_SYNCS;
using dunedaq::datahandlinglibs::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace snbmodules {

template<class ReadoutType, class RequestHandlerType, class LatencyBufferType, class RawDataProcessorType, class InputDataType = ReadoutType>
class SNBDataHandlingModel : public datahandlinglibs::DataHandlingConcept
{
public:
  // Using shorter typenames
  using RDT = ReadoutType;
  using RHT = RequestHandlerType;
  using LBT = LatencyBufferType;
  using RPT = RawDataProcessorType;
  using IDT = InputDataType;

  // Using timestamp typenames
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)
  static inline constexpr timestamp_t ns = 1;
  static inline constexpr timestamp_t us = 1000 * ns;
  static inline constexpr timestamp_t ms = 1000 * us;
  static inline constexpr timestamp_t s = 1000 * ms;

  // Explicit constructor with run marker pass-through
  explicit SNBDataHandlingModel(std::atomic<bool>& run_marker)
    : m_run_marker(run_marker)
    , m_callback_mode(false)
    , m_fake_trigger(false)
    , m_current_fake_trigger_id(0)
    , m_consumer_thread(0)
    , m_raw_receiver_timeout_ms(0)
    , m_raw_receiver_sleep_us(0)
    , m_raw_data_receiver(nullptr)
    , m_timesync_thread(0)
    , m_latency_buffer_impl(nullptr)
    , m_raw_processor_impl(nullptr)
  {
  }

  virtual ~SNBDataHandlingModel() = default;

  // Initializes the readoutmodel and its internals
  void init(const appmodel::DataHandlerModule* modconf);

  // Configures the readoutmodel and its internals
  void conf(const appfwk::DAQModule::CommandData_t& args);

  // Unconfigures readoutmodel's internals
  void scrap(const appfwk::DAQModule::CommandData_t& args)
  {
    m_request_handler_impl->scrap(args);
    m_latency_buffer_impl->scrap(args);
    m_raw_processor_impl->scrap(args);
  }

  // Starts readoutmodel's internals
  void start(const appfwk::DAQModule::CommandData_t& args);

  // Stops readoutmodel's internals
  void stop(const appfwk::DAQModule::CommandData_t& args);

  // Record function: invokes request handler's record implementation
  void record(const appfwk::DAQModule::CommandData_t& args) override 
  { 
    m_request_handler_impl->record(args); 
  }

  // Opmon get_info call implementation
  //void get_info(opmonlib::InfoCollector& ci, int level);

  // Consume callback
  std::function<void(IDT&&)> m_consume_callback;

protected:

  // Perform processing operations on payload
  void process_item(RDT&& payload);
  
  // Transform payload if needed, then perform processing
  void transform_and_process(IDT&& payload);

  // Raw data consume callback
  void consume_callback(IDT&& payload);

  // Raw data consumer's work function
  void run_consume();

  // Timesync thread's work function
  void run_timesync();

  // Postprocess scheduler thread's work function
  void run_postprocess_scheduler();

  // Postprocess schedule coroutine
  folly::coro::Task<void> postprocess_schedule();  

  // Dispatch data request
  void dispatch_requests(dfmessages::DataRequest&& data_request);
  
  // Transform input data type to readout
  virtual std::vector<RDT> transform_payload(IDT& original) const
  {
    return { reinterpret_cast<RDT&>(original) };
  }

  // Operational monitoring
  virtual void generate_opmon_data() override;

  // Constructor params
  std::atomic<bool>& m_run_marker;

  // CONFIGURATION
  //appfwk::app::ModInit m_queue_config;
  bool m_callback_mode;
  bool m_fake_trigger;
  bool m_generate_timesync = false;
  int m_current_fake_trigger_id;
  daqdataformats::SourceID m_sourceid;
  daqdataformats::run_number_t m_run_number;
  uint64_t m_processing_delay_ticks;
  uint64_t m_post_processing_delay_min_wait;
  uint64_t m_post_processing_delay_max_wait;

  // STATS
  using metric_t = dunedaq::datahandlinglibs::opmon::DataHandlerInfo;
  using num_payload_t = std::remove_const<std::invoke_result<decltype(&metric_t::num_payloads),metric_t>::type>::type;
  using sum_payload_t = std::remove_const<std::invoke_result<decltype(&metric_t::sum_payloads),metric_t>::type>::type;
  using num_request_t = std::remove_const<std::invoke_result<decltype(&metric_t::num_requests),metric_t>::type>::type;
  using sum_request_t = std::remove_const<std::invoke_result<decltype(&metric_t::sum_requests),metric_t>::type>::type;
  using rawq_timeout_count_t = std::remove_const<std::invoke_result<decltype(&metric_t::num_data_input_timeouts),metric_t>::type>::type;
  using num_lb_insert_failures_t = std::remove_const<std::invoke_result<decltype(&metric_t::num_lb_insert_failures),metric_t>::type>::type;
  using num_post_processing_delay_max_waits_t = std::remove_const<std::invoke_result<decltype(&metric_t::num_post_processing_delay_max_waits),metric_t>::type>::type;

  std::atomic<num_payload_t> m_num_payloads{ 0 };
  std::atomic<sum_payload_t> m_sum_payloads{ 0 };
  std::atomic<num_request_t> m_num_requests{ 0 };
  std::atomic<sum_request_t> m_sum_requests{ 0 };
  std::atomic<rawq_timeout_count_t> m_rawq_timeout_count{ 0 };
  std::atomic<num_lb_insert_failures_t> m_num_lb_insert_failures{ 0 };
  std::atomic<num_post_processing_delay_max_waits_t> m_num_post_processing_delay_max_waits{ 0 };
  std::atomic<int> m_stats_packet_count{ 0 };

  // CONSUMER
  utilities::ReusableThread m_consumer_thread;

  // RAW RECEIVER
  std::chrono::milliseconds m_raw_receiver_timeout_ms;
  std::chrono::microseconds m_raw_receiver_sleep_us;
  using raw_receiver_ct = iomanager::ReceiverConcept<InputDataType>;
  std::shared_ptr<raw_receiver_ct> m_raw_data_receiver;
  std::string m_raw_data_receiver_connection_name;

  // REQUEST RECEIVERS
  using request_receiver_ct = iomanager::ReceiverConcept<dfmessages::DataRequest>;
  std::shared_ptr<request_receiver_ct> m_data_request_receiver;

  // FRAGMENT SENDER
  //std::chrono::milliseconds m_fragment_sender_timeout_ms;
  //using fragment_sender_ct = iomanager::SenderConcept<std::pair<std::unique_ptr<daqdataformats::Fragment>, std::string>>;
  //std::shared_ptr<fragment_sender_ct> m_fragment_sender;

  // TIME-SYNC
  using timesync_sender_ct = iomanager::SenderConcept<dfmessages::TimeSync>; // no timeout -> published
  std::shared_ptr<timesync_sender_ct> m_timesync_sender;
  utilities::ReusableThread m_timesync_thread;
  std::string m_timesync_connection_name;

  // POSTPROCESS SCHEDULER
  utilities::ReusableThread m_postprocess_scheduler_thread;
  folly::coro::Baton m_baton;
  std::unique_ptr<folly::ThreadWheelTimekeeper> m_timekeeper;

  // LATENCY BUFFER
  std::shared_ptr<LatencyBufferType> m_latency_buffer_impl;

  // RAW PROCESSING
  std::shared_ptr<RawDataProcessorType> m_raw_processor_impl;

  // REQUEST HANDLER
  std::shared_ptr<RequestHandlerType> m_request_handler_impl;
  bool m_request_handler_supports_cutoff_timestamp;

  // ERROR REGISTRY
  std::unique_ptr<datahandlinglibs::FrameErrorRegistry> m_error_registry;

  // RUN START T0
  std::chrono::time_point<std::chrono::high_resolution_clock> m_t0;
};

} // namespace snbmodules
} // namespace dunedaq

// Declarations
#include "detail/SNBDataHandlingModel.hxx"

#endif // SNBMODULES_INCLUDE_SNBMODULES_READOUT_SNBDATAHANDLINGMODEL_HPP_
