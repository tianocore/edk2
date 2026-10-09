/** @file
  Legacy Boot Maintenance UI implementation.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LegacyBootMaintUi.h"

LEGACY_BOOT_OPTION_CALLBACK_DATA  *mLegacyBootOptionPrivate = NULL;
EFI_GUID                          mLegacyBootOptionGuid     = LEGACY_BOOT_OPTION_FORMSET_GUID;
CHAR16                            mLegacyBootStorageName[]  = L"LegacyBootData";
BBS_TYPE                          mBbsType[]                = { BBS_FLOPPY, BBS_HARDDISK, BBS_CDROM, BBS_EMBED_NETWORK, BBS_BEV_DEVICE, BBS_UNKNOWN };
BOOLEAN                           mFirstEnterLegacyForm     = FALSE;

///
/// Legacy FD Info from LegacyBios.GetBbsInfo()
///
LEGACY_MENU_OPTION  LegacyFDMenu = {
  LEGACY_MENU_OPTION_SIGNATURE,
  { NULL },
  0
};

///
/// Legacy HD Info from LegacyBios.GetBbsInfo()
///
LEGACY_MENU_OPTION  LegacyHDMenu = {
  LEGACY_MENU_OPTION_SIGNATURE,
  { NULL },
  0
};

///
/// Legacy CD Info from LegacyBios.GetBbsInfo()
///
LEGACY_MENU_OPTION  LegacyCDMenu = {
  LEGACY_MENU_OPTION_SIGNATURE,
  { NULL },
  0
};

///
/// Legacy NET Info from LegacyBios.GetBbsInfo()
///
LEGACY_MENU_OPTION  LegacyNETMenu = {
  LEGACY_MENU_OPTION_SIGNATURE,
  { NULL },
  0
};

///
/// Legacy NET Info from LegacyBios.GetBbsInfo()
///
LEGACY_MENU_OPTION  LegacyBEVMenu = {
  LEGACY_MENU_OPTION_SIGNATURE,
  { NULL },
  0
};

VOID                *mLegacyStartOpCodeHandle = NULL;
VOID                *mLegacyEndOpCodeHandle   = NULL;
EFI_IFR_GUID_LABEL  *mLegacyStartLabel        = NULL;
EFI_IFR_GUID_LABEL  *mLegacyEndLabel          = NULL;

HII_VENDOR_DEVICE_PATH  mLegacyBootOptionHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    { 0x6bc75598, 0x89b4, 0x483d, { 0x91, 0x60, 0x7f, 0x46, 0x9a, 0x96, 0x35, 0x31 }
    }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

/**

  Build the LegacyFDMenu LegacyHDMenu LegacyCDMenu according to LegacyBios.GetBbsInfo().

**/
VOID
GetLegacyOptions (
  VOID
  );

/**

  Base on the L"LegacyDevOrder" variable to build the current order data.

**/
VOID
GetLegacyOptionsOrder (
  VOID
  );

/**
  Re-order the Boot Option according to the DevOrder.

  The routine re-orders the Boot Option in BootOption array according to
  the order specified by DevOrder.

  @param DevOrder           Pointer to buffer containing the BBS Index,
                            high 8-bit value 0xFF indicating a disabled boot option
  @param DevOrderCount      Count of the BBS Index
  @param EnBootOption       Callee allocated buffer containing the enabled Boot Option Numbers
  @param EnBootOptionCount  Count of the enabled Boot Option Numbers
  @param DisBootOption      Callee allocated buffer containing the disabled Boot Option Numbers
  @param DisBootOptionCount Count of the disabled Boot Option Numbers

  @return EFI_SUCCESS       The function completed successfully.
  @retval other             Contain some error, details see  the status return by gRT->SetVariable.
**/
EFI_STATUS
OrderLegacyBootOption4SameType (
  UINT16  *DevOrder,
  UINTN   DevOrderCount,
  UINT16  **EnBootOption,
  UINTN   *EnBootOptionCount,
  UINT16  **DisBootOption,
  UINTN   *DisBootOptionCount
  )
{
  EFI_STATUS  Status;
  UINT16      *NewBootOption;
  UINT16      *BootOrder;
  UINTN       BootOrderSize;
  UINTN       Index;
  UINTN       StartPosition;

  EFI_BOOT_MANAGER_LOAD_OPTION  BootOption;

  CHAR16  OptionName[sizeof ("Boot####")];
  UINT16  *BbsIndexArray;
  UINT16  *DeviceTypeArray;

  GetEfiGlobalVariable2 (L"BootOrder", (VOID **)&BootOrder, &BootOrderSize);
  ASSERT (BootOrder != NULL);

  BbsIndexArray       = AllocatePool (BootOrderSize);
  DeviceTypeArray     = AllocatePool (BootOrderSize);
  *EnBootOption       = AllocatePool (BootOrderSize);
  *DisBootOption      = AllocatePool (BootOrderSize);
  *DisBootOptionCount = 0;
  *EnBootOptionCount  = 0;
  Index               = 0;
  Status              = EFI_SUCCESS;

  ASSERT (BbsIndexArray != NULL);
  ASSERT (DeviceTypeArray != NULL);
  ASSERT (*EnBootOption != NULL);
  ASSERT (*DisBootOption != NULL);

  for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
    UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", BootOrder[Index]);
    Status = EfiBootManagerVariableToLoadOption (OptionName, &BootOption);
    ASSERT_EFI_ERROR (Status);

    if ((DevicePathType (BootOption.FilePath) == BBS_DEVICE_PATH) &&
        (DevicePathSubType (BootOption.FilePath) == BBS_BBS_DP))
    {
      //
      // Legacy Boot Option
      //
      ASSERT (BootOption.OptionalDataSize == sizeof (LEGACY_BOOT_OPTION_BBS_DATA));

      DeviceTypeArray[Index] = ((BBS_BBS_DEVICE_PATH *)BootOption.FilePath)->DeviceType;
      BbsIndexArray[Index]   = ((LEGACY_BOOT_OPTION_BBS_DATA *)BootOption.OptionalData)->BbsIndex;
    } else {
      DeviceTypeArray[Index] = BBS_TYPE_UNKNOWN;
      BbsIndexArray[Index]   = 0xFFFF;
    }

    EfiBootManagerFreeLoadOption (&BootOption);
  }

  //
  // Record the corresponding Boot Option Numbers according to the DevOrder
  // Record the EnBootOption and DisBootOption according to the DevOrder
  //
  StartPosition = BootOrderSize / sizeof (UINT16);
  NewBootOption = AllocatePool (DevOrderCount * sizeof (UINT16));
  ASSERT (NewBootOption != NULL);
  while (DevOrderCount-- != 0) {
    for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
      if (BbsIndexArray[Index] == (DevOrder[DevOrderCount] & 0xFF)) {
        StartPosition                = MIN (StartPosition, Index);
        NewBootOption[DevOrderCount] = BootOrder[Index];

        if ((DevOrder[DevOrderCount] & 0xFF00) == 0xFF00) {
          (*DisBootOption)[*DisBootOptionCount] = BootOrder[Index];
          (*DisBootOptionCount)++;
        } else {
          (*EnBootOption)[*EnBootOptionCount] = BootOrder[Index];
          (*EnBootOptionCount)++;
        }

        break;
      }
    }
  }

  //
  // Overwrite the old BootOption
  //
  CopyMem (&BootOrder[StartPosition], NewBootOption, (*DisBootOptionCount + *EnBootOptionCount) * sizeof (UINT16));
  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  BootOrderSize,
                  BootOrder
                  );

  FreePool (NewBootOption);
  FreePool (DeviceTypeArray);
  FreePool (BbsIndexArray);

  return Status;
}

