/*++

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  PchRegsLpss.h

Abstract:

  Register names for VLV Low Input Output (LPSS) module.

  Conventions:

  - Prefixes:
    Definitions beginning with "R_" are registers
    Definitions beginning with "B_" are bits within registers
    Definitions beginning with "V_" are meaningful values of bits within the registers
    Definitions beginning with "S_" are register sizes
    Definitions beginning with "N_" are the bit position
  - In general, PCH registers are denoted by "_PCH_" in register names
  - Registers / bits that are different between PCH generations are denoted by
    "_PCH_<generation_name>_" in register/bit names. e.g., "_PCH_VLV_"
  - Registers / bits that are different between SKUs are denoted by "_<SKU_name>"
    at the end of the register/bit names
  - Registers / bits of new devices introduced in a PCH generation will be just named
    as "_PCH_" without <generation_name> inserted.

--*/
#ifndef _PCH_REGS_LPSS_H_
#define _PCH_REGS_LPSS_H_


//
// Low Power Input Output (LPSS) Module Registers
//

//
// LPSS DMAC Modules
// PCI Config Space Registers
//
#define PCI_DEVICE_NUMBER_PCH_LPSS_DMAC0          30
#define PCI_DEVICE_NUMBER_PCH_LPSS_DMAC1          24
#define PCI_FUNCTION_NUMBER_PCH_LPSS_DMAC         0

#define R_PCH_LPSS_DMAC_DEVVENDID                 0x00  // Device ID & Vendor ID
#define B_PCH_LPSS_DMAC_DEVVENDID_DID             0xFFFF0000 // Device ID
#define B_PCH_LPSS_DMAC_DEVVENDID_VID             0x0000FFFF // Vendor ID

#define R_PCH_LPSS_DMAC_STSCMD                    0x04  // Status & Command
#define B_PCH_LPSS_DMAC_STSCMD_RMA                BIT29 // RMA
#define B_PCH_LPSS_DMAC_STSCMD_RCA                BIT28 // RCA
#define B_PCH_LPSS_DMAC_STSCMD_CAPLIST            BIT20 // Capability List
#define B_PCH_LPSS_DMAC_STSCMD_INTRSTS            BIT19 // Interrupt Status
#define B_PCH_LPSS_DMAC_STSCMD_INTRDIS            BIT10 // Interrupt Disable
#define B_PCH_LPSS_DMAC_STSCMD_SERREN             BIT8  // SERR# Enable
#define B_PCH_LPSS_DMAC_STSCMD_BME                BIT2  // Bus Master Enable
#define B_PCH_LPSS_DMAC_STSCMD_MSE                BIT1  // Memory Space Enable

#define R_PCH_LPSS_DMAC_REVCC                     0x08  // Revision ID & Class Code
#define B_PCH_LPSS_DMAC_REVCC_CC                  0xFFFFFF00 // Class Code
#define B_PCH_LPSS_DMAC_REVCC_RID                 0x000000FF // Revision ID

#define R_PCH_LPSS_DMAC_CLHB                      0x0C
#define B_PCH_LPSS_DMAC_CLHB_MULFNDEV             BIT23
#define B_PCH_LPSS_DMAC_CLHB_HT                   0x007F0000 // Header Type
#define B_PCH_LPSS_DMAC_CLHB_LT                   0x0000FF00 // Latency Timer
#define B_PCH_LPSS_DMAC_CLHB_CLS                  0x000000FF // Cache Line Size

#define R_PCH_LPSS_DMAC_BAR                       0x10  // BAR
#define B_PCH_LPSS_DMAC_BAR_BA                    0xFFFFC000 // Base Address
#define V_PCH_LPSS_DMAC_BAR_SIZE                  0x4000
#define N_PCH_LPSS_DMAC_BAR_ALIGNMENT             14
#define B_PCH_LPSS_DMAC_BAR_SI                    0x00000FF0 // Size Indicator
#define B_PCH_LPSS_DMAC_BAR_PF                    BIT3  // Prefetchable
#define B_PCH_LPSS_DMAC_BAR_TYPE                  (BIT2 | BIT1) // Type
#define B_PCH_LPSS_DMAC_BAR_MS                    BIT0  // Message Space

