/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  CK505ClockPlatformInfo.h

Abstract:

  Protocol to communicate product clock routing information.

GUID Info:
 {3C485EA4-449A-46ce-BB08-2A336EA96B4E}
 0x3c485ea4, 0x449a, 0x46ce, 0xbb, 0x8, 0x2a, 0x33, 0x6e, 0xa9, 0x6b, 0x4e);

**/

#ifndef _CLOCK_PLATFORM_INFO_H_
#define _CLOCK_PLATFORM_INFO_H_


#define EFI_CK505_CLOCK_PLATFORM_INFO_GUID  \
  {0x3c485ea4, 0x449a, 0x46ce, 0xbb, 0x8, 0x2a, 0x33, 0x6e, 0xa9, 0x6b, 0x4e}

//
// Structure to hold register modifications
//
typedef enum {
  None           = 0x00000000,
  nICS9LP505_1   = 0x00000001,
  nICS9LP505_2   = 0x00000002,
  nIDTCV163      = 0x00000004,
  nIDTCV174      = 0x00000008,
  nSLG505YC56    = 0x00000010,
  nSLG505YC64    = 0x00000020,
  nCY28505       = 0x00000040,
  nCY28505_2     = 0x00000080,
  nCY28505LF     = 0x00000100,
  nPI6C505_OLD   = 0x00000200,
  nPI6C505_RevD  = 0x00000400,
  nGENERIC_505   = 0x00000800,
  nSLG505YC264   = 0x00001000,
  nIDTCV183      = 0x00002000,
  nSLG505YC256   = 0x00004000,
  nIDTCV184      = 0x00008000,
  nIDTCV190      = 0x00010000,
  All            = 0xFFFFFFFF
} EFI_CLOCKS_SUPPORTED;

typedef enum {
  Disabled,
  Enabled,
  EnabledWithoutSwitch,
  EnabledWithSwitch
} EFI_SIGNAL_STATE;

typedef enum {
  SrcClk11,
  SrcClk10,
  SrcClk9,
  SrcClk8,
  SrcClk7,
  SrcClk6,
  SrcClk5,
  SrcClk4,
  SrcClk3,
  SrcClk2,
  SrcClk1,
  SrcClk0,
  CpuClk1,
  CpuClk0,
  Ref0,
  Dot96,
  Usb48,
  PciClkF5,
  PciClk4,
  PciClk3,
  PciClk2,
  PciClk1,
  PciClk0,
  SaveClockConfiguration,
  MePresent,
  Cr_A,
  Cr_B,
  Cr_C,
  Cr_D,
  Cr_E,
  Cr_F,
  Cr_G,
  Cr_H,
  Clk_None // Dummy entry for dynamic detection
} EFI_CLOCK_SIGNAL_NAME;

typedef struct {
  EFI_CLOCK_SIGNAL_NAME     Signal;
  EFI_SIGNAL_STATE          State;
  EFI_CLOCKS_SUPPORTED      Supported;
} EFI_STATIC_SIGNALS;

typedef struct {
  BOOLEAN               BehindBridge;
  UINT16                BridgeBus;
  UINT16                BridgeDev;
  UINT16                BridgeFunction;
  UINT16                TargetDevice;
  EFI_CLOCK_SIGNAL_NAME Signal;
} EFI_DYNAMIC_SIGNALS;


typedef struct {
  EFI_STATIC_SIGNALS      *StaticClockTable;
  UINTN                   StaticClockTableCount;
  EFI_STATIC_SIGNALS      *SxClockTable;
  UINTN                   SxClockTableCount;
  EFI_STATIC_SIGNALS	  *DynamicDisabledClocksTable;
  UINTN			  DynamicDisabledClocksTableCount;
} EFI_CLOCK_PLATFORM_INFO;

extern EFI_GUID gEfiCk505ClockPlatformInfoGuid;

#endif
