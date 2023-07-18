
#ifndef SNBMODULES_INCLUDE_SNBMODULES_STATUSENUM_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_STATUSENUM_HPP_
namespace dunedaq::snbmodules
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

        // TODO : specify ?

        CHECKING,
        HASHING,
        UPLOADING,
        DOWNLOADING,
    };
} // namespace dunedaq::snbmodules
#endif // SNBMODULES_INCLUDE_SNBMODULES_STATUSENUM_HPP_