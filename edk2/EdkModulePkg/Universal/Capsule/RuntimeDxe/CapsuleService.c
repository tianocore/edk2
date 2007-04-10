/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  CapsuleService.c

Abstract:

  Capsule Runtime Service.

--*/

#include "CapsuleService.h"

EFI_STATUS
EFIAPI
UpdateCapsule (
  IN UEFI_CAPSULE_HEADER     **CapsuleHeaderArray,
  IN UINTN                   CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS    ScatterGatherList OPTIONAL
  )
/*++

Routine Description:

  This code finds whether the capsules need reset to update, if not, update immediately.

Arguments:

  CapsuleHeaderArray             A array of pointers to capsule headers passed in
  CapsuleCount                   The number of capsule
  ScatterGatherList              Physical address of datablock list points to capsule

Returns:

  EFI STATUS
  EFI_SUCCESS                    Valid capsule was passed.If CAPSULE_FLAG_PERSIT_ACROSS_RESET is
                                 not set, the capsule has been successfully processed by the firmware.
                                 If it set, the ScattlerGatherList is successfully to be set.
  EFI_INVALID_PARAMETER          CapsuleCount is less than 1,CapsuleGuid is not supported.
  EFI_DEVICE_ERROR               Failed to SetVariable or AllocatePool or ProcessFirmwareVolume.

--*/
{
  UINTN                     CapsuleSize;
  UINTN                     ArrayNumber;
  VOID                      *BufferPtr;
  EFI_STATUS                Status;
  EFI_HANDLE                FvHandle;
  UEFI_CAPSULE_HEADER       *CapsuleHeader;

  if (CapsuleCount < 1) {
    return EFI_INVALID_PARAMETER;
  }

  BufferPtr       = NULL;
  CapsuleHeader   = NULL;

  //
  //Compare GUIDs with EFI_CAPSULE_GUID, if capsule header contains CAPSULE_FLAGS_PERSIST_ACROSS_RESET
  //and CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE flags,whatever the GUID is ,the service supports.
  //
  for (ArrayNumber = 0; ArrayNumber < CapsuleCount; ArrayNumber++) {
    CapsuleHeader = CapsuleHeaderArray[ArrayNumber];
    if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE)) == CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) {
      return EFI_INVALID_PARAMETER;
    }
    if (!CompareGuid (&CapsuleHeader->CapsuleGuid, &gEfiCapsuleGuid)) {
      if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) == 0) {
        return EFI_UNSUPPORTED;
      }
    }
  }

  //
  //Assume that capsules have the same flags on reseting or not.
  //
  CapsuleHeader = CapsuleHeaderArray[0];

  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) != 0) {
    //
    //Check if the platform supports update capsule across a system reset
    //
    if (!FeaturePcdGet(PcdSupportUpdateCapsuleRest)) {
      return EFI_UNSUPPORTED;
    }

    if (ScatterGatherList == 0) {
      return EFI_INVALID_PARAMETER;
    } else {
      Status = EfiSetVariable (
                 EFI_CAPSULE_VARIABLE_NAME,
                 &gEfiCapsuleVendorGuid,
                 EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 sizeof (UINTN),
                 (VOID *) &ScatterGatherList
                 );
      if (Status != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }
    }
    return EFI_SUCCESS;
  }

  //
  //The rest occurs in the condition of non-reset mode
  //
  if (EfiAtRuntime ()) {
    return EFI_INVALID_PARAMETER;
  }

  //
  //Here should be in the boot-time
  //
  for (ArrayNumber = 0; ArrayNumber < CapsuleCount ; ArrayNumber++) {
    CapsuleHeader = CapsuleHeaderArray[ArrayNumber];
    CapsuleSize = CapsuleHeader->CapsuleImageSize - CapsuleHeader->HeaderSize;

    BufferPtr = AllocatePool (CapsuleSize);
    if (BufferPtr == NULL) {
      return EFI_DEVICE_ERROR;
    }

    CopyMem (BufferPtr, (UINT8*)CapsuleHeader+ CapsuleHeader->HeaderSize, CapsuleSize);

    //
    //Call DXE service ProcessFirmwareVolume to process immediatelly
    //
    Status = gDS->ProcessFirmwareVolume (BufferPtr, CapsuleSize, &FvHandle);
    if (Status != EFI_SUCCESS) {
      FreePool (BufferPtr);
      return EFI_DEVICE_ERROR;
    }
    gDS->Dispatch ();
    FreePool (BufferPtr);
  }

  return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
QueryCapsuleCapabilities (
  IN  UEFI_CAPSULE_HEADER  **CapsuleHeaderArray,
  IN  UINTN                CapsuleCount,
  OUT UINT64               *MaxiumCapsuleSize,
  OUT EFI_RESET_TYPE       *ResetType
  )
/*++

Routine Description:

  This code is to query about capsule capability.

Arguments:

  CapsuleHeaderArray              A array of pointers to capsule headers passed in
  CapsuleCount                    The number of capsule
  MaxiumCapsuleSize               Max capsule size is supported
  ResetType                       Reset type the capsule indicates, if reset is not needed,return EfiResetCold.
                                  If reset is needed, return EfiResetWarm.

Returns:

  EFI STATUS
  EFI_SUCCESS                     Valid answer returned
  EFI_INVALID_PARAMETER           MaxiumCapsuleSize is NULL,ResetType is NULL.CapsuleCount is less than 1,CapsuleGuid is not supported.
  EFI_UNSUPPORTED                 The capsule type is not supported.

--*/
{
  UINTN                     ArrayNumber;
  UEFI_CAPSULE_HEADER       *CapsuleHeader;

  if (CapsuleCount < 1) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MaxiumCapsuleSize == NULL) ||(ResetType == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  CapsuleHeader = NULL;

  //
  //Compare GUIDs with EFI_CAPSULE_GUID, if capsule header contains CAPSULE_FLAGS_PERSIST_ACROSS_RESET
  //and CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE flags,whatever the GUID is ,the service supports.
  //
  for (ArrayNumber = 0; ArrayNumber < CapsuleCount; ArrayNumber++) {
    CapsuleHeader = CapsuleHeaderArray[ArrayNumber];
    if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE)) == CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) {
      return EFI_INVALID_PARAMETER;
    }
    if (!CompareGuid (&CapsuleHeader->CapsuleGuid, &gEfiCapsuleGuid)) {
      if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) == 0) {
        return EFI_UNSUPPORTED;
      }
    }
  }

  //
  //Assume that capsules have the same flags on reseting or not.
  //
  CapsuleHeader = CapsuleHeaderArray[0];
  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) != 0) {
    //
    //Check if the platform supports update capsule across a system reset
    //
    if (!FeaturePcdGet(PcdSupportUpdateCapsuleRest)) {
      return EFI_UNSUPPORTED;
    }
    *ResetType = EfiResetWarm;
    *MaxiumCapsuleSize = FixedPcdGet32(PcdMaxSizePopulateCapsule);
  } else {
    *ResetType = EfiResetCold;
    *MaxiumCapsuleSize = FixedPcdGet32(PcdMaxSizeNonPopulateCapsule);
  }
  return EFI_SUCCESS;
}

