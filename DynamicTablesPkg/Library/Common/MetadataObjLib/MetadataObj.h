/** @file
  Metadata Object Library.

  Copyright (c) 2025, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef METADATA_OBJ_H_
#define METADATA_OBJ_H_

#include <Library/MetadataObjLib.h>

/** Metadata Entry.

  Store the generated Metadata along the associated CmObj/Token
  in this structure.
**/
typedef struct MetadataEntry {
  /// List entry
  LIST_ENTRY         List;

  /// Metadata Type
  METADATA_TYPE      Type;

  /// Token
  CM_OBJECT_TOKEN    Token;

  /// Metadata. Must be interpreted per-METADATA_TYPE
  VOID               *Metadata;
} METADATA_ENTRY;

/** Metadata Object Type.

  There is one entry for for each METADATA_TYPE.
**/
typedef struct MetadataList {
  /// Per-METADATA_TYPE list of METADATA_ENTRY struct
  LIST_ENTRY    List;
} METADATA_LIST;

/** Metadata static information.

  There is one entry for for each METADATA_TYPE.
**/
typedef struct MetadataStaticInfo {
  /// Expected size for this METADATA_TYPE
  UINT32    ExpectedSize;
} METADATA_STATIC_INFO;

/** Metadata Root.

  All the METADATA_ENTRY are attached to a root.
**/
typedef struct MetadataRoot {
  /// Array of METADATA_LIST. One entry for each MetadataType.
  METADATA_LIST    MetadataList[MetadataTypeMax];
} METADATA_ROOT;

#endif // METADATA_OBJ_H_
