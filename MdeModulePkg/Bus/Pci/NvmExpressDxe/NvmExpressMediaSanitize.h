/** @file
  Header file for MEDIA_SANITIZE_PROTOCOL interface.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NVME_MEDIA_SANITIZE_H_
#define NVME_MEDIA_SANITIZE_H_

#define NVME_NO_DEALLOCATE_AFTER_SANITZE  0x1

/**
  Send NVM Express FormatNVM Admin Command

  The Format NVM command is used to low level format the NVM media. This command is used by
  the host to change the LBA data size and/or metadata size.

  A low level format may destroy all data and metadata associated with all namespaces or only
  the specific namespace associated with the command (refer to the Format NVM Attributes field
  in the Identify Controller data structure).

  After the Format NVM command successfully completes, the controller shall not return any user
  data that was previously contained in an affected namespace.

  @param[in] This            Indicates a pointer to the calling context (Block IO Protocol)
  @param[in] NamespaceId     The NVM Express namespace ID  for which a device path node is to be
                             allocated and built. Caller must set the NamespaceId to zero if the
                             device path node will contain a valid UUID.
  @param[in] Ses             Secure Erase Setting (SES) value
                               - 000b: No secure erase operation requested
                               - 001b: User Data Erase
                               - 010b: Cryptographic Erase
                               - 011b to 111b: Reserved
  @param[in] Flbas           New LBA size (in terms of LBA Format size Index (bits 3:0) in NamespaceData).
                             If this param is 0 (NULL), then use existing LBA size.

  @retval EFI_SUCCESS           The device formatted correctly.
  @retval EFI_WRITE_PROTECTED   The device can not be formatted due to write protection.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the format.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The format request contains parameters that are not valid.

**/
EFI_STATUS
NvmExpressFormatNvm (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 NamespaceId,
  IN UINT32                 Ses,
  IN UINT32                 Flbas
  );

/**
  Send NVM Express Sanitize Admin Command

  The Sanitize command is used to start a sanitize operation or to recover from a previously
  failed sanitize operation. The sanitize operation types that may be supported are Block
  Erase, Crypto Erase, and Overwrite.

  All sanitize operations are processed in the background (i.e., completion of the Sanitize
  command does not indicate completion of the sanitize operation).

  @param[in] This                   Indicates a pointer to the calling context (Block IO Protocol)
  @param[in] NamespaceId            The NVM Express namespace ID  for which a device path node is to be
                                    allocated and built. Caller must set the NamespaceId to zero if the
                                    device path node will contain a valid UUID.
  @param[in] SanitizeAction         Sanitize action
  @param[in] NoDeallocAfterSanitize No deallocate after sanitize option
  @param[in] OverwritePattern       Pattern to overwrite old user data

  @retval EFI_SUCCESS           The media was sanitized successfully on the device.
  @retval EFI_WRITE_PROTECTED   The device can not be sanitized due to write protection.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the sanitize.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not match the current device.
  @retval EFI_INVALID_PARAMETER The sanitize request contains parameters that are not valid.

**/
EFI_STATUS
NvmExpressSanitize (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 NamespaceId,
  IN UINT32                 SanitizeAction,
  IN UINT32                 NoDeallocAfterSanitize,
  IN UINT32                 OverwritePattern
  );

/**
  Clear Media utilizes transport native WRITE commands to write a fixed pattern
  of non-sensitive data to the media.

  NOTE: The caller shall send buffer of one sector/LBA size with overwrite data.
  NOTE: This operation is a blocking call.
  NOTE: This function must be called from TPL_APPLICATION or TPL_CALLBACK.

  Functions are defined to erase and purge data at a block level from mass
  storage devices as well as to manage such devices in the EFI boot services
  environment.

  @param[in] This           Indicates a pointer to the calling context.
  @param[in] MediaId        The media ID that the write request is for.
  @param[in] PassCount      The number of passes to write over media.
  @param[in] SectorOwBuffer A pointer to the overwrite buffer.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
NvmExpressMediaClear (
  IN MEDIA_SANITIZE_PROTOCOL  *This,
  IN UINT32                   MediaId,
  IN UINT32                   PassCount,
  IN VOID                     *SectorOwBuffer
  );

/**
  Purge Media utilizes transport native Sanitize operations. Sanitize specific
  purge actions include: overwrite, block erase, or crypto erase.

  Functions are defined to erase and purge data at a block level from mass
  storage devices as well as to manage such devices in the EFI boot services
  environment. Sanitization refers to a process that renders access to target
  data on the media infeasible for a given level of effort.

  NOTE: This operation is a blocking call.
  NOTE: This function must be called from TPL_APPLICATION or TPL_CALLBACK.

  @param[in] This             Indicates a pointer to the calling context.
  @param[in] MediaId          The media ID that the write request is for.
  @param[in] PurgeAction      The purage action (overwrite, crypto erase, block erase).
  @param[in] OverwritePattern 32-bit pattern to overwrite on media (for overwrite).

  @retval EFI_SUCCESS           The media was purged successfully on the device.
  @retval EFI_WRITE_PROTECTED   The device can not be purged due to write protection.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the purge.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not match the current device.
  @retval EFI_INVALID_PARAMETER The purge request contains parameters that are not valid.

**/
EFI_STATUS
EFIAPI
NvmExpressMediaPurge (
  IN MEDIA_SANITIZE_PROTOCOL  *This,
  IN UINT32                   MediaId,
  IN UINT32                   PurgeAction,
  IN UINT32                   OverwritePattern
  );

/**
  Format Media utilizes native format operations to modify sector/LBA size.
  Secure erase actions are used to define how latent user data is erased.

  NOTE: This function must be called from TPL_APPLICATION or TPL_CALLBACK.

  @param[in] This              Indicates a pointer to the calling context.
  @param[in] MediaId           The media ID that the clear request is for.
  @param[in] LbaSize           Size of LBA (in terms of power of two: 2^n).
  @param[in] SecureEraseAction Secure erase action, if any, to apply to format.
                                 - 000b: No secure erase operation requested
                                 - 001b: User Data Erase
                                 - 010b: Cryptographic Erase
                                 - 011b to 111b: Reserved

  @retval EFI_SUCCESS             The media format request completed successfully on the device.
  @retval EFI_WRITE_PROTECTED     The device can't be formatted due to write protection.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting to perform the format operation.
  @retval EFI_INVALID_PARAMETER   The format request contains parameters that are not valid.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.

 **/
EFI_STATUS
EFIAPI
NvmExpressMediaFormat (
  IN MEDIA_SANITIZE_PROTOCOL  *This,
  IN UINT32                   MediaId,
  IN UINT32                   LbaSize,
  IN UINT32                   SecureEraseAction
  );

#endif
