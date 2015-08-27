/** @file
  FrontPage routines to handle the callbacks and browser calls

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Bds.h"
#include "FrontPage.h"
#include "Language.h"
#include "Hotkey.h"

BOOLEAN   mModeInitialized = FALSE;

BOOLEAN   gConnectAllHappened = FALSE;
UINTN     gCallbackKey;
CHAR8     *mLanguageString;

//
// Boot video resolution and text mode.
//
UINT32    mBootHorizontalResolution    = 0;
UINT32    mBootVerticalResolution      = 0;
UINT32    mBootTextModeColumn          = 0;
UINT32    mBootTextModeRow             = 0;
//
// BIOS setup video resolution and text mode.
//
UINT32    mSetupTextModeColumn         = 0;
UINT32    mSetupTextModeRow            = 0;
UINT32    mSetupHorizontalResolution   = 0;
UINT32    mSetupVerticalResolution     = 0;

EFI_FORM_BROWSER2_PROTOCOL      *gFormBrowser2;

FRONT_PAGE_CALLBACK_DATA  gFrontPagePrivate = {
  FRONT_PAGE_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    FrontPageCallback
  }
};

HII_VENDOR_DEVICE_PATH  mFrontPageHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    FRONT_PAGE_FORMSET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

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
FakeExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  if (Progress == NULL || Results == NULL) {
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
FakeRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;
  if (!HiiIsConfigHdrMatch (Configuration, &gBootMaintFormSetGuid, mBootMaintStorageName)
      && !HiiIsConfigHdrMatch (Configuration, &gFileExploreFormSetGuid, mFileExplorerStorageName)) {
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);
  return EFI_SUCCESS;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR      The variable could not be saved.
  @retval  EFI_UNSUPPORTED       The specified Action is not supported by the callback.

**/
EFI_STATUS
EFIAPI
FrontPageCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  CHAR8                         *LangCode;
  CHAR8                         *Lang;
  UINTN                         Index;

  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED) {
    //
    // All other action return unsupported.
    //
    return EFI_UNSUPPORTED;
  }
  
  gCallbackKey = QuestionId;

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((Value == NULL) || (ActionRequest == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    switch (QuestionId) {
    case FRONT_PAGE_KEY_CONTINUE:
      //
      // This is the continue - clear the screen and return an error to get out of FrontPage loop
      //
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
      break;

    case FRONT_PAGE_KEY_LANGUAGE:
      //
      // Allocate working buffer for RFC 4646 language in supported LanguageString.
      //
      Lang = AllocatePool (AsciiStrSize (mLanguageString));
      ASSERT (Lang != NULL);  

      Index = 0;
      LangCode = mLanguageString;
      while (*LangCode != 0) {
        GetNextLanguage (&LangCode, Lang);

        if (Index == Value->u8) {
          break;
        }

        Index++;
      }

      if (Index == Value->u8) {
        BdsDxeSetVariableAndReportStatusCodeOnError (
                        L"PlatformLang",
                        &gEfiGlobalVariableGuid,
                        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                        AsciiStrSize (Lang),
                        Lang
                        );
      } else {
        ASSERT (FALSE);
      }

      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;

      FreePool (Lang);
      break;

    default:
      break;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGING) {
    if (Value == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // The first 4 entries in the Front Page are to be GUARANTEED to remain constant so IHV's can
    // describe to their customers in documentation how to find their setup information (namely
    // under the device manager and specific buckets)
    //
    switch (QuestionId) {
    case FRONT_PAGE_KEY_BOOT_MANAGER:
      //
      // Boot Manager
      //
      break;

    case FRONT_PAGE_KEY_DEVICE_MANAGER:
      //
      // Device Manager
      //
      break;

    case FRONT_PAGE_KEY_BOOT_MAINTAIN:
      //
      // Boot Maintenance Manager
      //
      break;

    default:
      gCallbackKey = 0;
      break;
    }
  }

  return EFI_SUCCESS;  
}

/**
  Initialize HII information for the FrontPage


  @param InitializeHiiData    TRUE if HII elements need to be initialized.

  @retval  EFI_SUCCESS        The operation is successful.
  @retval  EFI_DEVICE_ERROR   If the dynamic opcode creation failed.

**/
EFI_STATUS
InitializeFrontPage (
  IN BOOLEAN                         InitializeHiiData
  )
{
  EFI_STATUS                  Status;
  CHAR8                       *LangCode;
  CHAR8                       *Lang;
  CHAR8                       *CurrentLang;
  UINTN                       OptionCount;
  CHAR16                      *StringBuffer;
  EFI_HII_HANDLE              HiiHandle;
  VOID                        *OptionsOpCodeHandle;
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *EndLabel;
  EFI_HII_STRING_PROTOCOL     *HiiString;
  UINTN                       StringSize;

  Lang         = NULL;
  StringBuffer = NULL;

  if (InitializeHiiData) {
    //
    // Initialize the Device Manager
    //
    InitializeDeviceManager ();

    //
    // Initialize the Device Manager
    //
    InitializeBootManager ();

    gCallbackKey  = 0;

    //
    // Locate Hii relative protocols
    //
    Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &gFormBrowser2);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Install Device Path Protocol and Config Access protocol to driver handle
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &gFrontPagePrivate.DriverHandle,
                    &gEfiDevicePathProtocolGuid,
                    &mFrontPageHiiVendorDevicePath,
                    &gEfiHiiConfigAccessProtocolGuid,
                    &gFrontPagePrivate.ConfigAccess,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Publish our HII data
    //
    gFrontPagePrivate.HiiHandle = HiiAddPackages (
                                    &gFrontPageFormSetGuid,
                                    gFrontPagePrivate.DriverHandle,
                                    FrontPageVfrBin,
                                    BdsDxeStrings,
                                    NULL
                                    );
    if (gFrontPagePrivate.HiiHandle == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }


  //
  // Init OpCode Handle and Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);
  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_SELECT_LANGUAGE;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Collect the languages from what our current Language support is based on our VFR
  //
  HiiHandle = gFrontPagePrivate.HiiHandle;

  GetEfiGlobalVariable2 (L"PlatformLang", (VOID**)&CurrentLang, NULL);

  //
  // Get Support language list from variable.
  //
  if (mLanguageString == NULL){
    GetEfiGlobalVariable2 (L"PlatformLangCodes", (VOID**)&mLanguageString, NULL);
    if (mLanguageString == NULL) {
      mLanguageString = AllocateCopyPool (
                                 AsciiStrSize ((CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes)),
                                 (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes)
                                 );
      ASSERT (mLanguageString != NULL);
    }
  }

  if (gFrontPagePrivate.LanguageToken == NULL) {
    //
    // Count the language list number.
    //  
    LangCode      = mLanguageString;
    Lang          = AllocatePool (AsciiStrSize (mLanguageString));
    ASSERT (Lang != NULL);
    OptionCount = 0;
    while (*LangCode != 0) {
      GetNextLanguage (&LangCode, Lang);
      OptionCount ++;
    }

    //
    // Allocate extra 1 as the end tag.
    //
    gFrontPagePrivate.LanguageToken = AllocateZeroPool ((OptionCount + 1) * sizeof (EFI_STRING_ID));
    ASSERT (gFrontPagePrivate.LanguageToken != NULL);

    Status = gBS->LocateProtocol (&gEfiHiiStringProtocolGuid, NULL, (VOID **) &HiiString);
    ASSERT_EFI_ERROR (Status);

    LangCode     = mLanguageString;
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
        StringBuffer = AllocatePool (AsciiStrSize (Lang) * sizeof (CHAR16));
        ASSERT (StringBuffer != NULL);
        AsciiStrToUnicodeStr (Lang, StringBuffer);
      }

      ASSERT (StringBuffer != NULL);
      gFrontPagePrivate.LanguageToken[OptionCount] = HiiSetString (HiiHandle, 0, StringBuffer, NULL);
      FreePool (StringBuffer);

      OptionCount++;
    }
  }

  ASSERT (gFrontPagePrivate.LanguageToken != NULL);
  LangCode     = mLanguageString;
  OptionCount  = 0;
  if (Lang == NULL) {
    Lang = AllocatePool (AsciiStrSize (mLanguageString));
    ASSERT (Lang != NULL);
  }
  while (*LangCode != 0) {
    GetNextLanguage (&LangCode, Lang);

    if (CurrentLang != NULL && AsciiStrCmp (Lang, CurrentLang) == 0) {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        gFrontPagePrivate.LanguageToken[OptionCount],
        EFI_IFR_OPTION_DEFAULT,
        EFI_IFR_NUMERIC_SIZE_1,
        (UINT8) OptionCount
        );
    } else {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        gFrontPagePrivate.LanguageToken[OptionCount],
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

  Status = HiiUpdateForm (
             HiiHandle,
             &gFrontPageFormSetGuid,
             FRONT_PAGE_FORM_ID,
             StartOpCodeHandle, // LABEL_SELECT_LANGUAGE
             EndOpCodeHandle    // LABEL_END
             );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  HiiFreeOpCodeHandle (OptionsOpCodeHandle);
  return Status;
}

/**
  Call the browser and display the front page

  @return   Status code that will be returned by
            EFI_FORM_BROWSER2_PROTOCOL.SendForm ().

**/
EFI_STATUS
CallFrontPage (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;

  //
  // Begin waiting for USER INPUT
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_INPUT_WAIT)
    );

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                            gFormBrowser2,
                            &gFrontPagePrivate.HiiHandle,
                            1,
                            &gFrontPageFormSetGuid,
                            0,
                            NULL,
                            &ActionRequest
                            );
  //
  // Check whether user change any option setting which needs a reset to be effective
  //
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  return Status;
}

