/** @file
  Provides interface to the shell protocols.

  Copyright (c) 2024, 9elements GmbH.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SHELL_PROTOCOLS_LIB__
#define __SHELL_PROTOCOLS_LIB__

#include <Uefi.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Shell.h>
#include <Protocol/ShellParameters.h>
#include <Guid/ShellVariableGuid.h>

typedef struct {
  LIST_ENTRY            Link;
  EFI_SHELL_PROTOCOL    *Interface;
  EFI_HANDLE            Handle;
} SHELL_PROTOCOL_HANDLE_LIST;

typedef struct {
  UINT32    NoConsoleOut : 1;  ///< Was "-noconsoleout"  found on command line.
  UINT32    NoNest       : 1;  ///< Was "-nonest"        found on command line
  UINT32    Reserved     : 14; ///< Extra bits
} SHELL_PROTOCOL_BITS;

typedef union {
  SHELL_PROTOCOL_BITS  Bits;
  UINT16               AllBits;
} SHELL_PROTOCOL_BIT_UNION;

typedef struct {
  EFI_SHELL_PARAMETERS_PROTOCOL  *NewShellParametersProtocol;
  EFI_SHELL_PROTOCOL             *NewEfiShellProtocol;
  BOOLEAN                        RootShellInstance;
  SHELL_PROTOCOL_BIT_UNION       ShellProtocolBitUnion;
  BUFFER_LIST                    BufferToFreeList;    ///< List of buffers that were returned to the user to free.
  EFI_HII_HANDLE                 HiiHandle;           ///< Handle from HiiLib.
  EFI_DEVICE_PATH_PROTOCOL       *ImageDevPath;       ///< DevicePath for ourselves.
  EFI_DEVICE_PATH_PROTOCOL       *FileDevPath;        ///< DevicePath for ourselves.
  EFI_SHELL_PARAMETERS_PROTOCOL  *OldShellParameters; ///< old shell parameters to reinstall upon exiting.
  SHELL_PROTOCOL_HANDLE_LIST     OldShellList;        ///< List of other instances to reinstall when closing.
} SHELL_PROTOCOL_INFO;

#pragma pack(1)
///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} SHELL_MAN_HII_VENDOR_DEVICE_PATH;
#pragma pack()

extern SHELL_PROTOCOL_INFO  ShellProtocolsInfoObject;

/**
  Function to create and install on the current handle.

  Will overwrite any existing ShellProtocols in the system to be sure that
  the current shell is in control.

  This must be removed via calling CleanUpShellProtocol().

  @param[in, out] NewShell   The pointer to the pointer to the structure
  to install.

  @retval EFI_SUCCESS     The operation was successful.
  @return                 An error from LocateHandle, CreateEvent, or other core function.
**/
EFI_STATUS
CreatePopulateInstallShellProtocol (
  IN OUT EFI_SHELL_PROTOCOL  **NewShell
  );

/**
  Opposite of CreatePopulateInstallShellProtocol.

  Free all memory and restore the system to the state it was in before calling
  CreatePopulateInstallShellProtocol.

  @param[in, out] NewShell   The pointer to the new shell protocol structure.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
CleanUpShellProtocol (
  IN OUT EFI_SHELL_PROTOCOL  *NewShell
  );

/**
  Cleanup the shell environment.

  @param[in, out] NewShell   The pointer to the new shell protocol structure.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
CleanUpShellEnvironment (
  IN OUT EFI_SHELL_PROTOCOL  *NewShell
  );

/**
  Calculate the 32-bit CRC in a EFI table using the service provided by the
  gRuntime service.

  @param  Hdr                    Pointer to an EFI standard header

**/
VOID
CalculateEfiHdrCrc (
  IN  OUT EFI_TABLE_HEADER  *Hdr
  );

#endif
