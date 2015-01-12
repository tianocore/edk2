/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



  @file
  PchRegsHda.h

  @brief
  Register names for PCH High Definition Audio device.

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
#ifndef _PCH_REGS_HDA_H_
#define _PCH_REGS_HDA_H_

///
/// Azalia Controller Registers (D27:F0)
///
#define PCI_DEVICE_NUMBER_PCH_AZALIA       27
#define PCI_FUNCTION_NUMBER_PCH_AZALIA     0

#define R_PCH_HDA_PCS                      0x54  // Power Management Control and Status
#define B_PCH_HDA_PCS_DATA                 0xFF000000 // Data, does not apply
#define B_PCH_HDA_PCS_CCE                  BIT23 // Bus Power Control Enable, does not apply
#define B_PCH_HDA_PCS_PMES                 BIT15 // PME Status
#define B_PCH_HDA_PCS_PMEE                 BIT8  // PME Enable
#define B_PCH_HDA_PCS_PS                   (BIT1 | BIT0) // Power State - D0/D3 Hot
#define V_PCH_HDA_PCS_PS0                  0x00
#define V_PCH_HDA_PCS_PS3                  0x03

#endif
