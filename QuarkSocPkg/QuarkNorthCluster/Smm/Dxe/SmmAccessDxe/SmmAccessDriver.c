/** @file
This is the driver that publishes the SMM Access Protocol
instance for the Tylersburg chipset.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SmmAccessDriver.h"



SMM_ACCESS_PRIVATE_DATA  mSmmAccess;

VOID
SmmAccessOnBoot (
  IN EFI_EVENT                          Event,
  IN VOID                               *Context
);

EFI_STATUS
EFIAPI
SmmAccessDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  Installs an SMM Access Protocol.

Arguments:

  ImageHandle  -  Handle for the image of this driver.
  SystemTable  -  Pointer to the EFI System Table.

Returns:

  EFI_SUCCESS     -  Protocol successfully started and installed.
  EFI_UNSUPPORTED -  Protocol can't be started.
  EFI_NOT_FOUND   -  Protocol not found.
--*/
{

  EFI_STATUS                      Status;
  EFI_EVENT                       BootEvent;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  UINTN                           Index;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *DescriptorBlock;
  EFI_HOB_GUID_TYPE               *GuidHob;


  //
  // Initialize private data
  //
  ZeroMem (&mSmmAccess, sizeof (mSmmAccess));

  Status = gBS->LocateProtocol (
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  (VOID **) &PciRootBridgeIo
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Build SMM related information
  //
  mSmmAccess.Signature        = SMM_ACCESS_PRIVATE_DATA_SIGNATURE;
  mSmmAccess.Handle           = NULL;
  mSmmAccess.PciRootBridgeIo  = PciRootBridgeIo;

  //
  // Get Hob list
  //
  GuidHob    = GetFirstGuidHob (&gEfiSmmPeiSmramMemoryReserveGuid);
  DescriptorBlock = GET_GUID_HOB_DATA (GuidHob);
  ASSERT (DescriptorBlock);


  //
  // Get CPU Max bus number
  //
  mSmmAccess.MaxBusNumber         = PCI_BUS_NUMBER_QNC;
  for (Index = 0; Index < MAX_CPU_SOCKET; Index++) {
    mSmmAccess.SocketPopulated[Index] = TRUE;
  }

  //
  // Use the hob to publish SMRAM capabilities
  //
  ASSERT (DescriptorBlock->NumberOfSmmReservedRegions <= MAX_SMRAM_RANGES);
  for (Index = 0; Index < DescriptorBlock->NumberOfSmmReservedRegions; Index++) {
    mSmmAccess.SmramDesc[Index].PhysicalStart = DescriptorBlock->Descriptor[Index].PhysicalStart;
    mSmmAccess.SmramDesc[Index].CpuStart      = DescriptorBlock->Descriptor[Index].CpuStart;
    mSmmAccess.SmramDesc[Index].PhysicalSize  = DescriptorBlock->Descriptor[Index].PhysicalSize;
    mSmmAccess.SmramDesc[Index].RegionState   = DescriptorBlock->Descriptor[Index].RegionState;
    DEBUG ((EFI_D_INFO, "SM RAM index[%d] startaddr:%08X Size :%08X\n", Index, mSmmAccess.SmramDesc[Index].CpuStart,
      mSmmAccess.SmramDesc[Index].PhysicalSize));
  }

  mSmmAccess.NumberRegions              = Index;
  mSmmAccess.SmmAccess.Open             = Open;
  mSmmAccess.SmmAccess.Close            = Close;
  mSmmAccess.SmmAccess.Lock             = Lock;
  mSmmAccess.SmmAccess.GetCapabilities  = GetCapabilities;
  mSmmAccess.SmmAccess.LockState        = FALSE;
  mSmmAccess.SmmAccess.OpenState        = FALSE;
  mSmmAccess.SMMRegionState             = EFI_SMRAM_CLOSED;

  //
  // Install our protocol interfaces on the device's handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                &mSmmAccess.Handle,
                &gEfiSmmAccess2ProtocolGuid,
                &mSmmAccess.SmmAccess,
                NULL
                );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_INFO, "SMM  Base: %08X\n", (UINT32)(mSmmAccess.SmramDesc[mSmmAccess.NumberRegions-1].PhysicalStart)));
  DEBUG ((EFI_D_INFO, "SMM  Size: %08X\n", (UINT32)(mSmmAccess.SmramDesc[mSmmAccess.NumberRegions-1].PhysicalSize)));

  mSmmAccess.TsegSize = (UINT8)(mSmmAccess.SmramDesc[mSmmAccess.NumberRegions-1].PhysicalSize);
  //
  // T Seg setting done in QPI RC
  //

  //
  // Prior ReadyToBoot, lock CSEG
  //
  Status = EfiCreateEventReadyToBootEx(
           TPL_NOTIFY,
           SmmAccessOnBoot,
           NULL,
           &BootEvent );
  ASSERT (!EFI_ERROR (Status));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Open (
  IN EFI_SMM_ACCESS2_PROTOCOL    *This
  )
