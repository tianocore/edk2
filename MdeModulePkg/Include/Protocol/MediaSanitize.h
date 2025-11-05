/** @file
  This file defines the Media Sanitize Protocol.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MEDIA_SANITIZE_PROTOCOL_H_
#define MEDIA_SANITIZE_PROTOCOL_H_

#define MEDIA_SANITIZE_PROTOCOL_GUID \
  { \
    0x0d799a99, 0x25af, 0x429e, { 0x92, 0x72, 0xd0, 0xb2, 0x7d, 0x6d, 0x5f, 0x14 } \
  }

typedef struct _MEDIA_SANITIZE_PROTOCOL MEDIA_SANITIZE_PROTOCOL;

#define MEDIA_SANITIZE_PROTOCOL_REVISION  0x00010000

///
/// Sanitize actions for purge operation.
///
/// NOTE: First four actions (no action, overwrite, block erase, crypto erase) cannot
/// be overlapped. All other fields may be overlapped as they apply.
///
#define PURGE_ACTION_NO_ACTION                         0x00000000 // No purge action requested
#define PURGE_ACTION_OVERWRITE                         0x00000001 // Overwrite with 32-bit pattern
#define PURGE_ACTION_BLOCK_ERASE                       0x00000002 // Erase Blocks with indeterminate pattern
#define PURGE_ACTION_CRYPTO_ERASE                      0x00000004 // Delete encryption keys only
#define PURGE_ACTION_RESET_REQUIRED                    0x00000008 // Reset required after purge
#define PURGE_ACTION_NO_DEALLOCATE                     0x00000010 // Do no deallocate (trim) flash medai after sanitize
#define PURGE_ACTION_INVERT_OW_PATTERN                 0x00000020 // Invert overwrite pattern between passes
#define PURGE_ACTION_ALLOW_UNRESTRICTED_SANITIZE_EXIT  0x00000040 // Allow exit without restrictions

///
/// Secure erase action for media format operation
///
#define FORMAT_SES_NO_SECURE_ERASE_REQUESTED  0x0 // No secure erase operation requested
#define FORMAT_SES_USER_DATA_ERASE            0x1 // User Data Erase
#define FORMAT_SES_CRYPTOGRAPHIC_ERASE        0x2 // Cryptographic Erase

/**
  Clear Media utilizes transport native WRITE commands to write a fixed pattern
  of non-sensitive data. The size of the overwrite buffer shall be equal to the
  one sector/LBA (in bytes).

  NOTE: This function must be called from TPL aaplication or callback.

  @param[in]       This           Indicates a pointer to the calling context.
  @param[in]       MediaId        The media ID that the clear request is for.
  @param[in]       PassCount      Number of passes to write over the media.
  @param[in]       SectorOwBuffer Pointer to overwrite pattern buffer.

  @retval EFI_SUCCESS             The media clear request completed successfully
                                  on the device.
  @retval EFI_WRITE_PROTECTED     The device can't be cleared due to write
                                  protection.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the clear operation.
  @retval EFI_INVALID_PARAMETER   The clear request contains parameters that
                                  are not valid.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.

**/
typedef
EFI_STATUS
(EFIAPI *BLOCK_MEDIA_CLEAR)(
  IN     MEDIA_SANITIZE_PROTOCOL   *This,
  IN     UINT32                    MediaId,
  IN     UINT32                    PassCount,
  IN     VOID                      *SectorOwBuffer
  );

/**
  Purge Media utilizes native Sanitize operations. Transport specific
  overwrite, block erase, or crypto erase functions shall be invoked based
  on transport.

  NOTE: This function must be called from TPL aaplication or callback.

  @param[in] This             Indicates a pointer to the calling context.
  @param[in] MediaId          The media ID that the clear request is for.
  @param[in] PurgeAction      Purge action: overwrite, crypto or block erase.
  @param[in] OverwritePattern 32-bit pattern to overwrite on media.

  @retval EFI_SUCCESS             The media purge request completed successfully
                                  on the device.
  @retval EFI_WRITE_PROTECTED     The device can't be purged due to write
                                  protection.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the purge operation.
  @retval EFI_INVALID_PARAMETER   The purge request contains parameters that
                                  are not valid.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.

**/
typedef
EFI_STATUS
(EFIAPI *BLOCK_MEDIA_PURGE)(
  IN     MEDIA_SANITIZE_PROTOCOL   *This,
  IN     UINT32                    MediaId,
  IN     UINT32                    PurgeAction,
  IN     UINT32                    OverwritePattern
  );

/**
  Format Media utilizes native format operations to modify sector/LBA size.
  Secure erase actions are used to define how latent user data is erased.

  NOTE: This function must be called from TPL aaplication or callback.

  @param[in] This              Indicates a pointer to the calling context.
  @param[in] MediaId           The media ID that the clear request is for.
  @param[in] LbaSize           Size of LBA (in terms of power of two: 2^n).
  @param[in] SecureEraseAction Secure erase action, if any, to apply to format.

  @retval EFI_SUCCESS             The media format request comopleted
                                  successfully on the device.
  @retval EFI_WRITE_PROTECTED     The device can't be formatted due to write
                                  protection.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the format operation.
  @retval EFI_INVALID_PARAMETER   The format request contains parameters that
                                  are not valid.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.

 **/
typedef
EFI_STATUS
(EFIAPI *BLOCK_MEDIA_FORMAT)(
  IN     MEDIA_SANITIZE_PROTOCOL  *This,
  IN     UINT32                   MediaId,
  IN     UINT32                   LbaSize,
  IN     UINT32                   SecureEraseAction
  );

///
/// The Media Sanitize Protocol provides the ability for a device to expose
/// sanitize functionality. This optional protocol is installed on the same handle
/// as the EFI_BLOCK_IO_PROTOCOL or EFI_BLOCK_IO2_PROTOCOL.
///
struct _MEDIA_SANITIZE_PROTOCOL {
  ///
  /// The revision to which the MEDIA_SANITIZE_PROTOCOL adheres. All future
  /// revisions must be backwards compatible. If a future version is not
  /// backwards compatible, it is not the same GUID.
  ///
  UINT64                Revision;

  ///
  /// A pointer to the EFI_BLOCK_IO_MEDIA data for this device.
  /// Type EFI_BLOCK_IO_MEDIA is defined in BlockIo.h.
  ///
  EFI_BLOCK_IO_MEDIA    *Media;

  ///
  /// SanitizeCapabilities shall which sanitize operations (crypto erase, block
  /// erase, overwrite) is supported by this Block Io device.
  ///
  UINT32                SanitizeCapabilities;

  BLOCK_MEDIA_CLEAR     MediaClear;
  BLOCK_MEDIA_PURGE     MediaPurge;
  BLOCK_MEDIA_FORMAT    MediaFormat;
};

extern EFI_GUID  gMediaSanitizeProtocolGuid;

#endif
