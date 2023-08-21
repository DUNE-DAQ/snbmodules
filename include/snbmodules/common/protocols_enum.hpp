/**
 * @file protocols_enum.hpp e_protocol_type enum definition
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_INCLUDE_SNBMODULES_COMMON_PROTOCOLS_ENUM_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_COMMON_PROTOCOLS_ENUM_HPP_
namespace dunedaq::snbmodules
{
    /// @brief Different type of protocols available for communication
    enum e_protocol_type
    {
        BITTORRENT,
        RCLONE,
        SCP,
        dummy,
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_COMMON_PROTOCOLS_ENUM_HPP_