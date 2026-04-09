/** @file
  Metadata Handler Library.

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

/* Metadata handlers. */
STATIC METADATA_HANDLERS  mMetadataHandlers[MetadataTypeMax] = {
  // MetadataTypeUid
  {
    MetadataGenerateUid,
    MetadataValidateUid,
  },
  // MetadataTypeProximityDomain
  {
    MetadataGenerateProximityDomain,
    MetadataValidateProximityDomain,
  },
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
MetadataHandlerGenerate (
  IN      METADATA_ROOT_HANDLE  Root,
  IN      METADATA_TYPE         Type,
  IN      CM_OBJECT_TOKEN       Token,
  IN      VOID                  *Context,
  IN OUT  VOID                  *Metadata,
  IN      UINT32                MetadataSize
  )
{
  EFI_STATUS  Status;

  if ((Type >= MetadataTypeMax) ||
      (Token == CM_NULL_TOKEN)  ||
      (Metadata == NULL)        ||
      (MetadataSize == 0))
  {
    ASSERT (Type < MetadataTypeMax);
    ASSERT (Token != CM_NULL_TOKEN);
    ASSERT (Metadata != NULL);
    ASSERT (MetadataSize != 0);
    return EFI_INVALID_PARAMETER;
  }

  Status = mMetadataHandlers[Type].Generate (
                                     Root,
                                     Type,
                                     Token,
                                     Context,
                                     Metadata,
                                     MetadataSize
                                     );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Validate the Metadata.

  @param[in]  Root  Root of the Metadata information.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
EFIAPI
MetadataHandlerValidate (
  IN  METADATA_ROOT_HANDLE  Root
  )
{
  EFI_STATUS     Status;
  METADATA_TYPE  Type;

  for (Type = 0; Type < MetadataTypeMax; Type++) {
    Status = mMetadataHandlers[Type].Validate (Root);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
