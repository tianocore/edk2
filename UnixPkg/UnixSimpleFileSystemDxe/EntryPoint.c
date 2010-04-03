/**@file
  Entry Point Source file.

  This file contains the user entry point 

  Copyright (c) 2006 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
   are licensed and made available under the terms and conditions of the BSD License
   which accompanies this distribution. The full text of the license may be found at
   http://opensource.org/licenses/bsd-license.php
   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/



#include "UnixSimpleFileSystem.h"

/**
  The user Entry Point for module UnixSimpleFileSystem. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeUnixSimpleFileSystem(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = EfiLibInstallAllDriverProtocols (
             ImageHandle,
             SystemTable,
             &gUnixSimpleFileSystemDriverBinding,
             ImageHandle,
             &gUnixSimpleFileSystemComponentName,
             NULL,
             NULL
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}
