/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  Gpio.h

Abstract:

EFI 2.0 PEIM to provide platform specific information to other
modules and to do some platform specific initialization.

--*/

#ifndef _PEI_GPIO_H
#define _PEI_GPIO_H

//#include "Efi.h"
//#include "EfiCommonLib.h"
//#include "Pei.h"
//#include "Numbers.h"

////
//// GPIO Register Settings for BeaverBridge (FFVS) (Cedarview/Tigerpoint)
////
//// Field Descriptions:
////    USE: Defines the pin's usage model:  GPIO (G) or Native (N) mode.
////    I/O: Defines whether GPIOs are inputs (I) or outputs (O).
////         (Note:  Only meaningful for pins used as GPIOs.)
////    LVL: This field gives you the initial value for "output" GPIO's.
////         (Note: The output level is dependent upon whether the pin is inverted.)
////    INV: Defines whether Input GPIOs activation level is inverted.
////         (Note:  Only affects the level sent to the GPE logic and does not
////         affect the level read through the GPIO registers.)
////
//// Notes:
////    1. BoardID is GPIO [8:38:34]
////
////Signal         UsedAs               USE     I/O      LVL     INV
////--------------------------------------------------------------------------
////GPIO0           Nonfunction       G     O            H       -
////GPIO1           SMC_RUNTIME_SCI#    G        I           -       I
////PIRQE#/GPIO2    Nonfunction G   O   H   -
////PIRQF#/GPIO3    Nonfunction G   O   H   -
////PIRQG#/GPIO4    Nonfunction G   O   H   -
////PIRQH#/GPIO5    Nonfunction G   O   H   -
////GPIO6   unused  G   O   L   -
////GPIO7   unused  G   O   L   -
////GPIO8          BOARD ID2    G   I   -   -
////GPIO9   unused  G   O   L   -
////GPIO10  SMC_EXTSMI# G   I   -   I
////GPIO11  Nonfunction G   O   H   -
////GPIO12  unused  G   O   L   -
////GPIO13  SMC_WAKE_SCI#   G   I   -   I
////GPIO14  unused  G   O   L   -
////GPIO15  unused  G   O   L   -
////GPIO16  PM_DPRSLPVR N   -   -   -
////GNT5#/GPIO17    GNT5#   N   -   -   -
////STPPCI#/GPIO18  PM_STPPCI#  N   -   -   -
////STPCPU#/GPIO20  PM_STPCPU#  N   -   -   -
////GPIO22  CRT_RefClk  G   I   -   -
////GPIO23  unused  G   O   L   -
////GPIO24  unused  G   O   L   -
////GPIO25  DMI strap   G   O   L   -
////GPIO26  unused  G   O   L   -
////GPIO27  unused  G   O   L   -
////GPIO28  RF_KILL#    G   O   H   -
////OC5#/GPIO29 OC  N   -   -   -
////OC6#/GPIO30 OC  N   -   -   -
////OC7#/GPIO31 OC  N   -   -   -
////CLKRUN#/GPIO32  PM_CLKRUN#  N   -   -   -
////GPIO33  NC  G   O   L   -
////GPIO34  BOARD ID0   G   I   -   -
////GPIO36  unused  G   O   L   -
////GPIO38  BOARD ID1   G   I   -   -
////GPIO39  unused  G   O   L   -
////GPIO48  unused  G   O   L   -
////CPUPWRGD/GPIO49 H_PWRGD N   -   -   -
//
//#define   GPIO_USE_SEL_VAL              0x1FC0FFFF       //GPIO1, 10, 13 is EC signal
//#define   GPIO_USE_SEL2_VAL             0x000100D6
//#define   GPIO_IO_SEL_VAL               0x00402502
//#define   GPIO_IO_SEL2_VAL              0x00000044
//#define   GPIO_LVL_VAL                  0x1800083D
//#define   GPIO_LVL2_VAL                 0x00000000
//#define   GPIO_INV_VAL                  0x00002402
//#define   GPIO_BLNK_VAL                 0x00000000
//#define   ICH_GPI_ROUTE (ICH_GPI_ROUTE_SCI(13) | ICH_GPI_ROUTE_SCI(1))

//
// GPIO Register Settings for CedarRock and CedarFalls platforms
//
//      GPIO Register Settings for NB10_CRB
//---------------------------------------------------------------------------------
//Signal        Used As         USE         I/O     LVL
//---------------------------------------------------------------------------------
//
// GPIO0    FP_AUDIO_DETECT     G       I
// GPIO1    SMC_RUNTIME_SCI#    G       I
// GPIO2        INT_PIRQE_N     N       I
// GPIO3    INT_PIRQF_N     N       I
// GPIO4        INT_PIRQG_N     N       I
// GPIO5        INT_PIRQH_N     N       I
// GPIO6
// GPIO7
// GPIO8
// GPIO9    LPC_SIO_PME     G       I
// GPIO10   SMC_EXTSMI_N        G       I
// GPIO11   SMBALERT- pullup    N
// GPIO12   ICH_GP12        G       I
// GPIO13   SMC_WAKE_SCI_N      G       I
// GPIO14   LCD_PID0        G       O       H
// GPIO15   CONFIG_MODE_N       G       I
// GPIO16       PM_DPRSLPVR     N
// GPIO17   SPI_SELECT_STRAP1
//          /L_BKLTSEL0_N   G       I
// GPIO18   PM_STPPCI_N     N
// GPIO19
// GPIO20   PM_STPCPU_N     N
// GPIO21
// GPIO22   REQ4B           G       I
// GPIO23   L_DRQ1_N        N
// GPIO24   CRB_SV_DET_N        G       O       H
// GPIO25   DMI strap
//          / L_BKLTSEL1_N  G       O       H
// GPIO26   LCD_PID1        G       O       H
// GPIO27   TPEV_DDR3L_DETECT   G       O       H
// GPIO28   RF_KILL         G       O       H:enable
// GPIO29   OC          N
// GPIO30   OC          N
// GPIO31   OC          N
// GPIO32   PM_CLKRUN_N     Native
// GPIO33   MFG_MODE_N      G       I
// GPIO34   BOARD ID0       G       I
// GPIO35
// GPIO36   SV_SET_UP       G       O       H
// GPIO37
// GPIO38   BOARD ID1       G       I
// GPIO39   BOARD ID2       G       I
// GPIO48   FLASH_SEL0      N
// GPIO49   H_PWRGD         N

#define ICH_GPI_ROUTE_SMI(Gpio)          ((( 0 << ((Gpio * 2) + 1)) | (1 << (Gpio * 2))))
#define ICH_GPI_ROUTE_SCI(Gpio)          ((( 1 << ((Gpio * 2) + 1)) | (0 << (Gpio * 2))))

#define   GPIO_USE_SEL_VAL              0X1F42F7C3
#define   GPIO_USE_SEL2_VAL             0X000000D6
#define   GPIO_IO_SEL_VAL               0X1042B73F
#define   GPIO_IO_SEL2_VAL              0X000100C6
#define   GPIO_LVL_VAL                  0X1F15F601
#define   GPIO_LVL2_VAL                 0X000200D7
#define   GPIO_INV_VAL                  0x00002602
#define   GPIO_BLNK_VAL                 0x00040000
#define   ICH_GPI_ROUTE (ICH_GPI_ROUTE_SCI(13) | ICH_GPI_ROUTE_SCI(1))

#endif
