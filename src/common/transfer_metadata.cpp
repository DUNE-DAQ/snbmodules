#include "snbmodules/transfer_metadata.hpp"
#include "snbmodules/tools/magic_enum.hpp"

namespace dunedaq::snbmodules
{
    const std::string TransferMetadata::m_file_extension = ".tmetadata";

    std::string TransferMetadata::export_to_string_partial(bool force_all)
    {
        nlohmann::json j;
        j["file_path"] = get_file_path().string();
        j["source"] = get_src().get_ip_port();
        j["destination"] = get_dest().get_ip_port();
        j["group_id"] = get_group_id();

        // not mandatory : to be refreshed when needed
        if (force_all || m_modified_fields["hash"] == true)
            j["hash"] = get_hash();
        if (force_all || m_modified_fields["size"] == true)
            j["size"] = get_size();
        if (force_all || m_modified_fields["bytes_transferred"] == true)
            j["transfered"] = get_bytes_transferred();
        if (force_all || m_modified_fields["transmission_speed"] == true)
            j["speed"] = get_transmission_speed();
        if (force_all || m_modified_fields["status"] == true)
            j["status"] = static_cast<std::string>(magic_enum::enum_name(get_status()));
        if (force_all || m_modified_fields["magnet_link"] == true)
            j["magnet_link"] = get_magnet_link();
        if (force_all || m_modified_fields["error_code"] == true)
            j["error_code"] = get_error_code();
        if (force_all || m_modified_fields["start_time"] == true)
            j["start_t"] = get_start_time();
        if (force_all || m_modified_fields["end_time"] == true)
            j["end_t"] = get_end_time();
        if (force_all || m_modified_fields["duration"] == true)
            j["duration"] = m_duration;

        m_modified_fields.clear();

        return j.dump();
    }

    void TransferMetadata::from_string(std::string str)
    {
        nlohmann::json j = nlohmann::json::parse(str);

        TLOG() << "debug : Loading metadata from string " << str;

        if (j.contains("file_path"))
        {
            set_file_path(j["file_path"].get<std::filesystem::path>());
        }
        if (j.contains("hash"))
        {
            set_hash(j["hash"].get<std::string>());
        }
        if (j.contains("size"))
        {
            set_size(j["size"].get<unsigned long>());
        }
        if (j.contains("transfered"))
        {
            set_bytes_transferred(j["transfered"].get<unsigned long>());
        }
        if (j.contains("speed"))
        {
            set_transmission_speed(j["speed"].get<unsigned long>());
        }
        if (j.contains("source"))
        {
            set_src(IPFormat(j["source"].get<std::string>()));
        }
        if (j.contains("destination"))
        {
            set_dest(IPFormat(j["destination"].get<std::string>()));
        }
        if (j.contains("status"))
        {
            auto status = magic_enum::enum_cast<e_status>(j["status"].get<std::string>());
            if (status.has_value())
            {
                set_status(status.value());
            }
            else
            {
                ers::error(InvalidProtocolError(ERS_HERE, "reading metadata string process", j["status"].get<std::string>()));
            }
        }
        if (j.contains("magnet_link"))
        {
            set_magnet_link(j["magnet_link"].get<std::string>());
        }
        if (j.contains("group_id"))
        {
            set_group_id(j["group_id"].get<std::string>());
        }
        if (j.contains("error_code"))
        {
            set_error_code(j["error_code"].get<std::string>());
        }
        if (j.contains("start_t"))
        {
            set_start_time(j["start_t"].get<uint64_t>());
        }
        if (j.contains("end_t"))
        {
            set_end_time(j["end_t"].get<uint64_t>());
        }
        if (j.contains("duration"))
        {
            set_duration(j["duration"].get<uint64_t>());
        }
    }

    void TransferMetadata::generate_metadata_file(std::filesystem::path dest)
    {
        std::ofstream metadata_file;
        metadata_file.open(dest.append(get_file_name()).string() + TransferMetadata::m_file_extension);

        metadata_file << export_to_string();

        metadata_file.close();
    }

    void TransferMetadata::load_metadata_from_meta_file(std::filesystem::path src)
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
} // namespace dunedaq::snbmodules