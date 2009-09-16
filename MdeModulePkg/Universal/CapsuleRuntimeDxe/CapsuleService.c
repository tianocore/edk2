/** @file
  Capsule Runtime Driver produces two UEFI capsule runtime services.
  (UpdateCapsule, QueryCapsuleCapabilities)
  It installs the Capsule Architectural Protocol defined in PI1.0a to signify 
  the capsule runtime services are ready.

Copyright (c) 2006 - 2009, Intel Corporation. <BR>
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

//
// Handle for the installation of Capsule Architecture Protocol.
//
EFI_HANDLE  mNewHandle = NULL;

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
  @retval EFI_INVALID_PARAMETER CapsuleSize is NULL, or an incompatible set of flags were
                                set in the capsule header.
  @retval EFI_INVALID_PARAMETER CapsuleCount is Zero.
  @retval EFI_INVALID_PARAMETER For across reset capsule image, ScatterGatherList is NULL.
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
  BOOLEAN                   NeedReset;
  BOOLEAN                   InitiateReset;
  
  //
  // Capsule Count can't be less than one.
  //
  if (CapsuleCount < 1) {
    return EFI_INVALID_PARAMETER;
  }

  NeedReset     = FALSE;
  InitiateReset = FALSE;
  CapsuleHeader = NULL;

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
    // A capsule which has the CAPSULE_FLAGS_INITIATE_RESET flag must have
    // CAPSULE_FLAGS_PERSIST_ACROSS_RESET set in its header as well.
    //
    if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_INITIATE_RESET)) == CAPSULE_FLAGS_INITIATE_RESET) {
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
  // Walk through all capsules, record whether there is a capsule needs reset
  // or initiate reset. And then process capsules which has no reset flag directly.
  //
  for (ArrayNumber = 0; ArrayNumber < CapsuleCount ; ArrayNumber++) {
    CapsuleHeader = CapsuleHeaderArray[ArrayNumber];
    //
    // Here should be in the boot-time for non-reset capsule image
    // Platform specific update for the non-reset capsule image.
    //
    if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) == 0) {
      if (EfiAtRuntime ()) { 
        Status = EFI_UNSUPPORTED;
      } else {
        Status = ProcessCapsuleImage(CapsuleHeader);
      }
      if (EFI_ERROR(Status)) {
        return Status;
      }
    } else {
      NeedReset = TRUE;
      if ((CapsuleHeader->Flags & CAPSULE_FLAGS_INITIATE_RESET) != 0) {
        InitiateReset = TRUE;
      }
    }
  }
  
  //
  // After launching all capsules who has no reset flag, if no more capsules claims
  // for a system reset just return.
  //
  if (!NeedReset) {
    return EFI_SUCCESS;
  }

  //
  // ScatterGatherList is only referenced if the capsules are defined to persist across
  // system reset. 
  //
  if (ScatterGatherList == (EFI_PHYSICAL_ADDRESS) (UINTN) NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check if the platform supports update capsule across a system reset
  //
  if (!FeaturePcdGet(PcdSupportUpdateCapsuleReset)) {
    return EFI_UNSUPPORTED;
  }

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
  if (!EFI_ERROR (Status) && InitiateReset) {
    //
    // Firmware that encounters a capsule which has the CAPSULE_FLAGS_INITIATE_RESET Flag set in its header
    // will initiate a reset of the platform which is compatible with the passed-in capsule request and will 
    // not return back to the caller.
    //
    gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
  }
  return Status;
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
  // Check whether input parameter is valid
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
    // A capsule which has the CAPSULE_FLAGS_INITIATE_RESET flag must have
    // CAPSULE_FLAGS_PERSIST_ACROSS_RESET set in its header as well.
    //
    if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_INITIATE_RESET)) == CAPSULE_FLAGS_INITIATE_RESET) {
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
    *MaxiumCapsuleSize = PcdGet32(PcdMaxSizePopulateCapsule);
  } else {
    *MaxiumCapsuleSize = PcdGet32(PcdMaxSizeNonPopulateCapsule);
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
  
  //
  // Install capsule runtime services into UEFI runtime service tables.
  //
  gRT->UpdateCapsule                    = UpdateCapsule;
  gRT->QueryCapsuleCapabilities         = QueryCapsuleCapabilities;

  //
  // Install the Capsule Architectural Protocol on a new handle
  // to signify the capsule runtime services are ready.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mNewHandle,
                  &gEfiCapsuleArchProtocolGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
