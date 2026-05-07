/** @file
  CXL CDAT structure definitions

  This file contains the structure definitions based on the
  Coherent Device Attribute Table (CDAT) Specification 1.01

  Copyright (c) 2026, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

///
/// Definition for the CDAT Header.
/// Coherent Device Attribute Table 1.01 Specification.
///
typedef struct {
  UINT32    Length;   // Length of the enture table in bytes.
  UINT8     Revision; // Must be 1.
  UINT8     Checksum; // Checksum value that results in sum of 0.
  UINT8     Reserved[6];
  UINT32    Sequence; // Sequence number to indicate changes in sub tables.
} CXL_CDAT_HEADER;

typedef struct {
  CXL_CDAT_HEADER    Header;
  /// Subtables, each beginning with CXL_CDAT_COMMON_SUBTABLE_HEADER.
  UINT8              Data[];
} CXL_CDAT_STRUCTURE;

///
/// CDAT Structure Types.
/// Coherent Device Attribute Table 1.01 Specification.
///
typedef enum {
  CxlCdatTypeDsmas   = 0,
  CxlCdatTypeDslbis  = 1,
  CxlCdatTypeDsmscis = 2,
  CxlCdatTypeDsis    = 3,
  CxlCdatTypeDsemts  = 4,
  CxlCdatTypeSslbis  = 5,
} CXL_CDAT_TYPE;

typedef struct {
  UINT8     Type; // CXL_CDAT_TYPE type.
  UINT8     Reserved;
  UINT16    Length; // Length of only the subtable in bytes.
} CXL_CDAT_COMMON_SUBTABLE_HEADER;

///
/// Definition for the Device Scoped Memory Affinity Structure (DSMAS).
/// Coherent Device Attribute Table 1.01 Specification.
///
/// The DSMAS table contains handles to Device Scoped Memory Affinity Domains
/// (DSMAD, not a typo of DSMAS). A DSMAD is a contiguous Device Physical
/// Address (DPA) range that should be treated as a distinct memory proximity
/// domain.
///
/// Host-side software is expected to map either:
///   * a non-interleaved DSMAD handle to a single SRAT proximity domain; or
///   * an interleaved set of DSMAD handles to a single SRAT proximity domain.
///
typedef struct {
  CXL_CDAT_COMMON_SUBTABLE_HEADER    Header;
  UINT8                              DsmadHandle;
  UINT8                              Flags;
  UINT8                              Reserved[2];
  UINT64                             DpaBase;
  UINT64                             DpaLength;
} CXL_CDAT_DEVICE_SCOPED_MEMORY_AFFINITY_STRUCTURE;

///
/// Definition for the Device Scoped Memory Latency and Bandwidth Structure.
/// Coherent Device Attribute Table 1.01 Specification.
///
typedef struct {
  CXL_CDAT_COMMON_SUBTABLE_HEADER    Header;
  UINT8                              Handle;
  UINT8                              Flags;
  UINT8                              DataType;
  UINT8                              Reserved0;
  UINT64                             EntryBaseUnit;
  UINT16                             Entry[3]; // See the spec for a full definition of this field.
  UINT8                              Reserved1[2];
} CXL_CDAT_DEVICE_SCOPED_LAT_BW_STRUCTURE;

///
/// Definition for the Device Scoped Memory Side Cache Information Structure
/// Coherent Device Attribute Table 1.01 Specification.
///
typedef struct {
  CXL_CDAT_COMMON_SUBTABLE_HEADER    Header;
  UINT8                              DsmasHandle;
  UINT8                              Flags;
  UINT8                              Reserved[3];
  UINT64                             MemorySideCacheSize;
  UINT32                             CacheAttributes;
} CXL_CDAT_DEVICE_SCOPED_MEMORY_SIDE_CACHE_INFO_STRUCTURE;

///
/// Definition for the Device Scoped Initiator Structure
/// Coherent Device Attribute Table 1.01 Specification.
///
typedef struct {
  CXL_CDAT_COMMON_SUBTABLE_HEADER    Header;
  UINT8                              Flags;
  UINT8                              Handle;
  UINT8                              Reserved[2];
} CXL_CDAT_DEVICE_SCOPED_INITIATOR_STRUCTURE;

typedef enum {
  CxlCdatEfiConventionalMemory   = 0,
  CxlCdatEfiConventionalMemorySp = 1,
  CxlCdatEfiReservedMemory       = 2,
} CXL_CDAT_EFI_MEMORY_TYPE;

///
/// Definition for the Device Scoped EFI Memory Type Structure
/// Coherent Device Attribute Table 1.01 Specification.
///
typedef struct {
  CXL_CDAT_COMMON_SUBTABLE_HEADER    Header;
  UINT8                              DsmasHandle;
  UINT8                              EfiMemoryType;
  UINT8                              Reserved[2];
  UINT64                             DpaOffset;
  UINT64                             DpaLength;
} CXL_CDAT_DEVICE_SCOPED_EFI_MEMORY_TYPE_STRUCTURE;

///
/// Definition for the Switch Scoped Latency and Bandwidth Information Structure
/// Coherent Device Attribute Table 1.01 Specification.
///
typedef struct {
  UINT16    PortXId;
  UINT16    PortYId;
  UINT16    LatOrBw;
  UINT16    Reserved;
} CXL_CDAT_SSLBE_ENTRY;

typedef struct {
  CXL_CDAT_COMMON_SUBTABLE_HEADER    Header;
  UINT8                              DataType;
  UINT8                              Reserved[3];
  UINT64                             EntryBaseUnit;
  CXL_CDAT_SSLBE_ENTRY               Entry[];
} CXL_CDAT_SWITCH_SCOPED_LAT_BW_INFO_STRUCTURE;
