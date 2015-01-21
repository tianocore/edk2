/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  PlatformEarlyInit.h

Abstract:

  Platform Early Stage header file



--*/

/*++
 This file contains an 'Intel Peripheral Driver' and is
 licensed for Intel CPUs and chipsets under the terms of your
 license agreement with Intel or your vendor.  This file may
 be modified by the user, subject to additional terms of the
 license agreement
--*/

#ifndef _EFI_PLATFORM_EARLY_INIT_H_
#define _EFI_PLATFORM_EARLY_INIT_H_

#define EFI_FORWARD_DECLARATION(x) typedef struct _##x x
#include <FrameworkPei.h>
#include "PlatformBaseAddresses.h"
#include "PchAccess.h"
#include "VlvAccess.h"
#include "SetupMode.h"
#include "PlatformBootMode.h"
#include "Platform.h"
#include "LegacySpeaker.h"

#include <Ppi/Stall.h>
#include <Guid/PlatformInfo.h>
#include <Guid/SetupVariable.h>
#include <Ppi/AtaController.h>
#include <Ppi/FindFv.h>
#include <Ppi/BootInRecoveryMode.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/Capsule.h>
#include <Guid/EfiVpdData.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MtrrLib.h>
#include <Library/CpuIA32.h>

#include <IndustryStandard/Pci22.h>
#include <Ppi/Speaker.h>
#include <Guid/FirmwareFileSystem.h>
#include <Guid/MemoryTypeInformation.h>
#include <Ppi/Cache.h>
#include <Ppi/Smbus.h>
#include <Library/PchPlatformLib.h>
#include <Ppi/SmbusPolicy.h>
#include <Ppi/Reset.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/VlvPolicy.h>
#include <Guid/GlobalVariable.h>
#include <Ppi/RecoveryModule.h>
#include <Ppi/DeviceRecoveryModule.h>
#include <Guid/Capsule.h>
#include <Guid/RecoveryDevice.h>
#include <Ppi/MasterBootMode.h>
#include <Guid/PlatformCpuInfo.h>
#include <Guid/OsSelection.h>

#define SMC_LAN_ON       0x46
#define SMC_LAN_OFF    0x47
#define SMC_DEEP_S3_STS    0xB2




//
// Wake Event Types
//
#define SMBIOS_WAKEUP_TYPE_RESERVED           0x00
#define SMBIOS_WAKEUP_TYPE_OTHERS             0x01
#define SMBIOS_WAKEUP_TYPE_UNKNOWN            0x02
#define SMBIOS_WAKEUP_TYPE_APM_TIMER          0x03
#define SMBIOS_WAKEUP_TYPE_MODEM_RING         0x04
#define SMBIOS_WAKEUP_TYPE_LAN_REMOTE         0x05
#define SMBIOS_WAKEUP_TYPE_POWER_SWITCH       0x06
#define SMBIOS_WAKEUP_TYPE_PCI_PME            0x07
#define SMBIOS_WAKEUP_TYPE_AC_POWER_RESTORED  0x08

#define EFI_CPUID_VIRT_PHYS_ADDRESS_SIZE       0x80000008

//
// Defines for stall ppi
//
#define PEI_STALL_RESOLUTION  1

//
// Used in PEI memory test routines
//
#define MEMORY_TEST_COVER_SPAN  0x40000
#define MEMORY_TEST_PATTERN     0x5A5A5A5A

#define EFI_LOW_BEEP_FREQUENCY            0x31B
#define EFI_HIGH_BEEP_FREQUENCY           0x254

//
// General Purpose Constants
//
#define ICH_ACPI_TIMER_MAX_VALUE  0x1000000 //The timer is 24 bit overflow



//
//
//              GPIO Register Settings for ValleyFalls (Tablet)
//
//
//                   IO Space configyuration registers
// Field Descriptions:
//    USE: Defines the pin's usage model:  GPIO (G) or Native (N) mode.
//    I/O: Defines whether GPIOs are inputs (I) or outputs (O).
//         (Note:  Only meaningful for pins used as GPIOs.)
//    LVL: This field gives you the initial value for "output" GPIO's.
//         (Note: The output level is dependent upon whether the pin is inverted.)
//    TPE: Defines whether Trigger Positive Edge Enable.
//    TNE: Defines whether Trigger Negative Edge Enable.
//    WAKE_EN: only support in SUS community
//         (Note:  Only affects the level sent to the GPE logic and does not
//         affect the level read through the GPIO registers.)
//
//
//                    Memory spcae configuration registers
//
// Field Descriptions:
//   PAD releated:
//    PAD_CONF0
//      PAD_CONF1
//      PAD_VAL
//      PAD_DFT
//
// Notes:
//    1. N = Native , G = GPIO , I = Input, O = Output, - = BOTH/NOT SURE
//
// Signal          UsedAs                       USE     I/O      LVL     TPE     TNE   PCONF0   PCONF1   PVAL  PDFT
// -------------------------------------------------------------------------------------------------------------------------
// GPIO0           UART1_RXD-L                   N       I        -       -       -     cd29h    -         -     -
// GPIO1           UART1_TXD-0                   N       O        -       -       -     cd29h    -         -     -
// *GPIO2           UART1_RTS_B-1                N       I        -       -       -     cca9h    -         -     -
// *GPIO3           UART1_CTS_B-H                N       O        -       -       -     cca9h    -         -     -

// GPIO4           I2C1_SDA-OD-O                 N       -        -       -       -     cca9h    -         -     -
// GPIO5           I2C1_SCL-OD-O                 N       -        -       -       -     cca9h    -         -     -
// GPIO6           I2S_SYSCLK-0                  N       O        -       -       -     8d51h    -         -     -
// GPIO7           I2S_L_R-0 (SP)                N       O        -       -       -     8cd1h    -         -     -
// GPIO8           I2S_DATA_OUT-0                N       O        -       -       -     8cd1h    -         -     -
// GPIO9           I2S_SDATA_IN-L                N       I        -       -       -     8cd1h    -         -     -

// GPIO10          PCM_CLK-0                     N       O        -       -       -     8d51h    -         -     -
// GPIO11          PCM_FSYNC-0 (SP)              N       O        -       -       -     8cd1h    -         -     -
// GPIO12          PCM_DATA_OUT-0 (SP)           N       O        -       -       -     8cd1h    -         -     -
// GPIO13          PCM_DATA_IN-L                 N       I        -       -       -     8d51h    -         -     -

// GPIO14          SATA_GP0                      N       -        -       -       -      -       -         -     -
// GPIO15          I2C2_SDA-OD-O/I               N       -        -       -       -     ccaah    -         -     -

// GPIO16          SATA_LEDN                     N       O        -       -       -      -       -         -     -
// GPIO17          UART2_RTS_B-1                 N       I        -       -       -     cd2ah    -         -     -
// GPIO18          UART2_CTS_B-H                 N       O        -       -       -     ccaah    -         -     -
// GPIO19          UART2_RXD-H                   N       I        -       -       -     ccaah    -         -     -

// GPIO20          I2C2_SCL-OD-O/I               N       -        -       -       -     ccaah    -         -     -
// GPIO21          **PCIE_CLKREQ4B               N       -        -       -       -      -       -         -     -
// GPIO22          UART2_TXD-0                   N       O        -       -       -     ccaah    -         -     -
// GPIO23          FLEX_CLK_SE1                  N       -        -       -       -      -       -         -     -

// GPIO24          SPI0_SCK-0                    N       O        -       -       -     8d02h    -         -     -
// GPIO25          SPI0_CS-1                     N       O        -       -       -     8d02h    -         -     -
// GPIO26          SPI0_MOSI-0                   N       O        -       -       -     8d02h    -         -     -
// GPIO27          SPI0_MISO-L                   N       I        -       -       -     8d02h    -         -     -

// GPIO28          UART3_RXD-L                   N       I        -       -       -      -       -         -     -
// GPIO29          UART3_TXD-0                   N       O        -       -       -      -       -         -     -
// GPIO30          UART4_RXD-L                   N       I        -       -       -      -       -         -     -
// GPIO31          UART4_TXD-0                   N       O        -       -       -      -       -         -     -

// GPIO32          SDMMC1_CLK                    N       -        -       -       -     208d51h   -         -     -
// GPIO33          SDMMC1_D0                     N       -        -       -       -     8cd1h     -         -     -
// GPIO34          SDMMC1_D1                     N       -        -       -       -     8cd1h     -         -     -
// GPIO35          SDMMC1_D2                     N       -        -       -       -     8cd1h     -         -     -
// GPIO36          SDMMC1_D3_CD_B                N       -        -       -       -     8cd1h     -         -     -
// GPIO37          MMC1_D4_SD_WE                 N       -        -       -       -     8cd1h     -         -     -
// GPIO38          MMC1_D5                       N       -        -       -       -     8cd1h     -         -     -
// GPIO39          MMC1_D6                       N       -        -       -       -     8cd1h     -         -     -
// GPIO40          MMC1_D7                       N       -        -       -       -     8cd1h     -         -     -
// GPIO41          SDMMC1_CMD                    N       -        -       -       -     8cd1h     -         -     -
// GPIO42          MMC1_RESET_B                  N       -        -       -       -     208d51h   -         -     -

// GPIO43          SDMMC2_CLK                    N       -        -       -       -     208d51h   -         -     -
// GPIO44          SDMMC2_D0                     N       -        -       -       -     8cd1h     -         -     -
// GPIO45          SDMMC2_D1                     N       -        -       -       -     8cd1h     -         -     -
// GPIO46          SDMMC2_D2                     N       -        -       -       -     8cd1h     -         -     -
// GPIO47          SDMMC2_D3_CD_B                N       -        -       -       -     8cd1h     -         -     -
// GPIO48          SDMMC2_CMD                    N       -        -       -       -     8cd1h     -         -     -

