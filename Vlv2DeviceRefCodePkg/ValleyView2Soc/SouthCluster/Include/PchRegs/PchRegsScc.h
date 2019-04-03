/*++

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



Module Name:

  PchRegsScc.h

Abstract:

  Register names for VLV SCC module.

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
#ifndef _PCH_REGS_SCC_H_
#define _PCH_REGS_SCC_H_


//
// SCC Modules Registers
//

//
// SCC SDIO Modules
// PCI Config Space Registers
//
#define PCI_DEVICE_NUMBER_PCH_SCC_SDIO_0         16
#define PCI_DEVICE_NUMBER_PCH_SCC_SDIO_1         17
#define PCI_DEVICE_NUMBER_PCH_SCC_SDIO_2         18
#define PCI_DEVICE_NUMBER_PCH_SCC_SDIO_3         23

#define PCI_FUNCTION_NUMBER_PCH_SCC_SDIO         0

#endif
