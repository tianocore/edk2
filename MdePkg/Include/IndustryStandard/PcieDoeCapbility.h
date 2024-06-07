/** @file
  PCIe DOE Capability structure definitions

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCIE_DOE_CAPBILITY_H_
#define PCIE_DOE_CAPBILITY_H_

#include <IndustryStandard/PciExpress21.h>

//
// The Data Object Exchange PCI Express Extended Capability definitions.
// Based on section x.x.x of PCI Express Base Specification x.x.
//
#define PCI_EXPRESS_EXTENDED_CAPABILITY_DOE_ID    0x002E
#define PCI_EXPRESS_EXTENDED_CAPABILITY_DOE_VER1  0x1

//
// Register offsets from Data Object Exchange PCIe Ext Cap Header
//
#define PCI_EXPRESS_REG_DOE_CAPABILITIES_OFFSET        0x04
#define PCI_EXPRESS_REG_DOE_CONTROL_OFFSET             0x08
#define PCI_EXPRESS_REG_DOE_STATUS_OFFSET              0x0C
#define PCI_EXPRESS_REG_DOE_WRITE_DATA_MAILBOX_OFFSET  0x10
#define PCI_EXPRESS_REG_DOE_READ_DATA_MAILBOX_OFFSET   0x14

#pragma pack(1)

typedef union {
  struct {
    UINT32    InterruptSupport          : 1;      // bit 0
    UINT32    DoeInterruptMessageNumber : 11;     // bit 1:11
    UINT32    Reserved                  : 20;     // Reserved bit 12:31
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_DOE_CAPABILITIES;

typedef union {
  struct {
    UINT32    DoeAbort           : 1;             // bit 0
    UINT32    DoeInterruptEnable : 1;             // bit 1
    UINT32    Reserved           : 29;            // Reserved bit 2:30
    UINT32    DoeGo              : 1;             // bit 31
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_DOE_CONTROL;

typedef union {
  struct {
    UINT32    DoeBusy            : 1;             // bit 0
    UINT32    DoeInterruptStatus : 1;             // bit 1
    UINT32    DoeError           : 1;             // bit 2
    UINT32    Reserved           : 28;            // Reserved bit 3:30
    UINT32    DataObjectReady    : 1;             // bit 31
  } Bits;
  UINT32    Uint32;
} PCI_EXPRESS_REG_DOE_STATUS;

typedef struct {
  PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER    Header;
  PCI_EXPRESS_REG_DOE_CAPABILITIES            Capability;
  PCI_EXPRESS_REG_DOE_CONTROL                 Control;
  PCI_EXPRESS_REG_DOE_STATUS                  Status;
  UINT32                                      DoeWriteDataMailbox;
  UINT32                                      DoeReadDataMailbox;
} PCI_EXPRESS_EXTENDED_CAPABILITIES_DOE;

typedef union {
  struct {
    UINT32    Header1;
    UINT32    Header2;
    UINT32    Dw0;
  } Packet;
  UINT32    Uint32[3];
} PCI_EXPRESS_DOE_DISCOVERY_PACKET;

#pragma pack()

#endif