// GPIO49          SDMMC3_CLK                    N       -        -       -       -     8d51h      -         -     -
// GPIO50          SDMMC3_D0                     N       -        -       -       -     8cd1h      -         -     -
// GPIO51          SDMMC3_D1                     N       -        -       -       -      8cd1h      -         -     -
// GPIO52          SDMMC3_D2                     N       -        -       -       -      8cd1h      -         -     -
// GPIO53          SDMMC3_D3                     N       -        -       -       -      8cd1h      -         -     -
// GPIO54          SDMMC3_CD_B                   N       -        -       -       -      cca9h      -         -     -
// GPIO55          SDMMC3_CMD                    N       -        -       -       -      8cd1h      -         -     -
// GPIO56          SDMMC3_1P8_EN                 N       -        -       -       -      cd29h   -         -     -

// GPIO57            LPC_AD0                     N       -        -       -       -      -       -         -     -
// GPIO58            LPC_AD1                     N       -        -       -       -      -       -         -     -
// GPIO59            LPC_AD2                     N       -        -       -       -      -       -         -     -
// GPIO60            LPC_AD3                     N       -        -       -       -      -       -         -     -
// GPIO61            LPC_FRAMEB                  N       O        -       -       -      -       -         -     -
// GPIO62            LPC_CLKOUT0                 N       O        -       -       -      -       -         -     -
// GPIO63            LPC_CLKOUT1                 N       O        -       -       -      -       -         -     -
// GPIO64            LPC_CLKRUNB                 N       -        -       -       -      -       -         -     -

// GPIO65            SMB_DATA                    N       -        -       -       -      -       -         -     -
// GPIO66            SMB_CLK                     N       -        -       -       -      -       -         -     -
// GPIO67            SMB_ALERTB                  N       -        -       -       -      -       -         -     -

// GPIO68            ILB_SEIRQ                   N       -       -        -       -      -       -         -     -
// GPIO69            SPKR                        N       O       -        -       -      -       -         -     -

//SUS WELL

//GPIO_SUS0             BT_WAKEUP_VLV               N       O       -        -       -    CCA8h       -         -     -
//GPIO_SUS1             BT_CLOCK_REQ                N       O       -        -       -    CCA8h       -         -     -
//GPIO_SUS2             WIFI_PWR_EN                 N       O       -        -       -    CCA8h       -         -     -
//GPIO_SUS3             SD_CARD_PWR_EN              N       O       -        -       -    CD28h       -         -     -
//GPIO_SUS4             GPIO_SUS4                   N       O       -        -       -    CD28h       -         -     -
//GPIO_SUS5             GPIO_SUS5                   N       O       -        -       -    CD28h       -         -     -
//GPIO_SUS6             SUSPWRDNACK                 N       O       -        -       -    8850h       -         -     -
//GPIO_SUS7             PMU_SLP_DDRVTT_B            N       O       -        -       -    8850h       -         -     -
//GPIO_SUS8             PMU_WAKE_B                  N       O       -        -       -    CCA8h       -         -     -
//GPIO_SUS9             PMU_PWRBTN_B                N       O       -        -       -    CCA8h       -         -     -
//GPIO_SUS10            PMU_WAKE_LAN_B              N       O       -        -       -    CCA8h       -         -     -
//GPIO_SUS11            SUS_STAT_B                  N       O       -        -       -    C828h       -         -     -
//GPIO_SUS12            GPIO_SUS12                  N       O       -        -       -    C828h      -         -     -
//GPIO_SUS13            USB_OC0_B-20K,H             N       O       -        -       -    CCA8h       -         -     -
//GPIO_SUS14            GPIO_SUS14                  N       O       -        -       -    CCA8h       -         -     -
//GPIO_SUS15            SPI_CS1_B-20K,H             N       O       -        -       -    8C80h       -         -     -
//GPIO_SUS16            PMU_SUSCLK                  N       O       -        -       -    C828h       -         -     -
//


#define VF_TAB_GPIO_USE_SEL_VAL_0_31        0x00000000
#define VF_TAB_GPIO_USE_SEL_VAL_32_63       0x00000000
#define VF_TAB_GPIO_USE_SEL_VAL_64_70       0x00000000
#define VF_TAB_GPIO_USE_SEL_VAL_SUS         0x00000000

//
//1010 --00 0100 01-- 0101 --0- 0001 1010
//
#define VF_TAB_GPIO_IO_SEL_VAL_0_31         0x00000000 // BIT30 | BIT28 | BIT27 | BIT19 | BIT17 | BIT13 | BIT9 | BIT2 | BIT0
#define VF_TAB_GPIO_IO_SEL_VAL_32_63        0x00000000
#define VF_TAB_GPIO_IO_SEL_VAL_64_70        0x00000000
#define VF_TAB_GPIO_IO_SEL_VAL_SUS          0x00000000


#define VF_TAB_GPIO_LVL_VAL_0_31            0x00000000
#define VF_TAB_GPIO_LVL_VAL_32_63           0x00000000
#define VF_TAB_GPIO_LVL_VAL_64_70           0x00000000
#define VF_TAB_GPIO_LVL_VAL_SUS             0x00000000

#define VF_TAB_GPIO_TPE_VAL_0_31            0x00000000
#define VF_TAB_GPIO_TPE_VAL_SUS             0x00000000

#define VF_TAB_GPIO_TNE_VAL_0_31            0x00000000
#define VF_TAB_GPIO_TNE_VAL_SUS             0x00000000

#define VF_TAB_GPIO_TS_VAL_0_31             0x00000000
#define VF_TAB_GPIO_TS_VAL_SUS              0x00000000


//
// Memory space registers
//

//
// CONF0
//
#define  VF_TAB_PAD_CONF0_GPIO0  0xcd29
#define  VF_TAB_PAD_CONF0_GPIO1  0xcd29
#define  VF_TAB_PAD_CONF0_GPIO2  0xcca9
#define  VF_TAB_PAD_CONF0_GPIO3  0xcca9
#define  VF_TAB_PAD_CONF0_GPIO4  0xcca9
#define  VF_TAB_PAD_CONF0_GPIO5  0xcca9
#define  VF_TAB_PAD_CONF0_GPIO6  0x8d51
#define  VF_TAB_PAD_CONF0_GPIO7  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO8  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO9  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO10  0x8d51
#define  VF_TAB_PAD_CONF0_GPIO11  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO12  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO13  0x8d51
#define  VF_TAB_PAD_CONF0_GPIO14  0xCCA8
#define  VF_TAB_PAD_CONF0_GPIO15  0xccaa
#define  VF_TAB_PAD_CONF0_GPIO16  0xC828
#define  VF_TAB_PAD_CONF0_GPIO17  0xcd2a
#define  VF_TAB_PAD_CONF0_GPIO18  0xccaa
#define  VF_TAB_PAD_CONF0_GPIO19  0xccaa
#define  VF_TAB_PAD_CONF0_GPIO20  0xccaa
#define  VF_TAB_PAD_CONF0_GPIO21  0xCCA9
#define  VF_TAB_PAD_CONF0_GPIO22  0xccaa
#define  VF_TAB_PAD_CONF0_GPIO23  0xCD2A
#define  VF_TAB_PAD_CONF0_GPIO24  0x8d02
#define  VF_TAB_PAD_CONF0_GPIO25  0x8d02
#define  VF_TAB_PAD_CONF0_GPIO26  0x8d02
#define  VF_TAB_PAD_CONF0_GPIO27  0x8d02
#define  VF_TAB_PAD_CONF0_GPIO28  0x8D02
#define  VF_TAB_PAD_CONF0_GPIO29  0x8D02
#define  VF_TAB_PAD_CONF0_GPIO30  0x8D00
#define  VF_TAB_PAD_CONF0_GPIO31  0xCD2A
#define  VF_TAB_PAD_CONF0_GPIO32  0x208d51
#define  VF_TAB_PAD_CONF0_GPIO33  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO34  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO35  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO36  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO37  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO38  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO39  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO40  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO41  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO42  0x208d51
#define  VF_TAB_PAD_CONF0_GPIO43  0x208d51
#define  VF_TAB_PAD_CONF0_GPIO44  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO45  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO46  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO47  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO48  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO49  0x8d51
#define  VF_TAB_PAD_CONF0_GPIO50  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO51  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO52  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO53  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO54  0xcca9
#define  VF_TAB_PAD_CONF0_GPIO55  0x8cd1
#define  VF_TAB_PAD_CONF0_GPIO56  0xcd29
#define  VF_TAB_PAD_CONF0_GPIO57  0x8C80
#define  VF_TAB_PAD_CONF0_GPIO58  0x8C80
#define  VF_TAB_PAD_CONF0_GPIO59  0x8C80
#define  VF_TAB_PAD_CONF0_GPIO60  0x8C80
#define  VF_TAB_PAD_CONF0_GPIO61  0x8800
#define  VF_TAB_PAD_CONF0_GPIO62  0x8D00
#define  VF_TAB_PAD_CONF0_GPIO63  0x8800
#define  VF_TAB_PAD_CONF0_GPIO64  0x8800
#define  VF_TAB_PAD_CONF0_GPIO65  0xC828
#define  VF_TAB_PAD_CONF0_GPIO66  0xC828
#define  VF_TAB_PAD_CONF0_GPIO67  0xC828
#define  VF_TAB_PAD_CONF0_GPIO68  0xCCA8
#define  VF_TAB_PAD_CONF0_GPIO69  0xC828
#define  VF_TAB_PAD_CONF0_GPIO70  0xCCA8



