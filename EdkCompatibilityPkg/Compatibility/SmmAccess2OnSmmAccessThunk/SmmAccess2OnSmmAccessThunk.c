/** @file
  SMM Access2 Protocol on SMM Access Protocol Thunk driver.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SmmAccess2OnSmmAccessThunk.h"

EFI_SMM_ACCESS2_PROTOCOL gSmmAccess2 = {
  SmmAccess2Open,
  SmmAccess2Close,
  SmmAccess2Lock,
  SmmAccess2GetCapabilities,
  FALSE,
  FALSE
};

EFI_SMM_ACCESS_PROTOCOL  *mSmmAccess;
UINTN                     mSmramRegionNumber;

/**
  Opens the SMRAM area to be accessible by a boot-service driver.

  This function "opens" SMRAM so that it is visible while not inside of SMM. The function should 
  return EFI_UNSUPPORTED if the hardware does not support hiding of SMRAM. The function 
  should return EFI_DEVICE_ERROR if the SMRAM configuration is locked.

  @param[in] This           The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_UNSUPPORTED   The system does not support opening and closing of SMRAM.
  @retval EFI_DEVICE_ERROR  SMRAM cannot be opened, perhaps because it is locked.
**/
EFI_STATUS
EFIAPI
SmmAccess2Open (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  EFI_STATUS Status;
  UINTN      DescriptorIndex;

  ///
  /// Open all SMRAM regions via SMM Access Protocol
  ///

  Status = EFI_SUCCESS;
  for (DescriptorIndex = 0; DescriptorIndex < mSmramRegionNumber && !EFI_ERROR (Status); DescriptorIndex++) {
    Status = mSmmAccess->Open (mSmmAccess, DescriptorIndex);
  }
  if (!EFI_ERROR (Status)) {
    gSmmAccess2.OpenState = TRUE;
  }
  return Status;
}

/**
  Inhibits access to the SMRAM.

  This function "closes" SMRAM so that it is not visible while outside of SMM. The function should 
  return EFI_UNSUPPORTED if the hardware does not support hiding of SMRAM.

  @param [in] This           The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_UNSUPPORTED   The system does not support opening and closing of SMRAM.
  @retval EFI_DEVICE_ERROR  SMRAM cannot be closed.
**/
EFI_STATUS
EFIAPI
SmmAccess2Close (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  EFI_STATUS Status;
  UINTN      DescriptorIndex;

  ///
  /// Close all SMRAM regions via SMM Access Protocol
  ///

  Status = EFI_SUCCESS;
  for (DescriptorIndex = 0; DescriptorIndex < mSmramRegionNumber && !EFI_ERROR (Status); DescriptorIndex++) {
    Status = mSmmAccess->Close (mSmmAccess, DescriptorIndex);
  }
  if (!EFI_ERROR (Status)) {
    gSmmAccess2.OpenState = FALSE;
  }
  return Status;
}

/**
  Inhibits access to the SMRAM.

  This function prohibits access to the SMRAM region.  This function is usually implemented such 
  that it is a write-once operation. 

  @param[in] This          The EFI_SMM_ACCESS2_PROTOCOL instance.

  @retval EFI_SUCCESS      The device was successfully locked.
  @retval EFI_UNSUPPORTED  The system does not support locking of SMRAM.
**/
EFI_STATUS
EFIAPI
SmmAccess2Lock (
  IN EFI_SMM_ACCESS2_PROTOCOL  *This
  )
{
  EFI_STATUS Status;
  UINTN      DescriptorIndex;

  ///
  /// Lock all SMRAM regions via SMM Access Protocol
  ///

  Status = EFI_SUCCESS;
  for (DescriptorIndex = 0; DescriptorIndex < mSmramRegionNumber && !EFI_ERROR (Status); DescriptorIndex++) {
    Status = mSmmAccess->Lock (mSmmAccess, DescriptorIndex);
  }
  if (!EFI_ERROR (Status)) {
    gSmmAccess2.LockState = TRUE;
  }
  return Status;
}

/**
  Queries the memory controller for the possible regions that will support SMRAM.

  @param[in]     This           The EFI_SMM_ACCESS2_PROTOCOL instance.
  @param[in, out] SmramMapSize   A pointer to the size, in bytes, of the SmramMemoryMap buffer.
  @param[in, out] SmramMap       A pointer to the buffer in which firmware places the current memory map.

  @retval EFI_SUCCESS           The chipset supported the given resource.
  @retval EFI_BUFFER_TOO_SMALL  The SmramMap parameter was too small.  The current buffer size 
                                needed to hold the memory map is returned in SmramMapSize.
**/
EFI_STATUS
EFIAPI
SmmAccess2GetCapabilities (
  IN CONST EFI_SMM_ACCESS2_PROTOCOL  *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  )
{
  return mSmmAccess->GetCapabilities (mSmmAccess, SmramMapSize, SmramMap);
}

/**
  Entry Point for SMM Access2 On SMM Access Thunk driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The entry point is executed successfully.
  @retval other        Some error occurred when executing this entry point.
**/
EFI_STATUS
EFIAPI
SmmAccess2ThunkMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  UINTN                 SmramMapSize;

  ///
  /// Locate SMM Access Protocol
  ///
  Status = gBS->LocateProtocol (&gEfiSmmAccessProtocolGuid, NULL, (VOID **)&mSmmAccess);
  ASSERT_EFI_ERROR (Status);

  ///
  /// Calculate number of SMRAM regions
  ///
  SmramMapSize = 0;
  Status = mSmmAccess->GetCapabilities (mSmmAccess, &SmramMapSize, NULL);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  mSmramRegionNumber =  SmramMapSize/sizeof (EFI_SMRAM_DESCRIPTOR);
  ASSERT (mSmramRegionNumber > 0);

  ///
  /// Assume all SMRAM regions have consistent OPEN and LOCK states
  ///
  gSmmAccess2.OpenState = mSmmAccess->OpenState;
  gSmmAccess2.LockState = mSmmAccess->LockState;

  ///
  /// Publish PI SMM Access2 Protocol
  ///
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiSmmAccess2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gSmmAccess2
                  );
  return Status;
}

