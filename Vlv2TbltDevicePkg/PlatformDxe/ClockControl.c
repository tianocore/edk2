/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  ClockControl.c

Abstract:

  Sets platform/SKU specific clock routing information.



--*/

#include "PlatformDxe.h"
#include <Protocol/CK505ClockPlatformInfo.h>

//
// Default clock routing informtion (All On)
//
EFI_CLOCK_PLATFORM_INFO mDefClockPolicy = {NULL, 0, NULL, 0, NULL, 0};

//
// Clock Settings
//
// Static clock table.
//  This should be used to define any clock settings that are static
//  (Always On or Always Off).  Dynamic clocks should be set to enabled
//  in this table.
//
EFI_STATIC_SIGNALS mAtxStaticClocks[] = {
  {SrcClk8,   Enabled,  All},
  {SrcClk7,   Enabled,  All},
  {SrcClk6,   Enabled,  All},
  {SrcClk5,   Enabled,  All},
  {SrcClk4,   Enabled,  All},
  {SrcClk3,   Enabled,  All},
  {SrcClk2,   Enabled,  All},
  {SrcClk1,   Enabled,  All},
  {SrcClk0,   Enabled,  All},
  {Ref0,      Enabled,  All},
  {Dot96,     Enabled,  All},
  {Usb48,     Enabled,  All},
  {PciClkF5,  Enabled,  All},
  {PciClk0,   Enabled,  All},
  {PciClk2,   Enabled,  All},
  {PciClk3,   Enabled,  All},
  {PciClk4,   Disabled,  All},
  {Cr_B,   EnabledWithSwitch,  All},
};

//
// ClockSxInfo Table
//  This is a list of clocks that need to be set to a known state when the
//  system enters S4 or S5.
//
EFI_STATIC_SIGNALS mAtxSxClocks[] = {
  {SaveClockConfiguration, Disabled, All}
};

//
// ATX settings structure
//
EFI_CLOCK_PLATFORM_INFO mAtxClockSettings = {
  mAtxStaticClocks,
  sizeof(mAtxStaticClocks) / sizeof(mAtxStaticClocks[0]),
  mAtxSxClocks,
  sizeof(mAtxSxClocks) / sizeof(mAtxSxClocks[0])
};

#if defined( RVP_SUPPORT ) && RVP_SUPPORT
//
// RVP Clock Settings
//
// Static clock table.
//  This should be used to define any clock settings that are static
//  (Always On or Always Off).  Dynamic clocks should be set to enabled
//  in this table.
//
//UPSD_TBD Check with Jan if any porting required.
//
EFI_STATIC_SIGNALS mRvpStaticClocks[] = {
  {SrcClk11,  Enabled,  All},     // Not used/not present but leave coding enabled
  {SrcClk10,  Enabled,  All},     // Not used/not present but leave coding enabled
  {SrcClk9,   Enabled,  All},     // Not used/not present but leave coding enabled
  {SrcClk8,   Enabled,  All},     // ICHSATAII
  {SrcClk7,   Enabled,  All},     // DPL_REFSSCLKIN
  {SrcClk6,   Enabled,  All},     // 100M_MCH
  {SrcClk5,   Enabled,  All},     // Mini-PCIe  //TODO PNV: Need to check ICH GPIO38:
                                                // 0: turn on; 1: turn off
  {SrcClk4,   Enabled,  All},     // ICHSATA
  {SrcClk3,   Enabled,  All},     // 100M_ICH
  {SrcClk2,   Enabled,  All},     // 100M_LAN
  {SrcClk1,   Enabled,  All},     // 25M_LAN
  {SrcClk0,   Enabled,  All},     // 96M_DREF
  {Ref0,      Enabled,  All},
  {Dot96,     Enabled,  All},
  {Usb48,     Enabled,  All},
  {PciClkF5,  Enabled,  All},     // 33M_ICH
  {PciClk0,   Enabled,  All},     // 33M_RISER
  {PciClk1,   Enabled,  All},     // 33M_RISER
  {PciClk2,   Enabled,  All},     // VDD_Clock
  {PciClk3,   Enabled,  All},     // 33M_S1
  {PciClk4,   Enabled,  All},     // 33M_PA
};

//
// Dynamic clock table
// This is used to determine if a clock should be left on or turned off based
// on the presence of a device.  The bridge information is used so the bus
// number for the device to be detected can be found.
//

//
// ClockSxInfo Table
// This is a list of clocks that need to be set to a known state when the
// system enters S4 or S5.
//
EFI_STATIC_SIGNALS mRvpSxClocks[] = {
  {SaveClockConfiguration, Disabled, All}
};

//
// RVP settings structure
//
EFI_CLOCK_PLATFORM_INFO mRvpClockSettings = {
  mRvpStaticClocks,
  sizeof(mRvpStaticClocks) / sizeof(mRvpStaticClocks[0]),
  0,  // No clocks will be turned off mRvpDynamicClocks,
  0, // No clocks will be turned off sizeof(mRvpDynamicClocks) / sizeof(mRvpDynamicClocks[0]),
  mRvpSxClocks,
  sizeof(mRvpSxClocks) / sizeof(mRvpSxClocks[0])
};
#endif

VOID
InitializeClockRouting(
  )
{
  EFI_STATUS                      Status;
  UINTN                           BoardIdVarSize;
  EFI_BOARD_FEATURES              BoardIdVar;
  EFI_CLOCK_PLATFORM_INFO         *ClockPolicy;
  EFI_HANDLE                      Handle;

  ClockPolicy = &mDefClockPolicy;

  //
  // Do modifications based on board type
  //
  BoardIdVarSize = sizeof (EFI_BOARD_FEATURES);
  Status = gRT->GetVariable (
                  BOARD_FEATURES_NAME,
                  &gEfiBoardFeaturesGuid,
                  NULL,
                  &BoardIdVarSize,
                  &BoardIdVar
                  );
  if (!EFI_ERROR (Status)) {

#if defined( RVP_SUPPORT ) && RVP_SUPPORT
    if (BoardIdVar & B_BOARD_FEATURES_RVP) {
      ClockPolicy = &mRvpClockSettings;
    }
#else

    //
    // Isolate board type information
    //
    BoardIdVar = BoardIdVar & (B_BOARD_FEATURES_FORM_FACTOR_ATX |
                               B_BOARD_FEATURES_FORM_FACTOR_BTX |
                               B_BOARD_FEATURES_FORM_FACTOR_MICRO_ATX |
                               B_BOARD_FEATURES_FORM_FACTOR_MICRO_BTX);

    if (BoardIdVar == B_BOARD_FEATURES_FORM_FACTOR_ATX ||
        BoardIdVar == B_BOARD_FEATURES_FORM_FACTOR_MICRO_ATX) {
      ClockPolicy = &mAtxClockSettings;
    }

#endif

  }

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiCk505ClockPlatformInfoGuid,
                  EFI_NATIVE_INTERFACE,
                  ClockPolicy
                  );
  ASSERT_EFI_ERROR(Status);

}


