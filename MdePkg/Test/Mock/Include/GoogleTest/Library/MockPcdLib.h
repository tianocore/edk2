/** @file MockPcdLib.h
  Google Test mocks for PcdLib

  Copyright (c) 2024, Intel Corporation. All rights reserved.
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PCD_LIB_H_
#define MOCK_PCD_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/PcdLib.h>
}

struct MockPcdLib {
  MOCK_INTERFACE_DECLARATION (MockPcdLib);

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    LibPcdSetSku,
    (IN UINTN  SkuId)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    LibPcdGet8,
    (IN UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    LibPcdGet16,
    (IN UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    LibPcdGet32,
    (IN UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    LibPcdGet64,
    (IN UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    LibPcdGetPtr,
    (IN UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    LibPcdGetBool,
    (IN UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    LibPcdGetSize,
    (IN UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    LibPcdGetEx8,
    (IN CONST GUID   *Guid,
     IN       UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    LibPcdGetEx16,
    (IN CONST GUID   *Guid,
     IN       UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT32,
    LibPcdGetEx32,
    (IN CONST GUID   *Guid,
     IN       UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    LibPcdGetEx64,
    (IN CONST GUID   *Guid,
     IN       UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    LibPcdGetExPtr,
    (IN CONST GUID   *Guid,
     IN       UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    LibPcdGetExBool,
    (IN CONST GUID   *Guid,
     IN       UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    LibPcdGetExSize,
    (IN CONST GUID   *Guid,
     IN       UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSet8S,
    (IN UINTN  TokenNumber,
     IN UINT8  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSet16S,
    (IN UINTN   TokenNumber,
     IN UINT16  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSet32S,
    (IN UINTN   TokenNumber,
     IN UINT32  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSet64S,
    (IN UINTN   TokenNumber,
     IN UINT64  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSetPtrS,
    (IN       UINTN  TokenNumber,
     IN OUT   UINTN  *SizeOfBuffer,
     IN CONST VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSetBoolS,
    (IN UINTN    TokenNumber,
     IN BOOLEAN  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSetEx8S,
    (IN CONST GUID   *Guid,
     IN       UINTN  TokenNumber,
     IN       UINT8  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSetEx16S,
    (IN CONST GUID    *Guid,
     IN       UINTN   TokenNumber,
     IN       UINT16  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSetEx32S,
    (IN CONST GUID    *Guid,
     IN       UINTN   TokenNumber,
     IN       UINT32  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSetEx64S,
    (IN CONST GUID    *Guid,
     IN       UINTN   TokenNumber,
     IN       UINT64  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSetExPtrS,
    (IN CONST GUID   *Guid,
     IN       UINTN  TokenNumber,
     IN OUT   UINTN  *SizeOfBuffer,
     IN       VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPcdSetExBoolS,
    (IN CONST GUID     *Guid,
     IN       UINTN    TokenNumber,
     IN       BOOLEAN  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    LibPcdCallbackOnSet,
    (IN CONST GUID          *Guid OPTIONAL,
     IN       UINTN         TokenNumber,
     IN       PCD_CALLBACK  NotificationFunction)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    LibPcdCancelCallback,
    (IN CONST GUID          *Guid OPTIONAL,
     IN       UINTN         TokenNumber,
     IN       PCD_CALLBACK  NotificationFunction)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    LibPcdGetNextToken,
    (IN CONST GUID   *Guid OPTIONAL,
     IN       UINTN  TokenNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    GUID *,
    LibPcdGetNextTokenSpace,
    (IN CONST GUID  *TokenSpaceGuid)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    LibPatchPcdSetPtr,
    (OUT      VOID   *PatchVariable,
     IN       UINTN  MaximumDatumSize,
     IN OUT   UINTN  *SizeOfBuffer,
     IN CONST VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPatchPcdSetPtrS,
    (OUT      VOID   *PatchVariable,
     IN       UINTN  MaximumDatumSize,
     IN OUT   UINTN  *SizeOfBuffer,
     IN CONST VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    LibPatchPcdSetPtrAndSize,
    (OUT      VOID   *PatchVariable,
     OUT      UINTN  *SizeOfPatchVariable,
     IN       UINTN  MaximumDatumSize,
     IN OUT   UINTN  *SizeOfBuffer,
     IN CONST VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    LibPatchPcdSetPtrAndSizeS,
    (OUT      VOID   *PatchVariable,
     OUT      UINTN  *SizeOfPatchVariable,
     IN       UINTN  MaximumDatumSize,
     IN OUT   UINTN  *SizeOfBuffer,
     IN CONST VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    LibPcdGetInfo,
    (IN  UINTN     TokenNumber,
     OUT PCD_INFO  *PcdInfo)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    LibPcdGetInfoEx,
    (IN CONST GUID      *Guid,
     IN       UINTN     TokenNumber,
     OUT      PCD_INFO  *PcdInfo)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    LibPcdGetSku,
    ()
    );
};

#endif
