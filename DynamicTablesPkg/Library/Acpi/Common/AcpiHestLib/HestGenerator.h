/** @file

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#pragma once

#pragma pack(1)

/** A structure that describes Source indexer
    used to get related source for GHES.
*/
typedef struct HestSourceIndexer {
  /// Index token for the Source
  CM_OBJECT_TOKEN    Token;

  /// Source offset from the start of the HEST table
  UINT32             Offset;

  /// Pointer to the Source
  VOID               *Object;

  /// Source Id
  UINT16             SourceId;

  /// Found related GHES
  BOOLEAN            GhesFound;
} HEST_SOURCE_INDEXER;

typedef struct AcpiHestGenerator {
  /// ACPI Table generator header
  ACPI_TABLE_GENERATOR    Header;

  // Hest Generator private data

  /// Pointer to the FirmwareFirst Source indexer array
  HEST_SOURCE_INDEXER     *FirmwareFirstIndexer;

  /// FirmwareFirst Source Count
  UINT32                  FirmwareFirstSourceCount;

  /// Pointer to the GHES Assist Source indexer array
  HEST_SOURCE_INDEXER     *GhesAssistIndexer;

  /// GhesAssist Source Count
  UINT32                  GhesAssistSourceCount;
} ACPI_HEST_GENERATOR;

#pragma pack()
