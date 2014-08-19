/** @file
  Generic Capsule services

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Common/CapsuleName.h>


//
//Max size capsule services support are platform policy,to populate capsules we just need
//memory to maintain them across reset,it is not a problem. And to special capsules ,for
//example,update flash,it is mostly decided by the platform. Here is a sample size for
//different type capsules.
//
#define MAX_SIZE_POPULATE              (0)
#define MAX_SIZE_NON_POPULATE          (0)
#define MAX_SUPPORT_CAPSULE_NUM        0x10


BOOLEAN
EFIAPI
SupportUpdateCapsuleRest (
  VOID
  )
{
  //
  //If the platform has a way to guarantee the memory integrity across a system reset, return
  //TRUE, else FALSE.
  //
  return FALSE;
}



VOID
EFIAPI
SupportCapsuleSize (
  IN OUT UINT32 *MaxSizePopulate,
  IN OUT UINT32 *MaxSizeNonPopulate
  )
{
  //
  //Here is a sample size, different platforms have different sizes.
  //
  *MaxSizePopulate    = MAX_SIZE_POPULATE;
  *MaxSizeNonPopulate = MAX_SIZE_NON_POPULATE;
  return;
}




EFI_STATUS
LibUpdateCapsule (
  IN UEFI_CAPSULE_HEADER     **CapsuleHeaderArray,
  IN UINTN                   CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS    ScatterGatherList OPTIONAL
  )
/*++

Routine Description:

  This code finds if the capsule needs reset to update, if no, update immediately.

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

  if ((CapsuleCount < 1) || (CapsuleCount > MAX_SUPPORT_CAPSULE_NUM)){
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
    if (!SupportUpdateCapsuleRest()) {
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
    Status = gBS->AllocatePool (EfiBootServicesData, CapsuleSize, &BufferPtr);
    if (Status != EFI_SUCCESS) {
      goto Done;
    }
    gBS->CopyMem (BufferPtr, (UINT8*)CapsuleHeader+ CapsuleHeader->HeaderSize, CapsuleSize);

    //
    //Call DXE service ProcessFirmwareVolume to process immediatelly
    //
    Status = gDS->ProcessFirmwareVolume (BufferPtr, CapsuleSize, &FvHandle);
    if (Status != EFI_SUCCESS) {
      gBS->FreePool (BufferPtr);
      return EFI_DEVICE_ERROR;
    }
    gDS->Dispatch ();
    gBS->FreePool (BufferPtr);
  }
  return EFI_SUCCESS;

Done:
  if (BufferPtr != NULL) {
    gBS->FreePool (BufferPtr);
  }
  return EFI_DEVICE_ERROR;
}


EFI_STATUS
QueryCapsuleCapabilities (
  IN  UEFI_CAPSULE_HEADER  **CapsuleHeaderArray,
  IN  UINTN                CapsuleCount,
  OUT UINT64               *MaxiumCapsuleSize,
  OUT EFI_RESET_TYPE       *ResetType
  )
/*++

Routine Description:

  This code is query about capsule capability.

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
  UINT32                    MaxSizePopulate;
  UINT32                    MaxSizeNonPopulate;


  if ((CapsuleCount < 1) || (CapsuleCount > MAX_SUPPORT_CAPSULE_NUM)){
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

  SupportCapsuleSize(&MaxSizePopulate,&MaxSizeNonPopulate);
  //
  //Assume that capsules have the same flags on reseting or not.
  //
  CapsuleHeader = CapsuleHeaderArray[0];
  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) != 0) {
    //
    //Check if the platform supports update capsule across a system reset
    //
    if (!SupportUpdateCapsuleRest()) {
      return EFI_UNSUPPORTED;
    }
    *ResetType = EfiResetWarm;
    *MaxiumCapsuleSize = MaxSizePopulate;
  } else {
    *ResetType = EfiResetCold;
    *MaxiumCapsuleSize = MaxSizeNonPopulate;
  }
  return EFI_SUCCESS;
}


VOID
LibCapsuleVirtualAddressChangeEvent (
  VOID
  )
{
}

VOID
LibCapsuleInitialize (
  VOID
  )
{
}
