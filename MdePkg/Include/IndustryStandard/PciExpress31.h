/** @file
Support for the PCI Express 3.1 standard.

This header file may not define all structures.  Please extend as required.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PCIEXPRESS31_H_
#define _PCIEXPRESS31_H_

#include <IndustryStandard/PciExpress30.h>

#pragma pack(1)

//
// Downstream Port Containment (DPC) Extended Capability.
//
#define PCI_EXPRESS_EXTENDED_CAPABILITY_DPC_ID  0x001D

typedef union {
  struct {
    UINT16    DpcInterruptMsgNo             : 5;    // [4:0]
    UINT16    RpExtensionsForDpc            : 1;    // [5]
    UINT16    PoisonedTlpEgressBlockingSupp : 1;    // [6]
    UINT16    DpcSoftwareTriggerSupp        : 1;    // [7]
    UINT16    RpPioLogSize                  : 4;    // [11:8] Bits [3:0] of log size
    UINT16    DlActiveErrCorSignalingSupp   : 1;    // [12]
    UINT16    RpPioLogSizeExt               : 1;    // [13]   Bit [4] of log size
    UINT16    Reserved                      : 2;    // [15:14]
  } Bits;
  UINT32    Uint16;
} PCI_EXPRESS_REG_DPC_CAPABILITY;

typedef union {
  struct {
    UINT16    DpcTriggerEn                : 2;    // [1:0]
    UINT16    DpcCompletionCtl            : 1;    // [2]
    UINT16    DpcInterruptEn              : 1;    // [3]
    UINT16    DpcErrCorEn                 : 1;    // [4]
    UINT16    PoisonedTlpEgressBlockingEn : 1;    // [5]
    UINT16    DpcSoftwareTrigger          : 1;    // [6]
    UINT16    DlActiveErrCorEn            : 1;    // [7]
    UINT16    DpcSigSfwEn                 : 1;    // [8]
    UINT16    Reserved                    : 7;    // [15:9]
  } Bits;
  UINT16    Uint16;
} PCI_EXPRESS_REG_DPC_CONTROL;

typedef union {
  struct {
    UINT16    DpcTriggerStatus          : 1;    // [0]
    UINT16    DpcTriggerReason          : 2;    // [2:1]
    UINT16    DpcInterruptStatus        : 1;    // [3]
    UINT16    DpcRpBusy                 : 1;    // [4]
    UINT16    DpcTriggerReasonExtension : 2;    // [6:5]
    UINT16    Reserved0                 : 1;    // [7]
    UINT16    RpPioFirstErrorPointer    : 5;    // [12:8]
    UINT16    DpcSigSfwStatus           : 1;    // [13]
    UINT16    Reserved1                 : 2;    // [15:14]
  } Bits;
  UINT16    Uint16;
} PCI_EXPRESS_REG_DPC_STATUS;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER    Header;
  PCI_EXPRESS_REG_DPC_CAPABILITY              Capability;         // Offset 04h size 2
  PCI_EXPRESS_REG_DPC_CONTROL                 Control;            // Offset 06h size 2
  PCI_EXPRESS_REG_DPC_STATUS                  Status;             // Offset 08h size 2
  UINT16                                      ErrSourceId;        // Offset 0Ah size 2
  UINT32                                      RpPioStatus;        // Offset 0Ch size 4
  UINT32                                      RpPioMask;          // Offset 10h size 4
  UINT32                                      RpPioSeverity;      // Offset 14h size 4
  UINT32                                      RpPioSysErr;        // Offset 18h size 4
  UINT32                                      RpPioException;     // Offset 1Ch size 4
  UINT32                                      RpPioHdrLog[4];     // Offset 20h size 16 header log DW 1-4
  UINT32                                      RpPioImpSpecLog;    // Offset 30h size 4
  UINT32                                      RpPioHdrLogExt[10]; // Offset 34h size 40 header log DW 5-14
} PCI_EXPRESS_EXTENDED_CAPABILITIES_DPC;

//
// L1 PM Substates Extended Capability.
//
#define PCI_EXPRESS_EXTENDED_CAPABILITY_L1_PM_SUBSTATES_ID    0x001E
#define PCI_EXPRESS_EXTENDED_CAPABILITY_L1_PM_SUBSTATES_VER1  0x1

typedef union {
  struct {
    UINT32    PciPmL12              : 1;
    UINT32    PciPmL11              : 1;
    UINT32    AspmL12               : 1;
    UINT32    AspmL11               : 1;
    UINT32    L1PmSubstates         : 1;
    UINT32    Reserved              : 3;
    UINT32    CommonModeRestoreTime : 8;
    UINT32    TPowerOnScale         : 2;
    UINT32    Reserved2             : 1;
    UINT32    TPowerOnValue         : 5;
    UINT32    Reserved3             : 8;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_L1_PM_SUBSTATES_CAPABILITY;

typedef union {
  struct {
    UINT32    PciPmL12              : 1;
    UINT32    PciPmL11              : 1;
    UINT32    AspmL12               : 1;
    UINT32    AspmL11               : 1;
    UINT32    Reserved              : 4;
    UINT32    CommonModeRestoreTime : 8;
    UINT32    LtrL12ThresholdValue  : 10;
    UINT32    Reserved2             : 3;
    UINT32    LtrL12ThresholdScale  : 3;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_L1_PM_SUBSTATES_CONTROL1;

typedef union {
  struct {
    UINT32    TPowerOnScale : 2;
    UINT32    Reserved      : 1;
    UINT32    TPowerOnValue : 5;
    UINT32    Reserved2     : 24;
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_L1_PM_SUBSTATES_CONTROL2;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER      Header;
  PCI_EXPRESS_REG_L1_PM_SUBSTATES_CAPABILITY    Capability;
  PCI_EXPRESS_REG_L1_PM_SUBSTATES_CONTROL1      Control1;
  PCI_EXPRESS_REG_L1_PM_SUBSTATES_CONTROL2      Control2;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_L1_PM_SUBSTATES;

//
// Process Address Space ID (PASID) Extended Capability Structure.
//
#define PCI_EXPRESS_EXTENDED_CAPABILITY_PASID_ID    0x001B
#define PCI_EXPRESS_EXTENDED_CAPABILITY_PASID_VER1  0x1

typedef union {
  struct {
    UINT16    Reserved0                     : 1;    // [0]
    UINT16    ExecutePermissionSupport      : 1;    // [1]
    UINT16    PrivilegedModeSupport         : 1;    // [2]
    UINT16    TranslatedReqWithPasidSupport : 1;    // [3]
    UINT16    Reserved1                     : 4;    // [7:4]
    UINT16    MaxPasidWidth                 : 5;    // [12:8]
    UINT16    Reserved2                     : 3;    // [15:13]
  } Bits;
  UINT16    Uint16;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PASID_CAPABILITY;

typedef union {
  struct {
    UINT16    PasidEnable                  : 1;    // [0]
    UINT16    ExecutePermissionEnable      : 1;    // [1]
    UINT16    PrivilegedModeEnable         : 1;    // [2]
    UINT16    TranslatedReqWithPasidEnable : 1;    // [3]
    UINT16    Reserved                     : 12;
  } Bits;
  UINT16    Uint16;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PASID_CONTROL;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER              Header;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_PASID_CAPABILITY    Capability;
  PCI_EXPRESS_EXTENDED_CAPABILITIES_PASID_CONTROL       Control;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_PASID;

#pragma pack()

#endif
