/** @file

  This library class defines a set of interfaces to customize Ui module

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/GlobalVariable.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiString.h>

#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "FrontPageCustomizedUiSupport.h"

//
// This is the VFR compiler generated header file which defines the
// string identifiers.
//
#define PRINTABLE_LANGUAGE_NAME_STRING_ID     0x0001

#define UI_HII_DRIVER_LIST_SIZE               0x8

#define FRONT_PAGE_KEY_CONTINUE               0x1000
#define FRONT_PAGE_KEY_RESET                  0x1001
#define FRONT_PAGE_KEY_LANGUAGE               0x1002
#define FRONT_PAGE_KEY_DRIVER                 0x2000

typedef struct {
  EFI_STRING_ID   PromptId;
  EFI_STRING_ID   HelpId;
  EFI_STRING_ID   DevicePathId;
  EFI_GUID        FormSetGuid;
  BOOLEAN         EmptyLineAfter;
} UI_HII_DRIVER_INSTANCE;

CHAR8                        *gLanguageString;
EFI_STRING_ID                *gLanguageToken;
UI_HII_DRIVER_INSTANCE       *gHiiDriverList;
extern EFI_HII_HANDLE        gStringPackHandle;
UINT8                        gCurrentLanguageIndex;


/**
  Get next language from language code list (with separator ';').

  If LangCode is NULL, then ASSERT.
  If Lang is NULL, then ASSERT.

  @param  LangCode    On input: point to first language in the list. On
                                 output: point to next language in the list, or
                                 NULL if no more language in the list.
  @param  Lang           The first language in the list.

**/
VOID
GetNextLanguage (
  IN OUT CHAR8      **LangCode,
  OUT CHAR8         *Lang
  )
{
  UINTN  Index;
  CHAR8  *StringPtr;

  ASSERT (LangCode != NULL);
  ASSERT (*LangCode != NULL);
  ASSERT (Lang != NULL);

  Index = 0;
  StringPtr = *LangCode;
  while (StringPtr[Index] != 0 && StringPtr[Index] != ';') {
    Index++;
  }

  CopyMem (Lang, StringPtr, Index);
  Lang[Index] = 0;

  if (StringPtr[Index] == ';') {
    Index++;
  }
  *LangCode = StringPtr + Index;
}

/**
  This function processes the language changes in configuration.

  @param Value           A pointer to the data being sent to the original exporting driver.


  @retval  TRUE          The callback successfully handled the action.
  @retval  FALSE         The callback not supported in this handler.

**/
EFI_STATUS
LanguageChangeHandler (
  IN  EFI_IFR_TYPE_VALUE                     *Value
  )
{
  CHAR8                         *LangCode;
  CHAR8                         *Lang;
  UINTN                         Index;
  EFI_STATUS                    Status;

  //
  // Allocate working buffer for RFC 4646 language in supported LanguageString.
  //
  Lang = AllocatePool (AsciiStrSize (gLanguageString));
  ASSERT (Lang != NULL);

  Index = 0;
  LangCode = gLanguageString;
  while (*LangCode != 0) {
    GetNextLanguage (&LangCode, Lang);

    if (Index == Value->u8) {
      gCurrentLanguageIndex = Value->u8;
      break;
    }

    Index++;
  }

  if (Index == Value->u8) {
    Status = gRT->SetVariable (
                    L"PlatformLang",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrSize (Lang),
                    Lang
                    );
    if (EFI_ERROR (Status)) {
      FreePool (Lang);
      return EFI_DEVICE_ERROR;
    }
  } else {
    ASSERT (FALSE);
  }
  FreePool (Lang);

  return EFI_SUCCESS;
}

/**
  This function processes the results of changes in configuration.


  @param HiiHandle       Points to the hii handle for this formset.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.
  @param Status          Return the handle status.

  @retval  TRUE          The callback successfully handled the action.
  @retval  FALSE         The callback not supported in this handler.

**/
BOOLEAN
UiSupportLibCallbackHandler (
  IN  EFI_HII_HANDLE                         HiiHandle,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest,
  OUT EFI_STATUS                             *Status
  )
{
  if (QuestionId != FRONT_PAGE_KEY_CONTINUE &&
      QuestionId != FRONT_PAGE_KEY_RESET &&
      QuestionId != FRONT_PAGE_KEY_LANGUAGE) {
    return FALSE;
  }

  if (Action == EFI_BROWSER_ACTION_RETRIEVE) {
    if (QuestionId == FRONT_PAGE_KEY_LANGUAGE) {
      Value->u8 = gCurrentLanguageIndex;
      *Status = EFI_SUCCESS;
    } else {
      *Status = EFI_UNSUPPORTED;
    }
    return TRUE;
  }

  if (Action != EFI_BROWSER_ACTION_CHANGED) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed.
    //
    *Status = EFI_UNSUPPORTED;
    return TRUE;
  }

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((Value == NULL) || (ActionRequest == NULL)) {
      *Status = EFI_INVALID_PARAMETER;
      return TRUE;
    }

    *Status = EFI_SUCCESS;
    switch (QuestionId) {
    case FRONT_PAGE_KEY_CONTINUE:
      //
      // This is the continue - clear the screen and return an error to get out of FrontPage loop
      //
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
      break;

    case FRONT_PAGE_KEY_LANGUAGE:
      *Status = LanguageChangeHandler(Value);
      break;

    case FRONT_PAGE_KEY_RESET:
      //
      // Reset
      //
      gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
      *Status = EFI_UNSUPPORTED;

    default:
      break;
    }
  }

  return TRUE;
}

