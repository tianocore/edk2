/** @file
Header file for  QNC Smm Power Management driver

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMM_POWER_MANAGEMENT_H_
#define _SMM_POWER_MANAGEMENT_H_

#include <PiSmm.h>
#include <IntelQNCDxe.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/GlobalNvsArea.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/MpService.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/MemoryAllocationLib.h>

#include <IndustryStandard/Acpi.h>

#include <AcpiCpuData.h>

#include "Ppm.h"

//
// Module global variable
//
extern EFI_SMM_CPU_PROTOCOL                    *mSmmCpu;
extern EFI_GLOBAL_NVS_AREA                     *mGlobalNvsAreaPtr;
extern EFI_MP_SERVICES_PROTOCOL                *mMpService;

//
// Function prototypes
//

#endif
