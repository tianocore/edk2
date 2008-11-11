/** @file
  Include file of PciSegmentPciRootBridgeIo Library.

  Copyright (c) 2007 - 2008, Intel Corporation All rights
  reserved. This program and the accompanying materials are
  licensed and made available under the terms and conditions of
  the BSD License which accompanies this distribution.  The full
  text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DXE_PCI_SEGMENT_LIB__
#define __DXE_PCI_SEGMENT_LIB__

#include <PiDxe.h>

#include <Protocol/PciRootBridgeIo.h>

#include <Library/PciSegmentLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#include <IndustryStandard/Acpi.h>

typedef struct {
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL    *PciRootBridgeIo;
  UINT32                             SegmentNumber;
  UINT64                             MinBusNumber;
  UINT64                             MaxBusNumber;
} PCI_ROOT_BRIDGE_DATA;

/**
  Assert the validity of a PCI Segment address.
  A valid PCI address should not contain 1's in bits 31:28

  @param  A The address to validate.
  @param  M Additional bits to assert to be zero.

**/
#define ASSERT_INVALID_PCI_SEGMENT_ADDRESS(A,M) \
  ASSERT (((A) & (0xf0000000 | (M))) == 0)

/**
  Translate PCI Lib address into format of PCI CFG2 PPI.

  @param  A  Address that encodes the PCI Bus, Device, Function and
             Register.

**/
#define PCI_TO_PCICFG2_ADDRESS(A) \
  (((A) << 4) & 0xff000000) | (((A) >> 4) & 0x00000700) | (((A) << 1) & 0x001f0000) | ((UINT64)((A) & 0xFFF) << 32)

#endif
