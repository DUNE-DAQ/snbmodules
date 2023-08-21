/**
 * @file notification_enum.hpp e_notification_type enum definition
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_INCLUDE_SNBMODULES_COMMON_NOTIFICATION_ENUM_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_COMMON_NOTIFICATION_ENUM_HPP_
namespace dunedaq::snbmodules
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
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_COMMON_NOTIFICATION_ENUM_HPP_