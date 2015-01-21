/*++

Copyright (c) 2012  - 2014, Intel Corporation.  All rights reserved.
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   



Module Name:

  MiscOemType0x94Data.c

Abstract:

  This file contains the Misc version Data (SMBIOS data type 0x94)

--*/

#include "CommonHeader.h"

#include "MiscSubclassDriver.h"


//
// Static (possibly build generated) Oem data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_OEM_TYPE_0x94, MiscOemType0x94) = {

STRING_TOKEN (STR_MISC_GOP_VERSION),
STRING_TOKEN (STR_MISC_SEC_VERSION),
STRING_TOKEN (STR_MISC_MRC_VERSION_VALUE),
STRING_TOKEN (STR_MISC_UCODE_VERSION),
STRING_TOKEN (STR_MISC_PUNIT_FW_VALUE),
STRING_TOKEN (STR_MISC_PMC_FW_VALUE),
STRING_TOKEN (STR_MISC_ULPMC_FW_VALUE),
STRING_TOKEN (STR_MISC_SOC_VALUE),
STRING_TOKEN (STR_MISC_BOARD_ID_VALUE),
STRING_TOKEN (STR_MISC_FAB_ID_VALUE),
STRING_TOKEN (STR_MISC_CPU_FLAVOR_VALUE),
STRING_TOKEN (STR_MISC_BIOS_VERSION),
STRING_TOKEN (STR_MISC_PMIC_VERSION),
STRING_TOKEN (STR_MISC_TOUCH_VERSION),
STRING_TOKEN (STR_MISC_SECURE_BOOT),
STRING_TOKEN (STR_MISC_BOOT_MODE),
STRING_TOKEN (STR_MISC_SPEED_STEP),
STRING_TOKEN (STR_MISC_CPU_TURBO),
STRING_TOKEN (STR_MISC_CSTATE),
STRING_TOKEN (STR_MISC_GFX_TURBO),
STRING_TOKEN (STR_MISC_S0IX_VALUE),
STRING_TOKEN (STR_MISC_RC6_VALUE),

};
