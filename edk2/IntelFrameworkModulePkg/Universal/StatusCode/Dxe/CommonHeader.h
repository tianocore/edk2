/**@file
  Common header file shared by all source files.

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

  Copyright (c) 2006, Intel Corporation.
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
#include <FrameworkDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Guid/StatusCode.h>
#include <Protocol/DataHub.h>
#include <Protocol/SerialIo.h>
#include <Guid/MemoryStatusCodeRecord.h>
#include <Protocol/StatusCode.h>
#include <Guid/StatusCodeDataTypeId.h>
//
// The Library classes this module consumes
//
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/SerialPortLib.h>
#include <Library/OemHookStatusCodeLib.h>

//
// Declaration for callback Event.
//
VOID
EFIAPI
VirtualAddressChangeCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

//
// Declaration for original Entry Point.
//
EFI_STATUS
EFIAPI
DxeStatusCodeDriverEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

#endif
