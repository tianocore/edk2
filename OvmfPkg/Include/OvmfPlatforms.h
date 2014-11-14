/** @file
  OVMF Platform definitions

  Copyright (c) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  This program and the accompanying materials are licensed and made
  available under the terms and conditions of the BSD License which
  accompanies this distribution.   The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __OVMF_PLATFORMS_H__
#define __OVMF_PLATFORMS_H__

#include <Library/PciLib.h>
#include <IndustryStandard/Pci22.h>

//
// Host Bridge Device ID (DID) values for PIIX4 and Q35/MCH
//
#define INTEL_82441_DEVICE_ID     0x1237  // PIIX4
#define INTEL_Q35_MCH_DEVICE_ID   0x29C0  // Q35

//
// OVMF Host Bridge DID Address
//
#define OVMF_HOSTBRIDGE_DID \
  PCI_LIB_ADDRESS (0, 0, 0, PCI_DEVICE_ID_OFFSET)

//
// Power Management Device and Function numbers for PIIX4 and Q35/MCH
//
#define OVMF_PM_DEVICE_PIIX4  0x01
#define OVMF_PM_FUNC_PIIX4    0x03
#define OVMF_PM_DEVICE_Q35    0x1f
#define OVMF_PM_FUNC_Q35      0x00

//
// Power Management Register access for PIIX4 and Q35/MCH
//
#define POWER_MGMT_REGISTER_PIIX4(Offset) \
  PCI_LIB_ADDRESS (0, OVMF_PM_DEVICE_PIIX4, OVMF_PM_FUNC_PIIX4, (Offset))
#define POWER_MGMT_REGISTER_Q35(Offset) \
  PCI_LIB_ADDRESS (0, OVMF_PM_DEVICE_Q35, OVMF_PM_FUNC_Q35, (Offset))

#endif
