/**
 * @file SNBTransferBookkeeper.hpp SNBTransferBookkeeper Bookkeeper module retriving transfers informations from SNBFileTransfer clients.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_PLUGINS_SNBTRANSFERBOOKKEEPER_HPP_
#define SNBMODULES_PLUGINS_SNBTRANSFERBOOKKEEPER_HPP_

#include "snbmodules/bookkeeper.hpp"

#include "appfwk/DAQModule.hpp"

#include "utilities/WorkerThread.hpp"

// #include <ers/Issue.h>

#include <memory>
#include <string>
#include <vector>

namespace dunedaq::snbmodules
{
    /// @brief SNBTransferBookkeeper is a DAQModule that transfers files between clients and get transfer status
    class SNBTransferBookkeeper : public dunedaq::appfwk::DAQModule
    {
    public:
        explicit SNBTransferBookkeeper(const std::string &name);

        SNBTransferBookkeeper(const SNBTransferBookkeeper &) = delete;
        SNBTransferBookkeeper &operator=(const SNBTransferBookkeeper &) = delete;
        SNBTransferBookkeeper(SNBTransferBookkeeper &&) = delete;
        SNBTransferBookkeeper &operator=(SNBTransferBookkeeper &&) = delete;

        void init(const nlohmann::json &obj) override;
        // void get_info(opmonlib::InfoCollector &ci, int level) override;

    private:
        // Commands
        void do_conf(const nlohmann::json &obj);
        void do_start(const nlohmann::json &obj);
        void do_stop(const nlohmann::json &obj);
        void do_scrap(const nlohmann::json &obj);
        void do_info(const nlohmann::json &args);

        // Configuration
        std::shared_ptr<Bookkeeper> m_bookkeeper;
        std::string m_name;

        // Threading
        std::unique_ptr<dunedaq::utilities::WorkerThread> m_thread;
    };
} // namespace dunedaq::snbmodules

#endif // SNBMODULES_PLUGINS_SNBTRANSFERBOOKKEEPER_HPP_