#define R_PCH_LPSS_DMAC_BAR1                      0x14  // BAR 1
#define B_PCH_LPSS_DMAC_BAR1_BA                   0xFFFFF000 // Base Address
#define B_PCH_LPSS_DMAC_BAR1_SI                   0x00000FF0 // Size Indicator
#define B_PCH_LPSS_DMAC_BAR1_PF                   BIT3  // Prefetchable
#define B_PCH_LPSS_DMAC_BAR1_TYPE                 (BIT2 | BIT1) // Type
#define B_PCH_LPSS_DMAC_BAR1_MS                   BIT0  // Message Space

#define R_PCH_LPSS_DMAC_SSID                      0x2C  // Sub System ID
#define B_PCH_LPSS_DMAC_SSID_SID                  0xFFFF0000 // Sub System ID
#define B_PCH_LPSS_DMAC_SSID_SVID                 0x0000FFFF // Sub System Vendor ID

#define R_PCH_LPSS_DMAC_ERBAR                     0x30  // Expansion ROM BAR
#define B_PCH_LPSS_DMAC_ERBAR_BA                  0xFFFFFFFF // Expansion ROM Base Address

#define R_PCH_LPSS_DMAC_CAPPTR                    0x34  // Capability Pointer
#define B_PCH_LPSS_DMAC_CAPPTR_CPPWR              0xFF  // Capability Pointer Power

#define R_PCH_LPSS_DMAC_INTR                      0x3C  // Interrupt
#define B_PCH_LPSS_DMAC_INTR_ML                   0xFF000000 // Max Latency
#define B_PCH_LPSS_DMAC_INTR_MG                   0x00FF0000
#define B_PCH_LPSS_DMAC_INTR_IP                   0x00000F00 // Interrupt Pin
#define B_PCH_LPSS_DMAC_INTR_IL                   0x000000FF // Interrupt Line

#define R_PCH_LPSS_DMAC_PCAPID                    0x80  // Power Capability ID
#define B_PCH_LPSS_DMAC_PCAPID_PS                 0xF8000000 // PME Support
#define B_PCH_LPSS_DMAC_PCAPID_VS                 0x00070000 // Version
#define B_PCH_LPSS_DMAC_PCAPID_NC                 0x0000FF00 // Next Capability
#define B_PCH_LPSS_DMAC_PCAPID_PC                 0x000000FF // Power Capability

#define R_PCH_LPSS_DMAC_PCS                       0x84  // PME Control Status
#define B_PCH_LPSS_DMAC_PCS_PMESTS                BIT15 // PME Status
#define B_PCH_LPSS_DMAC_PCS_PMEEN                 BIT8  // PME Enable
#define B_PCH_LPSS_DMAC_PCS_NSS                   BIT3  // No Soft Reset
#define B_PCH_LPSS_DMAC_PCS_PS                    (BIT1 | BIT0) // Power State

#define R_PCH_LPSS_DMAC_MANID                     0xF8  // Manufacturer ID
#define B_PCH_LPSS_DMAC_MANID_MANID               0xFFFFFFFF // Manufacturer ID


//
// LPSS I2C Module
// PCI Config Space Registers
//
#define PCI_DEVICE_NUMBER_PCH_LPSS_I2C            24
#define PCI_FUNCTION_NUMBER_PCH_LPSS_I2C0         1
#define PCI_FUNCTION_NUMBER_PCH_LPSS_I2C1         2
#define PCI_FUNCTION_NUMBER_PCH_LPSS_I2C2         3
#define PCI_FUNCTION_NUMBER_PCH_LPSS_I2C3         4
#define PCI_FUNCTION_NUMBER_PCH_LPSS_I2C4         5
#define PCI_FUNCTION_NUMBER_PCH_LPSS_I2C5         6
#define PCI_FUNCTION_NUMBER_PCH_LPSS_I2C6         7

#define R_PCH_LPSS_I2C_DEVVENDID                  0x00  // Device ID & Vendor ID
#define B_PCH_LPSS_I2C_DEVVENDID_DID              0xFFFF0000 // Device ID
#define B_PCH_LPSS_I2C_DEVVENDID_VID              0x0000FFFF // Vendor ID