//
// PAD_CONF1
//
#define  VF_TAB_PAD_CONF1_GPIO0  0x20002
#define  VF_TAB_PAD_CONF1_GPIO1  0x20002
#define  VF_TAB_PAD_CONF1_GPIO2  0x20002
#define  VF_TAB_PAD_CONF1_GPIO3  0x20002
#define  VF_TAB_PAD_CONF1_GPIO4  0x20002
#define  VF_TAB_PAD_CONF1_GPIO5  0x20002
#define  VF_TAB_PAD_CONF1_GPIO6  0x1F000F
#define  VF_TAB_PAD_CONF1_GPIO7  0x1F000F
#define  VF_TAB_PAD_CONF1_GPIO8  0x1F000F
#define  VF_TAB_PAD_CONF1_GPIO9  0x1F000F
#define  VF_TAB_PAD_CONF1_GPIO10  0x1F000F
#define  VF_TAB_PAD_CONF1_GPIO11  0x1F000F
#define  VF_TAB_PAD_CONF1_GPIO12  0x1F000F
#define  VF_TAB_PAD_CONF1_GPIO13  0x1F000F
#define  VF_TAB_PAD_CONF1_GPIO14  0x20002
#define  VF_TAB_PAD_CONF1_GPIO15  0x20002
#define  VF_TAB_PAD_CONF1_GPIO16  0x20002
#define  VF_TAB_PAD_CONF1_GPIO17  0x20002
#define  VF_TAB_PAD_CONF1_GPIO18  0x20002
#define  VF_TAB_PAD_CONF1_GPIO19  0x20002
#define  VF_TAB_PAD_CONF1_GPIO20  0x20002
#define  VF_TAB_PAD_CONF1_GPIO21  0x20002
#define  VF_TAB_PAD_CONF1_GPIO22  0x20002
#define  VF_TAB_PAD_CONF1_GPIO23  0x20002
#define  VF_TAB_PAD_CONF1_GPIO24  0x00000
#define  VF_TAB_PAD_CONF1_GPIO25  0x00000
#define  VF_TAB_PAD_CONF1_GPIO26  0x00000
#define  VF_TAB_PAD_CONF1_GPIO27  0x00000
#define  VF_TAB_PAD_CONF1_GPIO28  0x00000
#define  VF_TAB_PAD_CONF1_GPIO29  0x00000
#define  VF_TAB_PAD_CONF1_GPIO30  0x00000
#define  VF_TAB_PAD_CONF1_GPIO31  0x20002
#define  VF_TAB_PAD_CONF1_GPIO32  0x00000
#define  VF_TAB_PAD_CONF1_GPIO33  0x00000
#define  VF_TAB_PAD_CONF1_GPIO34  0x00000
#define  VF_TAB_PAD_CONF1_GPIO35  0x00000
#define  VF_TAB_PAD_CONF1_GPIO36  0x00000
#define  VF_TAB_PAD_CONF1_GPIO37  0x00000
#define  VF_TAB_PAD_CONF1_GPIO38  0x00000
#define  VF_TAB_PAD_CONF1_GPIO39  0x00000
#define  VF_TAB_PAD_CONF1_GPIO40  0x00000
#define  VF_TAB_PAD_CONF1_GPIO41  0x00000
#define  VF_TAB_PAD_CONF1_GPIO42  0x00000
#define  VF_TAB_PAD_CONF1_GPIO43  0x00000
#define  VF_TAB_PAD_CONF1_GPIO44  0x00000
#define  VF_TAB_PAD_CONF1_GPIO45  0x00000
#define  VF_TAB_PAD_CONF1_GPIO46  0x00000
#define  VF_TAB_PAD_CONF1_GPIO47  0x00000
#define  VF_TAB_PAD_CONF1_GPIO48  0x00000
#define  VF_TAB_PAD_CONF1_GPIO49  0x00000
#define  VF_TAB_PAD_CONF1_GPIO50  0x00000
#define  VF_TAB_PAD_CONF1_GPIO51  0x00000
#define  VF_TAB_PAD_CONF1_GPIO52  0x00000
#define  VF_TAB_PAD_CONF1_GPIO53  0x00000
#define  VF_TAB_PAD_CONF1_GPIO54  0x20002
#define  VF_TAB_PAD_CONF1_GPIO55  0x00000
#define  VF_TAB_PAD_CONF1_GPIO56  0x20002
#define  VF_TAB_PAD_CONF1_GPIO57  0x00000
#define  VF_TAB_PAD_CONF1_GPIO58  0x00000
#define  VF_TAB_PAD_CONF1_GPIO59  0x00000
#define  VF_TAB_PAD_CONF1_GPIO60  0x00000
#define  VF_TAB_PAD_CONF1_GPIO61  0x00000
#define  VF_TAB_PAD_CONF1_GPIO62  0x00000
#define  VF_TAB_PAD_CONF1_GPIO63  0x00000
#define  VF_TAB_PAD_CONF1_GPIO64  0x00000
#define  VF_TAB_PAD_CONF1_GPIO65  0x20002
#define  VF_TAB_PAD_CONF1_GPIO66  0x20002
#define  VF_TAB_PAD_CONF1_GPIO67  0x20002
#define  VF_TAB_PAD_CONF1_GPIO68  0x20002
#define  VF_TAB_PAD_CONF1_GPIO69  0x20002
#define  VF_TAB_PAD_CONF1_GPIO70  0x20002


//
// PAD_VAL
//
#define  VF_TAB_PAD_VAL_GPIO0  0x2
#define  VF_TAB_PAD_VAL_GPIO1  0x2
#define  VF_TAB_PAD_VAL_GPIO2  0x2
#define  VF_TAB_PAD_VAL_GPIO3  0x2
#define  VF_TAB_PAD_VAL_GPIO4  0x2
#define  VF_TAB_PAD_VAL_GPIO5  0x2
#define  VF_TAB_PAD_VAL_GPIO6  0x2
#define  VF_TAB_PAD_VAL_GPIO7  0x2
#define  VF_TAB_PAD_VAL_GPIO8  0x2
#define  VF_TAB_PAD_VAL_GPIO9  0x2
#define  VF_TAB_PAD_VAL_GPIO10  0x2
#define  VF_TAB_PAD_VAL_GPIO11  0x2
#define  VF_TAB_PAD_VAL_GPIO12  0x2
#define  VF_TAB_PAD_VAL_GPIO13  0x2
#define  VF_TAB_PAD_VAL_GPIO14  0x2
#define  VF_TAB_PAD_VAL_GPIO15  0x2
#define  VF_TAB_PAD_VAL_GPIO16  0x4
#define  VF_TAB_PAD_VAL_GPIO17  0x2
#define  VF_TAB_PAD_VAL_GPIO18  0x2
#define  VF_TAB_PAD_VAL_GPIO19  0x2
#define  VF_TAB_PAD_VAL_GPIO20  0x2
#define  VF_TAB_PAD_VAL_GPIO21  0x2
#define  VF_TAB_PAD_VAL_GPIO22  0x2
#define  VF_TAB_PAD_VAL_GPIO23  0x2
#define  VF_TAB_PAD_VAL_GPIO24  0x2
#define  VF_TAB_PAD_VAL_GPIO25  0x2
#define  VF_TAB_PAD_VAL_GPIO26  0x2
#define  VF_TAB_PAD_VAL_GPIO27  0x2
#define  VF_TAB_PAD_VAL_GPIO28  0x2
#define  VF_TAB_PAD_VAL_GPIO29  0x2
#define  VF_TAB_PAD_VAL_GPIO30  0x2
#define  VF_TAB_PAD_VAL_GPIO31  0x2
#define  VF_TAB_PAD_VAL_GPIO32  0x2
#define  VF_TAB_PAD_VAL_GPIO33  0x2
#define  VF_TAB_PAD_VAL_GPIO34  0x2
#define  VF_TAB_PAD_VAL_GPIO35  0x2
#define  VF_TAB_PAD_VAL_GPIO36  0x2
#define  VF_TAB_PAD_VAL_GPIO37  0x2
#define  VF_TAB_PAD_VAL_GPIO38  0x2
#define  VF_TAB_PAD_VAL_GPIO39  0x2
#define  VF_TAB_PAD_VAL_GPIO40  0x2
#define  VF_TAB_PAD_VAL_GPIO41  0x2
#define  VF_TAB_PAD_VAL_GPIO42  0x2
#define  VF_TAB_PAD_VAL_GPIO43  0x2
#define  VF_TAB_PAD_VAL_GPIO44  0x2
#define  VF_TAB_PAD_VAL_GPIO45  0x2
#define  VF_TAB_PAD_VAL_GPIO46  0x2
#define  VF_TAB_PAD_VAL_GPIO47  0x2
#define  VF_TAB_PAD_VAL_GPIO48  0x2
#define  VF_TAB_PAD_VAL_GPIO49  0x2
#define  VF_TAB_PAD_VAL_GPIO50  0x2
#define  VF_TAB_PAD_VAL_GPIO51  0x2
#define  VF_TAB_PAD_VAL_GPIO52  0x2
#define  VF_TAB_PAD_VAL_GPIO53  0x2
#define  VF_TAB_PAD_VAL_GPIO54  0x2
#define  VF_TAB_PAD_VAL_GPIO55  0x2
#define  VF_TAB_PAD_VAL_GPIO56  0x2
#define  VF_TAB_PAD_VAL_GPIO57  0x2
#define  VF_TAB_PAD_VAL_GPIO58  0x2
#define  VF_TAB_PAD_VAL_GPIO59  0x2
#define  VF_TAB_PAD_VAL_GPIO60  0x2
#define  VF_TAB_PAD_VAL_GPIO61  0x4
#define  VF_TAB_PAD_VAL_GPIO62  0x2
#define  VF_TAB_PAD_VAL_GPIO63  0x2
#define  VF_TAB_PAD_VAL_GPIO64  0x2
#define  VF_TAB_PAD_VAL_GPIO65  0x2
#define  VF_TAB_PAD_VAL_GPIO66  0x2
#define  VF_TAB_PAD_VAL_GPIO67  0x0
#define  VF_TAB_PAD_VAL_GPIO68  0x2
#define  VF_TAB_PAD_VAL_GPIO69  0x4
#define  VF_TAB_PAD_VAL_GPIO70  0x2


