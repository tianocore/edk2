/** @file
  Register names for SPI device.

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REGS_SPI_H_
#define REGS_SPI_H_

#define R_SPI_BASE                      0x10           ///< 32-bit Memory Base Address Register
#define B_SPI_BAR0_MASK                 0x0FFF
#define R_SPI_BCR                       0xDC           ///< BIOS Control  Register
#define B_SPI_BCR_SRC                    (BIT3 | BIT2) ///< SPI Read Configuration (SRC)
#define V_SPI_BCR_SRC_PREF_DIS_CACHE_DIS 0x04          ///< Prefetch Disable, Cache Disable
#define B_SPI_BCR_SYNC_SS                BIT8
#define B_SPI_BCR_BIOSWE                 BIT0          ///< Write Protect Disable (WPD)

///
/// SPI Host Interface Registers
#define R_SPI_HSFS                       0x04          ///< Hardware Sequencing Flash Status and Control Register(32bits)
#define B_SPI_HSFS_FDBC_MASK             0x3F000000    ///< Flash Data Byte Count ( <= 64), Count = (Value in this field) + 1.
#define N_SPI_HSFS_FDBC                  24
#define B_SPI_HSFS_CYCLE_MASK            0x001E0000    ///< Flash Cycle.
#define N_SPI_HSFS_CYCLE                 17
#define V_SPI_HSFS_CYCLE_READ            0             ///< Flash Cycle Read
#define V_SPI_HSFS_CYCLE_WRITE           2             ///< Flash Cycle Write
#define V_SPI_HSFS_CYCLE_4K_ERASE        3             ///< Flash Cycle 4K Block Erase
#define V_SPI_HSFS_CYCLE_64K_ERASE       4             ///< Flash Cycle 64K Sector Erase
#define V_SPI_HSFS_CYCLE_READ_SFDP       5             ///< Flash Cycle Read SFDP
#define V_SPI_HSFS_CYCLE_READ_JEDEC_ID   6             ///< Flash Cycle Read JEDEC ID
#define V_SPI_HSFS_CYCLE_WRITE_STATUS    7             ///< Flash Cycle Write Status
#define V_SPI_HSFS_CYCLE_READ_STATUS     8             ///< Flash Cycle Read Status
#define B_SPI_HSFS_CYCLE_FGO             BIT16         ///< Flash Cycle Go.
#define B_SPI_HSFS_FDV                   BIT14         ///< Flash Descriptor Valid
#define B_SPI_HSFS_SCIP                  BIT5          ///< SPI Cycle in Progress
#define B_SPI_HSFS_FCERR                 BIT1          ///< Flash Cycle Error
#define B_SPI_HSFS_FDONE                 BIT0          ///< Flash Cycle Done


#define R_SPI_FADDR                      0x08   ///< SPI Flash Address
#define B_SPI_FADDR_MASK                 0x07FFFFFF ///< SPI Flash Address Mask (0~26bit)


#define R_SPI_FDATA00                    0x10  ///< SPI Data 00 (32 bits)

#define R_SPI_FRAP                       0x50  ///< SPI Flash Regions Access Permissions Register
#define B_SPI_FRAP_BRWA_PLATFORM         BIT12 //< Region write access for Region4 PlatformData
#define B_SPI_FRAP_BRWA_GBE              BIT11 //< Region write access for Region3 GbE
#define B_SPI_FRAP_BRWA_SEC              BIT10 ///< Region Write Access for Region2 SEC
#define B_SPI_FRAP_BRWA_BIOS             BIT9  ///< Region Write Access for Region1 BIOS
#define B_SPI_FRAP_BRWA_FLASHD           BIT8  ///< Region Write Access for Region0 Flash Descriptor
#define B_SPI_FRAP_BRRA_PLATFORM         BIT4       ///< Region read access for Region4 PlatformData
#define B_SPI_FRAP_BRRA_GBE              BIT3       ///< Region read access for Region3 GbE
#define B_SPI_FRAP_BRRA_SEC              BIT2       ///< Region Read Access for Region2 SEC
#define B_SPI_FRAP_BRRA_BIOS             BIT1       ///< Region Read Access for Region1 BIOS
#define B_SPI_FRAP_BRRA_FLASHD           BIT0       ///< Region Read Access for Region0 Flash Descriptor


#define R_SPI_FREG0_FLASHD               0x54       ///< Flash Region 0 (Flash Descriptor) (32bits)
#define B_SPI_FREG0_LIMIT_MASK           0x7FFF0000 ///< Size, [30:16] here represents limit[26:12]
#define N_SPI_FREG0_LIMIT                4          ///< Bit 30:16 identifies address bits [26:12]
#define B_SPI_FREG0_BASE_MASK            0x00007FFF ///< Base, [14:0]  here represents base [26:12]
#define N_SPI_FREG0_BASE                 12         ///< Bit 14:0 identifies address bits [26:2]

#define R_SPI_FREG1_BIOS                 0x58       ///< Flash Region 1 (BIOS) (32bits)
#define B_SPI_FREG1_LIMIT_MASK           0x7FFF0000 ///< Size, [30:16] here represents limit[26:12]
#define N_SPI_FREG1_LIMIT                4          ///< Bit 30:16 identifies address bits [26:12]
#define B_SPI_FREG1_BASE_MASK            0x00007FFF ///< Base, [14:0]  here represents base [26:12]
#define N_SPI_FREG1_BASE                 12         ///< Bit 14:0 identifies address bits [26:2]

#define R_SPI_FREG2_SEC                  0x5C       ///< Flash Region 2 (SEC) (32bits)
#define B_SPI_FREG2_LIMIT_MASK           0x7FFF0000 ///< Size, [30:16] here represents limit[26:12]
#define N_SPI_FREG2_LIMIT                4          //< Bit 30:16 identifies address bits [26:12]
#define B_SPI_FREG2_BASE_MASK            0x00007FFF ///< Base, [14:0]  here represents base [26:12]
#define N_SPI_FREG2_BASE                 12         //< Bit 14:0 identifies address bits [26:2]

#define R_SPI_FREG3_GBE                  0x60       //< Flash Region 3(GbE)(32bits)
#define B_SPI_FREG3_LIMIT_MASK           0x7FFF0000 ///< Size, [30:16] here represents limit[26:12]
#define N_SPI_FREG3_LIMIT                4          //< Bit 30:16 identifies address bits [26:12]
#define B_SPI_FREG3_BASE_MASK            0x00007FFF ///< Base, [14:0]  here represents base [26:12]
#define N_SPI_FREG3_BASE                 12         //< Bit 14:0 identifies address bits [26:2]

#define R_SPI_FREG4_PLATFORM_DATA        0x64       ///< Flash Region 4 (Platform Data) (32bits)
#define B_SPI_FREG4_LIMIT_MASK           0x7FFF0000 ///< Size, [30:16] here represents limit[26:12]
#define N_SPI_FREG4_LIMIT                4          ///< Bit 30:16 identifies address bits [26:12]
#define B_SPI_FREG4_BASE_MASK            0x00007FFF ///< Base, [14:0]  here represents base [26:12]
#define N_SPI_FREG4_BASE                 12         ///< Bit 14:0 identifies address bits [26:2]


#define S_SPI_FREGX                      4          ///< Size of Flash Region register
#define B_SPI_FREGX_LIMIT_MASK           0x7FFF0000 ///< Flash Region Limit [30:16] represents [26:12], [11:0] are assumed to be FFFh
#define N_SPI_FREGX_LIMIT                16         ///< Region limit bit position
#define N_SPI_FREGX_LIMIT_REPR           12         ///< Region limit bit represents position
#define B_SPI_FREGX_BASE_MASK            0x00007FFF ///< Flash Region Base, [14:0] represents [26:12]


#define R_SPI_FDOC                       0xB4  ///< Flash Descriptor Observability Control Register (32 bits)
#define B_SPI_FDOC_FDSS_MASK             (BIT14 | BIT13 | BIT12) ///< Flash Descriptor Section Select
#define V_SPI_FDOC_FDSS_FSDM             0x0000 ///< Flash Signature and Descriptor Map
#define V_SPI_FDOC_FDSS_COMP             0x1000 ///< Component
#define B_SPI_FDOC_FDSI_MASK             0x0FFC ///< Flash Descriptor Section Index

#define R_SPI_FDOD                       0xB8  ///< Flash Descriptor Observability Data Register (32 bits)


#define R_SPI_LVSCC                      0xC4  ///<Vendor Specific Component Capabilities for Component 0 (32 bits)
#define B_SPI_LVSCC_EO_64K               BIT29 ///<< 64k Erase valid (EO_64k_valid)

#define R_SPI_UVSCC                      0xC8  ///< Vendor Specific Component Capabilities for Component 1 (32 bits)


#define R_SPI_FDBAR_FLASH_MAP0           0x14  ///< Flash MAP 0
#define N_SPI_FDBAR_NC                   8       ///<< Number Of Components
#define B_SPI_FDBAR_NC                   0x00000300 ///< Number Of Components

#define R_SPI_FDBAR_FLASH_MAP1           0x18  ///< Flash MAP 1
#define B_SPI_FDBAR_FPSBA                0x00FF0000 ///< Flash Strap Base Address


//
// Flash Component Base Address (FCBA) from Flash Region 0
//
#define R_SPI_FCBA_FLCOMP                0x00  ///< Flash Components Register
#define B_SPI_FLCOMP_COMP1_MASK          0x0F  ///< Flash Component 1 Density


#endif
