/** @file
  Token Mapper

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "TokenMapper.h"

/** Add a CmObjDesc to the TokenMapper.

  @param [in] TokenMapper   The TokenMapper instance.
  @param [in] Token         CmObj token.
  @param [in] ObjectId      CmObj ObjectId.
  @param [in] Size          CmObj Size.
  @param [in] Data          CmObj Data.
                            This memory is referenced, not copied.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    Buffer too small.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
**/
EFI_STATUS
EFIAPI
TokenMapperAddObject (
  IN  TOKEN_MAPPER     *TokenMapper,
  IN  CM_OBJECT_TOKEN  Token,
  IN  CM_OBJECT_ID     ObjectId,
  IN  UINT32           Size,
  IN  VOID             *Data
  )
{
  TOKEN_MAP_DESCRIPTOR  *TokenMapDesc;
  CM_OBJ_DESCRIPTOR     *CmObjDesc;

  if ((TokenMapper == NULL)                 ||
      (TokenMapper->TokenDescArray == NULL) ||
      (Size == 0)                           ||
      (Data == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (TokenMapper->ItemCount >= TokenMapper->MaxTokenDescCount) {
    ASSERT (0);
    return EFI_BUFFER_TOO_SMALL;
  }

  TokenMapDesc        = &TokenMapper->TokenDescArray[TokenMapper->ItemCount++];
  TokenMapDesc->Token = Token;
  CmObjDesc           = &TokenMapDesc->CmObjDesc;
  CmObjDesc->ObjectId = ObjectId;
  CmObjDesc->Size     = Size;

  // Point inside the finalized array.
  CmObjDesc->Data = Data;

  // Only EArchCommonObjCmRef CmObj can be added as
  // arrays (more than 1 elements).
  if ((GET_CM_NAMESPACE_ID (ObjectId) == EObjNameSpaceArchCommon) &&
      (GET_CM_OBJECT_ID (ObjectId) == EArchCommonObjCmRef))
  {
    CmObjDesc->Count = Size / sizeof (CM_ARCH_COMMON_OBJ_REF);
  } else {
    CmObjDesc->Count = 1;
  }

  return EFI_SUCCESS;
}

/** Get a CmObjDesc from a ObjectId/Token couple.

  The Token parameter is not optional. An existing token must be provided.

  @param [in]  TokenMapper   The TokenMapper instance.
  @param [in]  Token         Token of the CmObj to search.
  @param [in]  ObjectId      Object Id of the CmObj to search.
  @param [out] CmObjDesc     CM_OBJ_DESCRIPTOR containing the CmObj searched.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           Not found.
**/
EFI_STATUS
EFIAPI
TokenMapperGetObject (
  IN  TOKEN_MAPPER       *TokenMapper,
  IN  CM_OBJECT_TOKEN    Token,
  IN  CM_OBJECT_ID       ObjectId,
  OUT CM_OBJ_DESCRIPTOR  *CmObjDesc
  )
{
  UINTN                 Index;
  UINTN                 MaxCount;
  TOKEN_MAP_DESCRIPTOR  *TokenMapDesc;

  // Nothing to do.
  if ((TokenMapper != NULL) && (TokenMapper->MaxTokenDescCount == 0)) {
    goto exit_handler;
  }

  if ((Token == CM_NULL_TOKEN)              ||
      (CmObjDesc == NULL)                   ||
      (TokenMapper == NULL)                 ||
      (TokenMapper->TokenDescArray == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  TokenMapDesc = TokenMapper->TokenDescArray;
  MaxCount     = TokenMapper->MaxTokenDescCount;
  for (Index = 0; Index < MaxCount; Index++) {
    if ((TokenMapDesc->CmObjDesc.ObjectId == ObjectId)  &&
        (TokenMapDesc->Token == Token))
    {
      CopyMem (
        CmObjDesc,
        &TokenMapDesc->CmObjDesc,
        sizeof (CM_OBJ_DESCRIPTOR)
        );
      return EFI_SUCCESS;
    }

    TokenMapDesc++;
  } // for

exit_handler:
  DEBUG ((
    DEBUG_INFO,
    "INFO: Requested CmObj of type 0x%x with token 0x%x"
    " not found in the dynamic repository\n.",
    ObjectId,
    Token
    ));
  return EFI_NOT_FOUND;
}

/** Initialise a TokenMapper.

  @param [in] TokenMapper       The TokenMapper to initialise.
  @param [in] DescriptorCount   Number of entries to allocate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ALREADY_STARTED     Instance already initialised.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
**/
EFI_STATUS
EFIAPI
TokenMapperInitialise (
  IN  TOKEN_MAPPER  *TokenMapper,
  IN  UINTN         DescriptorCount
  )
{
  if (TokenMapper == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Nothing to do.
  if (DescriptorCount == 0) {
    return EFI_SUCCESS;
  }

  if (TokenMapper->TokenDescArray != NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Token mapper already initialised\n."));
    ASSERT (0);
    return EFI_ALREADY_STARTED;
  }

  TokenMapper->TokenDescArray =
    AllocateZeroPool (sizeof (TOKEN_MAP_DESCRIPTOR) * DescriptorCount);
  if (TokenMapper->TokenDescArray == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  TokenMapper->MaxTokenDescCount = DescriptorCount;
  TokenMapper->ItemCount         = 0;

  return EFI_SUCCESS;
}

/** Shutdown a TokenMapper.

  @param [in] TokenMapper       The TokenMapper to shutdown.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
**/
EFI_STATUS
EFIAPI
TokenMapperShutdown (
  IN  TOKEN_MAPPER  *TokenMapper
  )
{
  // Nothing to do.
  if ((TokenMapper != NULL) && (TokenMapper->MaxTokenDescCount == 0)) {
    return EFI_SUCCESS;
  }

  if ((TokenMapper == NULL) ||
      (TokenMapper->TokenDescArray == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (TokenMapper->TokenDescArray);
  TokenMapper->TokenDescArray    = NULL;
  TokenMapper->MaxTokenDescCount = 0;

  return EFI_SUCCESS;
}
