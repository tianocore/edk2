/** @file

  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FDT_PLATFORM_DXE_H__
#define __FDT_PLATFORM_DXE_H__

#include <Uefi.h>

#include <Protocol/DevicePathToText.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellDynamicCommand.h>

#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Guid/Fdt.h>

#include <libfdt.h>

extern EFI_HANDLE mFdtPlatformDxeHiiHandle;

/**
  Transcode one of the EFI return code used by the model into an EFI Shell return code.

  @param[in]  Status  EFI return code.

  @return  Transcoded EFI Shell return code.

**/
SHELL_STATUS
EfiCodeToShellCode (
  IN EFI_STATUS  Status
  );

/**
  Run the FDT installation process.

  Loop in priority order over the device paths from which the FDT has
  been asked to be retrieved for. For each device path, try to install
  the FDT. Stop as soon as an installation succeeds.

  @param[in]  SuccessfullDevicePath  If not NULL, address where to store the
                                     pointer to the text device path from
                                     which the FDT was successfully retrieved.
                                     Not used if the FDT installation failed.
                                     The returned address is the address of
                                     an allocated buffer that has to be
                                     freed by the caller.

  @retval  EFI_SUCCESS            The FDT was installed.
  @retval  EFI_NOT_FOUND          Failed to locate a protocol or a file.
  @retval  EFI_INVALID_PARAMETER  Invalid device path.
  @retval  EFI_UNSUPPORTED        Device path not supported.
  @retval  EFI_OUT_OF_RESOURCES   An allocation failed.

**/
EFI_STATUS
RunFdtInstallation (
  OUT CHAR16  **SuccessfullDevicePath
  );

/**
  This is the shell command "setfdt" handler function. This function handles
  the command when it is invoked in the shell.

  @param[in]  This             The instance of the
                               EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in]  SystemTable      The pointer to the UEFI system table.
  @param[in]  ShellParameters  The parameters associated with the command.
  @param[in]  Shell            The instance of the shell protocol used in the
                               context of processing this command.

  @return  SHELL_SUCCESS            The operation was successful.
  @return  SHELL_ABORTED            Operation aborted due to internal error.
  @return  SHELL_INVALID_PARAMETER  The parameters of the command are not valid.
  @return  SHELL_INVALID_PARAMETER  The EFI Shell file path is not valid.
  @return  SHELL_NOT_FOUND          Failed to locate a protocol or a file.
  @return  SHELL_UNSUPPORTED        Device path not supported.
  @return  SHELL_OUT_OF_RESOURCES   A memory allocation failed.
  @return  SHELL_DEVICE_ERROR       The "Fdt" variable could not be saved due to a hardware failure.
  @return  SHELL_ACCESS_DENIED      The "Fdt" variable is read-only.
  @return  SHELL_ACCESS_DENIED      The "Fdt" variable cannot be deleted.
  @return  SHELL_ACCESS_DENIED      The "Fdt" variable could not be written due to security violation.

**/
SHELL_STATUS
EFIAPI
ShellDynCmdSetFdtHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN EFI_SYSTEM_TABLE                    *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters,
  IN EFI_SHELL_PROTOCOL                  *Shell
  );

/**
  This is the shell command "setfdt" help handler function. This
  function returns the formatted help for the "setfdt" command.
  The format matchs that in Appendix B of the revision 2.1 of the
  UEFI Shell Specification.

  @param[in]  This      The instance of the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in]  Language  The pointer to the language string to use.

  @return  CHAR16*  Pool allocated help string, must be freed by caller.
**/
CHAR16*
EFIAPI
ShellDynCmdSetFdtGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                         *Language
  );

/**
  This is the shell command "dumpfdt" handler function. This function handles
  the command when it is invoked in the shell.

  @param[in]  This             The instance of the
                               EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in]  SystemTable      The pointer to the UEFI system table.
  @param[in]  ShellParameters  The parameters associated with the command.
  @param[in]  Shell            The instance of the shell protocol used in the
                               context of processing this command.

  @return  SHELL_SUCCESS            The operation was successful.
  @return  SHELL_ABORTED            Operation aborted due to internal error.
  @return  SHELL_NOT_FOUND          Failed to locate the Device Tree into the EFI Configuration Table
  @return  SHELL_OUT_OF_RESOURCES   A memory allocation failed.

**/
SHELL_STATUS
EFIAPI
ShellDynCmdDumpFdtHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN EFI_SYSTEM_TABLE                    *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters,
  IN EFI_SHELL_PROTOCOL                  *Shell
  );

/**
  This is the shell command "dumpfdt" help handler function. This
  function returns the formatted help for the "dumpfdt" command.
  The format matchs that in Appendix B of the revision 2.1 of the
  UEFI Shell Specification.

  @param[in]  This      The instance of the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in]  Language  The pointer to the language string to use.

  @return  CHAR16*  Pool allocated help string, must be freed by caller.
**/
CHAR16*
EFIAPI
ShellDynCmdDumpFdtGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                         *Language
  );

#endif /* __FDT_PLATFORM_DXE_H__ */
