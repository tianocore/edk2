/** @file
Support for the PCI Express 7.0 standard.

This header file may not define all structures. Please extend as required.

Copyright (c) 2025, American Megatrends International LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCIEXPRESS70_H_
#define PCIEXPRESS70_H_

#include <IndustryStandard/PciExpress60.h>

/// The Physical Layer PCI Express Extended Capability definitions.
///
/// Based on section 7.7.8 of PCI Express Base Specification 7.0
///@{
#define PCI_EXPRESS_EXTENDED_CAPABILITY_PHYSICAL_LAYER_128_0_ID    0x0039
#define PCI_EXPRESS_EXTENDED_CAPABILITY_PHYSICAL_LAYER_128_0_VER1  0x1

// Register offsets from Physical Layer PCI-E Ext Cap Header
#define PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_CAPABILITIES_OFFSET               0x04
#define PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_CONTROL_OFFSET                    0x08
#define PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_STATUS_OFFSET                     0x0C
#define PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_LANE_EQUALIZATION_CONTROL_OFFSET  0x10

#pragma pack(1)

typedef union {
  struct {
    UINT32    SupportedLinkSpeedsVector2  : 8;  // bits 0..7
    UINT32    LowerSkpOsGenLnkSpeedsVect2 : 8;  // bits 8..15
    UINT32    LowerSkpOsRecLnkSpeedsVect2 : 8;  // bits 16..23
    UINT32    Reserved                    : 8;  // bits 24..31
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_CAPABILITIES;

typedef union {
  struct {
    UINT32    Reserved : 32;  // Reserved bit 0:31
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_CONTROL;

typedef union {
  struct {
    UINT32    EqualizationComplete      : 1;  // bit 0
    UINT32    EqualizationPhase1Success : 1;  // bit 1
    UINT32    EqualizationPhase2Success : 1;  // bit 2
    UINT32    EqualizationPhase3Success : 1;  // bit 3
    UINT32    LinkEqualizationRequest   : 1;  // bit 4
    UINT32    TransmitterPrecodingOn    : 1;  // bit 5
    UINT32    TransmitterPrecodeRequest : 1;  // bit 6
    UINT32    NoEqualizationNeededRcvd  : 1;  // bit 7
    UINT32    Reserved                  : 24; // Reserved bit 8:31
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_STATUS;

typedef union {
  struct {
    UINT8    DownstreamPortTransmitterPreset : 4;  // bit 0..3
    UINT8    UpstreamPortTransmitterPreset   : 4;  // bit 4..7
  } Bits;
  UINT8    Uint8;
} PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_LANE_EQUALIZATION_CONTROL;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER                          Header;
  PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_CAPABILITIES                 Capablities;
  PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_CONTROL                      Control;
  PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_STATUS                       Status;
  PCI_EXPRESS_REG_PHYSICAL_LAYER_128_0_LANE_EQUALIZATION_CONTROL    LaneEqualizationControl[1];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PHYSICAL_LAYER_128_0;
///@}

#pragma pack()

#endif
