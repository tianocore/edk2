/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



  @file
  PchRegsUsb.h

  @brief
  Register names for PCH USB devices.

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
#ifndef _PCH_REGS_USB_H_
#define _PCH_REGS_USB_H_

///
/// USB Definitions
///

typedef enum {
  PchEhci1 = 0,
  PchEhciControllerMax
} PCH_USB20_CONTROLLER_TYPE;

#define PCH_USB_MAX_PHYSICAL_PORTS          4      /// Max Physical Connector EHCI + XHCI, not counting virtual ports like USB-R.
#define PCH_EHCI_MAX_PORTS                  4      /// Counting ports behind RMHs 8 from EHCI-1 and 6 from EHCI-2, not counting EHCI USB-R virtual ports.
#define PCH_HSIC_MAX_PORTS                  2
#define PCH_XHCI_MAX_USB3_PORTS             1

#define PCI_DEVICE_NUMBER_PCH_USB           29
#define PCI_FUNCTION_NUMBER_PCH_EHCI        0

#define R_PCH_USB_VENDOR_ID                 0x00  // Vendor ID
#define V_PCH_USB_VENDOR_ID                 V_PCH_INTEL_VENDOR_ID

#define R_PCH_USB_DEVICE_ID                 0x02  // Device ID
#define V_PCH_USB_DEVICE_ID_0               0x0F34  // EHCI#1

#define R_PCH_EHCI_SVID                     0x2C  // USB2 Subsystem Vendor ID
#define B_PCH_EHCI_SVID                     0xFFFF // USB2 Subsystem Vendor ID Mask

#define R_PCH_EHCI_PWR_CNTL_STS             0x54  // Power Management Control / Status
#define B_PCH_EHCI_PWR_CNTL_STS_PME_STS     BIT15 // PME Status
#define B_PCH_EHCI_PWR_CNTL_STS_DATASCL     (BIT14 | BIT13) // Data Scale
#define B_PCH_EHCI_PWR_CNTL_STS_DATASEL     (BIT12 | BIT11 | BIT10 | BIT9) // Data Select
#define B_PCH_EHCI_PWR_CNTL_STS_PME_EN      BIT8  // Power Enable
#define B_PCH_EHCI_PWR_CNTL_STS_PWR_STS     (BIT1 | BIT0) // Power State
#define V_PCH_EHCI_PWR_CNTL_STS_PWR_STS_D0  0     // D0 State
#define V_PCH_EHCI_PWR_CNTL_STS_PWR_STS_D3  (BIT1 | BIT0) // D3 Hot State

///
/// USB3 (XHCI) related definitions
///
#define PCI_DEVICE_NUMBER_PCH_XHCI          20
#define PCI_FUNCTION_NUMBER_PCH_XHCI        0
//
/////
///// XHCI PCI Config Space registers
/////

#define R_PCH_XHCI_SVID                     0x2C
#define B_PCH_XHCI_SVID                     0xFFFF


#define R_PCH_XHCI_PWR_CNTL_STS             0x74
#define B_PCH_XHCI_PWR_CNTL_STS_PME_STS     BIT15
#define B_PCH_XHCI_PWR_CNTL_STS_DATASCL     (BIT14 | BIT13)
#define B_PCH_XHCI_PWR_CNTL_STS_DATASEL     (BIT12 | BIT11 | BIT10 | BIT9)
#define B_PCH_XHCI_PWR_CNTL_STS_PME_EN      BIT8
#define B_PCH_XHCI_PWR_CNTL_STS_PWR_STS     (BIT1 | BIT0)
#define V_PCH_XHCI_PWR_CNTL_STS_PWR_STS_D3  (BIT1 | BIT0)

#endif
