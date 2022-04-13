/** @file
  Configuration manager Object Descriptor Utility.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CM_OBJECT_DESC_UTILITY_H_
#define CM_OBJECT_DESC_UTILITY_H_

#include <ConfigurationManagerObject.h>

#include "FdtHwInfoParser.h"

/** Create a CM_OBJ_DESCRIPTOR.

  @param [in]  ObjectId       CM_OBJECT_ID of the node.
  @param [in]  Count          Number of CmObj stored in the
                              data field.
  @param [in]  Data           Pointer to one or more CmObj objects.
                              The content of this Data buffer is copied.
  @param [in]  Size           Size of the Data buffer.
  @param [out] NewCmObjDesc   The created CM_OBJ_DESCRIPTOR.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    An allocation has failed.
**/
EFI_STATUS
EFIAPI
CreateCmObjDesc (
  IN  CM_OBJECT_ID       ObjectId,
  IN  UINT32             Count,
  IN  VOID               *Data,
  IN  UINT32             Size,
  OUT CM_OBJ_DESCRIPTOR  **NewCmObjDesc
  );

/** Free resources allocated for the CM_OBJ_DESCRIPTOR.

  @param [in] CmObjDesc           Pointer to the CM_OBJ_DESCRIPTOR.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FreeCmObjDesc (
  IN CM_OBJ_DESCRIPTOR  *CmObjDesc
  );

/** Add a single CmObj to the Configuration Manager.

  @param  [in]  FdtParserHandle   A handle to the parser instance.
  @param  [in]  ObjectId          CmObj ObjectId.
  @param  [in]  Data              CmObj Data.
  @param  [in]  Size              CmObj Size.
  @param  [out] Token             If provided and success,
                                  token generated for this CmObj.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AddSingleCmObj (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        CM_OBJECT_ID               ObjectId,
  IN        VOID                       *Data,
  IN        UINT32                     Size,
  OUT       CM_OBJECT_TOKEN            *Token    OPTIONAL
  );

/** Add multiple CmObj to the Configuration Manager.

  @param  [in]  FdtParserHandle   A handle to the parser instance.
  @param  [in]  CmObjDesc         CmObjDesc containing multiple CmObj
                                  to add.
  @param  [in]  TokenCount        If provided, count of entries in the
                                  TokenTable.
  @param  [out] TokenTable        If provided and success,
                                  token generated for these CmObj.
                                  Address of an array of CM_OBJECT_TOKEN
                                  with the same number of elements as the
                                  CmObjDesc.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AddMultipleCmObj (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE FdtParserHandle,
  IN  CONST CM_OBJ_DESCRIPTOR *CmObjDesc,
  IN        UINT32 TokenCount, OPTIONAL
  OUT       CM_OBJECT_TOKEN             *TokenTable   OPTIONAL
  );

/** Add multiple CmObj to the Configuration Manager.

  Get one token referencing a EArmObjCmRef CmObj itself referencing
  the input CmObj. In the table below, RefToken is returned.

  Token referencing an      Array of tokens             Array of CmObj
  array of EArmObjCmRef     referencing each            from the input:
  CmObj:                    CmObj from the input:

  RefToken         --->     CmObjToken[0]        --->   CmObj[0]
                            CmObjToken[1]        --->   CmObj[1]
                            CmObjToken[2]        --->   CmObj[2]

  @param  [in]  FdtParserHandle   A handle to the parser instance.
  @param  [in]  CmObjDesc         CmObjDesc containing multiple CmObj
                                  to add.
  @param  [out] Token             If success, token referencing an array
                                  of EArmObjCmRef CmObj, themselves
                                  referencing the input CmObjs.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    An allocation has failed.
**/
EFI_STATUS
EFIAPI
AddMultipleCmObjWithCmObjRef (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN  CM_OBJ_DESCRIPTOR                *CmObjDesc,
  OUT CM_OBJECT_TOKEN                  *Token
  );

#endif // CM_OBJECT_DESC_UTILITY_H_
