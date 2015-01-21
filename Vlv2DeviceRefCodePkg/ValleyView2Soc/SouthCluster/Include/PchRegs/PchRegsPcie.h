/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



  @file
  PchRegsPcie.h

  @brief
  Register names for VLV PCI-E root port devices

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
#ifndef _PCH_REGS_PCIE_H_
#define _PCH_REGS_PCIE_H_

#define PCH_PCIE_MAX_ROOT_PORTS                            4

///
/// VLV PCI Express Root Ports (D28:F0~F3)
///
#define PCI_DEVICE_NUMBER_PCH_PCIE_ROOT_PORTS              28
#define PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_1           0
#define PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_2           1
#define PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_3           2
#define PCI_FUNCTION_NUMBER_PCH_PCIE_ROOT_PORT_4           3

#define R_PCH_PCIE_ID                                      0x00  // Identifiers
#define B_PCH_PCIE_ID_DID                                  0xFFFF0000 // Device ID
#define V_PCH_PCIE_DEVICE_ID_0                             0x0F48  // PCIE Root Port #1
#define V_PCH_PCIE_DEVICE_ID_1                             0x0F4A  // PCIE Root Port #2
#define V_PCH_PCIE_DEVICE_ID_2                             0x0F4C  // PCIE Root Port #3
#define V_PCH_PCIE_DEVICE_ID_3                             0x0F4E  // PCIE Root Port #4
#define B_PCH_PCIE_ID_VID                                  0x0000FFFF // Vendor ID
#define V_PCH_PCIE_VENDOR_ID                               V_PCH_INTEL_VENDOR_ID


#define R_PCH_PCIE_BNUM_SLT                                0x18  // Bus Numbers; Secondary Latency Timer
#define B_PCH_PCIE_BNUM_SLT_SLT                            0xFF000000 // Secondary Latency Timer
#define B_PCH_PCIE_BNUM_SLT_SBBN                           0x00FF0000 // Subordinate Bus Number
#define B_PCH_PCIE_BNUM_SLT_SCBN                           0x0000FF00 // Secondary Bus Number
#define B_PCH_PCIE_BNUM_SLT_PBN                            0x000000FF // Primary Bus Number
#define R_PCH_PCIE_CAPP                                    0x34  // Capabilities List Pointer
#define B_PCH_PCIE_CAPP                                    0xFF  // Capabilities Pointer

#define R_PCH_PCIE_SLCTL_SLSTS                             0x58  // Slot Control; Slot Status
#define S_PCH_PCIE_SLCTL_SLSTS                             4
#define B_PCH_PCIE_SLCTL_SLSTS_DLLSC                       BIT24 // Data Link Layer State Changed
#define B_PCH_PCIE_SLCTL_SLSTS_PDS                         BIT22 // Presence Detect State
#define B_PCH_PCIE_SLCTL_SLSTS_MS                          BIT21 // MRL Sensor State
#define B_PCH_PCIE_SLCTL_SLSTS_PDC                         BIT19 // Presence Detect Changed
#define B_PCH_PCIE_SLCTL_SLSTS_MSC                         BIT18 // MRL Sensor Changed
#define B_PCH_PCIE_SLCTL_SLSTS_PFD                         BIT17 // Power Fault Detected
#define B_PCH_PCIE_SLCTL_SLSTS_DLLSCE                      BIT12 // Data Link Layer State Changed Enable
#define B_PCH_PCIE_SLCTL_SLSTS_PCC                         BIT10 // Power Controller Control
#define B_PCH_PCIE_SLCTL_SLSTS_HPE                         BIT5  // Hot Plug Interrupt Enable
#define B_PCH_PCIE_SLCTL_SLSTS_CCE                         BIT4  // Command Completed Interrupt Enable
#define B_PCH_PCIE_SLCTL_SLSTS_PDE                         BIT3  // Presence Detect Changed Enable

#define R_PCH_PCIE_SVID                                    0x94  // Subsystem Vendor IDs
#define S_PCH_PCIE_SVID                                    4
#define B_PCH_PCIE_SVID_SID                                0xFFFF0000 // Subsystem Identifier
#define B_PCH_PCIE_SVID_SVID                               0x0000FFFF // Subsystem Vendor Identifier

#endif