/**
  Acquire the string associated with the ProducerGuid and return it.


  @param ProducerGuid    The Guid to search the HII database for
  @param Token           The token value of the string to extract
  @param String          The string that is extracted

  @retval  EFI_SUCCESS  The function returns EFI_SUCCESS always.

**/
EFI_STATUS
GetProducerString (
  IN      EFI_GUID                  *ProducerGuid,
  IN      EFI_STRING_ID             Token,
  OUT     CHAR16                    **String
  )
{
  EFI_STRING      TmpString;

  TmpString = HiiGetPackageString (ProducerGuid, Token, NULL);
  if (TmpString == NULL) {
    *String = GetStringById (STRING_TOKEN (STR_MISSING_STRING));
  } else {
    *String = TmpString;
  }

  return EFI_SUCCESS;
}

/**
  Convert Processor Frequency Data to a string.

  @param ProcessorFrequency The frequency data to process
  @param Base10Exponent     The exponent based on 10
  @param String             The string that is created

**/
VOID
ConvertProcessorToString (
  IN  UINT16                               ProcessorFrequency,
  IN  UINT16                               Base10Exponent,
  OUT CHAR16                               **String
  )
{
  CHAR16  *StringBuffer;
  UINTN   Index;
  UINT32  FreqMhz;

  if (Base10Exponent >= 6) {
    FreqMhz = ProcessorFrequency;
    for (Index = 0; Index < (UINTN) (Base10Exponent - 6); Index++) {
      FreqMhz *= 10;
    }
  } else {
    FreqMhz = 0;
  }

  StringBuffer = AllocateZeroPool (0x20);
  ASSERT (StringBuffer != NULL);
  Index = UnicodeValueToString (StringBuffer, LEFT_JUSTIFY, FreqMhz / 1000, 3);
  StrCatS (StringBuffer, 0x20 / sizeof (CHAR16), L".");
  UnicodeValueToString (StringBuffer + Index + 1, PREFIX_ZERO, (FreqMhz % 1000) / 10, 2);
  StrCatS (StringBuffer, 0x20 / sizeof (CHAR16), L" GHz");
  *String = (CHAR16 *) StringBuffer;
  return ;
}


