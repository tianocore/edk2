/** @file
  Metadata Proximity Domain handlers.

  Copyright (c) 2025, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <ConfigurationManagerObject.h>

#include <Library/MetadataObjLib.h>
#include "MetadataHandler.h"

typedef struct ProximityDomainData {
  /// Current Id used for the Proximity Domain Id generation.
  UINT32    CurrentId;
} PROXIMITY_DOMAIN_DATA;

STATIC PROXIMITY_DOMAIN_DATA  mProximityDomainData = {
  0
};

/** Query the MetadataObjLib for metadata matching the input (Type/Token).
    If the metadata exists, return it.
    Otherwise:
    - Generate a new metadata object
    - Add it to the MetadataObjLib
    - return it

  @param[in]       Root          Root of the Metadata information.
  @param[in]       Type          METADATA_TYPE of the entry to generate.
  @param[in]       Token         Token uniquely identifying an entry among other
                                 objects with the input METADATA_TYPE.
  @param[in]       Context       Optional context to use during the Metadata generation.
  @param[in, out]  Metadata      On input, can contain METADATA_TYPE-specific information.
                                 On output and if success, contains the generated
                                 Metadata object.
  @param[in]       MetadataSize  Size of the input Metadata.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
EFIAPI
MetadataGenerateProximityDomain (
  IN      METADATA_ROOT_HANDLE  Root,
  IN      METADATA_TYPE         Type,
  IN      CM_OBJECT_TOKEN       Token,
  IN      VOID                  *Context,
  IN OUT  VOID                  *Metadata,
  IN      UINT32                MetadataSize
  )
{
  EFI_STATUS                     Status;
  METADATA_OBJ_PROXIMITY_DOMAIN  *ProximityDomain;

  if ((Type != MetadataTypeProximityDomain)   ||
      (Token == CM_NULL_TOKEN)                ||
      (Metadata == NULL))
  {
    ASSERT (Type == MetadataTypeProximityDomain);
    ASSERT (Token != CM_NULL_TOKEN);
    ASSERT (Metadata != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = MetadataGet (Root, Type, Token, Metadata, MetadataSize);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  } else if (Status == EFI_SUCCESS) {
    // Metadata for this (Type/Token) already exists.
    return EFI_SUCCESS;
  }

  // Generate new Metadata (i.e. Status == EFI_NOT_FOUND).

  ProximityDomain     = (METADATA_OBJ_PROXIMITY_DOMAIN *)Metadata;
  ProximityDomain->Id = mProximityDomainData.CurrentId++;

  Status = MetadataAdd (Root, Type, Token, Metadata, MetadataSize);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  return EFI_SUCCESS;
}

/** Validate the Metadata.

  @param[in]  Root    Root of the Metadata information.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
EFIAPI
MetadataValidateProximityDomain (
  IN  METADATA_ROOT_HANDLE  Root
  )
{
  METADATA_HANDLE                Handle0;
  METADATA_HANDLE                Handle1;
  METADATA_OBJ_PROXIMITY_DOMAIN  Metadata0;
  METADATA_OBJ_PROXIMITY_DOMAIN  Metadata1;

  Handle0 = NULL;

  while (TRUE) {
    Handle0 = MetadataIterate (
                Root,
                MetadataTypeProximityDomain,
                Handle0,
                &Metadata0,
                sizeof (METADATA_OBJ_PROXIMITY_DOMAIN)
                );
    if (Handle0 == NULL) {
      break;
    }

    // Loop starting from Handle0
    Handle1 = Handle0;

    while (TRUE) {
      Handle1 = MetadataIterate (
                  Root,
                  MetadataTypeProximityDomain,
                  Handle1,
                  &Metadata1,
                  sizeof (METADATA_OBJ_PROXIMITY_DOMAIN)
                  );

      if (Handle1 == NULL) {
        break;
      }

      if (Metadata0.Id == Metadata1.Id) {
        DEBUG ((
          DEBUG_ERROR,
          "Metadata: ProximityDomain: Same Id: %d\n",
          Metadata0.Id
          ));
        ASSERT (0);
      }
    }
  }

  return EFI_SUCCESS;
}
