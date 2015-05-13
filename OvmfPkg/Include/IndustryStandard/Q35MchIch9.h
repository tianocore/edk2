/** @file
  Various register numbers and value bits based on the following publications:
  - Intel(R) datasheet 316966-002
  - Intel(R) datasheet 316972-004

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.   The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __Q35_MCH_ICH9_H__
#define __Q35_MCH_ICH9_H__

#include <Library/PciLib.h>

//
// Host Bridge Device ID (DID) value for Q35/MCH
//
#define INTEL_Q35_MCH_DEVICE_ID 0x29C0

//
// B/D/F/Type: 0/0x1f/0/PCI
//
#define POWER_MGMT_REGISTER_Q35(Offset) \
  PCI_LIB_ADDRESS (0, 0x1f, 0, (Offset))

#endif