/**
  Convert Memory Size to a string.

  @param MemorySize      The size of the memory to process
  @param String          The string that is created

**/
VOID
ConvertMemorySizeToString (
  IN  UINT32          MemorySize,
  OUT CHAR16          **String
  )
{
  CHAR16  *StringBuffer;

  StringBuffer = AllocateZeroPool (0x20);
  ASSERT (StringBuffer != NULL);
  UnicodeValueToString (StringBuffer, LEFT_JUSTIFY, MemorySize, 6);
  StrCatS (StringBuffer, 0x20 / sizeof (CHAR16), L" MB RAM");

  *String = (CHAR16 *) StringBuffer;

  return ;
}

/**

  Acquire the string associated with the Index from smbios structure and return it.
  The caller is responsible for free the string buffer.

  @param    OptionalStrStart  The start position to search the string
  @param    Index             The index of the string to extract
  @param    String            The string that is extracted

  @retval   EFI_SUCCESS       The function returns EFI_SUCCESS always.

**/
EFI_STATUS
GetOptionalStringByIndex (
  IN      CHAR8                   *OptionalStrStart,
  IN      UINT8                   Index,
  OUT     CHAR16                  **String
  )
{
  UINTN          StrSize;

  if (Index == 0) {
    *String = AllocateZeroPool (sizeof (CHAR16));
    return EFI_SUCCESS;
  }

  StrSize = 0;
  do {
    Index--;
    OptionalStrStart += StrSize;
    StrSize           = AsciiStrSize (OptionalStrStart);
  } while (OptionalStrStart[StrSize] != 0 && Index != 0);

  if ((Index != 0) || (StrSize == 1)) {
    //
    // Meet the end of strings set but Index is non-zero, or
    // Find an empty string
    //
    *String = GetStringById (STRING_TOKEN (STR_MISSING_STRING));
  } else {
    *String = AllocatePool (StrSize * sizeof (CHAR16));
    AsciiStrToUnicodeStr (OptionalStrStart, *String);
  }

  return EFI_SUCCESS;
}


