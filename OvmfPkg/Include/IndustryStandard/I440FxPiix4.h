/** @file
  Various register numbers and value bits based on the following publications:
  - Intel(R) datasheet 290549-001
  - Intel(R) datasheet 290562-001
  - Intel(R) datasheet 297654-006
  - Intel(R) datasheet 297738-017

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __I440FX_PIIX4_H__
#define __I440FX_PIIX4_H__

#include <Library/PciLib.h>

//
// Host Bridge Device ID (DID) value for I440FX
//
#define INTEL_82441_DEVICE_ID 0x1237

//
// B/D/F/Type: 0/0/0/PCI
//
#define PMC_REGISTER_PIIX4(Offset) PCI_LIB_ADDRESS (0, 0, 0, (Offset))

#define PIIX4_PAM0              0x59
#define PIIX4_PAM1              0x5A
#define PIIX4_PAM2              0x5B
#define PIIX4_PAM3              0x5C
#define PIIX4_PAM4              0x5D
#define PIIX4_PAM5              0x5E
#define PIIX4_PAM6              0x5F

//
// B/D/F/Type: 0/1/3/PCI
//
#define POWER_MGMT_REGISTER_PIIX4(Offset) PCI_LIB_ADDRESS (0, 1, 3, (Offset))

#define PIIX4_PMBA             0x40
#define PIIX4_PMBA_MASK          (BIT15 | BIT14 | BIT13 | BIT12 | BIT11 | \
                                  BIT10 | BIT9  | BIT8  | BIT7  | BIT6)

#define PIIX4_PMREGMISC        0x80
#define PIIX4_PMREGMISC_PMIOSE   BIT0

//
// IO ports
//
#define PIIX4_CPU_HOTPLUG_BASE 0xAF00

#endif