#define R_PCH_LPSS_I2C_STSCMD                     0x04  // Status & Command
#define B_PCH_LPSS_I2C_STSCMD_RMA                 BIT29 // RMA
#define B_PCH_LPSS_I2C_STSCMD_RCA                 BIT28 // RCA
#define B_PCH_LPSS_I2C_STSCMD_CAPLIST             BIT20 // Capability List
#define B_PCH_LPSS_I2C_STSCMD_INTRSTS             BIT19 // Interrupt Status
#define B_PCH_LPSS_I2C_STSCMD_INTRDIS             BIT10 // Interrupt Disable
#define B_PCH_LPSS_I2C_STSCMD_SERREN              BIT8  // SERR# Enable
#define B_PCH_LPSS_I2C_STSCMD_BME                 BIT2  // Bus Master Enable
#define B_PCH_LPSS_I2C_STSCMD_MSE                 BIT1  // Memory Space Enable

#define R_PCH_LPSS_I2C_REVCC                      0x08  // Revision ID & Class Code
#define B_PCH_LPSS_I2C_REVCC_CC                   0xFFFFFF00 // Class Code
#define B_PCH_LPSS_I2C_REVCC_RID                  0x000000FF // Revision ID

#define R_PCH_LPSS_I2C_CLHB                       0x0C
#define B_PCH_LPSS_I2C_CLHB_MULFNDEV              BIT23
#define B_PCH_LPSS_I2C_CLHB_HT                    0x007F0000 // Header Type
#define B_PCH_LPSS_I2C_CLHB_LT                    0x0000FF00 // Latency Timer
#define B_PCH_LPSS_I2C_CLHB_CLS                   0x000000FF // Cache Line Size

#define R_PCH_LPSS_I2C_BAR                        0x10  // BAR
#define B_PCH_LPSS_I2C_BAR_BA                     0xFFFFF000 // Base Address
#define V_PCH_LPSS_I2C_BAR_SIZE                   0x1000
#define N_PCH_LPSS_I2C_BAR_ALIGNMENT              12
#define B_PCH_LPSS_I2C_BAR_SI                     0x00000FF0 // Size Indicator
#define B_PCH_LPSS_I2C_BAR_PF                     BIT3  // Prefetchable
#define B_PCH_LPSS_I2C_BAR_TYPE                   (BIT2 | BIT1) // Type
#define B_PCH_LPSS_I2C_BAR_MS                     BIT0  // Message Space

#define R_PCH_LPSS_I2C_BAR1                       0x14  // BAR 1
#define B_PCH_LPSS_I2C_BAR1_BA                    0xFFFFF000 // Base Address
#define B_PCH_LPSS_I2C_BAR1_SI                    0x00000FF0 // Size Indicator
#define B_PCH_LPSS_I2C_BAR1_PF                    BIT3  // Prefetchable
#define B_PCH_LPSS_I2C_BAR1_TYPE                  (BIT2 | BIT1) // Type
#define B_PCH_LPSS_I2C_BAR1_MS                    BIT0  // Message Space

#define R_PCH_LPSS_I2C_SSID                       0x2C  // Sub System ID
#define B_PCH_LPSS_I2C_SSID_SID                   0xFFFF0000 // Sub System ID
#define B_PCH_LPSS_I2C_SSID_SVID                  0x0000FFFF // Sub System Vendor ID

#define R_PCH_LPSS_I2C_ERBAR                      0x30  // Expansion ROM BAR
#define B_PCH_LPSS_I2C_ERBAR_BA                   0xFFFFFFFF // Expansion ROM Base Address

#define R_PCH_LPSS_I2C_CAPPTR                     0x34  // Capability Pointer
#define B_PCH_LPSS_I2C_CAPPTR_CPPWR               0xFF  // Capability Pointer Power

#define R_PCH_LPSS_I2C_INTR                       0x3C  // Interrupt
#define B_PCH_LPSS_I2C_INTR_ML                    0xFF000000 // Max Latency
#define B_PCH_LPSS_I2C_INTR_MG                    0x00FF0000
#define B_PCH_LPSS_I2C_INTR_IP                    0x00000F00 // Interrupt Pin
#define B_PCH_LPSS_I2C_INTR_IL                    0x000000FF // Interrupt Line

#define R_PCH_LPSS_I2C_PCAPID                     0x80  // Power Capability ID
#define B_PCH_LPSS_I2C_PCAPID_PS                  0xF8000000 // PME Support
#define B_PCH_LPSS_I2C_PCAPID_VS                  0x00070000 // Version
#define B_PCH_LPSS_I2C_PCAPID_NC                  0x0000FF00 // Next Capability
#define B_PCH_LPSS_I2C_PCAPID_PC                  0x000000FF // Power Capability

