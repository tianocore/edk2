/** @file
  Metadata Object Library.

  Copyright (c) 2025, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef METADATA_OBJ_H_
#define METADATA_OBJ_H_

/** Metadata Entry.

  Store the generated Metadata along the associated CmObj/Token
  in this structure.
**/
typedef struct MetadataEntry {
  /// List entry
  LIST_ENTRY         List;

  /// Metadata Id
  METADATA_ID        Id;

  /// Token
  CM_OBJECT_TOKEN    Token;

  /// Metadata. Must be interpreted per-METADATA_ID
  VOID               *Metadata;
} METADATA_ENTRY;

/** Metadata Object Type.

  There is one entry for for each METADATA_ID.
**/
typedef struct MetadataList {
  /// Per-METADATA_ID list of METADATA_ENTRY struct
  LIST_ENTRY    List;
} METADATA_LIST;

/** Metadata static information.

  There is one entry for for each METADATA_ID.
**/
typedef struct MetadataStaticInfo {
  /// Expected size for this METADATA_ID
  UINT32    ExpectedSize;
} METADATA_STATIC_INFO;

/** Metadata Root.

  All the METADATA_ENTRY are attached to a root.
**/
typedef struct MetadataRoot {
  /// Number of entries in the MetadataList array.
  UINT32           NumEntries;

  /// Array of METADATA_LIST. One entry for each MetadataType.
  METADATA_LIST    *MetadataList;
} METADATA_ROOT;

#endif // METADATA_OBJ_H_