/**
  Update the banner information for the Front Page based on DataHub information.

**/
VOID
UpdateFrontPageStrings (
  VOID
  )
{
  UINT8                             StrIndex;
  CHAR16                            *NewString;
  BOOLEAN                           Find[5];
  EFI_STATUS                        Status;
  EFI_STRING_ID                     TokenToUpdate;
  EFI_SMBIOS_HANDLE                 SmbiosHandle;
  EFI_SMBIOS_PROTOCOL               *Smbios;
  SMBIOS_TABLE_TYPE0                *Type0Record;
  SMBIOS_TABLE_TYPE1                *Type1Record;
  SMBIOS_TABLE_TYPE4                *Type4Record;
  SMBIOS_TABLE_TYPE19               *Type19Record;
  EFI_SMBIOS_TABLE_HEADER           *Record;

  ZeroMem (Find, sizeof (Find));

  //
  // Update Front Page strings
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **) &Smbios
                  );
  if (!EFI_ERROR (Status)) {
    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    do {
      Status = Smbios->GetNext (Smbios, &SmbiosHandle, NULL, &Record, NULL);
      if (EFI_ERROR(Status)) {
        break;
      }

      if (Record->Type == EFI_SMBIOS_TYPE_BIOS_INFORMATION) {
        Type0Record = (SMBIOS_TABLE_TYPE0 *) Record;
        StrIndex = Type0Record->BiosVersion;
        GetOptionalStringByIndex ((CHAR8*)((UINT8*)Type0Record + Type0Record->Hdr.Length), StrIndex, &NewString);
        TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_BIOS_VERSION);
        HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString, NULL);
        FreePool (NewString);
        Find[0] = TRUE;
      }

      if (Record->Type == EFI_SMBIOS_TYPE_SYSTEM_INFORMATION) {
        Type1Record = (SMBIOS_TABLE_TYPE1 *) Record;
        StrIndex = Type1Record->ProductName;
        GetOptionalStringByIndex ((CHAR8*)((UINT8*)Type1Record + Type1Record->Hdr.Length), StrIndex, &NewString);
        TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_COMPUTER_MODEL);
        HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString, NULL);
        FreePool (NewString);
        Find[1] = TRUE;
      }

      if (Record->Type == EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION) {
        Type4Record = (SMBIOS_TABLE_TYPE4 *) Record;
        StrIndex = Type4Record->ProcessorVersion;
        GetOptionalStringByIndex ((CHAR8*)((UINT8*)Type4Record + Type4Record->Hdr.Length), StrIndex, &NewString);
        TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_CPU_MODEL);
        HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString, NULL);
        FreePool (NewString);
        Find[2] = TRUE;
      }

      if (Record->Type == EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION) {
        Type4Record = (SMBIOS_TABLE_TYPE4 *) Record;
        ConvertProcessorToString(Type4Record->CurrentSpeed, 6, &NewString);
        TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_CPU_SPEED);
        HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString, NULL);
        FreePool (NewString);
        Find[3] = TRUE;
      }

      if ( Record->Type == EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS ) {
        Type19Record = (SMBIOS_TABLE_TYPE19 *) Record;
        ConvertMemorySizeToString (
          (UINT32)(RShiftU64((Type19Record->EndingAddress - Type19Record->StartingAddress + 1), 10)),
          &NewString
          );
        TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_MEMORY_SIZE);
        HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString, NULL);
        FreePool (NewString);
        Find[4] = TRUE;
      }
    } while ( !(Find[0] && Find[1] && Find[2] && Find[3] && Find[4]));
  }
  return ;
}


