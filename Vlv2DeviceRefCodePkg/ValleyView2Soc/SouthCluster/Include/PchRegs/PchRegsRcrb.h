/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



  @file
  PchRegsRcrb.h

  @brief
  Register names for VLV Chipset Configuration Registers

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
#ifndef _PCH_REGS_RCRB_H_
#define _PCH_REGS_RCRB_H_

///
/// Chipset Configuration Registers (Memory space)
/// RCBA
///
#define R_PCH_RCRB_GCS                    0x00  // General Control and Status
#define B_PCH_RCRB_GCS_BBSIZE             (BIT30 | BIT29) // Boot Block Size
#define B_PCH_RCRB_GCS_BBS                (BIT11 | BIT10) // Boot BIOS Straps
#define V_PCH_RCRB_GCS_BBS_SPI            (3 << 10) // Boot BIOS strapped to SPI
#define V_PCH_RCRB_GCS_BBS_LPC            (0 << 10) // Boot BIOS strapped to LPC
#define B_PCH_RCRB_GCS_TS                 BIT1 // Top Swap
#define B_PCH_RCRB_GCS_BILD               BIT0 // BIOS Interface Lock-Down


#endif