#define R_PCH_LPSS_I2C_PCS                        0x84  // PME Control Status
#define B_PCH_LPSS_I2C_PCS_PMESTS                 BIT15 // PME Status
#define B_PCH_LPSS_I2C_PCS_PMEEN                  BIT8  // PME Enable
#define B_PCH_LPSS_I2C_PCS_NSS                    BIT3  // No Soft Reset
#define B_PCH_LPSS_I2C_PCS_PS                     (BIT1 | BIT0) // Power State

#define R_PCH_LPSS_I2C_MANID                      0xF8  // Manufacturer ID
#define B_PCH_LPSS_I2C_MANID_MANID                0xFFFFFFFF // Manufacturer ID

//
// LPSS I2C Module
// Memory Space Registers
//
#define R_PCH_LPSS_I2C_MEM_RESETS                 0x804 // Software Reset
#define B_PCH_LPSS_I2C_MEM_RESETS_FUNC            BIT1  // Function Clock Domain Reset
#define B_PCH_LPSS_I2C_MEM_RESETS_APB             BIT0  // APB Domain Reset

//
// LPSS PWM Modules
// PCI Config Space Registers
//
#define PCI_DEVICE_NUMBER_PCH_LPSS_PWM            30
#define PCI_FUNCTION_NUMBER_PCH_LPSS_PWM0         1
#define PCI_FUNCTION_NUMBER_PCH_LPSS_PWM1         2

#define R_PCH_LPSS_PWM_DEVVENDID                  0x00  // Device ID & Vendor ID
#define B_PCH_LPSS_PWM_DEVVENDID_DID              0xFFFF0000 // Device ID
#define B_PCH_LPSS_PWM_DEVVENDID_VID              0x0000FFFF // Vendor ID

#define R_PCH_LPSS_PWM_STSCMD                     0x04  // Status & Command
#define B_PCH_LPSS_PWM_STSCMD_RMA                 BIT29 // RMA
#define B_PCH_LPSS_PWM_STSCMD_RCA                 BIT28 // RCA
#define B_PCH_LPSS_PWM_STSCMD_CAPLIST             BIT20 // Capability List
#define B_PCH_LPSS_PWM_STSCMD_INTRSTS             BIT19 // Interrupt Status
#define B_PCH_LPSS_PWM_STSCMD_INTRDIS             BIT10 // Interrupt Disable
#define B_PCH_LPSS_PWM_STSCMD_SERREN              BIT8  // SERR# Enable
#define B_PCH_LPSS_PWM_STSCMD_BME                 BIT2  // Bus Master Enable
#define B_PCH_LPSS_PWM_STSCMD_MSE                 BIT1  // Memory Space Enable

#define R_PCH_LPSS_PWM_REVCC                      0x08  // Revision ID & Class Code
#define B_PCH_LPSS_PWM_REVCC_CC                   0xFFFFFF00 // Class Code
#define B_PCH_LPSS_PWM_REVCC_RID                  0x000000FF // Revision ID

#define R_PCH_LPSS_PWM_CLHB                       0x0C
#define B_PCH_LPSS_PWM_CLHB_MULFNDEV              BIT23
#define B_PCH_LPSS_PWM_CLHB_HT                    0x007F0000 // Header Type
#define B_PCH_LPSS_PWM_CLHB_LT                    0x0000FF00 // Latency Timer
#define B_PCH_LPSS_PWM_CLHB_CLS                   0x000000FF // Cache Line Size

#define R_PCH_LPSS_PWM_BAR                        0x10  // BAR
#define B_PCH_LPSS_PWM_BAR_BA                     0xFFFFF000 // Base Address
#define V_PCH_LPSS_PWM_BAR_SIZE                   0x1000
#define N_PCH_LPSS_PWM_BAR_ALIGNMENT              12
#define B_PCH_LPSS_PWM_BAR_SI                     0x00000FF0 // Size Indicator
#define B_PCH_LPSS_PWM_BAR_PF                     BIT3  // Prefetchable
#define B_PCH_LPSS_PWM_BAR_TYPE                   (BIT2 | BIT1) // Type
#define B_PCH_LPSS_PWM_BAR_MS                     BIT0  // Message Space

