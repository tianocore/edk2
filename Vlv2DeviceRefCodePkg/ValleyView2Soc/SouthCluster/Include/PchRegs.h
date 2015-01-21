/**

Copyright (c) 2011  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



  @file
  PchRegs.h

  @brief
  Register names for VLV SC.

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
#ifndef _PCH_REGS_H_
#define _PCH_REGS_H_

///
/// Bit Definitions. BUGBUG: drive these definitions to code base. Should not need
/// to be part of chipset modules
///
#ifndef BIT0
#define BIT0  0x0001
#define BIT1  0x0002
#define BIT2  0x0004
#define BIT3  0x0008
#define BIT4  0x0010
#define BIT5  0x0020
#define BIT6  0x0040
#define BIT7  0x0080
#define BIT8  0x0100
#define BIT9  0x0200
#define BIT10 0x0400
#define BIT11 0x0800
#define BIT12 0x1000
#define BIT13 0x2000
#define BIT14 0x4000
#define BIT15 0x8000
#define BIT16 0x00010000
#define BIT17 0x00020000
#define BIT18 0x00040000
#define BIT19 0x00080000
#define BIT20 0x00100000
#define BIT21 0x00200000
#define BIT22 0x00400000
#define BIT23 0x00800000
#define BIT24 0x01000000
#define BIT25 0x02000000
#define BIT26 0x04000000
#define BIT27 0x08000000
#define BIT28 0x10000000
#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000
#define BIT32 0x100000000
#define BIT33 0x200000000
#define BIT34 0x400000000
#define BIT35 0x800000000
#define BIT36 0x1000000000
#define BIT37 0x2000000000
#define BIT38 0x4000000000
#define BIT39 0x8000000000
#define BIT40 0x10000000000
#define BIT41 0x20000000000
#define BIT42 0x40000000000
#define BIT43 0x80000000000
#define BIT44 0x100000000000
#define BIT45 0x200000000000
#define BIT46 0x400000000000
#define BIT47 0x800000000000
#define BIT48 0x1000000000000
#define BIT49 0x2000000000000
#define BIT50 0x4000000000000
#define BIT51 0x8000000000000
#define BIT52 0x10000000000000
#define BIT53 0x20000000000000
#define BIT54 0x40000000000000
#define BIT55 0x80000000000000
#define BIT56 0x100000000000000
#define BIT57 0x200000000000000
#define BIT58 0x400000000000000
#define BIT59 0x800000000000000
#define BIT60 0x1000000000000000
#define BIT61 0x2000000000000000
#define BIT62 0x4000000000000000
#define BIT63 0x8000000000000000
#endif
///
/// The default PCH PCI bus number
///
#define DEFAULT_PCI_BUS_NUMBER_PCH  0

///
/// Default Vendor ID and Subsystem ID
///
#define V_PCH_INTEL_VENDOR_ID   0x8086
#define V_PCH_DEFAULT_SID       0x7270
#define V_PCH_DEFAULT_SVID_SID  (V_PCH_INTEL_VENDOR_ID + (V_PCH_DEFAULT_SID << 16))

///
/// Include device register definitions
///
#include "PchRegs/PchRegsHda.h"
#include "PchRegs/PchRegsLpss.h"
#include "PchRegs/PchRegsPcie.h"
#include "PchRegs/PchRegsPcu.h"
#include "PchRegs/PchRegsRcrb.h"
#include "PchRegs/PchRegsSata.h"
#include "PchRegs/PchRegsScc.h"
#include "PchRegs/PchRegsSmbus.h"
#include "PchRegs/PchRegsSpi.h"
#include "PchRegs/PchRegsUsb.h"
//#include "PchRegs/PchRegsLpe.h"

///
/// Device IDS that are PCH Server specific
///
#define IS_PCH_DEVICE_ID(DeviceId) \
    ( \
      (DeviceId == V_PCH_LPC_DEVICE_ID_0) || \
      (DeviceId == V_PCH_LPC_DEVICE_ID_1) || \
      (DeviceId == V_PCH_LPC_DEVICE_ID_2) || \
      (DeviceId == V_PCH_LPC_DEVICE_ID_3) \
    )

#define IS_PCH_VLV_LPC_DEVICE_ID(DeviceId) \
    ( \
      IS_PCH_DEVICE_ID (DeviceId) \
    )

#define IS_PCH_VLV_SATA_DEVICE_ID(DeviceId) \
    ( \
      IS_PCH_VLV_SATA_AHCI_DEVICE_ID (DeviceId) || \
      IS_PCH_VLV_SATA_MODE_DEVICE_ID (DeviceId) || \
      IS_PCH_VLV_SATA_RAID_DEVICE_ID (DeviceId) \
    )

#define IS_PCH_VLV_SATA_AHCI_DEVICE_ID(DeviceId) \
    ( \
      (DeviceId == V_PCH_SATA_DEVICE_ID_D_AHCI) || \
      (DeviceId == V_PCH_SATA_DEVICE_ID_M_AHCI) \
    )

#define IS_PCH_VLV_SATA_RAID_DEVICE_ID(DeviceId) \
    ( \
      (DeviceId == V_PCH_SATA_DEVICE_ID_D_RAID) || \
      (DeviceId == V_PCH_SATA_DEVICE_ID_M_RAID) \
    )

#define IS_PCH_VLV_SATA_MODE_DEVICE_ID(DeviceId) \
    ( \
      (DeviceId == V_PCH_SATA_DEVICE_ID_D_IDE) || \
      (DeviceId == V_PCH_SATA_DEVICE_ID_M_IDE) \
    )
#define IS_PCH_VLV_USB_DEVICE_ID(DeviceId) \
    ( \
      (DeviceId == V_PCH_USB_DEVICE_ID_0) || \
      (DeviceId == V_PCH_USB_DEVICE_ID_1) \
    )
#define IS_PCH_VLV_PCIE_DEVICE_ID(DeviceId) \
    ( \
      (DeviceId == V_PCH_PCIE_DEVICE_ID_0) || \
      (DeviceId == V_PCH_PCIE_DEVICE_ID_1) || \
      (DeviceId == V_PCH_PCIE_DEVICE_ID_2) || \
      (DeviceId == V_PCH_PCIE_DEVICE_ID_3) || \
      (DeviceId == V_PCH_PCIE_DEVICE_ID_4) || \
      (DeviceId == V_PCH_PCIE_DEVICE_ID_5) || \
      (DeviceId == V_PCH_PCIE_DEVICE_ID_6) || \
      (DeviceId == V_PCH_PCIE_DEVICE_ID_7) \
    )

///
/// Any device ID that is Valleyview SC
///
#define IS_PCH_VLV_DEVICE_ID(DeviceId) \
    ( \
      IS_PCH_VLV_LPC_DEVICE_ID (DeviceId) || \
      IS_PCH_VLV_SATA_DEVICE_ID (DeviceId) || \
      IS_PCH_VLV_USB_DEVICE_ID (DeviceId) || \
      IS_PCH_VLV_PCIE_DEVICE_ID (DeviceId) || \
      (DeviceId) == V_PCH_SMBUS_DEVICE_ID || \
      (DeviceId) == V_PCH_HDA_DEVICE_ID_0 || \
      (DeviceId) == V_PCH_HDA_DEVICE_ID_1 \
    )

#define IS_SUPPORTED_DEVICE_ID(DeviceId)  IS_PCH_VLV_DEVICE_ID (DeviceId)

#endif
