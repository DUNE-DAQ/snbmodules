// Declarations for SNBRequestHandlerModel

namespace dunedaq {
namespace snbmodules {

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::conf(const appmodel::DataHandlerModule* conf)
{

  auto reqh_conf = conf->get_module_configuration()->get_request_handler();
  m_sourceid.id = conf->get_source_id();
  m_sourceid.subsystem = RDT::subsystem;
  m_detid = conf->get_detector_id();

  m_buffer_capacity = conf->get_module_configuration()->get_latency_buffer()->get_size();
  m_num_request_handling_threads = reqh_conf->get_handler_threads();
  m_request_timeout_ms = reqh_conf->get_request_timeout();

  for (auto output : conf->get_outputs()) {
    if (output->get_data_type() == "Fragment") {
      m_fragment_send_timeout_ms = output->get_send_timeout_ms();
      // 19-Dec-2024, KAB: store the names/IDs of the Fragment output connections so that
      // we can confirm that they are ready for sending at 'start' time.
      m_frag_out_conn_ids.push_back(output->UID());
    }
  }

  if (m_recording_configured == false) {
    auto dr = reqh_conf->get_data_recorder();
    if (dr != nullptr) {
      m_output_file = dr->get_output_file();
      if (remove(m_output_file.c_str()) == 0) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Removed existing output file from previous run: " << m_output_file << std::endl;
      }
      m_stream_buffer_size = dr->get_streaming_buffer_size();
      m_buffered_writer.open(
        m_output_file, m_stream_buffer_size, dr->get_compression_algorithm(), dr->get_use_o_direct());
      m_recording_configured = true;
    }
  }

  m_warn_on_timeout = reqh_conf->get_warn_on_timeout();
  m_warn_about_empty_buffer = reqh_conf->get_warn_on_empty_buffer();
  m_periodic_data_transmission_ms = reqh_conf->get_periodic_data_transmission_ms();

  m_recording_thread.set_name("recording", m_sourceid.id);
  m_cleanup_thread.set_name("cleanup", m_sourceid.id);
  m_periodic_transmission_thread.set_name("periodic", m_sourceid.id);

