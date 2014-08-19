/** @file
  Include file for basic command line parser for EBL (Embedded Boot Loader)

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EBL_H__
#define __EBL_H__

#include <PiDxe.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadFile.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/PxeBaseCode.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/EblAddCommand.h>
#include <Protocol/PciIo.h>
#include <Protocol/DevicePath.h>

#include <Guid/FileInfo.h>
#include <Guid/DxeServices.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/MemoryAllocationHob.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/EfiFileLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/EblCmdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/EblNetworkLib.h>
#include <Library/TimerLib.h>

#include <IndustryStandard/Pci.h>

//
// Prompt for the command line
//
#define CMD_SEPARATOR             ';'
#define EBL_MAX_COMMAND_COUNT     0x100
#define MAX_CMD_HISTORY           16
#define MAX_CMD_LINE              256
#define MAX_ARGS                  32

#define EBL_CR                    0x0a
#define EBL_LF                    0x0d

#define EFI_SET_TIMER_TO_SECOND   10000000



EBL_COMMAND_TABLE *
EblGetCommand (
  IN CHAR8                        *CommandName
  );


EFI_STATUS
EblPathToDevice (
  IN  CHAR8                       *Path,
  OUT EFI_HANDLE                  *DeviceHandle,
  OUT EFI_DEVICE_PATH_PROTOCOL    **PathDevicePath,
  OUT VOID                        **Buffer,
  OUT UINTN                       *BufferSize
  );

BOOLEAN
EblAnyKeyToContinueQtoQuit (
  IN  UINTN                       *CurrentRow,
  IN  BOOLEAN                     PrefixNewline
  );

VOID
EblUpdateDeviceLists (
  VOID
  );

VOID
EblInitializeCmdTable (
  VOID
  );

VOID
EblShutdownExternalCmdTable (
  VOID
  );

VOID
EblSetTextColor (
  UINTN                           Attribute
  );


EFI_STATUS
EblGetCharKey (
  IN OUT EFI_INPUT_KEY            *Key,
  IN     UINTN                    TimoutInSec,
  IN     EBL_GET_CHAR_CALL_BACK   CallBack   OPTIONAL
  );

// BugBug: Move me to a library
INTN
EFIAPI
AsciiStrniCmp (
  IN      CONST CHAR8             *FirstString,
  IN      CONST CHAR8             *SecondString,
  IN      UINTN                   Length
  );


VOID
EblInitializeDeviceCmd (
  VOID
  );

VOID
EblInitializemdHwDebugCmds (
  VOID
  );

VOID
EblInitializeDirCmd (
  VOID
  );

VOID
EblInitializeHobCmd (
  VOID
  );

VOID
EblInitializemdHwIoDebugCmds (
  VOID
  );

VOID
EblInitializeScriptCmd (
  VOID
  );

VOID
EblInitializeNetworkCmd (
  VOID
  );

VOID
EblInitializeVariableCmds (
  VOID
  );

CHAR8 *
ParseArguments (
  IN  CHAR8                       *CmdLine,
  OUT UINTN                       *Argc,
  OUT CHAR8                       **Argv
  );

EFI_STATUS
ProcessCmdLine (
  IN CHAR8                        *CmdLine,
  IN UINTN                        MaxCmdLineSize
  );

EFI_STATUS
OutputData (
  IN UINT8                        *Address,
  IN UINTN                        Length,
  IN UINTN                         Width,
  IN UINTN                        Offset
  );

UINTN
WidthFromCommandName (
  IN CHAR8                        *Argv,
  IN UINTN                        Default
  );


extern UINTN                      gScreenColumns;
extern UINTN                      gScreenRows;
extern BOOLEAN                    gPageBreak;
extern CHAR8                      *gMemMapType[];

#endif

