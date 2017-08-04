/** @file
  Provide common routines used by BasePciSegmentLibSegmentInfo and
  DxeRuntimePciSegmentLibSegmentInfo libraries.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PCI_SEGMENT_LIB_COMMON_H_
#define _PCI_SEGMENT_LIB_COMMON_H_

#include <Base.h>
#include <IndustryStandard/PciExpress21.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/PciSegmentInfoLib.h>

/**
  Return the linear address for the physical address.

  @param  Address  The physical address.

  @retval The linear address.
**/
UINTN
PciSegmentLibVirtualAddress (
  IN UINTN                     Address
  );

/**
  Internal function that converts PciSegmentLib format address that encodes the PCI Bus, Device,
  Function and Register to ECAM (Enhanced Configuration Access Mechanism) address.

  @param Address     The address that encodes the PCI Bus, Device, Function and
                     Register.
  @param SegmentInfo An array of PCI_SEGMENT_INFO holding the segment information.
  @param Count       Number of segments.

  @retval ECAM address.
**/
UINTN
PciSegmentLibGetEcamAddress (
  IN UINT64                    Address,
  IN CONST PCI_SEGMENT_INFO    *SegmentInfo,
  IN UINTN                     Count
  );

#endif