/**
  Function waits for a given event to fire, or for an optional timeout to expire.

  @param   Event              The event to wait for
  @param   Timeout            An optional timeout value in 100 ns units.

  @retval  EFI_SUCCESS      Event fired before Timeout expired.
  @retval  EFI_TIME_OUT     Timout expired before Event fired..

**/
EFI_STATUS
WaitForSingleEvent (
  IN EFI_EVENT                  Event,
  IN UINT64                     Timeout OPTIONAL
  )
{
  UINTN       Index;
  EFI_STATUS  Status;
  EFI_EVENT   TimerEvent;
  EFI_EVENT   WaitList[2];

  if (Timeout != 0) {
    //
    // Create a timer event
    //
    Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
    if (!EFI_ERROR (Status)) {
      //
      // Set the timer event
      //
      gBS->SetTimer (
             TimerEvent,
             TimerRelative,
             Timeout
             );

      //
      // Wait for the original event or the timer
      //
      WaitList[0] = Event;
      WaitList[1] = TimerEvent;
      Status      = gBS->WaitForEvent (2, WaitList, &Index);
      gBS->CloseEvent (TimerEvent);

      //
      // If the timer expired, change the return to timed out
      //
      if (!EFI_ERROR (Status) && Index == 1) {
        Status = EFI_TIMEOUT;
      }
    }
  } else {
    //
    // No timeout... just wait on the event
    //
    Status = gBS->WaitForEvent (1, &Event, &Index);
    ASSERT (!EFI_ERROR (Status));
    ASSERT (Index == 0);
  }

  return Status;
}