#define R_PCH_LPSS_PWM_BAR1                       0x14  // BAR 1
#define B_PCH_LPSS_PWM_BAR1_BA                    0xFFFFF000 // Base Address
#define B_PCH_LPSS_PWM_BAR1_SI                    0x00000FF0 // Size Indicator
#define B_PCH_LPSS_PWM_BAR1_PF                    BIT3  // Prefetchable
#define B_PCH_LPSS_PWM_BAR1_TYPE                  (BIT2 | BIT1) // Type
#define B_PCH_LPSS_PWM_BAR1_MS                    BIT0  // Message Space

#define R_PCH_LPSS_PWM_SSID                       0x2C  // Sub System ID
#define B_PCH_LPSS_PWM_SSID_SID                   0xFFFF0000 // Sub System ID
#define B_PCH_LPSS_PWM_SSID_SVID                  0x0000FFFF // Sub System Vendor ID

#define R_PCH_LPSS_PWM_ERBAR                      0x30  // Expansion ROM BAR
#define B_PCH_LPSS_PWM_ERBAR_BA                   0xFFFFFFFF // Expansion ROM Base Address

#define R_PCH_LPSS_PWM_CAPPTR                     0x34  // Capability Pointer
#define B_PCH_LPSS_PWM_CAPPTR_CPPWR               0xFF  // Capability Pointer Power

#define R_PCH_LPSS_PWM_INTR                       0x3C  // Interrupt
#define B_PCH_LPSS_PWM_INTR_ML                    0xFF000000 // Max Latency
#define B_PCH_LPSS_PWM_INTR_MG                    0x00FF0000
#define B_PCH_LPSS_PWM_INTR_IP                    0x00000F00 // Interrupt Pin
#define B_PCH_LPSS_PWM_INTR_IL                    0x000000FF // Interrupt Line

#define R_PCH_LPSS_PWM_PCAPID                     0x80  // Power Capability ID
#define B_PCH_LPSS_PWM_PCAPID_PS                  0xF8000000 // PME Support
#define B_PCH_LPSS_PWM_PCAPID_VS                  0x00070000 // Version
#define B_PCH_LPSS_PWM_PCAPID_NC                  0x0000FF00 // Next Capability
#define B_PCH_LPSS_PWM_PCAPID_PC                  0x000000FF // Power Capability

#define R_PCH_LPSS_PWM_PCS                        0x84  // PME Control Status
#define B_PCH_LPSS_PWM_PCS_PMESTS                 BIT15 // PME Status
#define B_PCH_LPSS_PWM_PCS_PMEEN                  BIT8  // PME Enable
#define B_PCH_LPSS_PWM_PCS_NSS                    BIT3  // No Soft Reset
#define B_PCH_LPSS_PWM_PCS_PS                     (BIT1 | BIT0) // Power State

#define R_PCH_LPSS_PWM_MANID                      0xF8  // Manufacturer ID
#define B_PCH_LPSS_PWM_MANID_MANID                0xFFFFFFFF // Manufacturer ID

//
// LPSS PWM Module
// Memory Space Registers
//
#define R_PCH_LPSS_PWM_MEM_RESETS                 0x804 // Software Reset
#define B_PCH_LPSS_PWM_MEM_RESETS_FUNC            BIT1  // Function Clock Domain Reset
#define B_PCH_LPSS_PWM_MEM_RESETS_APB             BIT0  // APB Domain Reset

//
// LPSS HSUART Modules
// PCI Config Space Registers
//
#define PCI_DEVICE_NUMBER_PCH_LPSS_HSUART         30
#define PCI_FUNCTION_NUMBER_PCH_LPSS_HSUART0      3
#define PCI_FUNCTION_NUMBER_PCH_LPSS_HSUART1      4

#define R_PCH_LPSS_HSUART_DEVVENDID               0x00  // Device ID & Vendor ID
#define B_PCH_LPSS_HSUART_DEVVENDID_DID           0xFFFF0000 // Device ID
#define B_PCH_LPSS_HSUART_DEVVENDID_VID           0x0000FFFF // Vendor ID

