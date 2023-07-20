

#ifndef SNBMODULES_INCLUDE_SNBMODULES_TRANSFERINTERFACE_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_TRANSFERINTERFACE_HPP_

#include "snbmodules/group_metadata.hpp"
#include "snbmodules/transfer_metadata.hpp"
#include "snbmodules/ip_format.hpp"
#include "appfwk/cmd/Nljs.hpp"

// errors handling
#include "snbmodules/common/errors_declaration.hpp"
#include "logging/Logging.hpp"

#include <set>
#include <iostream>

namespace dunedaq::snbmodules
{
    class TransferInterfaceAbstract
    {
    protected:
        /// @brief MetadataAbstract of the transfer, contain settings and status of the transfer
        GroupMetadata *m_config;

    public:
        TransferInterfaceAbstract(GroupMetadata *config) : m_config(config) {}
        virtual ~TransferInterfaceAbstract() = default;

        GroupMetadata *get_transfer_options() { return m_config; }

        virtual bool upload_file(TransferMetadata *f_meta) = 0;
        virtual bool download_file(TransferMetadata *f_meta, std::filesystem::path dest) = 0;
        virtual bool pause_file(TransferMetadata *f_meta) = 0;
        virtual bool resume_file(TransferMetadata *f_meta) = 0;
        virtual bool hash_file(TransferMetadata *f_meta) = 0;
        virtual bool cancel_file(TransferMetadata *f_meta) = 0;
    };

    /// @brief Example of a transfer interface class.
    /// This class is used to test the session class.
    class TransferInterfaceDummy : public TransferInterfaceAbstract
    {

    public:
        TransferInterfaceDummy(GroupMetadata *config) : TransferInterfaceAbstract(config) {}

        virtual bool upload_file(TransferMetadata *f_meta) override
        {
            TLOG() << "Upload file " << f_meta->get_file_name();
            return true;
        }
        virtual bool download_file(TransferMetadata *f_meta, std::filesystem::path dest) override
        {
            (void)dest;
            TLOG() << "Download file " << f_meta->get_file_name();
            return true;
        }
        virtual bool pause_file(TransferMetadata *f_meta) override
        {
            TLOG() << "Pause file " << f_meta->get_file_name();
            return true;
        }
        virtual bool resume_file(TransferMetadata *f_meta) override
        {
            TLOG() << "Resume file " << f_meta->get_file_name();
            return true;
        }
        virtual bool cancel_file(TransferMetadata *f_meta) override
        {
            TLOG() << "Cancel file " << f_meta->get_file_name();
            return true;
        }
        virtual bool hash_file(TransferMetadata *f_meta) override
        {
            TLOG() << "Hash file " << f_meta->get_file_name();
            return true;
        }
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_TRANSFERINTERFACE_HPP_