//
// PAD_DFT
//
#define  VF_TAB_PAD_DFT_GPIO0  0xC
#define  VF_TAB_PAD_DFT_GPIO1  0xC
#define  VF_TAB_PAD_DFT_GPIO2  0xC
#define  VF_TAB_PAD_DFT_GPIO3  0xC
#define  VF_TAB_PAD_DFT_GPIO4  0xC
#define  VF_TAB_PAD_DFT_GPIO5  0xC
#define  VF_TAB_PAD_DFT_GPIO6  0xC
#define  VF_TAB_PAD_DFT_GPIO7  0xC
#define  VF_TAB_PAD_DFT_GPIO8  0xC
#define  VF_TAB_PAD_DFT_GPIO9  0xC
#define  VF_TAB_PAD_DFT_GPIO10  0xC
#define  VF_TAB_PAD_DFT_GPIO11  0xC
#define  VF_TAB_PAD_DFT_GPIO12  0xC
#define  VF_TAB_PAD_DFT_GPIO13  0xC
#define  VF_TAB_PAD_DFT_GPIO14  0xC
#define  VF_TAB_PAD_DFT_GPIO15  0xC
#define  VF_TAB_PAD_DFT_GPIO16  0xC
#define  VF_TAB_PAD_DFT_GPIO17  0xC
#define  VF_TAB_PAD_DFT_GPIO18  0xC
#define  VF_TAB_PAD_DFT_GPIO19  0xC
#define  VF_TAB_PAD_DFT_GPIO20  0xC
#define  VF_TAB_PAD_DFT_GPIO21  0xC
#define  VF_TAB_PAD_DFT_GPIO22  0xC
#define  VF_TAB_PAD_DFT_GPIO23  0xC
#define  VF_TAB_PAD_DFT_GPIO24  0xC
#define  VF_TAB_PAD_DFT_GPIO25  0xC
#define  VF_TAB_PAD_DFT_GPIO26  0xC
#define  VF_TAB_PAD_DFT_GPIO27  0xC
#define  VF_TAB_PAD_DFT_GPIO28  0xC
#define  VF_TAB_PAD_DFT_GPIO29  0xC
#define  VF_TAB_PAD_DFT_GPIO30  0xC
#define  VF_TAB_PAD_DFT_GPIO31  0xC
#define  VF_TAB_PAD_DFT_GPIO32  0xC
#define  VF_TAB_PAD_DFT_GPIO33  0xC
#define  VF_TAB_PAD_DFT_GPIO34  0xC
#define  VF_TAB_PAD_DFT_GPIO35  0xC
#define  VF_TAB_PAD_DFT_GPIO36  0xC
#define  VF_TAB_PAD_DFT_GPIO37  0xC
#define  VF_TAB_PAD_DFT_GPIO38  0xC
#define  VF_TAB_PAD_DFT_GPIO39  0xC
#define  VF_TAB_PAD_DFT_GPIO40  0xC
#define  VF_TAB_PAD_DFT_GPIO41  0xC
#define  VF_TAB_PAD_DFT_GPIO42  0xC
#define  VF_TAB_PAD_DFT_GPIO43  0xC
#define  VF_TAB_PAD_DFT_GPIO44  0xC
#define  VF_TAB_PAD_DFT_GPIO45  0xC
#define  VF_TAB_PAD_DFT_GPIO46  0xC
#define  VF_TAB_PAD_DFT_GPIO47  0xC
#define  VF_TAB_PAD_DFT_GPIO48  0xC
#define  VF_TAB_PAD_DFT_GPIO49  0xC
#define  VF_TAB_PAD_DFT_GPIO50  0xC
#define  VF_TAB_PAD_DFT_GPIO51  0xC
#define  VF_TAB_PAD_DFT_GPIO52  0xC
#define  VF_TAB_PAD_DFT_GPIO53  0xC
#define  VF_TAB_PAD_DFT_GPIO54  0xC
#define  VF_TAB_PAD_DFT_GPIO55  0xC
#define  VF_TAB_PAD_DFT_GPIO56  0xC
#define  VF_TAB_PAD_DFT_GPIO57  0xC
#define  VF_TAB_PAD_DFT_GPIO58  0xC
#define  VF_TAB_PAD_DFT_GPIO59  0xC
#define  VF_TAB_PAD_DFT_GPIO60  0xC
#define  VF_TAB_PAD_DFT_GPIO61  0xC
#define  VF_TAB_PAD_DFT_GPIO62  0xC
#define  VF_TAB_PAD_DFT_GPIO63  0xC
#define  VF_TAB_PAD_DFT_GPIO64  0xC
#define  VF_TAB_PAD_DFT_GPIO65  0xC
#define  VF_TAB_PAD_DFT_GPIO66  0xC
#define  VF_TAB_PAD_DFT_GPIO67  0xC
#define  VF_TAB_PAD_DFT_GPIO68  0xC
#define  VF_TAB_PAD_DFT_GPIO69  0xC
#define  VF_TAB_PAD_DFT_GPIO70  0xC


//
//SUS WELL
//

//
// CONF0
//
#define  VF_TAB_PAD_CONF0_GPIO_SUS0  0xCCA8
#define  VF_TAB_PAD_CONF0_GPIO_SUS1  0xCCA8
#define  VF_TAB_PAD_CONF0_GPIO_SUS2  0xCCA8
#define  VF_TAB_PAD_CONF0_GPIO_SUS3  0xCD28
#define  VF_TAB_PAD_CONF0_GPIO_SUS4  0xCD28
#define  VF_TAB_PAD_CONF0_GPIO_SUS5  0xCD28
#define  VF_TAB_PAD_CONF0_GPIO_SUS6  0x8850
#define  VF_TAB_PAD_CONF0_GPIO_SUS7  0x8850
#define  VF_TAB_PAD_CONF0_GPIO_SUS8  0xCCA8
#define  VF_TAB_PAD_CONF0_GPIO_SUS9  0xCCA8
#define  VF_TAB_PAD_CONF0_GPIO_SUS10  0xCCA8
#define  VF_TAB_PAD_CONF0_GPIO_SUS11  0xC828
#define  VF_TAB_PAD_CONF0_GPIO_SUS12  0xC828
#define  VF_TAB_PAD_CONF0_GPIO_SUS13  0xCCA8
#define  VF_TAB_PAD_CONF0_GPIO_SUS14  0xCCA8
#define  VF_TAB_PAD_CONF0_GPIO_SUS15  0x8C80
#define  VF_TAB_PAD_CONF0_GPIO_SUS16  0xC828

//
// CONF1
//
#define  VF_TAB_PAD_CONF1_GPIO_SUS0  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS1  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS2  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS3  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS4  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS5  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS6  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS7  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS8  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS9  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS10  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS11  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS12  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS13  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS14  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS15  0
#define  VF_TAB_PAD_CONF1_GPIO_SUS16  0

//
// PAD_VAL
//
#define  VF_TAB_PAD_VAL_GPIO_SUS0  0
#define  VF_TAB_PAD_VAL_GPIO_SUS1  0
#define  VF_TAB_PAD_VAL_GPIO_SUS2  0
#define  VF_TAB_PAD_VAL_GPIO_SUS3  0
#define  VF_TAB_PAD_VAL_GPIO_SUS4  0
#define  VF_TAB_PAD_VAL_GPIO_SUS5  0
#define  VF_TAB_PAD_VAL_GPIO_SUS6  0
#define  VF_TAB_PAD_VAL_GPIO_SUS7  0
#define  VF_TAB_PAD_VAL_GPIO_SUS8  0
#define  VF_TAB_PAD_VAL_GPIO_SUS9  0
#define  VF_TAB_PAD_VAL_GPIO_SUS10  0
#define  VF_TAB_PAD_VAL_GPIO_SUS11  0
#define  VF_TAB_PAD_VAL_GPIO_SUS12  0
#define  VF_TAB_PAD_VAL_GPIO_SUS13  0
#define  VF_TAB_PAD_VAL_GPIO_SUS14  0
#define  VF_TAB_PAD_VAL_GPIO_SUS15  0
#define  VF_TAB_PAD_VAL_GPIO_SUS16  0

//
// PAD_DFT
//
#define  VF_TAB_PAD_DFT_GPIO_SUS0  0
#define  VF_TAB_PAD_DFT_GPIO_SUS1  0
#define  VF_TAB_PAD_DFT_GPIO_SUS2  0
#define  VF_TAB_PAD_DFT_GPIO_SUS3  0
#define  VF_TAB_PAD_DFT_GPIO_SUS4  0
#define  VF_TAB_PAD_DFT_GPIO_SUS5  0
#define  VF_TAB_PAD_DFT_GPIO_SUS6  0
#define  VF_TAB_PAD_DFT_GPIO_SUS7  0
#define  VF_TAB_PAD_DFT_GPIO_SUS8  0
#define  VF_TAB_PAD_DFT_GPIO_SUS9  0
#define  VF_TAB_PAD_DFT_GPIO_SUS10  0
#define  VF_TAB_PAD_DFT_GPIO_SUS11  0
#define  VF_TAB_PAD_DFT_GPIO_SUS12  0
#define  VF_TAB_PAD_DFT_GPIO_SUS13  0
#define  VF_TAB_PAD_DFT_GPIO_SUS14  0
#define  VF_TAB_PAD_DFT_GPIO_SUS15  0
#define  VF_TAB_PAD_DFT_GPIO_SUS16  0


