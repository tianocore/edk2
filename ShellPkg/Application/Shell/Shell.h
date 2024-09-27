/** @file
  function definitions for internal to shell functions.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_INTERNAL_HEADER_
#define _SHELL_INTERNAL_HEADER_

#include <Uefi.h>

#include <Guid/ShellVariableGuid.h>
#include <Guid/ShellAliasGuid.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/Shell.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellEnvironment2.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/BlockIo.h>
#include <Protocol/HiiPackageList.h>

#include <Library/BaseLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellLib.h>
#include <Library/ShellProtocolInteractivityLib.h>
#include <Library/ShellProtocolsLib.h>
#include <Library/SortLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/FileHandleLib.h>
#include <Library/UefiHiiServicesLib.h>

#include <ShellInternals.h>

#include "ShellProtocol.h"
#include "FileHandleWrappers.h"

typedef struct {
  UINT32    Startup      : 1; ///< Was "-startup"       found on command line.
  UINT32    NoStartup    : 1; ///< Was "-nostartup"     found on command line.
  UINT32    NoConsoleIn  : 1; ///< Was "-noconsolein"   found on command line.
  UINT32    NoInterrupt  : 1; ///< Was "-nointerrupt"   found on command line.
  UINT32    NoMap        : 1; ///< Was "-nomap"         found on command line.
  UINT32    NoVersion    : 1; ///< Was "-noversion"     found on command line.
  UINT32    Delay        : 1; ///< Was "-delay[:n]      found on command line
  UINT32    Exit         : 1; ///< Was "-_exit"         found on command line
  UINT32    Reserved     : 6; ///< Extra bits
} SHELL_BITS;

typedef union {
  SHELL_BITS    Bits;
  UINT16        AllBits;
} SHELL_BIT_UNION;

typedef struct {
  SHELL_BIT_UNION    BitUnion;
  UINTN              Delay;        ///< Seconds of delay default:5.
  CHAR16             *FileName;    ///< Filename to run upon successful initialization.
  CHAR16             *FileOptions; ///< Options to pass to FileName.
} SHELL_INIT_SETTINGS;

typedef struct {
  SHELL_INIT_SETTINGS       ShellInitSettings;
  EFI_HII_HANDLE            HiiHandle;           ///< Handle from HiiLib.
  EFI_EVENT                 UserBreakTimer;      ///< Timer event for polling for CTRL-C.
} SHELL_INFO;

extern SHELL_INFO  ShellInfoObject;

/**
  Process all Uefi Shell 2.0 command line options.

  see Uefi Shell 2.0 section 3.2 for full details.

  the command line should resemble the following:

  shell.efi [ShellOpt-options] [options] [file-name [file-name-options]]

  ShellOpt options  Options which control the initialization behavior of the shell.
                    These options are read from the EFI global variable "ShellOpt"
                    and are processed before options or file-name.

  options           Options which control the initialization behavior of the shell.

  file-name         The name of a UEFI shell application or script to be executed
                    after initialization is complete. By default, if file-name is
                    specified, then -nostartup is implied. Scripts are not supported
                    by level 0.

  file-nameoptions  The command-line options that are passed to file-name when it
                    is invoked.

  This will initialize the ShellInitSettings global variable.

  @retval EFI_SUCCESS           the variable is initialized.
**/
EFI_STATUS
ProcessCommandLine (
  VOID
  );

/**
  Handles all interaction with the default startup script.

  this will check that the correct command line parameters were passed, handle the delay, and then start running the script.

  @param[in] ImagePath          The path to the image for shell.  The first place to look for the startup script.
  @param[in] FilePath           The path to the file for shell.  The second place to look for the startup script.

  @retval EFI_SUCCESS           The variable is initialized.
**/
EFI_STATUS
DoStartupScript (
  IN EFI_DEVICE_PATH_PROTOCOL  *ImagePath,
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

/**
  Function to perform the shell prompt looping.  It will do a single prompt,
  dispatch the result, and then return.  It is expected that the caller will
  call this function in a loop many times.

  @retval EFI_SUCCESS
  @retval RETURN_ABORTED
**/
EFI_STATUS
DoShellPrompt (
  VOID
  );

#endif //_SHELL_INTERNAL_HEADER_
