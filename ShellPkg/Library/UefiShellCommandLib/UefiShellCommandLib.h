/** @file
  Provides interface to shell internal functions for shell commands.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved. <BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UEFI_COMMAND_LIB_INTERNAL_HEADER_
#define _UEFI_COMMAND_LIB_INTERNAL_HEADER_

#include <Uefi.h>

#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellEnvironment2.h>
#include <Protocol/Shell.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/BlockIo.h>
#include <Protocol/ShellDynamicCommand.h>

#include <Library/DevicePathLib.h>
#include <Library/SortLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/OrderedCollectionLib.h>

typedef struct{
  LIST_ENTRY                  Link;
  CHAR16                      *CommandString;
  SHELL_GET_MAN_FILENAME      GetManFileName;
  SHELL_RUN_COMMAND           CommandHandler;
  BOOLEAN                     LastError;
  EFI_HII_HANDLE              HiiHandle;
  EFI_STRING_ID               ManFormatHelp;
} SHELL_COMMAND_INTERNAL_LIST_ENTRY;

typedef struct {
  LIST_ENTRY Link;
  SCRIPT_FILE *Data;
} SCRIPT_FILE_LIST;

typedef struct {
  EFI_FILE_PROTOCOL *FileHandle;
  CHAR16            *Path;
} SHELL_COMMAND_FILE_HANDLE;

//
// Collects multiple EFI_SHELL_FILE_INFO objects that share the same name.
//
typedef struct {
  //
  // A string that compares equal to either the FileName or the FullName fields
  // of all EFI_SHELL_FILE_INFO objects on SameNameList, according to
  // gUnicodeCollation->StriColl(). The string is not dynamically allocated;
  // instead, it *aliases* the FileName or FullName field of the
  // EFI_SHELL_FILE_INFO object that was first encountered with this name.
  //
  CONST CHAR16 *Alias;
  //
  // A list of EFI_SHELL_FILE_INFO objects whose FileName or FullName fields
  // compare equal to Alias, according to gUnicodeCollation->StriColl().
  //
  LIST_ENTRY SameNameList;
} SHELL_SORT_UNIQUE_NAME;

#endif //_UEFI_COMMAND_LIB_INTERNAL_HEADER_