/**
  Function show progress bar to wait for user input.


  @param   TimeoutDefault  The fault time out value before the system continue to boot.

  @retval  EFI_SUCCESS       User pressed some key except "Enter"
  @retval  EFI_TIME_OUT      Timeout expired or user press "Enter"

**/
EFI_STATUS
ShowProgress (
  IN UINT16                       TimeoutDefault
  )
{
  CHAR16                        *TmpStr;
  UINT16                        TimeoutRemain;
  EFI_STATUS                    Status;
  EFI_INPUT_KEY                 Key;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;

  if (TimeoutDefault != 0) {
    DEBUG ((EFI_D_INFO, "\n\nStart showing progress bar... Press any key to stop it! ...Zzz....\n"));

    SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
    SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
    SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
    
    TmpStr = GetStringById (STRING_TOKEN (STR_START_BOOT_OPTION));

    if (!FeaturePcdGet(PcdBootlogoOnlyEnable)) {
      //
      // Clear the progress status bar first
      //
      if (TmpStr != NULL) {
        PlatformBdsShowProgress (Foreground, Background, TmpStr, Color, 0, 0);
      }
    }
    

    TimeoutRemain = TimeoutDefault;
    while (TimeoutRemain != 0) {
      DEBUG ((EFI_D_INFO, "Showing progress bar...Remaining %d second!\n", TimeoutRemain));

      Status = WaitForSingleEvent (gST->ConIn->WaitForKey, ONE_SECOND);
      if (Status != EFI_TIMEOUT) {
        break;
      }
      TimeoutRemain--;
      
      if (!FeaturePcdGet(PcdBootlogoOnlyEnable)) {
        //
        // Show progress
        //
        if (TmpStr != NULL) {
          PlatformBdsShowProgress (
            Foreground,
            Background,
            TmpStr,
            Color,
            ((TimeoutDefault - TimeoutRemain) * 100 / TimeoutDefault),
            0
            );
        }
      }
    }
    
    if (TmpStr != NULL) {
      gBS->FreePool (TmpStr);
    }

    //
    // Timeout expired
    //
    if (TimeoutRemain == 0) {
      return EFI_TIMEOUT;
    }
  }

  //
  // User pressed some key
  //
  if (!PcdGetBool (PcdConInConnectOnDemand)) {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
      //
      // User pressed enter, equivalent to select "continue"
      //
      return EFI_TIMEOUT;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function is the main entry of the platform setup entry.
  The function will present the main menu of the system setup,
  this is the platform reference part and can be customize.


  @param TimeoutDefault     The fault time out value before the system
                            continue to boot.
  @param ConnectAllHappened The indicater to check if the connect all have
                            already happened.

**/
VOID
PlatformBdsEnterFrontPage (
  IN UINT16                       TimeoutDefault,
  IN BOOLEAN                      ConnectAllHappened
  )
{
  EFI_STATUS                         Status;
  EFI_STATUS                         StatusHotkey; 
  EFI_BOOT_LOGO_PROTOCOL             *BootLogo;
  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *SimpleTextOut;
  UINTN                              BootTextColumn;
  UINTN                              BootTextRow;
  UINT64                             OsIndication;
  UINTN                              DataSize;
  EFI_INPUT_KEY                      Key;

  GraphicsOutput = NULL;
  SimpleTextOut = NULL;

  PERF_START (NULL, "BdsTimeOut", "BDS", 0);
  //
  // Indicate if we need connect all in the platform setup
  //
  if (ConnectAllHappened) {
    gConnectAllHappened = TRUE;
  }

  if (!mModeInitialized) {
    //
    // After the console is ready, get current video resolution 
    // and text mode before launching setup at first time.
    //
    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID**)&GraphicsOutput
                    );
    if (EFI_ERROR (Status)) {
      GraphicsOutput = NULL;
    }
    
    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiSimpleTextOutProtocolGuid,
                    (VOID**)&SimpleTextOut
                    );
    if (EFI_ERROR (Status)) {
      SimpleTextOut = NULL;
    }  

    if (GraphicsOutput != NULL) {
      //
      // Get current video resolution and text mode.
      //
      mBootHorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
      mBootVerticalResolution   = GraphicsOutput->Mode->Info->VerticalResolution;
    }

    if (SimpleTextOut != NULL) {
      Status = SimpleTextOut->QueryMode (
                                SimpleTextOut,
                                SimpleTextOut->Mode->Mode,
                                &BootTextColumn,
                                &BootTextRow
                                );
      mBootTextModeColumn = (UINT32)BootTextColumn;
      mBootTextModeRow    = (UINT32)BootTextRow;
    }

    //
    // Get user defined text mode for setup.
    //  
    mSetupHorizontalResolution = PcdGet32 (PcdSetupVideoHorizontalResolution);
    mSetupVerticalResolution   = PcdGet32 (PcdSetupVideoVerticalResolution);      
    mSetupTextModeColumn       = PcdGet32 (PcdSetupConOutColumn);
    mSetupTextModeRow          = PcdGet32 (PcdSetupConOutRow);

    mModeInitialized           = TRUE;
  }


  //
  // goto FrontPage directly when EFI_OS_INDICATIONS_BOOT_TO_FW_UI is set
  //
  OsIndication = 0;
  DataSize = sizeof(UINT64);
  Status = gRT->GetVariable (
                  L"OsIndications",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  &OsIndication
                  );

  //
  // goto FrontPage directly when EFI_OS_INDICATIONS_BOOT_TO_FW_UI is set. Skip HotkeyBoot
  //
  if (!EFI_ERROR(Status) && ((OsIndication & EFI_OS_INDICATIONS_BOOT_TO_FW_UI) != 0)) {
    //
    // Clear EFI_OS_INDICATIONS_BOOT_TO_FW_UI to acknowledge OS
    // 
    OsIndication &= ~((UINT64)EFI_OS_INDICATIONS_BOOT_TO_FW_UI);
    Status = gRT->SetVariable (
                    L"OsIndications",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof(UINT64),
                    &OsIndication
                    );
    //
    // Changing the content without increasing its size with current variable implementation shouldn't fail.
    //
    ASSERT_EFI_ERROR (Status);

    //
    // Follow generic rule, Call ReadKeyStroke to connect ConIn before enter UI
    //
    if (PcdGetBool (PcdConInConnectOnDemand)) {
      gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
    }

    //
    // Ensure screen is clear when switch Console from Graphics mode to Text mode
    //
    gST->ConOut->EnableCursor (gST->ConOut, TRUE);
    gST->ConOut->ClearScreen (gST->ConOut);

  } else {

    HotkeyBoot ();
    if (TimeoutDefault != 0xffff) {
      Status = ShowProgress (TimeoutDefault);
      StatusHotkey = HotkeyBoot ();

      if (!FeaturePcdGet(PcdBootlogoOnlyEnable) || !EFI_ERROR(Status) || !EFI_ERROR(StatusHotkey)){
        //
        // Ensure screen is clear when switch Console from Graphics mode to Text mode
        // Skip it in normal boot 
        //
        gST->ConOut->EnableCursor (gST->ConOut, TRUE);
        gST->ConOut->ClearScreen (gST->ConOut);
      }

      if (EFI_ERROR (Status)) {
        //
        // Timeout or user press enter to continue
        //
        goto Exit;
      }
    }
  }

  //
  // Boot Logo is corrupted, report it using Boot Logo protocol.
  //
  Status = gBS->LocateProtocol (&gEfiBootLogoProtocolGuid, NULL, (VOID **) &BootLogo);
  if (!EFI_ERROR (Status) && (BootLogo != NULL)) {
    BootLogo->SetBootLogo (BootLogo, NULL, 0, 0, 0, 0);
  }

  //
  // Install BM HiiPackages. 
  // Keep BootMaint HiiPackage, so that it can be covered by global setting. 
  //
  InitBMPackage ();

  Status = EFI_SUCCESS;
  do {
    //
    // Set proper video resolution and text mode for setup
    //
    BdsSetConsoleMode (TRUE);
    
    InitializeFrontPage (FALSE);

    //
    // Update Front Page strings
    //
    UpdateFrontPageStrings ();

    gCallbackKey = 0;
    CallFrontPage ();

    //
    // If gCallbackKey is greater than 1 and less or equal to 5,
    // it will launch configuration utilities.
    // 2 = set language
    // 3 = boot manager
    // 4 = device manager
    // 5 = boot maintenance manager
    //
    if (gCallbackKey != 0) {
      REPORT_STATUS_CODE (
        EFI_PROGRESS_CODE,
        (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_USER_SETUP)
        );
    }
    //
    // Based on the key that was set, we can determine what to do
    //
    switch (gCallbackKey) {
    //
    // The first 4 entries in the Front Page are to be GUARANTEED to remain constant so IHV's can
    // describe to their customers in documentation how to find their setup information (namely
    // under the device manager and specific buckets)
    //
    // These entries consist of the Continue, Select language, Boot Manager, and Device Manager
    //
    case FRONT_PAGE_KEY_CONTINUE:
      //
      // User hit continue
      //
      break;

    case FRONT_PAGE_KEY_LANGUAGE:
      //
      // User made a language setting change - display front page again
      //
      break;

    case FRONT_PAGE_KEY_BOOT_MANAGER:
      //
      // Remove the installed BootMaint HiiPackages when exit.
      //
      FreeBMPackage ();

      //
      // User chose to run the Boot Manager
      //
      CallBootManager ();

      //
      // Reinstall BootMaint HiiPackages after exiting from Boot Manager.
      //
      InitBMPackage ();
      break;

    case FRONT_PAGE_KEY_DEVICE_MANAGER:
      //
      // Display the Device Manager
      //
      do {
        CallDeviceManager ();
      } while (gCallbackKey == FRONT_PAGE_KEY_DEVICE_MANAGER);
      break;

    case FRONT_PAGE_KEY_BOOT_MAINTAIN:
      //
      // Display the Boot Maintenance Manager
      //
      BdsStartBootMaint ();
      break;
    }

  } while ((Status == EFI_SUCCESS) && (gCallbackKey != FRONT_PAGE_KEY_CONTINUE));

  if (mLanguageString != NULL) {
    FreePool (mLanguageString);
    mLanguageString = NULL;
  }
  //
  //Will leave browser, check any reset required change is applied? if yes, reset system
  //
  SetupResetReminder ();

  //
  // Remove the installed BootMaint HiiPackages when exit.
  //
  FreeBMPackage ();

