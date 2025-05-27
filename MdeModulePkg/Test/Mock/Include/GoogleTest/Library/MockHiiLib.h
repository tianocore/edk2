/** @file MockHiiLib.h
  Google Test mocks for HiiLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_HII_LIB_H_
#define MOCK_HII_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/HiiLib.h>
}

struct MockHiiLib {
  MOCK_INTERFACE_DECLARATION (MockHiiLib);

  MOCK_FUNCTION_DECLARATION (
    VOID,
    HiiRemovePackages,
    (IN EFI_HII_HANDLE  HiiHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STRING_ID,
    HiiSetString,
    (IN       EFI_HII_HANDLE  HiiHandle,
     IN       EFI_STRING_ID   StringId OPTIONAL,
     IN CONST EFI_STRING      String,
     IN CONST CHAR8           *SupportedLanguages OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STRING,
    HiiGetString,
    (IN       EFI_HII_HANDLE  HiiHandle,
     IN       EFI_STRING_ID   StringId,
     IN CONST CHAR8           *Language OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STRING,
    HiiGetStringEx,
    (IN       EFI_HII_HANDLE  HiiHandle,
     IN       EFI_STRING_ID   StringId,
     IN CONST CHAR8           *Language OPTIONAL,
     IN       BOOLEAN         TryBestLanguage)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STRING,
    HiiGetPackageString,
    (IN CONST EFI_GUID       *PackageListGuid,
     IN       EFI_STRING_ID  StringId,
     IN CONST CHAR8          *Language OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_HII_HANDLE *,
    HiiGetHiiHandles,
    (IN CONST EFI_GUID  *PackageListGuid OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    HiiGetFormSetFromHiiHandle,
    (IN  EFI_HII_HANDLE    Handle,
     OUT EFI_IFR_FORM_SET  **Buffer,
     OUT UINTN             *BufferSize)
    );

  MOCK_FUNCTION_DECLARATION (
    CHAR8 *,
    HiiGetSupportedLanguages,
    (IN EFI_HII_HANDLE  HiiHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STRING,
    HiiConstructConfigHdr,
    (IN CONST EFI_GUID    *Guid OPTIONAL,
     IN CONST CHAR16      *Name OPTIONAL,
     IN       EFI_HANDLE  DriverHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HiiSetToDefaults,
    (IN CONST EFI_STRING  Request OPTIONAL,
     IN       UINT16      DefaultId)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HiiValidateSettings,
    (IN CONST EFI_STRING  Request OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HiiIsConfigHdrMatch,
    (IN CONST EFI_STRING  ConfigHdr,
     IN CONST EFI_GUID    *Guid OPTIONAL,
     IN CONST CHAR16      *Name OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HiiGetBrowserData,
    (IN CONST EFI_GUID  *VariableGuid OPTIONAL,
     IN CONST CHAR16    *VariableName OPTIONAL,
     IN       UINTN     BufferSize,
     OUT      UINT8     *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    HiiSetBrowserData,
    (IN CONST EFI_GUID  *VariableGuid OPTIONAL,
     IN CONST CHAR16    *VariableName OPTIONAL,
     IN       UINTN     BufferSize,
     IN CONST UINT8     *Buffer,
     IN CONST CHAR16    *RequestElement OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    HiiAllocateOpCodeHandle,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    HiiFreeOpCodeHandle,
    (VOID  *OpCodeHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateRawOpCodes,
    (IN VOID   *OpCodeHandle,
     IN UINT8  *RawBuffer,
     IN UINTN  RawBufferSize)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateEndOpCode,
    (IN VOID  *OpCodeHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateOneOfOptionOpCode,
    (IN VOID    *OpCodeHandle,
     IN UINT16  StringId,
     IN UINT8   Flags,
     IN UINT8   Type,
     IN UINT64  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateDefaultOpCode,
    (IN VOID    *OpCodeHandle,
     IN UINT16  DefaultId,
     IN UINT8   Type,
     IN UINT64  Value)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateGuidOpCode,
    (IN       VOID      *OpCodeHandle,
     IN CONST EFI_GUID  *Guid,
     IN CONST VOID      *GuidOpCode OPTIONAL,
     IN       UINTN     OpCodeSize)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateActionOpCode,
    (IN VOID             *OpCodeHandle,
     IN EFI_QUESTION_ID  QuestionId,
     IN EFI_STRING_ID    Prompt,
     IN EFI_STRING_ID    Help,
     IN UINT8            QuestionFlags,
     IN EFI_STRING_ID    QuestionConfig)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateSubTitleOpCode,
    (IN VOID           *OpCodeHandle,
     IN EFI_STRING_ID  Prompt,
     IN EFI_STRING_ID  Help,
     IN UINT8          Flags,
     IN UINT8          Scope)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateGotoOpCode,
    (IN VOID             *OpCodeHandle,
     IN EFI_FORM_ID      FormId,
     IN EFI_STRING_ID    Prompt,
     IN EFI_STRING_ID    Help,
     IN UINT8            QuestionFlags,
     IN EFI_QUESTION_ID  QuestionId)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateGotoExOpCode,
    (IN VOID             *OpCodeHandle,
     IN EFI_FORM_ID      RefFormId,
     IN EFI_STRING_ID    Prompt,
     IN EFI_STRING_ID    Help,
     IN UINT8            QuestionFlags,
     IN EFI_QUESTION_ID  QuestionId,
     IN EFI_QUESTION_ID  RefQuestionId,
     IN EFI_GUID         *RefFormSetId OPTIONAL,
     IN EFI_STRING_ID    RefDevicePath)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateCheckBoxOpCode,
    (IN VOID             *OpCodeHandle,
     IN EFI_QUESTION_ID  QuestionId,
     IN EFI_VARSTORE_ID  VarStoreId,
     IN UINT16           VarOffset,
     IN EFI_STRING_ID    Prompt,
     IN EFI_STRING_ID    Help,
     IN UINT8            QuestionFlags,
     IN UINT8            CheckBoxFlags,
     IN VOID             *DefaultsOpCodeHandle OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateNumericOpCode,
    (IN VOID             *OpCodeHandle,
     IN EFI_QUESTION_ID  QuestionId,
     IN EFI_VARSTORE_ID  VarStoreId,
     IN UINT16           VarOffset,
     IN EFI_STRING_ID    Prompt,
     IN EFI_STRING_ID    Help,
     IN UINT8            QuestionFlags,
     IN UINT8            NumericFlags,
     IN UINT64           Minimum,
     IN UINT64           Maximum,
     IN UINT64           Step,
     IN VOID             *DefaultsOpCodeHandle OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateStringOpCode,
    (IN VOID             *OpCodeHandle,
     IN EFI_QUESTION_ID  QuestionId,
     IN EFI_VARSTORE_ID  VarStoreId,
     IN UINT16           VarOffset,
     IN EFI_STRING_ID    Prompt,
     IN EFI_STRING_ID    Help,
     IN UINT8            QuestionFlags,
     IN UINT8            StringFlags,
     IN UINT8            MinSize,
     IN UINT8            MaxSize,
     IN VOID             *DefaultsOpCodeHandle OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateOneOfOpCode,
    (IN VOID             *OpCodeHandle,
     IN EFI_QUESTION_ID  QuestionId,
     IN EFI_VARSTORE_ID  VarStoreId,
     IN UINT16           VarOffset,
     IN EFI_STRING_ID    Prompt,
     IN EFI_STRING_ID    Help,
     IN UINT8            QuestionFlags,
     IN UINT8            OneOfFlags,
     IN VOID             *OptionsOpCodeHandle,
     IN VOID             *DefaultsOpCodeHandle OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateOrderedListOpCode,
    (IN VOID             *OpCodeHandle,
     IN EFI_QUESTION_ID  QuestionId,
     IN EFI_VARSTORE_ID  VarStoreId,
     IN UINT16           VarOffset,
     IN EFI_STRING_ID    Prompt,
     IN EFI_STRING_ID    Help,
     IN UINT8            QuestionFlags,
     IN UINT8            OrderedListFlags,
     IN UINT8            DataType,
     IN UINT8            MaxContainers,
     IN VOID             *OptionsOpCodeHandle,
     IN VOID             *DefaultsOpCodeHandle OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateTextOpCode,
    (IN VOID           *OpCodeHandle,
     IN EFI_STRING_ID  Prompt,
     IN EFI_STRING_ID  Help,
     IN EFI_STRING_ID  TextTwo)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateDateOpCode,
    (IN VOID             *OpCodeHandle,
     IN EFI_QUESTION_ID  QuestionId,
     IN EFI_VARSTORE_ID  VarStoreId OPTIONAL,
     IN UINT16           VarOffset OPTIONAL,
     IN EFI_STRING_ID    Prompt,
     IN EFI_STRING_ID    Help,
     IN UINT8            QuestionFlags,
     IN UINT8            DateFlags,
     IN VOID             *DefaultsOpCodeHandle OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8 *,
    HiiCreateTimeOpCode,
    (IN VOID             *OpCodeHandle,
     IN EFI_QUESTION_ID  QuestionId,
     IN EFI_VARSTORE_ID  VarStoreId OPTIONAL,
     IN UINT16           VarOffset OPTIONAL,
     IN EFI_STRING_ID    Prompt,
     IN EFI_STRING_ID    Help,
     IN UINT8            QuestionFlags,
     IN UINT8            TimeFlags,
     IN VOID             *DefaultsOpCodeHandle OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    HiiUpdateForm,
    (IN EFI_HII_HANDLE  HiiHandle,
     IN EFI_GUID        *FormSetGuid OPTIONAL,
     IN EFI_FORM_ID     FormId,
     IN VOID            *StartOpCodeHandle,
     IN VOID            *EndOpCodeHandle OPTIONAL)
    );
};

#endif