/**
  Update the legacy BBS boot option. L"LegacyDevOrder" and gEfiLegacyDevOrderVariableGuid EFI Variable
  is updated with the new Legacy Boot order. The EFI Variable of "Boot####" and gEfiGlobalVariableGuid
  is also updated.

  @param NVMapData   The data for legacy BBS boot.

  @return EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         If L"LegacyDevOrder" and gEfiLegacyDevOrderVariableGuid EFI Variable can not be found.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate memory resource
  @retval other                 Contain some error, details see  the status return by gRT->SetVariable.
**/
EFI_STATUS
UpdateBBSOption (
  IN LEGACY_BOOT_NV_DATA  *NVMapData
  )
{
  UINTN                   Index;
  UINTN                   Index2;
  UINTN                   CurrentType;
  VOID                    *BootOptionVar;
  CHAR16                  VarName[100];
  UINTN                   OptionSize;
  EFI_STATUS              Status;
  UINT32                  *Attribute;
  LEGACY_MENU_OPTION      *OptionMenu;
  UINT16                  *LegacyDev;
  UINT16                  *InitialLegacyDev;
  UINT8                   *VarData;
  UINTN                   VarSize;
  LEGACY_DEV_ORDER_ENTRY  *DevOrder;
  UINT8                   *OriginalPtr;
  UINT8                   *DisMap;
  UINTN                   Pos;
  UINTN                   Bit;
  UINT16                  *NewOrder;
  UINT16                  Tmp;
  UINT16                  *EnBootOption;
  UINTN                   EnBootOptionCount;
  UINT16                  *DisBootOption;
  UINTN                   DisBootOptionCount;
  UINTN                   BufferSize;

  DisMap        = NULL;
  NewOrder      = NULL;
  CurrentType   = 0;
  EnBootOption  = NULL;
  DisBootOption = NULL;

  DisMap = mLegacyBootOptionPrivate->MaintainMapData->DisableMap;
  Status = EFI_SUCCESS;

  //
  // Update the Variable "LegacyDevOrder"
  //
  GetVariable2 (VAR_LEGACY_DEV_ORDER, &gEfiLegacyDevOrderVariableGuid, (VOID **)&VarData, &VarSize);
  if (VarData == NULL) {
    return EFI_NOT_FOUND;
  }

  OriginalPtr = VarData;

  while (mBbsType[CurrentType] != BBS_UNKNOWN) {
    switch (mBbsType[CurrentType]) {
      case BBS_FLOPPY:
        OptionMenu       = (LEGACY_MENU_OPTION *)&LegacyFDMenu;
        LegacyDev        = NVMapData->LegacyFD;
        InitialLegacyDev = mLegacyBootOptionPrivate->MaintainMapData->InitialNvData.LegacyFD;
        BufferSize       = sizeof (NVMapData->LegacyFD);
        break;

      case BBS_HARDDISK:
        OptionMenu       = (LEGACY_MENU_OPTION *)&LegacyHDMenu;
        LegacyDev        = NVMapData->LegacyHD;
        InitialLegacyDev = mLegacyBootOptionPrivate->MaintainMapData->InitialNvData.LegacyHD;

        BufferSize = sizeof (NVMapData->LegacyHD);
        break;

      case BBS_CDROM:
        OptionMenu       = (LEGACY_MENU_OPTION *)&LegacyCDMenu;
        LegacyDev        = NVMapData->LegacyCD;
        InitialLegacyDev = mLegacyBootOptionPrivate->MaintainMapData->InitialNvData.LegacyCD;
        BufferSize       = sizeof (NVMapData->LegacyCD);
        break;

      case BBS_EMBED_NETWORK:
        OptionMenu       = (LEGACY_MENU_OPTION *)&LegacyNETMenu;
        LegacyDev        = NVMapData->LegacyNET;
        InitialLegacyDev = mLegacyBootOptionPrivate->MaintainMapData->InitialNvData.LegacyNET;
        BufferSize       = sizeof (NVMapData->LegacyNET);
        break;

      default:
        ASSERT (mBbsType[CurrentType] == BBS_BEV_DEVICE);
        OptionMenu       = (LEGACY_MENU_OPTION *)&LegacyBEVMenu;
        LegacyDev        = NVMapData->LegacyBEV;
        InitialLegacyDev = mLegacyBootOptionPrivate->MaintainMapData->InitialNvData.LegacyBEV;
        BufferSize       = sizeof (NVMapData->LegacyBEV);
        break;
    }

    //
    // Check whether has value changed.
    //
    if (CompareMem (LegacyDev, InitialLegacyDev, BufferSize) == 0) {
      CurrentType++;
      continue;
    }

    DevOrder = (LEGACY_DEV_ORDER_ENTRY *)OriginalPtr;
    while (VarData < OriginalPtr + VarSize) {
      if (DevOrder->BbsType == mBbsType[CurrentType]) {
        break;
      }

      VarData += sizeof (BBS_TYPE) + DevOrder->Length;
      DevOrder = (LEGACY_DEV_ORDER_ENTRY *)VarData;
    }

    if (VarData >= OriginalPtr + VarSize) {
      FreePool (OriginalPtr);
      return EFI_NOT_FOUND;
    }

    NewOrder = AllocateZeroPool (DevOrder->Length - sizeof (DevOrder->Length));
    if (NewOrder == NULL) {
      FreePool (OriginalPtr);
      return EFI_OUT_OF_RESOURCES;
    }

    for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
      if (0xFF == LegacyDev[Index]) {
        break;
      }

      NewOrder[Index] = LegacyDev[Index];
    }

    //
    // Only the enable/disable state of each boot device with same device type can be changed,
    // so we can count on the index information in DevOrder.
    // DisMap bit array is the only reliable source to check a device's en/dis state,
    // so we use DisMap to set en/dis state of each item in NewOrder array
    //
    for (Index2 = 0; Index2 < OptionMenu->MenuNumber; Index2++) {
      Tmp = (UINT16)(DevOrder->Data[Index2] & 0xFF);
      Pos = Tmp / 8;
      Bit = 7 - (Tmp % 8);
      if ((DisMap[Pos] & (1 << Bit)) != 0) {
        NewOrder[Index] = (UINT16)(0xFF00 | Tmp);
        Index++;
      }
    }

    CopyMem (
      DevOrder->Data,
      NewOrder,
      DevOrder->Length - sizeof (DevOrder->Length)
      );
    FreePool (NewOrder);

    //
    // Update BootOrder and Boot####.Attribute
    //
    // 1. Re-order the Option Number in BootOrder according to Legacy Dev Order
    //
    ASSERT (OptionMenu->MenuNumber == DevOrder->Length / sizeof (UINT16) - 1);

    Status = OrderLegacyBootOption4SameType (
               DevOrder->Data,
               DevOrder->Length / sizeof (UINT16) - 1,
               &EnBootOption,
               &EnBootOptionCount,
               &DisBootOption,
               &DisBootOptionCount
               );
    if (EFI_ERROR (Status)) {
      goto Fail;
    }

    //
    // 2. Deactivate the DisBootOption and activate the EnBootOption
    //
    for (Index = 0; Index < DisBootOptionCount; Index++) {
      UnicodeSPrint (VarName, sizeof (VarName), L"Boot%04x", DisBootOption[Index]);
      GetEfiGlobalVariable2 (VarName, (VOID **)&BootOptionVar, &OptionSize);
      if (BootOptionVar != NULL) {
        Attribute   = (UINT32 *)BootOptionVar;
        *Attribute &= ~LOAD_OPTION_ACTIVE;

        Status = gRT->SetVariable (
                        VarName,
                        &gEfiGlobalVariableGuid,
                        VAR_FLAG,
                        OptionSize,
                        BootOptionVar
                        );

        FreePool (BootOptionVar);
      }
    }

    for (Index = 0; Index < EnBootOptionCount; Index++) {
      UnicodeSPrint (VarName, sizeof (VarName), L"Boot%04x", EnBootOption[Index]);
      GetEfiGlobalVariable2 (VarName, (VOID **)&BootOptionVar, &OptionSize);
      if (BootOptionVar != NULL) {
        Attribute   = (UINT32 *)BootOptionVar;
        *Attribute |= LOAD_OPTION_ACTIVE;

        Status = gRT->SetVariable (
                        VarName,
                        &gEfiGlobalVariableGuid,
                        VAR_FLAG,
                        OptionSize,
                        BootOptionVar
                        );

        FreePool (BootOptionVar);
      }
    }

    FreePool (EnBootOption);
    FreePool (DisBootOption);

    CurrentType++;
  }

  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &gEfiLegacyDevOrderVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  VarSize,
                  OriginalPtr
                  );

Fail:
  if (EnBootOption != NULL) {
    FreePool (EnBootOption);
  }

  if (DisBootOption != NULL) {
    FreePool (DisBootOption);
  }

  FreePool (OriginalPtr);
  return Status;
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
LegacyBootOptionExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
LegacyBootOptionRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT       EFI_STRING                      *Progress
  )
{
  EFI_STATUS                       Status;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *ConfigRouting;
  LEGACY_BOOT_NV_DATA              *CurrentNVMapData;
  UINTN                            BufferSize;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  //
  // Check routing data in <ConfigHdr>.
  // Note: there is no name for Name/Value storage, only GUID will be checked
  //
  if (!HiiIsConfigHdrMatch (Configuration, &mLegacyBootOptionGuid, mLegacyBootStorageName)) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **)&ConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  CurrentNVMapData = &mLegacyBootOptionPrivate->MaintainMapData->CurrentNvData;
  Status           = ConfigRouting->ConfigToBlock (
                                      ConfigRouting,
                                      Configuration,
                                      (UINT8 *)CurrentNVMapData,
                                      &BufferSize,
                                      Progress
                                      );
  ASSERT_EFI_ERROR (Status);

  Status = UpdateBBSOption (CurrentNVMapData);

  return Status;
}

/**
  Refresh the global UpdateData structure.

**/
VOID
RefreshLegacyUpdateData (
  VOID
  )
{
  //
  // Free current updated date
  //
  if (mLegacyStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mLegacyStartOpCodeHandle);
  }

  if (mLegacyEndOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mLegacyEndOpCodeHandle);
  }

  //
  // Create new OpCode Handle
  //
  mLegacyStartOpCodeHandle = HiiAllocateOpCodeHandle ();
  mLegacyEndOpCodeHandle   = HiiAllocateOpCodeHandle ();

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mLegacyStartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                              mLegacyStartOpCodeHandle,
                                              &gEfiIfrTianoGuid,
                                              NULL,
                                              sizeof (EFI_IFR_GUID_LABEL)
                                              );
  mLegacyStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

  mLegacyStartLabel->Number = FORM_BOOT_LEGACY_DEVICE_ID;

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mLegacyEndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                            mLegacyEndOpCodeHandle,
                                            &gEfiIfrTianoGuid,
                                            NULL,
                                            sizeof (EFI_IFR_GUID_LABEL)
                                            );
  mLegacyEndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

  mLegacyEndLabel->Number = FORM_BOOT_LEGACY_LABEL_END;
}

/**
  Get the Menu Entry from the list in Menu Entry List.

  If MenuNumber is great or equal to the number of Menu
  Entry in the list, then ASSERT.

  @param MenuOption      The Menu Entry List to read the menu entry.
  @param MenuNumber      The index of Menu Entry.

  @return The Menu Entry.

**/
LEGACY_MENU_ENTRY *
GetMenuEntry (
  LEGACY_MENU_OPTION  *MenuOption,
  UINTN               MenuNumber
  )
{
  LEGACY_MENU_ENTRY  *NewMenuEntry;
  UINTN              Index;
  LIST_ENTRY         *List;

  ASSERT (MenuNumber < MenuOption->MenuNumber);

  List = MenuOption->Head.ForwardLink;
  for (Index = 0; Index < MenuNumber; Index++) {
    List = List->ForwardLink;
  }

  NewMenuEntry = CR (List, LEGACY_MENU_ENTRY, Link, LEGACY_MENU_ENTRY_SIGNATURE);

  return NewMenuEntry;
}

/**
  Create string tokens for a menu from its help strings and display strings

  @param HiiHandle          Hii Handle of the package to be updated.
  @param MenuOption         The Menu whose string tokens need to be created

**/
VOID
CreateLegacyMenuStringToken (
  IN EFI_HII_HANDLE      HiiHandle,
  IN LEGACY_MENU_OPTION  *MenuOption
  )
{
  LEGACY_MENU_ENTRY  *NewMenuEntry;
  UINTN              Index;

  for (Index = 0; Index < MenuOption->MenuNumber; Index++) {
    NewMenuEntry = GetMenuEntry (MenuOption, Index);

    NewMenuEntry->DisplayStringToken = HiiSetString (
                                         HiiHandle,
                                         0,
                                         NewMenuEntry->DisplayString,
                                         NULL
                                         );

    if (NULL == NewMenuEntry->HelpString) {
      NewMenuEntry->HelpStringToken = NewMenuEntry->DisplayStringToken;
    } else {
      NewMenuEntry->HelpStringToken = HiiSetString (
                                        HiiHandle,
                                        0,
                                        NewMenuEntry->HelpString,
                                        NULL
                                        );
    }
  }
}