Exit:
  //
  // Automatically load current entry
  // Note: The following lines of code only execute when Auto boot
  // takes affect
  //
  PERF_END (NULL, "BdsTimeOut", "BDS", 0);
}

/**
  This function will change video resolution and text mode
  according to defined setup mode or defined boot mode  

  @param  IsSetupMode   Indicate mode is changed to setup mode or boot mode. 

  @retval  EFI_SUCCESS  Mode is changed successfully.
  @retval  Others             Mode failed to be changed.

**/
EFI_STATUS
EFIAPI
BdsSetConsoleMode (
  BOOLEAN  IsSetupMode
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL       *SimpleTextOut;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  UINT32                                MaxGopMode;
  UINT32                                MaxTextMode;
  UINT32                                ModeNumber;
  UINT32                                NewHorizontalResolution;
  UINT32                                NewVerticalResolution;
  UINT32                                NewColumns;
  UINT32                                NewRows;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *HandleBuffer;
  EFI_STATUS                            Status;
  UINTN                                 Index;
  UINTN                                 CurrentColumn;
  UINTN                                 CurrentRow;  

  MaxGopMode  = 0;
  MaxTextMode = 0;

  //
  // Get current video resolution and text mode 
  //
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID**)&GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID**)&SimpleTextOut
                  );
  if (EFI_ERROR (Status)) {
    SimpleTextOut = NULL;
  }  

  if ((GraphicsOutput == NULL) || (SimpleTextOut == NULL)) {
    return EFI_UNSUPPORTED;
  }

  if (IsSetupMode) {
    //
    // The requried resolution and text mode is setup mode.
    //
    NewHorizontalResolution = mSetupHorizontalResolution;
    NewVerticalResolution   = mSetupVerticalResolution;
    NewColumns              = mSetupTextModeColumn;
    NewRows                 = mSetupTextModeRow;
  } else {
    //
    // The required resolution and text mode is boot mode.
    //
    NewHorizontalResolution = mBootHorizontalResolution;
    NewVerticalResolution   = mBootVerticalResolution;
    NewColumns              = mBootTextModeColumn;
    NewRows                 = mBootTextModeRow;   
  }
  
  if (GraphicsOutput != NULL) {
    MaxGopMode  = GraphicsOutput->Mode->MaxMode;
  } 

  if (SimpleTextOut != NULL) {
    MaxTextMode = SimpleTextOut->Mode->MaxMode;
  }

  //
  // 1. If current video resolution is same with required video resolution,
  //    video resolution need not be changed.
  //    1.1. If current text mode is same with required text mode, text mode need not be changed.
  //    1.2. If current text mode is different from required text mode, text mode need be changed.
  // 2. If current video resolution is different from required video resolution, we need restart whole console drivers.
  //
  for (ModeNumber = 0; ModeNumber < MaxGopMode; ModeNumber++) {
    Status = GraphicsOutput->QueryMode (
                       GraphicsOutput,
                       ModeNumber,
                       &SizeOfInfo,
                       &Info
                       );
    if (!EFI_ERROR (Status)) {
      if ((Info->HorizontalResolution == NewHorizontalResolution) &&
          (Info->VerticalResolution == NewVerticalResolution)) {
        if ((GraphicsOutput->Mode->Info->HorizontalResolution == NewHorizontalResolution) &&
            (GraphicsOutput->Mode->Info->VerticalResolution == NewVerticalResolution)) {
          //
          // Current resolution is same with required resolution, check if text mode need be set
          //
          Status = SimpleTextOut->QueryMode (SimpleTextOut, SimpleTextOut->Mode->Mode, &CurrentColumn, &CurrentRow);
          ASSERT_EFI_ERROR (Status);
          if (CurrentColumn == NewColumns && CurrentRow == NewRows) {
            //
            // If current text mode is same with required text mode. Do nothing
            //
            FreePool (Info);
            return EFI_SUCCESS;
          } else {
            //
            // If current text mode is different from requried text mode.  Set new video mode
            //
            for (Index = 0; Index < MaxTextMode; Index++) {
              Status = SimpleTextOut->QueryMode (SimpleTextOut, Index, &CurrentColumn, &CurrentRow);
              if (!EFI_ERROR(Status)) {
                if ((CurrentColumn == NewColumns) && (CurrentRow == NewRows)) {
                  //
                  // Required text mode is supported, set it.
                  //
                  Status = SimpleTextOut->SetMode (SimpleTextOut, Index);
                  ASSERT_EFI_ERROR (Status);
                  //
                  // Update text mode PCD.
                  //
                  PcdSet32 (PcdConOutColumn, mSetupTextModeColumn);
                  PcdSet32 (PcdConOutRow, mSetupTextModeRow);
                  FreePool (Info);
                  return EFI_SUCCESS;
                }
              }
            }
            if (Index == MaxTextMode) {
              //
              // If requried text mode is not supported, return error.
              //
              FreePool (Info);
              return EFI_UNSUPPORTED;
            }
          }
        } else {
          //
          // If current video resolution is not same with the new one, set new video resolution.
          // In this case, the driver which produces simple text out need be restarted.
          //
          Status = GraphicsOutput->SetMode (GraphicsOutput, ModeNumber);
          if (!EFI_ERROR (Status)) {
            FreePool (Info);
            break;
          }
        }
      }
      FreePool (Info);
    }
  }

  if (ModeNumber == MaxGopMode) {
    //
    // If the resolution is not supported, return error.
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Set PCD to Inform GraphicsConsole to change video resolution.
  // Set PCD to Inform Consplitter to change text mode.
  //
  PcdSet32 (PcdVideoHorizontalResolution, NewHorizontalResolution);
  PcdSet32 (PcdVideoVerticalResolution, NewVerticalResolution);
  PcdSet32 (PcdConOutColumn, NewColumns);
  PcdSet32 (PcdConOutRow, NewRows);
  
  
  //
  // Video mode is changed, so restart graphics console driver and higher level driver.
  // Reconnect graphics console driver and higher level driver.
  // Locate all the handles with GOP protocol and reconnect it.
  //
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiSimpleTextOutProtocolGuid,
                   NULL,
                   &HandleCount,
                   &HandleBuffer
                   );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
    }
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }
    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }
  }

  return EFI_SUCCESS;
}

