/** @file
  Entry point to a UEFI Application.

Copyright (c) 2007, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __UEFI_APPLICATION_ENTRY_POINT_H__
#define __UEFI_APPLICATION_ENTRY_POINT_H__

//
// Declare the EFI/UEFI Specification Revision to which this driver is implemented 
//
extern const UINT32                   _gUefiDriverRevision;

/**
  Enrty point to UEFI Application.

  @param  ImageHandle ImageHandle of the loaded driver.
  @param  SystemTable Pointer to the EFI System Table.

  @retval  EFI_SUCCESS One or more of the drivers returned a success code.
  @retval  !EFI_SUCESS The return status from the last driver entry point in the list.

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );


/**
  Enrty point wrapper of UEFI Application.

  @param  ImageHandle ImageHandle of the loaded driver.
  @param  SystemTable Pointer to the EFI System Table.

  @retval  EFI_SUCCESS One or more of the drivers returned a success code.
  @retval  !EFI_SUCESS The return status from the last driver entry point in the list.

**/
EFI_STATUS
EFIAPI
EfiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );


/**
  Invoke the destuctors of all libraries and call gBS->Exit
  to return control to firmware core.

  @param  Status Status returned by the application that is exiting.
  
  @retval VOID

**/
VOID
EFIAPI
Exit (
  IN EFI_STATUS  Status
  );


/**
  Call constructors for all libraries.  Autogen tool inserts the implementation
  of this function into Autogen.c.

  @param  ImageHandle ImageHandle of the loaded driver.
  @param  SystemTable Pointer to the EFI System Table.
  
  @retval VOID

**/
VOID
EFIAPI
ProcessLibraryConstructorList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );


/**
  Call destructors for all libraries. Autogen tool inserts the implementation
  of this function into Autogen.c.

  @param  ImageHandle ImageHandle of the loaded driver.
  @param  SystemTable Pointer to the EFI System Table.

  @retval VOID
**/
VOID
EFIAPI
ProcessLibraryDestructorList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Call driver entry point. For UEFI application, user
  can only specify one entry point. Tool will automatically insert
  this to Autogen.c.

  @param  ImageHandle ImageHandle of the loaded driver.
  @param  SystemTable Pointer to the EFI System Table.

  @return Status returned by entry points specified by
          the user.  
 
**/

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif
