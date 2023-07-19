
#ifndef SNBMODULES_INCLUDE_SNBMODULES_SNBTRANSFERBOOKKEEPER_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_SNBTRANSFERBOOKKEEPER_HPP_

#include "snbmodules/bookkeeper.hpp"

#include "appfwk/DAQModule.hpp"

#include "utilities/WorkerThread.hpp"

// #include <ers/Issue.h>

#include <memory>
#include <string>
#include <vector>

namespace dunedaq
{
    namespace snbmodules
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

            void do_tr_new(const nlohmann::json &args);
            void do_tr_start(const nlohmann::json &args);
            void do_tr_pause(const nlohmann::json &args);
            void do_tr_resume(const nlohmann::json &args);
            void do_tr_stop(const nlohmann::json &args);

            // Configuration
            Bookkeeper *m_bookkeeper;
            std::string m_name;

            // Threading
            dunedaq::utilities::WorkerThread *m_thread;
        };
    } // namespace readoutmodules
} // namespace dunedaq

#endif // SNBMODULES_INCLUDE_SNBMODULES_SNBTRANSFERBOOKKEEPER_HPP_