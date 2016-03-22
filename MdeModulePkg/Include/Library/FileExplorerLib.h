/** @file

  This library class defines a set of interfaces for how to do file explorer.

Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FILE_EXPLORER_LIB_H__
#define __FILE_EXPLORER_LIB_H__

#include <Protocol/FileExplorer.h>

/**
  Choose a file in the specified directory. 

  If user input NULL for the RootDirectory, will choose file in the system.

  If user input *File != NULL, function will return the allocate device path
  info for the choosed file, caller has to free the memory after use it.

  @param  RootDirectory    Pointer to the root directory.
  @param  FileType         The file type need to choose.
  @param  ChooseHandler    Function pointer to the extra task need to do
                           after choose one file.
  @param  File             Return the device path for the last time chosed file.

  @retval EFI_SUCESS       Choose the file success.
  @retval Other errors     Choose the file failed.
**/
EFI_STATUS
EFIAPI
ChooseFile (
  IN  EFI_DEVICE_PATH_PROTOCOL  *RootDirectory,
  IN  CHAR16                    *FileType,  OPTIONAL
  IN  CHOOSE_HANDLER            ChooseHandler,  OPTIONAL
  OUT EFI_DEVICE_PATH_PROTOCOL  **File  OPTIONAL
  );

#endif