/**
  Create Select language menu in the front page with oneof opcode.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
UiCreateLanguageMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  )
{
  CHAR8                       *LangCode;
  CHAR8                       *Lang;
  UINTN                       LangSize;
  CHAR8                       *CurrentLang;
  UINTN                       OptionCount;
  CHAR16                      *StringBuffer;
  VOID                        *OptionsOpCodeHandle;
  UINTN                       StringSize;
  EFI_STATUS                  Status;
  EFI_HII_STRING_PROTOCOL     *HiiString;

  Lang         = NULL;
  StringBuffer = NULL;

  //
  // Init OpCode Handle and Allocate space for creation of UpdateData Buffer
  //
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  GetEfiGlobalVariable2 (L"PlatformLang", (VOID**)&CurrentLang, NULL);

  //
  // Get Support language list from variable.
  //
  GetEfiGlobalVariable2 (L"PlatformLangCodes", (VOID**)&gLanguageString, NULL);
  if (gLanguageString == NULL) {
    gLanguageString = AllocateCopyPool (
                               AsciiStrSize ((CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes)),
                               (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes)
                               );
    ASSERT (gLanguageString != NULL);
  }

  if (gLanguageToken == NULL) {
    //
    // Count the language list number.
    //
    LangCode = gLanguageString;
    Lang = AllocatePool (AsciiStrSize (gLanguageString));
    ASSERT (Lang != NULL);

    OptionCount = 0;
    while (*LangCode != 0) {
      GetNextLanguage (&LangCode, Lang);
      OptionCount ++;
    }

    //
    // Allocate extra 1 as the end tag.
    //
    gLanguageToken = AllocateZeroPool ((OptionCount + 1) * sizeof (EFI_STRING_ID));
    ASSERT (gLanguageToken != NULL);

    Status = gBS->LocateProtocol (&gEfiHiiStringProtocolGuid, NULL, (VOID **) &HiiString);
    ASSERT_EFI_ERROR (Status);

    LangCode     = gLanguageString;
    OptionCount  = 0;
    while (*LangCode != 0) {
      GetNextLanguage (&LangCode, Lang);

      StringSize = 0;
      Status = HiiString->GetString (HiiString, Lang, HiiHandle, PRINTABLE_LANGUAGE_NAME_STRING_ID, StringBuffer, &StringSize, NULL);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        StringBuffer = AllocateZeroPool (StringSize);
        ASSERT (StringBuffer != NULL);
        Status = HiiString->GetString (HiiString, Lang, HiiHandle, PRINTABLE_LANGUAGE_NAME_STRING_ID, StringBuffer, &StringSize, NULL);
        ASSERT_EFI_ERROR (Status);
      }

      if (EFI_ERROR (Status)) {
        LangSize = AsciiStrSize (Lang);
        StringBuffer = AllocatePool (LangSize * sizeof (CHAR16));
        ASSERT (StringBuffer != NULL);
        AsciiStrToUnicodeStrS (Lang, StringBuffer, LangSize);
      }

      ASSERT (StringBuffer != NULL);
      gLanguageToken[OptionCount] = HiiSetString (HiiHandle, 0, StringBuffer, NULL);
      FreePool (StringBuffer);

      OptionCount++;
    }
  }

  ASSERT (gLanguageToken != NULL);
  LangCode = gLanguageString;
  OptionCount = 0;
  if (Lang == NULL) {
    Lang = AllocatePool (AsciiStrSize (gLanguageString));
    ASSERT (Lang != NULL);
  }
  while (*LangCode != 0) {
    GetNextLanguage (&LangCode, Lang);

    if (CurrentLang != NULL && AsciiStrCmp (Lang, CurrentLang) == 0) {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        gLanguageToken[OptionCount],
        EFI_IFR_OPTION_DEFAULT,
        EFI_IFR_NUMERIC_SIZE_1,
        (UINT8) OptionCount
        );
      gCurrentLanguageIndex = (UINT8) OptionCount;
    } else {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        gLanguageToken[OptionCount],
        0,
        EFI_IFR_NUMERIC_SIZE_1,
        (UINT8) OptionCount
        );
    }

    OptionCount++;
  }

  if (CurrentLang != NULL) {
    FreePool (CurrentLang);
  }
  FreePool (Lang);

  HiiCreateOneOfOpCode (
    StartOpCodeHandle,
    FRONT_PAGE_KEY_LANGUAGE,
    0,
    0,
    STRING_TOKEN (STR_LANGUAGE_SELECT),
    STRING_TOKEN (STR_LANGUAGE_SELECT_HELP),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_1,
    OptionsOpCodeHandle,
    NULL
    );
}

/**
  Create continue menu in the front page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
UiCreateContinueMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  )
{
  HiiCreateActionOpCode (
    StartOpCodeHandle,
    FRONT_PAGE_KEY_CONTINUE,
    STRING_TOKEN (STR_CONTINUE_PROMPT),
    STRING_TOKEN (STR_CONTINUE_PROMPT),
    EFI_IFR_FLAG_CALLBACK,
    0
    );
}

/**
  Create empty line menu in the front page.

  @param    HiiHandle           The hii handle for the Uiapp driver.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
UiCreateEmptyLine (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  )
{
  HiiCreateSubTitleOpCode (StartOpCodeHandle, STRING_TOKEN (STR_NULL_STRING), 0, 0, 0);
}

/**
  Create Reset menu in the front page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
UiCreateResetMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  )
{
  HiiCreateActionOpCode (
    StartOpCodeHandle,
    FRONT_PAGE_KEY_RESET,
    STRING_TOKEN (STR_RESET_STRING),
    STRING_TOKEN (STR_RESET_STRING),
    EFI_IFR_FLAG_CALLBACK,
    0
    );
}

/**
  Extract device path for given HII handle and class guid.

  @param Handle          The HII handle.

  @retval  NULL          Fail to get the device path string.
  @return  PathString    Get the device path string.

**/
CHAR16 *
ExtractDevicePathFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle
  )
{
  EFI_STATUS                       Status;
  EFI_HANDLE                       DriverHandle;

  ASSERT (Handle != NULL);

  if (Handle == NULL) {
    return NULL;
  }

  Status = gHiiDatabase->GetPackageListHandle (gHiiDatabase, Handle, &DriverHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return ConvertDevicePathToText(DevicePathFromHandle (DriverHandle), FALSE, FALSE);
}

/**
  Check whether this driver need to be shown in the front page.

  @param    HiiHandle           The hii handle for the driver.
  @param    Guid                The special guid for the driver which is the target.
  @param    PromptId            Return the prompt string id.
  @param    HelpId              Return the help string id.
  @param    FormsetGuid         Return the formset guid info.

  @retval   EFI_SUCCESS         Search the driver success

**/
BOOLEAN
RequiredDriver (
  IN  EFI_HII_HANDLE              HiiHandle,
  IN  EFI_GUID                    *Guid,
  OUT EFI_STRING_ID               *PromptId,
  OUT EFI_STRING_ID               *HelpId,
  OUT VOID                        *FormsetGuid
  )
{
  EFI_STATUS                  Status;
  UINT8                       ClassGuidNum;
  EFI_GUID                    *ClassGuid;
  EFI_IFR_FORM_SET            *Buffer;
  UINTN                       BufferSize;
  UINT8                       *Ptr;
  UINTN                       TempSize;
  BOOLEAN                     RetVal;

  Status = HiiGetFormSetFromHiiHandle(HiiHandle, &Buffer,&BufferSize);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  RetVal = FALSE;
  TempSize = 0;
  Ptr = (UINT8 *) Buffer;
  while(TempSize < BufferSize)  {
    TempSize += ((EFI_IFR_OP_HEADER *) Ptr)->Length;

    if (((EFI_IFR_OP_HEADER *) Ptr)->Length <= OFFSET_OF (EFI_IFR_FORM_SET, Flags)){
      Ptr += ((EFI_IFR_OP_HEADER *) Ptr)->Length;
      continue;
    }

    ClassGuidNum = (UINT8) (((EFI_IFR_FORM_SET *)Ptr)->Flags & 0x3);
    ClassGuid = (EFI_GUID *) (VOID *)(Ptr + sizeof (EFI_IFR_FORM_SET));
    while (ClassGuidNum-- > 0) {
      if (!CompareGuid (Guid, ClassGuid)){
        ClassGuid ++;
        continue;
      }

      *PromptId = ((EFI_IFR_FORM_SET *)Ptr)->FormSetTitle;
      *HelpId = ((EFI_IFR_FORM_SET *)Ptr)->Help;
      CopyMem (FormsetGuid, &((EFI_IFR_FORM_SET *) Ptr)->Guid, sizeof (EFI_GUID));
      RetVal = TRUE;
    }
  }

  FreePool (Buffer);

  return RetVal;
}

/**
  Search the drivers in the system which need to show in the front page
  and insert the menu to the front page.

  @param    HiiHandle           The hii handle for the Uiapp driver.
  @param    ClassGuid           The class guid for the driver which is the target.
  @param    SpecialHandlerFn    The pointer to the special handler function, if any.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

  @retval   EFI_SUCCESS         Search the driver success

**/
EFI_STATUS
UiListThirdPartyDrivers (
  IN EFI_HII_HANDLE              HiiHandle,
  IN EFI_GUID                    *ClassGuid,
  IN DRIVER_SPECIAL_HANDLER      SpecialHandlerFn,
  IN VOID                        *StartOpCodeHandle
  )
{
  UINTN                       Index;
  EFI_STRING                  String;
  EFI_STRING_ID               Token;
  EFI_STRING_ID               TokenHelp;
  EFI_HII_HANDLE              *HiiHandles;
  CHAR16                      *DevicePathStr;
  UINTN                       Count;
  UINTN                       CurrentSize;
  UI_HII_DRIVER_INSTANCE      *DriverListPtr;
  EFI_STRING                  NewName;
  BOOLEAN                     EmptyLineAfter;

  if (gHiiDriverList != NULL) {
    FreePool (gHiiDriverList);
  }

  HiiHandles = HiiGetHiiHandles (NULL);
  ASSERT (HiiHandles != NULL);

  gHiiDriverList = AllocateZeroPool (UI_HII_DRIVER_LIST_SIZE * sizeof (UI_HII_DRIVER_INSTANCE));
  ASSERT (gHiiDriverList != NULL);
  DriverListPtr = gHiiDriverList;
  CurrentSize = UI_HII_DRIVER_LIST_SIZE;

  for (Index = 0, Count = 0; HiiHandles[Index] != NULL; Index++) {
    if (!RequiredDriver (HiiHandles[Index], ClassGuid, &Token, &TokenHelp, &gHiiDriverList[Count].FormSetGuid)) {
      continue;
    }

    String = HiiGetString (HiiHandles[Index], Token, NULL);
    if (String == NULL) {
      String = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
      ASSERT (String != NULL);
    } else if (SpecialHandlerFn != NULL) {
      //
      // Check whether need to rename the driver name.
      //
      EmptyLineAfter = FALSE;
      if (SpecialHandlerFn (String, &NewName, &EmptyLineAfter)) {
        FreePool (String);
        String = NewName;
        DriverListPtr[Count].EmptyLineAfter = EmptyLineAfter;
      }
    }
    DriverListPtr[Count].PromptId = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    String = HiiGetString (HiiHandles[Index], TokenHelp, NULL);
    if (String == NULL) {
      String = HiiGetString (gStringPackHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
      ASSERT (String != NULL);
    }
    DriverListPtr[Count].HelpId = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    DevicePathStr = ExtractDevicePathFromHiiHandle(HiiHandles[Index]);
    if (DevicePathStr != NULL){
      DriverListPtr[Count].DevicePathId = HiiSetString (HiiHandle, 0, DevicePathStr, NULL);
      FreePool (DevicePathStr);
    } else {
      DriverListPtr[Count].DevicePathId = 0;
    }

    Count++;
    if (Count >= CurrentSize) {
      DriverListPtr = ReallocatePool (
                        CurrentSize * sizeof (UI_HII_DRIVER_INSTANCE),
                        (Count + UI_HII_DRIVER_LIST_SIZE)
                          * sizeof (UI_HII_DRIVER_INSTANCE),
                        gHiiDriverList
                        );
      ASSERT (DriverListPtr != NULL);
      gHiiDriverList = DriverListPtr;
      CurrentSize += UI_HII_DRIVER_LIST_SIZE;
    }
  }

  FreePool (HiiHandles);

  Index = 0;
  while (gHiiDriverList[Index].PromptId != 0) {
    HiiCreateGotoExOpCode (
      StartOpCodeHandle,
      0,
      gHiiDriverList[Index].PromptId,
      gHiiDriverList[Index].HelpId,
      0,
      (EFI_QUESTION_ID) (Index + FRONT_PAGE_KEY_DRIVER),
      0,
      &gHiiDriverList[Index].FormSetGuid,
      gHiiDriverList[Index].DevicePathId
    );

    if (gHiiDriverList[Index].EmptyLineAfter) {
      UiCreateEmptyLine (HiiHandle, StartOpCodeHandle);
    }

    Index ++;
  }

  return EFI_SUCCESS;
}
