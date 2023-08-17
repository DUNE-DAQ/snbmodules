/**
 * @file SNBFileTransfer.hpp Module to transfer files between nodes
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_PLUGINS_SNBFILETRANSFER_HPP_
#define SNBMODULES_PLUGINS_SNBFILETRANSFER_HPP_

#include "snbmodules/transfer_client.hpp"
#include "snbmodules/common/protocols_enum.hpp"

#include "appfwk/DAQModule.hpp"

#include "utilities/WorkerThread.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq::snbmodules
{

    /// @brief SNBFileTransfer is a DAQModule that transfers files between clients and send transfer status to a bookkeeper
    class SNBFileTransfer : public dunedaq::appfwk::DAQModule
    {
    public:
        explicit SNBFileTransfer(const std::string &name);

        SNBFileTransfer(const SNBFileTransfer &) = delete;
        SNBFileTransfer &operator=(const SNBFileTransfer &) = delete;
        SNBFileTransfer(SNBFileTransfer &&) = delete;
        SNBFileTransfer &operator=(SNBFileTransfer &&) = delete;

        void init(const nlohmann::json &obj) override;
        // void get_info(opmonlib::InfoCollector &ci, int level) override;

    private:
        // Commands
        void do_conf(const nlohmann::json &obj);
        void do_start(const nlohmann::json &obj);
        void do_stop(const nlohmann::json &obj);
        void do_scrap(const nlohmann::json &obj);

        void do_tr_new(const nlohmann::json &args);
        void do_tr_start(const nlohmann::json &args);
        void do_tr_pause(const nlohmann::json &args);
        void do_tr_resume(const nlohmann::json &args);
        void do_tr_cancel(const nlohmann::json &args);

        // Configuration
        std::shared_ptr<TransferClient> m_client;
        std::string m_name;

        // Threading
        std::unique_ptr<dunedaq::utilities::WorkerThread> m_thread;
    };
} // namespace dunedaq::snbmodules

#endif // SNBMODULES_PLUGINS_SNBFILETRANSFER_HPP_