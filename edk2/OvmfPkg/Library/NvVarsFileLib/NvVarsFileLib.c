/** @file
  Save Non-Volatile Variables to a file system.

  Copyright (c) 2009, Intel Corporation. <BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "NvVarsFileLib.h"
#include <Library/DebugLib.h>
#include <Library/NvVarsFileLib.h>


/**
  Attempts to connect the NvVarsFileLib to the specified file system.

  @param[in]  FsHandle - Handle for a gEfiSimpleFileSystemProtocolGuid instance

  @return     The EFI_STATUS while attempting to connect the NvVarsFileLib
              to the file system instance.
  @retval     EFI_SUCCESS - The given file system was connected successfully

**/
EFI_STATUS
EFIAPI
ConnectNvVarsToFileSystem (
  IN EFI_HANDLE    FsHandle
  )
{
  EFI_STATUS Status;

  //
  // We might fail to load the variable, since the file system initially
  // will not have the NvVars file.
  //
  LoadNvVarsFromFs (FsHandle);

  //
  // We must be able to save the variables successfully to the file system
  // to have connected successfully.
  //
  Status = SaveNvVarsToFs (FsHandle);

  return Status;
}


