/** @file
  Member functions of EFI_SHELL_PARAMETERS_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PARAMETERS_PROTOCOL.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_PARAMETERS_PROTOCOL_PROVIDER_HEADER_
#define _SHELL_PARAMETERS_PROTOCOL_PROVIDER_HEADER_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FileHandleLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/ShellProtocolsLib.h>
#include <Library/SortLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/SimpleFileSystem.h>

/**
  creates a new EFI_SHELL_PARAMETERS_PROTOCOL instance and populates it and then
  installs it on our handle and if there is an existing version of the protocol
  that one is cached for removal later.

  @param[in, out] NewShellParameters on a successful return, a pointer to pointer
                                     to the newly installed interface.
  @param[in, out] RootShellInstance  on a successful return, pointer to boolean.
                                     TRUE if this is the root shell instance.

  @retval EFI_SUCCESS               the operation completed successfully.
  @return other                     the operation failed.
  @sa ReinstallProtocolInterface
  @sa InstallProtocolInterface
  @sa ParseCommandLineToArgs
**/
EFI_STATUS
CreatePopulateInstallShellParametersProtocol (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  **NewShellParameters,
  IN OUT BOOLEAN                        *RootShellInstance
  );

/**
  frees all memory used by creation and installation of shell parameters protocol
  and if there was an old version installed it will restore that one.

  @param NewShellParameters the interface of EFI_SHELL_PARAMETERS_PROTOCOL that is
  being cleaned up.

  @retval EFI_SUCCESS     the cleanup was successful
  @return other           the cleanup failed
  @sa ReinstallProtocolInterface
  @sa UninstallProtocolInterface
**/
EFI_STATUS
CleanUpShellParametersProtocol (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *NewShellParameters
  );

/**
  function to populate Argc and Argv.

  This function parses the CommandLine and divides it into standard C style Argc/Argv
  parameters for inclusion in EFI_SHELL_PARAMETERS_PROTOCOL.  this supports space
  delimited and quote surrounded parameter definition.

  @param[in] CommandLine          String of command line to parse
  @param[in] StripQuotation       if TRUE then strip the quotation marks surrounding
                                  the parameters.
  @param[in, out] Argv            pointer to array of strings; one for each parameter
  @param[in, out] Argc            pointer to number of strings in Argv array

  @return EFI_SUCCESS           the operation was successful
  @return EFI_INVALID_PARAMETER some parameters are invalid
  @return EFI_OUT_OF_RESOURCES  a memory allocation failed.
**/
EFI_STATUS
ParseCommandLineToArgs (
  IN CONST CHAR16  *CommandLine,
  IN BOOLEAN       StripQuotation,
  IN OUT CHAR16    ***Argv,
  IN OUT UINTN     *Argc
  );

#endif //_SHELL_PARAMETERS_PROTOCOL_PROVIDER_HEADER_