//
//
//              GPIO Register Settings for ValleyFalls (Netbook)
//
//
//                   IO Space configyuration registers
// Field Descriptions:
//    USE: Defines the pin's usage model:  GPIO (G) or Native (N) mode.
//    I/O: Defines whether GPIOs are inputs (I) or outputs (O).
//         (Note:  Only meaningful for pins used as GPIOs.)
//    LVL: This field gives you the initial value for "output" GPIO's.
//         (Note: The output level is dependent upon whether the pin is inverted.)
//    TPE: Defines whether Trigger Positive Edge Enable.
//    TNE: Defines whether Trigger Negative Edge Enable.
//    WAKE_EN: only support in SUS community
//         (Note:  Only affects the level sent to the GPE logic and does not
//         affect the level read through the GPIO registers.)
//
//
//                    Memory spcae configuration registers
//
// Field Descriptions:
//   PAD releated:
//    PAD_CONF0
//      PAD_CONF1
//      PAD_VAL
//      PAD_DFT
//
// Notes:
//    1. N = Native , G = GPIO , I = Input, O = Output, - = BOTH/NOT SURE
//
// Signal          UsedAs                       USE     I/O      LVL     TPE     TNE   PCONF0   PCONF1   PVAL  PDFT
// -------------------------------------------------------------------------------------------------------------------------
// GPIO0           UART1_RXD-L                   N       I        -       -       -     cd29h    -         -     -
// GPIO1           UART1_TXD-0                   N       O        -       -       -     cd29h    -         -     -
// *GPIO2           UART1_RTS_B-1                N       I        -       -       -     cca9h    -         -     -
// *GPIO3           UART1_CTS_B-H                N       O        -       -       -     cca9h    -         -     -

// GPIO4           NMI_B-H                       G       -        -       -       -     cca9h    -         -     -
// GPIO5           GPIO_D5                       G       -        -       -       -     cca9h    -         -     -
// GPIO6           GPIO_D6                       G       O        -       -       -     8d51h    -         -     -
// GPIO7           GPIO_D7                       G       O        -       -       -     8cd1h    -         -     -
// GPIO8           GPIO_D8                       G       O        -       -       -     8cd1h    -         -     -
// GPIO9           GPIO_D9                       G       I        -       -       -     8cd1h    -         -     -

// GPIO10          GPIO_D10                      G       O        -       -       -     8d51h    -         -     -
// GPIO11          GPIO_D11                      G       O        -       -       -     8cd1h    -         -     -
// GPIO12          GPIO_D12                      G       O        -       -       -     8cd1h    -         -     -
// GPIO13          GPIO_D13                      G       I        -       -       -     8d51h    -         -     -

// GPIO14          SATA_GP0                      N       -        -       -       -      -       -         -     -
// GPIO15          SATA_GP1-L                    N       -        -       -       -     ccaah    -         -     -

// GPIO16          SATA_LEDN-OD-O                N       O        -       -       -      -       -         -     -
// GPIO17          PCIE_CLKREQ0B-20K,H           N       I        -       -       -     cd2ah    -         -     -
// GPIO18          PCIE_CLKREQ1B-20K,H           N       O        -       -       -     ccaah    -         -     -
// GPIO19          PCIE_CLKREQ2B-20K,H           N       I        -       -       -     ccaah    -         -     -
// GPIO20          PCIE_CLKREQ3B-20K,H           N       -        -       -       -     ccaah    -         -     -
// GPIO21          PCIE_CLKREQ4B-20K,H           N       -        -       -       -      -       -         -     -
// GPIO22          FLEX_CLK_SE0-20K,L            N       O        -       -       -     ccaah    -         -     -
// GPIO23          FLEX_CLK_SE1-20K,L            N       -        -       -       -      -       -         -     -

// GPIO24          HDA_RSTB                      N       O        -       -       -     8d02h    -         -     -
// GPIO25          HDA_SYNC                      N       O        -       -       -     8d02h    -         -     -
// GPIO26          HDA_CLK                       N       O        -       -       -     8d02h    -         -     -
// GPIO27          HDA_SDO                       N       I        -       -       -     8d02h    -         -     -
// GPIO28          HDA_SDI0                      N       I        -       -       -      -       -         -     -
// GPIO29          HDA_SDI1                      N       O        -       -       -      -       -         -     -
// GPIO30          HDA_DOCKRSTB                  N       I        -       -       -      -       -         -     -
// GPIO31          HDA_DOCKENB                   N       O        -       -       -      -       -         -     -

// GPIO32          SDMMC1_CLK                    N       -        -       -       -     208d51h   -         -     -
// GPIO33          SDMMC1_D0                     N       -        -       -       -     8cd1h     -         -     -
// GPIO34          SDMMC1_D1                     N       -        -       -       -     8cd1h     -         -     -
// GPIO35          SDMMC1_D2                     N       -        -       -       -     8cd1h     -         -     -
// GPIO36          SDMMC1_D3_CD_B                N       -        -       -       -     8cd1h     -         -     -
// GPIO37          MMC1_D4_SD_WE                 N       -        -       -       -     8cd1h     -         -     -
// GPIO38          MMC1_D5                       N       -        -       -       -     8cd1h     -         -     -
// GPIO39          MMC1_D6                       N       -        -       -       -     8cd1h     -         -     -
// GPIO40          MMC1_D7                       N       -        -       -       -     8cd1h     -         -     -
// GPIO41          SDMMC1_CMD                    N       -        -       -       -     8cd1h     -         -     -
// GPIO42          MMC1_RESET_B                  N       -        -       -       -     208d51h   -         -     -

// GPIO43          SDMMC2_CLK                    N       -        -       -       -     208d51h   -         -     -
// GPIO44          SDMMC2_D0                     N       -        -       -       -     8cd1h     -         -     -
// GPIO45          SDMMC2_D1                     N       -        -       -       -     8cd1h     -         -     -
// GPIO46          SDMMC2_D2                     N       -        -       -       -     8cd1h     -         -     -
// GPIO47          SDMMC2_D3_CD_B                N       -        -       -       -     8cd1h     -         -     -
// GPIO48          SDMMC2_CMD                    N       -        -       -       -     8cd1h     -         -     -

// GPIO49          SDMMC3_CLK                    N       -        -       -       -     8d51h      -         -     -
// GPIO50          SDMMC3_D0                     N       -        -       -       -     8cd1h      -         -     -
// GPIO51          SDMMC3_D1                     N       -        -       -       -      8cd1h      -         -     -
// GPIO52          SDMMC3_D2                     N       -        -       -       -      8cd1h      -         -     -
// GPIO53          SDMMC3_D3                     N       -        -       -       -      8cd1h      -         -     -
// GPIO54          SDMMC3_CD_B                   N       -        -       -       -      cca9h      -         -     -
// GPIO55          SDMMC3_CMD                    N       -        -       -       -      8cd1h      -         -     -
// GPIO56          SDMMC3_1P8_EN                 N       -        -       -       -      cd29h   -         -     -

// GPIO57          LPC_AD0                     N       -        -       -       -      -       -         -     -
// GPIO58          LPC_AD1                     N       -        -       -       -      -       -         -     -
// GPIO59          LPC_AD2                     N       -        -       -       -      -       -         -     -
// GPIO60          LPC_AD3                     N       -        -       -       -      -       -         -     -
// GPIO61          LPC_FRAMEB                  N       O        -       -       -      -       -         -     -
// GPIO62          LPC_CLKOUT0                 N       O        -       -       -      -       -         -     -
// GPIO63          LPC_CLKOUT1                 N       O        -       -       -      -       -         -     -
// GPIO64          LPC_CLKRUNB                 N       -        -       -       -      -       -         -     -

// GPIO65          SMB_DATA                    N       -        -       -       -      -       -         -     -
// GPIO66          SMB_CLK                     N       -        -       -       -      -       -         -     -
// GPIO67          SMB_ALERTB                  N       -        -       -       -      -       -         -     -

// GPIO68          ILB_SEIRQ                   N       -       -        -       -      -       -         -     -
// GPIO69          SPKR                        N       O       -        -       -      -       -         -     -

//SUS WELL


//GPIO_SUS0             GPIO_SUS0                   N       O       -        -       -     CCA8h       -         -     -
//GPIO_SUS1             GPIO_SUS1                   N       O       -        -       -     CCA8h       -         -     -
//GPIO_SUS2             GPIO_SUS2                   N       O       -        -       -     CCA8h       -         -     -
//GPIO_SUS3             GPIO_SUS3                   N       O       -        -       -     CD28h       -         -     -
//GPIO_SUS4             GPIO_SUS4                   N       O       -        -       -     CD28h       -         -     -
//GPIO_SUS5             GPIO_SUS5                   N       O       -        -       -     CD28h       -         -     -
//GPIO_SUS6             SUSPWRDNACK-0               N       O       -        -       -     8850h       -         -     -
//GPIO_SUS7             PMU_SLP_DDRVTT_B-0          N       O       -        -       -     8850h       -         -     -
//GPIO_SUS8             PMU_WAKE_B-20K,H            N       O       -        -       -     CCA8h       -         -     -
//GPIO_SUS9             PMU_PWRBTN_B-20K,H          N       O       -        -       -     CCA8h       -         -     -
//GPIO_SUS10            PMU_WAKE_LAN_B-20K,H        N       O       -        -       -     CCA8h       -         -     -
//GPIO_SUS11            SUS_STAT_B-1                N       O       -        -       -     C828h       -         -     -
//GPIO_SUS12            PMU_SUSCLK-0                N       O       -        -       -     C828h       -         -     -
//GPIO_SUS13            USB_OC0_B-20K,H             N       O       -        -       -     CCA8h       -         -     -
//GPIO_SUS14            USB_OC1_B-20K,H             N       O       -        -       -     CCA8h       -         -     -
//GPIO_SUS15            SPI_CS1_B-20K,H             N       O       -        -       -     8C80h       -         -     -
//GPIO_SUS16            SPI_CS1_B-20K,H             N       O       -        -       -     C828h       -         -     -
//

#define VF_NET_GPIO_USE_SEL_VAL_0_31        0x00000000
#define VF_NET_GPIO_USE_SEL_VAL_32_63       0x00000000
#define VF_NET_GPIO_USE_SEL_VAL_64_70       0x00000000
#define VF_NET_GPIO_USE_SEL_VAL_SUS         0x00000000

//
//1010 --00 0100 01-- 0101 --0- 0001 1010
//
#define VF_NET_GPIO_IO_SEL_VAL_0_31         0x00000000 // BIT30 | BIT28 | BIT27 | BIT19 | BIT17 | BIT13 | BIT9 | BIT2 | BIT0
#define VF_NET_GPIO_IO_SEL_VAL_32_63        0x00000000
#define VF_NET_GPIO_IO_SEL_VAL_64_70        0x00000000
#define VF_NET_GPIO_IO_SEL_VAL_SUS          0x00000000


