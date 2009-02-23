/** @file
  Capsule Runtime Drivers produces two UEFI capsule runtime services.
  (UpdateCapsule, QueryCapsuleCapabilities)
  It installs the Capsule Architectural Protocol (EDKII definition) to signify 
  the capsule runtime services are ready.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Protocol/Capsule.h>
#include <Guid/CapsuleVendor.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/CapsuleLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

/**
  Passes capsules to the firmware with both virtual and physical mapping. Depending on the intended
  consumption, the firmware may process the capsule immediately. If the payload should persist
  across a system reset, the reset value returned from EFI_QueryCapsuleCapabilities must
  be passed into ResetSystem() and will cause the capsule to be processed by the firmware as
  part of the reset process.

  @param  CapsuleHeaderArray    Virtual pointer to an array of virtual pointers to the capsules
                                being passed into update capsule.
  @param  CapsuleCount          Number of pointers to EFI_CAPSULE_HEADER in
                                CaspuleHeaderArray.
  @param  ScatterGatherList     Physical pointer to a set of
                                EFI_CAPSULE_BLOCK_DESCRIPTOR that describes the
                                location in physical memory of a set of capsules.

  @retval EFI_SUCCESS           Valid capsule was passed. If
                                CAPSULE_FLAGS_PERSIT_ACROSS_RESET is not set, the
                                capsule has been successfully processed by the firmware.
  @retval EFI_DEVICE_ERROR      The capsule update was started, but failed due to a device error.
  @retval EFI_INVALID_PARAMETER CapsuleCount is Zero, or CapsuleImage is not valid.
                                For across reset capsule image, ScatterGatherList is NULL.
  @retval EFI_UNSUPPORTED       CapsuleImage is not recognized by the firmware.

**/
EFI_STATUS
EFIAPI
UpdateCapsule (
  IN EFI_CAPSULE_HEADER      **CapsuleHeaderArray,
  IN UINTN                   CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS    ScatterGatherList OPTIONAL
  )
{
  UINTN                     ArrayNumber;
  EFI_STATUS                Status;
  EFI_CAPSULE_HEADER        *CapsuleHeader;
  
  //
  // Capsule Count can't be less than one.
  //
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
  
  //
  //  Process across reset capsule image.
  //
  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) != 0) {
    //
    // Check if the platform supports update capsule across a system reset
    //
    if (!FeaturePcdGet(PcdSupportUpdateCapsuleReset)) {
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
      Status = gRT->SetVariable (
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
      // Successfully set the capsule image address into EFI variable.
      //
      return EFI_SUCCESS;
    }
  }

  //
  // Process the non-reset capsule image.
  //
  if (EfiAtRuntime ()) {
    //
    // Runtime mode doesn't support the non-reset capsule image.
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Here should be in the boot-time for non-reset capsule image
  // Platform specific update for the non-reset capsule image.
  //
  for (ArrayNumber = 0; ArrayNumber < CapsuleCount; ArrayNumber++) {
    Status = ProcessCapsuleImage (CapsuleHeaderArray[ArrayNumber]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Returns if the capsule can be supported via UpdateCapsule().

  @param  CapsuleHeaderArray    Virtual pointer to an array of virtual pointers to the capsules
                                being passed into update capsule.
  @param  CapsuleCount          Number of pointers to EFI_CAPSULE_HEADER in
                                CaspuleHeaderArray.
  @param  MaxiumCapsuleSize     On output the maximum size that UpdateCapsule() can
                                support as an argument to UpdateCapsule() via
                                CapsuleHeaderArray and ScatterGatherList.
  @param  ResetType             Returns the type of reset required for the capsule update.

  @retval EFI_SUCCESS           Valid answer returned.
  @retval EFI_UNSUPPORTED       The capsule image is not supported on this platform, and
                                MaximumCapsuleSize and ResetType are undefined.
  @retval EFI_INVALID_PARAMETER MaximumCapsuleSize is NULL, or ResetTyep is NULL,
                                Or CapsuleCount is Zero, or CapsuleImage is not valid.

**/
EFI_STATUS
EFIAPI
QueryCapsuleCapabilities (
  IN  EFI_CAPSULE_HEADER   **CapsuleHeaderArray,
  IN  UINTN                CapsuleCount,
  OUT UINT64               *MaxiumCapsuleSize,
  OUT EFI_RESET_TYPE       *ResetType
  )
{
  UINTN                     ArrayNumber;
  EFI_CAPSULE_HEADER        *CapsuleHeader;

  //
  // Capsule Count can't be less than one.
  //
  if (CapsuleCount < 1) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check whether input paramter is valid
  //
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
    // Check Capsule image without populate flag is supported by firmware
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
    //Check if the platform supports update capsule across a system reset
    //
    if (!FeaturePcdGet(PcdSupportUpdateCapsuleReset)) {
      return EFI_UNSUPPORTED;
    }
    *ResetType = EfiResetWarm;   
  } else {
    //
    // For non-reset capsule image.
    //
    *ResetType = EfiResetCold;
  }
  
  //
  // The support max capsule image size
  //
  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) != 0) {
    *MaxiumCapsuleSize = FixedPcdGet32(PcdMaxSizePopulateCapsule);
  } else {
    *MaxiumCapsuleSize = FixedPcdGet32(PcdMaxSizeNonPopulateCapsule);
  }

  return EFI_SUCCESS;
}


/**

  This code installs UEFI capsule runtime service.

  @param  ImageHandle    The firmware allocated handle for the EFI image.  
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    UEFI Capsule Runtime Services are installed successfully. 

**/
EFI_STATUS
EFIAPI
CapsuleServiceInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  NewHandle;
  
  //
  // Install capsule runtime services into UEFI runtime service tables.
  //
  gRT->UpdateCapsule                    = UpdateCapsule;
  gRT->QueryCapsuleCapabilities         = QueryCapsuleCapabilities;

  //
  // Install the Capsule Architectural Protocol on a new handle
  // to signify the capsule runtime services are ready.
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
