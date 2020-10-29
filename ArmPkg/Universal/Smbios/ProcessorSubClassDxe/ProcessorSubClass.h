/** @file
*
*  Copyright (c) 2015, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2015, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef PROCESSOR_SUBCLASS_DRIVER_H
#define PROCESSOR_SUBCLASS_DRIVER_H

#include <Uefi.h>
#include <Protocol/Smbios.h>
#include <IndustryStandard/ArmStdSmc.h>
#include <IndustryStandard/SmBios.h>
#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmLib/ArmLibPrivate.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OemMiscLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

extern UINT8 ProcessorSubClassStrings[];

#define CPU_CACHE_L1_Instruction        0
#define CPU_CACHE_L1_Data               1
#define CPU_CACHE_L2                    2
#define CPU_CACHE_L3                    3
#define MAX_CACHE_LEVEL                 4

#endif // PROCESSOR_SUBCLASS_DRIVER_H
