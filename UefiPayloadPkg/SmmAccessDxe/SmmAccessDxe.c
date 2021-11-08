/** @file
  This driver publishes the SMM Access 2 Protocol.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmmAccessDxe.h"

SMM_ACCESS_PRIVATE_DATA         mSmmAccess;

/**
   Update region state from SMRAM description

   @param[in] OrLogic     Indicate to use OR if true or AND if false.
   @param[in] Value       The value to set to region state based on OrLogic.

**/
VOID
SyncRegionState2SmramDesc(
  IN BOOLEAN                OrLogic,
  IN UINT64                 Value
  )
{
  UINT32                    Index;

  for (Index = 0; Index < mSmmAccess.NumberRegions; Index++) {
    if (OrLogic) {
      mSmmAccess.SmramDesc[Index].RegionState |= Value;
    } else {
      mSmmAccess.SmramDesc[Index].RegionState &= Value;
    }
  }
}

/**
   This routine accepts a request to "open" a region of SMRAM.  The
   region could be legacy ABSEG, HSEG, or TSEG near top of physical memory.
   The use of "open" means that the memory is visible from all boot-service
   and SMM agents.

   @param This                    Pointer to the SMM Access Interface.

   @retval EFI_SUCCESS            The region was successfully opened.
   @retval EFI_DEVICE_ERROR       The region could not be opened because locked by chipset.
   @retval EFI_INVALID_PARAMETER  The descriptor index was out of bounds.

**/
EFI_STATUS
EFIAPI
Open (
  IN EFI_SMM_ACCESS2_PROTOCOL          *This
  )
{
  if ((mSmmAccess.SmmRegionState & EFI_SMRAM_LOCKED) != 0) {
    //
    // Cannot open a "locked" region
    //
    DEBUG ((DEBUG_INFO, "Cannot open the locked SMRAM Region\n"));
    return EFI_DEVICE_ERROR;
  }

  mSmmAccess.SmmRegionState &= ~(EFI_SMRAM_CLOSED | EFI_ALLOCATED);
  SyncRegionState2SmramDesc(FALSE, (UINT64)(UINTN)(~(EFI_SMRAM_CLOSED | EFI_ALLOCATED)));

  mSmmAccess.SmmRegionState |= EFI_SMRAM_OPEN;
  SyncRegionState2SmramDesc(TRUE, EFI_SMRAM_OPEN);
  mSmmAccess.SmmAccess.OpenState = TRUE;

  return EFI_SUCCESS;
}

/**
   This routine accepts a request to "close" a region of SMRAM. The region
   could be legacy AB or TSEG near top of physical memory.
   The use of "close" means that the memory is only visible from SMM agents,
   not from BS or RT code.

   @param This                      Pointer to the SMM Access Interface.

   @retval EFI_SUCCESS              The region was successfully closed.
   @retval EFI_DEVICE_ERROR         The region could not be closed because locked by
                                    chipset.
   @retval EFI_INVALID_PARAMETER    The descriptor index was out of bounds.

**/
EFI_STATUS
EFIAPI
Close (
  IN EFI_SMM_ACCESS2_PROTOCOL    *This
  )
{
  if ((mSmmAccess.SmmRegionState & EFI_SMRAM_LOCKED) != 0) {
    //
    // Cannot close a "locked" region
    //
    DEBUG ((DEBUG_INFO, "Cannot close the locked SMRAM Region\n"));
    return EFI_DEVICE_ERROR;
  }

  if ((mSmmAccess.SmmRegionState & EFI_SMRAM_CLOSED) != 0) {
    return EFI_DEVICE_ERROR;
  }

  mSmmAccess.SmmRegionState &= ~EFI_SMRAM_OPEN;
  SyncRegionState2SmramDesc(FALSE, (UINT64)(UINTN)(~EFI_SMRAM_OPEN));

  mSmmAccess.SmmRegionState |= (EFI_SMRAM_CLOSED | EFI_ALLOCATED);
  SyncRegionState2SmramDesc(TRUE, EFI_SMRAM_CLOSED | EFI_ALLOCATED);

  mSmmAccess.SmmAccess.OpenState = FALSE;

  return EFI_SUCCESS;
}

/**
   This routine accepts a request to "lock" SMRAM.  The
   region could be legacy AB or TSEG near top of physical memory.
   The use of "lock" means that the memory can no longer be opened
   to BS state.

   @param This                     Pointer to the SMM Access Interface.

   @retval EFI_SUCCESS             The region was successfully locked.
   @retval EFI_DEVICE_ERROR        The region could not be locked because at least
                                   one range is still open.
   @retval EFI_INVALID_PARAMETER   The descriptor index was out of bounds.

**/
EFI_STATUS
EFIAPI
Lock (
  IN EFI_SMM_ACCESS2_PROTOCOL    *This
  )
{
  if (mSmmAccess.SmmAccess.OpenState) {
    DEBUG ((DEBUG_INFO, "Cannot lock SMRAM when it is still open\n"));
    return EFI_DEVICE_ERROR;
  }

  mSmmAccess.SmmRegionState |= EFI_SMRAM_LOCKED;
  SyncRegionState2SmramDesc(TRUE, EFI_SMRAM_LOCKED);
  mSmmAccess.SmmAccess.LockState = TRUE;
  return EFI_SUCCESS;
}

