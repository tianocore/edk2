/**@file
  Common header file shared by all source files.

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

  Copyright (c) 2006 - 2007, Intel Corporation.
  All rights reserved. This program and the accompanying materials
   are licensed and made available under the terms and conditions of the BSD License
   which accompanies this distribution. The full text of the license may be found at
   http://opensource.org/licenses/bsd-license.php
   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __EDK_GENERIC_BDS_LIB_INTERNAL_H_
#define __EDK_GENERIC_BDS_LIB_INTERNAL_H_


//
// The package level header files this module uses
//
#include <PiDxe.h>
#include <WinNtDxe.h>
#include <Protocol/Cpu.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/FormBrowserFramework.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/LoadFile.h>
#include <Protocol/DevicePath.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/Performance.h>
#include <Protocol/WinNtIo.h>
#include <Guid/PcAnsi.h>
#include <Guid/GlobalVariable.h>
#include <Guid/GenericPlatformVariable.h>
#include <Guid/ShellFile.h>
#include <Library/EdkGenericBdsLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PcdLib.h>


#define PERF_TOKEN_LENGTH       28
#define PERF_PEI_ENTRY_MAX_NUM  50

typedef struct {
  CHAR8   Token[PERF_TOKEN_LENGTH];
  UINT32  Duration;
} PERF_DATA;

typedef struct {
  UINT64        BootToOs;
  UINT64        S3Resume;
  UINT32        S3EntryNum;
  PERF_DATA     S3Entry[PERF_PEI_ENTRY_MAX_NUM];
  UINT64        CpuFreq;
  UINT64        BDSRaw;
  UINT32        Count;
  UINT32        Signiture;
} PERF_HEADER;

VOID
WriteBootToOsPerformanceData (
  VOID
  );

VOID
ClearDebugRegisters (
  VOID
  );

EFI_STATUS
EFIAPI
UpdateFvFileDevicePath (
  IN  OUT EFI_DEVICE_PATH_PROTOCOL      **DevicePath,
  IN  EFI_GUID                          *FileGuid
  );

#endif
