/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  PeiPostCode.c

Abstract:

  Worker functions for PostCode

--*/

#include "EfiStatusCode.h"

#pragma pack(1)
typedef struct {
  EFI_STATUS_CODE_VALUE   StatusValue;
  UINT8                   Port80Value;
} EFI_STATUS_CODE_TO_PORT_80;
#pragma pack()

//
// see Edk\Foundation\Library\EfiCommonLib\PostCode.c for DXE/BDS POST codes.
//
EFI_STATUS_CODE_TO_PORT_80  mPeiPort80Table[] = {
  //
  // Platform init
  //
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_PLATFORM_PEI_INIT,          0x11},
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_PLATFORM_PEI_STEP1,         0x12},
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_PLATFORM_PEI_STEP2,         0x13},
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_PLATFORM_PEI_STEP3,         0x14},
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_PLATFORM_PEI_STEP4,         0x15},

  //
  // SMBUS
  //
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_SMBUS_PEI_INIT,             0x16},
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_SMBUS_PEI_EXEC_ENTRY,       0x17},
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_SMBUS_PEI_EXEC_EXIT,        0x18},

  //
  // Clock
  //
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_CLOCK_PEI_INIT_ENTRY,       0x19},
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_CLOCK_PEI_INIT_EXIT,        0x1A},

  //
  // Over clocking support
  //
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_OVERCLOCK_PEI_INIT_ENTRY,   0x1B},
  {EFI_COMPUTING_UNIT_CHIPSET | EFI_CU_OVERCLOCK_PEI_INIT_EXIT,    0x1C},

  //
  // MRC
  //
  {EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_INIT_BEGIN,        0x21},
  {EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_SPD_READ,          0x23},
  {EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_PRESENCE_DETECT,   0x24},
  {EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_TIMING,            0x25},
  {EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_OPTIMIZING,        0x26},
  {EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_CONFIGURING,       0x27},
  {EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_TEST,              0x28},
  {EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_COMPLETE,          0x29},

  //
  // Platform Init after MRC
  //
  {EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_PROG_MTRR,         0x2A},
  {EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_PROG_MTRR_END,     0x2B},

  {EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_PC_RECOVERY_BEGIN,        0x31},
  {EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_PC_RECOVERY_AUTO,         0x32},
  {EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_PC_CAPSULE_LOAD,          0x33},
  {EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_PC_CAPSULE_START,         0x34},
  {EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_EC_NO_RECOVERY_CAPSULE,   0x35},

  {EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_PEI_INIT,      0x41},
  {EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_PEI_STEP1,     0x42},
  {EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_PEI_END,       0x43},
  {EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_SMM_PEI_INIT,  0x44},
  {EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_SMM_PEI_STEP1, 0x45},
  {EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_PC_SMM_PEI_END,   0x46}
};

BOOLEAN
PeiCodeTypeToPostCode (
  IN  EFI_STATUS_CODE_TYPE    CodeType,
  IN  EFI_STATUS_CODE_VALUE   Value,
  OUT UINT8                   *PostCode
  )
{
  UINTN             Index;

  if (CodeType == EFI_PROGRESS_CODE) {
    if ((Value == (EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT_BEGIN)) ||
        (Value == (EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT_END)) ||
        (Value == (EFI_SOFTWARE_DXE_CORE | EFI_SW_PC_INIT_BEGIN)) ||
        (Value == (EFI_SOFTWARE_DXE_CORE | EFI_SW_PC_INIT_END))) {
      return FALSE;
    }
  } else {
    return FALSE;
  }

  for (Index = 0; Index < sizeof(mPeiPort80Table)/sizeof(EFI_STATUS_CODE_TO_PORT_80); Index++) {
    if (mPeiPort80Table[Index].StatusValue == Value) {
      *PostCode = mPeiPort80Table[Index].Port80Value;
      return TRUE;
    }
  }

  return FALSE;
}