#define VF_NET_GPIO_LVL_VAL_0_31            0x00000000
#define VF_NET_GPIO_LVL_VAL_32_63           0x00000000
#define VF_NET_GPIO_LVL_VAL_64_70           0x00000000
#define VF_NET_GPIO_LVL_VAL_SUS             0x00000000

#define VF_NET_GPIO_TPE_VAL_0_31            0x00000000
#define VF_NET_GPIO_TPE_VAL_SUS             0x00000000

#define VF_NET_GPIO_TNE_VAL_0_31            0x00000000
#define VF_NET_GPIO_TNE_VAL_SUS             0x00000000

#define VF_NET_GPIO_TS_VAL_0_31             0x00000000
#define VF_NET_GPIO_TS_VAL_SUS              0x00000000


//
// Memory space registers
//


//
// CONF0
//
#define  VF_NET_PAD_CONF0_GPIO0  0xcd29
#define  VF_NET_PAD_CONF0_GPIO1  0xcd29
#define  VF_NET_PAD_CONF0_GPIO2  0xcca9
#define  VF_NET_PAD_CONF0_GPIO3  0xcca9
#define  VF_NET_PAD_CONF0_GPIO4  0xcca8
#define  VF_NET_PAD_CONF0_GPIO5  0xcca8
#define  VF_NET_PAD_CONF0_GPIO6  0x8d50
#define  VF_NET_PAD_CONF0_GPIO7  0x8cd0
#define  VF_NET_PAD_CONF0_GPIO8  0x8cd0
#define  VF_NET_PAD_CONF0_GPIO9  0x8cd0
#define  VF_NET_PAD_CONF0_GPIO10  0x8d50
#define  VF_NET_PAD_CONF0_GPIO11  0x8cd0
#define  VF_NET_PAD_CONF0_GPIO12  0x8cd0
#define  VF_NET_PAD_CONF0_GPIO13  0x8d50
#define  VF_NET_PAD_CONF0_GPIO14  0xCCA8
#define  VF_NET_PAD_CONF0_GPIO15  0xccaa
#define  VF_NET_PAD_CONF0_GPIO16  0xC828
#define  VF_NET_PAD_CONF0_GPIO17  0xcd2a
#define  VF_NET_PAD_CONF0_GPIO18  0xccaa
#define  VF_NET_PAD_CONF0_GPIO19  0xccaa
#define  VF_NET_PAD_CONF0_GPIO20  0xccaa
#define  VF_NET_PAD_CONF0_GPIO21  0xCCA9
#define  VF_NET_PAD_CONF0_GPIO22  0xccaa
#define  VF_NET_PAD_CONF0_GPIO23  0xCD2A
#define  VF_NET_PAD_CONF0_GPIO24  0x8d02
#define  VF_NET_PAD_CONF0_GPIO25  0x8d02
#define  VF_NET_PAD_CONF0_GPIO26  0x8d02
#define  VF_NET_PAD_CONF0_GPIO27  0x8d02
#define  VF_NET_PAD_CONF0_GPIO28  0x8D02
#define  VF_NET_PAD_CONF0_GPIO29  0x8D02
#define  VF_NET_PAD_CONF0_GPIO30  0x8D00
#define  VF_NET_PAD_CONF0_GPIO31  0xCD2A
#define  VF_NET_PAD_CONF0_GPIO32  0x208d51
#define  VF_NET_PAD_CONF0_GPIO33  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO34  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO35  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO36  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO37  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO38  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO39  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO40  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO41  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO42  0x208d51
#define  VF_NET_PAD_CONF0_GPIO43  0x208d51
#define  VF_NET_PAD_CONF0_GPIO44  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO45  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO46  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO47  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO48  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO49  0x8d51
#define  VF_NET_PAD_CONF0_GPIO50  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO51  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO52  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO53  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO54  0xcca9
#define  VF_NET_PAD_CONF0_GPIO55  0x8cd1
#define  VF_NET_PAD_CONF0_GPIO56  0xcd29
#define  VF_NET_PAD_CONF0_GPIO57  0x8C80
#define  VF_NET_PAD_CONF0_GPIO58  0x8C80
#define  VF_NET_PAD_CONF0_GPIO59  0x8C80
#define  VF_NET_PAD_CONF0_GPIO60  0x8C80
#define  VF_NET_PAD_CONF0_GPIO61  0x8800
#define  VF_NET_PAD_CONF0_GPIO62  0x8D00
#define  VF_NET_PAD_CONF0_GPIO63  0x8800
#define  VF_NET_PAD_CONF0_GPIO64  0x8800
#define  VF_NET_PAD_CONF0_GPIO65  0xC828
#define  VF_NET_PAD_CONF0_GPIO66  0xC828
#define  VF_NET_PAD_CONF0_GPIO67  0xC828
#define  VF_NET_PAD_CONF0_GPIO68  0xCCA8
#define  VF_NET_PAD_CONF0_GPIO69  0xC828
#define  VF_NET_PAD_CONF0_GPIO70  0xCCA8




//
// PAD_CONF1
//
#define  VF_NET_PAD_CONF1_GPIO0  0x20002
#define  VF_NET_PAD_CONF1_GPIO1  0x20002
#define  VF_NET_PAD_CONF1_GPIO2  0x20002
#define  VF_NET_PAD_CONF1_GPIO3  0x20002
#define  VF_NET_PAD_CONF1_GPIO4  0x20002
#define  VF_NET_PAD_CONF1_GPIO5  0x20002
#define  VF_NET_PAD_CONF1_GPIO6  0x1F000F
#define  VF_NET_PAD_CONF1_GPIO7  0x1F000F
#define  VF_NET_PAD_CONF1_GPIO8  0x1F000F
#define  VF_NET_PAD_CONF1_GPIO9  0x1F000F
#define  VF_NET_PAD_CONF1_GPIO10  0x1F000F
#define  VF_NET_PAD_CONF1_GPIO11  0x1F000F
#define  VF_NET_PAD_CONF1_GPIO12  0x1F000F
#define  VF_NET_PAD_CONF1_GPIO13  0x1F000F
#define  VF_NET_PAD_CONF1_GPIO14  0x20002
#define  VF_NET_PAD_CONF1_GPIO15  0x20002
#define  VF_NET_PAD_CONF1_GPIO16  0x20002
#define  VF_NET_PAD_CONF1_GPIO17  0x20002
#define  VF_NET_PAD_CONF1_GPIO18  0x20002
#define  VF_NET_PAD_CONF1_GPIO19  0x20002
#define  VF_NET_PAD_CONF1_GPIO20  0x20002
#define  VF_NET_PAD_CONF1_GPIO21  0x20002
#define  VF_NET_PAD_CONF1_GPIO22  0x20002
#define  VF_NET_PAD_CONF1_GPIO23  0x20002
#define  VF_NET_PAD_CONF1_GPIO24  0x00000
#define  VF_NET_PAD_CONF1_GPIO25  0x00000
#define  VF_NET_PAD_CONF1_GPIO26  0x00000
#define  VF_NET_PAD_CONF1_GPIO27  0x00000
#define  VF_NET_PAD_CONF1_GPIO28  0x00000
#define  VF_NET_PAD_CONF1_GPIO29  0x00000
#define  VF_NET_PAD_CONF1_GPIO30  0x00000
#define  VF_NET_PAD_CONF1_GPIO31  0x20002
#define  VF_NET_PAD_CONF1_GPIO32  0x00000
#define  VF_NET_PAD_CONF1_GPIO33  0x00000
#define  VF_NET_PAD_CONF1_GPIO34  0x00000
#define  VF_NET_PAD_CONF1_GPIO35  0x00000
#define  VF_NET_PAD_CONF1_GPIO36  0x00000
#define  VF_NET_PAD_CONF1_GPIO37  0x00000
#define  VF_NET_PAD_CONF1_GPIO38  0x00000
#define  VF_NET_PAD_CONF1_GPIO39  0x00000
#define  VF_NET_PAD_CONF1_GPIO40  0x00000
#define  VF_NET_PAD_CONF1_GPIO41  0x00000
#define  VF_NET_PAD_CONF1_GPIO42  0x00000
#define  VF_NET_PAD_CONF1_GPIO43  0x00000
#define  VF_NET_PAD_CONF1_GPIO44  0x00000
#define  VF_NET_PAD_CONF1_GPIO45  0x00000
#define  VF_NET_PAD_CONF1_GPIO46  0x00000
#define  VF_NET_PAD_CONF1_GPIO47  0x00000
#define  VF_NET_PAD_CONF1_GPIO48  0x00000
#define  VF_NET_PAD_CONF1_GPIO49  0x00000
#define  VF_NET_PAD_CONF1_GPIO50  0x00000
#define  VF_NET_PAD_CONF1_GPIO51  0x00000
#define  VF_NET_PAD_CONF1_GPIO52  0x00000
#define  VF_NET_PAD_CONF1_GPIO53  0x00000
#define  VF_NET_PAD_CONF1_GPIO54  0x20002
#define  VF_NET_PAD_CONF1_GPIO55  0x00000
#define  VF_NET_PAD_CONF1_GPIO56  0x20002
#define  VF_NET_PAD_CONF1_GPIO57  0x00000
#define  VF_NET_PAD_CONF1_GPIO58  0x00000
#define  VF_NET_PAD_CONF1_GPIO59  0x00000
#define  VF_NET_PAD_CONF1_GPIO60  0x00000
#define  VF_NET_PAD_CONF1_GPIO61  0x00000
#define  VF_NET_PAD_CONF1_GPIO62  0x00000
#define  VF_NET_PAD_CONF1_GPIO63  0x00000
#define  VF_NET_PAD_CONF1_GPIO64  0x00000
#define  VF_NET_PAD_CONF1_GPIO65  0x20002
#define  VF_NET_PAD_CONF1_GPIO66  0x20002
#define  VF_NET_PAD_CONF1_GPIO67  0x20002
#define  VF_NET_PAD_CONF1_GPIO68  0x20002
#define  VF_NET_PAD_CONF1_GPIO69  0x20002
#define  VF_NET_PAD_CONF1_GPIO70  0x20002



