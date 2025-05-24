/** @file
  Metadata Object Library.

  Copyright (c) 2025, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef METADATA_OBJ_LIB_H_
#define METADATA_OBJ_LIB_H_

/** Metadata Ids

  Some general rules:
  -
  Each Id has an associated strucure.
  -
  It is possible to Add/Remove/Get the Metadata associated to
  a (Id/Token). To modify the Metadata, the object must be removed
  then added again.
  -
  A (Id/Token) pair allows to uniquely identify a Metadata structure.
  A Token might have multiple Metadata structures allocated with other Ids.
  An Id might have multiple entries with different Tokens.
*/
typedef enum MetadataId {
  MetadataUidId,
  MetadataProximityDomainId,
  MetadataMaxId,
} METADATA_ID;

/** MetadataUidId related structure.
 */
typedef struct MetadataObjUid {
  /// Uid
  UINT32    Uid;

  /// _HID or _CID of the device (NULL-terminated string).
  /// This provides a mean to uniquely identify a device type.
  /// If not populated, EisaId must be set.
  CHAR8     NameId[9];

  /// EisaId of the device.
  /// This provides a mean to uniquely identify a device type.
  /// If not populated, NameId must be set.
  UINT32    EisaId;
} METADATA_OBJ_UID;

/** MetadataProximityDomainId related structure.
 */
typedef struct MetadataObjProximityDomain {
  /// Proximity Domain Id
  UINT32    Id;
} METADATA_OBJ_PROXIMITY_DOMAIN;

/* Handle to the structure containing all the the Metadata information
   (i.e. all the METADATA_HANDLE).
*/
typedef VOID *METADATA_ROOT_HANDLE;
/* Handle to an internal Metadata structure, depicting a single Metadata. */
typedef VOID *METADATA_HANDLE;

/** Initialize the Metadata Root.

  @param[out]  Root  If success, Root of the Metadata information.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
MetadataInitializeHandle (
  OUT METADATA_ROOT_HANDLE  *Root
  );

/** Free the Metadata Root.

  @param[in]  Root  Root of the Metadata information to free.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
MetadataFreeHandle (
  IN METADATA_ROOT_HANDLE  Root
  );

/** Attach some Metadata to a (Id/Token) pair.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Id            METADATA_ID of the entry to allocate.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_ID.
  @param[in]  Metadata      Metadata to associate to the (Id/Token) pair.
                            The data is copied.
  @param[in]  MetadataSize  Size of the input Metadata.

  @retval EFI_SUCCESS     Success.
  @retval EFI_ALREADY_STARTED   (Id/Token) pair is already present.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataAdd (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_ID           Id,
  IN  CM_OBJECT_TOKEN       Token,
  IN  VOID                  *Metadata,
  IN  UINT32                MetadataSize
  );

/** Remove a (Id/Token) pair and its associated Metadata.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Id            METADATA_ID of the entry to remove.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_ID.

  @retval EFI_SUCCESS     Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataRemove (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_ID           Id,
  IN  CM_OBJECT_TOKEN       Token
  );

/** Get the Metadata associated to an (Id/Token).

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Id            METADATA_ID of the entry to get.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_ID.
  @param[out] Metadata      If success, contains the Metadata associated to the
                            input (Id/Token).
  @param[in]  MetadataSize  Size of the input Metadata.

  @retval EFI_SUCCESS     Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataGet (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_ID           Id,
  IN  CM_OBJECT_TOKEN       Token,
  OUT VOID                  *Metadata,
  IN  UINT32                MetadataSize
  );

/** Iterate over the existing Metadata with the same Id.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Id            METADATA_ID to iterate over.
  @param[in]  PrevHandle    On entry: Handle to the Metadata structure previously
                            iterated over.
                            On exit: Handle to the Metadata structure currently
                            iterated over.
  @param[out] Metadata      Metadata of the current Handle.
  @param[in]  MetadataSize  Size of the input Metadata.

  @return METADATA_HANDLE   The handle.
**/
METADATA_HANDLE
EFIAPI
MetadataIterate (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_ID           Id,
  IN  METADATA_HANDLE       PrevHandle,
  OUT VOID                  *Metadata,
  IN  UINT32                MetadataSize
  );

#endif // METADATA_OBJ_LIB_H_
