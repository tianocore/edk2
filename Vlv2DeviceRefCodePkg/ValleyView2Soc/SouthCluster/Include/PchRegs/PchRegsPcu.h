/*++

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  PchRegsPcu.h

Abstract:

  Register names for VLV PCU device.

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
#ifndef _PCH_REGS_LPC_H_
#define _PCH_REGS_LPC_H_

//
// VLV PCU Registers (D31:F0)
//
#define PCI_DEVICE_NUMBER_PCH_LPC                 31
#define PCI_FUNCTION_NUMBER_PCH_LPC               0

// Silicon Steppings
typedef enum {
  PchA0         = 0,
  PchA1         = 1,
  PchB0         = 2,
  PchB1         = 3,
  PchB2         = 4,
  PchB3         = 5,
  PchC0         = 6,
  PchD0         = 7,
  PchSteppingMax
} PCH_STEPPING;

#define R_PCH_LPC_REG_ID                          0x00  // Identifiers Register
#define B_PCH_LPC_DEVICE_ID                       0xFFFF0000 // Device Identification
#define B_PCH_LPC_VENDOR_ID                       0x0000FFFF // Vendor Identification
#define V_PCH_LPC_VENDOR_ID                       V_PCH_INTEL_VENDOR_ID // Vendor ID for Intel
//
// General VLV PCU Device ID
//
#define V_PCH_LPC_DEVICE_ID_0                     0x0F1C
#define V_PCH_LPC_DEVICE_ID_1                     0x0F1D
#define V_PCH_LPC_DEVICE_ID_2                     0x0F1E
#define V_PCH_LPC_DEVICE_ID_3                     0x0F1F

#define R_PCH_LPC_COMMAND                         0x04  // Command
#define B_PCH_LPC_COMMAND_ID                      BIT10 // Interrupt Disable
#define B_PCH_LPC_COMMAND_FBE                     BIT9  // Fast Back to Back Enable
#define B_PCH_LPC_COMMAND_SERR_EN                 BIT8  // SERR# Enable
#define B_PCH_LPC_COMMAND_WCC                     BIT7  // Wait Cycle Control
#define B_PCH_LPC_COMMAND_PER                     BIT6  // Parity Error Response Enable
#define B_PCH_LPC_COMMAND_VPS                     BIT5  // VGA Palette Snoop
#define B_PCH_LPC_COMMAND_MWIE                    BIT4  // Memory Write and Invalidate Enable
#define B_PCH_LPC_COMMAND_SCE                     BIT3  // Special Cycle Enable
#define B_PCH_LPC_COMMAND_BME                     BIT2  // Bus Master Enable
#define B_PCH_LPC_COMMAND_MSE                     BIT1  // Memory Space Enable
#define B_PCH_LPC_COMMAND_IOSE                    BIT0  // I/O Space Enable

#define R_PCH_LPC_DEV_STS                         0x06  // Status
#define B_PCH_LPC_DEV_STS_DPE                     BIT15 // Detected Parity Error
#define B_PCH_LPC_DEV_STS_SSE                     BIT14 // Signaled System Error
#define B_PCH_LPC_DEV_STS_RMA                     BIT13 // Received Master Abort
#define B_PCH_LPC_DEV_STS_RTA                     BIT12 // Received Target Abort
#define B_PCH_LPC_DEV_STS_STA                     BIT11 // Signaled Target Abort
#define B_PCH_LPC_DEV_STS_DEVT_STS                (BIT10 | BIT9) // DEVSEL# Timing Status
#define B_PCH_LPC_DEV_STS_MDPED                   BIT8  // Data Parity Error
#define B_PCH_LPC_DEV_STS_FB2B                    BIT7  // Fast Back to Back Capable
#define B_PCH_LPC_DEV_STS_66MHZ_CAP               BIT5  // 66 MHz capable
#define B_PCH_LPC_DEV_STS_CAP_LIST                BIT4  // Capabilities List
#define B_PCH_LPC_DEV_STS_INT_STS                 BIT3  // Interrupt Status

#define R_PCH_LPC_RID_CC                          0x08  // Revision ID & Class Code
#define B_PCH_LPC_RID_CC_BCC                      0xFF000000 // Base Class Code
#define B_PCH_LPC_RID_CC_SCC                      0x00FF0000 // Sub-Class Code
#define B_PCH_LPC_RID_CC_PI                       0x0000FF00 // Programming Interface
#define B_PCH_LPC_RID_CC_RID                      0x000000FF // Revision ID

// Silicon Steppings

#define V_PCH_LPC_RID_0                           0x01  // A0 Stepping (17 x 17)
#define V_PCH_LPC_RID_1                           0x02  // A0 Stepping (25 x 27)
#define V_PCH_LPC_RID_2                           0x03  // A1 Stepping (17 x 17)
#define V_PCH_LPC_RID_3                           0x04  // A1 Stepping (25 x 27)
#define V_PCH_LPC_RID_4                           0x05  // B0 Stepping (17 x 17)
#define V_PCH_LPC_RID_5                           0x06  // B0 Stepping (25 x 27)
#define V_PCH_LPC_RID_6                           0x07  // B1 Stepping (17 x 17)
#define V_PCH_LPC_RID_7                           0x08  // B1 Stepping (25 x 27)
#define V_PCH_LPC_RID_8                           0x09  // B2 Stepping (17 x 17)
#define V_PCH_LPC_RID_9                           0x0A  // B2 Stepping (25 x 27)
#define V_PCH_LPC_RID_A                           0x0B  // B3 Stepping (17 x 17)
#define V_PCH_LPC_RID_B                           0x0C  // B3 Stepping (25 x 27)
#define V_PCH_LPC_RID_C                           0x0D  // C0 Stepping (17 x 17)
#define V_PCH_LPC_RID_D                           0x0E  // C0 Stepping (25 x 27)
#define V_PCH_LPC_RID_E                           0x10  // D0 Stepping (17 x 17)
#define V_PCH_LPC_RID_F                           0x11  // D0 Stepping (25 x 27)

#define R_PCH_LPC_MLT                             0x0D  // Master Latency Timer
#define B_PCH_LPC_MLT_MLC                         0xF8  // Master Latency Count

#define R_PCH_LPC_HEADTYP                         0x0E  // Header Type
#define B_PCH_LPC_HEADTYP_MFD                     BIT7  // Multi-function Device
#define B_PCH_LPC_HEADTYP_HT                      0x7F  // Header Type

#define R_PCH_LPC_SS                              0x2C  // Subsystem ID & Vendor ID
#define B_PCH_LPC_SS_SSID                         0xFFFF0000 // Subsystem ID
#define B_PCH_LPC_SS_SSVID                        0x0000FFFF // Subsystem Vendor ID

#define R_PCH_LPC_CAP_LIST                        0x34  // Capability List
#define B_PCH_LPC_CAP_LIST_CP                     0xFF  // Capability Pointer

#define R_PCH_LPC_ACPI_BASE                       0x40 // ABASE, 16bit
#define B_PCH_LPC_ACPI_BASE_BAR                   0x0000FF80 // Base Address, 128 Bytes
#define B_PCH_LPC_ACPI_BASE_EN                    BIT1 // Enable Bit
#define B_PCH_LPC_ACPI_BASE_MEMI                  BIT0 // Memory Space Indication

#define R_PCH_LPC_PMC_BASE                        0x44  // PBASE, 32bit, 512 Bytes
#define B_PCH_LPC_PMC_BASE_BAR                    0xFFFFFE00 // Base Address
#define B_PCH_LPC_PMC_BASE_PREF                   BIT3  // Prefetchable
#define B_PCH_LPC_PMC_BASE_ADDRNG                 BIT2  // Address Range
#define B_PCH_LPC_PMC_BASE_EN                     BIT1  // Enable Bit
#define B_PCH_LPC_PMC_BASE_MEMI                   BIT0  // Memory Space Indication

#define R_PCH_LPC_GPIO_BASE                       0x48  // GBASE, 16bit
#define B_PCH_LPC_GPIO_BASE_BAR                   0xFF00 // Base Address, 256 Bytes
#define B_PCH_LPC_GPIO_BASE_EN                    BIT1  // Enable Bit
#define B_PCH_LPC_GPIO_BASE_MEMI                  BIT0  // Memory Space Indication

#define R_PCH_LPC_IO_BASE                         0x4C  // IOBASE, 32bit
#define B_PCH_LPC_IO_BASE_BAR                     0xFFFFC000 // Base Address, 16 KiloBytes
#define B_PCH_LPC_IO_BASE_PREF                    BIT3  // Prefetchable
#define B_PCH_LPC_IO_BASE_ADDRNG                  BIT2  // Address Range
#define B_PCH_LPC_IO_BASE_EN                      BIT1  // Enable Bit
#define B_PCH_LPC_IO_BASE_MEMI                    BIT0  // Memory Space Indication

#define R_PCH_LPC_ILB_BASE                        0x50  // IBASE, 32bit
#define B_PCH_LPC_ILB_BASE_BAR                    0xFFFFFE00 // Base Address, 512 bytes
#define B_PCH_LPC_ILB_BASE_PREF                   BIT3  // Prefetchable
#define B_PCH_LPC_ILB_BASE_ADDRNG                 BIT2  // Address Range
#define B_PCH_LPC_ILB_BASE_EN                     BIT1  // Enable Bit
#define B_PCH_LPC_ILB_BASE_MEMI                   BIT0  // Memory Space Indication

#define R_PCH_LPC_SPI_BASE                        0x54  // SBASE, 32bit
#define B_PCH_LPC_SPI_BASE_BAR                    0xFFFFFE00 // Base Address, 512 bytes
#define B_PCH_LPC_SPI_BASE_PREF                   BIT3  // Prefetchable
#define B_PCH_LPC_SPI_BASE_ADDRNG                 BIT2  // Address Range
#define B_PCH_LPC_SPI_BASE_EN                     BIT1  // Enable Bit
#define B_PCH_LPC_SPI_BASE_MEMI                   BIT0  // Memory Space Indicator

#define R_PCH_LPC_MPHY_BASE                       0x58 // MPBASE, 32bit
#define B_PCH_LPC_MPHY_BASE_BAR                   0xFFF00000 // Base Address, 1 MegaByte
#define B_PCH_LPC_MPHY_BASE_PREF                  BIT3  // Prefetchable
#define B_PCH_LPC_MPHY_BASE_ADDRNG                BIT2  // Address Range
#define B_PCH_LPC_MPHY_BASE_EN                    BIT1  // Enable Bit
#define B_PCH_LPC_MPHY_BASE_MEMI                  BIT0  // Memory Space Indicator

#define R_PCH_LPC_PUNIT_BASE                      0x5C  // PUBASE, 32bit
#define B_PCH_LPC_PUNIT_BASE_BAR                  0xFFFFF800 // Base Address, 2K bytes
#define B_PCH_LPC_PUNIT_BASE_PREF                 BIT3  // Prefetchable
#define B_PCH_LPC_PUNIT_BASE_ADDRNG               BIT2  // Address Range
#define B_PCH_LPC_PUNIT_BASE_EN                   BIT1  // Enable Bit
#define B_PCH_LPC_PUNIT_BASE_MEMI                 BIT0  // Memory Space Indicator

#define R_PCH_LPC_UART_CTRL                       0x80  // UART Control
#define B_PCH_LPC_UART_CTRL_COM1_EN               BIT0  // COM1 Enable

#define R_PCH_LPC_FWH_BIOS_DEC                    0xD8  // BIOS Decode Enable
#define B_PCH_LPC_FWH_BIOS_DEC_EF8                BIT15 // F8-FF Enable
#define B_PCH_LPC_FWH_BIOS_DEC_EF0                BIT14 // F0-F8 Enable
#define B_PCH_LPC_FWH_BIOS_DEC_EE8                BIT13 // E8-EF Enable
#define B_PCH_LPC_FWH_BIOS_DEC_EE0                BIT12 // E0-E8 Enable
#define B_PCH_LPC_FWH_BIOS_DEC_ED8                BIT11 // D8-DF Enable
#define B_PCH_LPC_FWH_BIOS_DEC_ED0                BIT10 // D0-D8 Enable
#define B_PCH_LPC_FWH_BIOS_DEC_EC8                BIT9  // C8-CF Enable
#define B_PCH_LPC_FWH_BIOS_DEC_EC0                BIT8  // C0-C8 Enable
#define B_PCH_LPC_FWH_BIOS_DEC_LFE                BIT7  // Legacy F Segment Enable
#define B_PCH_LPC_FWH_BIOS_DEC_LEE                BIT6  // Legacy E Segment Enable
#define B_PCH_LPC_FWH_BIOS_DEC_E70                BIT3  // 70-7F Enable
#define B_PCH_LPC_FWH_BIOS_DEC_E60                BIT2  // 60-6F Enable
#define B_PCH_LPC_FWH_BIOS_DEC_E50                BIT1  // 50-5F Enable
#define B_PCH_LPC_FWH_BIOS_DEC_E40                BIT0  // 40-4F Enable

#define R_PCH_LPC_FDCAP                           0xE0  // Feature Detection Capability ID
#define B_PCH_LPC_FDCAP_NEXT                      0xFF00 // Next Capability
#define B_PCH_LPC_FDCAP_CAPID                     0x00FF // Capability ID

#define R_PCH_LPC_FDLEN                           0xE2  // Feature Detection Capability Length
#define B_PCH_LPC_FDLEN_CAPLEN                    0xFF  // Capability Length

#define R_PCH_LPC_FDVER                           0xE3  // Feature Detection Capability Version
#define B_PCH_LPC_FDVER_VSCID                     0xF0  // Vendor Specific Capability ID
#define B_PCH_LPC_FDVER_CAPVER                    0x0F  // Capability Version

#define R_PCH_LPC_FVECTIDX                        0xE4  // Feature Vector Index

#define R_PCH_LPC_FVECTD                          0xE8  // Feature Vector Data

#define R_PCH_LPC_RCBA                            0xF0  // RCBA, 32bit
#define B_PCH_LPC_RCBA_BAR                        0xFFFFFC00 // Base Address, 1 KiloByte
#define B_PCH_LPC_RCBA_EN                         BIT0  // Enable Bit

#define R_PCH_LPC_ULT_OBS                         0xF4  // ULT Observability
#define B_PCH_LPC_ULT_OBS_WNUM                    0x3FF000 // Reserved Wafer Number
#define B_PCH_LPC_ULT_OBS_XLOC                    0xFC0 // Reserved X Loc
#define B_PCH_LPC_ULT_OBS_YLOC                    0x3F  // Reserved Y Loc

#define R_PCH_LPC_MAN_ID                          0xF8  // Manufacturer ID
#define B_PCH_LPC_MAN_ID_DPID                     0xF000000 // Dot Portion of Process ID
#define B_PCH_LPC_MAN_ID_MSID                     0xFF0000 // Manufacturing Stepping Identifier
#define B_PCH_LPC_MAN_ID_MID                      0xFF00 // Manufacturing Identifier
#define B_PCH_LPC_MAN_ID_PPID                     0xFF  // Process Portion of Process ID

#define R_PCH_LPC_CGC                             0xFC  // Clock Gating Control
#define B_PCH_LPC_CGC_SBLCG                       BIT9  // IOSF-SB Local Clock Gating Disable
#define B_PCH_LPC_CGC_SBTCG                       BIT8  // IOSF-SB Trunk Clock Gating (Request) Disable
#define B_PCH_LPC_CGC_PRILCG                      BIT1  // IOSF-PRI Local Clock Gating Disable
#define B_PCH_LPC_CGC_PRITCG                      BIT0  // IOSF-PRI Trunk Clock Gating (Request) Disable

//
// iLB Memory Space Registers (IBASE)
//
#define R_PCH_ILB_ACPI_CNT                        0x00  // ACPI Control
#define B_PCH_ILB_ACPI_CNT_SCI_IRQ_SEL            (BIT2 | BIT1 | BIT0) // SCI IRQ Select
#define V_PCH_ILB_ACPI_CNT_SCI_IRQ_9              0     // IRQ9
#define V_PCH_ILB_ACPI_CNT_SCI_IRQ_10             BIT0  // IRQ10
#define V_PCH_ILB_ACPI_CNT_SCI_IRQ_11             BIT1  // IRQ11
#define V_PCH_ILB_ACPI_CNT_SCI_IRQ_20             BIT2  // IRQ20 (Only if APIC enabled)
#define V_PCH_ILB_ACPI_CNT_SCI_IRQ_21             (BIT2 | BIT0) // IRQ21 (Only if APIC enabled)
#define V_PCH_ILB_ACPI_CNT_SCI_IRQ_22             (BIT2 | BIT1) // IRQ22 (Only if APIC enabled)
#define V_PCH_ILB_ACPI_CNT_SCI_IRQ_23             (BIT2 | BIT1 | BIT0) // IRQ23 (Only if APIC enabled)

#define R_PCH_ILB_MC                              0x04  // Miscellaneous Control
#define B_PCH_ILB_MC_DRTC                         BIT3  // Disable RTC
#define B_PCH_ILB_MC_D8259                        BIT2  // Disable 8259
#define B_PCH_ILB_MC_D8254                        BIT1  // Disable 8254
#define B_PCH_ILB_MC_AME                          BIT0  // Alternate Access Mode Enable

#define R_PCH_ILB_PIRQA_ROUT                      0x08  // PIRQA Routing Control
#define R_PCH_ILB_PIRQB_ROUT                      0x09  // PIRQB Routing Control
#define R_PCH_ILB_PIRQC_ROUT                      0x0A  // PIRQC Routing Control
#define R_PCH_ILB_PIRQD_ROUT                      0x0B  // PIRQD Routing Control
#define R_PCH_ILB_PIRQE_ROUT                      0x0C  // PIRQE Routing Control
#define R_PCH_ILB_PIRQF_ROUT                      0x0D  // PIRQF Routing Control
#define R_PCH_ILB_PIRQG_ROUT                      0x0E  // PIRQG Routing Control
#define R_PCH_ILB_PIRQH_ROUT                      0x0F  // PIRQH Routing Control
//
// Bit values are the same for R_PCH_ILB_PIRQA_ROUT to R_PCH_ILB_PIRQH_ROUT
//
#define B_PCH_ILB_PIRQX_ROUT_IRQEN                BIT7  // Interrupt Routing Enable
#define B_PCH_ILB_PIRQX_ROUT                      0x0F  // IRQ Routing
#define V_PCH_ILB_PIRQX_ROUT_IRQ_3                0x03  // Route to IRQ3
#define V_PCH_ILB_PIRQX_ROUT_IRQ_4                0x04  // Route to IRQ4
#define V_PCH_ILB_PIRQX_ROUT_IRQ_5                0x05  // Route to IRQ5
#define V_PCH_ILB_PIRQX_ROUT_IRQ_6                0x06  // Route to IRQ6
#define V_PCH_ILB_PIRQX_ROUT_IRQ_7                0x07  // Route to IRQ7
#define V_PCH_ILB_PIRQX_ROUT_IRQ_9                0x09  // Route to IRQ9
#define V_PCH_ILB_PIRQX_ROUT_IRQ_10               0x0A  // Route to IRQ10
#define V_PCH_ILB_PIRQX_ROUT_IRQ_11               0x0B  // Route to IRQ11
#define V_PCH_ILB_PIRQX_ROUT_IRQ_12               0x0C  // Route to IRQ12
#define V_PCH_ILB_PIRQX_ROUT_IRQ_14               0x0E  // Route to IRQ14
#define V_PCH_ILB_PIRQX_ROUT_IRQ_15               0x0F  // Route to IRQ15

#define R_PCH_ILB_SERIRQ_CNT                      0x10  // Serial IRQ Control
#define B_PCH_ILB_SERIRQ_CNT_SIRQMD               BIT7  // Mode

#define R_PCH_ILB_ULKMC                           0x14  // USB Legacy Keyboard / Mouse Control
#define B_PCH_ILB_ULKMC_TRAPBY64W                 BIT11 // SMI Caused by Port 64 Write
#define B_PCH_ILB_ULKMC_TRAPBY64R                 BIT10 // SMI Caused by Port 64 Read
#define B_PCH_ILB_ULKMC_TRAPBY60W                 BIT9  // SMI Caused by Port 60 Write
#define B_PCH_ILB_ULKMC_TRAPBY60R                 BIT8  // SMI Caused by Port 60 Read
#define B_PCH_ILB_ULKMC_64WEN                     BIT3  // SMI on Port 64 Writes Enable
#define B_PCH_ILB_ULKMC_64REN                     BIT2  // SMI on Port 64 Reads Enable
#define B_PCH_ILB_ULKMC_60WEN                     BIT1  // SMI on Port 60 Writes Enable
#define B_PCH_ILB_ULKMC_60REN                     BIT0  // SMI on Port 60 Reads Enable

#define R_PCH_ILB_FWH_BIOS_SEL                    0x18  // FWH ID Select
#define B_PCH_ILB_FWH_BIOS_SEL_F8                 0xF0000000 // F8-FF ID Select
#define B_PCH_ILB_FWH_BIOS_SEL_F0                 0x0F000000 // F0-F7 ID Select
#define B_PCH_ILB_FWH_BIOS_SEL_E8                 0x00F00000 // E8-EF ID Select
#define B_PCH_ILB_FWH_BIOS_SEL_E0                 0x000F0000 // E0-E7 ID Select
#define B_PCH_ILB_FWH_BIOS_SEL_D8                 0x0000F000 // D8-DF ID Select
#define B_PCH_ILB_FWH_BIOS_SEL_D0                 0x00000F00 // D0-D7 ID Select
#define B_PCH_ILB_FWH_BIOS_SEL_C8                 0x000000F0 // C8-CF ID Select
#define B_PCH_ILB_FWH_BIOS_SEL_C0                 0x0000000F // C0-C7 ID Select

#define R_PCH_ILB_BIOS_CNTL                       0x1C  // BIOS Control
#define S_PCH_ILB_BIOS_CNTL                       4
#define B_PCH_ILB_BIOS_CNTL_PFE                   BIT8  // Prefetch Enable
#define B_PCH_ILB_BIOS_CNTL_LE                    BIT1  // Lock Enable
#define N_PCH_ILB_BIOS_CNTL_LE                    1
#define B_PCH_ILB_BIOS_CNTL_WP                    BIT0  // Write Protect

#define R_PCH_ILB_D0IR                            0x20  // Device 0 Interrupt Route
#define R_PCH_ILB_D1IR                            0x22  // Device 1 Interrupt Route
#define R_PCH_ILB_D2IR                            0x24  // Device 2 Interrupt Route
#define R_PCH_ILB_D3IR                            0x26  // Device 3 Interrupt Route
#define R_PCH_ILB_D4IR                            0x28  // Device 4 Interrupt Route
#define R_PCH_ILB_D5IR                            0x2A  // Device 5 Interrupt Route
#define R_PCH_ILB_D6IR                            0x2C  // Device 6 Interrupt Route
#define R_PCH_ILB_D7IR                            0x2E  // Device 7 Interrupt Route
#define R_PCH_ILB_D8IR                            0x30  // Device 8 Interrupt Route
#define R_PCH_ILB_D9IR                            0x32  // Device 9 Interrupt Route
#define R_PCH_ILB_D10IR                           0x34  // Device 10 Interrupt Route
#define R_PCH_ILB_D11IR                           0x36  // Device 11 Interrupt Route
#define R_PCH_ILB_D12IR                           0x38  // Device 12 Interrupt Route
#define R_PCH_ILB_D13IR                           0x3A  // Device 13 Interrupt Route
#define R_PCH_ILB_D14IR                           0x3C  // Device 14 Interrupt Route
#define R_PCH_ILB_D15IR                           0x3E  // Device 15 Interrupt Route
#define R_PCH_ILB_D16IR                           0x40  // Device 16 Interrupt Route
#define R_PCH_ILB_D17IR                           0x42  // Device 17 Interrupt Route
#define R_PCH_ILB_D18IR                           0x44  // Device 18 Interrupt Route
#define R_PCH_ILB_D19IR                           0x46  // Device 19 Interrupt Route
#define R_PCH_ILB_D20IR                           0x48  // Device 20 Interrupt Route
#define R_PCH_ILB_D21IR                           0x4A  // Device 21 Interrupt Route
#define R_PCH_ILB_D22IR                           0x4C  // Device 22 Interrupt Route
#define R_PCH_ILB_D23IR                           0x4E  // Device 23 Interrupt Route
#define R_PCH_ILB_D24IR                           0x50  // Device 24 Interrupt Route
#define R_PCH_ILB_D25IR                           0x52  // Device 25 Interrupt Route
#define R_PCH_ILB_D26IR                           0x54  // Device 26 Interrupt Route
#define R_PCH_ILB_D27IR                           0x56  // Device 27 Interrupt Route
#define R_PCH_ILB_D28IR                           0x58  // Device 28 Interrupt Route
#define R_PCH_ILB_D29IR                           0x5A  // Device 29 Interrupt Route
#define R_PCH_ILB_D30IR                           0x5C  // Device 30 Interrupt Route
#define R_PCH_ILB_D31IR                           0x5E  // Device 31 Interrupt Route

#define B_PCH_ILB_DXXIR_IDR_MASK                  (BIT14 | BIT13 | BIT12) // INTD Mask
#define V_PCH_ILB_DXXIR_IDR_PIRQA                 0                       // INTD Mapping to IRQ A
#define V_PCH_ILB_DXXIR_IDR_PIRQB                 BIT12                   // INTD Mapping to IRQ B
#define V_PCH_ILB_DXXIR_IDR_PIRQC                 BIT13                   // INTD Mapping to IRQ C
#define V_PCH_ILB_DXXIR_IDR_PIRQD                 (BIT13 | BIT12)         // INTD Mapping to IRQ D
#define V_PCH_ILB_DXXIR_IDR_PIRQE                 BIT14                   // INTD Mapping to IRQ E
#define V_PCH_ILB_DXXIR_IDR_PIRQF                 (BIT14 | BIT12)         // INTD Mapping to IRQ F
#define V_PCH_ILB_DXXIR_IDR_PIRQG                 (BIT14 | BIT13)         // INTD Mapping to IRQ G
#define V_PCH_ILB_DXXIR_IDR_PIRQH                 (BIT14 | BIT13 | BIT12) // INTD Mapping to IRQ H

#define B_PCH_ILB_DXXIR_ICR_MASK                  (BIT10 | BIT9 | BIT8) // INTC Mask
#define V_PCH_ILB_DXXIR_ICR_PIRQA                 0                     // INTC Mapping to IRQ A
#define V_PCH_ILB_DXXIR_ICR_PIRQB                 BIT8                  // INTC Mapping to IRQ B
#define V_PCH_ILB_DXXIR_ICR_PIRQC                 BIT9                  // INTC Mapping to IRQ C
#define V_PCH_ILB_DXXIR_ICR_PIRQD                 (BIT9 | BIT8)         // INTC Mapping to IRQ D
#define V_PCH_ILB_DXXIR_ICR_PIRQE                 BIT10                 // INTC Mapping to IRQ E
#define V_PCH_ILB_DXXIR_ICR_PIRQF                 (BIT10 | BIT8)        // INTC Mapping to IRQ F
#define V_PCH_ILB_DXXIR_ICR_PIRQG                 (BIT10 | BIT9)        // INTC Mapping to IRQ G
#define V_PCH_ILB_DXXIR_ICR_PIRQH                 (BIT10 | BIT9 | BIT8) // INTC Mapping to IRQ H

#define B_PCH_ILB_DXXIR_IBR_MASK                  (BIT6 | BIT5 | BIT4) // INTB Mask
#define V_PCH_ILB_DXXIR_IBR_PIRQA                 0                    // INTB Mapping to IRQ A
#define V_PCH_ILB_DXXIR_IBR_PIRQB                 BIT4                 // INTB Mapping to IRQ B
#define V_PCH_ILB_DXXIR_IBR_PIRQC                 BIT5                 // INTB Mapping to IRQ C
#define V_PCH_ILB_DXXIR_IBR_PIRQD                 (BIT5 | BIT4)        // INTB Mapping to IRQ D
#define V_PCH_ILB_DXXIR_IBR_PIRQE                 BIT6                 // INTB Mapping to IRQ E
#define V_PCH_ILB_DXXIR_IBR_PIRQF                 (BIT6 | BIT4)        // INTB Mapping to IRQ F
#define V_PCH_ILB_DXXIR_IBR_PIRQG                 (BIT6 | BIT5)        // INTB Mapping to IRQ G
#define V_PCH_ILB_DXXIR_IBR_PIRQH                 (BIT6 | BIT5 | BIT4) // INTB Mapping to IRQ H

#define B_PCH_ILB_DXXIR_IAR_MASK                  (BIT2 | BIT1 | BIT0) // INTA Mask
#define V_PCH_ILB_DXXIR_IAR_PIRQA                 0                    // INTA Mapping to IRQ A
#define V_PCH_ILB_DXXIR_IAR_PIRQB                 BIT0                 // INTA Mapping to IRQ B
#define V_PCH_ILB_DXXIR_IAR_PIRQC                 BIT1                 // INTA Mapping to IRQ C
#define V_PCH_ILB_DXXIR_IAR_PIRQD                 (BIT1 | BIT0)        // INTA Mapping to IRQ D
#define V_PCH_ILB_DXXIR_IAR_PIRQE                 BIT2                 // INTA Mapping to IRQ E
#define V_PCH_ILB_DXXIR_IAR_PIRQF                 (BIT2 | BIT0)        // INTA Mapping to IRQ F
#define V_PCH_ILB_DXXIR_IAR_PIRQG                 (BIT2 | BIT1)        // INTA Mapping to IRQ G
#define V_PCH_ILB_DXXIR_IAR_PIRQH                 (BIT2 | BIT1 | BIT0) // INTA Mapping to IRQ H

#define R_PCH_ILB_OIC                             0x60  // Other Interrupt Controller
#define B_PCH_ILB_OIC_SIRQEN                      BIT12 // Serial IRQ Enable
#define B_PCH_ILB_OIC_AEN                         BIT8  // APIC Enable

#define R_PCH_ILB_RTC_CONF                        0x64  // RTC Control
#define B_PCH_ILB_RTC_CONF_UCMOS_LOCK             BIT1  // Upper 128 Byte Lock
#define B_PCH_ILB_RTC_CONF_LCMOS_LOCK             BIT0  // Lower 128 Byte Lock

#define R_PCH_ILB_RTM                             0x68  // RTC Test Mode
#define B_PCH_ILB_RTM_RTM1                        (BIT2 | BIT1 | BIT0)

#define R_PCH_ILB_BCS                             0x6C  // BIOS Control Status
#define B_PCH_ILB_BCS_SMIWPEN                     BIT1  // SMI WPD Enable
#define B_PCH_ILB_BCS_SMIWPST                     BIT0  // SMI WPD Status

#define R_PCH_ILB_LE                              0x70  // LE
#define B_PCH_ILB_LE_IRQ12C                       BIT1  // IRQ12 Cause
#define B_PCH_ILB_LE_IRQ1C                        BIT0  // IRQ1 Cause

#define R_PCH_ILB_RTCC                            0x74  // RTC HIP Configuration
#define B_PCH_ILB_RTCC_RTCB4                      BIT6  // RTC Bias Resistor 4, Adds 480 Kohm
#define B_PCH_ILB_RTCC_RTCB3                      BIT5  // RTC Bias Resistor 3, Adds 240 Kohm
#define B_PCH_ILB_RTCC_RTCB2                      BIT4  // RTC Bias Resistor 2, Adds 120 Kohm
#define B_PCH_ILB_RTCC_RTCB1                      BIT3  // RTC Bias Resistor 1, Adds 60 Kohm
#define B_PCH_ILB_RTCC_RTCB0                      BIT2  // RTC Bias Resistor 0, Adds 30 Kohm
#define B_PCH_ILB_RTCC_DSWEN                      BIT1  // Deep Sleep Well Enable
#define B_PCH_ILB_RTCC_FEN                        BIT0  // Enable the Fast Oscillator Bypass Mode

#define R_PCH_ILB_DEF0                            0x78  // Defeature Register 0
#define B_PCH_ILB_DEF0_SHRTSYNC                   BIT22 // Short Sync Abort Defeature
#define B_PCH_ILB_DEF0_SDD                        BIT21 // Sub Decode Disable

#define R_PCH_ILB_DEF1                            0x7C  // Defeature Register 1
#define B_PCH_ILB_DEF1_TPMPF                      BIT10 // usb2leg_chknbit_TPM_PF
#define B_PCH_ILB_DEF1_HPETDEF                    BIT8  // usb2leg_chknbit_hpet
#define B_PCH_ILB_DEF1_ECWS                       BIT6  // 8254 Early CW Select
#define B_PCH_ILB_DEF1_FOF                        BIT5  // 8254 Freeze on first on 1st rd wr11
#define B_PCH_ILB_DEF1_FOAR                       BIT4  // 8254 Freeze_On_AnyRead
#define B_PCH_ILB_DEF1_LMOO                       BIT3  // 8259 L2L0_Match_On_OCW2
#define B_PCH_ILB_DEF1_DFP                        BIT2  // 8259 Disable_Freeze_Priority
#define B_PCH_ILB_DEF1_EETI                       BIT1  // 8259 Extend_EdgeTrig_IRQ
#define B_PCH_ILB_DEF1_DSAEOI                     BIT0  // 8259 Disable_Slave_AEOI

#define R_PCH_ILB_GNMI                            0x80  // NMI Register
#define S_PCH_ILB_GNMI                            4
#define B_PCH_ILB_GNMI_NMI2SMIEN                  BIT6  // NMI to SMI Enable
#define N_PCH_ILB_GNMI_NMI2SMIEN                  6
#define B_PCH_ILB_GNMI_NMI2SMIST                  BIT5  // NMI to SMI Status
#define N_PCH_ILB_GNMI_NMI2SMIST                  5
#define B_PCH_ILB_GNMI_NMIN                       BIT4  // NMI NOW
#define B_PCH_ILB_GNMI_NMINS                      BIT3  // NMI NOW Status
#define B_PCH_ILB_GNMI_GNMIED                     BIT2  // GPIO NMI Edge Detection
#define B_PCH_ILB_GNMI_GNMIE                      BIT1  // GPIO NMI Enable
#define B_PCH_ILB_GNMI_GNMIS                      BIT0  // GPIO NMI Status

#define R_PCH_ILB_LPCC                            0x84  // LPC Control
#define B_PCH_ILB_LPCC_LPCCLK_SLC                 BIT8  // iLPCCLK Mux Select
#define B_PCH_ILB_LPCC_LPCCLK_FORCE_OFF           BIT3
#define B_PCH_ILB_LPCC_CLKRUN_EN                  BIT2  // LPC CLKRUN Protocol Enable
#define B_PCH_ILB_LPCC_LPCCLK1EN                  BIT1  // Clock 1 Enable
#define B_PCH_ILB_LPCC_LPCCLK0EN                  BIT0  // Clock 0 Enable

#define R_PCH_ILB_IRQE                            0x88  // IRQ Enable Control
#define B_PCH_ILB_IRQE_IRQ4TO7EN                  (BIT7 | BIT6 | BIT5 | BIT4) // IRQ4 - IRQ7 Enable
#define B_PCH_ILB_IRQE_UARTIRQEN_IRQ3             BIT3  // UART IRQ3 Enable

//
// ACPI and Legacy I/O Registers (ABASE)
//
#define R_PCH_ACPI_PM1_STS                        0x00  // Power Management 1 Status
#define S_PCH_ACPI_PM1_STS                        2
#define B_PCH_ACPI_PM1_STS_WAK                    BIT15 // Wake Status
#define B_PCH_ACPI_PM1_STS_WAK_PCIE0              BIT14 // PCI Express 0 Wake Status
#define B_PCH_ACPI_PM1_STS_USB_CLKLESS            BIT13 // USB Clockless Status
#define B_PCH_ACPI_PM1_STS_PRBTNOR                BIT11 // Power Button Override Status
#define B_PCH_ACPI_PM1_STS_RTC                    BIT10 // RTC Status
#define B_PCH_ACPI_PM1_STS_PWRBTN                 BIT8  // Power Button Status
#define B_PCH_ACPI_PM1_STS_GBL                    BIT5  // Global Status
#define B_PCH_ACPI_PM1_STS_WAK_PCIE3              BIT4  // PCI Express 3 Wake Status
#define B_PCH_ACPI_PM1_STS_WAK_PCIE2              BIT3  // PCI Express 2 Wake Status
#define B_PCH_ACPI_PM1_STS_WAK_PCIE1              BIT2  // PCI Express 1 Wake Status
#define B_PCH_ACPI_PM1_STS_TMROF                  BIT0  // Timer Overflow Status
#define N_PCH_ACPI_PM1_STS_WAK                    15
#define N_PCH_ACPI_PM1_STS_PRBTNOR                11
#define N_PCH_ACPI_PM1_STS_RTC                    10
#define N_PCH_ACPI_PM1_STS_PWRBTN                 8
#define N_PCH_ACPI_PM1_STS_GBL                    5
#define N_PCH_ACPI_PM1_STS_TMROF                  0

#define R_PCH_ACPI_PM1_EN                         0x02  // Power Management 1 Enables
#define S_PCH_ACPI_PM1_EN                         2
#define B_PCH_ACPI_PM1_WAK_DIS_PCIE0              BIT14 // PCI Express 0 Disable
#define B_PCH_ACPI_PM1_EN_USB_CLKLESS             BIT13 // USB Clockless Enable Bit
#define B_PCH_ACPI_PM1_EN_RTC                     BIT10 // RTC Alarm Enable Bit
#define B_PCH_ACPI_PM1_EN_PWRBTN                  BIT8  // Power Button Enable Bit
#define B_PCH_ACPI_PM1_EN_GBL                     BIT5  // Global Enable Bit
#define B_PCH_ACPI_PM1_WAK_DIS_PCIE3              BIT4  // PCI Express 3 Disable
#define B_PCH_ACPI_PM1_WAK_DIS_PCIE2              BIT3  // PCI Express 2 Disable
#define B_PCH_ACPI_PM1_WAK_DIS_PCIE1              BIT2  // PCI Express 1 Disable
#define B_PCH_ACPI_PM1_EN_TMROF                   BIT0  // Timer Overflow Interrupt Enable Bit
#define N_PCH_ACPI_PM1_EN_RTC                     10
#define N_PCH_ACPI_PM1_EN_PWRBTN                  8
#define N_PCH_ACPI_PM1_EN_GBL                     5
#define N_PCH_ACPI_PM1_EN_TMROF                   0

#define R_PCH_ACPI_PM1_CNT                        0x04  // Power Management 1 Control
#define S_PCH_ACPI_PM1_CNT                        4
#define B_PCH_ACPI_PM1_CNT_SLP_EN                 BIT13 // Sleep enable
#define B_PCH_ACPI_PM1_CNT_SLP_TYP                (BIT12 | BIT11 | BIT10) // Sleep Type
#define V_PCH_ACPI_PM1_CNT_S0                     0x00000000 // ON (S0)
#define V_PCH_ACPI_PM1_CNT_S1                     0x00000400 // Puts CPU in S1 state (S1)
#define V_PCH_ACPI_PM1_CNT_S3                     0x00001400 // Suspend-to-RAM (S3)
#define V_PCH_ACPI_PM1_CNT_S4                     0x00001800 // Suspend-to-Disk (S4)
#define V_PCH_ACPI_PM1_CNT_S5                     0x00001C00 // Soft Off (S5)
#define B_PCH_ACPI_PM1_CNT_GBL_RLS                BIT2
#define B_PCH_ACPI_PM1_CNT_BM_RLD                 BIT1  // Treated as Scratchpad Bit
#define B_PCH_ACPI_PM1_CNT_SCI_EN                 BIT0  // SCI Enable

#define R_PCH_ACPI_PM1_TMR                        0x08  // Power Management 1 Timer
#define B_PCH_ACPI_PM1_TMR_VAL                    0xFFFFFF // The timer value mask
#define V_PCH_ACPI_PM1_TMR_MAX_VAL                0x1000000 // The timer is 24 bit overflow
#define V_PCH_ACPI_PM1_TMR_FREQUENCY              3579545 // Timer Frequency
#define V_PCH_ACPI_PM1_TMR_NUM_BITS               24    // Programmed to 24 not 32
#define V_PCH_ACPI_PM1_TMR_MAX_BITS               32

#define R_PCH_ACPI_GPE0a_STS                      0x20  // General Purpose Event 0a Status
#define S_PCH_ACPI_GPE0a_STS                      4
#define B_PCH_ACPI_GPE0a_STS_CORE_GPIO            0xFF000000 // CORE GPIO Status
#define B_PCH_ACPI_GPE0a_STS_SUS_GPIO             0x00FF0000 // SUS GPIO Status
#define B_PCH_ACPI_GPE0a_STS_PME_B0               BIT13 // Power Management Event Bus 0 Status
#define B_PCH_ACPI_GPE0a_STS_BATLOW               BIT10 // Battery Low Status
#define B_PCH_ACPI_GPE0a_STS_PCI_EXP              BIT9  // PCI Express Status
#define B_PCH_ACPI_GPE0a_STS_GUNIT_SCI            BIT5  // GUNIT SCI Status
#define B_PCH_ACPI_GPE0a_STS_PUNIT_SCI            BIT4  // PUNIT SCI Status
#define B_PCH_ACPI_GPE0a_STS_SWGPE                BIT2  // Software GPE Status
#define B_PCH_ACPI_GPE0a_STS_HOT_PLUG             BIT1  // Hot Plug Status
#define N_PCH_ACPI_GPE0a_STS_PME_B0               13
#define N_PCH_ACPI_GPE0a_STS_BATLOW               10
#define N_PCH_ACPI_GPE0a_STS_PCI_EXP              9
#define N_PCH_ACPI_GPE0a_STS_GUNIT_SCI            5
#define N_PCH_ACPI_GPE0a_STS_PUNIT_SCI            4
#define N_PCH_ACPI_GPE0a_STS_SWGPE                2
#define N_PCH_ACPI_GPE0a_STS_HOT_PLUG             1

#define R_PCH_ACPI_GPE0a_EN                       0x28  // General Purpose Event 0a Enables
#define S_PCH_ACPI_GPE0a_EN                       4
#define B_PCH_ACPI_GPE0a_EN_CORE_GPIO             0xFF000000 // CORE GPIO Enable
#define B_PCH_ACPI_GPE0a_EN_SUS_GPIO              0x00FF0000 // SUS GPIO Enable
#define B_PCH_ACPI_GPE0a_EN_PME_B0                BIT13 // Power Management Event Bus 0 Enable
#define B_PCH_ACPI_GPE0a_EN_BATLOW                BIT10 // Battery Low Enable
#define B_PCH_ACPI_GPE0a_EN_PCI_EXP               BIT9  // PCI Express Enable
#define B_PCH_ACPI_GPE0a_EN_SWGPE                 BIT2  // Software GPE Enable
#define B_PCH_ACPI_GPE0a_EN_HOT_PLUG              BIT1  // Hot Plug Enable
#define N_PCH_ACPI_GPE0a_EN_PME_B0                13
#define N_PCH_ACPI_GPE0a_EN_BATLOW                10
#define N_PCH_ACPI_GPE0a_EN_PCI_EXP               9
#define N_PCH_ACPI_GPE0a_EN_SWGPE                 2
#define N_PCH_ACPI_GPE0a_EN_HOT_PLUG              1

#define R_PCH_SMI_EN                              0x30  // SMI Control and Enable
#define S_PCH_SMI_EN                              4
#define B_PCH_SMI_EN_LEGACY_USB3                  BIT31 // Legacy USB 3 Enable
#define B_PCH_SMI_EN_INTEL_USB2                   BIT18 // Intel USB 2 Enable
#define B_PCH_SMI_EN_LEGACY_USB2                  BIT17 // Legacy USB 2 Enable
#define B_PCH_SMI_EN_PERIODIC                     BIT14 // Periodic Enable
#define B_PCH_SMI_EN_TCO                          BIT13 // TCO Enable
#define B_PCH_SMI_EN_BIOS_RLS                     BIT7  // BIOS RLS
#define B_PCH_SMI_EN_SWSMI_TMR                    BIT6  // Software SMI Timer Enable
#define B_PCH_SMI_EN_APMC                         BIT5  // APMC Enable
#define B_PCH_SMI_EN_ON_SLP_EN                    BIT4  // SMI On Sleep Enable
#define B_PCH_SMI_EN_BIOS                         BIT2  // BIOS Enable
#define B_PCH_SMI_EN_EOS                          BIT1  // End of SMI
#define B_PCH_SMI_EN_GBL_SMI                      BIT0  // Global SMI Enable
#define N_PCH_SMI_EN_LEGACY_USB3                  31
#define N_PCH_SMI_EN_GPIO_UNLOCK                  27
#define N_PCH_SMI_EN_INTEL_USB2                   18
#define N_PCH_SMI_EN_LEGACY_USB2                  17
#define N_PCH_SMI_EN_PERIODIC                     14
#define N_PCH_SMI_EN_TCO                          13
#define N_PCH_SMI_EN_BIOS_RLS                     7
#define N_PCH_SMI_EN_SWSMI_TMR                    6
#define N_PCH_SMI_EN_APMC                         5
#define N_PCH_SMI_EN_ON_SLP_EN                    4
#define N_PCH_SMI_EN_BIOS                         2
#define N_PCH_SMI_EN_EOS                          1
#define N_PCH_SMI_EN_GBL_SMI                      0

#define R_PCH_SMI_STS                             0x34  // SMI Status Register
#define S_PCH_SMI_STS                             4
#define B_PCH_SMI_STS_LEGACY_USB3                 BIT31 // Legacy USB 3 Status
#define B_PCH_SMI_STS_GUNIT_SMI                   BIT29 // GUNIT SMI Status
#define B_PCH_SMI_STS_PUNIT_SMI                   BIT28 // PUNIT SMI Status
#define B_PCH_SMI_STS_SPI                         BIT26 // SPI SMI Status
#define B_PCH_SMI_STS_PCI_EXP                     BIT20 // PCI Express SMI Status
#define B_PCH_SMI_STS_INTEL_USB2                  BIT18 // Intel USB 2 Status
#define B_PCH_SMI_STS_LEGACY_USB2                 BIT17 // Legacy USB 2 Status
#define N_PCH_SMI_STS_TCO                         13
#define B_PCH_SMI_STS_SMBUS                       BIT16 // SMBUS SMI Status
#define B_PCH_SMI_STS_ILB                         BIT15 // ILB SMI Status
#define B_PCH_SMI_STS_PERIODIC                    BIT14 // Periodic Status
#define B_PCH_SMI_STS_TCO                         BIT13 // TCO Status
#define B_PCH_SMI_STS_GPE0                        BIT9  // GPE0 Status
#define B_PCH_SMI_STS_PM1_STS_REG                 BIT8  // PM1 Status Register
#define B_PCH_SMI_STS_SWSMI_TMR                   BIT6  // Software SMI Timer Status
#define B_PCH_SMI_STS_APM                         BIT5  // APM Status
#define B_PCH_SMI_STS_ON_SLP_EN                   BIT4  // SMI On Sleep Enable Status
#define B_PCH_SMI_STS_BIOS                        BIT2  // BIOS Status
#define N_PCH_SMI_STS_LEGACY_USB3                 31
#define N_PCH_SMI_STS_SPI                         26
#define N_PCH_SMI_STS_PCI_EXP                     20
#define N_PCH_SMI_STS_INTEL_USB2                  18
#define N_PCH_SMI_STS_LEGACY_USB2                 17
#define N_PCH_SMI_STS_SMBUS                       16
#define N_PCH_SMI_STS_ILB                         15
#define N_PCH_SMI_STS_PERIODIC                    14
#define N_PCH_SMI_STS_TCO                         13
#define N_PCH_SMI_STS_GPE0                        9
#define N_PCH_SMI_STS_PM1_STS_REG                 8
#define N_PCH_SMI_STS_SWSMI_TMR                   6
#define N_PCH_SMI_STS_APM                         5
#define N_PCH_SMI_STS_ON_SLP_EN                   4
#define N_PCH_SMI_STS_BIOS                        2

#define R_PCH_ALT_GP_SMI_EN                       0x38  // Alternate GPI SMI Enable
#define S_PCH_ALT_GP_SMI_EN                       2
#define B_PCH_ALT_GP_SMI_EN_CORE_GPIO             0xFF00 // SUS GPIO SMI Enable
#define B_PCH_ALT_GP_SMI_EN_SUS_GPIO              0x00FF // CORE GPIO SMI Enable

#define R_PCH_ALT_GP_SMI_STS                      0x3A  // Alternate GPI SMI Status
#define S_PCH_ALT_GP_SMI_STS                      2
#define B_PCH_ALT_GP_SMI_STS_CORE_GPIO            0xFF00 // SUS GPIO SMI Status
#define B_PCH_ALT_GP_SMI_STS_SUS_GPIO             0x00FF // CORE GPIO SMI Status

#define R_PCH_UPRWC                               0x3C  // USB Per-Port Registers Write Control
#define S_PCH_UPRWC                               2
#define B_PCH_UPRWC_WR_EN_SMI_STS                 BIT8 // Write Enable Status
#define B_PCH_UPRWC_WR_EN                         BIT1 // USB Per-Port Registers Write Enable
#define B_PCH_UPRWC_WR_EN_SMI_EN                  BIT0 // Write Enable SMI Enable

#define R_PCH_ACPI_GPE_CNTL                       0x40  // General Purpose Event Control
#define B_PCH_ACPI_GPE_CNTL_SWGPE_CTRL            BIT17 // Software GPE Control
#define B_PCH_ACPI_GPE_CNTL_PCIE3_SCI_EN          BIT3
#define B_PCH_ACPI_GPE_CNTL_PCIE2_SCI_EN          BIT2
#define B_PCH_ACPI_GPE_CNTL_PCIE1_SCI_EN          BIT1
#define B_PCH_ACPI_GPE_CNTL_PCIE0_SCI_EN          BIT0

#define R_PCH_ACPI_PM2_CNT                        0x50  // PM2a Control Block
#define B_PCH_ACPI_PM2_CNT_ARB_DIS                BIT0  // Scratchpad Bit

#define R_PCH_TCO_RLD                             0x60  // TCO Reload
#define B_PCH_TCO_RLD_VAL                         0x3FF // TCO Timer Count Value

#define R_PCH_TCO_STS                             0x64  // TCO Timer Status
#define S_PCH_TCO_STS                             4
#define B_PCH_TCO_STS_SECOND_TO                   BIT17 // Second Timeout Status
#define B_PCH_TCO_STS_TIMEOUT                     BIT3  // Timeout
#define N_PCH_TCO_STS_TIMEOUT                     3

#define R_PCH_TCO_CNT                             0x68  // TCO Control
#define S_PCH_TCO_CNT                             2
#define B_PCH_TCO_CNT_OS_POLICY                   (BIT21 | BIT20) // OS Policy
#define B_PCH_TCO_CNT_LOCK                        BIT12 // TCO Enable Lock
#define B_PCH_TCO_CNT_TMR_HLT                     BIT11 // TCO Timer Halt

#define R_PCH_TCO_TMR                             0x70  // TCO Timer
#define B_PCH_TCO_TMR_TCO_TRLD                    0x3FF0000

//
// PMC Memory Space Registers (PBASE)
//
#define R_PCH_PMC_PRSTS                           0x00  // Power and Reset Status
#define B_PCH_PMC_PRSTS_PRODID                    0xFF000000 // Power Management Controller Product ID
#define B_PCH_PMC_PRSTS_REVID                     0x00FF0000 // Power Management Controller Revision ID
#define B_PCH_PMC_PRSTS_PM_WD_TMR                 BIT15 // PMC Watchdog Timer Status
#define B_PCH_PMC_PRSTS_CODE_COPIED_STS           BIT11 // Patch Copied Over Status
#define B_PCH_PMC_PRSTS_CODE_LOAD_TO              BIT9  // Patch Load Timeout Status
#define B_PCH_PMC_PRSTS_OP_STS                    BIT8  // PMC Operational Status
#define B_PCH_PMC_PRSTS_SEC_GBLRST_STS            BIT7  // SEC Global Reset Status
#define B_PCH_PMC_PRSTS_SEC_WD_TMR_STS            BIT6  // SEC Watchdog Timer Status
#define B_PCH_PMC_PRSTS_WOL_OVR_WK_STS            BIT5  // Wake On LAN Override Wake Status
#define B_PCH_PMC_PRSTS_HOST_WAKE_STS             BIT4  // PMC Host Wake Status

#define R_PCH_PMC_PM_CFG                          0x08  // Power Management Configuration
#define B_PCH_PMC_PM_CFG_SPS                      BIT5  // Shutdown Policy Select
#define B_PCH_PMC_PM_CFG_NO_REBOOT                BIT4  // No Reboot Strap
#define B_PCH_PMC_PM_CFG_SX_ENT_TO_EN             BIT3  // S1 / 3 / 4 / 5 Entry Timeout Enable
#define B_PCH_PMC_PM_CFG_TIMING_T581              (BIT1 | BIT0) // Timing t581

#define R_PCH_PMC_PM_STS                          0x0C  // Power Management Status
#define B_PCH_PMC_PM_STS_PMC_MSG_FULL             BIT24 // PMC Message Full Status
#define B_PCH_PMC_PM_STS_PMC_MSG_4_FULL           BIT23 // PMC 4 Message Full Status
#define B_PCH_PMC_PM_STS_PMC_MSG_3_FULL           BIT22 // PMC 3 Message Full Status
#define B_PCH_PMC_PM_STS_PMC_MSG_2_FULL           BIT21 // PMC 2 Message Full Status
#define B_PCH_PMC_PM_STS_PMC_MSG_1_FULL           BIT20 // PMC 1 Message Full Status
#define B_PCH_PMC_PM_STS_CODE_REQ                 BIT8  // Patch Request Status
#define B_PCH_PMC_PM_STS_HPR_ENT_TO               BIT2  // Host partition Reset Entry Timeout Status
#define B_PCH_PMC_PM_STS_SX_ENT_TO                BIT1  // S3 / 4 / 5 Entry Timeout Status

#define R_PCH_PMC_MTPMC                           0x10  // Message to PMC

#define R_PCH_PMC_GEN_PMCON_1                     0x20  // General PM Configuration 1
#define B_PCH_PMC_GEN_PMCON_UART_EN               BIT24 // UART Debug Port Enable
#define B_PCH_PMC_GEN_PMCON_DRAM_INIT             BIT23 // DRAM Initialization Scratchpad Bit
#define B_PCH_PMC_GEN_PMCON_MEM_SR                BIT21 // Memory Placed in Self-Refresh
#define B_PCH_PMC_GEN_PMCON_SRS                   BIT20 // System Reset Status
#define B_PCH_PMC_GEN_PMCON_CTS                   BIT19 // CPU Thermal Trip Status
#define B_PCH_PMC_GEN_PMCON_MIN_SLP_S4            BIT18 // Minimum SLP_S4# Assertion Width Violation Status
#define B_PCH_PMC_GEN_PMCON_PWROK_FLR             BIT16 // PWROK Failure
#define B_PCH_PMC_GEN_PMCON_PME_B0_S5_DIS         BIT15 // PME B0 S5 Disable
#define B_PCH_PMC_GEN_PMCON_SUS_PWR_FLR           BIT14 // SUS Well Power Failure
#define B_PCH_PMC_GEN_PMCON_WOL_ENABLE_OVERRIDE   BIT13 // WOL Enable Override
#define B_PCH_PMC_GEN_PMCON_DISABLE_SX_STRETCH    BIT12 // Disable SLP_X Scretching After SUS Well Power Up
#define B_PCH_PMC_GEN_PMCON_SLP_S3_MAW            (BIT11 | BIT10) // SLP_S3# Minimum Assertion Width
#define V_PCH_PMC_GEN_PMCON_SLP_S3_MAW_60US       0x000 // 60 micro seconds
#define V_PCH_PMC_GEN_PMCON_SLP_S3_MAW_1MS        0x400 // 1 milli second
#define V_PCH_PMC_GEN_PMCON_SLP_S3_MAW_50MS       0x800 // 50 milli seconds
#define V_PCH_PMC_GEN_PMCON_SLP_S3_MAW_2S         0xC00 // 2 seconds
#define B_PCH_PMC_GEN_PMCON_GEN_RST_STS           BIT9  // General Reset Status
#define B_PCH_PMC_GEN_PMCON_RTC_RESERVED          BIT8  // RTC Reserved
#define B_PCH_PMC_GEN_PMCON_SWSMI_RTSL            (BIT7 | BIT6)  // SWSMI Rate Select
#define V_PCH_PMC_GEN_PMCON_SWSMI_RTSL_64MS       0xC0  // 64ms +/- 4ms
#define V_PCH_PMC_GEN_PMCON_SWSMI_RTSL_32MS       0x80  // 32ms +/- 4ms
#define V_PCH_PMC_GEN_PMCON_SWSMI_RTSL_16MS       0x40  // 16ms +/- 4ms
#define V_PCH_PMC_GEN_PMCON_SWSMI_RTSL_1_5MS      0x00  // 1.5ms +/- 0.6ms
#define B_PCH_PMC_GEN_PMCON_SLP_S4_MAW            (BIT5 | BIT4) // SLP_S4# Minimum Assertion Width
#define V_PCH_PMC_GEN_PMCON_SLP_S4_MAW_1S         0x30  // 1 second
#define V_PCH_PMC_GEN_PMCON_SLP_S4_MAW_2S         0x20  // 2 seconds
#define V_PCH_PMC_GEN_PMCON_SLP_S4_MAW_3S         0x10  // 3 seconds
#define V_PCH_PMC_GEN_PMCON_SLP_S4_MAW_4S         0x00  // 4 seconds
#define B_PCH_PMC_GEN_PMCON_SLP_S4_ASE            BIT3  // SLP_S4# Assertion Scretch Enable
#define B_PCH_PMC_GEN_PMCON_RTC_PWR_STS           BIT2  // RTC Power Status
#define B_PCH_PMC_GEN_PMCON_AFTERG3_EN            BIT0  // After G3 State Enable

#define R_PCH_PMC_GEN_PMCON_2                     0x24  // General PM Configuration 2
#define B_PCH_PMC_GEN_PMCON_LOCK_S4_STRET_LD      BIT18 // SLP_S3 / SLP_S4 Stretching Policy Lock-Down
#define B_PCH_PMC_GEN_PMCON_BIOS_PCI_EXP_EN       BIT10 // BIOS PCI Express Enable
#define B_PCH_PMC_GEN_PMCON_PWRBTN_LVL            BIT9  // Power Button Level
#define B_PCH_PMC_GEN_PMCON_SMI_LOCK              BIT4  // SMI Lock
#define B_PCH_PMC_GEN_PMCON_PER_SMI_SEL           (BIT1 | BIT0) // Period SMI Select
#define V_PCH_PMC_GEN_PMCON_PER_SMI_64S           0x0000 // 64 seconds
#define V_PCH_PMC_GEN_PMCON_PER_SMI_32S           0x0001 // 32 seconds
#define V_PCH_PMC_GEN_PMCON_PER_SMI_16S           0x0002 // 16 seconds
#define V_PCH_PMC_GEN_PMCON_PER_SMI_8S            0x0003 //  8 seconds

#define R_PCH_PMC_MFPMC                           0x28  // Message from PMC

#define R_PCH_PMC_SEC_STS                         0x2C  // SEC Status
#define B_PCH_PMC_SEC_STS_SEC                     (BIT3 | BIT2 | BIT1 | BIT0) // SEC Exclusion Cause

#define R_PCH_PMC_CRID                            0x30  // Configured Revision ID
#define B_PCH_PMC_CRID_RID_SEL                    (BIT1 | BIT0) // Revision ID Select

#define R_PCH_PMC_FUNC_DIS                        0x34  // Function Disable Register
#define B_PCH_PMC_FUNC_DIS_LPSS2_FUNC7            BIT31 // LPSS2 I2C #7
#define B_PCH_PMC_FUNC_DIS_LPSS2_FUNC6            BIT30 // LPSS2 I2C #6
#define B_PCH_PMC_FUNC_DIS_LPSS2_FUNC5            BIT29 // LPSS2 I2C #5
#define B_PCH_PMC_FUNC_DIS_LPSS2_FUNC4            BIT28 // LPSS2 I2C #4
#define B_PCH_PMC_FUNC_DIS_LPSS2_FUNC3            BIT27 // LPSS2 I2C #3
#define B_PCH_PMC_FUNC_DIS_LPSS2_FUNC2            BIT26 // LPSS2 I2C #2
#define B_PCH_PMC_FUNC_DIS_LPSS2_FUNC1            BIT25 // LPSS2 I2C #1
#define B_PCH_PMC_FUNC_DIS_LPSS2_FUNC0            BIT24 // LPSS2 DMA Disable
#define B_PCH_PMC_FUNC_DIS_PCI_EX_FUNC3           BIT23 // PCI Express Function 3 Disable
#define B_PCH_PMC_FUNC_DIS_PCI_EX_FUNC2           BIT22 // PCI Express Function 2 Disable
#define B_PCH_PMC_FUNC_DIS_PCI_EX_FUNC1           BIT21 // PCI Express Function 1 Disable
#define B_PCH_PMC_FUNC_DIS_PCI_EX_FUNC0           BIT20 // PCI Express Function 0 Disable
#define N_PCH_PMC_FUNC_DIS_PCI_EX_FUNC0           20
#define B_PCH_PMC_FUNC_DIS_SEC                    BIT19 // SEC Disable
#define B_PCH_PMC_FUNC_DIS_USB                    BIT18 // USB Disable
#define B_PCH_PMC_FUNC_DIS_SATA                   BIT17 // SATA Disable
#define B_PCH_PMC_FUNC_DIS_USH                    BIT15 // USH (USB3) Disable
#define B_PCH_PMC_FUNC_DIS_OTG                    BIT14 // USB OTG Disable
#define B_PCH_PMC_FUNC_DIS_LPE                    BIT13 // LPE Disable
#define B_PCH_PMC_FUNC_DIS_AZALIA                 BIT12 // Azalia Disable
#define B_PCH_PMC_FUNC_DIS_MIPI                   BIT11 // MIPI-HSI Disable
#define B_PCH_PMC_FUNC_DIS_SDIO4                  BIT11 // SCC SDIO #4 (Device 23, eMMC4.5) Disable
#define B_PCH_PMC_FUNC_DIS_SDIO3                  BIT10 // SCC SDIO #3 (Device 18, SD Card) Disable
#define B_PCH_PMC_FUNC_DIS_SDIO2                  BIT9  // SCC SDIO #2 (Device 17, SDIO) Disable
#define B_PCH_PMC_FUNC_DIS_SDIO1                  BIT8  // SCC SDIO #1 (Device 16, eMMC) Disable
#define B_PCH_PMC_FUNC_DIS_LPSS1_FUNC7            BIT7  // LPSS1 Spare #2 Disable
#define B_PCH_PMC_FUNC_DIS_LPSS1_FUNC6            BIT6  // LPSS1 Spare #1 Disable
#define B_PCH_PMC_FUNC_DIS_LPSS1_FUNC5            BIT5  // LPSS1 SPI Disable
#define B_PCH_PMC_FUNC_DIS_LPSS1_FUNC4            BIT4  // LPSS1 HS-UART #2 Disable
#define B_PCH_PMC_FUNC_DIS_LPSS1_FUNC3            BIT3  // LPSS1 HS-UART #1 Disable
#define B_PCH_PMC_FUNC_DIS_LPSS1_FUNC2            BIT2  // LPSS1 PWM #2 Disable
#define B_PCH_PMC_FUNC_DIS_LPSS1_FUNC1            BIT1  // LPSS1 PWM #1 Disable
#define B_PCH_PMC_FUNC_DIS_LPSS1_FUNC0            BIT0  // LPSS1 DMA Disable

#define R_PCH_PMC_FUNC_DIS2                       0x38  // Function Disable 2 Register
#define B_PCH_PMC_FUNC_DIS2_USH_SS_PHY            BIT2  // USH Super Speed PHY Disable
#define B_PCH_PMC_FUNC_DIS2_OTG_SS_PHY            BIT1  // OTG Super Speed PHY Disable
#define B_PCH_PMC_FUNC_DIS2_SMBUS                 BIT0  // SMBus Disable

#define R_PCH_PMC_PMIR                            0x48  // Extended Test Mode Register (ETR)
#define B_PCH_PMC_PMIR_CF9LOCK                    BIT31 // CF9h Lockdown
#define B_PCH_PMC_PMIR_LTR_DEF                    BIT22 // LTR Default
#define B_PCH_PMC_PMIR_IGNORE_HPET                BIT21 // Ignore HPET Disable Check Before Going to S0i2
#define B_PCH_PMC_PMIR_CF9GR                      BIT20 // CF9h Global Reset

#define R_PCH_PMC_VLT                             0x50  // Voltage Detect Register
#define B_PCH_PMC_VLT_FUSES                       0xFF  // Voltage Detect Fuses

#define R_PCH_PMC_GPI_ROUT                        0x58  // GPI Rout
#define B_PCH_PMC_GPI_ROUT_0                      (BIT1 | BIT0)
#define B_PCH_PMC_GPI_ROUT_1                      (BIT3 | BIT2)
#define B_PCH_PMC_GPI_ROUT_2                      (BIT5 | BIT4)
#define B_PCH_PMC_GPI_ROUT_3                      (BIT7 | BIT6)
#define B_PCH_PMC_GPI_ROUT_4                      (BIT9 | BIT8)
#define B_PCH_PMC_GPI_ROUT_5                      (BIT11 | BIT10)
#define B_PCH_PMC_GPI_ROUT_6                      (BIT13 | BIT12)
#define B_PCH_PMC_GPI_ROUT_7                      (BIT15 | BIT14)
#define B_PCH_PMC_GPI_ROUT_8                      (BIT17 | BIT16)
#define B_PCH_PMC_GPI_ROUT_9                      (BIT19 | BIT18)
#define B_PCH_PMC_GPI_ROUT_10                     (BIT21 | BIT20)
#define B_PCH_PMC_GPI_ROUT_11                     (BIT23 | BIT22)
#define B_PCH_PMC_GPI_ROUT_12                     (BIT25 | BIT24)
#define B_PCH_PMC_GPI_ROUT_13                     (BIT27 | BIT26)
#define B_PCH_PMC_GPI_ROUT_14                     (BIT29 | BIT28)
#define B_PCH_PMC_GPI_ROUT_15                     (BIT31 | BIT30)

#define R_PCH_PMC_PCC0                            0x60  // Platform Clock Control 0
#define B_PCH_PMC_PCC0_CLK_FREQ                   BIT2  // Clock Frequency
#define B_PCH_PMC_PCC0_CLK_CTL                    (BIT1 | BIT0) // Clock Gating

#define R_PCH_PMC_PCC1                            0x64  // Platform Clock Control 1
#define B_PCH_PMC_PCC1_CLK_FREQ                   BIT2  // Clock Frequency
#define B_PCH_PMC_PCC1_CLK_CTL                    (BIT1 | BIT0) // Clock Gating

#define R_PCH_PMC_PCC2                            0x68  // Platform Clock Control 2
#define B_PCH_PMC_PCC2_CLK_FREQ                   BIT2  // Clock Frequency
#define B_PCH_PMC_PCC2_CLK_CTL                    (BIT1 | BIT0) // Clock Gating

#define R_PCH_PMC_PCC3                            0x6C  // Platform Clock Control 3
#define B_PCH_PMC_PCC3_CLK_FREQ                   BIT2  // Clock Frequency
#define B_PCH_PMC_PCC3_CLK_CTL                    (BIT1 | BIT0) // Clock Gating

#define R_PCH_PMC_PCC4                            0x70  // Platform Clock Control 4
#define B_PCH_PMC_PCC4_CLK_FREQ                   BIT2  // Clock Frequency
#define B_PCH_PMC_PCC4_CLK_CTL                    (BIT1 | BIT0) // Clock Gating

#define R_PCH_PMC_PCC5                            0x74  // Platform Clock Control 5
#define B_PCH_PMC_PCC5_CLK_FREQ                   BIT2  // Clock Frequency
#define B_PCH_PMC_PCC5_CLK_CTL                    (BIT1 | BIT0) // Clock Gating

#define R_PCH_PMC_S0IR_TMR                        0x80  // S0I Ready Residency Timer
#define B_PCH_PMC_S0IR_TMR_RTIME                  0xFFFFFFFF // Time Spent in S0I Ready State

#define R_PCH_PMC_S0I1_TMR                        0x84  // S0I1 Ready Residency Timer
#define B_PCH_PMC_S0I1_TMR_RTIME                  0xFFFFFFFF // Time Spent in S0I1 Ready State

#define R_PCH_PMC_S0I2_TMR                        0x88  // S0I2 Ready Residency Timer
#define B_PCH_PMC_S0I2_TMR_RTIME                  0xFFFFFFFF // Time Spent in S0I2 Ready State

#define R_PCH_PMC_S0I3_TMR                        0x8C  // S0I3 Ready Residency Timer
#define B_PCH_PMC_S0I3_TMR_RTIME                  0xFFFFFFFF // Time Spent in S0I3 Ready State

#define R_PCH_PMC_S0_TMR                          0x90  // S0 Residency Timer
#define B_PCH_PMC_S0_TMR_RTIME                    0xFFFFFFFF // Time Spent in S0 State

#define R_PCH_PMC_PSS                             0x98  // Power Island Power Status
#define B_PCH_PMC_PSS_PG_STS                      0x3FFFF // Power Gate Status of All Power Islands
#define B_PCH_PMC_PSS_PG_STS_USB_SUS              BIT17 // USB SUS
#define B_PCH_PMC_PSS_PG_STS_USB                  BIT16 // USB
#define B_PCH_PMC_PSS_PG_STS_OTG_VCCACLK          BIT15 // OTG VCCACLK
#define B_PCH_PMC_PSS_PG_STS_OTG VCCA             BIT14 // OTG VCCA
#define B_PCH_PMC_PSS_PG_STS_OTG_VCCS             BIT13 // OTG VCCS
#define B_PCH_PMC_PSS_PG_STS_OTG_CTL              BIT12 // OTG Control
#define B_PCH_PMC_PSS_PG_STS_USH_VCCA             BIT11 // USH VCCA
#define B_PCH_PMC_PSS_PG_STS_USH_VCCS             BIT10 // USH VCCS
#define B_PCH_PMC_PSS_PG_STS_USH_SUS              BIT9  // USH SUS
#define B_PCH_PMC_PSS_PG_STS_USH_CTL              BIT8  // USH Control
#define B_PCH_PMC_PSS_PG_STS_DFX                  BIT7  // DFX
#define B_PCH_PMC_PSS_PG_STS_LPE                  BIT6  // LPE Audio
#define B_PCH_PMC_PSS_PG_STS_LPSS                 BIT5  // LPSS
#define B_PCH_PMC_PSS_PG_STS_PCIE                 BIT4  // PCIe
#define B_PCH_PMC_PSS_PG_STS_HDA                  BIT2  // HDA
#define B_PCH_PMC_PSS_PG_STS_SATA                 BIT1  // SATA

#define R_PCH_PMC_D3_STS_0                        0xA0  // D3 Status 0
#define B_PCH_PMC_D3_STS_0_LPSS1F7                BIT31 // LPSS 1 Function 7
#define B_PCH_PMC_D3_STS_0_LPSS1F6                BIT30 // LPSS 1 Function 6
#define B_PCH_PMC_D3_STS_0_LPSS1F5                BIT29 // LPSS 1 Function 5
#define B_PCH_PMC_D3_STS_0_LPSS1F4                BIT28 // LPSS 1 Function 4
#define B_PCH_PMC_D3_STS_0_LPSS1F3                BIT27 // LPSS 1 Function 3
#define B_PCH_PMC_D3_STS_0_LPSS1F2                BIT26 // LPSS 1 Function 2
#define B_PCH_PMC_D3_STS_0_LPSS1F1                BIT25 // LPSS 1 Function 1
#define B_PCH_PMC_D3_STS_0_LPSS1F0                BIT24 // LPSS 1 Function 0
#define B_PCH_PMC_D3_STS_0_PCIEF3                 BIT23 // PCIe Function 3
#define B_PCH_PMC_D3_STS_0_PCIEF2                 BIT22 // PCIe Function 2
#define B_PCH_PMC_D3_STS_0_PCIEF1                 BIT21 // PCIe Function 1
#define B_PCH_PMC_D3_STS_0_PCIEF0                 BIT20 // PCIe Function 0
#define B_PCH_PMC_D3_STS_0_USB                    BIT18 // USB
#define B_PCH_PMC_D3_STS_0_SATA                   BIT17 // SATA
#define B_PCH_PMC_D3_STS_0_USH                    BIT15 // USH
#define B_PCH_PMC_D3_STS_0_OTG                    BIT14 // OTG
#define B_PCH_PMC_D3_STS_0_LPE                    BIT13 // LPE
#define B_PCH_PMC_D3_STS_0_HDA                    BIT12 // HDA
#define B_PCH_PMC_D3_STS_0_MIPI                   BIT11 // MIPI-HSI
#define B_PCH_PMC_D3_STS_0_SCCF2                  BIT10 // SCC Function 2
#define B_PCH_PMC_D3_STS_0_SCCF1                  BIT9  // SCC Function 1
#define B_PCH_PMC_D3_STS_0_SCCF0                  BIT8  // SCC Function 0
#define B_PCH_PMC_D3_STS_0_LPSS0F7                BIT7  // LPSS 0 Function 7
#define B_PCH_PMC_D3_STS_0_LPSS0F6                BIT6  // LPSS 0 Function 6
#define B_PCH_PMC_D3_STS_0_LPSS0F5                BIT5  // LPSS 0 Function 5
#define B_PCH_PMC_D3_STS_0_LPSS0F4                BIT4  // LPSS 0 Function 4
#define B_PCH_PMC_D3_STS_0_LPSS0F3                BIT3  // LPSS 0 Function 3
#define B_PCH_PMC_D3_STS_0_LPSS0F2                BIT2  // LPSS 0 Function 2
#define B_PCH_PMC_D3_STS_0_LPSS0F1                BIT1  // LPSS 0 Function 1
#define B_PCH_PMC_D3_STS_0_LPSS0F0                BIT0  // LPSS 0 Function 0

#define R_PCH_PMC_D3_STS_1                        0xA4  // D3 Status 1
#define B_PCH_PMC_D3_STS_1_DFX                    BIT3  // DFX
#define B_PCH_PMC_D3_STS_1_OTG_SS                 BIT2  // OTG SS
#define B_PCH_PMC_D3_STS_1_USH_SS                 BIT1  // USH SS
#define B_PCH_PMC_D3_STS_1_SMB                    BIT0  // SMBus
#define R_PCH_PMC_D3_STDBY_STS_0                  0xA8  // D3 Standby Status 0
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS1F7          BIT31 // LPSS 1 Function 7
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS1F6          BIT30 // LPSS 1 Function 6
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS1F5          BIT29 // LPSS 1 Function 5
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS1F4          BIT28 // LPSS 1 Function 4
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS1F3          BIT27 // LPSS 1 Function 3
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS1F2          BIT26 // LPSS 1 Function 2
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS1F1          BIT25 // LPSS 1 Function 1
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS1F0          BIT24 // LPSS 1 Function 0
#define B_PCH_PMC_D3_STDBY_STS_0_PCIEF3           BIT23 // PCIe Function 3
#define B_PCH_PMC_D3_STDBY_STS_0_PCIEF2           BIT22 // PCIe Function 2
#define B_PCH_PMC_D3_STDBY_STS_0_PCIEF1           BIT21 // PCIe Function 1
#define B_PCH_PMC_D3_STDBY_STS_0_PCIEF0           BIT20 // PCIe Function 0
#define B_PCH_PMC_D3_STDBY_STS_0_USB              BIT18 // USB
#define B_PCH_PMC_D3_STDBY_STS_0_SATA             BIT17 // SATA
#define B_PCH_PMC_D3_STDBY_STS_0_USH              BIT15 // USH
#define B_PCH_PMC_D3_STDBY_STS_0_OTG              BIT14 // OTG
#define B_PCH_PMC_D3_STDBY_STS_0_LPE              BIT13 // LPE
#define B_PCH_PMC_D3_STDBY_STS_0_HDA              BIT12 // HDA
#define B_PCH_PMC_D3_STDBY_STS_0_MIPI             BIT11 // MIPI-HSI
#define B_PCH_PMC_D3_STDBY_STS_0_SCCF2            BIT10 // SCC Function 2
#define B_PCH_PMC_D3_STDBY_STS_0_SCCF1            BIT9  // SCC Function 1
#define B_PCH_PMC_D3_STDBY_STS_0_SCCF0            BIT8  // SCC Function 0
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS0F7          BIT7  // LPSS 0 Function 7
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS0F6          BIT6  // LPSS 0 Function 6
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS0F5          BIT5  // LPSS 0 Function 5
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS0F4          BIT4  // LPSS 0 Function 4
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS0F3          BIT3  // LPSS 0 Function 3
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS0F2          BIT2  // LPSS 0 Function 2
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS0F1          BIT1  // LPSS 0 Function 1
#define B_PCH_PMC_D3_STDBY_STS_0_LPSS0F0          BIT0  // LPSS 0 Function 0

#define R_PCH_PMC_D3_STDBY_STS_1                  0xAC  // D3 Standby Status 1
#define B_PCH_PMC_D3_STDBY_STS_1_DFX              BIT3  // DFX
#define B_PCH_PMC_D3_STDBY_STS_1_OTG_SS           BIT2  // OTG SS
#define B_PCH_PMC_D3_STDBY_STS_1_USH_SS           BIT1  // USH SS
#define B_PCH_PMC_D3_STDBY_STS_1_SMB              BIT0  // SMBus

#define R_PCH_PMC_MTPMC1                          0xB0  // Message to PMC 1

#define R_PCH_PMC_MTPMC2                          0xB4  // Message to PMC 2

#define R_PCH_PMC_MTPMC3                          0xB8  // Message to PMC 3

#define R_PCH_PMC_MTPMC4                          0xBC  // Message to PMC 4

//
// IO Memory Space Registers (IOBASE)
//
#define R_PCH_CFIO_PAD_CONF0                      0x00  // CFIO PAD_CONF0
#define R_PCH_CFIO_PAD_CONF1                      0x04  // CFIO PAD_CONF1
#define R_PCH_CFIO_PAD_VAL                        0x08  // CFIO PAD_VAL
#define R_PCH_CFIO_PAD_DFT                        0x0C  // CFIO PAD_CFT

//
// GPIO Register Offsets from GBASE
//
#define R_PCH_GPIO_SC_USE_SEL                     0x00  // GPIO South Usage Select [31:0]
#define R_PCH_GPIO_SC_IO_SEL                      0x04  // GPIO South Input / Output Select [31:0]
#define R_PCH_GPIO_SC_LVL                         0x08  // GPIO South Level for Input or Output [31:0]

#define R_PCH_GPIO_SC_TPE                         0x0C  // GPIO South Trigger Positive Edge Enable [31:0]
#define R_PCH_GPIO_SC_TNE                         0x10  // GPIO South Trigger Negative Edge Enable [31:0]
#define R_PCH_GPIO_SC_TS                          0x14  // GPIO South Trigger Status [31:0]

#define R_PCH_GPIO_SC_USE_SEL2                    0x20  // GPIO South Usage Select 2 [63:32]
#define R_PCH_GPIO_SC_IO_SEL2                     0x24  // GPIO South Input / Output Select 2 [63:32]
#define R_PCH_GPIO_SC_LVL2                        0x28  // GPIO South Level for Input or Output 2 [63:32]

#define R_PCH_GPIO_SC_TPE2                        0x2C  // GPIO South Trigger Positive Edge Enable 2 [63:32]
#define R_PCH_GPIO_SC_TNE2                        0x30  // GPIO South Trigger Negative Edge Enable 2 [63:32]
#define R_PCH_GPIO_SC_TS2                         0x34  // GPIO South Trigger Status 2 [63:32]

#define R_PCH_GPIO_SC_USE_SEL3                    0x40  // GPIO South Usage Select 3 [95:64]
#define R_PCH_GPIO_SC_IO_SEL3                     0x44  // GPIO South Input / Output Select 3 [95:64]
#define R_PCH_GPIO_SC_LVL3                        0x48  // GPIO South Level for Input or Output 3 [95:64]

#define R_PCH_GPIO_SC_TPE3                        0x4C  // GPIO South Trigger Positive Edge Enable 3 [95:64]
#define R_PCH_GPIO_SC_TNE3                        0x50  // GPIO South Trigger Negative Edge Enable 3 [95:64]
#define R_PCH_GPIO_SC_TS3                         0x54  // GPIO South Trigger Status 3 [95:64]

#define R_PCH_GPIO_SC_USE_SEL4                    0x60  // GPIO South Usage Select 4 [127:96]
#define R_PCH_GPIO_SC_IO_SEL4                     0x64  // GPIO South Input / Output Select 4 [127:96]
#define R_PCH_GPIO_SC_LVL4                        0x68  // GPIO South Level for Input or Output 4 [127:96]

#define R_PCH_GPIO_SC_TPE4                        0x6C  // GPIO South Trigger Positive Edge Enable 4 [127:96]
#define R_PCH_GPIO_SC_TNE4                        0x70  // GPIO South Trigger Negative Edge Enable 4 [127:96]
#define R_PCH_GPIO_SC_TS4                         0x74  // GPIO South Trigger Status 4 [127:96]

#define R_PCH_GPIO_SUS_USE_SEL                    0x80  // GPIO Suspend Use Select [31:0]
#define R_PCH_GPIO_SUS_IO_SEL                     0x84  // GPIO Suspend Input / Output Select [31:0]
#define R_PCH_GPIO_SUS_LVL                        0x88  // GPIO Suspend Level for Input or Output [31:0]

#define R_PCH_GPIO_SUS_TPE                        0x8C  // GPIO Suspend Trigger Positive Edge Enable [31:0]
#define R_PCH_GPIO_SUS_TNE                        0x90  // GPIO Suspend Trigger Negative Edge Enable [31:0]
#define R_PCH_GPIO_SUS_TS                         0x94  // GPIO Suspend Trigger Status [31:0]

#define R_PCH_GPIO_SUS_WAKE_EN                    0x98  // GPIO Suspend Wake Enable [31:0]

#define R_PCH_GPIO_SUS_USE_SEL2                   0x100 // GPIO Suspend Use Select 2 [42:32]
#define R_PCH_GPIO_SUS_IO_SEL2                    0x104 // GPIO Suspend Input / Output Select 2 [42:32]
#define R_PCH_GPIO_SUS_LVL2                       0x108 // GPIO Suspend Level for Input or Output 2 [42:32]

#define R_PCH_GPIO_SUS_TPE2                       0x10C // GPIO Suspend Trigger Positive Edge Enable [42:32]
#define R_PCH_GPIO_SUS_TNE2                       0x110 // GPIO Suspend Trigger Negative Edge Enable [42:32]
#define R_PCH_GPIO_SUS_TS2                        0x114 // GPIO Suspend Trigger Status [42:32]

#define R_PCH_GPIO_SUS_WAKE_EN2                   0x118 // GPIO Suspend Wake Enable 2 [42:32]

//
// Fixed IO Space
//

//
// Processor Interface Registers
//
#define R_PCH_NMI_SC                              0x61  // NMI Status and Control
#define B_PCH_NMI_SC_SERR_NMI_STS                 BIT7  // SERR# NMI Status
#define B_PCH_NMI_SC_IOCHK_NMI_STS                BIT6  // IOCHK NMI Status
#define B_PCH_NMI_SC_TMR2_OUT_STS                 BIT5  // Timer Counter 2 Status
#define B_PCH_NMI_SC_REF_TOGGLE                   BIT4  // Refresh Cycle toggle Status
#define B_PCH_NMI_SC_IOCHK_NMI_EN                 BIT3  // IOCHK NMI Enable
#define B_PCH_NMI_SC_PCI_SERR_EN                  BIT2  // SERR# NMI Enable
#define B_PCH_NMI_SC_SPKR_DAT_EN                  BIT1  // Speaker Data Enable
#define B_PCH_NMI_SC_TIM_CNT2_EN                  BIT0  // Timer Counter 2 Enable

#define R_PCH_NMI_EN                              0x70  // NMI Enable and Real Time Clock Index, Co-function with R_PCH_RTC_INDEX
#define B_PCH_NMI_EN_NMI_EN                       BIT7  // NMI Enable, must preserve this bit first before writing to IO port 0x70

//
// RTC Registers
//
#define R_PCH_RTC_INDEX                           0x70  // NMI Enable and Real Time Clock Index, Co-function with R_PCH_NMI_EN
#define R_PCH_RTC_TARGET                          0x71  // Real-Time Clock Target Register
#define R_PCH_RTC_EXT_INDEX                       0x72  // Extended RAM Index Register
#define R_PCH_RTC_EXT_TARGET                      0x73  // Extended RAM Target Register
#define R_PCH_RTC_INDEX2                          0x74  // Real-Time Clock Index Register
#define R_PCH_RTC_TARGET2                         0x75  // Real-Time Clock Target Register
#define R_PCH_RTC_EXT_INDEX2                      0x76  // Extended RAM Index Register
#define R_PCH_RTC_EXT_TARGET2                     0x77  // Extended RAM Target Register

#define R_PCH_RTC_SECONDS                         0x00  // Seconds, Range 0..59
#define R_PCH_RTC_SECONDSALARM                    0x01  // Seconds Alarm, Range 0..59
#define R_PCH_RTC_MINUTES                         0x02  // Minutes, Range 0..59
#define R_PCH_RTC_MINUTESALARM                    0x03  // Minutes Alarm, Range 0..59
#define R_PCH_RTC_HOURS                           0x04  // Hours, Range 1..12 or 0..23 Bit 7 is AM/PM
#define R_PCH_RTC_HOURSALARM                      0x05  // Hours Alarm, Range 1..12 or 0..23 Bit 7 is AM/PM
#define R_PCH_RTC_DAYOFWEEK                       0x06  // Day of Week, Range 1..7
#define R_PCH_RTC_DAYOFMONTH                      0x07  // Day of Month, Range 1..31
#define R_PCH_RTC_MONTH                           0x08  // Month, Range 1..12
#define R_PCH_RTC_YEAR                            0x09  // Year, Range 0..99

#define R_PCH_RTC_REGISTERA                       0x0A  // RTC Register A
#define B_PCH_RTC_REGISTERA_UIP                   BIT7  // Update In Progress
#define B_PCH_RTC_REGISTERA_DV                    (BIT6 | BIT5 | BIT4) // Division Chain Select
#define V_PCH_RTC_REGISTERA_DV_NORM_OP            0x20  // Normal Operation
#define V_PCH_RTC_REGISTERA_DV_BYP_5              0x30  // Bypass 5 Stages (Test mode only)
#define V_PCH_RTC_REGISTERA_DV_BYP_10             0x40  // Bypass 10 Stages (Test mode only)
#define V_PCH_RTC_REGISTERA_DV_BYP_15             0x50  // Bypass 15 Stages (Test mode only)
#define V_PCH_RTC_REGISTERA_DV_DIV_RST1           0x60  // Divider Reset
#define V_PCH_RTC_REGISTERA_DV_DIV_RST2           0x70  // Divider Reset
#define B_PCH_RTC_REGISTERA_RS                    (BIT3 | BIT2 | BIT1 | BIT0) // Rate Select
#define V_PCH_RTC_REGISTERA_RS_INT_NV_TGL         0x00  // Interrupt Never Toggles
#define V_PCH_RTC_REGISTERA_RS_3P906MS1           0x01  // 3.90625 ms
#define V_PCH_RTC_REGISTERA_RS_7P812MS1           0x02  // 7.8125 ms
#define V_PCH_RTC_REGISTERA_RS_122P0US            0x03  // 122.070 us
#define V_PCH_RTC_REGISTERA_RS_244P1US            0x04  // 244.141 us
#define V_PCH_RTC_REGISTERA_RS_488P2US            0x05  // 488.281 us
#define V_PCH_RTC_REGISTERA_RS_976P5US            0x06  // 976.5625 us
#define V_PCH_RTC_REGISTERA_RS_1P953MS            0x07  // 1.953125 ms
#define V_PCH_RTC_REGISTERA_RS_3P906MS            0x08  // 3.90625 ms
#define V_PCH_RTC_REGISTERA_RS_7P812MS            0x09  // 7.8125 ms
#define V_PCH_RTC_REGISTERA_RS_15P62MS            0x0A  // 15.625 ms
#define V_PCH_RTC_REGISTERA_RS_31P25MS            0x0B  // 31.25 ms
#define V_PCH_RTC_REGISTERA_RS_62P5MS             0x0C  // 62.5 ms
#define V_PCH_RTC_REGISTERA_RS_125MS              0x0D  // 125 ms
#define V_PCH_RTC_REGISTERA_RS_250MS              0x0E  // 250 ms
#define V_PCH_RTC_REGISTERA_RS_500MS              0x0F  // 500 ms

#define R_PCH_RTC_REGISTERB                       0x0B  // RTC Register B
#define B_PCH_RTC_REGISTERB_SET                   BIT7  // Update Cycle Inhibit 1: Stop auto update, begin set value; 0: Update cycle occurs
#define B_PCH_RTC_REGISTERB_PIE                   BIT6  // Periodic Interrupt Enable
#define B_PCH_RTC_REGISTERB_AIE                   BIT5  // Alarm Interrupt Enable
#define B_PCH_RTC_REGISTERB_UIE                   BIT4  // Update-ended Interrupt Enable
#define B_PCH_RTC_REGISTERB_SQWE                  BIT3  // Square Wave Enable (Not implemented)
#define B_PCH_RTC_REGISTERB_DM                    BIT2  // Data Mode 1: Binary; 0:BCD
#define B_PCH_RTC_REGISTERB_HF                    BIT1  // Hour Format 1: 24 mode; 0: 12 mode.
#define B_PCH_RTC_REGISTERB_DSE                   BIT0  // Daylight Savings Enable (Not Implemented)

#define R_PCH_RTC_REGISTERC                       0x0C  // RTC Register C
#define B_PCH_RTC_REGISTERC_IRQF                  BIT7  // Interrupt Request Flag
#define B_PCH_RTC_REGISTERC_PF                    BIT6  // Periodic Interrupt Flag
#define B_PCH_RTC_REGISTERC_AF                    BIT5  // Alarm Flag
#define B_PCH_RTC_REGISTERC_UF                    BIT4  // Update-ended Flag
#define B_PCH_RTC_REGISTERC_RESERVED              (BIT3 | BIT2 | BIT1 | BIT0)

#define R_PCH_RTC_REGISTERD                       0x0D  // RTC Register D
#define B_PCH_RTC_REGISTERD_VRT                   BIT7  // Valid RAM and Time Bit
#define B_PCH_RTC_REGISTERD_RESERVED              BIT6
#define B_PCH_RTC_REGISTERD_DA                    0x3F  // Date Alarm

#define B_PCH_RTC_CENTURY                         0x32  // Century Data

//
// APM Registers
//
#define R_PCH_APM_CNT                             0xB2  // Advanced Power Management Control Port
#define R_PCH_APM_STS                             0xB3  // Advanced Power Management Status Port

//
// INIT Register
//
#define R_PCH_PORT92                              0x92
#define B_PCH_PORT92_ALT_A20_GATE                 BIT1  // Alternate A20 Gate
#define B_PCH_PORT92_INIT_NOW                     BIT0  // Init Now

//
// PCU UART
//
#define R_PCH_COM1_BASE                           0x3F8 // COM1 IO BASE

//
// Reset Control Register
//
#define R_PCH_RST_CNT                             0xCF9 // Reset Control
#define B_PCH_RST_CNT_FULL_RST                    BIT3
#define B_PCH_RST_CNT_RST_CPU                     BIT2
#define B_PCH_RST_CNT_SYS_RST                     BIT1
#define V_PCH_RST_CNT_FULLRESET                   0x0E
#define V_PCH_RST_CNT_HARDRESET                   0x06
#define V_PCH_RST_CNT_SOFTRESET                   0x04  // Not supported by VLV
#define V_PCH_RST_CNT_HARDSTARTSTATE              0x02
#define V_PCH_RST_CNT_SOFTSTARTSTATE              0x00

//
// Fixed Memory Region
//

//
// IO APIC Registers
//
#define R_PCH_IO_APIC_INDEX                       0xFEC00000 // IOAPIC Index Register, 8bit
#define R_PCH_IO_APIC_WINDOW                      0xFEC00010 // IOAPIC Window Register, 32bit
#define R_PCH_IO_APIC_EOI                         0xFEC00040 // IOAPIC EOI Register, 8bit

#define R_PCH_IO_APIC_ID                          0x00  // Identification
#define B_PCH_IO_APIC_ID_AID                      (BIT27 | BIT26 | BIT25 | BIT24) // APIC Identification

#define R_PCH_IO_APIC_VS                          0x01  // Version
#define B_PCH_IO_APIC_VS_MRE                      0xFF0000 // Maximum Redirection Entries
#define B_PCH_IO_APIC_VS_PRQ                      BIT15 // Pin Assertion Register Supported
#define B_PCH_IO_APIC_VS_VS                       0xFF  // Version

//
// HPET Registers
//
#define R_PCH_PCH_HPET                            0xFED00000 // HPET Base Address

#define R_PCH_PCH_HPET_GCID                       0x00  // HPET General Capabilities and ID, 64bit
#define B_PCH_PCH_HPET_GCID_CTP                   0xFFFFFFFF00000000 // Counter Tick Period
#define B_PCH_PCH_HPET_GCID_VID                   0xFFFF0000 // Vendor ID
#define B_PCH_PCH_HPET_GCID_LRC                   BIT15 // Legacy Rout Capable
#define B_PCH_PCH_HPET_GCID_CS                    BIT13 // Counter Size
#define B_PCH_PCH_HPET_GCID_NT                    0x1F00 // Number of Timers
#define B_PCH_PCH_HPET_GCID_RID                   0xFF  // Revision ID
#define N_PCH_HPET_ADDR_ASEL                      12

#define R_PCH_PCH_HPET_GCFG                       0x10  // HPET General Configuration
#define B_PCH_PCH_HPET_GCFG_LRE                   BIT1  // Legacy Rout Enable
#define B_PCH_PCH_HPET_GCFG_EN                    BIT0  // Overall Enable

#define R_PCH_PCH_HPET_GIS                        0x20  // HPET General Interrupt Status
#define B_PCH_PCH_HPET_GIS_T2                     BIT2  // Timer 2 Status
#define B_PCH_PCH_HPET_GIS_T1                     BIT1  // Timer 1 Status
#define B_PCH_PCH_HPET_GIS_T0                     BIT0  // Timer 0 Status

#define R_PCH_PCH_HPET_MCV                        0xF0  // HPET Main Counter Value, 64bit

#define R_PCH_PCH_HPET_T0C                        0x100 // HPET Timer 0 Config and Capabilities
#define R_PCH_PCH_HPET_T0CV_L                     0x108 // HPET Timer 0 Lower Comparator Value
#define R_PCH_PCH_HPET_T0CV_H                     0x10C // HPET Timer 0 Upper Comparator Value

#define R_PCH_PCH_HPET_T1C                        0x120 // HPET Timer 1 Config and Capabilities
#define R_PCH_PCH_HPET_T1CV                       0x128 // HPET Timer 1 Comparator Value

#define R_PCH_PCH_HPET_T2C                        0x140 // HPET Timer 2 Config and Capabilities
#define R_PCH_PCH_HPET_T2CV                       0x148 // HPET Timer 2 Comparator Value

#define B_PCH_PCH_HPET_TXC_IRC                    0xFFFFFFFF00000000 // Interrupt Rout Capability
#define B_PCH_PCH_HPET_TXC_FID                    BIT15 // FSB Interrupt Delivery
#define B_PCH_PCH_HPET_TXC_FE                     BIT14 // FSB Enable
#define B_PCH_PCH_HPET_TXC_IR                     0x3E00 // Interrupt Rout
#define B_PCH_PCH_HPET_TXC_T32M                   BIT8  // Timer 32-bit Mode
#define B_PCH_PCH_HPET_TXC_TVS                    BIT6  // Timer Value Set
#define B_PCH_PCH_HPET_TXC_TS                     BIT5  // Timer Size
#define B_PCH_PCH_HPET_TXC_PIC                    BIT4  // Periodic Interrupt Capable
#define B_PCH_PCH_HPET_TXC_TYP                    BIT3  // Timer Type
#define B_PCH_PCH_HPET_TXC_IE                     BIT2  // Interrupt Enable
#define B_PCH_PCH_HPET_TXC_IT                     BIT1  // Timer Interrupt Type

#endif
