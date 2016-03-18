/** @file
Common header file shared by all source files.

This file includes package header files, library classes and protocol, PPI & GUID definitions.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
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
#include <IntelQNCDxe.h>

//
// The protocols, PPI and GUID definitions for this module
//
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/LegacyRegion2.h>
#include <Protocol/SmbusHc.h>
#include <Protocol/QncS3Support.h>

//
// The Library classes this module consumes
//
#include <Library/BaseLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/MtrrLib.h>
#include <Library/IoLib.h>
#include <Library/SmbusLib.h>
#include <Library/S3IoLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/IntelQNCLib.h>
#include <Library/QNCAccessLib.h>
#include <AcpiCpuData.h>

extern EFI_HANDLE gQNCInitImageHandle;
extern QNC_DEVICE_ENABLES mQNCDeviceEnables;

#endif
