/** @file

  This file explorer protocol defines defines a set of interfaces for
  how to do file explorer.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FILE_EXPLORER_H__
#define __FILE_EXPLORER_H__

#define EFI_FILE_EXPLORER_PROTOCOL_GUID  \
  { 0x2C03C536, 0x4594, 0x4515, { 0x9E, 0x7A, 0xD3, 0xD2, 0x04, 0xFE, 0x13, 0x63 } }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_FILE_EXPLORER_PROTOCOL  EFI_FILE_EXPLORER_PROTOCOL;

/**
  Prototype for the next process after user chosed one file.

  @param[in] FilePath     The device path of the find file.

  @retval    TRUE         Need exit file explorer after do the extra task.
  @retval    FALSE        Not need to exit file explorer after do the extra task.

**/
typedef
BOOLEAN
(EFIAPI *CHOOSE_HANDLER)(
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

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
typedef
EFI_STATUS
(EFIAPI   *CHOOSE_FILE) (
  IN  EFI_DEVICE_PATH_PROTOCOL  *RootDirectory,
  IN  CHAR16                    *FileType,  OPTIONAL
  IN  CHOOSE_HANDLER            ChooseHandler,  OPTIONAL
  OUT EFI_DEVICE_PATH_PROTOCOL  **File  OPTIONAL
  );

struct _EFI_FILE_EXPLORER_PROTOCOL {
  CHOOSE_FILE                          ChooseFile;
};

extern EFI_GUID gEfiFileExplorerProtocolGuid;

#endif