#define R_PCH_LPSS_HSUART_STSCMD                  0x04  // Status & Command
#define B_PCH_LPSS_HSUART_STSCMD_RMA              BIT29 // RMA
#define B_PCH_LPSS_HSUART_STSCMD_RCA              BIT28 // RCA
#define B_PCH_LPSS_HSUART_STSCMD_CAPLIST          BIT20 // Capability List
#define B_PCH_LPSS_HSUART_STSCMD_INTRSTS          BIT19 // Interrupt Status
#define B_PCH_LPSS_HSUART_STSCMD_INTRDIS          BIT10 // Interrupt Disable
#define B_PCH_LPSS_HSUART_STSCMD_SERREN           BIT8  // SERR# Enable
#define B_PCH_LPSS_HSUART_STSCMD_BME              BIT2  // Bus Master Enable
#define B_PCH_LPSS_HSUART_STSCMD_MSE              BIT1  // Memory Space Enable

#define R_PCH_LPSS_HSUART_REVCC                   0x08  // Revision ID & Class Code
#define B_PCH_LPSS_HSUART_REVCC_CC                0xFFFFFF00 // Class Code
#define B_PCH_LPSS_HSUART_REVCC_RID               0x000000FF // Revision ID

#define R_PCH_LPSS_HSUART_CLHB                    0x0C
#define B_PCH_LPSS_HSUART_CLHB_MULFNDEV           BIT23
#define B_PCH_LPSS_HSUART_CLHB_HT                 0x007F0000 // Header Type
#define B_PCH_LPSS_HSUART_CLHB_LT                 0x0000FF00 // Latency Timer
#define B_PCH_LPSS_HSUART_CLHB_CLS                0x000000FF // Cache Line Size

#define R_PCH_LPSS_HSUART_BAR                     0x10  // BAR
#define B_PCH_LPSS_HSUART_BAR_BA                  0xFFFFF000 // Base Address
#define V_PCH_LPSS_HSUART_BAR_SIZE                0x1000
#define N_PCH_LPSS_HSUART_BAR_ALIGNMENT           12
#define B_PCH_LPSS_HSUART_BAR_SI                  0x00000FF0 // Size Indicator
#define B_PCH_LPSS_HSUART_BAR_PF                  BIT3  // Prefetchable
#define B_PCH_LPSS_HSUART_BAR_TYPE                (BIT2 | BIT1) // Type
#define B_PCH_LPSS_HSUART_BAR_MS                  BIT0  // Message Space

#define R_PCH_LPSS_HSUART_BAR1                    0x14  // BAR 1
#define B_PCH_LPSS_HSUART_BAR1_BA                 0xFFFFF000 // Base Address
#define B_PCH_LPSS_HSUART_BAR1_SI                 0x00000FF0 // Size Indicator
#define B_PCH_LPSS_HSUART_BAR1_PF                 BIT3  // Prefetchable
#define B_PCH_LPSS_HSUART_BAR1_TYPE               (BIT2 | BIT1) // Type
#define B_PCH_LPSS_HSUART_BAR1_MS                 BIT0  // Message Space

#define R_PCH_LPSS_HSUART_SSID                    0x2C  // Sub System ID
#define B_PCH_LPSS_HSUART_SSID_SID                0xFFFF0000 // Sub System ID
#define B_PCH_LPSS_HSUART_SSID_SVID               0x0000FFFF // Sub System Vendor ID

#define R_PCH_LPSS_HSUART_ERBAR                   0x30  // Expansion ROM BAR
#define B_PCH_LPSS_HSUART_ERBAR_BA                0xFFFFFFFF // Expansion ROM Base Address

#define R_PCH_LPSS_HSUART_CAPPTR                  0x34  // Capability Pointer
#define B_PCH_LPSS_HSUART_CAPPTR_CPPWR            0xFF  // Capability Pointer Power

#define R_PCH_LPSS_HSUART_INTR                    0x3C  // Interrupt
#define B_PCH_LPSS_HSUART_INTR_ML                 0xFF000000 // Max Latency
#define B_PCH_LPSS_HSUART_INTR_MG                 0x00FF0000
#define B_PCH_LPSS_HSUART_INTR_IP                 0x00000F00 // Interrupt Pin
#define B_PCH_LPSS_HSUART_INTR_IL                 0x000000FF // Interrupt Line

#define R_PCH_LPSS_HSUART_PCAPID                  0x80  // Power Capability ID
#define B_PCH_LPSS_HSUART_PCAPID_PS               0xF8000000 // PME Support
#define B_PCH_LPSS_HSUART_PCAPID_VS               0x00070000 // Version
#define B_PCH_LPSS_HSUART_PCAPID_NC               0x0000FF00 // Next Capability
#define B_PCH_LPSS_HSUART_PCAPID_PC               0x000000FF // Power Capability