/*++

Routine Description:

  This routine accepts a request to "open" a region of SMRAM.  The
  region could be legacy ABSEG, HSEG, or TSEG near top of physical memory.
  The use of "open" means that the memory is visible from all boot-service
  and SMM agents.

Arguments:

  This             -  Pointer to the SMM Access Interface.
  DescriptorIndex  -  Region of SMRAM to Open.

Returns:

  EFI_SUCCESS            -  The region was successfully opened.
  EFI_DEVICE_ERROR       -  The region could not be opened because locked by
                            chipset.
  EFI_INVALID_PARAMETER  -  The descriptor index was out of bounds.

--*/
{
  SMM_ACCESS_PRIVATE_DATA *SmmAccess;

  SmmAccess = SMM_ACCESS_PRIVATE_DATA_FROM_THIS (This);

  if (mSmmAccess.SMMRegionState & EFI_SMRAM_LOCKED) {
    DEBUG ((EFI_D_ERROR, "Cannot open a locked SMRAM region\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // Open TSEG
  //
  if (!QNCOpenSmramRegion ()) {
    mSmmAccess.SMMRegionState |= EFI_SMRAM_LOCKED;
    return EFI_DEVICE_ERROR;
  }

  mSmmAccess.SMMRegionState &= ~(EFI_SMRAM_CLOSED | EFI_ALLOCATED);
  SyncRegionState2SmramDesc(FALSE, (UINT64)(UINTN)(~(EFI_SMRAM_CLOSED | EFI_ALLOCATED)));
  mSmmAccess.SMMRegionState |= EFI_SMRAM_OPEN;
  SyncRegionState2SmramDesc(TRUE, EFI_SMRAM_OPEN);
  SmmAccess->SmmAccess.OpenState = TRUE;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Close (
  IN EFI_SMM_ACCESS2_PROTOCOL *This
  )
/*++

Routine Description:

  This routine accepts a request to "close" a region of SMRAM.  This is valid for
  compatible SMRAM region.

Arguments:

  This             -  Pointer to the SMM Access Interface.
  DescriptorIndex  -  Region of SMRAM to Close.

Returns:

  EFI_SUCCESS            -  The region was successfully closed.
  EFI_DEVICE_ERROR       -  The region could not be closed because locked by
                            chipset.
  EFI_INVALID_PARAMETER  -  The descriptor index was out of bounds.

--*/
{
  SMM_ACCESS_PRIVATE_DATA *SmmAccess;
  BOOLEAN                 OpenState;
  UINTN                   Index;

  SmmAccess     = SMM_ACCESS_PRIVATE_DATA_FROM_THIS (This);

  if (mSmmAccess.SMMRegionState & EFI_SMRAM_LOCKED) {
    //
    // Cannot close a "locked" region
    //
    DEBUG ((EFI_D_WARN, "Cannot close the locked SMRAM Region\n"));
    return EFI_DEVICE_ERROR;
  }

  if (mSmmAccess.SMMRegionState & EFI_SMRAM_CLOSED) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Close TSEG
  //
  if (!QNCCloseSmramRegion ()) {
    mSmmAccess.SMMRegionState |= EFI_SMRAM_LOCKED;
    return EFI_DEVICE_ERROR;
  }

  mSmmAccess.SMMRegionState &= ~EFI_SMRAM_OPEN;
  SyncRegionState2SmramDesc(FALSE, (UINT64)(UINTN)(~EFI_SMRAM_OPEN));
  mSmmAccess.SMMRegionState |= (EFI_SMRAM_CLOSED | EFI_ALLOCATED);
  SyncRegionState2SmramDesc(TRUE, EFI_SMRAM_CLOSED | EFI_ALLOCATED);

  //
  // Find out if any regions are still open
  //
  OpenState = FALSE;
  for (Index = 0; Index < mSmmAccess.NumberRegions; Index++) {
    if ((SmmAccess->SmramDesc[Index].RegionState & EFI_SMRAM_OPEN) == EFI_SMRAM_OPEN) {
      OpenState = TRUE;
    }
  }

  SmmAccess->SmmAccess.OpenState = OpenState;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Lock (
  IN EFI_SMM_ACCESS2_PROTOCOL   *This
  )
/*++

Routine Description:

  This routine accepts a request to "lock" SMRAM.  The
  region could be legacy AB or TSEG near top of physical memory.
  The use of "lock" means that the memory can no longer be opened
  to BS state..

Arguments:

  This             -  Pointer to the SMM Access Interface.
  DescriptorIndex  -  Region of SMRAM to Lock.

Returns:

  EFI_SUCCESS            -  The region was successfully locked.
  EFI_DEVICE_ERROR       -  The region could not be locked because at least
                            one range is still open.
  EFI_INVALID_PARAMETER  -  The descriptor index was out of bounds.

--*/
{
  SMM_ACCESS_PRIVATE_DATA *SmmAccess;

  SmmAccess = SMM_ACCESS_PRIVATE_DATA_FROM_THIS (This);

  if (SmmAccess->SmmAccess.OpenState) {
    return EFI_DEVICE_ERROR;
  }

  mSmmAccess.SMMRegionState |= EFI_SMRAM_LOCKED;
  SyncRegionState2SmramDesc(TRUE, EFI_SMRAM_LOCKED);
  SmmAccess->SmmAccess.LockState                     = TRUE;

  //
  // Lock TSEG
  //
  QNCLockSmramRegion ();

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetCapabilities (
  IN CONST  EFI_SMM_ACCESS2_PROTOCOL *This,
  IN OUT UINTN                       *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR        *SmramMap
  )
/*++

Routine Description:

  This routine services a user request to discover the SMRAM
  capabilities of this platform.  This will report the possible
  ranges that are possible for SMRAM access, based upon the
  memory controller capabilities.

Arguments:

  This          -  Pointer to the SMRAM Access Interface.
  SmramMapSize  -  Pointer to the variable containing size of the
                   buffer to contain the description information.
  SmramMap      -  Buffer containing the data describing the Smram
                   region descriptors.
Returns:

  EFI_BUFFER_TOO_SMALL  -  The user did not provide a sufficient buffer.
  EFI_SUCCESS           -  The user provided a sufficiently-sized buffer.

--*/
{
  EFI_STATUS                Status;
  SMM_ACCESS_PRIVATE_DATA  *SmmAccess;
  UINTN                     BufferSize;

  SmmAccess           = SMM_ACCESS_PRIVATE_DATA_FROM_THIS (This);
  BufferSize          = SmmAccess->NumberRegions * sizeof (EFI_SMRAM_DESCRIPTOR);

  if (*SmramMapSize < BufferSize) {
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    CopyMem (SmramMap, SmmAccess->SmramDesc, *SmramMapSize);
    Status = EFI_SUCCESS;
  }
  *SmramMapSize = BufferSize;

  return Status;
}

VOID
SmmAccessOnBoot (
  IN EFI_EVENT                          Event,
  IN VOID                               *Context
)
{

}
VOID
SyncRegionState2SmramDesc(
  IN BOOLEAN  OrAnd,
  IN UINT64   Value
  )
{
  UINT32 Index;

  for (Index = 0; Index < mSmmAccess.NumberRegions; Index++) {
    if (OrAnd) {
      mSmmAccess.SmramDesc[Index].RegionState |= Value;
    } else {
      mSmmAccess.SmramDesc[Index].RegionState &= Value;
    }
  }
}
