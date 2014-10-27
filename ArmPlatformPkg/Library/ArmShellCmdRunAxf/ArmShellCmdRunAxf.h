/** @file
*
*  Copyright (c) 2014, ARM Ltd. All rights reserved.<BR>
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __ARM_SHELL_CMD_RUNAXF__
#define __ARM_SHELL_CMD_RUNAXF__

#include <ShellBase.h>

#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellDynamicCommand.h>

#include <Library/HiiLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>

extern EFI_GUID      gRunAxfHiiGuid;
extern EFI_HANDLE    gRunAxfHiiHandle;
extern EFI_HANDLE    gRunAxfImageHandle;

// List of data segments to load to memory from AXF/ELF file.
typedef struct {
  LIST_ENTRY  Link;       // This attribute must be the first entry of this
                          // structure (to avoid pointer computation)
  UINTN       MemOffset;  // Where the data should go, Dest
  UINTN       FileOffset; // Where the data is from, Src
  BOOLEAN     Zeroes;     // A section of Zeroes. Like .bss in ELF
  UINTN       Length;     // Number of bytes.
} RUNAXF_LOAD_LIST;


/**
  This is the shell command handler function pointer callback type. This
  function handles the command when it is invoked in the shell.

  @param[in] This             The instance of the
                              EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] SystemTable      The pointer to the system table.
  @param[in] ShellParameters  The parameters associated with the command.
  @param[in] Shell            The instance of the shell protocol used in the
                              context of processing this command.

  @return EFI_SUCCESS         The operation was successful.
  @return other               The operation failed.
**/
SHELL_STATUS
EFIAPI
ShellDynCmdRunAxfHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL    *This,
  IN EFI_SYSTEM_TABLE                      *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL         *ShellParameters,
  IN EFI_SHELL_PROTOCOL                    *Shell
  );


/**
  This is the command help handler function pointer callback type. This
  function is responsible for displaying help information for the associated
  command.

  @param[in] This             The instance of the
                              EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] Language         The pointer to the language string to use.

  @return string              Pool allocated help string, must be freed by
                              caller.
**/
CHAR16*
EFIAPI
ShellDynCmdRunAxfGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL    *This,
  IN CONST CHAR8                           *Language
  );

#endif //__ARM_SHELL_CMD_RUNAXF__
