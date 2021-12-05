/** @file

  Copyright (c) 2018, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#ifndef IORT_GENERATOR_H_
#define IORT_GENERATOR_H_

#pragma pack(1)

/** A structure that describes the Node indexer
    used for indexing the IORT nodes.
*/
typedef struct IortNodeIndexer {
  /// Index token for the Node
  CM_OBJECT_TOKEN    Token;
  /// Pointer to the node
  VOID               *Object;
  /// Node offset from the start of the IORT table
  UINT32             Offset;
} IORT_NODE_INDEXER;

typedef struct AcpiIortGenerator {
  /// ACPI Table generator header
  ACPI_TABLE_GENERATOR    Header;

  // IORT Generator private data

  /// IORT node count
  UINT32                  IortNodeCount;
  /// Pointer to the node indexer array
  IORT_NODE_INDEXER       *NodeIndexer;
} ACPI_IORT_GENERATOR;

#pragma pack()

#endif // IORT_GENERATOR_H_
