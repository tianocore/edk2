/** @file

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#ifndef RHCT_GENERATOR_H_
#define RHCT_GENERATOR_H_

#pragma pack(1)

/** A structure that describes the Node indexer
    used for indexing the RHCT nodes.
*/
typedef struct RhctNodeIndexer {
  /// Index token for the Node
  CM_OBJECT_TOKEN    Token;
  /// Pointer to the node
  VOID               *Object;
  /// Node offset from the start of the RHCT table
  UINT32             Offset;
} RHCT_NODE_INDEXER;

typedef struct AcpiRhctGenerator {
  /// ACPI Table generator header
  ACPI_TABLE_GENERATOR    Header;

  // RHCT Generator private data

  /// RHCT node count
  UINT32                  RhctNodeCount;
  /// Pointer to the node indexer array
  RHCT_NODE_INDEXER       *NodeIndexer;
} ACPI_RHCT_GENERATOR;

#pragma pack()

#endif // RHCT_GENERATOR_H_
