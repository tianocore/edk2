/** @file
  Various register numbers and value bits based on the following publications:
  - Intel(R) datasheet 290549-001
  - Intel(R) datasheet 290562-001
  - Intel(R) datasheet 297654-006
  - Intel(R) datasheet 297738-017

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.   The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __I440FX_PIIX4_H__
#define __I440FX_PIIX4_H__

#include <Library/PciLib.h>

//
// Host Bridge Device ID (DID) value for I440FX
//
#define INTEL_82441_DEVICE_ID 0x1237

//
// B/D/F/Type: 0/1/3/PCI
//
#define POWER_MGMT_REGISTER_PIIX4(Offset) PCI_LIB_ADDRESS (0, 1, 3, (Offset))

#define PIIX4_PMBA             0x40

#define PIIX4_PMREGMISC        0x80
#define PIIX4_PMREGMISC_PMIOSE   BIT0

#endif
