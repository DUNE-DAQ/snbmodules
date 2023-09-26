/**
 * @file status_enum.hpp status_type::e_status enum definition, possible status of a transfer
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_INCLUDE_SNBMODULES_COMMON_STATUS_ENUM_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_COMMON_STATUS_ENUM_HPP_

#include <map>
#include <string>
#include <optional>

namespace dunedaq::snbmodules
{
    struct status_type
    {
        /// @brief Different type of session status
        /// Need to be sorted by priority (highest last)
        enum e_status
        {
            /// @brief waiting for the transfer to start, can be waiting to receive
            /// expected files metadata
            PREPARING, // when the file is not yet available to execute transfer and need more time (ex: torrent file is being generated)
            ERROR,

            SUCCESS_UPLOAD,
            SUCCESS_DOWNLOAD,
            FINISHED,
            CANCELLED,

            PAUSED,
            WAITING,

            // TODO Aug-14-2022 Leo Joly leo.vincent.andre.joly@cern.ch : should we add more status and specify value ?

            CHECKING,
            HASHING,
            UPLOADING,
            DOWNLOADING,
        };

        static std::string status_to_string(e_status e)
        {
            const std::map<e_status, std::string> MyEnumStrings{
                {PREPARING, "PREPARING"},
                {ERROR, "ERROR"},
                {SUCCESS_UPLOAD, "SUCCESS_UPLOAD"},
                {SUCCESS_DOWNLOAD, "SUCCESS_DOWNLOAD"},
                {FINISHED, "FINISHED"},
                {CANCELLED, "CANCELLED"},
                {PAUSED, "PAUSED"},
                {WAITING, "WAITING"},
                {CHECKING, "CHECKING"},
                {HASHING, "HASHING"},
                {UPLOADING, "UPLOADING"},
                {DOWNLOADING, "DOWNLOADING"}};
            auto it = MyEnumStrings.find(e);
            return it == MyEnumStrings.end() ? "Not supported" : it->second;
        }

        static std::optional<e_status> string_to_status(std::string s)
        {
            const std::map<std::string, e_status> MyStringsEnum{
                {"PREPARING", PREPARING},
                {"ERROR", ERROR},
                {"SUCCESS_UPLOAD", SUCCESS_UPLOAD},
                {"SUCCESS_DOWNLOAD", SUCCESS_DOWNLOAD},
                {"FINISHED", FINISHED},
                {"CANCELLED", CANCELLED},
                {"PAUSED", PAUSED},
                {"WAITING", WAITING},
                {"CHECKING", CHECKING},
                {"HASHING", HASHING},
                {"UPLOADING", UPLOADING},
                {"DOWNLOADING", DOWNLOADING}};
            auto it = MyStringsEnum.find(s);
            if (it == MyStringsEnum.end())
            {
                return std::nullopt;
            }
            return it->second;
        }
    };

} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_COMMON_STATUS_ENUM_HPP_