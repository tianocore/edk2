/** @file
  Entry point library instance to a UEFI application. This version is specific to QEMU, and ties
  dispatch of the application in question on the value of a QEMU fw_cfg boolean
  variable which is referenced by name via a fixed pointer PCD.

  Copyright (c) 2024, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/QemuFwCfgSimpleParserLib.h>

/**
  Entry point to UEFI Application.

  This function is the entry point for a UEFI Application. This function must call
  ProcessLibraryConstructorList(), ProcessModuleEntryPointList(), and ProcessLibraryDestructorList().
  The return value from ProcessModuleEntryPointList() is returned.
  If _gUefiDriverRevision is not zero and SystemTable->Hdr.Revision is less than _gUefiDriverRevison,
  then return EFI_INCOMPATIBLE_VERSION.

  @param  ImageHandle                The image handle of the UEFI Application.
  @param  SystemTable                A pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The UEFI Application exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than SystemTable->Hdr.Revision.
  @retval  Other                     Return value from ProcessModuleEntryPointList().

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS     Status;
  BOOLEAN        Enabled;
  RETURN_STATUS  RetStatus;

  if (_gUefiDriverRevision != 0) {
    //
    // Make sure that the EFI/UEFI spec revision of the platform is >= EFI/UEFI spec revision of the application.
    //
    if (SystemTable->Hdr.Revision < _gUefiDriverRevision) {
      return EFI_INCOMPATIBLE_VERSION;
    }
  }

  //
  // Call constructor for all libraries.
  //
  ProcessLibraryConstructorList (ImageHandle, SystemTable);

  RetStatus = QemuFwCfgParseBool (
                FixedPcdGetPtr (PcdEntryPointOverrideFwCfgVarName),
                &Enabled
                );

  if (!RETURN_ERROR (RetStatus) && !Enabled) {
    //
    // The QEMU fw_cfg variable tells us not to load this image.  So abort.
    //
    Status = EFI_ABORTED;

    // If an error message is set, print it and wait for two secs
    if (AsciiStrCmp (FixedPcdGetPtr (PcdEntryPointOverrideErrorMessage), "") != 0) {
      Print (L"%a\n", FixedPcdGetPtr (PcdEntryPointOverrideErrorMessage));
      gBS->Stall (2000000); // two secs delay
    }
  } else {
    //
    // Call the module's entry point
    //
    Status = ProcessModuleEntryPointList (ImageHandle, SystemTable);
  }

  //
  // Process destructor for all libraries.
  //
  ProcessLibraryDestructorList (ImageHandle, SystemTable);

  //
  // Return the return status code from the driver entry point
  //
  return Status;
}

/**
  Invokes the library destructors for all dependent libraries and terminates
  the UEFI Application.

  This function calls ProcessLibraryDestructorList() and the EFI Boot Service Exit()
  with a status specified by Status.

  @param  Status  Status returned by the application that is exiting.

**/
VOID
EFIAPI
Exit (
  IN EFI_STATUS  Status
  )

{
  ProcessLibraryDestructorList (gImageHandle, gST);

  gBS->Exit (gImageHandle, Status, 0, NULL);
}

/**
  Required by the EBC compiler and identical in functionality to _ModuleEntryPoint().

  @param  ImageHandle  The image handle of the UEFI Application.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The UEFI Application exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than SystemTable->Hdr.Revision.
  @retval  Other                     Return value from ProcessModuleEntryPointList().

**/
EFI_STATUS
EFIAPI
EfiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return _ModuleEntryPoint (ImageHandle, SystemTable);
}