/**
  Create a dynamic page so that Legacy Device boot order
  can be set for specified device type.

  @param UpdatePageId    The form ID. It also specifies the legacy device type.


**/
VOID
UpdateLegacyDeviceOrderPage (
  IN UINT16  UpdatePageId
  )
{
  LEGACY_MENU_OPTION  *OptionMenu;
  LEGACY_MENU_ENTRY   *NewMenuEntry;
  EFI_STRING_ID       StrRef;
  EFI_STRING_ID       StrRefHelp;
  UINT16              *Default;
  UINT16              Index;
  UINT16              Key;
  CHAR16              String[100];
  CHAR16              *TypeStr;
  CHAR16              *TypeStrHelp;
  CHAR16              *FormTitle;
  VOID                *OptionsOpCodeHandle;
  VOID                *DefaultOpCodeHandle;

  Key         = 0;
  StrRef      = 0;
  StrRefHelp  = 0;
  OptionMenu  = NULL;
  TypeStr     = NULL;
  TypeStrHelp = NULL;
  Default     = NULL;

  RefreshLegacyUpdateData ();

  //
  // Create oneof option list
  //
  switch (UpdatePageId) {
    case FORM_FLOPPY_BOOT_ID:
      OptionMenu  = (LEGACY_MENU_OPTION *)&LegacyFDMenu;
      Key         = (UINT16)LEGACY_FD_QUESTION_ID;
      TypeStr     = STR_FLOPPY;
      TypeStrHelp = STR_FLOPPY_HELP;
      FormTitle   = STR_FLOPPY_TITLE;
      Default     = mLegacyBootOptionPrivate->MaintainMapData->CurrentNvData.LegacyFD;
      break;

    case FORM_HARDDISK_BOOT_ID:
      OptionMenu  = (LEGACY_MENU_OPTION *)&LegacyHDMenu;
      Key         = (UINT16)LEGACY_HD_QUESTION_ID;
      TypeStr     = STR_HARDDISK;
      TypeStrHelp = STR_HARDDISK_HELP;
      FormTitle   = STR_HARDDISK_TITLE;
      Default     = mLegacyBootOptionPrivate->MaintainMapData->CurrentNvData.LegacyHD;
      break;

    case FORM_CDROM_BOOT_ID:
      OptionMenu  = (LEGACY_MENU_OPTION *)&LegacyCDMenu;
      Key         = (UINT16)LEGACY_CD_QUESTION_ID;
      TypeStr     = STR_CDROM;
      TypeStrHelp = STR_CDROM_HELP;
      FormTitle   = STR_CDROM_TITLE;
      Default     = mLegacyBootOptionPrivate->MaintainMapData->CurrentNvData.LegacyCD;
      break;

    case FORM_NET_BOOT_ID:
      OptionMenu  = (LEGACY_MENU_OPTION *)&LegacyNETMenu;
      Key         = (UINT16)LEGACY_NET_QUESTION_ID;
      TypeStr     = STR_NET;
      TypeStrHelp = STR_NET_HELP;
      FormTitle   = STR_NET_TITLE;
      Default     = mLegacyBootOptionPrivate->MaintainMapData->CurrentNvData.LegacyNET;
      break;

    case FORM_BEV_BOOT_ID:
      OptionMenu  = (LEGACY_MENU_OPTION *)&LegacyBEVMenu;
      Key         = (UINT16)LEGACY_BEV_QUESTION_ID;
      TypeStr     = STR_BEV;
      TypeStrHelp = STR_BEV_HELP;
      FormTitle   = STR_BEV_TITLE;
      Default     = mLegacyBootOptionPrivate->MaintainMapData->CurrentNvData.LegacyBEV;
      break;

    default:
      DEBUG ((DEBUG_ERROR, "Invalid command ID for updating page!\n"));
      return;
  }

  HiiSetString (mLegacyBootOptionPrivate->HiiHandle, STRING_TOKEN (STR_ORDER_CHANGE_PROMPT), FormTitle, NULL);

  CreateLegacyMenuStringToken (mLegacyBootOptionPrivate->HiiHandle, OptionMenu);

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
    NewMenuEntry = GetMenuEntry (OptionMenu, Index);
    //
    // Create OneOf for each legacy device
    //
    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      NewMenuEntry->DisplayStringToken,
      0,
      EFI_IFR_TYPE_NUM_SIZE_16,
      ((LEGACY_DEVICE_CONTEXT *)NewMenuEntry->VariableContext)->BbsIndex
      );
  }

  //
  // Create OneOf for item "Disabled"
  //
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_DISABLE_LEGACY_DEVICE),
    0,
    EFI_IFR_TYPE_NUM_SIZE_16,
    0xFF
    );

  //
  // Create oneof tag here for FD/HD/CD #1 #2
  //
  for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
    DefaultOpCodeHandle = HiiAllocateOpCodeHandle ();
    ASSERT (DefaultOpCodeHandle != NULL);

    HiiCreateDefaultOpCode (
      DefaultOpCodeHandle,
      EFI_HII_DEFAULT_CLASS_STANDARD,
      EFI_IFR_TYPE_NUM_SIZE_16,
      *Default++
      );

    //
    // Create the string for oneof tag
    //
    UnicodeSPrint (String, sizeof (String), TypeStr, Index);
    StrRef = HiiSetString (mLegacyBootOptionPrivate->HiiHandle, 0, String, NULL);

    UnicodeSPrint (String, sizeof (String), TypeStrHelp, Index);
    StrRefHelp = HiiSetString (mLegacyBootOptionPrivate->HiiHandle, 0, String, NULL);

    HiiCreateOneOfOpCode (
      mLegacyStartOpCodeHandle,
      (EFI_QUESTION_ID)(Key + Index),
      VARSTORE_ID_LEGACY_BOOT,
      (UINT16)(Key + Index * 2 - CONFIG_OPTION_OFFSET),
      StrRef,
      StrRefHelp,
      EFI_IFR_FLAG_CALLBACK,
      EFI_IFR_NUMERIC_SIZE_2,
      OptionsOpCodeHandle,
      DefaultOpCodeHandle // NULL //
      );

    HiiFreeOpCodeHandle (DefaultOpCodeHandle);
  }

  HiiUpdateForm (
    mLegacyBootOptionPrivate->HiiHandle,
    &mLegacyBootOptionGuid,
    LEGACY_ORDER_CHANGE_FORM_ID,
    mLegacyStartOpCodeHandle,
    mLegacyEndOpCodeHandle
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);
}

/**
  Adjust question value when one question value has been changed.

  @param QuestionId    The question id for the value changed question.
  @param Value         The value for the changed question.

**/
VOID
AdjustOptionValue (
  IN  UINT16              QuestionId,
  IN  EFI_IFR_TYPE_VALUE  *Value
  )
{
  UINTN                Number;
  UINT16               *Default;
  LEGACY_BOOT_NV_DATA  *CurrentNVMap;
  UINT16               *CurrentVal;
  UINTN                Index;
  UINTN                Index2;
  UINTN                Index3;
  UINTN                NewValuePos;
  UINTN                OldValue;
  UINTN                NewValue;
  UINT8                *DisMap;
  UINTN                Pos;
  UINTN                Bit;

  Number      = 0;
  CurrentVal  = 0;
  Default     = NULL;
  NewValue    = 0;
  NewValuePos = 0;
  OldValue    = 0;

  //
  // Update Select FD/HD/CD/NET/BEV Order Form
  //
  ASSERT ((QuestionId >= LEGACY_FD_QUESTION_ID) && (QuestionId < LEGACY_BEV_QUESTION_ID + MAX_MENU_NUMBER));

  CurrentNVMap = &mLegacyBootOptionPrivate->MaintainMapData->CurrentNvData;
  HiiGetBrowserData (&mLegacyBootOptionGuid, mLegacyBootStorageName, sizeof (LEGACY_BOOT_NV_DATA), (UINT8 *)CurrentNVMap);
  DisMap = mLegacyBootOptionPrivate->MaintainMapData->DisableMap;

  if ((QuestionId >= LEGACY_FD_QUESTION_ID) && (QuestionId < LEGACY_FD_QUESTION_ID + MAX_MENU_NUMBER)) {
    Number     = (UINT16)LegacyFDMenu.MenuNumber;
    CurrentVal = CurrentNVMap->LegacyFD;
    Default    = mLegacyBootOptionPrivate->MaintainMapData->LastTimeNvData.LegacyFD;
  } else if ((QuestionId >= LEGACY_HD_QUESTION_ID) && (QuestionId < LEGACY_HD_QUESTION_ID + MAX_MENU_NUMBER)) {
    Number     = (UINT16)LegacyHDMenu.MenuNumber;
    CurrentVal = CurrentNVMap->LegacyHD;
    Default    = mLegacyBootOptionPrivate->MaintainMapData->LastTimeNvData.LegacyHD;
  } else if ((QuestionId >= LEGACY_CD_QUESTION_ID) && (QuestionId < LEGACY_CD_QUESTION_ID + MAX_MENU_NUMBER)) {
    Number     = (UINT16)LegacyCDMenu.MenuNumber;
    CurrentVal = CurrentNVMap->LegacyCD;
    Default    = mLegacyBootOptionPrivate->MaintainMapData->LastTimeNvData.LegacyCD;
  } else if ((QuestionId >= LEGACY_NET_QUESTION_ID) && (QuestionId < LEGACY_NET_QUESTION_ID + MAX_MENU_NUMBER)) {
    Number     = (UINT16)LegacyNETMenu.MenuNumber;
    CurrentVal = CurrentNVMap->LegacyNET;
    Default    = mLegacyBootOptionPrivate->MaintainMapData->LastTimeNvData.LegacyNET;
  } else if ((QuestionId >= LEGACY_BEV_QUESTION_ID) && (QuestionId < LEGACY_BEV_QUESTION_ID + MAX_MENU_NUMBER)) {
    Number     = (UINT16)LegacyBEVMenu.MenuNumber;
    CurrentVal = CurrentNVMap->LegacyBEV;
    Default    = mLegacyBootOptionPrivate->MaintainMapData->LastTimeNvData.LegacyBEV;
  }

  //
  //  First, find the different position
  //  if there is change, it should be only one
  //
  for (Index = 0; Index < Number; Index++) {
    if (CurrentVal[Index] != Default[Index]) {
      OldValue = Default[Index];
      NewValue = CurrentVal[Index];
      break;
    }
  }

  if (Index != Number) {
    //
    // there is change, now process
    //
    if (0xFF == NewValue) {
      //
      // This item will be disable
      // Just move the items behind this forward to overlap it
      //
      Pos         = OldValue / 8;
      Bit         = 7 - (OldValue % 8);
      DisMap[Pos] = (UINT8)(DisMap[Pos] | (UINT8)(1 << Bit));
      for (Index2 = Index; Index2 < Number - 1; Index2++) {
        CurrentVal[Index2] = CurrentVal[Index2 + 1];
      }

      CurrentVal[Index2] = 0xFF;
    } else {
      for (Index2 = 0; Index2 < Number; Index2++) {
        if (Index2 == Index) {
          continue;
        }

        if (Default[Index2] == NewValue) {
          //
          // If NewValue is in OldLegacyDev array
          // remember its old position
          //
          NewValuePos = Index2;
          break;
        }
      }

      if (Index2 != Number) {
        //
        // We will change current item to an existing item
        // (It's hard to describe here, please read code, it's like a cycle-moving)
        //
        for (Index2 = NewValuePos; Index2 != Index;) {
          if (NewValuePos < Index) {
            CurrentVal[Index2] = Default[Index2 + 1];
            Index2++;
          } else {
            CurrentVal[Index2] = Default[Index2 - 1];
            Index2--;
          }
        }
      } else {
        //
        // If NewValue is not in OldlegacyDev array, we are changing to a disabled item
        // so we should modify DisMap to reflect the change
        //
        Pos         = NewValue / 8;
        Bit         = 7 - (NewValue % 8);
        DisMap[Pos] = (UINT8)(DisMap[Pos] & (~(UINT8)(1 << Bit)));
        if (0xFF != OldValue) {
          //
          // Because NewValue is a item that was disabled before
          // so after changing the OldValue should be disabled
          // actually we are doing a swap of enable-disable states of two items
          //
          Pos         = OldValue / 8;
          Bit         = 7 - (OldValue % 8);
          DisMap[Pos] = (UINT8)(DisMap[Pos] | (UINT8)(1 << Bit));
        }
      }
    }

    //
    // To prevent DISABLE appears in the middle of the list
    // we should perform a re-ordering
    //
    Index3 = Index;
    Index  = 0;
    while (Index < Number) {
      if (0xFF != CurrentVal[Index]) {
        Index++;
        continue;
      }

      Index2 = Index;
      Index2++;
      while (Index2 < Number) {
        if (0xFF != CurrentVal[Index2]) {
          break;
        }

        Index2++;
      }

      if (Index2 < Number) {
        CurrentVal[Index]  = CurrentVal[Index2];
        CurrentVal[Index2] = 0xFF;
      }

      Index++;
    }

    //
    // Return correct question value.
    //
    Value->u16 = CurrentVal[Index3];
    CopyMem (Default, CurrentVal, sizeof (UINT16) * Number);
  }

  //
  // Pass changed uncommitted data back to Form Browser
  //
  HiiSetBrowserData (&mLegacyBootOptionGuid, mLegacyBootStorageName, sizeof (LEGACY_BOOT_NV_DATA), (UINT8 *)CurrentNVMap, NULL);
}

/**
  This call back function is registered with Boot Manager formset.
  When user selects a boot option, this call back function will
  be triggered. The boot option is saved for later processing.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
LegacyBootOptionCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  if ((Action != EFI_BROWSER_ACTION_CHANGED) && (Action != EFI_BROWSER_ACTION_CHANGING) && (Action != EFI_BROWSER_ACTION_FORM_OPEN)) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed or the form is open.
    //
    return EFI_UNSUPPORTED;
  }

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    if (QuestionId == FORM_FLOPPY_BOOT_ID) {
      if (!mFirstEnterLegacyForm) {
        //
        // The legacyBootMaintUiLib depends on the LegacyBootManagerLib to realize its functionality.
        // We need to do the legacy boot options related actions after the LegacyBootManagerLib has been initialized.
        // Opening the legacy menus is the appropriate time that the LegacyBootManagerLib has already been initialized.
        //
        mFirstEnterLegacyForm = TRUE;
        GetLegacyOptions ();
        GetLegacyOptionsOrder ();
      }
    }
  }

  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    switch (QuestionId) {
      case FORM_FLOPPY_BOOT_ID:
      case FORM_HARDDISK_BOOT_ID:
      case FORM_CDROM_BOOT_ID:
      case FORM_NET_BOOT_ID:
      case FORM_BEV_BOOT_ID:
        UpdateLegacyDeviceOrderPage (QuestionId);
        break;

      default:
        break;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((Value == NULL) || (ActionRequest == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if ((QuestionId >= LEGACY_FD_QUESTION_ID) && (QuestionId < LEGACY_BEV_QUESTION_ID + MAX_MENU_NUMBER)) {
      AdjustOptionValue (QuestionId, Value);
    }
  }

  return EFI_SUCCESS;
}

/**
  Create a menu entry by given menu type.

  @param MenuType        The Menu type to be created.

  @retval NULL           If failed to create the menu.
  @return the new menu entry.

**/
LEGACY_MENU_ENTRY *
CreateMenuEntry (
  VOID
  )
{
  LEGACY_MENU_ENTRY  *MenuEntry;

  //
  // Create new menu entry
  //
  MenuEntry = AllocateZeroPool (sizeof (LEGACY_MENU_ENTRY));
  if (MenuEntry == NULL) {
    return NULL;
  }

  MenuEntry->VariableContext = AllocateZeroPool (sizeof (LEGACY_DEVICE_CONTEXT));
  if (MenuEntry->VariableContext == NULL) {
    FreePool (MenuEntry);
    return NULL;
  }

  MenuEntry->Signature = LEGACY_MENU_ENTRY_SIGNATURE;
  return MenuEntry;
}