  std::ostringstream oss;
  oss << "RequestHandler configured. ";
  TLOG_DEBUG(TLVL_WORK_STEPS) << oss.str();
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::scrap(const appfwk::DAQModule::CommandData_t& /*args*/)
{
  if (m_buffered_writer.is_open()) {
    m_buffered_writer.close();
  }
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::start(const appfwk::DAQModule::CommandData_t& /*args*/)
{
  // Reset opmon variables
  m_num_requests_found = 0;
  m_num_requests_bad = 0;
  m_num_requests_old_window = 0;
  m_num_requests_delayed = 0;
  m_num_requests_uncategorized = 0;
  m_num_buffer_cleanups = 0;
  m_num_requests_timed_out = 0;
  m_handled_requests = 0;
  m_response_time_acc = 0;
  m_pop_reqs = 0;
  m_pops_count = 0;
  m_payloads_written = 0;
  m_bytes_written = 0;

  m_t0 = std::chrono::high_resolution_clock::now();

  // 19-Dec-2024, KAB: check that Fragment senders are ready to send. This is done so
  // that the IOManager infrastructure fetches the necessary connection details from
  // the ConnectivityService at 'start' time, instead of the first time that the sender
  // is used to send data.  This avoids delays in the sending of the first fragment in
  // the first data-taking run in a DAQ session. Such delays can lead to undesirable
  // system behavior like trigger inhibits.
  for (auto frag_out_conn : m_frag_out_conn_ids) {
    auto sender = get_iom_sender<std::unique_ptr<daqdataformats::Fragment>>(frag_out_conn);
    if (sender != nullptr) {
      bool is_ready = sender->is_ready_for_sending(std::chrono::milliseconds(100));
      TLOG_DEBUG(0) << "The Fragment sender for " << frag_out_conn << " " << (is_ready ? "is" : "is not")
                    << " ready, my source_id is [" << m_sourceid << "]";
    }
  }

  m_request_handler_thread_pool = std::make_unique<boost::asio::thread_pool>(m_num_request_handling_threads);

  m_run_marker.store(true);
  m_cleanup_thread.set_work(&SNBRequestHandlerModel<RDT, LBT>::periodic_cleanups, this);
  if (m_periodic_data_transmission_ms > 0) {
    m_periodic_transmission_thread.set_work(&SNBRequestHandlerModel<RDT, LBT>::periodic_data_transmissions, this);
  }

  m_waiting_queue_thread = std::thread(&SNBRequestHandlerModel<RDT, LBT>::check_waiting_requests, this);
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::stop(const appfwk::DAQModule::CommandData_t& /*args*/)
{
  m_run_marker.store(false);
  while (!m_recording_thread.get_readiness()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  while (!m_cleanup_thread.get_readiness()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  while (!m_periodic_transmission_thread.get_readiness()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  TLOG() << "Latency buffer occupancy at stop: " << m_latency_buffer->occupancy();
  m_waiting_queue_thread.join();
  m_request_handler_thread_pool->join();
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::record(const appfwk::DAQModule::CommandData_t& /*args*/)
{
  ers::error(datahandlinglibs::CommandError(ERS_HERE, m_sourceid, "DLH is not configured for recording"));
  return;
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::cleanup_check()
{
  std::unique_lock<std::mutex> lock(m_cv_mutex);
  if (!m_pop_list.empty() && !m_cleanup_requested.exchange(true)) {
    m_cv.wait(lock, [&] { return m_requests_running == 0; });
    cleanup();
    m_cleanup_requested = false;
    m_cv.notify_all();
  }
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::issue_request(dfmessages::DataRequest datarequest, bool is_retry)
{
  boost::asio::post(*m_request_handler_thread_pool, [&, datarequest, is_retry]() { // start a thread from pool
    auto t_req_begin = std::chrono::high_resolution_clock::now();
    {
      std::unique_lock<std::mutex> lock(m_cv_mutex);
      m_cv.wait(lock, [&] { return !m_cleanup_requested; });
      m_requests_running++;
    }
    m_cv.notify_all();
    auto result = data_request(datarequest);
    {
      std::lock_guard<std::mutex> lock(m_cv_mutex);
      m_requests_running--;
    }
    m_cv.notify_all();
    if ((result.result_code == ResultCode::kNotYet || result.result_code == ResultCode::kPartial) &&
        m_request_timeout_ms > 0 && is_retry == false) {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Re-queue request. "
                                  << " with timestamp=" << result.data_request.trigger_timestamp;
      std::lock_guard<std::mutex> wait_lock_guard(m_waiting_requests_lock);
      m_waiting_requests.push_back(RequestElement(datarequest, std::chrono::high_resolution_clock::now()));
    } else {
      try { // Send to fragment connection
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Sending fragment with trigger/sequence_number "
                                    << result.fragment->get_trigger_number() << "."
                                    << result.fragment->get_sequence_number() << ", run number "
                                    << result.fragment->get_run_number() << ", and DetectorID "
                                    << result.fragment->get_detector_id() << ", and SourceID "
                                    << result.fragment->get_element_id() << ", and size " << result.fragment->get_size()
                                    << ", and result code " << result.result_code;
        // Send fragment
        get_iom_sender<std::unique_ptr<daqdataformats::Fragment>>(datarequest.data_destination)
          ->send(std::move(result.fragment), std::chrono::milliseconds(m_fragment_send_timeout_ms));

      } catch (const ers::Issue& excpt) {
        ers::warning(datahandlinglibs::CannotWriteToQueue(ERS_HERE, m_sourceid, datarequest.data_destination, excpt));
      }
    }

    auto t_req_end = std::chrono::high_resolution_clock::now();
    auto us_req_took = std::chrono::duration_cast<std::chrono::microseconds>(t_req_end - t_req_begin);
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Responding to data request took: " << us_req_took.count() << "[us]";
    m_response_time_acc.fetch_add(us_req_took.count());
    if (us_req_took.count() > m_response_time_max.load())
      m_response_time_max.store(us_req_took.count());
    if (us_req_took.count() < m_response_time_min.load())
      m_response_time_min.store(us_req_took.count());
    m_handled_requests++;
  });
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::generate_opmon_data()
{
  datahandlinglibs::opmon::RequestHandlerInfo info;

  info.set_num_requests_handled(m_handled_requests.exchange(0));
  info.set_num_requests_found(m_num_requests_found.exchange(0));
  info.set_num_requests_bad(m_num_requests_bad.exchange(0));
  info.set_num_requests_old_window(m_num_requests_old_window.exchange(0));
  info.set_num_requests_delayed(m_num_requests_delayed.exchange(0));
  info.set_num_requests_uncategorized(m_num_requests_uncategorized.exchange(0));
  info.set_num_requests_timed_out(m_num_requests_timed_out.exchange(0));
  info.set_num_requests_waiting(m_waiting_requests.size());

  int new_pop_reqs = 0;
  int new_pop_count = 0;
  int new_occupancy = 0;
  info.set_tot_request_response_time(m_response_time_acc.exchange(0));
  info.set_max_request_response_time(m_response_time_max.exchange(0));
  info.set_min_request_response_time(m_response_time_min.exchange(std::numeric_limits<int>::max()));
  auto now = std::chrono::high_resolution_clock::now();
  new_pop_reqs = m_pop_reqs.exchange(0);
  new_pop_count = m_pops_count.exchange(0);
  new_occupancy = m_occupancy;
  double seconds = std::chrono::duration_cast<std::chrono::microseconds>(now - m_t0).count() / 1000000.;
  TLOG_DEBUG(TLVL_HOUSEKEEPING) << "Cleanup request rate: " << new_pop_reqs / seconds / 1. << " [Hz]"
                                << " Dropped: " << new_pop_count << " Occupancy: " << new_occupancy;

  if (info.num_requests_handled() > 0) {

    info.set_avg_request_response_time(info.tot_request_response_time() / info.num_requests_handled());
    TLOG_DEBUG(TLVL_HOUSEKEEPING) << "Completed requests: " << info.num_requests_handled()
                                  << " | Avarage response time: " << info.avg_request_response_time() << "[us]"
                                  << " | Periodic sends: " << info.num_periodic_sent();
  }

  m_t0 = now;

  info.set_num_buffer_cleanups(m_num_buffer_cleanups.exchange(0));
  info.set_num_periodic_sent(m_num_periodic_sent.exchange(0));
  info.set_num_periodic_send_failed(m_num_periodic_send_failed.exchange(0));

  this->publish(std::move(info));

  datahandlinglibs::opmon::RecordingInfo rinfo;
  rinfo.set_recording_status(m_recording ? "Y" : "N");
  rinfo.set_packets_recorded(m_payloads_written.exchange(0));
  rinfo.set_bytes_recorded(m_bytes_written.exchange(0));
  this->publish(std::move(rinfo));
}

template<class RDT, class LBT>
std::unique_ptr<daqdataformats::Fragment>
SNBRequestHandlerModel<RDT, LBT>::create_empty_fragment(const dfmessages::DataRequest& dr)
{
  auto frag_header = create_fragment_header(dr);
  frag_header.error_bits |= (0x1 << static_cast<size_t>(daqdataformats::FragmentErrorBits::kDataNotFound));
  auto fragment = std::make_unique<daqdataformats::Fragment>(std::vector<std::pair<void*, size_t>>());
  fragment->set_header_fields(frag_header);
  return fragment;
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::periodic_cleanups()
{
  while (m_run_marker.load()) {
    cleanup_check();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::periodic_data_transmissions()
{
  while (m_run_marker.load()) {
    periodic_data_transmission();
    std::this_thread::sleep_for(std::chrono::milliseconds(m_periodic_data_transmission_ms));
  }
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::periodic_data_transmission()
{
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::cleanup()
{
  ++m_pop_reqs;
  unsigned popped = 0;
  {
    std::lock_guard<std::mutex> lk(m_pop_list_mutex);
    while (!m_pop_list.empty() && m_latency_buffer->occupancy() > 1) {
      auto ts = m_latency_buffer->front()->get_timestamp();
      if (m_pop_list.count(ts)) {
        m_latency_buffer->pop(1);
        popped++;
        m_pop_list.erase(ts);
      } else {
        break;
      }
    }
  }
  m_occupancy = m_latency_buffer->occupancy();
  m_pops_count += popped;
  m_error_registry->remove_errors_until(m_latency_buffer->front()->get_timestamp());

  // Update hte oldest timestamp monitorable
  m_oldest_timestamp = m_latency_buffer->front()->get_timestamp();
  m_num_buffer_cleanups++;
}

template<class RDT, class LBT>
void
SNBRequestHandlerModel<RDT, LBT>::check_waiting_requests()
{
  // At run stop, we wait until all waiting requests have either:
  //
  // 1. been serviced because an item past the end of the window arrived in the buffer
  // 2. timed out by going past m_request_timeout_ms, and returned a partial fragment
  while (m_run_marker.load()) {
    if (m_waiting_requests.size() > 0) {

      std::lock_guard<std::mutex> lock_guard(m_waiting_requests_lock);

      auto last_frame = m_latency_buffer->back();                                       // NOLINT
      uint64_t newest_ts = last_frame == nullptr ? std::numeric_limits<uint64_t>::min() // NOLINT(build/unsigned)
                                                 : last_frame->get_timestamp();

      for (auto iter = m_waiting_requests.begin(); iter != m_waiting_requests.end();) {
        if ((*iter).request.request_information.window_end <= newest_ts) {
          issue_request((*iter).request, true);
          iter = m_waiting_requests.erase(iter);
        } else if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -
                                                                         (*iter).start_time)
                     .count() >= m_request_timeout_ms) {
          issue_request((*iter).request, true);
          if (m_warn_on_timeout) {
            ers::warning(
              dunedaq::datahandlinglibs::VerboseRequestTimedOut(ERS_HERE,
                                                                m_sourceid,
                                                                (*iter).request.trigger_number,
                                                                (*iter).request.sequence_number,
                                                                (*iter).request.run_number,
                                                                (*iter).request.request_information.window_begin,
                                                                (*iter).request.request_information.window_end,
                                                                (*iter).request.data_destination));
          }
          m_num_requests_bad++;
          m_num_requests_timed_out++;
          iter = m_waiting_requests.erase(iter);
        } else {
          ++iter;
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

template<class RDT, class LBT>
std::vector<std::pair<void*, size_t>>
SNBRequestHandlerModel<RDT, LBT>::get_fragment_pieces(uint64_t start_win_ts, uint64_t end_win_ts, RequestResult& rres)
{

  TLOG_DEBUG(TLVL_WORK_STEPS) << "Looking for frags between " << start_win_ts << " and " << end_win_ts;

  std::vector<std::pair<void*, size_t>> frag_pieces;
  // Data availability is calculated here
  auto front_element = m_latency_buffer->front();     // NOLINT
  auto last_element = m_latency_buffer->back();       // NOLINT
  uint64_t last_ts = front_element->get_timestamp();  // NOLINT(build/unsigned)
  uint64_t newest_ts = last_element->get_timestamp(); // NOLINT(build/unsigned)

  if (start_win_ts > newest_ts) {
    // No element is as small as the start window-> request is far in the future
    rres.result_code = ResultCode::kNotYet; // give it another chance
  } else if (end_win_ts <= last_ts) {
    rres.result_code = ResultCode::kTooOld;
  } else {
    RDT request_element = RDT();
    auto start_timestamp = start_win_ts;
    request_element.set_timestamp(start_timestamp);

    auto start_iter = m_error_registry->has_error("MISSING_FRAMES")
                        ? m_latency_buffer->lower_bound(request_element, true)
                        : m_latency_buffer->lower_bound(request_element, false);
    if (!start_iter.good()) {
      // Accessor problem
      rres.result_code = ResultCode::kNotFound;
    } else {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Lower bound found " << start_iter->get_timestamp()
                                  << ", --> distance from window: "
                                  << int64_t(start_win_ts) - int64_t(start_iter->get_timestamp());
      if (end_win_ts > newest_ts) {
        rres.result_code = ResultCode::kPartial;
      } else if (start_win_ts < last_ts) {
        rres.result_code = ResultCode::kPartiallyOld;
      } else {
        rres.result_code = ResultCode::kFound;
      }

      auto elements_handled = 0;

      RDT* element = &(*start_iter);

      while (start_iter.good() && element->get_timestamp() <= end_win_ts) {
        std::lock_guard<std::mutex> lk(m_pop_list_mutex);
        if (m_pop_list.count(element->get_timestamp()) ) {
          TLOG_DEBUG(50) << "skip processing for current element " << element->get_timestamp()
                         << ", already included in trigger.";
        }
        else if (element->get_timestamp() < start_win_ts) {
          TLOG_DEBUG(50) << "skip processing for current element " << element->get_timestamp()
                         << ", out of readout window.";
        }

        else {
          // TLOG_DEBUG(50) << "Add element " << element->get_timestamp();
          //  SNB mode, the whole aggregated object (e.g.: superchunk) can be copied
          frag_pieces.emplace_back(
            std::make_pair<void*, size_t>(static_cast<void*>((*start_iter).begin()), element->get_payload_size()));

          m_pop_list.insert(element->get_timestamp());
        }

        elements_handled++;
        ++start_iter;
        element = &(*start_iter);
      }
    }
  }
  TLOG_DEBUG(TLVL_WORK_STEPS) << "*** Number of frames retrieved: " << frag_pieces.size();
  return frag_pieces;
}

template<class RDT, class LBT>
typename SNBRequestHandlerModel<RDT, LBT>::RequestResult
SNBRequestHandlerModel<RDT, LBT>::data_request(dfmessages::DataRequest dr)
{
  // Prepare response
  RequestResult rres(ResultCode::kUnknown, dr);

  // Prepare FragmentHeader and empty Fragment pieces list
  auto frag_header = create_fragment_header(dr);
  std::vector<std::pair<void*, size_t>> frag_pieces;
  std::ostringstream oss;

  // bool local_data_not_found_flag = false;
  if (m_latency_buffer->occupancy() == 0) {
    if (m_warn_about_empty_buffer) {
      ers::warning(datahandlinglibs::RequestOnEmptyBuffer(ERS_HERE, m_sourceid, "Data not found"));
    }
    frag_header.error_bits |= (0x1 << static_cast<size_t>(daqdataformats::FragmentErrorBits::kDataNotFound));
    rres.result_code = ResultCode::kNotFound;
    ++m_num_requests_bad;
  } else {
    frag_pieces = get_fragment_pieces(dr.request_information.window_begin, dr.request_information.window_end, rres);

    auto front_element = m_latency_buffer->front();     // NOLINT
    auto last_element = m_latency_buffer->back();       // NOLINT
    uint64_t last_ts = front_element->get_timestamp();  // NOLINT(build/unsigned)
    uint64_t newest_ts = last_element->get_timestamp(); // NOLINT(build/unsigned)
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Data request for trig/seq_num=" << dr.trigger_number << "." << dr.sequence_number
                                << " and SourceID[" << m_sourceid << "] with" << " Trigger TS=" << dr.trigger_timestamp
                                << " Oldest stored TS=" << last_ts << " Newest stored TS=" << newest_ts
                                << " Start of window TS=" << dr.request_information.window_begin
                                << " End of window TS=" << dr.request_information.window_end
                                << " Latency buffer occupancy=" << m_latency_buffer->occupancy()
                                << " frag_pieces result_code=" << rres.result_code
                                << " number of frag_pieces=" << frag_pieces.size();

    switch (rres.result_code) {
      case ResultCode::kTooOld:
        // return empty frag
        ++m_num_requests_old_window;
        ++m_num_requests_bad;
        frag_header.error_bits |= (0x1 << static_cast<size_t>(daqdataformats::FragmentErrorBits::kDataNotFound));
        break;
      case ResultCode::kPartiallyOld:
        ++m_num_requests_old_window;
        ++m_num_requests_found;
        frag_header.error_bits |= (0x1 << static_cast<size_t>(daqdataformats::FragmentErrorBits::kIncomplete));
        frag_header.error_bits |= (0x1 << static_cast<size_t>(daqdataformats::FragmentErrorBits::kDataNotFound));
        break;
      case ResultCode::kFound:
        ++m_num_requests_found;
        break;
      case ResultCode::kPartial:
        frag_header.error_bits |= (0x1 << static_cast<size_t>(daqdataformats::FragmentErrorBits::kIncomplete));
        ++m_num_requests_delayed;
        break;
      case ResultCode::kNotYet:
        frag_header.error_bits |= (0x1 << static_cast<size_t>(daqdataformats::FragmentErrorBits::kDataNotFound));
        ++m_num_requests_delayed;
        break;
      default:
        // Unknown result of data search
        ++m_num_requests_bad;
        frag_header.error_bits |= (0x1 << static_cast<size_t>(daqdataformats::FragmentErrorBits::kDataNotFound));
    }
  }
  // Create fragment from pieces
  rres.fragment = std::make_unique<daqdataformats::Fragment>(frag_pieces);

  // Set header
  rres.fragment->set_header_fields(frag_header);

  return rres;
}

} // namespace snbmodules
} // namespace dunedaq