#define R_PCH_LPSS_HSUART_PCS                     0x84  // PME Control Status
#define B_PCH_LPSS_HSUART_PCS_PMESTS              BIT15 // PME Status
#define B_PCH_LPSS_HSUART_PCS_PMEEN               BIT8  // PME Enable
#define B_PCH_LPSS_HSUART_PCS_NSS                 BIT3  // No Soft Reset
#define B_PCH_LPSS_HSUART_PCS_PS                  (BIT1 | BIT0) // Power State

#define R_PCH_LPSS_HSUART_MANID                   0xF8  // Manufacturer ID
#define B_PCH_LPSS_HSUART_MANID_MANID             0xFFFFFFFF // Manufacturer ID

//
// LPSS HSUART Module
// Memory Space Registers
//
#define R_PCH_LPSS_HSUART_MEM_PCP                 0x800 // Private Clock Parameters
#define B_PCH_LPSS_HSUART_MEM_PCP_CLKUPDATE       BIT31 // Clock Divider Update
#define B_PCH_LPSS_HSUART_MEM_PCP_NVAL            0x7FFF0000 // N value for the M over N divider
#define B_PCH_LPSS_HSUART_MEM_PCP_MVAL            0x0000FFFE // M value for the M over N divider
#define B_PCH_LPSS_HSUART_MEM_PCP_CLKEN           BIT0  // Clock Enable

#define R_PCH_LPSS_HSUART_MEM_RESETS              0x804 // Software Reset
#define B_PCH_LPSS_HSUART_MEM_RESETS_FUNC         BIT1  // Function Clock Domain Reset
#define B_PCH_LPSS_HSUART_MEM_RESETS_APB          BIT0  // APB Domain Reset

//
// LPSS SPI Module
// PCI Config Space Registers
//
#define PCI_DEVICE_NUMBER_PCH_LPSS_SPI            30
#define PCI_FUNCTION_NUMBER_PCH_LPSS_SPI          5

#define R_PCH_LPSS_SPI_DEVVENDID                  0x00  // Device ID & Vendor ID
#define B_PCH_LPSS_SPI_DEVVENDID_DID              0xFFFF0000 // Device ID
#define B_PCH_LPSS_SPI_DEVVENDID_VID              0x0000FFFF // Vendor ID

#define R_PCH_LPSS_SPI_STSCMD                     0x04  // Status & Command
#define B_PCH_LPSS_SPI_STSCMD_RMA                 BIT29 // RMA
#define B_PCH_LPSS_SPI_STSCMD_RCA                 BIT28 // RCA
#define B_PCH_LPSS_SPI_STSCMD_CAPLIST             BIT20 // Capability List
#define B_PCH_LPSS_SPI_STSCMD_INTRSTS             BIT19 // Interrupt Status
#define B_PCH_LPSS_SPI_STSCMD_INTRDIS             BIT10 // Interrupt Disable
#define B_PCH_LPSS_SPI_STSCMD_SERREN              BIT8  // SERR# Enable
#define B_PCH_LPSS_SPI_STSCMD_BME                 BIT2  // Bus Master Enable
#define B_PCH_LPSS_SPI_STSCMD_MSE                 BIT1  // Memory Space Enable

#define R_PCH_LPSS_SPI_REVCC                      0x08  // Revision ID & Class Code
#define B_PCH_LPSS_SPI_REVCC_CC                   0xFFFFFF00 // Class Code
#define B_PCH_LPSS_SPI_REVCC_RID                  0x000000FF // Revision ID

#define R_PCH_LPSS_SPI_CLHB                       0x0C
#define B_PCH_LPSS_SPI_CLHB_MULFNDEV              BIT23
#define B_PCH_LPSS_SPI_CLHB_HT                    0x007F0000 // Header Type
#define B_PCH_LPSS_SPI_CLHB_LT                    0x0000FF00 // Latency Timer
#define B_PCH_LPSS_SPI_CLHB_CLS                   0x000000FF // Cache Line Size

#define R_PCH_LPSS_SPI_BAR                        0x10  // BAR
#define B_PCH_LPSS_SPI_BAR_BA                     0xFFFFF000 // Base Address
#define V_PCH_LPSS_SPI_BAR_SIZE                   0x1000
#define N_PCH_LPSS_SPI_BAR_ALIGNMENT              12
#define B_PCH_LPSS_SPI_BAR_SI                     0x00000FF0 // Size Indicator
#define B_PCH_LPSS_SPI_BAR_PF                     BIT3  // Prefetchable
#define B_PCH_LPSS_SPI_BAR_TYPE                   (BIT2 | BIT1) // Type
#define B_PCH_LPSS_SPI_BAR_MS                     BIT0  // Message Space

