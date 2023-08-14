#ifndef SNBMODULES_INCLUDE_SNBMODULES_TRANSFER_METADATA_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_TRANSFER_METADATA_HPP_

#include "snbmodules/metadata_abstract.hpp"
#include "snbmodules/common/protocols_enum.hpp"
#include "snbmodules/ip_format.hpp"
#include "snbmodules/common/status_enum.hpp"
#include "snbmodules/tools/natural_sort.hpp"

#include <string>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include <map>

namespace dunedaq::snbmodules
{
    class TransferMetadata : public MetadataAbstract
    {

    public:
        static const std::string m_file_extension;

        /// @brief Export only the modified fields to a string
        /// @param force_all Export everything if true
        /// @return String containing the metadata
        virtual std::string export_to_string_partial(bool force_all = false);

        // Overriden methods
        std::string export_to_string() override { return export_to_string_partial(true); }
        void from_string(std::string) override;

        // Used to export the metadata to a json file
        void generate_metadata_file(std::filesystem::path dest = ".") override;
        void load_metadata_from_meta_file(std::filesystem::path src = ".") override;

        bool operator==(MetadataAbstract const &other) const override
        {
            const TransferMetadata *o = dynamic_cast<const TransferMetadata *>(&other);
            return m_file_path == o->m_file_path && m_src == o->m_src && m_dest == o->m_dest && m_group_id == o->m_group_id;
        }

        /// @brief Operator < overload
        /// @param other
        /// @return
        bool operator<(MetadataAbstract const &other) const override
        {
            const TransferMetadata *o = dynamic_cast<const TransferMetadata *>(&other);
            return SI::natural::compare<std::string>(m_file_path.string(), o->m_file_path.string());
        }

        /// @brief Constructor
        /// @param file_path
        /// @param bytes_size
        /// @param hash
        /// @param src
        /// @param dest
        /// @param bytes_transferred
        /// @param status
        TransferMetadata(std::filesystem::path file_path,
                         uint64_t bytes_size,
                         IPFormat src,
                         std::string hash = "",
                         IPFormat dest = IPFormat(),
                         std::string group_id = "",
                         uint64_t bytes_transferred = 0,
                         e_status status = e_status::WAITING)
            : m_hash(hash),
              m_bytes_size(bytes_size),
              m_bytes_transferred(bytes_transferred),
              m_status(status),
              m_src(src),
              m_dest(dest),
              m_group_id(group_id),
              m_modified_fields({{"hash", true}, {"size", true}, {"bytes_transferred", true}, {"transmission_speed", true}, {"status", true}, {"magnet_link", true}, {"error_code", true}, {"start_time", true}, {"end_time", true}, {"duration", true}})
        {
            // remove all occurences of ./ in the file path
            std::string file_path_str = file_path.string();
            std::string x = "./";

            size_t pos = 0;
            while (1)
            {
                pos = file_path_str.find(x, pos);
                if (pos == std::string::npos)
                {
                    break;
                }

                file_path_str.replace(pos, x.length(), "");
            }

            m_file_path = std::filesystem::absolute(file_path_str);
            if (group_id == "")
            {
                m_group_id = get_file_name();
            }
        }

        /// @brief Load from file constructor
        explicit TransferMetadata(std::filesystem::path src, bool is_path = true)
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

        virtual ~TransferMetadata() {}

        // Setters
        inline void set_file_path(std::filesystem::path file_path)
        {
            // remove all occurences of ./ in the file path
            std::string file_path_str = file_path.string();
            std::string x = "./";

            size_t pos = 0;
            while (1)
            {
                pos = file_path_str.find(x, pos);
                if (pos == std::string::npos)
                {
                    break;
                }

                file_path_str.replace(pos, x.length(), "");
            }
            m_file_path = std::filesystem::absolute(file_path_str);
        }
        inline void set_group_id(std::string group_id) { m_group_id = group_id; }
        void set_src(IPFormat source)
        {
            // Check if the source is not equal to the dest
            if (!(source == m_dest))
            {
                m_src = source;
            }
            else
            {
                throw std::invalid_argument("The source cannot be equal to the destination");
            }
        }

        void set_dest(IPFormat dest)
        {
            // Check if the dest is not equal to the source
            if (!(dest == m_src))
            {
                m_dest = dest;
            }
            else
            {
                throw std::invalid_argument("The destination cannot be equal to the source");
            }
        }

        inline void set_hash(std::string hash)
        {
            m_hash = hash;
            m_modified_fields["hash"] = true;
        }

