/** @file
  Support for the PCI Express 3.0 standard.

  This header file may not define all structures.  Please extend as required.

  Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PCIEXPRESS30_H_
#define _PCIEXPRESS30_H_

#include <IndustryStandard/PciExpress21.h>

#pragma pack(1)

#define PCI_EXPRESS_EXTENDED_CAPABILITY_SECONDARY_PCIE_ID    0x0019
#define PCI_EXPRESS_EXTENDED_CAPABILITY_SECONDARY_PCIE_VER1  0x1

typedef union {
  struct {
    UINT32    PerformEqualization                    : 1;
    UINT32    LinkEqualizationRequestInterruptEnable : 1;
    UINT32    Reserved                               : 30;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_LINK_CONTROL3;

typedef union {
  struct {
    UINT16    DownstreamPortTransmitterPreset  : 4;
    UINT16    DownstreamPortReceiverPresetHint : 3;
    UINT16    Reserved                         : 1;
    UINT16    UpstreamPortTransmitterPreset    : 4;
    UINT16    UpstreamPortReceiverPresetHint   : 3;
    UINT16    Reserved2                        : 1;
  } Bits;
  UINT16    Uint16;
} PCI_EXPRESS_REG_LANE_EQUALIZATION_CONTROL;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER     Header;
  PCI_EXPRESS_REG_LINK_CONTROL3                LinkControl3;
  UINT32                                       LaneErrorStatus;
  PCI_EXPRESS_REG_LANE_EQUALIZATION_CONTROL    EqualizationControl[2];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_SECONDARY_PCIE;

/// VF Resizable BAR Extended Capability Structure
///
/// Based on section 7.22 of PCI Express Base Specification 3.0
///@{
#define PCI_EXPRESS_EXTENDED_CAPABILITY_VF_RESIZABLE_BAR_ID    0x0024
#define PCI_EXPRESS_EXTENDED_CAPABILITY_VF_RESIZABLE_BAR_VER1  0x1

typedef union {
  struct {
    UINT32    Reserved            : 4;
    UINT32    VfBarSizeCapability : 28;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_VF_RESIZABLE_BAR_CAPABILITY;

typedef union {
  struct {
    UINT32    VfBarIndex           : 3;
    UINT32    Reserved             : 2;
    UINT32    VfResizableBarNumber : 3;
    UINT32    VfBarSize            : 6;
    UINT32    Reserved2            : 2;
    UINT32    VfBarSizeCapability  : 16;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_VF_RESIZABLE_BAR_CONTROL;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_VF_RESIZABLE_BAR_CAPABILITY    VfResizableBarCapability;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_VF_RESIZABLE_BAR_CONTROL       VfResizableBarControl;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_VF_RESIZABLE_BAR_ENTRY;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER                    Header;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_VF_RESIZABLE_BAR_ENTRY    Capability[1];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_VF_RESIZABLE_BAR;
///@}

#pragma pack()

#endif
