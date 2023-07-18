/**
 * @file SNBFileTransfer.cpp
 *
 * Implementations of SNBFileTransfer's functions
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "SNBFileTransfer.hpp"

#include "snbmodules/snbfiletransfer/Nljs.hpp"
#include "snbmodules/snbfiletransferinfo/InfoNljs.hpp"

#include <string>

namespace dunedaq::snbmodules {

SNBFileTransfer::SNBFileTransfer(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
{
  register_command("conf", &SNBFileTransfer::do_conf);
}

void
SNBFileTransfer::init(const data_t& /* structured args */)
{}

void
SNBFileTransfer::get_info(opmonlib::InfoCollector& ci, int /* level */)
{
  snbfiletransferinfo::Info info;
  info.total_amount = m_total_amount;
  info.amount_since_last_get_info_call = m_amount_since_last_get_info_call.exchange(0);

  ci.add(info);
}

void
SNBFileTransfer::do_conf(const data_t& conf_as_json)
{
  auto conf_as_cpp = conf_as_json.get<snbfiletransfer::Conf>();
  m_some_configured_value = conf_as_cpp.some_configured_value;
}

} // namespace dunedaq::snbmodules

DEFINE_DUNE_DAQ_MODULE(dunedaq::snbmodules::SNBFileTransfer)
