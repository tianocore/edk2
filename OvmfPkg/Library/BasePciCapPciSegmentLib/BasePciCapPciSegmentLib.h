/** @file
  Plug a PciSegmentLib backend into PciCapLib, for config space access --
  internal macro and type definitions.

  Copyright (C) 2018, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __BASE_PCI_CAP_PCI_SEGMENT_LIB_H__
#define __BASE_PCI_CAP_PCI_SEGMENT_LIB_H__

#include <Library/DebugLib.h>

#include <Library/PciCapPciSegmentLib.h>

#define SEGMENT_DEV_SIG SIGNATURE_64 ('P', 'C', 'P', 'S', 'G', 'M', 'N', 'T')

typedef struct {
  //
  // Signature identifying the derived class.
  //
  UINT64 Signature;
  //
  // Members added by the derived class, specific to the use of PciSegmentLib.
  //
  PCI_CAP_DOMAIN MaxDomain;
  UINT16         SegmentNr;
  UINT8          BusNr;
  UINT8          DeviceNr;
  UINT8          FunctionNr;
  //
  // Base class.
  //
  PCI_CAP_DEV BaseDevice;
} SEGMENT_DEV;

#define SEGMENT_DEV_FROM_PCI_CAP_DEV(PciDevice) \
  CR (PciDevice, SEGMENT_DEV, BaseDevice, SEGMENT_DEV_SIG)

#endif // __BASE_PCI_CAP_PCI_SEGMENT_LIB_H__
