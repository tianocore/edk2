/** @file
  EFI Shell Dynamic Command registration protocol

  Copyright (c) 2012 Hewlett-Packard Company. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL__
#define __EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL__

#include <ShellBase.h>
#include <Protocol/EfiShellParameters.h>
#include <Protocol/EfiShell.h>


// {0CD3258C-D677-4fcc-B343-934D30983888}
#define EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL_GUID \
  { \
  0xcd3258c, 0xd677, 0x4fcc, { 0xb3, 0x43, 0x93, 0x4d, 0x30, 0x98, 0x38, 0x88 } \
  }


//
// Define for forward reference.
//
typedef struct _EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL;


/**
  This is the shell command handler function pointer callback type.  This
  function handles the command when it is invoked in the shell.

  @param[in] This                   The instance of the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] SystemTable            The pointer to the system table.
  @param[in] ShellParameters        The parameters associated with the command.
  @param[in] Shell                  The instance of the shell protocol used in the context
                                    of processing this command.

  @return EFI_SUCCESS               the operation was sucessful
  @return other                     the operation failed.
**/
typedef
SHELL_STATUS
(EFIAPI * SHELL_COMMAND_HANDLER)(
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL    *This,
  IN EFI_SYSTEM_TABLE                      *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL         *ShellParameters,
  IN EFI_SHELL_PROTOCOL                    *Shell
  );

/**
  This is the command help handler function pointer callback type.  This
  function is responsible for displaying help information for the associated
  command.

  @param[in] This                   The instance of the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] Language               The pointer to the language string to use.

  @return string                    Pool allocated help string, must be freed by caller
**/
typedef
CHAR16*
(EFIAPI * SHELL_COMMAND_GETHELP)(
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL    *This,
  IN CONST CHAR8                           *Language
  );

/// EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL protocol structure.
typedef struct _EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL {
  
  CONST CHAR16           *CommandName;
  SHELL_COMMAND_HANDLER  Handler;
  SHELL_COMMAND_GETHELP  GetHelp;

} EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL;

extern EFI_GUID gEfiShellDynamicCommandProtocolGuid;

#endif
