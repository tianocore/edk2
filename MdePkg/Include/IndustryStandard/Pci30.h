/** @file
  Support for PCI 3.0 standard.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <IndustryStandard/Pci23.h>

///
/// PCI_CLASS_MASS_STORAGE, Base Class 01h.
///
///@{
#define PCI_CLASS_MASS_STORAGE_SATADPA  0x06
#define   PCI_IF_MASS_STORAGE_SATA      0x00
#define   PCI_IF_MASS_STORAGE_AHCI      0x01
///@}

///
/// PCI_CLASS_WIRELESS, Base Class 0Dh.
///
///@{
#define PCI_SUBCLASS_ETHERNET_80211A  0x20
#define PCI_SUBCLASS_ETHERNET_80211B  0x21
///@}

/**
  Macro that checks whether device is a SATA controller.

  @param  _p      Specified device.

  @retval TRUE    Device is a SATA controller.
  @retval FALSE   Device is not a SATA controller.

**/
#define IS_PCI_SATADPA(_p)  IS_CLASS2 (_p, PCI_CLASS_MASS_STORAGE, PCI_CLASS_MASS_STORAGE_SATADPA)

//
// Symbol EFI_PCI_CAPABILITY_ID_PCIEXP is obsolete, use PCI_EXPRESS_CAPABILITY_ID.
// PCI_EXPRESS_CAPABILITY_ID is defined beside the capability registers structure
// in PciExpress21.h. This ID is not EFI nor PCI symbol, but PCI Express.
//
#define EFI_PCI_CAPABILITY_ID_PCIEXP  0x10

///
/// Enhanced Allocation (EA) Capability
/// PCI Local Bus Specification 3.0, Enhanced Allocation ECN
///
///@{
#define EFI_PCI_CAPABILITY_ID_EA  0x14

///
/// Number of entries, encoded in the first DWORD of the capability.
///
#define PCI_EA_CAP_NUM_ENTRIES(Dw0)  (((Dw0) >> 16) & 0x3F)

///
/// Type 1 functions carry one additional DWORD (fixed secondary and
/// subordinate bus numbers) before the first entry.
///
#define PCI_EA_CAP_TYPE1_EXTRA_DWORDS  1

///
/// Fields of the additional DWORD of Type 1 functions. A fixed secondary bus
/// number of 0 indicates that the bridge does not use fixed bus numbers.
///
#define PCI_EA_CAP_FIXED_SECONDARY_BUS(Dw1)    ((Dw1) & 0xFF)
#define PCI_EA_CAP_FIXED_SUBORDINATE_BUS(Dw1)  (((Dw1) >> 8) & 0xFF)

///
/// Fields of the first DWORD (header) of each EA entry.
///
#define PCI_EA_ENTRY_SIZE(Hdr)                  ((Hdr) & 0x7)
#define PCI_EA_ENTRY_BEI(Hdr)                   (((Hdr) >> 4) & 0xF)
#define PCI_EA_ENTRY_PRIMARY_PROPERTIES(Hdr)    (((Hdr) >> 8) & 0xFF)
#define PCI_EA_ENTRY_SECONDARY_PROPERTIES(Hdr)  (((Hdr) >> 16) & 0xFF)
#define PCI_EA_ENTRY_WRITABLE  BIT30
#define PCI_EA_ENTRY_ENABLE    BIT31

///
/// BAR Equivalent Indicator (BEI) values.
///
#define PCI_EA_BEI_BAR0      0
#define PCI_EA_BEI_BAR5      5
#define PCI_EA_BEI_BRIDGE    6
#define PCI_EA_BEI_ENI       7
#define PCI_EA_BEI_ROM       8
#define PCI_EA_BEI_VF_BAR0   9
#define PCI_EA_BEI_VF_BAR5   14
#define PCI_EA_BEI_RESERVED  15

///
/// Primary/Secondary Properties values.
///
#define PCI_EA_PROP_MEM              0x00
#define PCI_EA_PROP_MEM_PREFETCH     0x01
#define PCI_EA_PROP_IO               0x02
#define PCI_EA_PROP_VF_MEM_PREFETCH  0x03
#define PCI_EA_PROP_VF_MEM           0x04
#define PCI_EA_PROP_BRIDGE_MEM       0x05
#define PCI_EA_PROP_BRIDGE_PREFETCH  0x06
#define PCI_EA_PROP_BRIDGE_IO        0x07
#define PCI_EA_PROP_MEM_RESERVED     0xFD
#define PCI_EA_PROP_IO_RESERVED      0xFE
#define PCI_EA_PROP_UNAVAILABLE      0xFF

///
/// Base and MaxOffset DWORD fields. Bits [1:0] of MaxOffset are implied
/// to be 11b.
///
#define PCI_EA_FIELD_64BIT  BIT1
#define PCI_EA_FIELD_MASK   0xFFFFFFFC
///@}

#pragma pack(1)

///
/// PCI Data Structure Format
/// Section 5.1.2, PCI Firmware Specification, Revision 3.0
///
typedef struct {
  UINT32    Signature;  ///< "PCIR"
  UINT16    VendorId;
  UINT16    DeviceId;
  UINT16    DeviceListOffset;
  UINT16    Length;
  UINT8     Revision;
  UINT8     ClassCode[3];
  UINT16    ImageLength;
  UINT16    CodeRevision;
  UINT8     CodeType;
  UINT8     Indicator;
  UINT16    MaxRuntimeImageLength;
  UINT16    ConfigUtilityCodeHeaderOffset;
  UINT16    DMTFCLPEntryPointOffset;
} PCI_3_0_DATA_STRUCTURE;

#pragma pack()
