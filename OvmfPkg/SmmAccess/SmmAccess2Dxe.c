/** @file

  A DXE_DRIVER providing SMRAM access by producing EFI_SMM_ACCESS2_PROTOCOL.

  Q35 TSEG is expected to have been verified and set up by the SmmAccessPei
  driver.

  Copyright (C) 2013, 2015, Red Hat, Inc.<BR>
  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SmmAccess2.h>

#include "SmramInternal.h"

/**
  Opens the SMRAM area to be accessible by a boot-service driver.

  This function "opens" SMRAM so that it is visible while not inside of SMM.
  The function should return EFI_UNSUPPORTED if the hardware does not support
  hiding of SMRAM. The function should return EFI_DEVICE_ERROR if the SMRAM
  configuration is locked.

  @param[in] This           The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_UNSUPPORTED   The system does not support opening and closing of
                            SMRAM.
  @retval EFI_DEVICE_ERROR  SMRAM cannot be opened, perhaps because it is
                            locked.
**/
STATIC
EFI_STATUS
EFIAPI
SmmAccess2DxeOpen (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  return SmramAccessOpen (&This->LockState, &This->OpenState);
}

/**
  Inhibits access to the SMRAM.

  This function "closes" SMRAM so that it is not visible while outside of SMM.
  The function should return EFI_UNSUPPORTED if the hardware does not support
  hiding of SMRAM.

  @param[in] This           The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_UNSUPPORTED   The system does not support opening and closing of
                            SMRAM.
  @retval EFI_DEVICE_ERROR  SMRAM cannot be closed.
**/
STATIC
EFI_STATUS
EFIAPI
SmmAccess2DxeClose (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  return SmramAccessClose (&This->LockState, &This->OpenState);
}

/**
  Inhibits access to the SMRAM.

  This function prohibits access to the SMRAM region.  This function is usually
  implemented such that it is a write-once operation.

  @param[in] This          The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS      The device was successfully locked.
  @retval EFI_UNSUPPORTED  The system does not support locking of SMRAM.
**/
STATIC
EFI_STATUS
EFIAPI
SmmAccess2DxeLock (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  return SmramAccessLock (&This->LockState, &This->OpenState);
}

/**
  Queries the memory controller for the possible regions that will support
  SMRAM.

  @param[in]     This           The EFI_SMM_ACCESS2_PROTOCOL instance.
  @param[in,out] SmramMapSize   A pointer to the size, in bytes, of the
                                SmramMemoryMap buffer.
  @param[in,out] SmramMap       A pointer to the buffer in which firmware
                                places the current memory map.

  @retval EFI_SUCCESS           The chipset supported the given resource.
  @retval EFI_BUFFER_TOO_SMALL  The SmramMap parameter was too small.  The
                                current buffer size needed to hold the memory
                                map is returned in SmramMapSize.
**/
STATIC
EFI_STATUS
EFIAPI
SmmAccess2DxeGetCapabilities (
  IN CONST EFI_SMM_ACCESS2_PROTOCOL  *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  )
{
  return SmramAccessGetCapabilities (This->LockState, This->OpenState,
           SmramMapSize, SmramMap);
}

//
// LockState and OpenState will be filled in by the entry point.
//
STATIC EFI_SMM_ACCESS2_PROTOCOL mAccess2 = {
  &SmmAccess2DxeOpen,
  &SmmAccess2DxeClose,
  &SmmAccess2DxeLock,
  &SmmAccess2DxeGetCapabilities
};

//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
SmmAccess2DxeEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  //
  // This module should only be included if SMRAM support is required.
  //
  ASSERT (FeaturePcdGet (PcdSmmSmramRequire));

  InitQ35TsegMbytes ();
  GetStates (&mAccess2.LockState, &mAccess2.OpenState);
  return gBS->InstallMultipleProtocolInterfaces (&ImageHandle,
                &gEfiSmmAccess2ProtocolGuid, &mAccess2,
                NULL);
}
