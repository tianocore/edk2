/** @file
Support for the PCI Express 4.0 standard.

This header file may not define all structures.  Please extend as required.

Copyright (c) 2018, American Megatrends, Inc. All rights reserved.<BR>
Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PCIEXPRESS40_H_
#define _PCIEXPRESS40_H_

#include <IndustryStandard/PciExpress31.h>

#pragma pack(1)

/// Precision Time Management Extended Capability definitions.
///
/// Based on section 7.9.16 of PCI Express Base Specification 4.0.
///@{
#define PCI_EXPRESS_EXTENDED_CAPABILITY_PTM_ID    0x001F
#define PCI_EXPRESS_EXTENDED_CAPABILITY_PTM_VER1  0x1

#define PCI_EXPRESS_EXTENDED_CAPABILITIES_PTM_CAPABILITY_OFFSET  0x04
#define PCI_EXPRESS_EXTENDED_CAPABILITIES_PTM_CONTROL_OFFSET     0x08

typedef union {
  struct {
    UINT32    PTMRequesterCapable                  : 1;
    UINT32    PTMResponderCapable                  : 1;
    UINT32    PTMRootCapable                       : 1;
    UINT32    ePTMCapable                          : 1;
    UINT32    PTMPropagationDelayAdaptationCapable : 1;
    UINT32    Reserved                             : 3;
    UINT32    LocalClockGranularity                : 8;
    UINT32    Reserved2                            : 16;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PTM_CAPABILITY;

typedef union {
  struct {
    UINT32    PTMEnable            : 1;
    UINT32    RootSelect           : 1;
    UINT32    Reserved             : 6;
    UINT32    EffectiveGranularity : 8;
    UINT32    Reserved2            : 16;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PTM_CONTROL;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER            Header;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_PTM_CAPABILITY    Capability;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_PTM_CONTROL       Control;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PTM;
///@}

/// The Physical Layer PCI Express Extended Capability definitions.
///
/// Based on section 7.7.5 of PCI Express Base Specification 4.0.
///@{
#define PCI_EXPRESS_EXTENDED_CAPABILITY_PHYSICAL_LAYER_16_0_ID    0x0026
#define PCI_EXPRESS_EXTENDED_CAPABILITY_PHYSICAL_LAYER_16_0_VER1  0x1

// Register offsets from Physical Layer PCI-E Ext Cap Header
#define PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_CAPABILITIES_OFFSET                       0x04
#define PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_CONTROL_OFFSET                            0x08
#define PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_STATUS_OFFSET                             0x0C
#define PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_LOCAL_DATA_PARITY_STATUS_OFFSET           0x10
#define PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_FIRST_RETIMER_DATA_PARITY_STATUS_OFFSET   0x14
#define PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_SECOND_RETIMER_DATA_PARITY_STATUS_OFFSET  0x18
#define PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_LANE_EQUALIZATION_CONTROL_OFFSET          0x20

typedef union {
  struct {
    UINT32    Reserved : 32;               // Reserved bit 0:31
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_CAPABILITIES;

typedef union {
  struct {
    UINT32    Reserved : 32;               // Reserved bit 0:31
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_CONTROL;

typedef union {
  struct {
    UINT32    EqualizationComplete      : 1;  // bit 0
    UINT32    EqualizationPhase1Success : 1;  // bit 1
    UINT32    EqualizationPhase2Success : 1;  // bit 2
    UINT32    EqualizationPhase3Success : 1;  // bit 3
    UINT32    LinkEqualizationRequest   : 1;  // bit 4
    UINT32    Reserved                  : 27; // Reserved bit 5:31
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_STATUS;

typedef union {
  struct {
    UINT8    DownstreamPortTransmitterPreset : 4; // bit 0..3
    UINT8    UpstreamPortTransmitterPreset   : 4; // bit 4..7
  } Bits;
  UINT8    Uint8;
} PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_LANE_EQUALIZATION_CONTROL;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER                         Header;
  PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_CAPABILITIES                 Capablities;
  PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_CONTROL                      Control;
  PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_STATUS                       Status;
  UINT32                                                           LocalDataParityMismatchStatus;
  UINT32                                                           FirstRetimerDataParityMismatchStatus;
  UINT32                                                           SecondRetimerDataParityMismatchStatus;
  UINT32                                                           Reserved;
  PCI_EXPRESS_REG_PHYSICAL_LAYER_16_0_LANE_EQUALIZATION_CONTROL    LaneEqualizationControl[1];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PHYSICAL_LAYER_16_0;
///@}

/// The Designated Vendor Specific Capability definitions
///
/// Based on section 7.9.6 of PCI Express Base Specification 4.0.
///@{
#define PCI_EXPRESS_EXTENDED_CAPABILITY_DESIGNATED_VENDOR_SPECIFIC_ID    0x0023
#define PCI_EXPRESS_EXTENDED_CAPABILITY_DESIGNATED_VENDOR_SPECIFIC_VER1  0x1

typedef union {
  struct {
    UINT32    DvsecVendorId : 16;                                     // bit 0..15
    UINT32    DvsecRevision : 4;                                      // bit 16..19
    UINT32    DvsecLength   : 12;                                     // bit 20..31
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_1;

typedef union {
  struct {
    UINT16    DvsecId : 16;                                           // bit 0..15
  } Bits;
  UINT16    Uint16;
} PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_2;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER           Header;
  PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_1    DesignatedVendorSpecificHeader1;
  PCI_EXPRESS_DESIGNATED_VENDOR_SPECIFIC_HEADER_2    DesignatedVendorSpecificHeader2;
  UINT8                                              DesignatedVendorSpecific[1];
} PCI_EXPRESS_EXTENDED_CAPABILITIES_DESIGNATED_VENDOR_SPECIFIC;
///@}

/// Data Link Feature Extended Capability Structure
///
/// Based on section 7.7.4 of PCI Express Base Specification 4.0
///@{
#define PCI_EXPRESS_EXTENDED_CAPABILITY_DATA_LINK_FEATURE_ID    0x0025
#define PCI_EXPRESS_EXTENDED_CAPABILITY_DATA_LINK_FEATURE_VER1  0x1

typedef union {
  struct {
    UINT32    Reserved1                : 1;
    UINT32    ScrambleDisableSupported : 1;
    UINT32    Reserved2                : 30;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_DATA_LINK_FEATURE_CAPABILITY;

typedef union {
  struct {
    UINT32    Reserved1       : 1;
    UINT32    ScrambleDisable : 1;
    UINT32    Reserved2       : 30;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_DATA_LINK_FEATURE_CONTROL;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER                          Header;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_DATA_LINK_FEATURE_CAPABILITY    Capability;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_DATA_LINK_FEATURE_CONTROL       Control;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_DATA_LINK_FEATURE;
///@}

/// Lane Margining at Receiver Extended Capability Structure
///
/// Based on section 7.7.6 of PCI Express Base Specification 4.0
///@{
#define PCI_EXPRESS_EXTENDED_CAPABILITY_LANE_MARGINING_AT_RECEIVER_ID    0x0027
#define PCI_EXPRESS_EXTENDED_CAPABILITY_LANE_MARGINING_AT_RECEIVER_VER1  0x1

typedef union {
  struct {
    UINT8    MaxLaneNumber : 5;
    UINT8    Reserved      : 3;
  } Bits;
  UINT8    Uint8;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_LANE_MARGINING_CAPABILITY;

typedef union {
  struct {
    UINT8    LaneNumber            : 5;
    UINT8    RcvErrorCounterSelect : 2;
    UINT8    LaneMarginStepSelect  : 1;
  } Bits;
  UINT8    Uint8;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_LANE_MARGINING_CONTROL;

typedef union {
  struct {
    UINT8    MaxLanesReceivingTestPattern : 5;
    UINT8    Reserved                     : 3;
  } Bits;
  UINT8    Uint8;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_LANE_MARGINING_STATUS;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER                       Header;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_LANE_MARGINING_CAPABILITY    Capability;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_LANE_MARGINING_CONTROL       Control;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_LANE_MARGINING_STATUS        Status;
  UINT32                                                         ErrorCounter;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_LANE_MARGINING_AT_RECEIVER;
///@}

/// Page Request Interface Extended Capability Structure
///
/// Based on section 10.5.2 of PCI Express Base Specification 4.0
///@{
#define PCI_EXPRESS_EXTENDED_CAPABILITY_PRI_ID    0x0013
#define PCI_EXPRESS_EXTENDED_CAPABILITY_PRI_VER1  0x1

typedef union {
  struct {
    UINT32    PriRequestCapable     : 1;
    UINT32    PriCompletionCapable  : 1;
    UINT32    Page256RequestCapable : 1;
    UINT32    Page512RequestCapable : 1;
    UINT32    Page1KRequestCapable  : 1;
    UINT32    Page2KRequestCapable  : 1;
    UINT32    Page4KRequestCapable  : 1;
    UINT32    Page8KRequestCapable  : 1;
    UINT32    Reserved              : 24;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PRI_CAPABILITY;

typedef union {
  struct {
    UINT32    PriEnable : 1;
    UINT32    PriReset  : 1;
    UINT32    Reserved  : 30;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PRI_CONTROL;

typedef union {
  struct {
    UINT32    OutstandingPageRequest : 1;
    UINT32    ResponseFailure        : 1;
    UINT32    Stopped                : 1;
    UINT32    PpRqIdParity           : 1;
    UINT32    Reserved               : 28;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PRI_STATUS;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER            Header;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_PRI_CAPABILITY    Capability;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_PRI_CONTROL       Control;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_PRI_STATUS        Status;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PRI;
///@}

#pragma pack()

#endif
