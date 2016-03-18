/** @file
Header file for MemorySubClass Driver.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MEMORY_SUB_CLASS_H
#define _MEMORY_SUB_CLASS_H

//
// The package level header files this module uses
//
#include <FrameworkDxe.h>
//
// The protocols, PPI and GUID definitions for this module
//
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/SmbusHc.h>
#include <Guid/DataHubRecords.h>
#include <Guid/MemoryConfigData.h>
#include <Protocol/HiiDatabase.h>
#include <Guid/MdeModuleHii.h>

//
// The Library classes this module consumes
//
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciLib.h>
#include <Library/QNCAccessLib.h>

#include "QNCAccess.h"



//
// This is the generated header file which includes whatever needs to be exported (strings + IFR)
//

#define EFI_MEMORY_SUBCLASS_DRIVER_GUID \
  { 0xef17cee7, 0x267d, 0x4bfd, { 0xa2, 0x57, 0x4a, 0x6a, 0xb3, 0xee, 0x85, 0x91 }}

//
// Prototypes
//
EFI_STATUS
MemorySubClassEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

#endif
