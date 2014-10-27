/** @file
*
*  Definitions for the Dynamic Shell command library
*
*  Copyright (C) 2014, ARM Ltd
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution. The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
*  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef _ARM_SHELL_CMD_LIB_H_
#define _ARM_SHELL_CMD_LIB_H_

/**

  Initialize and Install EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL for RunAxf command.

  @param[in] ImageHandle        Handle the protocol should be attached to.

  @retval EFI_SUCCESS           The command has been installed successfully.

  @retval EFI_UNSUPPORTED       Help for the command failed to initialise.

  @return                       Status code returned by InstallProtocolInterface
                                Boot Service function.

**/
EFI_STATUS
ShellDynCmdRunAxfInstall (
  IN EFI_HANDLE             ImageHandle
  );

/**

  Uninstall the RunAxf Command

  @param[in] ImageHandle  Handle of the device where the protocol should have
                          been installed.

  @retval EFI_SUCCESS     The device has been un-initialized successfully.

  @return                 Status code returned by UninstallProtocolInterface
                          Boot Service function.

**/
EFI_STATUS
ShellDynCmdRunAxfUninstall (
  IN EFI_HANDLE             ImageHandle
  );

#endif // _ARM_SHELL_CMD_LIB_H_
