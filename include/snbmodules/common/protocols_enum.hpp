
#ifndef SNBMODULES_INCLUDE_SNBMODULES_PROTOCOLENUM_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_PROTOCOLENUM_HPP_
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
#endif // SNBMODULES_INCLUDE_SNBMODULES_PROTOCOLENUM_HPP_