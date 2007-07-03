/**@file
  Common header file shared by all source files.

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
   are licensed and made available under the terms and conditions of the BSD License
   which accompanies this distribution. The full text of the license may be found at
   http://opensource.org/licenses/bsd-license.php
   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __COMMON_HEADER_H_
#define __COMMON_HEADER_H_


//
// The package level header files this module uses
//
#include <PiDxe.h>

//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/FormCallbackFramework.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Protocol/DevicePath.h>
#include <Guid/BootState.h>
#include <Protocol/LoadFile.h>
#include <Protocol/CpuIo.h>
#include <Guid/FileInfo.h>
#include <Protocol/Bds.h>
#include <Protocol/DataHub.h>
#include <Protocol/FormBrowserFramework.h>
#include <Guid/Bmp.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/BlockIo.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/ConsoleControl.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/HiiFramework.h>
#include <Guid/GenericPlatformVariable.h>
#include <Protocol/SerialIo.h>
#include <Protocol/LegacyBios.h>
#include <Common/MaxBbsEntries.h>
#include <Guid/PcAnsi.h>
#include <Guid/DataHubRecords.h>
#include <Protocol/Performance.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/GraphicsLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PrintLib.h>
#include <Library/IfrSupportLibFramework.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/HobLib.h>
#include <Library/EdkGenericBdsLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLibFramework.h>
#include <Library/PeCoffLib.h>

//
// The rest of the includes
//
#include <Common/FirmwareRevision.h>

extern UINT8 PlatformBdsStrings[];

#endif
