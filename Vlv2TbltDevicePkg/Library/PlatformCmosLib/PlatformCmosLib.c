/*++

Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  CmosTable.h

Abstract:

--*/

#include <Base.h>
#include <Library/IoLib.h>
#include <Library/PlatformCmosLib.h>
#include "CmosMap.h"
#include <PchAccess.h>
#include "PlatformBaseAddresses.h"

#define DEFAULT_VALUE          0
#define  DEFAULT_ATTRIBUTES     0
#define  EXCLUDE_FROM_CHECKSUM   CMOS_ATTRIBUTE_EXCLUDE_FROM_CHECKSUM

#define CMOS_DEBUG_PRINT_LEVEL_DEFAULT_VALUE      0x46   // EFI_D_WARN|EFI_D_INFO|EFI_D_LOAD
#define CMOS_DEBUG_PRINT_LEVEL_3_DEFAULT_VALUE    0x80   // EFI_D_ERROR

//
// Add the CMOS entry below
//
CMOS_ENTRY mCmosTable[] = {
{ CPU_HT_POLICY, CPU_HT_POLICY_ENABLED, EXCLUDE_FROM_CHECKSUM },
{ TPM_POLICY, TPM_POLICY_ENABLED, DEFAULT_ATTRIBUTES },
{ CMOS_LCDPANELTYPE_REG, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_LCDPANELSCALING_REG, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_IGDBOOTTYPE_REG, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_BACKLIGHT_REG, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_LFP_PANEL_COLOR_DEPTH_REG, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_EDP_ACTIVE_LFP_CONFIG_REG, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_PRIMARY_DISPLAY_REG, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_IGD_DISPLAY_PIPE_B_REG, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_SDVOPANELTYPE_REG, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_PLATFORM_RESET_OS, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_CPU_BSP_SELECT, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_CPU_RATIO_OFFSET, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_ICH_PORT80_OFFSET, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ CMOS_MAXRATIO_CONFIG_REG, DEFAULT_VALUE, DEFAULT_ATTRIBUTES },
{ RTC_ADDRESS_CENTURY, RTC_ADDRESS_CENTURY_DEFAULT, CMOS_ATTRIBUTE_EXCLUDE_FROM_CHECKSUM },
{ CMOS_POST_CODE_BREAK_REG, DEFAULT_VALUE, EXCLUDE_FROM_CHECKSUM },
{ CMOS_POST_CODE_BREAK_1_REG, DEFAULT_VALUE, EXCLUDE_FROM_CHECKSUM },
{ CMOS_POST_CODE_BREAK_2_REG, DEFAULT_VALUE, EXCLUDE_FROM_CHECKSUM },
{ CMOS_POST_CODE_BREAK_3_REG, DEFAULT_VALUE, EXCLUDE_FROM_CHECKSUM },
{ CMOS_DEBUG_PRINT_LEVEL_REG, CMOS_DEBUG_PRINT_LEVEL_DEFAULT_VALUE, EXCLUDE_FROM_CHECKSUM },
{ CMOS_DEBUG_PRINT_LEVEL_1_REG, DEFAULT_VALUE, EXCLUDE_FROM_CHECKSUM },
{ CMOS_DEBUG_PRINT_LEVEL_2_REG, DEFAULT_VALUE, EXCLUDE_FROM_CHECKSUM },
{ CMOS_DEBUG_PRINT_LEVEL_3_REG, CMOS_DEBUG_PRINT_LEVEL_3_DEFAULT_VALUE, EXCLUDE_FROM_CHECKSUM },
};

/**
  Funtion to return platform CMOS entry.

  @param [out]  CmosEntry  Platform CMOS entry.

  @param [out]  CmosEntryCount Number of platform CMOS entry.

  @return Status.
**/
RETURN_STATUS
EFIAPI
GetPlatformCmosEntry (
  OUT CMOS_ENTRY  **CmosEntry,
  OUT UINTN       *CmosEntryCount
  )
{
  *CmosEntry = mCmosTable;
  *CmosEntryCount = sizeof(mCmosTable)/sizeof(mCmosTable[0]);
  return RETURN_SUCCESS;
}

/**
  Function to check if Battery lost or CMOS cleared.

  @reval TRUE  Battery is always present.
  @reval FALSE CMOS is cleared.
**/
BOOLEAN
EFIAPI
CheckCmosBatteryStatus (
  VOID
  )
{
  //
  // Check if the CMOS battery is present
  // Checks RTC_PWR_STS bit in the GEN_PMCON_1 register
  //

  if ((MmioRead8 (PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1) & B_PCH_PMC_GEN_PMCON_RTC_PWR_STS) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}
