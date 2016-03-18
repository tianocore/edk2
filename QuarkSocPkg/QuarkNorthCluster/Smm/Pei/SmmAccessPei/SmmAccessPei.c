/** @file
This is the driver that publishes the SMM Access Ppi
instance for the Quark SOC.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <PiPei.h>
#include <Ppi/SmmAccess.h>
#include <Guid/SmramMemoryReserve.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PciLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/QNCSmmLib.h>
#include <QNCAccess.h>

#define SMM_ACCESS_PRIVATE_DATA_FROM_THIS(a) \
  CR ( \
  a, \
  SMM_ACCESS_PRIVATE_DATA, \
  SmmAccess, \
  SMM_ACCESS_PRIVATE_DATA_SIGNATURE \
  )

#define MAX_CPU_SOCKET      1
#define MAX_SMRAM_RANGES    4

typedef struct {
  UINTN                            Signature;
  EFI_HANDLE                       Handle;
  PEI_SMM_ACCESS_PPI               SmmAccess;
  UINTN                            NumberRegions;
  EFI_SMRAM_DESCRIPTOR             SmramDesc[MAX_SMRAM_RANGES];
  UINT8                            TsegSize;
  UINT8                            MaxBusNumber;
  UINT8                            SocketPopulated[MAX_CPU_SOCKET];
  UINT8                            SocketBusNum[MAX_CPU_SOCKET];
} SMM_ACCESS_PRIVATE_DATA;

#define  SMM_ACCESS_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ('i', 's', 'm', 'a')


EFI_STATUS
EFIAPI
Open (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN PEI_SMM_ACCESS_PPI         *This,
  IN UINTN                      DescriptorIndex
  )
/*++

Routine Description:

  This routine accepts a request to "open" a region of SMRAM.  The
  region could be legacy ABSEG, HSEG, or TSEG near top of physical memory.
  The use of "open" means that the memory is visible from all PEIM
  and SMM agents.

Arguments:

  PeiServices      - General purpose services available to every PEIM.
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

  if (DescriptorIndex >= SmmAccess->NumberRegions) {
    return EFI_INVALID_PARAMETER;
  } else if (SmmAccess->SmramDesc[DescriptorIndex].RegionState & EFI_SMRAM_LOCKED) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Open TSEG
  //
  if (!QNCOpenSmramRegion ()) {
    SmmAccess->SmramDesc[DescriptorIndex].RegionState |= EFI_SMRAM_LOCKED;
    return EFI_DEVICE_ERROR;
  }

  SmmAccess->SmramDesc[DescriptorIndex].RegionState &= ~(EFI_SMRAM_CLOSED | EFI_ALLOCATED);
  SmmAccess->SmramDesc[DescriptorIndex].RegionState |= EFI_SMRAM_OPEN;
  SmmAccess->SmmAccess.OpenState = TRUE;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
Close (
  IN EFI_PEI_SERVICES        **PeiServices,
  IN PEI_SMM_ACCESS_PPI      *This,
  IN UINTN                   DescriptorIndex
  )
/*++

Routine Description:

  This routine accepts a request to "close" a region of SMRAM.  This is valid for
  compatible SMRAM region.

Arguments:

  PeiServices      - General purpose services available to every PEIM.
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

  if (DescriptorIndex >= SmmAccess->NumberRegions) {
    return EFI_INVALID_PARAMETER;
  } else if (SmmAccess->SmramDesc[DescriptorIndex].RegionState & EFI_SMRAM_LOCKED) {
    return EFI_DEVICE_ERROR;
  }

  if (SmmAccess->SmramDesc[DescriptorIndex].RegionState & EFI_SMRAM_CLOSED) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Close TSEG
  //
  if (!QNCCloseSmramRegion ()) {
    SmmAccess->SmramDesc[DescriptorIndex].RegionState |= EFI_SMRAM_LOCKED;
    return EFI_DEVICE_ERROR;
  }

  SmmAccess->SmramDesc[DescriptorIndex].RegionState &= ~EFI_SMRAM_OPEN;
  SmmAccess->SmramDesc[DescriptorIndex].RegionState |= (EFI_SMRAM_CLOSED | EFI_ALLOCATED);

  //
  // Find out if any regions are still open
  //
  OpenState = FALSE;
  for (Index = 0; Index < SmmAccess->NumberRegions; Index++) {
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
  IN EFI_PEI_SERVICES          **PeiServices,
  IN PEI_SMM_ACCESS_PPI        *This,
  IN UINTN                     DescriptorIndex
  )
/*++

Routine Description:

  This routine accepts a request to "lock" SMRAM.  The
  region could be legacy AB or TSEG near top of physical memory.
  The use of "lock" means that the memory can no longer be opened
  to PEIM.

Arguments:

  PeiServices      - General purpose services available to every PEIM.
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

  if (DescriptorIndex >= SmmAccess->NumberRegions) {
    return EFI_INVALID_PARAMETER;
  } else if (SmmAccess->SmmAccess.OpenState) {
    return EFI_DEVICE_ERROR;
  }

  SmmAccess->SmramDesc[DescriptorIndex].RegionState |= EFI_SMRAM_LOCKED;
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
  IN EFI_PEI_SERVICES                **PeiServices,
  IN PEI_SMM_ACCESS_PPI              *This,
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

  PeiServices   - General purpose services available to every PEIM.
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


EFI_STATUS
EFIAPI
SmmAccessPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
/*++

Routine Description:

    This is the constructor for the SMM Access Ppi

Arguments:

    FfsHeader       - FfsHeader.
    PeiServices     - General purpose services available to every PEIM.

Returns:

  EFI_SUCCESS     -  Protocol successfully started and installed.
  EFI_UNSUPPORTED -  Protocol can't be started.
--*/
{

  EFI_STATUS                      Status;
  UINTN                           Index;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *DescriptorBlock;
  SMM_ACCESS_PRIVATE_DATA         *SmmAccessPrivate;
  EFI_PEI_PPI_DESCRIPTOR          *PpiList;
  EFI_HOB_GUID_TYPE               *GuidHob;

  //
  // Initialize private data
  //
  SmmAccessPrivate = AllocatePool (sizeof(*SmmAccessPrivate));
  ASSERT(SmmAccessPrivate);

  PpiList = AllocatePool (sizeof(*PpiList));
  ASSERT (PpiList);

  //
  // Build SMM related information
  //
  SmmAccessPrivate->Signature = SMM_ACCESS_PRIVATE_DATA_SIGNATURE;

  //
  // Get Hob list
  //
  GuidHob    = GetFirstGuidHob (&gEfiSmmPeiSmramMemoryReserveGuid);
  DescriptorBlock = GET_GUID_HOB_DATA (GuidHob);
  ASSERT (DescriptorBlock);

  // Get CPU Max bus number

  SmmAccessPrivate->MaxBusNumber = PCI_BUS_NUMBER_QNC;
  for (Index = 0; Index < MAX_CPU_SOCKET; Index++) {
    SmmAccessPrivate->SocketPopulated[Index] = TRUE;
    SmmAccessPrivate->SocketBusNum[Index]    = PCI_BUS_NUMBER_QNC;
  }

  //
  // Use the hob to publish SMRAM capabilities
  //
  ASSERT (DescriptorBlock->NumberOfSmmReservedRegions <= MAX_SMRAM_RANGES);
  for (Index = 0; Index < DescriptorBlock->NumberOfSmmReservedRegions; Index++) {
    SmmAccessPrivate->SmramDesc[Index].PhysicalStart = DescriptorBlock->Descriptor[Index].PhysicalStart;
    SmmAccessPrivate->SmramDesc[Index].CpuStart      = DescriptorBlock->Descriptor[Index].CpuStart;
    SmmAccessPrivate->SmramDesc[Index].PhysicalSize  = DescriptorBlock->Descriptor[Index].PhysicalSize;
    SmmAccessPrivate->SmramDesc[Index].RegionState   = DescriptorBlock->Descriptor[Index].RegionState;
  }

  SmmAccessPrivate->NumberRegions              = Index;
  SmmAccessPrivate->SmmAccess.Open             = Open;
  SmmAccessPrivate->SmmAccess.Close            = Close;
  SmmAccessPrivate->SmmAccess.Lock             = Lock;
  SmmAccessPrivate->SmmAccess.GetCapabilities  = GetCapabilities;
  SmmAccessPrivate->SmmAccess.LockState        = FALSE;
  SmmAccessPrivate->SmmAccess.OpenState        = FALSE;

  PpiList->Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  PpiList->Guid  = &gPeiSmmAccessPpiGuid;
  PpiList->Ppi   = &SmmAccessPrivate->SmmAccess;

  Status      = (**PeiServices).InstallPpi (PeiServices, PpiList);
  ASSERT_EFI_ERROR(Status);

  DEBUG (
    (EFI_D_INFO, "SMM Base:Size %08X:%08X\n",
    (UINTN)(SmmAccessPrivate->SmramDesc[SmmAccessPrivate->NumberRegions-1].PhysicalStart),
    (UINTN)(SmmAccessPrivate->SmramDesc[SmmAccessPrivate->NumberRegions-1].PhysicalSize)
    ));

  SmmAccessPrivate->TsegSize = (UINT8)(SmmAccessPrivate->SmramDesc[SmmAccessPrivate->NumberRegions-1].PhysicalSize);

  return EFI_SUCCESS;
}

