/** @file
The functions for Boot Maintainence Main menu.

Copyright (c) 2004 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootMaintenanceManager.h"
#include "BootMaintenanceManagerCustomizedUiSupport.h"

#define UI_HII_DRIVER_LIST_SIZE  0x8

typedef struct {
  EFI_STRING_ID    PromptId;
  EFI_STRING_ID    HelpId;
  EFI_STRING_ID    DevicePathId;
  EFI_GUID         FormSetGuid;
  BOOLEAN          EmptyLineAfter;
} UI_HII_DRIVER_INSTANCE;

STATIC UI_HII_DRIVER_INSTANCE  *gHiiDriverList;

/**
  Create the dynamic item to allow user to set the "BootNext" vaule.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateBootNextMenu (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  )
{
  BM_MENU_ENTRY    *NewMenuEntry;
  BM_LOAD_CONTEXT  *NewLoadContext;
  UINT16           Index;
  VOID             *OptionsOpCodeHandle;
  UINT32           BootNextIndex;

  if (BootOptionMenu.MenuNumber == 0) {
    return;
  }

  BootNextIndex = NONE_BOOTNEXT_VALUE;

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (Index = 0; (UINTN)Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry   = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    NewLoadContext = (BM_LOAD_CONTEXT *)NewMenuEntry->VariableContext;

    if (NewLoadContext->IsBootNext) {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        NewMenuEntry->DisplayStringToken,
        EFI_IFR_OPTION_DEFAULT,
        EFI_IFR_TYPE_NUM_SIZE_32,
        Index
        );
      BootNextIndex = Index;
    } else {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        NewMenuEntry->DisplayStringToken,
        0,
        EFI_IFR_TYPE_NUM_SIZE_32,
        Index
        );
    }
  }

  if (BootNextIndex == NONE_BOOTNEXT_VALUE) {
    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      STRING_TOKEN (STR_NONE),
      EFI_IFR_OPTION_DEFAULT,
      EFI_IFR_TYPE_NUM_SIZE_32,
      NONE_BOOTNEXT_VALUE
      );
  } else {
    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      STRING_TOKEN (STR_NONE),
      0,
      EFI_IFR_TYPE_NUM_SIZE_32,
      NONE_BOOTNEXT_VALUE
      );
  }

  HiiCreateOneOfOpCode (
    StartOpCodeHandle,
    (EFI_QUESTION_ID)BOOT_NEXT_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    BOOT_NEXT_VAR_OFFSET,
    STRING_TOKEN (STR_BOOT_NEXT),
    STRING_TOKEN (STR_BOOT_NEXT_HELP),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    OptionsOpCodeHandle,
    NULL
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);
}

/**
  Create Time Out Menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateTimeOutMenu (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  )
{
  HiiCreateNumericOpCode (
    StartOpCodeHandle,
    (EFI_QUESTION_ID)FORM_TIME_OUT_ID,
    VARSTORE_ID_BOOT_MAINT,
    BOOT_TIME_OUT_VAR_OFFSET,
    STRING_TOKEN (STR_NUM_AUTO_BOOT),
    STRING_TOKEN (STR_HLP_AUTO_BOOT),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_2 | EFI_IFR_DISPLAY_UINT_DEC,
    0,
    65535,
    0,
    NULL
    );
}

/**
  Create Boot Option menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateBootOptionMenu (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  )
{
  HiiCreateGotoOpCode (
    StartOpCodeHandle,
    FORM_BOOT_SETUP_ID,
    STRING_TOKEN (STR_FORM_BOOT_SETUP_TITLE),
    STRING_TOKEN (STR_FORM_BOOT_SETUP_HELP),
    EFI_IFR_FLAG_CALLBACK,
    FORM_BOOT_SETUP_ID
    );
}

/**
  Create Driver Option menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateDriverOptionMenu (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  )
{
  HiiCreateGotoOpCode (
    StartOpCodeHandle,
    FORM_DRIVER_SETUP_ID,
    STRING_TOKEN (STR_FORM_DRIVER_SETUP_TITLE),
    STRING_TOKEN (STR_FORM_DRIVER_SETUP_HELP),
    EFI_IFR_FLAG_CALLBACK,
    FORM_DRIVER_SETUP_ID
    );
}

/**
  Create Com Option menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateComOptionMenu (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  )
{
  HiiCreateGotoOpCode (
    StartOpCodeHandle,
    FORM_CON_MAIN_ID,
    STRING_TOKEN (STR_FORM_CON_MAIN_TITLE),
    STRING_TOKEN (STR_FORM_CON_MAIN_HELP),
    EFI_IFR_FLAG_CALLBACK,
    FORM_CON_MAIN_ID
    );
}

/**
  Create Com Option menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateBootFromFileMenu (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  )
{
  HiiCreateGotoOpCode (
    StartOpCodeHandle,
    FORM_MAIN_ID,
    STRING_TOKEN (STR_BOOT_FROM_FILE),
    STRING_TOKEN (STR_BOOT_FROM_FILE_HELP),
    EFI_IFR_FLAG_CALLBACK,
    KEY_VALUE_BOOT_FROM_FILE
    );
}

/**
  Create empty line menu in the front page.

  @param    HiiHandle           The hii handle for the Uiapp driver.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateEmptyLine (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  )
{
  HiiCreateSubTitleOpCode (StartOpCodeHandle, STRING_TOKEN (STR_NULL_STRING), 0, 0, 0);
}

/**
  Extract device path for given HII handle and class guid.

  @param Handle          The HII handle.

  @retval  NULL          Fail to get the device path string.
  @return  PathString    Get the device path string.

**/
CHAR16 *
ExtractDevicePathFromHandle (
  IN      EFI_HII_HANDLE  Handle
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DriverHandle;

  ASSERT (Handle != NULL);

  if (Handle == NULL) {
    return NULL;
  }

  Status = gHiiDatabase->GetPackageListHandle (gHiiDatabase, Handle, &DriverHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return ConvertDevicePathToText (DevicePathFromHandle (DriverHandle), FALSE, FALSE);
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
IsRequiredDriver (
  IN  EFI_HII_HANDLE  HiiHandle,
  IN  EFI_GUID        *Guid,
  OUT EFI_STRING_ID   *PromptId,
  OUT EFI_STRING_ID   *HelpId,
  OUT VOID            *FormsetGuid
  )
{
  EFI_STATUS        Status;
  UINT8             ClassGuidNum;
  EFI_GUID          *ClassGuid;
  EFI_IFR_FORM_SET  *Buffer;
  UINTN             BufferSize;
  UINT8             *Ptr;
  UINTN             TempSize;
  BOOLEAN           RetVal;

  Buffer = NULL;

  Status = HiiGetFormSetFromHiiHandle (HiiHandle, &Buffer, &BufferSize);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  RetVal   = FALSE;
  TempSize = 0;
  Ptr      = (UINT8 *)Buffer;
  while (TempSize < BufferSize) {
    TempSize += ((EFI_IFR_OP_HEADER *)Ptr)->Length;

    if (((EFI_IFR_OP_HEADER *)Ptr)->Length <= OFFSET_OF (EFI_IFR_FORM_SET, Flags)) {
      Ptr += ((EFI_IFR_OP_HEADER *)Ptr)->Length;
      continue;
    }

    ClassGuidNum = (UINT8)(((EFI_IFR_FORM_SET *)Ptr)->Flags & 0x3);
    ClassGuid    = (EFI_GUID *)(VOID *)(Ptr + sizeof (EFI_IFR_FORM_SET));
    while (ClassGuidNum-- > 0) {
      if (!CompareGuid (Guid, ClassGuid)) {
        ClassGuid++;
        continue;
      }

      *PromptId = ((EFI_IFR_FORM_SET *)Ptr)->FormSetTitle;
      *HelpId   = ((EFI_IFR_FORM_SET *)Ptr)->Help;
      CopyMem (FormsetGuid, &((EFI_IFR_FORM_SET *)Ptr)->Guid, sizeof (EFI_GUID));
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
  @param    SpecialHandlerFn    The pointer to the specail handler function, if any.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

  @retval   EFI_SUCCESS         Search the driver success

**/
EFI_STATUS
BmmListThirdPartyDrivers (
  IN EFI_HII_HANDLE          HiiHandle,
  IN EFI_GUID                *ClassGuid,
  IN DRIVER_SPECIAL_HANDLER  SpecialHandlerFn,
  IN VOID                    *StartOpCodeHandle
  )
{
  UINTN                   Index;
  EFI_STRING              String;
  EFI_STRING_ID           Token;
  EFI_STRING_ID           TokenHelp;
  EFI_HII_HANDLE          *HiiHandles;
  CHAR16                  *DevicePathStr;
  UINTN                   Count;
  UINTN                   CurrentSize;
  UI_HII_DRIVER_INSTANCE  *DriverListPtr;
  EFI_STRING              NewName;
  BOOLEAN                 EmptyLineAfter;

  if (gHiiDriverList != NULL) {
    FreePool (gHiiDriverList);
  }

  HiiHandles = HiiGetHiiHandles (NULL);
  ASSERT (HiiHandles != NULL);
  if (HiiHandles != NULL) {
    gHiiDriverList = AllocateZeroPool (UI_HII_DRIVER_LIST_SIZE * sizeof (UI_HII_DRIVER_INSTANCE));
    ASSERT (gHiiDriverList != NULL);
    if (gHiiDriverList != NULL) {
      DriverListPtr = gHiiDriverList;
      CurrentSize   = UI_HII_DRIVER_LIST_SIZE;

      for (Index = 0, Count = 0; HiiHandles[Index] != NULL; Index++) {
        if (!IsRequiredDriver (HiiHandles[Index], ClassGuid, &Token, &TokenHelp, &gHiiDriverList[Count].FormSetGuid)) {
          continue;
        }

        String = HiiGetString (HiiHandles[Index], Token, NULL);
        if (String == NULL) {
          String = HiiGetString (HiiHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
          ASSERT (String != NULL);
        } else if (SpecialHandlerFn != NULL) {
          //
          // Check whether need to rename the driver name.
          //
          EmptyLineAfter = FALSE;
          if (SpecialHandlerFn (String, &NewName, &EmptyLineAfter)) {
            FreePool (String);
            String                              = NewName;
            DriverListPtr[Count].EmptyLineAfter = EmptyLineAfter;
          }
        }

        DriverListPtr[Count].PromptId = HiiSetString (HiiHandle, 0, String, NULL);
        if (String != NULL) {
          FreePool (String);
        }

        String = HiiGetString (HiiHandles[Index], TokenHelp, NULL);
        if (String == NULL) {
          String = HiiGetString (HiiHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
          ASSERT (String != NULL);
        }

        DriverListPtr[Count].HelpId = HiiSetString (HiiHandle, 0, String, NULL);
        if (String != NULL) {
          FreePool (String);
        }

        DevicePathStr = ExtractDevicePathFromHandle (HiiHandles[Index]);
        if (DevicePathStr != NULL) {
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
          if (DriverListPtr != NULL) {
            gHiiDriverList = DriverListPtr;
          }

          CurrentSize += UI_HII_DRIVER_LIST_SIZE;
        }
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
        0,
        0,
        &gHiiDriverList[Index].FormSetGuid,
        gHiiDriverList[Index].DevicePathId
        );

      if (gHiiDriverList[Index].EmptyLineAfter) {
        BmmCreateEmptyLine (HiiHandle, StartOpCodeHandle);
      }

      Index++;
    }
  }

  return EFI_SUCCESS;
}