#define R_PCH_LPSS_SPI_BAR1                       0x14  // BAR 1
#define B_PCH_LPSS_SPI_BAR1_BA                    0xFFFFF000 // Base Address
#define B_PCH_LPSS_SPI_BAR1_SI                    0x00000FF0 // Size Indicator
#define B_PCH_LPSS_SPI_BAR1_PF                    BIT3  // Prefetchable
#define B_PCH_LPSS_SPI_BAR1_TYPE                  (BIT2 | BIT1) // Type
#define B_PCH_LPSS_SPI_BAR1_MS                    BIT0  // Message Space

#define R_PCH_LPSS_SPI_SSID                       0x2C  // Sub System ID
#define B_PCH_LPSS_SPI_SSID_SID                   0xFFFF0000 // Sub System ID
#define B_PCH_LPSS_SPI_SSID_SVID                  0x0000FFFF // Sub System Vendor ID

#define R_PCH_LPSS_SPI_ERBAR                      0x30  // Expansion ROM BAR
#define B_PCH_LPSS_SPI_ERBAR_BA                   0xFFFFFFFF // Expansion ROM Base Address

#define R_PCH_LPSS_SPI_CAPPTR                     0x34  // Capability Pointer
#define B_PCH_LPSS_SPI_CAPPTR_CPPWR               0xFF  // Capability Pointer Power

#define R_PCH_LPSS_SPI_INTR                       0x3C  // Interrupt
#define B_PCH_LPSS_SPI_INTR_ML                    0xFF000000 // Max Latency
#define B_PCH_LPSS_SPI_INTR_MG                    0x00FF0000
#define B_PCH_LPSS_SPI_INTR_IP                    0x00000F00 // Interrupt Pin
#define B_PCH_LPSS_SPI_INTR_IL                    0x000000FF // Interrupt Line

#define R_PCH_LPSS_SPI_PCAPID                     0x80  // Power Capability ID
#define B_PCH_LPSS_SPI_PCAPID_PS                  0xF8000000 // PME Support
#define B_PCH_LPSS_SPI_PCAPID_VS                  0x00070000 // Version
#define B_PCH_LPSS_SPI_PCAPID_NC                  0x0000FF00 // Next Capability
#define B_PCH_LPSS_SPI_PCAPID_PC                  0x000000FF // Power Capability

#define R_PCH_LPSS_SPI_PCS                        0x84  // PME Control Status
#define B_PCH_LPSS_SPI_PCS_PMESTS                 BIT15 // PME Status
#define B_PCH_LPSS_SPI_PCS_PMEEN                  BIT8  // PME Enable
#define B_PCH_LPSS_SPI_PCS_NSS                    BIT3  // No Soft Reset
#define B_PCH_LPSS_SPI_PCS_PS                     (BIT1 | BIT0) // Power State

#define R_PCH_LPSS_SPI_MANID                      0xF8  // Manufacturer ID
#define B_PCH_LPSS_SPI_MANID_MANID                0xFFFFFFFF // Manufacturer ID

//
// LPSS SPI Module
// Memory Space Registers
//
#define R_PCH_LPSS_SPI_MEM_PCP                    0x400 // Private Clock Parameters
#define B_PCH_LPSS_SPI_MEM_PCP_CLKUPDATE          BIT31 // Clock Divider Update
#define B_PCH_LPSS_SPI_MEM_PCP_NVAL               0x7FFF0000 // N value for the M over N divider
#define B_PCH_LPSS_SPI_MEM_PCP_MVAL               0x0000FFFE // M value for the M over N divider
#define B_PCH_LPSS_SPI_MEM_PCP_CLKEN              BIT0  // Clock Enable

#define R_PCH_LPSS_SPI_MEM_RESETS                 0x404 // Software Reset
#define B_PCH_LPSS_SPI_MEM_RESETS_FUNC            BIT1  // Function Clock Domain Reset
#define B_PCH_LPSS_SPI_MEM_RESETS_APB             BIT0  // APB Domain Reset

#endif
