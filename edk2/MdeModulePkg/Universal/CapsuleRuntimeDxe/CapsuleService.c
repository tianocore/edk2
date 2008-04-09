/** @file
  Capsule Runtime Service.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CapsuleService.h"

EFI_STATUS
EFIAPI
UpdateCapsule (
  IN EFI_CAPSULE_HEADER      **CapsuleHeaderArray,
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
  EFI_DEVICE_ERROR               Failed to SetVariable or ProcessFirmwareVolume.

--*/
{
  UINTN                     ArrayNumber;
  EFI_STATUS                Status;
  EFI_CAPSULE_HEADER        *CapsuleHeader;

  if (CapsuleCount < 1) {
    return EFI_INVALID_PARAMETER;
  }

  CapsuleHeader   = NULL;

  for (ArrayNumber = 0; ArrayNumber < CapsuleCount; ArrayNumber++) {
    //
    // A capsule which has the CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE flag must have
    // CAPSULE_FLAGS_PERSIST_ACROSS_RESET set in its header as well.
    //
    CapsuleHeader = CapsuleHeaderArray[ArrayNumber];
    if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE)) == CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // Check Capsule image without populate flag by firmware support capsule function  
    //
    if (((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) == 0) && 
        (SupportCapsuleImage (CapsuleHeader) != EFI_SUCCESS)) {
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Assume that capsules have the same flags on reseting or not.
  //
  CapsuleHeader = CapsuleHeaderArray[0];

  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) != 0) {
    //
    // Check if the platform supports update capsule across a system reset
    //
    if (!FeaturePcdGet(PcdSupportUpdateCapsuleRest)) {
      return EFI_UNSUPPORTED;
    }
    //
    // ScatterGatherList is only referenced if the capsules are defined to persist across
    // system reset. 
    //
    if (ScatterGatherList == (EFI_PHYSICAL_ADDRESS) (UINTN) NULL) {
      return EFI_INVALID_PARAMETER;
    } else {
      //
      // ScatterGatherList is only referenced if the capsules are defined to persist across
      // system reset. Set its value into NV storage to let pre-boot driver to pick it up 
      // after coming through a system reset.
      //
      Status = EfiSetVariable (
                 EFI_CAPSULE_VARIABLE_NAME,
                 &gEfiCapsuleVendorGuid,
                 EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 sizeof (UINTN),
                 (VOID *) &ScatterGatherList
                 );
      if (Status != EFI_SUCCESS) {
        return Status;
      }
      //
      // Successfully set the capsule image address into variable.
      //
      return EFI_SUCCESS;
    }
  }

  //
  // The rest occurs in the condition of non-reset mode
  // Now Runtime mode doesn't support the non-reset capsule image.
  //
  if (EfiAtRuntime ()) {
    return EFI_UNSUPPORTED;
  }

  //
  // Here should be in the boot-time for non-reset capsule image
  // Default process to Update Capsule image into Flash.
  //
  for (ArrayNumber = 0; ArrayNumber < CapsuleCount ; ArrayNumber++) {
    Status = ProcessCapsuleImage (CapsuleHeaderArray[ArrayNumber]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
QueryCapsuleCapabilities (
  IN  EFI_CAPSULE_HEADER   **CapsuleHeaderArray,
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
  EFI_CAPSULE_HEADER        *CapsuleHeader;

  if (CapsuleCount < 1) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MaxiumCapsuleSize == NULL) ||(ResetType == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  CapsuleHeader = NULL;

  for (ArrayNumber = 0; ArrayNumber < CapsuleCount; ArrayNumber++) {
    CapsuleHeader = CapsuleHeaderArray[ArrayNumber];
    //
    // A capsule which has the CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE flag must have
    // CAPSULE_FLAGS_PERSIST_ACROSS_RESET set in its header as well.
    //
    if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE)) == CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // Check Capsule image without populate flag by firmware support capsule function  
    //
    if (((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) == 0) && 
        (SupportCapsuleImage (CapsuleHeader) != EFI_SUCCESS)) {
      return EFI_UNSUPPORTED;
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


EFI_STATUS
EFIAPI
CapsuleServiceInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  This code is capsule runtime service initialization.

Arguments:

  ImageHandle          The image handle
  SystemTable          The system table.

Returns:

  EFI STATUS

--*/
{
  EFI_STATUS  Status;
  EFI_HANDLE  NewHandle;

  SystemTable->RuntimeServices->UpdateCapsule                    = UpdateCapsule;
  SystemTable->RuntimeServices->QueryCapsuleCapabilities         = QueryCapsuleCapabilities;

  //
  // Now install the Capsule Architectural Protocol on a new handle
  //
  NewHandle = NULL;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &NewHandle,
                  &gEfiCapsuleArchProtocolGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
