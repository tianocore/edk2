/** @file
  Token Mapper

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef TOKEN_MAPPER_H_
#define TOKEN_MAPPER_H_

#pragma pack(1)

/** Token mapping descriptor.

  Bind a token and a CmObj together.
*/
typedef struct TokenMapDescriptor {
  /// Object Token.
  CM_OBJECT_TOKEN      Token;

  /// CmObjectDescriptor CM_OBJ_DESCRIPTOR.Data is a reference copy
  /// and not allocated. It points to the individual objects in the
  /// Dynamic Plat Repo ArmNameSpaceObjectArray.
  CM_OBJ_DESCRIPTOR    CmObjDesc;
} TOKEN_MAP_DESCRIPTOR;

/** Token mapper.

  Contain all the Token/CmObj couple mapping.
**/
typedef struct TokenMapper {
  /// Maximum number of TOKEN_MAP_DESCRIPTOR entries in TokenDescArray.
  UINTN                   MaxTokenDescCount;

  /// Next TOKEN_MAP_DESCRIPTOR entry to use in TokenDescArray.
  UINTN                   ItemCount;

  /// Array of TOKEN_MAP_DESCRIPTOR.
  TOKEN_MAP_DESCRIPTOR    *TokenDescArray;
} TOKEN_MAPPER;

#pragma pack()

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
  );

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
  );

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
  );

/** Shutdown a TokenMapper.

  @param [in] TokenMapper       The TokenMapper to shutdown.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
**/
EFI_STATUS
EFIAPI
TokenMapperShutdown (
  IN  TOKEN_MAPPER  *TokenMapper
  );

#endif // TOKEN_MAPPER_H_