        inline void set_size(uint64_t size)
        {
            m_bytes_size = size;
            m_modified_fields["size"] = true;
        }
        inline void set_bytes_transferred(uint64_t bytes_transferred)
        {
            m_bytes_transferred = bytes_transferred;
            m_modified_fields["bytes_transferred"] = true;
        }
        void set_progress(int purcent)
        {
            if (purcent < 0 || purcent > 100)
            {
                throw std::invalid_argument("The progress must be between 0 and 100");
            }
            set_bytes_transferred((purcent * m_bytes_size) / 100);
        }
        void set_status(e_status status)
        {
            m_status = status;
            if (status == e_status::DOWNLOADING || status == e_status::UPLOADING)
            {
                m_start_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            }
            if (status == e_status::FINISHED || status == e_status::ERROR)
            {
                m_end_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                m_duration += m_end_time - m_start_time;
            }
            if (status == e_status::PAUSED)
            {
                m_duration += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - m_start_time;
            }

            m_modified_fields["status"] = true;
        }
        inline void set_magnet_link(std::string magnet_link)
        {
            m_magnet_link = magnet_link;
            m_modified_fields["magnet_link"] = true;
        }
        inline void set_error_code(std::string error_code)
        {
            m_error_code = error_code;
            m_modified_fields["error_code"] = true;
        }
        inline void set_transmission_speed(uint64_t transmission_speed)
        {
            m_transmission_speed = transmission_speed;
            m_modified_fields["transmission_speed"] = true;
        }
        inline void set_duration(uint64_t duration)
        {
            m_duration = duration;
            m_modified_fields["duration"] = true;
        }
        inline void set_start_time(uint64_t start_time)
        {
            m_start_time = start_time;
            m_modified_fields["start_time"] = true;
        }
        inline void set_end_time(uint64_t end_time)
        {
            m_end_time = end_time;
            m_modified_fields["end_time"] = true;
        }

        // Getters
        inline std::filesystem::path get_file_path() { return m_file_path; }
        inline std::string get_file_name() { return m_file_path.filename().string(); }
        inline std::string get_hash() { return m_hash; }
        inline IPFormat get_src() { return m_src; }
        inline IPFormat get_dest() { return m_dest; }
        inline uint64_t get_size() { return m_bytes_size; }
        inline uint64_t get_bytes_transferred() { return m_bytes_transferred; }
        inline e_status get_status() { return m_status; }
        inline std::string get_magnet_link() { return m_magnet_link; }
        inline std::string get_group_id() { return m_group_id; }
        inline int get_progress() { return m_bytes_size == 0 ? 0 : m_bytes_transferred * 100 / m_bytes_size; }
        inline std::string get_error_code() { return m_error_code; }
        inline uint64_t get_transmission_speed() { return m_transmission_speed; }
        uint64_t get_total_duration_ms()
        {
            if (m_start_time == 0)
            {
                return 0;
            }
            else if (m_end_time == 0)
            {
                return m_duration + (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - m_start_time);
            }
            else
            {
                return m_end_time - m_start_time;
            }
        }
        inline uint64_t get_start_time() { return m_start_time; }
        inline uint64_t get_end_time() { return m_end_time; }
        std::string get_start_time_str()
        {
            if (m_start_time == 0)
            {
                return "N/A";
            }
            std::time_t t = m_start_time / 1000;
            std::tm tm = *std::localtime(&t);
            std::stringstream ss;
            ss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
            return ss.str();
        }
        std::string get_end_time_str()
        {
            if (m_end_time == 0)
            {
                return "N/A";
            }
            std::time_t t = m_end_time / 1000;
            std::tm tm = *std::localtime(&t);
            std::stringstream ss;
            ss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
            return ss.str();
        }

    private:
        /// @brief Path of the file on the src filesystem
        std::filesystem::path m_file_path = "";

        /// @brief TODO : Hash of the file sha1
        std::string m_hash = "";

        /// @brief Total size of the file in bytes
        uint64_t m_bytes_size = 0;

        /// @brief Number of bytes transferred or received
        uint64_t m_bytes_transferred = 0;

        /// @brief Transmission speed in bytes/s
        uint64_t m_transmission_speed = 0;

        /// @brief Status of the file transfer
        e_status m_status = e_status::WAITING;

        /// @brief Source of the file transfer
        IPFormat m_src = IPFormat();

        /// @brief Destination of the file transfer, not always initialized
        IPFormat m_dest = IPFormat();

        /// @brief Magnet link of the file only for bit torrent
        std::string m_magnet_link = "";

        /// @brief Group id of the file, used to group files together
        std::string m_group_id = "";

        /// @brief show the error exception in case of error
        std::string m_error_code = "";

        /// @brief Start time of the transfer, 0 if not started, reset if resumed
        uint64_t m_start_time = 0;

        /// @brief End time of the transfer, 0 if not finished
        uint64_t m_end_time = 0;

        /// @brief Duration of the transfer
        uint64_t m_duration = 0;

        /// @brief Vector of modified fields in order : to send only the modified fields
        std::map<std::string, bool> m_modified_fields;
    };

} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_TRANSFER_METADATA_HPP_