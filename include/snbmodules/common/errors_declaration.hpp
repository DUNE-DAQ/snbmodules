/**
 * @file errors_declaration.hpp Error declaration of snbmodules.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef SNBMODULES_INCLUDE_SNBMODULES_COMMON_ERRORS_DECLARATION_HPP_
#define SNBMODULES_INCLUDE_SNBMODULES_COMMON_ERRORS_DECLARATION_HPP_

#include <ers/Issue.hpp>
#include <string>

namespace dunedaq
{
    // common
    ERS_DECLARE_ISSUE(snbmodules,
                      InvalidGroupTransferIDError,
                      "InvalidGroupTransferIDError: Transfer ID " << transfer_id << " not found in " << location,
                      ((std::string)transfer_id)((std::string)location)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      NotificationWrongDestinationError,
                      "NotificationWrongDestinationError: " << location << " Received notification from " << source << " to " << dest,
                      ((std::string)location)((std::string)source)((std::string)dest)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      InvalidNotificationReceivedError,
                      "InvalidNotificationReceivedError: " << location << " Received invalid notification " << notif << " from " << source,
                      ((std::string)location)((std::string)source)((std::string)notif)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      NotHandledNotificationError,
                      "NotHandledNotificationError: " << location << " Received notification " << notif << " from " << source << " but theire is no action for it !",
                      ((std::string)location)((std::string)source)((std::string)notif)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      InvalidProtocolError,
                      "InvalidProtocolError: " << location << " is unable to process protocol or is invalid: " << protocol,
                      ((std::string)location)((std::string)protocol)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      ConnectionNotFoundError,
                      "ConnectionNotFoundError: No matching connection were found containing id " << name,
                      ((std::string)name)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      NotificationSendError,
                      "NotificationSendError: Distant host on connection " << name << " is unreachable",
                      ((std::string)name)) // NOLINT

    // Client and session
    ERS_DECLARE_ISSUE(snbmodules,
                      TransferClientNotInitializedError,
                      "TransferClientNotInitializedError: client started without being properly initialized : " << location,
                      ((std::string)location)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      SessionIDNotFoundInClientError,
                      "SessionIDNotFoundInClientError: " << location << " client didn't found session " << session,
                      ((std::string)location)((std::string)session)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      SessionTypeNotSupportedError,
                      "SessionTypeNotSupportedError: trying to create a session with incorect type Downloader/Uploader for session: " << session,
                      ((std::string)session)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      SessionAccessToIncorrectActionError,
                      "SessionAccessToIncorrectActionError: session " << session << " try to access to " << action << " action but it is not allowed",
                      ((std::string)session)((std::string)action)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      SessionWrongStateTransitionError,
                      "SessionWrongStateTransitionError: " << location << " session try to change transition from " << current_state << " to " << future_state << " for file " << file,
                      ((std::string)location)((std::string)file)((std::string)current_state)((std::string)future_state)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      FileForTransferNotExistError,
                      "FileForTransferNotExistError: " << location << " can't add file " << file,
                      ((std::string)location)((std::string)file)) // NOLINT

    // plugins errors
    ERS_DECLARE_ISSUE(snbmodules,
                      InvalidSourceCommandRequestError,
                      "InvalidSourceCommandRequestError: " << command << " request need to be started by the Uploader only !",
                      ((std::string)command)) // NOLINT

    // protocols interface errors
    ERS_DECLARE_ISSUE(snbmodules,
                      ErrorSCPDownloadError,
                      "ErrorSCPDownloadError: An Error happend while executing the SCP transfer command. The file have not been send. " << error_msg,
                      ((std::string)error_msg)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      RCloneNotSupportError,
                      "RCloneNotSupportError: RClone does not support " << error_msg,
                      ((std::string)error_msg)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      BittorrentPeerDisconnectedError,
                      "BittorrentPeerDisconnectedError: Peer disconnected " << error_msg,
                      ((std::string)error_msg)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      BittorrentError,
                      "BittorrentError: " << error_msg,
                      ((std::string)error_msg)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      BittorrentInvalidMagnetLinkError,
                      "BittorrentInvalidMagnetLinkError: Unable to add magnet link file to bittorent session : " << error_msg,
                      ((std::string)error_msg)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      BittorrentInvalidTorrentFileError,
                      "BittorrentInvalidTorrentFileError: Unable to add torrent file to bittorent session :  " << error_msg,
                      ((std::string)error_msg)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      BittorrentSaveResumeFileError,
                      "BittorrentSaveResumeFileError: Cannot write save metadata files " << error_msg,
                      ((std::string)error_msg)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      BittorrentLoadResumeFileError,
                      "BittorrentLoadResumeFileError: Cannot load resume metadata from file " << file,
                      ((std::string)file)) // NOLINT

    // metadata errors
    ERS_DECLARE_ISSUE(snbmodules,
                      MetadataFileNotFoundError,
                      "MetadataFileNotFoundError: Error opening metadata file : " << err,
                      ((std::string)err)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      MetadataNotFoundInGroupError,
                      "MetadataNotFoundInGroupError: Didn't found metadata object in " << group << " group for specified file path : " << path,
                      ((std::string)group)((std::string)path)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      MetadataNotExpectedInGroupError,
                      "MetadataNotExpectedInGroupError: Didn't expecting adding " << path << " into group " << group,
                      ((std::string)group)((std::string)path)) // NOLINT

    ERS_DECLARE_ISSUE(snbmodules,
                      MetadataReadingWrongKeyValueDataError,
                      "MetadataReadingWrongKeyValueDataError: Reading empty or unknown key/value pair entry for metadata file : k=" << key << " v=" << value,
                      ((std::string)key)((std::string)value)) // NOLINT

} // namespace dunedaq

#endif // SNBMODULES_INCLUDE_SNBMODULES_COMMON_ERRORS_DECLARATION_HPP_