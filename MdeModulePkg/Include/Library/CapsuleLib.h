/** @file

  This library class defines a set of interfaces for how to process capsule image updates.

Copyright (c) 2007 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Guid/FileInfo.h>

//
// BOOLEAN Variable to indicate whether system is in the capsule on disk state.
//
#define COD_RELOCATION_INFO_VAR_NAME  L"CodRelocationInfo"

typedef struct {
  //
  // image address.
  //
  VOID             *ImageAddress;
  //
  // The file info of the image comes from.
  // if FileInfo == NULL, image does not come from file.
  //
  EFI_FILE_INFO    *FileInfo;
} IMAGE_INFO;

/**
  The firmware checks whether the capsule image is supported
  by the CapsuleGuid in CapsuleHeader or if there is other specific information in
  the capsule image.

  Caution: This function may receive untrusted input.

  @param  CapsuleHeader    Pointer to the UEFI capsule image to be checked.

  @retval EFI_SUCCESS      Input capsule is supported by firmware.
  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
SupportCapsuleImage (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  );

/**
  The firmware-specific implementation processes the capsule image
  if it recognized the format of this capsule image.

  Caution: This function may receive untrusted input.

  @param  CapsuleHeader    Pointer to the UEFI capsule image to be processed.

  @retval EFI_SUCCESS      Capsule Image processed successfully.
  @retval EFI_UNSUPPORTED  Capsule image is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
ProcessCapsuleImage (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  );

/**

  This routine is called to process capsules.

  Caution: This function may receive untrusted input.

  The capsules reported in EFI_HOB_UEFI_CAPSULE are processed.
  If there is no EFI_HOB_UEFI_CAPSULE, this routine does nothing.

  This routine should be called twice in BDS.
  1) The first call must be before EndOfDxe. The system capsules is processed.
     If device capsule FMP protocols are exposted at this time and device FMP
     capsule has zero EmbeddedDriverCount, the device capsules are processed.
     Each individual capsule result is recorded in capsule record variable.
     System may reset in this function, if reset is required by capsule and
     all capsules are processed.
     If not all capsules are processed, reset will be defered to second call.

  2) The second call must be after EndOfDxe and after ConnectAll, so that all
     device capsule FMP protocols are exposed.
     The system capsules are skipped. If the device capsules are NOT processed
     in first call, they are processed here.
     Each individual capsule result is recorded in capsule record variable.
     System may reset in this function, if reset is required by capsule
     processed in first call and second call.

  @retval EFI_SUCCESS             There is no error when processing capsules.
  @retval EFI_OUT_OF_RESOURCES    No enough resource to process capsules.

**/
EFI_STATUS
EFIAPI
ProcessCapsules (
  VOID
  );

/**
  This routine is called to check if CapsuleOnDisk flag in OsIndications Variable
  is enabled.

  @retval TRUE     Flag is enabled
  @retval FALSE    Flag is not enabled

**/
BOOLEAN
EFIAPI
CoDCheckCapsuleOnDiskFlag (
  VOID
  );

/**
  Check if any on-disk capsules are present.

  Connecting the boot device can discover option ROMs. The caller must provide
  a suitable image-dispatch policy before setting ConnectBootDevice to TRUE.

  @param[in]  MaxRetry          Max Connection Retry. Stall 100ms between each
                                connection try to ensure devices like USB can
                                get enumerated.
  @param[in]  ConnectBootDevice Connect the active boot device before expanding
                                its media path.
  @param[out] BootDeviceReady   Set to TRUE when the active boot option's
                                filesystem is available. Optional.
  @param[out] ConnectAllNeeded  Set to TRUE when an active media-only boot path
                                needs global device connection. Optional.

  @retval TRUE   At least one potential on-disk capsule was found on a boot
                 drive.
  @retval FALSE  No capsule candidates were discovered on a boot drive.
**/
BOOLEAN
EFIAPI
CoDPresent (
  IN  UINTN    MaxRetry,
  IN  BOOLEAN  ConnectBootDevice,
  OUT BOOLEAN  *BootDeviceReady OPTIONAL,
  OUT BOOLEAN  *ConnectAllNeeded OPTIONAL
  );

/**
  This routine is called to get all capsules from file. The capsule file image is
  copied to BS memory. Caller is responsible to free them.

  @param[in]    MaxRetry             Max Connection Retry. Stall 100ms between each connection try to ensure
                                     devices like USB can get enumerated.
  @param[out]   CapsulePtr           Copied Capsule file Image Info buffer
  @param[out]   CapsuleNum           CapsuleNumber
  @param[out]   FsHandle             File system handle
  @param[out]   LoadOptionNumber     OptionNumber of boot option

  @retval EFI_SUCCESS              Succeed to get all capsules.
  @retval EFI_WARN_DELETE_FAILURE  Valid capsules were loaded, but at least one source file could not be removed.

**/
EFI_STATUS
EFIAPI
CoDGetAll (
  IN  UINTN       MaxRetry,
  OUT IMAGE_INFO  **CapsulePtr,
  OUT UINTN       *CapsuleNum,
  OUT EFI_HANDLE  *FsHandle,
  OUT UINT16      *LoadOptionNumber
  );

/**
  Free resources allocated by CoDGetAll.

  @param[in]  ImageInfo  An array of data and information of files
  @param[in]  Count      Length of the array.

**/
VOID
EFIAPI
CoDFreeImages (
  IN IMAGE_INFO  *ImageInfo,
  IN UINTN       Count
  );

/**
  This routine is called to clear CapsuleOnDisk flags including OsIndications and BootNext variable.

  @retval EFI_SUCCESS   All Capsule On Disk flags are cleared

**/
EFI_STATUS
EFIAPI
CoDClearCapsuleOnDiskFlag (
  VOID
  );

/**
  Relocate Capsule on Disk from EFI system partition.

  Two solution to deliver Capsule On Disk:
  Solution A: If PcdCapsuleInRamSupport is enabled, relocate Capsule On Disk to memory and call UpdateCapsule().
  Solution B: If PcdCapsuleInRamSupport is disabled, relocate Capsule On Disk to a platform-specific NV storage
  device with BlockIo protocol.

  Device enumeration like USB costs time, user can input MaxRetry to tell function to retry.
  Function will stall 100ms between each retry.

  Side Effects:
    Capsule Delivery Supported Flag in OsIndication variable and BootNext variable will be cleared.
    Solution B: Content corruption. Block IO write directly touches low level write. Orignal partitions, file
  systems of the relocation device will be corrupted.

  @param[in]    MaxRetry             Max Connection Retry. Stall 100ms between each connection try to ensure
                                     devices like USB can get enumerated. Input 0 means no retry.

  @retval EFI_SUCCESS   Capsule on Disk images are successfully relocated.

**/
EFI_STATUS
EFIAPI
CoDRelocateCapsule (
  UINTN  MaxRetry
  );

/**
  Remove the temp file from the root of EFI System Partition.
  Device enumeration like USB costs time, user can input MaxRetry to tell function to retry.
  Function will stall 100ms between each retry.

  @param[in]    MaxRetry             Max Connection Retry. Stall 100ms between each connection try to ensure
                                     devices like USB can get enumerated. Input 0 means no retry.

  @retval EFI_SUCCESS   Remove the temp file successfully.

**/
EFI_STATUS
EFIAPI
CoDRemoveTempFile (
  UINTN  MaxRetry
  );