/**
   This routine services a user request to discover the SMRAM
   capabilities of this platform.  This will report the possible
   ranges that are possible for SMRAM access, based upon the
   memory controller capabilities.

   @param This            Pointer to the SMRAM Access Interface.
   @param SmramMapSize    Pointer to the variable containing size of the
                          buffer to contain the description information.
   @param SmramMap        Buffer containing the data describing the Smram
                          region descriptors.

   @retval EFI_BUFFER_TOO_SMALL  The user did not provide a sufficient buffer.
   @retval EFI_SUCCESS           The user provided a sufficiently-sized buffer.

**/
EFI_STATUS
EFIAPI
GetCapabilities (
  IN CONST EFI_SMM_ACCESS2_PROTOCOL     *This,
  IN OUT   UINTN                        *SmramMapSize,
  IN OUT   EFI_SMRAM_DESCRIPTOR         *SmramMap
  )
{
  EFI_STATUS                            Status;
  UINTN                                 NecessaryBufferSize;

  NecessaryBufferSize = mSmmAccess.NumberRegions * sizeof(EFI_SMRAM_DESCRIPTOR);
  if (*SmramMapSize < NecessaryBufferSize) {
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    CopyMem(SmramMap, mSmmAccess.SmramDesc, NecessaryBufferSize);
    Status = EFI_SUCCESS;
  }

  *SmramMapSize = NecessaryBufferSize;
  return Status;
}

/**
  This function installs EFI_SMM_ACCESS_PROTOCOL.

  @param  ImageHandle Handle for the image of this driver
  @param  SystemTable Pointer to the EFI System Table

  @retval EFI_UNSUPPORTED There's no Intel ICH on this platform
  @return The status returned from InstallProtocolInterface().

**/
EFI_STATUS
EFIAPI
SmmAccessEntryPoint (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_HOB_GUID_TYPE               *GuidHob;
  UINT32                          SmmRegionNum;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *SmramHob;
  UINT32                          Index;

  //
  // Get SMRAM info HOB
  //
  GuidHob = GetFirstGuidHob (&gEfiSmmSmramMemoryGuid);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_INFO, "SMRAM HOB NOT found\n"));
    return EFI_NOT_FOUND;
  }
  SmramHob     = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *) GET_GUID_HOB_DATA(GuidHob);
  SmmRegionNum = SmramHob->NumberOfSmmReservedRegions;
  mSmmAccess.SmramDesc = AllocateZeroPool (sizeof (EFI_SMRAM_DESCRIPTOR) * SmmRegionNum);
  if (mSmmAccess.SmramDesc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (mSmmAccess.SmramDesc, &SmramHob->Descriptor, sizeof (EFI_SMRAM_DESCRIPTOR) * SmmRegionNum);

  DEBUG ((DEBUG_INFO, "NumberOfSmmReservedRegions = 0x%x\n", SmmRegionNum));
  for (Index = 0; Index < SmmRegionNum; Index++) {
    DEBUG ((DEBUG_INFO, "%d: base=0x%x, size = 0x%x, State=0x%x\n",Index,
       SmramHob->Descriptor[Index].PhysicalStart,
       SmramHob->Descriptor[Index].PhysicalSize,
       SmramHob->Descriptor[Index].RegionState));
     mSmmAccess.SmramDesc[Index].RegionState &= EFI_ALLOCATED;
     mSmmAccess.SmramDesc[Index].RegionState |= EFI_SMRAM_CLOSED | EFI_CACHEABLE;
  }

  mSmmAccess.Signature                    = SMM_ACCESS_PRIVATE_DATA_SIGNATURE;
  mSmmAccess.NumberRegions                = SmmRegionNum;
  mSmmAccess.SmmAccess.Open               = Open;
  mSmmAccess.SmmAccess.Close              = Close;
  mSmmAccess.SmmAccess.Lock               = Lock;
  mSmmAccess.SmmAccess.GetCapabilities    = GetCapabilities;
  mSmmAccess.SmmAccess.LockState          = FALSE;
  mSmmAccess.SmmAccess.OpenState          = FALSE;
  mSmmAccess.SmmRegionState               = EFI_SMRAM_CLOSED;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSmmAccess.Handle,
                  &gEfiSmmAccess2ProtocolGuid,
                  &mSmmAccess.SmmAccess,
                  NULL
                  );

  return Status;
}
