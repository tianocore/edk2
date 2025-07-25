/** @file
  Metadata Object Library.

  Copyright (c) 2025, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef METADATA_OBJ_LIB_H_
#define METADATA_OBJ_LIB_H_

/** Metadata Types

  Some general rules:
  -
  Each Type has an associated strucure.
  -
  It is possible to Add/Remove/Get the Metadata associated to
  a (Type/Token). To modify the Metadata, the object must be removed
  then added again.
  -
  A (Type/Token) pair allows to uniquely identify a Metadata structure.
  A Token might have multiple Metadata structures allocated with other Types.
  A Type might have multiple entries with different Tokens.
*/
typedef enum MetadataType {
  MetadataTypeUid,
  MetadataTypeProximityDomain,
  MetadataTypeMax,
} METADATA_TYPE;

/* Maximal size of a NameId */
#define METADATA_UID_NAMEID_SIZE  9

/** MetadataTypeUid related structure.
 */
typedef struct MetadataObjUid {
  /// Uid
  UINT32    Uid;

  /// _HID or _CID of the device (NULL-terminated string).
  /// This provides a mean to uniquely identify a device type.
  /// If not populated, EisaId must be set.
  CHAR8     NameId[METADATA_UID_NAMEID_SIZE];

  /// EisaId of the device.
  /// This provides a mean to uniquely identify a device type.
  /// If not populated, NameId must be set.
  UINT32    EisaId;
} METADATA_OBJ_UID;

/** MetadataTypeProximityDomain related structure.
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

/** Attach some Metadata to a (Type/Token) pair.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Type          METADATA_TYPE of the entry to allocate.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_TYPE.
  @param[in]  Metadata      Metadata to associate to the (Type/Token) pair.
                            The data is copied.
  @param[in]  MetadataSize  Size of the input Metadata.

  @retval EFI_SUCCESS     Success.
  @retval EFI_ALREADY_STARTED   (Type/Token) pair is already present.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataAdd (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_TYPE         Type,
  IN  CM_OBJECT_TOKEN       Token,
  IN  VOID                  *Metadata,
  IN  UINT32                MetadataSize
  );

/** Remove a (Type/Token) pair and its associated Metadata.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Type          METADATA_TYPE of the entry to remove.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_TYPE.

  @retval EFI_SUCCESS     Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataRemove (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_TYPE         Type,
  IN  CM_OBJECT_TOKEN       Token
  );

/** Get the Metadata associated with an (Type/Token).

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Type          METADATA_TYPE of the entry to get.
  @param[in]  Token         Token uniquely identifying an entry among other
                            objects with the input METADATA_TYPE.
  @param[out] Metadata      If success, contains the Metadata associated to the
                            input (Type/Token).
  @param[in]  MetadataSize  Size of the input Metadata.

  @retval EFI_SUCCESS     Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND   Not found.
**/
EFI_STATUS
EFIAPI
MetadataGet (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_TYPE         Type,
  IN  CM_OBJECT_TOKEN       Token,
  OUT VOID                  *Metadata,
  IN  UINT32                MetadataSize
  );

/** Iterate over the existing Metadata with the same Type.

  @param[in]  Root          Root of the Metadata information.
  @param[in]  Type          METADATA_TYPE to iterate over.
  @param[in]  PrevHandle    MetadataIterate () returns the Metadata handle
                            following PrevHandle.
                            If PrevHandle==NULL, the first Handle of the type
                            is returned.
                            If PrevHandle is the last Handle of the type,
                            NULL is returned.
  @param[out] Metadata      Metadata of the current Handle.
  @param[in]  MetadataSize  Size of the input Metadata.

  @return METADATA_HANDLE   The Metadata handle following PrevHandle.
**/
METADATA_HANDLE
EFIAPI
MetadataIterate (
  IN  METADATA_ROOT_HANDLE  Root,
  IN  METADATA_TYPE         Type,
  IN  METADATA_HANDLE       PrevHandle,
  OUT VOID                  *Metadata,
  IN  UINT32                MetadataSize
  );

#endif // METADATA_OBJ_LIB_H_