/**

  Base on the L"LegacyDevOrder" variable to build the current order data.

**/
VOID
GetLegacyOptionsOrder (
  VOID
  )
{
  UINTN                   VarSize;
  UINT8                   *VarData;
  UINT8                   *VarTmp;
  LEGACY_DEV_ORDER_ENTRY  *DevOrder;
  UINT16                  *LegacyDev;
  UINTN                   Index;
  LEGACY_MENU_OPTION      *OptionMenu;
  UINT16                  VarDevOrder;
  UINTN                   Pos;
  UINTN                   Bit;
  UINT8                   *DisMap;
  UINTN                   TotalLength;

  LegacyDev  = NULL;
  OptionMenu = NULL;

  DisMap = ZeroMem (mLegacyBootOptionPrivate->MaintainMapData->DisableMap, sizeof (mLegacyBootOptionPrivate->MaintainMapData->DisableMap));

  //
  // Get Device Order from variable
  //
  GetVariable2 (VAR_LEGACY_DEV_ORDER, &gEfiLegacyDevOrderVariableGuid, (VOID **)&VarData, &VarSize);
  VarTmp = VarData;
  if (NULL != VarData) {
    DevOrder = (LEGACY_DEV_ORDER_ENTRY *)VarData;
    while (VarData < VarTmp + VarSize) {
      switch (DevOrder->BbsType) {
        case BBS_FLOPPY:
          LegacyDev  = mLegacyBootOptionPrivate->MaintainMapData->InitialNvData.LegacyFD;
          OptionMenu = &LegacyFDMenu;
          break;

        case BBS_HARDDISK:
          LegacyDev  = mLegacyBootOptionPrivate->MaintainMapData->InitialNvData.LegacyHD;
          OptionMenu = &LegacyHDMenu;
          break;

        case BBS_CDROM:
          LegacyDev  = mLegacyBootOptionPrivate->MaintainMapData->InitialNvData.LegacyCD;
          OptionMenu = &LegacyCDMenu;
          break;

        case BBS_EMBED_NETWORK:
          LegacyDev  = mLegacyBootOptionPrivate->MaintainMapData->InitialNvData.LegacyNET;
          OptionMenu = &LegacyNETMenu;
          break;

        case BBS_BEV_DEVICE:
          LegacyDev  = mLegacyBootOptionPrivate->MaintainMapData->InitialNvData.LegacyBEV;
          OptionMenu = &LegacyBEVMenu;
          break;

        case BBS_UNKNOWN:
        default:
          ASSERT (FALSE);
          DEBUG ((DEBUG_ERROR, "Unsupported device type found!\n"));
          break;
      }

      //
      // Create oneof tag here for FD/HD/CD #1 #2
      //
      for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
        TotalLength = sizeof (BBS_TYPE) + sizeof (UINT16) + Index * sizeof (UINT16);
        VarDevOrder = *(UINT16 *)((UINT8 *)DevOrder + TotalLength);

        if (0xFF00 == (VarDevOrder & 0xFF00)) {
          LegacyDev[Index] = 0xFF;
          Pos              = (VarDevOrder & 0xFF) / 8;
          Bit              = 7 - ((VarDevOrder & 0xFF) % 8);
          DisMap[Pos]      = (UINT8)(DisMap[Pos] | (UINT8)(1 << Bit));
        } else {
          LegacyDev[Index] = VarDevOrder & 0xFF;
        }
      }

      VarData++;
      VarData += *(UINT16 *)VarData;
      DevOrder = (LEGACY_DEV_ORDER_ENTRY *)VarData;
    }
  }

  CopyMem (&mLegacyBootOptionPrivate->MaintainMapData->LastTimeNvData, &mLegacyBootOptionPrivate->MaintainMapData->InitialNvData, sizeof (LEGACY_BOOT_NV_DATA));
  CopyMem (&mLegacyBootOptionPrivate->MaintainMapData->CurrentNvData, &mLegacyBootOptionPrivate->MaintainMapData->InitialNvData, sizeof (LEGACY_BOOT_NV_DATA));
}

