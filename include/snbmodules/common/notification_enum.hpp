/**
 * @file notification_enum.hpp notification_type::e_notification_type enum definition
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_INCLUDE_SNBMODULES_COMMON_NOTIFICATION_ENUM_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_COMMON_NOTIFICATION_ENUM_HPP_

#include <map>
#include <string>
#include <optional>

namespace dunedaq::snbmodules
{
    struct notification_type
    {
        /// @brief Different type of notifications possible to send
        enum e_notification_type
        {
            // bookkeeper
            CONNECTION_REQUEST,
            NEW_TRANSFER,
            UPDATE_REQUEST,

            // client
            GROUP_METADATA,
            TRANSFER_ERROR,

            // both
            START_TRANSFER,
            TRANSFER_METADATA,
            PAUSE_TRANSFER,
            RESUME_TRANSFER,
            CANCEL_TRANSFER,
        };

        static std::string notification_to_string(e_notification_type e)
        {
            const std::map<e_notification_type, std::string> MyEnumStrings{
                {CONNECTION_REQUEST, "CONNECTION_REQUEST"},
                {NEW_TRANSFER, "NEW_TRANSFER"},
                {UPDATE_REQUEST, "UPDATE_REQUEST"},
                {GROUP_METADATA, "GROUP_METADATA"},
                {TRANSFER_ERROR, "TRANSFER_ERROR"},
                {START_TRANSFER, "START_TRANSFER"},
                {TRANSFER_METADATA, "TRANSFER_METADATA"},
                {PAUSE_TRANSFER, "PAUSE_TRANSFER"},
                {RESUME_TRANSFER, "RESUME_TRANSFER"},
                {CANCEL_TRANSFER, "CANCEL_TRANSFER"}};
            auto it = MyEnumStrings.find(e);
            return it == MyEnumStrings.end() ? "Not supported" : it->second;
        }

        static std::optional<e_notification_type> string_to_notification(std::string s)
        {
            const std::map<std::string, e_notification_type> MyStringsEnum{
                {"CONNECTION_REQUEST", CONNECTION_REQUEST},
                {"NEW_TRANSFER", NEW_TRANSFER},
                {"UPDATE_REQUEST", UPDATE_REQUEST},
                {"GROUP_METADATA", GROUP_METADATA},
                {"TRANSFER_ERROR", TRANSFER_ERROR},
                {"START_TRANSFER", START_TRANSFER},
                {"TRANSFER_METADATA", TRANSFER_METADATA},
                {"PAUSE_TRANSFER", PAUSE_TRANSFER},
                {"RESUME_TRANSFER", RESUME_TRANSFER},
                {"CANCEL_TRANSFER", CANCEL_TRANSFER}};
            auto it = MyStringsEnum.find(s);
            if (it == MyStringsEnum.end())
            {
                return std::nullopt;
            }
            return it->second;
        }
    };

} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_COMMON_NOTIFICATION_ENUM_HPP_