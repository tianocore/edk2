/** @file
  Configuration manager Object Descriptor Utility.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <FdtHwInfoParserInclude.h>
#include <ConfigurationManagerObject.h>

#include "CmObjectDescUtility.h"

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
  )
{
  CM_OBJ_DESCRIPTOR  *CmObjDesc;
  VOID               *DataBuffer;

  if ((Count == 0)      ||
      (Data == NULL)    ||
      (Size == 0)       ||
      (NewCmObjDesc == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CmObjDesc = AllocateZeroPool (sizeof (CM_OBJ_DESCRIPTOR));
  if (CmObjDesc == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  DataBuffer = AllocateCopyPool (Size, Data);
  if (DataBuffer == NULL) {
    FreePool (CmObjDesc);
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjDesc->ObjectId = ObjectId;
  CmObjDesc->Count    = Count;
  CmObjDesc->Data     = DataBuffer;
  CmObjDesc->Size     = Size;

  *NewCmObjDesc = CmObjDesc;

  return EFI_SUCCESS;
}

/** Free resources allocated for the CM_OBJ_DESCRIPTOR.

  @param [in] CmObjDesc           Pointer to the CM_OBJ_DESCRIPTOR.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
FreeCmObjDesc (
  IN CM_OBJ_DESCRIPTOR  *CmObjDesc
  )
{
  if (CmObjDesc == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (CmObjDesc->Data != NULL) {
    FreePool (CmObjDesc->Data);
  }

  FreePool (CmObjDesc);
  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS         Status;
  CM_OBJ_DESCRIPTOR  CmObjDesc;

  if ((FdtParserHandle == NULL)             ||
      (FdtParserHandle->HwInfoAdd == NULL)  ||
      (Data == NULL)                        ||
      (Size == 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CmObjDesc.ObjectId = ObjectId;
  CmObjDesc.Count    = 1;
  CmObjDesc.Data     = Data;
  CmObjDesc.Size     = Size;

  // Add the CmObj.
  // Don't ask for a token.
  Status = FdtParserHandle->HwInfoAdd (
                              FdtParserHandle,
                              FdtParserHandle->Context,
                              &CmObjDesc,
                              Token
                              );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

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
  )
{
  EFI_STATUS         Status;
  UINT32             Index;
  UINT32             Count;
  UINT8              *Data;
  UINT32             Size;
  CM_OBJ_DESCRIPTOR  SingleCmObjDesc;

  if ((FdtParserHandle == NULL)             ||
      (FdtParserHandle->HwInfoAdd == NULL)  ||
      (CmObjDesc == NULL)                   ||
      (CmObjDesc->Count == 0)               ||
      (CmObjDesc->Data == NULL)             ||
      (CmObjDesc->Size == 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Count = CmObjDesc->Count;
  Data  = CmObjDesc->Data;
  Size  = CmObjDesc->Size / Count;

  SingleCmObjDesc.ObjectId = CmObjDesc->ObjectId;
  SingleCmObjDesc.Count    = 1;
  SingleCmObjDesc.Size     = Size;

  for (Index = 0; Index < Count; Index++) {
    SingleCmObjDesc.Data = (VOID *)&Data[Index * Size];
    // Add the CmObj.
    Status = FdtParserHandle->HwInfoAdd (
                                FdtParserHandle,
                                FdtParserHandle->Context,
                                &SingleCmObjDesc,
                                (TokenTable != NULL) ?
                                &TokenTable[Index] :
                                NULL
                                );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  } // for

  return Status;
}

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
  )
{
  EFI_STATUS         Status;
  CM_OBJ_DESCRIPTOR  CmObjRef;
  CM_OBJECT_TOKEN    *TokenTable;
  INT32              TokenTableSize;

  if ((FdtParserHandle == NULL)             ||
      (FdtParserHandle->HwInfoAdd == NULL)  ||
      (CmObjDesc == NULL)                   ||
      (CmObjDesc->Count == 0)               ||
      (CmObjDesc->Data == NULL)             ||
      (CmObjDesc->Size == 0)                ||
      (Token == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Allocate a buffer to store the tokens.
  TokenTableSize = CmObjDesc->Count * sizeof (CM_OBJECT_TOKEN);
  TokenTable     = AllocateZeroPool (TokenTableSize);
  if (TokenTable == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  // Add the input CmObjs.
  Status = AddMultipleCmObj (
             FdtParserHandle,
             CmObjDesc,
             CmObjDesc->Count,
             TokenTable
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  CmObjRef.ObjectId = CREATE_CM_ARM_OBJECT_ID (EArmObjCmRef);
  CmObjRef.Data     = TokenTable;
  CmObjRef.Count    = CmObjDesc->Count;
  CmObjRef.Size     = TokenTableSize;

  // Add the array of EArmObjCmRef CmObjs.
  Status = FdtParserHandle->HwInfoAdd (
                              FdtParserHandle,
                              FdtParserHandle->Context,
                              &CmObjRef,
                              Token
                              );
  ASSERT_EFI_ERROR (Status);

exit_handler:
  FreePool (TokenTable);
  return Status;
}