//
// PAD_VAL
//
#define  VF_NET_PAD_VAL_GPIO0  0x2
#define  VF_NET_PAD_VAL_GPIO1  0x2
#define  VF_NET_PAD_VAL_GPIO2  0x2
#define  VF_NET_PAD_VAL_GPIO3  0x2
#define  VF_NET_PAD_VAL_GPIO4  0x2
#define  VF_NET_PAD_VAL_GPIO5  0x2
#define  VF_NET_PAD_VAL_GPIO6  0x2
#define  VF_NET_PAD_VAL_GPIO7  0x2
#define  VF_NET_PAD_VAL_GPIO8  0x2
#define  VF_NET_PAD_VAL_GPIO9  0x2
#define  VF_NET_PAD_VAL_GPIO10  0x2
#define  VF_NET_PAD_VAL_GPIO11  0x2
#define  VF_NET_PAD_VAL_GPIO12  0x2
#define  VF_NET_PAD_VAL_GPIO13  0x2
#define  VF_NET_PAD_VAL_GPIO14  0x2
#define  VF_NET_PAD_VAL_GPIO15  0x2
#define  VF_NET_PAD_VAL_GPIO16  0x4
#define  VF_NET_PAD_VAL_GPIO17  0x2
#define  VF_NET_PAD_VAL_GPIO18  0x2
#define  VF_NET_PAD_VAL_GPIO19  0x2
#define  VF_NET_PAD_VAL_GPIO20  0x2
#define  VF_NET_PAD_VAL_GPIO21  0x2
#define  VF_NET_PAD_VAL_GPIO22  0x2
#define  VF_NET_PAD_VAL_GPIO23  0x2
#define  VF_NET_PAD_VAL_GPIO24  0x2
#define  VF_NET_PAD_VAL_GPIO25  0x2
#define  VF_NET_PAD_VAL_GPIO26  0x2
#define  VF_NET_PAD_VAL_GPIO27  0x2
#define  VF_NET_PAD_VAL_GPIO28  0x2
#define  VF_NET_PAD_VAL_GPIO29  0x2
#define  VF_NET_PAD_VAL_GPIO30  0x2
#define  VF_NET_PAD_VAL_GPIO31  0x2
#define  VF_NET_PAD_VAL_GPIO32  0x2
#define  VF_NET_PAD_VAL_GPIO33  0x2
#define  VF_NET_PAD_VAL_GPIO34  0x2
#define  VF_NET_PAD_VAL_GPIO35  0x2
#define  VF_NET_PAD_VAL_GPIO36  0x2
#define  VF_NET_PAD_VAL_GPIO37  0x2
#define  VF_NET_PAD_VAL_GPIO38  0x2
#define  VF_NET_PAD_VAL_GPIO39  0x2
#define  VF_NET_PAD_VAL_GPIO40  0x2
#define  VF_NET_PAD_VAL_GPIO41  0x2
#define  VF_NET_PAD_VAL_GPIO42  0x2
#define  VF_NET_PAD_VAL_GPIO43  0x2
#define  VF_NET_PAD_VAL_GPIO44  0x2
#define  VF_NET_PAD_VAL_GPIO45  0x2
#define  VF_NET_PAD_VAL_GPIO46  0x2
#define  VF_NET_PAD_VAL_GPIO47  0x2
#define  VF_NET_PAD_VAL_GPIO48  0x2
#define  VF_NET_PAD_VAL_GPIO49  0x2
#define  VF_NET_PAD_VAL_GPIO50  0x2
#define  VF_NET_PAD_VAL_GPIO51  0x2
#define  VF_NET_PAD_VAL_GPIO52  0x2
#define  VF_NET_PAD_VAL_GPIO53  0x2
#define  VF_NET_PAD_VAL_GPIO54  0x2
#define  VF_NET_PAD_VAL_GPIO55  0x2
#define  VF_NET_PAD_VAL_GPIO56  0x2
#define  VF_NET_PAD_VAL_GPIO57  0x2
#define  VF_NET_PAD_VAL_GPIO58  0x2
#define  VF_NET_PAD_VAL_GPIO59  0x2
#define  VF_NET_PAD_VAL_GPIO60  0x2
#define  VF_NET_PAD_VAL_GPIO61  0x4
#define  VF_NET_PAD_VAL_GPIO62  0x2
#define  VF_NET_PAD_VAL_GPIO63  0x2
#define  VF_NET_PAD_VAL_GPIO64  0x2
#define  VF_NET_PAD_VAL_GPIO65  0x2
#define  VF_NET_PAD_VAL_GPIO66  0x2
#define  VF_NET_PAD_VAL_GPIO67  0x0
#define  VF_NET_PAD_VAL_GPIO68  0x2
#define  VF_NET_PAD_VAL_GPIO69  0x4
#define  VF_NET_PAD_VAL_GPIO70  0x2


//
// PAD_DFT
//
#define  VF_NET_PAD_DFT_GPIO0  0xC
#define  VF_NET_PAD_DFT_GPIO1  0xC
#define  VF_NET_PAD_DFT_GPIO2  0xC
#define  VF_NET_PAD_DFT_GPIO3  0xC
#define  VF_NET_PAD_DFT_GPIO4  0xC
#define  VF_NET_PAD_DFT_GPIO5  0xC
#define  VF_NET_PAD_DFT_GPIO6  0xC
#define  VF_NET_PAD_DFT_GPIO7  0xC
#define  VF_NET_PAD_DFT_GPIO8  0xC
#define  VF_NET_PAD_DFT_GPIO9  0xC
#define  VF_NET_PAD_DFT_GPIO10  0xC
#define  VF_NET_PAD_DFT_GPIO11  0xC
#define  VF_NET_PAD_DFT_GPIO12  0xC
#define  VF_NET_PAD_DFT_GPIO13  0xC
#define  VF_NET_PAD_DFT_GPIO14  0xC
#define  VF_NET_PAD_DFT_GPIO15  0xC
#define  VF_NET_PAD_DFT_GPIO16  0xC
#define  VF_NET_PAD_DFT_GPIO17  0xC
#define  VF_NET_PAD_DFT_GPIO18  0xC
#define  VF_NET_PAD_DFT_GPIO19  0xC
#define  VF_NET_PAD_DFT_GPIO20  0xC
#define  VF_NET_PAD_DFT_GPIO21  0xC
#define  VF_NET_PAD_DFT_GPIO22  0xC
#define  VF_NET_PAD_DFT_GPIO23  0xC
#define  VF_NET_PAD_DFT_GPIO24  0xC
#define  VF_NET_PAD_DFT_GPIO25  0xC
#define  VF_NET_PAD_DFT_GPIO26  0xC
#define  VF_NET_PAD_DFT_GPIO27  0xC
#define  VF_NET_PAD_DFT_GPIO28  0xC
#define  VF_NET_PAD_DFT_GPIO29  0xC
#define  VF_NET_PAD_DFT_GPIO30  0xC
#define  VF_NET_PAD_DFT_GPIO31  0xC
#define  VF_NET_PAD_DFT_GPIO32  0xC
#define  VF_NET_PAD_DFT_GPIO33  0xC
#define  VF_NET_PAD_DFT_GPIO34  0xC
#define  VF_NET_PAD_DFT_GPIO35  0xC
#define  VF_NET_PAD_DFT_GPIO36  0xC
#define  VF_NET_PAD_DFT_GPIO37  0xC
#define  VF_NET_PAD_DFT_GPIO38  0xC
#define  VF_NET_PAD_DFT_GPIO39  0xC
#define  VF_NET_PAD_DFT_GPIO40  0xC
#define  VF_NET_PAD_DFT_GPIO41  0xC
#define  VF_NET_PAD_DFT_GPIO42  0xC
#define  VF_NET_PAD_DFT_GPIO43  0xC
#define  VF_NET_PAD_DFT_GPIO44  0xC
#define  VF_NET_PAD_DFT_GPIO45  0xC
#define  VF_NET_PAD_DFT_GPIO46  0xC
#define  VF_NET_PAD_DFT_GPIO47  0xC
#define  VF_NET_PAD_DFT_GPIO48  0xC
#define  VF_NET_PAD_DFT_GPIO49  0xC
#define  VF_NET_PAD_DFT_GPIO50  0xC
#define  VF_NET_PAD_DFT_GPIO51  0xC
#define  VF_NET_PAD_DFT_GPIO52  0xC
#define  VF_NET_PAD_DFT_GPIO53  0xC
#define  VF_NET_PAD_DFT_GPIO54  0xC
#define  VF_NET_PAD_DFT_GPIO55  0xC
#define  VF_NET_PAD_DFT_GPIO56  0xC
#define  VF_NET_PAD_DFT_GPIO57  0xC
#define  VF_NET_PAD_DFT_GPIO58  0xC
#define  VF_NET_PAD_DFT_GPIO59  0xC
#define  VF_NET_PAD_DFT_GPIO60  0xC
#define  VF_NET_PAD_DFT_GPIO61  0xC
#define  VF_NET_PAD_DFT_GPIO62  0xC
#define  VF_NET_PAD_DFT_GPIO63  0xC
#define  VF_NET_PAD_DFT_GPIO64  0xC
#define  VF_NET_PAD_DFT_GPIO65  0xC
#define  VF_NET_PAD_DFT_GPIO66  0xC
#define  VF_NET_PAD_DFT_GPIO67  0xC
#define  VF_NET_PAD_DFT_GPIO68  0xC
#define  VF_NET_PAD_DFT_GPIO69  0xC
#define  VF_NET_PAD_DFT_GPIO70  0xC

