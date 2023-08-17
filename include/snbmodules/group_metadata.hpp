#ifndef SNBMODULES_INCLUDE_SNBMODULES_GROUP_METADATA_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_GROUP_METADATA_HPP_

#include "snbmodules/metadata_abstract.hpp"
#include "snbmodules/transfer_metadata.hpp"
#include "snbmodules/common/protocols_enum.hpp"
#include "snbmodules/tools/natural_sort.hpp"
#include "appfwk/cmd/Nljs.hpp"

#include <set>
#include <filesystem>
#include <string>
#include <vector>
#include <utility>

namespace dunedaq::snbmodules
{
    class GroupMetadata : public MetadataAbstract
    {

    public:
        static const std::string m_file_extension;

        void generate_metadata_file(std::filesystem::path dest) override;
        void load_metadata_from_meta_file(std::filesystem::path src) override;
        std::string export_to_string() override;
        void from_string(const std::string &) override;

        /// @brief Return a status of the group transfer based on the status of his transfers,
        ///        The highest status in priority order is returned
        /// @return Status of the group transfer
        e_status get_group_status() const
        {
            e_status status = e_status::PREPARING;
            for (const TransferMetadata &transfer : m_transfers_meta)
            {
                if (transfer.get_status() > status)
                {
                    status = transfer.get_status();
                }
            }
            return status;
        }

        /// @brief Constructor
        /// @param protocol
        /// @param transfers_meta
        /// @param transfer_id
        GroupMetadata(std::string group_id, std::string source, IPFormat source_ip, e_protocol_type protocol, const nlohmann::json &protocol_options = nlohmann::json(), const std::vector<TransferMetadata> &transfers_meta = std::vector<TransferMetadata>())
            : m_group_id(group_id),
              m_protocol(protocol),
              m_transfers_meta(transfers_meta),
              m_expected_files(std::set<std::string>()),
              m_source_id(std::move(source)),
              m_source_ip(source_ip)
        {
            set_protocol_options(protocol_options);
        }

        /// @brief Load from file constructor
        explicit GroupMetadata(const std::filesystem::path &src, bool is_path = true)
        {
            if (is_path)
            {
                load_metadata_from_meta_file(src);
            }
            else
            {
                from_string(src.string());
            }
        }

        virtual ~GroupMetadata() = default;

        /// @brief  Operator ==
        /// @param other  Other GroupMetadata to compare
        /// @return  True if the transfer_id are equal, false otherwise
        bool operator==(MetadataAbstract const &other) const override
        {
            auto o = dynamic_cast<const GroupMetadata *>(&other);
            return m_group_id == o->m_group_id;
        }

        /// @brief  Operator <
        /// @param other  Other GroupMetadata to compare
        /// @return  True if the transfer_id is less than the other transfer_id, false otherwise
        bool operator<(MetadataAbstract const &other) const override
        {
            auto o = dynamic_cast<const GroupMetadata *>(&other);
            return SI::natural::compare<std::string>(m_group_id, o->m_group_id);
        }

        // Setters
        inline void set_group_id(std::string transfer_id) { m_group_id = std::move(transfer_id); }
        inline void set_protocol(e_protocol_type protocol) { m_protocol = protocol; }
        inline void set_transfers_meta(std::vector<TransferMetadata> files_meta) { m_transfers_meta = std::move(files_meta); }
        inline void set_protocol_options(nlohmann::json protocol_options) { m_protocol_options = std::move(protocol_options); }
        inline void set_source_id(std::string source_id) { m_source_id = std::move(source_id); }
        inline void set_expected_files(std::set<std::string> expected_files) { m_expected_files = std::move(expected_files); }
        TransferMetadata &add_file(TransferMetadata meta);
        void add_expected_file(const std::filesystem::path &file)
        {
            // remove all occurences of ./ in the file path
            std::string file_path_str = file.string();
            std::string x = "./";

            size_t pos = 0;
            while (true)
            {
                pos = file_path_str.find(x, pos);
                if (pos == std::string::npos)
                {
                    break;
                }

                file_path_str.replace(pos, x.length(), "");
            }

            m_expected_files.insert(std::filesystem::absolute(file_path_str));
        }

        // Getters
        inline std::string get_group_id() const { return m_group_id; }
        inline e_protocol_type get_protocol() const { return m_protocol; }
        inline std::vector<TransferMetadata> &get_transfers_meta() { return m_transfers_meta; }
        inline const std::vector<TransferMetadata> &get_transfers_meta() const { return m_transfers_meta; }
        TransferMetadata &get_transfer_meta_from_file_path(std::string file_path);
        inline const std::set<std::string> &get_expected_files() const { return m_expected_files; }
        inline nlohmann::json get_protocol_options() const { return m_protocol_options; }
        inline std::string get_source_id() const { return m_source_id; }
        inline IPFormat get_source_ip() const { return m_source_ip; }
        std::string to_string() const;

    private:
        /// @brief Unique identifier for the transfer
        std::string m_group_id;

        /// @brief Protocol used for the transfer, every files in the transfer must use the same protocol
        e_protocol_type m_protocol;

        /// Specific options for the selected protocol
        nlohmann::json m_protocol_options;

        /// @brief Set of files to transfer
        std::vector<TransferMetadata> m_transfers_meta;

        /// @brief Set of expected files metadata to add later
        std::set<std::string> m_expected_files;

        /// @brief  Get source id
        std::string m_source_id;

        /// @brief  Get source ip
        IPFormat m_source_ip;
    };

} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_GROUP_METADATA_HPP_