/** @file
  Configuration Manager object token fixer

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef CM_OBJECT_TOKEN_FIXER_H_
#define CM_OBJECT_TOKEN_FIXER_H_

/** CmObjectToken fixer function that updates the Tokens in the CmObjects.

  @param [in]  CmObject    Pointer to the Configuration Manager Object.
  @param [in]  Token       Token to be updated in the CmObject.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_UNSUPPORTED       Not supported.
**/
typedef
EFI_STATUS
(EFIAPI *CM_OBJECT_TOKEN_FIXER)(
  IN  CM_OBJ_DESCRIPTOR   *CmObject,
  IN  CM_OBJECT_TOKEN       Token
  );

/** CmObj token fixer.

  Some CmObj structures have a self-token, i.e. they are storing their own
  token value in the CmObj. Dynamically created CmObj need to have their
  self-token assigned at some point.

  @param [in]  CmObjDesc   Pointer to the Configuration Manager Object.
  @param [in]  Token       Token to update the CmObjDesc with.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_UNSUPPORTED       Not supported.
**/
EFI_STATUS
EFIAPI
FixupCmObjectSelfToken (
  IN  CM_OBJ_DESCRIPTOR  *CmObjDesc,
  IN  CM_OBJECT_TOKEN    Token
  );

#endif // CM_OBJECT_TOKEN_FIXER_H_
