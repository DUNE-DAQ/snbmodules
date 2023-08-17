
#include "snbmodules/group_metadata.hpp"
#include "snbmodules/tools/magic_enum.hpp"

#include <iostream>
#include <stdexcept>
#include <filesystem>
namespace dunedaq::snbmodules
{

    const std::string GroupMetadata::m_file_extension = ".gmetadata";

    TransferMetadata &GroupMetadata::get_transfer_meta_from_file_path(std::string file_path)
    {
        for (TransferMetadata &meta : get_transfers_meta())
        {
            if (meta.get_file_path() == file_path)
            {
                return meta;
            }
        }
        ers::fatal(MetadataNotFoundInGroupError(ERS_HERE, m_group_id, file_path));
        return m_transfers_meta[0]; // To avoid warning
    }

    TransferMetadata &GroupMetadata::add_file(TransferMetadata meta)
    {
        if (m_expected_files.find(meta.get_file_path()) != m_expected_files.end())
        {
            m_expected_files.erase(meta.get_file_path());
            meta.set_group_id(m_group_id);

            return m_transfers_meta.emplace_back(std::move(meta));
        }
        else if (meta.get_group_id() == m_group_id)
        {
            int pos = -1, i = 0;
            for (const auto &m : m_transfers_meta)
            {
                if (m == meta)
                {
                    // Already inserted, update the transfer
                    pos = i;
                    break;
                }
                i++;
            }
            if (pos != -1)
            {
                m_transfers_meta.erase(m_transfers_meta.begin() + pos);
            }
            return m_transfers_meta.emplace_back(std::move(meta));
        }
        else
        {
            ers::fatal(MetadataNotExpectedInGroupError(ERS_HERE, m_group_id, meta.get_file_name()));
            return m_transfers_meta[0]; // To avoid warning
        }
    }

    std::string GroupMetadata::export_to_string()
    {
        nlohmann::json j;
        j["transfer_id"] = get_group_id();
        j["source_id"] = get_source_id();
        j["source_ip"] = get_source_ip().get_ip_port();
        j["protocol"] = static_cast<std::string>(magic_enum::enum_name(get_protocol()));
        j["protocol_options"] = get_protocol_options().dump();

        std::vector<std::string> files;
        for (const auto &file : get_transfers_meta())
        {
            files.push_back(file.get_file_path().string());
        }
        j["files"] = files;

        return j.dump();
    }

    void GroupMetadata::from_string(const std::string &str)
    {
        nlohmann::json j = nlohmann::json::parse(str);

        TLOG() << "debug : Loading metadata from string " << str;

        if (j.contains("transfer_id"))
        {
            set_group_id(j["transfer_id"].get<std::string>());
        }
        if (j.contains("source_id"))
        {
            set_source_id(j["source_id"].get<std::string>());
        }
        if (j.contains("source_ip"))
        {
            m_source_ip = IPFormat(j["source_ip"].get<std::string>());
        }
        if (j.contains("protocol"))
        {
            set_protocol(magic_enum::enum_cast<e_protocol_type>(j["protocol"].get<std::string>()).value());
        }
        if (j.contains("protocol_options"))
        {
            set_protocol_options(nlohmann::json::parse(j["protocol_options"].get<std::string>()));
        }
        if (j.contains("files"))
        {
            auto files = j["files"].get<std::vector<std::filesystem::path>>();

            for (auto file : files)
            {
                add_expected_file(file);
            }
        }
    }

    void GroupMetadata::generate_metadata_file(std::filesystem::path dest)
    {
        std::ofstream metadata_file;
        metadata_file.open(dest.append(get_group_id()).string() + m_file_extension);

        metadata_file << export_to_string();

        metadata_file.close();
    }

    void GroupMetadata::load_metadata_from_meta_file(std::filesystem::path src)
    {
        std::ifstream metadata_file;
        try
        {
            metadata_file.open(src.string());
        }
        catch (const std::exception &e)
        {
            ers::error(MetadataFileNotFoundError(ERS_HERE, e.what()));
            return;
        }

        std::stringstream buffer;
        buffer << metadata_file.rdbuf();

        from_string(buffer.str());

        metadata_file.close();
    }

    std::string GroupMetadata::to_string() const
    {
        std::string str;
        str += "transfer_id " + get_group_id() + " ";
        str += "protocol " + static_cast<std::string>(magic_enum::enum_name(get_protocol())) + "\n";

        for (const auto &file : get_transfers_meta())
        {
            str += "*file " + file.get_file_name() + "\n";
        }
        for (const auto &file : get_expected_files())
        {
            str += "*expectedfile " + file + "\n";
        }
        return str;
    }
} // namespace dunedaq::snbmodules