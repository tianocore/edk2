/** @file

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Platform Security Firmware Update for the A-profile Specification 1.0
    (https://developer.arm.com/documentation/den0118/latest)

**/

#ifndef PSA_FWU_LIB_H_
#define PSA_FWU_LIB_H_

#include <IndustryStandard/PsaMmFwUpdate.h>

#define MM_SHARED_BUFFER_SIZE  FixedPcdGet64 (PcdMmBufferSize)

/**
 * Send a FWU discovery request to StMM and get the associated response.
 *
 * @param [out]   Discovery       Discovery data related to firmware update.
 *
 * @retval EFI_SUCCESS
 * @retval Others                 fwu_discovery isn't implemented or
 *                                Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuDiscovery (
  OUT PSA_MM_FWU_DISCOVER_RESP  **Discovery
  );

/**
 * Send a Begin Staging request to StMM and get the associated response thereby
 * start new staging process for firmware update.
 *
 * @param [in]   UpdateGuids        Image type guids to update (partial update).
 *                                  If NULL, try to update all.
 * @param [in]   UpdateCount        Number of Guids in @UpdateGuids.
 * @param [in]   VendorFlags        Vendor Specific flags.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_READY            - Firmware Store in Trial State or
 *                                    Not boot correctly using active bank.
 *                                  - Temporarily couldn't enter staging.
 * @retval EFI_NOT_FOUND            Couldn't find one image or more in @UpdateGuid.
 * @retval Others                   fwu_begin_staging isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuBeginStaging (
  IN CONST EFI_GUID  *UpdateGuids,
  IN UINT32          UpdateCount,
  IN UINT32          VendorFlags
  );

/**
 * Send a FWU Cancel Staging request to StMM and get the associated response
 * thereby cancel staging process for firmware update.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_READY            - Firmware store is not on staging state.
 *                                  - There're still open image handles.
 * @retval EFI_SECURITY_VIOLATION   Some updated image fail to authenticate.
 * @retval EFI_UNSUPPORTED          Update Agent doesn't support partial updates
 *                                  or Client has not updated all the images.
 * @retval Others                   fwu_end_staging isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuEndStaging (
  IN VOID
  );

/**
 * Send a FWU Cancel Staging request to StMM and get the associated response
 * thereby cancel staging process for firmware update.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_READY            Firmware store is not on staging state.
 * @retval Others                   fwu_end_staging isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuCancelStaging (
  IN VOID
  );

/**
 * Send a FWU Open request to StMM and get associated response
 * thereby open image file handle related to @ImageTypeGuid.
 *
 * @param [in]   ImageTypeGuid      Image type guid.
 * @param [in]   OpType             FwuOpStreamRead or FwuOpStreamWrite.
 * @param [out]  Handle             Image File Handle.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND            Couldn't find image with @ImageTypeGuid.
 * @retval EFI_NOT_READY            Try to open with write out of staging state.
 * @retval EFI_UNSUPPORTED          @ImageTypeGuid doesn't support open with @OpType.
 * @retval Others                   fwu_open isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuOpen (
  IN  CONST EFI_GUID  *ImageTypeGuid,
  IN  FWU_OP_TYPE     OpType,
  OUT UINT32          *Handle
  );

/**
 * Send FWU Write Stream request to StMM and get associated response
 * thereby write new firmware image to @Handle.
 *
 * @param [in]   Handle             Image file handle.
 * @param [in]   Buffer             New firmware image data.
 * @param [in]   BufferSize         Size of @Buffer in bytes.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND            Invalid @Handle.
 * @retval EFI_OUT_OF_RESOURCE      @BufferSize is over maximum image size.
 * @retval EFI_ACCESS_DENIED        Image cannot be write to.
 * @retval EFI_NOT_READY            Firmware store isn't staging state.
 * @retval Others                   fwu_write_stream isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuWriteStream (
  IN UINT32      Handle,
  IN CONST VOID  *Buffer,
  IN UINT32      BufferSize
  );

/**
 * Send FWU Read Stream request to StMM and get associated response
 * thereby read firmware image from @Handle.
 *
 * @param [in]   Handle             Image file handle.
 * @param [out]  Buffer             Data Buffer
 * @param [in]   BufferSize         Request bytes to read.
 * @param [out]  ReadyBytes         Real read bytes.
 * @param [out]  TotalBytes         Current image size of @Handle.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND            Invalid @Handle.
 * @retval EFI_OUT_OF_RESOURCE      @BufferSize is over the bytes can be read.
 * @retval EFI_ACCESS_DENIED        Image cannot be read from.
 * @retval EFI_NOT_READY            Image cannot be read temporarily.
 * @retval Others                   fwu_read_stream isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuReadStream (
  IN      UINT32  Handle,
  OUT     VOID    *Buffer,
  IN      UINT32  BufferSize,
  OUT     UINT32  *ReadBytes,
  OUT     UINT32  *TotalBytes
  );

/**
 * Send FWU Commit request to StMM and get associated response
 * thereby close the Handle and commit the image on firmware storage.
 *
 * @param [in]   Handle             Image file handle.
 * @param [in]   AcceptReq          If > 0, Not accept image in commit.
 *                                  remain as unacceptable status.
 *                                  otherwise, try to accept image in commit.
 * @param [in]   MaxAtomicTimeNs    Maximum time (in ns) executing without
 *                                  yielding back to client.
 * @param [out]  Progress           Unit of work already completed.
 * @param [out]  TotalWork          Unit of work must be completed.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND            Invalid @Handle.
 * @retval EFI_SECURITY_VIOLATION   Handle is closed but fail to authenticate.
 * @retval EFI_TIMEOUT              Update procedure is yielded.
 *                                  must call again fwu_commit.
 * @retval EFI_NOT_READY            Image can only be accepted after activation.
 *                                  @AcceptReq should be > 0.
 * @retval Others                   fwu_read_stream isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuCommit (
  IN      UINT32  Handle,
  IN      UINT32  AcceptReq,
  IN      UINT32  MaxAtomicTimeNs,
  OUT     UINT32  *Progress,
  OUT     UINT32  *TotalWork
  );

/**
 * Send FWU Accept Image request to StMM and get associated response
 * thereby accept updated image.
 *
 * Note:
 *  fwu_accept_image only allow when boot correctly (boot with active_index).
 *  That means this call only can accept the image on the active indexed bank.
 *
 * @param [in]   ImageTypeGuid      Image type guid.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_FOUND            Couldn't find image with @ImageTypeGuid
 * @retval EFI_NOT_READY            System boot incorrectly.
 * @retval Others                   fwu_accept_image isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuAcceptImage (
  IN  CONST EFI_GUID  *ImageTypeGuid
  );

/**
 * Send FWU Select Previous request to StMM and get associated response.
 * When Firmware store is in Trial state or System boot incorrectly,
 * Rollback active indexed bank's images using previous indexed ones.
 *
 * @retval EFI_SUCCESS
 * @retval EFI_NOT_READY            - System isn't in Trial state or boot correctly.
 *                                  - Previous indexed bank couldn't be booted.
 * @retval Others                   fwu_accept_image isn't implemented or
 *                                  Error while communicating StMM.
 */
EFI_STATUS
EFIAPI
FwuSelectPrevious (
  IN VOID
  );

/**
  Clear resources allocated by PsaFwuLibInit().
**/
VOID
EFIAPI
PsaFwuLibExit (
  IN VOID
  );

/**
  Platform Flash Access Lib Constructor.
**/
EFI_STATUS
EFIAPI
PsaFwuLibInit (
  IN VOID
  );

/**
 * Convert MmCommunication related addresses on VirtualAddress Change Event.
 *
 * @param [in]   Event      Registered VirtualAddress Change Event.
 * @param [in]   Context    Additional Data.
 *
 */
VOID
EFIAPI
PsaFwuVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

#endif // PSA_FWU_LIB_H_
