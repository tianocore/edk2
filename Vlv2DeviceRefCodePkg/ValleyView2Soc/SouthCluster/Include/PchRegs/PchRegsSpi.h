/**

Copyright (c) 2011  - 2015, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



  @file
  PchRegsSpi.h

  @brief
  Register names for PCH SPI device.

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

**/
#ifndef _PCH_REGS_SPI_H_
#define _PCH_REGS_SPI_H_

///
/// SPI Host Interface Registers
///

#define R_PCH_SPI_HSFS                       0x04  // Hardware Sequencing Flash Status Register (16bits)
#define B_PCH_SPI_HSFS_FLOCKDN               BIT15 // Flash Configuration Lock-Down
#define B_PCH_SPI_HSFS_FDV                   BIT14 // Flash Descriptor Valid
#define B_PCH_SPI_HSFS_FDOPSS                BIT13 // Flash Descriptor Override Pin-Strap Status
#define B_PCH_SPI_HSFS_SCIP                  BIT5  // SPI Cycle in Progress
#define B_PCH_SPI_HSFS_BERASE_MASK           (BIT4 | BIT3) // Block / Sector Erase Size
#define V_PCH_SPI_HSFS_BERASE_256B           0x00  // Block/Sector = 256 Bytes
#define V_PCH_SPI_HSFS_BERASE_4K             0x01  // Block/Sector = 4K Bytes
#define V_PCH_SPI_HSFS_BERASE_8K             0x10  // Block/Sector = 8K Bytes
#define V_PCH_SPI_HSFS_BERASE_64K            0x11  // Block/Sector = 64K Bytes
#define B_PCH_SPI_HSFS_AEL                   BIT2  // Access Error Log
#define B_PCH_SPI_HSFS_FCERR                 BIT1  // Flash Cycle Error
#define B_PCH_SPI_HSFS_FDONE                 BIT0  // Flash Cycle Done

#define R_PCH_SPI_PR0                        0x74  // Protected Region 0 Register
#define B_PCH_SPI_PR0_WPE                    BIT31 // Write Protection Enable
#define B_PCH_SPI_PR0_PRL_MASK               0x1FFF0000 // Protected Range Limit Mask, [28:16] here represents upper limit of address [24:12]
#define B_PCH_SPI_PR0_RPE                    BIT15 // Read Protection Enable
#define B_PCH_SPI_PR0_PRB_MASK               0x00001FFF // Protected Range Base Mask, [12:0] here represents base limit of address [24:12]

#define R_PCH_SPI_PR1                        0x78  // Protected Region 1 Register
#define B_PCH_SPI_PR1_WPE                    BIT31 // Write Protection Enable
#define B_PCH_SPI_PR1_PRL_MASK               0x1FFF0000 // Protected Range Limit Mask, [28:16] here represents upper limit of address [24:12]
#define B_PCH_SPI_PR1_RPE                    BIT15 // Read Protection Enable
#define B_PCH_SPI_PR1_PRB_MASK               0x00001FFF // Protected Range Base Mask, [12:0] here represents base limit of address [24:12]

#define R_PCH_SPI_PREOP                      0x94  // Prefix Opcode Configuration Register (16 bits)
#define B_PCH_SPI_PREOP1_MASK                0xFF00 // Prefix Opcode 1 Mask
#define B_PCH_SPI_PREOP0_MASK                0x00FF // Prefix Opcode 0 Mask

#define R_PCH_SPI_OPTYPE                     0x96  // Opcode Type Configuration
#define B_PCH_SPI_OPTYPE7_MASK               (BIT15 | BIT14) // Opcode Type 7 Mask
#define B_PCH_SPI_OPTYPE6_MASK               (BIT13 | BIT12) // Opcode Type 6 Mask
#define B_PCH_SPI_OPTYPE5_MASK               (BIT11 | BIT10) // Opcode Type 5 Mask
#define B_PCH_SPI_OPTYPE4_MASK               (BIT9 | BIT8) // Opcode Type 4 Mask
#define B_PCH_SPI_OPTYPE3_MASK               (BIT7 | BIT6) // Opcode Type 3 Mask
#define B_PCH_SPI_OPTYPE2_MASK               (BIT5 | BIT4) // Opcode Type 2 Mask
#define B_PCH_SPI_OPTYPE1_MASK               (BIT3 | BIT2) // Opcode Type 1 Mask
#define B_PCH_SPI_OPTYPE0_MASK               (BIT1 | BIT0) // Opcode Type 0 Mask
#define V_PCH_SPI_OPTYPE_RDNOADDR            0x00  // Read cycle type without address
#define V_PCH_SPI_OPTYPE_WRNOADDR            0x01  // Write cycle type without address
#define V_PCH_SPI_OPTYPE_RDADDR              0x02  // Address required; Read cycle type
#define V_PCH_SPI_OPTYPE_WRADDR              0x03  // Address required; Write cycle type

#define R_PCH_SPI_OPMENU0                    0x98  // Opcode Menu Configuration 0 (32bits)
#define R_PCH_SPI_OPMENU1                    0x9C  // Opcode Menu Configuration 1 (32bits)

#define R_PCH_SPI_IND_LOCK                   0xA4  // Indvidual Lock
#define B_PCH_SPI_IND_LOCK_PR0               BIT2  // PR0 LockDown


#define R_PCH_SPI_FDOC                       0xB0  // Flash Descriptor Observability Control Register (32 bits)
#define B_PCH_SPI_FDOC_FDSS_MASK             (BIT14 | BIT13 | BIT12) // Flash Descriptor Section Select
#define V_PCH_SPI_FDOC_FDSS_FSDM             0x0000 // Flash Signature and Descriptor Map
#define V_PCH_SPI_FDOC_FDSS_COMP             0x1000 // Component
#define V_PCH_SPI_FDOC_FDSS_REGN             0x2000 // Region
#define V_PCH_SPI_FDOC_FDSS_MSTR             0x3000 // Master
#define V_PCH_SPI_FDOC_FDSS_VLVS             0x4000 // Soft Straps
#define B_PCH_SPI_FDOC_FDSI_MASK             0x0FFC // Flash Descriptor Section Index

#define R_PCH_SPI_FDOD                       0xB4  // Flash Descriptor Observability Data Register (32 bits)

#define R_PCH_SPI_BCR                        0xFC  // BIOS Control Register
#define S_PCH_SPI_BCR                        1
#define B_PCH_SPI_BCR_SMM_BWP                BIT5  // SMM BIOS Write Protect Disable
#define B_PCH_SPI_BCR_SRC                    (BIT3 | BIT2) // SPI Read Configuration (SRC)
#define V_PCH_SPI_BCR_SRC_PREF_EN_CACHE_EN   0x08  // Prefetch Enable, Cache Enable
#define V_PCH_SPI_BCR_SRC_PREF_DIS_CACHE_DIS 0x04  // Prefetch Disable, Cache Disable
#define V_PCH_SPI_BCR_SRC_PREF_DIS_CACHE_EN  0x00  // Prefetch Disable, Cache Enable
#define B_PCH_SPI_BCR_BLE                    BIT1  // Lock Enable (LE)
#define B_PCH_SPI_BCR_BIOSWE                 BIT0  // Write Protect Disable (WPD)
#define N_PCH_SPI_BCR_BLE                    1
#define N_PCH_SPI_BCR_BIOSWE                 0

//
// Flash Descriptor Base Address Region (FDBAR) from Flash Region 0
//
#define R_PCH_SPI_FDBAR_FLVALSIG             0x00  // Flash Valid Signature
#define V_PCH_SPI_FDBAR_FLVALSIG             0x0FF0A55A

#endif
