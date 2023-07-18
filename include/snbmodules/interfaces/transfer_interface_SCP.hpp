

#ifndef SNBMODULES_INCLUDE_SNBMODULES_TRANSFERINTERFACESCP_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_TRANSFERINTERFACESCP_HPP_

#include "snbmodules/interfaces/transfer_interface_abstract.hpp"
#include "snbmodules/common/status_enum.hpp"

#include <chrono>
#include <iostream>
#include <stdio.h>
#include <fstream>

namespace dunedaq::snbmodules
{

    class TransferInterfaceSCP : public TransferInterfaceAbstract
    {
    private:
        struct scp_parameters
        {
            std::string user;
            bool use_password = false;
        } m_params;

        bool m_is_uploader;
        std::map<std::string, std::filesystem::path> m_files_being_transferred;

    public:
        TransferInterfaceSCP(GroupMetadata *config, bool is_uploader) : TransferInterfaceAbstract(config)
        {
            m_params.user = get_transfer_options()->get_protocol_options()["user"].get<std::string>();
            m_params.use_password = get_transfer_options()->get_protocol_options()["use_password"].get<bool>();
            m_is_uploader = is_uploader;
        }
        virtual ~TransferInterfaceSCP() = default;

        virtual bool upload_file(TransferMetadata *f_meta) override
        {
            TLOG() << "debug : SCP : Uploading file " << f_meta->get_file_name();
            f_meta->set_status(e_status::UPLOADING);

            // nothing to do
            TLOG() << "debug : SCP : Sucess Upload";
            f_meta->set_status(e_status::FINISHED);
            f_meta->set_bytes_transferred(f_meta->get_size());
            return true;
        }
        virtual bool download_file(TransferMetadata *f_meta, std::filesystem::path dest) override
        {
            TLOG() << "debug : SCP : Downloading file " << f_meta->get_file_name();
            f_meta->set_status(e_status::DOWNLOADING);

            m_files_being_transferred[f_meta->get_file_name()] = dest;

            char exec[180];
            if (m_params.use_password)
            {
                sprintf(exec, "scp %s@%s:%s %s", m_params.user.c_str(), f_meta->get_src().get_ip().c_str(), f_meta->get_file_path().string().c_str(), dest.c_str());
            }
            else
                sprintf(exec, "scp -o PasswordAuthentication='no' %s@%s:%s %s", m_params.user.c_str(), f_meta->get_src().get_ip().c_str(), f_meta->get_file_path().string().c_str(), dest.c_str());

            TLOG() << "debug : executing " << exec;
            if (system(exec) == 0)
            {
                TLOG() << "debug : SCP : Sucess Download";
                f_meta->set_status(e_status::FINISHED);
                f_meta->set_bytes_transferred(f_meta->get_size());
            }
            else
            {
                ers::error(ErrorSCPDownloadError(ERS_HERE, "Please check the logs for more information."));
                f_meta->set_status(e_status::ERROR);
                f_meta->set_error_code("Something went wrong during the download");
                f_meta->set_bytes_transferred(0);
                return false;
            }

            return true;
        }

        virtual bool pause_file(TransferMetadata *f_meta) override
        {
            TLOG() << "debug : SCP : Pausing file " << f_meta->get_file_name();
            f_meta->set_status(e_status::PAUSED);
            f_meta->set_bytes_transferred(0);
            return true;
        }

        virtual bool resume_file(TransferMetadata *f_meta) override
        {
            TLOG() << "debug : SCP : Resuming file " << f_meta->get_file_name();

            if (m_is_uploader)
            {
                return upload_file(f_meta);
            }
            else
            {
                return download_file(f_meta, m_files_being_transferred[f_meta->get_file_name()]);
            }
        }

        virtual bool hash_file(TransferMetadata *f_meta) override
        {
            TLOG() << "debug : SCP : Hashing file " << f_meta->get_file_name();
            f_meta->set_status(e_status::HASHING);
            return true;
        }

        virtual bool cancel_file(TransferMetadata *f_meta) override
        {
            TLOG() << "debug : SCP : Cancelling file " << f_meta->get_file_name();
            f_meta->set_status(e_status::CANCELLED);
            return true;
        }
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_TRANSFERINTERFACESCP_HPP_