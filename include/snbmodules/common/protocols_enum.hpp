/**
 * @file protocols_enum.hpp protocol_type::e_protocol_type enum definition
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_INCLUDE_SNBMODULES_COMMON_PROTOCOLS_ENUM_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_COMMON_PROTOCOLS_ENUM_HPP_

#include <map>
#include <string>
#include <optional>

namespace dunedaq::snbmodules
{
    struct protocol_type
    {
        /// @brief Different type of protocols available for communication
        enum e_protocol_type
        {
            BITTORRENT,
            RCLONE,
            SCP,
            dummy,
        };

        static std::string protocols_to_string(e_protocol_type e)
        {
            const std::map<e_protocol_type, std::string> MyEnumStrings{
                {BITTORRENT, "BITTORRENT"},
                {RCLONE, "RCLONE"},
                {SCP, "SCP"},
                {dummy, "dummy"}};
            auto it = MyEnumStrings.find(e);
            return it == MyEnumStrings.end() ? "Not supported" : it->second;
        }

        static std::optional<e_protocol_type> string_to_protocols(std::string s)
        {
            const std::map<std::string, e_protocol_type> MyStringsEnum{
                {"BITTORRENT", BITTORRENT},
                {"RCLONE", RCLONE},
                {"SCP", SCP},
                {"dummy", dummy}};
            auto it = MyStringsEnum.find(s);
            if (it == MyStringsEnum.end())
            {
                return std::nullopt;
            }
            return it->second;
        }
    };

} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_COMMON_PROTOCOLS_ENUM_HPP_