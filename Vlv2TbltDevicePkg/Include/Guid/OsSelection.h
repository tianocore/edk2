/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  OsSelection.h

Abstract:

  GUID used for LPSS, SCC and LPE configuration data entries in the HOB list.

--*/

#ifndef _OS_SELECTION_GUID_H_
#define _OS_SELECTION_GUID_H_

#ifndef ECP_FLAG
#include <PiPei.h>

#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#endif

#define ANDROID 1

#define EFI_OS_SELECTION_VARIABLE_GUID \
  { \
    0x86843f56, 0x675d, 0x40a5, 0x95, 0x30, 0xbc, 0x85, 0x83, 0x72, 0xf1, 0x03 \
  }

extern EFI_GUID gOsSelectionVariableGuid;

#pragma pack(1)

typedef struct {
  UINT8           LpssPciModeEnabled;
  //SCC
  UINT8           LpsseMMCEnabled;
  UINT8           LpssSdioEnabled;
  UINT8           LpssSdcardEnabled;
  UINT8           LpssSdCardSDR25Enabled;
  UINT8           LpssSdCardDDR50Enabled;
  UINT8           LpssMipiHsi;
  UINT8           LpsseMMC45Enabled;
  UINT8           LpsseMMC45DDR50Enabled;
  UINT8           LpsseMMC45HS200Enabled;
  UINT8           LpsseMMC45RetuneTimerValue;
  UINT8           eMMCBootMode;
  //LPSS2
  UINT8           LpssDma1Enabled;
  UINT8           LpssI2C0Enabled;
  UINT8           LpssI2C1Enabled;
  UINT8           LpssI2C2Enabled;
  UINT8           LpssI2C3Enabled;
  UINT8           LpssI2C4Enabled;
  UINT8           LpssI2C5Enabled;
  UINT8           LpssI2C6Enabled;
  //LPSS1
  UINT8           LpssDma0Enabled;
  UINT8           LpssPwm0Enabled;
  UINT8           LpssPwm1Enabled;
  UINT8           LpssHsuart0Enabled;
  UINT8           LpssHsuart1Enabled;
  UINT8           LpssSpiEnabled;
  UINT8           I2CTouchAd;
} EFI_PLATFORM_LPSS_DATA;

typedef struct _EFI_OS_SELECTION_HOB {
  UINT8                       OsSelection;
  UINT8                       OsSelectionChanged;
  UINT8                       Lpe;
  UINT8                       PchAzalia;
  EFI_PLATFORM_LPSS_DATA      LpssData;
} EFI_OS_SELECTION_HOB;

#pragma pack()

#endif
