/** @file
  PCCT Table Generator

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.4 Specification - January 2021
    s14 PLATFORM COMMUNICATIONS CHANNEL (PCC)

**/

#ifndef PCCT_GENERATOR_H_
#define PCCT_GENERATOR_H_

#pragma pack(1)

/** Structure used to map a Pcc Subspace to an index.
*/
typedef struct MappingTable {
  /// Mapping table for Subspace Ids.
  /// Subspace ID/Index <-> CM_ARM_PCC_SUBSPACE_TYPE[X]_INFO pointer
  VOID      **Table;

  /// Number of entries in the Table.
  UINT32    MaxIndex;
} MAPPING_TABLE;

/** A structure holding the Pcct generator and additional private data.
*/
typedef struct AcpiPcctGenerator {
  /// ACPI Table generator header
  ACPI_TABLE_GENERATOR    Header;

  // Private fields are defined from here.

  /// Table to map: Subspace ID/Index <-> CM_ARM_PCC_SUBSPACE_TYPE[X]_INFO pointer
  MAPPING_TABLE           MappingTable;
} ACPI_PCCT_GENERATOR;

#pragma pack()

#endif // PCCT_GENERATOR_H_