//
// PCONF0
//
#define  VF_NET_PAD_CONF0_GPIO_SUS0  0xCCA8
#define  VF_NET_PAD_CONF0_GPIO_SUS1  0xCCA8
#define  VF_NET_PAD_CONF0_GPIO_SUS2  0xCCA8
#define  VF_NET_PAD_CONF0_GPIO_SUS3  0xCD28
#define  VF_NET_PAD_CONF0_GPIO_SUS4  0xCD28
#define  VF_NET_PAD_CONF0_GPIO_SUS5  0xCD28
#define  VF_NET_PAD_CONF0_GPIO_SUS6  0x8850
#define  VF_NET_PAD_CONF0_GPIO_SUS7  0x8850
#define  VF_NET_PAD_CONF0_GPIO_SUS8  0xCCA8
#define  VF_NET_PAD_CONF0_GPIO_SUS9  0xCCA8
#define  VF_NET_PAD_CONF0_GPIO_SUS10  0xCCA8
#define  VF_NET_PAD_CONF0_GPIO_SUS11  0xC828
#define  VF_NET_PAD_CONF0_GPIO_SUS12  0xC828
#define  VF_NET_PAD_CONF0_GPIO_SUS13  0xCCA8
#define  VF_NET_PAD_CONF0_GPIO_SUS14  0xCCA8
#define  VF_NET_PAD_CONF0_GPIO_SUS15  0x8C80
#define  VF_NET_PAD_CONF0_GPIO_SUS16  0xC828

//
// PCONF1
//
#define  VF_NET_PAD_CONF1_GPIO_SUS0  0
#define  VF_NET_PAD_CONF1_GPIO_SUS1  0
#define  VF_NET_PAD_CONF1_GPIO_SUS2  0
#define  VF_NET_PAD_CONF1_GPIO_SUS3  0
#define  VF_NET_PAD_CONF1_GPIO_SUS4  0
#define  VF_NET_PAD_CONF1_GPIO_SUS5  0
#define  VF_NET_PAD_CONF1_GPIO_SUS6  0
#define  VF_NET_PAD_CONF1_GPIO_SUS7  0
#define  VF_NET_PAD_CONF1_GPIO_SUS8  0
#define  VF_NET_PAD_CONF1_GPIO_SUS9  0
#define  VF_NET_PAD_CONF1_GPIO_SUS10  0
#define  VF_NET_PAD_CONF1_GPIO_SUS11  0
#define  VF_NET_PAD_CONF1_GPIO_SUS12  0
#define  VF_NET_PAD_CONF1_GPIO_SUS13  0
#define  VF_NET_PAD_CONF1_GPIO_SUS14  0
#define  VF_NET_PAD_CONF1_GPIO_SUS15  0
#define  VF_NET_PAD_CONF1_GPIO_SUS16  0


#define  VF_NET_PAD_VAL_GPIO_SUS0  0
#define  VF_NET_PAD_VAL_GPIO_SUS1  0
#define  VF_NET_PAD_VAL_GPIO_SUS2  0
#define  VF_NET_PAD_VAL_GPIO_SUS3  0
#define  VF_NET_PAD_VAL_GPIO_SUS4  0
#define  VF_NET_PAD_VAL_GPIO_SUS5  0
#define  VF_NET_PAD_VAL_GPIO_SUS6  0
#define  VF_NET_PAD_VAL_GPIO_SUS7  0
#define  VF_NET_PAD_VAL_GPIO_SUS8  0
#define  VF_NET_PAD_VAL_GPIO_SUS9  0
#define  VF_NET_PAD_VAL_GPIO_SUS10  0
#define  VF_NET_PAD_VAL_GPIO_SUS11  0
#define  VF_NET_PAD_VAL_GPIO_SUS12  0
#define  VF_NET_PAD_VAL_GPIO_SUS13  0
#define  VF_NET_PAD_VAL_GPIO_SUS14  0
#define  VF_NET_PAD_VAL_GPIO_SUS15  0
#define  VF_NET_PAD_VAL_GPIO_SUS16  0


#define  VF_NET_PAD_DFT_GPIO_SUS0  0
#define  VF_NET_PAD_DFT_GPIO_SUS1  0
#define  VF_NET_PAD_DFT_GPIO_SUS2  0
#define  VF_NET_PAD_DFT_GPIO_SUS3  0
#define  VF_NET_PAD_DFT_GPIO_SUS4  0
#define  VF_NET_PAD_DFT_GPIO_SUS5  0
#define  VF_NET_PAD_DFT_GPIO_SUS6  0
#define  VF_NET_PAD_DFT_GPIO_SUS7  0
#define  VF_NET_PAD_DFT_GPIO_SUS8  0
#define  VF_NET_PAD_DFT_GPIO_SUS9  0
#define  VF_NET_PAD_DFT_GPIO_SUS10  0
#define  VF_NET_PAD_DFT_GPIO_SUS11  0
#define  VF_NET_PAD_DFT_GPIO_SUS12  0
#define  VF_NET_PAD_DFT_GPIO_SUS13  0
#define  VF_NET_PAD_DFT_GPIO_SUS14  0
#define  VF_NET_PAD_DFT_GPIO_SUS15  0
#define  VF_NET_PAD_DFT_GPIO_SUS16  0


//
// Function Prototypes
//
EFI_STATUS
PlatformPchInit (
  IN SYSTEM_CONFIGURATION        *SystemConfiguration,
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN UINT16                      PlatformType
  );

EFI_STATUS
PlatformCpuInit (
  IN CONST EFI_PEI_SERVICES            **PeiServices,
  IN SYSTEM_CONFIGURATION        *SystemConfiguration,
  IN EFI_PLATFORM_CPU_INFO       *PlatformCpuInfo
  );

EFI_STATUS
PeimInitializeFlashMap (
  IN EFI_FFS_FILE_HEADER        *FfsHeader,
  IN CONST EFI_PEI_SERVICES           **PeiServices
  );

EFI_STATUS
PeimInstallFlashMapPpi (
  IN EFI_FFS_FILE_HEADER        *FfsHeader,
  IN CONST EFI_PEI_SERVICES           **PeiServices
  );

EFI_STATUS
EFIAPI
IchReset (
  IN CONST EFI_PEI_SERVICES           **PeiServices
  )
;

BOOLEAN
GetSleepTypeAfterWakeup (
  IN  CONST EFI_PEI_SERVICES          **PeiServices,
  OUT UINT16                    *SleepType
  );

EFI_STATUS
EFIAPI
GetWakeupEventAndSaveToHob (
  IN CONST EFI_PEI_SERVICES   **PeiServices
  )
;

EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
;

EFI_STATUS
EFIAPI
PeiGetVariable (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     * VendorGuid,
  OUT UINT32                      *Attributes OPTIONAL,
  IN OUT UINTN                    *DataSize,
  OUT VOID                        *Data
  )
;

EFI_STATUS
EFIAPI
PeiGetNextVariableName (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 *VendorGuid
  )
;

EFI_STATUS
UpdateBootMode (
  IN CONST EFI_PEI_SERVICES                       **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB                    *PlatformInfoHob
  );

EFI_STATUS
EFIAPI
EndOfPeiPpiNotifyCallback (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_STATUS
EFIAPI
PeimInitializeRecovery (
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
;

VOID
CheckPowerOffNow (
  VOID
  );

VOID
IchGpioInit (
  IN UINT16                     PlatformType,
  IN SYSTEM_CONFIGURATION       *SystemConfiguration
  );

EFI_STATUS
PcieSecondaryBusReset (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN UINT8             Bus,
  IN UINT8             Dev,
  IN UINT8             Fun
  );

VOID
SetPlatformBootMode (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
  );

BOOLEAN
CheckIfJumperSetForRecovery(
  VOID
  );

EFI_STATUS
EFIAPI    
FindFv (
  IN EFI_PEI_FIND_FV_PPI              *This,
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN OUT UINT8                    *FvNumber,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **FVAddress
  );

BOOLEAN
IsA16Inverted (
  );

EFI_STATUS
EFIAPI
CpuOnlyReset (
  IN CONST EFI_PEI_SERVICES   **PeiServices
  );

EFI_STATUS
EFIAPI
InitLan (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN SYSTEM_CONFIGURATION      *Buffer
  );

EFI_STATUS
EFIAPI
Stall (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN CONST EFI_PEI_STALL_PPI    *This,
  IN UINTN                      Microseconds
  );

EFI_STATUS
MultiPlatformInfoInit (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
  );

BOOLEAN
IsRecoveryJumper (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
    IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
);

EFI_STATUS
CheckOsSelection (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN SYSTEM_CONFIGURATION            *SystemConfiguration
  );

EFI_STATUS
PlatformInfoUpdate (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob,
  IN SYSTEM_CONFIGURATION      *SystemConfiguration
  );

VOID
PlatformSsaInit (
IN SYSTEM_CONFIGURATION        *SystemConfiguration,
IN CONST EFI_PEI_SERVICES      **PeiServices
  );

EFI_STATUS
InitializePlatform (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PLATFORM_INFO_HOB        *PlatformInfoHob,
  IN SYSTEM_CONFIGURATION         *SystemConfiguration
);

VOID
MchInit (
IN CONST EFI_PEI_SERVICES                     **PeiServices
  );


EFI_STATUS
EFIAPI
SetPeiCacheMode (
  IN  CONST EFI_PEI_SERVICES    **PeiServices
  );

EFI_STATUS
EFIAPI
SetDxeCacheMode (
  IN  CONST EFI_PEI_SERVICES    **PeiServices
  );

EFI_STATUS
GPIO_initialization (
  IN EFI_PEI_SERVICES                   **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR          *NotifyDescriptor,
  IN VOID                               *SmbusPpi
  );


BOOLEAN
IsRtcUipAlwaysSet (
  IN CONST EFI_PEI_SERVICES       **PeiServices
  );



EFI_STATUS
InitPchUsb (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  );

EFI_STATUS
EFIAPI
PublishMemoryTypeInfo (
  void
  );


#endif
