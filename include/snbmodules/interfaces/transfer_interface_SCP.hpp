/**
 * @file transfer_interface_SCP.hpp TransferInterfaceSCP protocol class for a SCP transfer
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_INCLUDE_SNBMODULES_INTERFACES_TRANSFER_INTERFACE_SCP_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_INTERFACES_TRANSFER_INTERFACE_SCP_HPP_

#include "snbmodules/interfaces/transfer_interface_abstract.hpp"
#include "snbmodules/common/status_enum.hpp"

#include <chrono>
#include <iostream>
#include <cstdio>
#include <fstream>

#include <string>
#include <map>

namespace dunedaq::snbmodules
{

    class TransferInterfaceSCP : public TransferInterfaceAbstract
    {

    public:
        TransferInterfaceSCP(GroupMetadata &config, bool is_uploader) : TransferInterfaceAbstract(config)
        {
            if (config.get_protocol_options().contains("user"))
            {
                m_params.user = config.get_protocol_options()["user"].get<std::string>();
            }
            else
            {
                ers::fatal(ErrorSCPConfigError(ERS_HERE, "user is mandatory in SCP protocol options");
                return;
            }

            if (config.get_protocol_options().contains("use_password"))
            {
                m_params.use_password = config.get_protocol_options()["use_password"].get<bool>();
            }

            m_is_uploader = is_uploader;
        }
        virtual ~TransferInterfaceSCP() = default;

        bool upload_file(TransferMetadata &f_meta) override
        {
            TLOG() << "debug : SCP : Uploading file " << f_meta.get_file_name();
            f_meta.set_status(e_status::UPLOADING);

            // nothing to do
            TLOG() << "debug : SCP : Sucess Upload";
            f_meta.set_status(e_status::FINISHED);
            f_meta.set_bytes_transferred(f_meta.get_size());
            return true;
        }
        bool download_file(TransferMetadata &f_meta, std::filesystem::path dest) override
        {
            TLOG() << "debug : SCP : Downloading file " << f_meta.get_file_name();
            f_meta.set_status(e_status::DOWNLOADING);

            m_files_being_transferred[f_meta.get_file_name()] = dest;

            std::string exec = "";
            if (m_params.use_password)
            {
                exec = "scp " + m_params.user + "@" + f_meta.get_src().get_ip() + ":" + f_meta.get_file_path().string() + " " + dest.string();
            }
            else
            {

                exec = "scp -o PasswordAuthentication='no' " + m_params.user + "@" + f_meta.get_src().get_ip() + ":" + f_meta.get_file_path().string() + " " + dest.string();
            }

            TLOG() << "debug : executing " << exec;
            if (system(exec.c_str()) == 0) // NOLINT
            {
                TLOG() << "debug : SCP : Sucess Download";
                f_meta.set_status(e_status::FINISHED);
                f_meta.set_bytes_transferred(f_meta.get_size());
            }
            else
            {
                ers::error(ErrorSCPDownloadError(ERS_HERE, "Please check the logs for more information."));
                f_meta.set_status(e_status::ERROR);
                f_meta.set_error_code("Something went wrong during the download");
                f_meta.set_bytes_transferred(0);
                return false;
            }

            return true;
        }

        bool pause_file(TransferMetadata &f_meta) override
        {
            TLOG() << "debug : SCP : Pausing file " << f_meta.get_file_name();
            f_meta.set_status(e_status::PAUSED);
            f_meta.set_bytes_transferred(0);
            return true;
        }

        bool resume_file(TransferMetadata &f_meta) override
        {
            TLOG() << "debug : SCP : Resuming file " << f_meta.get_file_name();

            if (m_is_uploader)
            {
                return upload_file(f_meta);
            }
            else
            {
                return download_file(f_meta, m_files_being_transferred[f_meta.get_file_name()]);
            }
        }

        bool hash_file(TransferMetadata &f_meta) override
        {
            TLOG() << "debug : SCP : Hashing file " << f_meta.get_file_name();
            f_meta.set_status(e_status::HASHING);
            return true;
        }

        bool cancel_file(TransferMetadata &f_meta) override
        {
            TLOG() << "debug : SCP : Cancelling file " << f_meta.get_file_name();
            f_meta.set_status(e_status::CANCELLED);
            return true;
        }

    private:
        struct scp_parameters
        {
            std::string user;
            bool use_password = false;
        } m_params;

        bool m_is_uploader;
        std::map<std::string, std::filesystem::path> m_files_being_transferred;
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_INTERFACES_TRANSFER_INTERFACE_SCP_HPP_