/**

  Build the LegacyFDMenu LegacyHDMenu LegacyCDMenu according to LegacyBios.GetBbsInfo().

**/
VOID
GetLegacyOptions (
  VOID
  )
{
  LEGACY_MENU_ENTRY             *NewMenuEntry;
  LEGACY_DEVICE_CONTEXT         *NewLegacyDevContext;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;
  UINT16                        Index;
  UINTN                         FDNum;
  UINTN                         HDNum;
  UINTN                         CDNum;
  UINTN                         NETNum;
  UINTN                         BEVNum;

  //
  // Initialize Bbs Table Context from BBS info data
  //
  InitializeListHead (&LegacyFDMenu.Head);
  InitializeListHead (&LegacyHDMenu.Head);
  InitializeListHead (&LegacyCDMenu.Head);
  InitializeListHead (&LegacyNETMenu.Head);
  InitializeListHead (&LegacyBEVMenu.Head);

  FDNum  = 0;
  HDNum  = 0;
  CDNum  = 0;
  NETNum = 0;
  BEVNum = 0;

  EfiBootManagerConnectAll ();

  //
  // for better user experience
  // 1. User changes HD configuration (e.g.: unplug HDD), here we have a chance to remove the HDD boot option
  // 2. User enables/disables UEFI PXE, here we have a chance to add/remove EFI Network boot option
  //
  EfiBootManagerRefreshAllBootOption ();

  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
  for (Index = 0; Index < BootOptionCount; Index++) {
    if ((DevicePathType (BootOption[Index].FilePath) != BBS_DEVICE_PATH) ||
        (DevicePathSubType (BootOption[Index].FilePath) != BBS_BBS_DP)
        )
    {
      continue;
    }

    ASSERT (BootOption[Index].OptionalDataSize == sizeof (LEGACY_BOOT_OPTION_BBS_DATA));
    NewMenuEntry = CreateMenuEntry ();
    ASSERT (NewMenuEntry != NULL);

    NewLegacyDevContext              = (LEGACY_DEVICE_CONTEXT *)NewMenuEntry->VariableContext;
    NewLegacyDevContext->BbsIndex    = ((LEGACY_BOOT_OPTION_BBS_DATA *)BootOption[Index].OptionalData)->BbsIndex;
    NewLegacyDevContext->Description = AllocateCopyPool (StrSize (BootOption[Index].Description), BootOption[Index].Description);
    ASSERT (NewLegacyDevContext->Description != NULL);

    NewMenuEntry->DisplayString = NewLegacyDevContext->Description;
    NewMenuEntry->HelpString    = NULL;

    switch (((BBS_BBS_DEVICE_PATH *)BootOption[Index].FilePath)->DeviceType) {
      case BBS_TYPE_FLOPPY:
        InsertTailList (&LegacyFDMenu.Head, &NewMenuEntry->Link);
        FDNum++;
        break;

      case BBS_TYPE_HARDDRIVE:
        InsertTailList (&LegacyHDMenu.Head, &NewMenuEntry->Link);
        HDNum++;
        break;

      case BBS_TYPE_CDROM:
        InsertTailList (&LegacyCDMenu.Head, &NewMenuEntry->Link);
        CDNum++;
        break;

      case BBS_TYPE_EMBEDDED_NETWORK:
        InsertTailList (&LegacyNETMenu.Head, &NewMenuEntry->Link);
        NETNum++;
        break;

      case BBS_TYPE_BEV:
        InsertTailList (&LegacyBEVMenu.Head, &NewMenuEntry->Link);
        BEVNum++;
        break;
    }
  }

  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);

  LegacyFDMenu.MenuNumber  = FDNum;
  LegacyHDMenu.MenuNumber  = HDNum;
  LegacyCDMenu.MenuNumber  = CDNum;
  LegacyNETMenu.MenuNumber = NETNum;
  LegacyBEVMenu.MenuNumber = BEVNum;
}

/**

  Install Boot Manager Menu driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCCESS  Install Boot manager menu success.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
LegacyBootMaintUiLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                        Status;
  EFI_LEGACY_BIOS_PROTOCOL          *LegacyBios;
  LEGACY_BOOT_OPTION_CALLBACK_DATA  *LegacyBootOptionData;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **)&LegacyBios);
  if (!EFI_ERROR (Status)) {
    //
    // Create LegacyBootOptionData structures for Driver Callback
    //
    LegacyBootOptionData = AllocateZeroPool (sizeof (LEGACY_BOOT_OPTION_CALLBACK_DATA));
    ASSERT (LegacyBootOptionData != NULL);

    LegacyBootOptionData->MaintainMapData = AllocateZeroPool (sizeof (LEGACY_BOOT_MAINTAIN_DATA));
    ASSERT (LegacyBootOptionData->MaintainMapData != NULL);

    LegacyBootOptionData->ConfigAccess.ExtractConfig = LegacyBootOptionExtractConfig;
    LegacyBootOptionData->ConfigAccess.RouteConfig   = LegacyBootOptionRouteConfig;
    LegacyBootOptionData->ConfigAccess.Callback      = LegacyBootOptionCallback;

    //
    // Install Device Path Protocol and Config Access protocol to driver handle
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &LegacyBootOptionData->DriverHandle,
                    &gEfiDevicePathProtocolGuid,
                    &mLegacyBootOptionHiiVendorDevicePath,
                    &gEfiHiiConfigAccessProtocolGuid,
                    &LegacyBootOptionData->ConfigAccess,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Publish our HII data
    //
    LegacyBootOptionData->HiiHandle = HiiAddPackages (
                                        &mLegacyBootOptionGuid,
                                        LegacyBootOptionData->DriverHandle,
                                        LegacyBootMaintUiVfrBin,
                                        LegacyBootMaintUiLibStrings,
                                        NULL
                                        );
    ASSERT (LegacyBootOptionData->HiiHandle != NULL);

    mLegacyBootOptionPrivate = LegacyBootOptionData;
  }

  return EFI_SUCCESS;
}

/**
  Destructor of Customized Display Library Instance.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor completed successfully.
  @retval Other value   The destructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
LegacyBootMaintUiLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if ((mLegacyBootOptionPrivate != NULL) && (mLegacyBootOptionPrivate->DriverHandle != NULL)) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    mLegacyBootOptionPrivate->DriverHandle,
                    &gEfiDevicePathProtocolGuid,
                    &mLegacyBootOptionHiiVendorDevicePath,
                    &gEfiHiiConfigAccessProtocolGuid,
                    &mLegacyBootOptionPrivate->ConfigAccess,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    HiiRemovePackages (mLegacyBootOptionPrivate->HiiHandle);

    FreePool (mLegacyBootOptionPrivate->MaintainMapData);
    FreePool (mLegacyBootOptionPrivate);
  }

  return EFI_SUCCESS;
}
