/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  FrontPage.c

Abstract:

  FrontPage routines to handle the callbacks and browser calls

--*/
#include "Bds.h"
#include "BdsPlatform.h"
#include "FrontPage.h"
#include "BdsString.h"

EFI_GUID                    mProcessorSubClass  = EFI_PROCESSOR_SUBCLASS_GUID;
EFI_GUID                    mMemorySubClass     = EFI_MEMORY_SUBCLASS_GUID;
EFI_GUID                    mMiscSubClass       = EFI_MISC_SUBCLASS_GUID;

UINT16                      mLastSelection;
EFI_HII_HANDLE              gFrontPageHandle;
EFI_HANDLE                  FrontPageCallbackHandle;
EFI_FORM_CALLBACK_PROTOCOL  FrontPageCallback;
EFI_FORM_BROWSER_PROTOCOL   *gBrowser;
UINTN                       gCallbackKey;
BOOLEAN                     gConnectAllHappened = FALSE;

extern EFI_HII_HANDLE       gFrontPageHandle;

EFI_STATUS
EFIAPI
FrontPageCallbackRoutine (
  IN EFI_FORM_CALLBACK_PROTOCOL       *This,
  IN UINT16                           KeyValue,
  IN EFI_IFR_DATA_ARRAY               *DataArray,
  OUT EFI_HII_CALLBACK_PACKET         **Packet
  )
/*++

Routine Description:

  This is the function that is called to provide results data to the driver.  This data
  consists of a unique key which is used to identify what data is either being passed back
  or being asked for.

Arguments:

  KeyValue -        A unique value which is sent to the original exporting driver so that it
                    can identify the type of data to expect.  The format of the data tends to
                    vary based on the op-code that geerated the callback.

  Data -            A pointer to the data being sent to the original exporting driver.

Returns:

--*/
{
  CHAR16        *LanguageString;
  UINTN         Count;
  CHAR16        UnicodeLang[3];
  CHAR8         Lang[3];
  EFI_STATUS    Status;
  UINTN         Index;
  CHAR16        *TmpStr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;

  SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
  SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
  SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);

  Count = 0;

  //
  // The first 4 entries in the Front Page are to be GUARANTEED to remain constant so IHV's can
  // describe to their customers in documentation how to find their setup information (namely
  // under the device manager and specific buckets)
  //
  switch (KeyValue) {
  case 0x0001:
    //
    // This is the continue - clear the screen and return an error to get out of FrontPage loop
    //
    gCallbackKey = 1;
    break;

  case 0x1234:
    //
    // Collect the languages from what our current Language support is based on our VFR
    //
    gHii->GetPrimaryLanguages (gHii, gFrontPageHandle, &LanguageString);

    //
    // Based on the DataArray->Data->Data value, we can determine
    // which language was chosen by the user
    //
    for (Index = 0; Count != (UINTN) (((EFI_IFR_DATA_ENTRY *) (DataArray + 1))->Data); Index += 3) {
      Count++;
    }
    //
    // Preserve the choice the user made
    //
    mLastSelection = (UINT16) Count;

    //
    // The Language (in Unicode format) the user chose
    //
    CopyMem (UnicodeLang, &LanguageString[Index], 6);

    //
    // Convert Unicode to ASCII (Since the ISO standard assumes ASCII equivalent abbreviations
    // we can be safe in converting this Unicode stream to ASCII without any loss in meaning.
    //
    for (Index = 0; Index < 3; Index++) {
      Lang[Index] = (CHAR8) UnicodeLang[Index];
    }

    Status = gRT->SetVariable (
                    L"Lang",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    3,
                    Lang
                    );

    FreePool (LanguageString);
    gCallbackKey = 2;
    break;

  case 0x1064:
    //
    // Boot Manager
    //
    gCallbackKey = 3;
    break;

  case 0x8567:
    //
    // Device Manager
    //
    gCallbackKey = 4;
    break;

  case 0x9876:
    //
    // Boot Maintenance Manager
    //
    gCallbackKey = 5;
    break;

  case 0xFFFE:

    break;

  case 0xFFFF:
    //
    // FrontPage TimeOut Callback
    //
    TmpStr = GetStringById (STRING_TOKEN (STR_START_BOOT_OPTION));
    if (TmpStr != NULL) {
      PlatformBdsShowProgress (
        Foreground,
        Background,
        TmpStr,
        Color,
        (UINTN) (((EFI_IFR_DATA_ENTRY *) (DataArray+1))->Data),
        0
        );
      FreePool (TmpStr);
    }
    break;

  default:
    gCallbackKey = 0;
    break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InitializeFrontPage (
  BOOLEAN                         ReInitializeStrings
  )
/*++

Routine Description:

  Initialize HII information for the FrontPage

Arguments:
  None

Returns:
  EFI_SUCCESS       - The operation is successful.
  EFI_DEVICE_ERROR  - If the dynamic opcode creation failed.

--*/
{
  EFI_STATUS          Status;
  EFI_HII_PACKAGES    *PackageList;
  EFI_HII_UPDATE_DATA *UpdateData;
  IFR_OPTION          *OptionList;
  CHAR16              *LanguageString;
  UINTN               OptionCount;
  UINTN               Index;
  STRING_REF          Token;
  UINT16              Key;
  CHAR8               AsciiLang[4];
  CHAR16              UnicodeLang[4];
  CHAR16              Lang[4];
  CHAR16              *StringBuffer;
  UINTN               BufferSize;
  UINT8               *TempBuffer;

  UpdateData  = NULL;
  OptionList  = NULL;

  if (ReInitializeStrings) {
    //
    // BugBug: Dont' use a goto
    //
    goto ReInitStrings;
  }
  //
  // Go ahead and initialize the Device Manager
  //
  InitializeDeviceManager ();

  //
  // BugBug: if FrontPageVfrBin is generated by a tool, why are we patching it here
  //
  TempBuffer    = (UINT8 *) FrontPageVfrBin;
  TempBuffer    = TempBuffer + sizeof (EFI_HII_PACK_HEADER);
  TempBuffer    = (UINT8 *) &((EFI_IFR_FORM_SET *) TempBuffer)->NvDataSize;
  *TempBuffer   = 1;

  gCallbackKey  = 0;

  PackageList   = PreparePackages (1, &gEfiCallerIdGuid, FrontPageVfrBin);

  Status        = gHii->NewPack (gHii, PackageList, &gFrontPageHandle);

  FreePool (PackageList);

  //
  // There will be only one FormConfig in the system
  // If there is another out there, someone is trying to install us
  // again.  Fail that scenario.
  //
  Status = gBS->LocateProtocol (
                  &gEfiFormBrowserProtocolGuid,
                  NULL,
                  &gBrowser
                  );

  //
  // This example does not implement worker functions
  // for the NV accessor functions.  Only a callback evaluator
  //
  FrontPageCallback.NvRead    = NULL;
  FrontPageCallback.NvWrite   = NULL;
  FrontPageCallback.Callback  = FrontPageCallbackRoutine;

  //
  // Install protocol interface
  //
  FrontPageCallbackHandle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &FrontPageCallbackHandle,
                  &gEfiFormCallbackProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &FrontPageCallback
                  );
  ASSERT_EFI_ERROR (Status);

ReInitStrings:
  //
  // BugBug: This logic is in BdsInitLanguage. It should not be in two places!
  //
  BufferSize = 4;
  Status = gRT->GetVariable (
                  L"Lang",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &BufferSize,
                  AsciiLang
                  );

  for (Index = 0; Index < 3; Index++) {
    UnicodeLang[Index] = (CHAR16) AsciiLang[Index];
  }

  UnicodeLang[3] = 0;

  //
  // Allocate space for creation of UpdateData Buffer
  //
  UpdateData = AllocateZeroPool (0x1000);
  ASSERT (UpdateData != NULL);

  OptionList = AllocateZeroPool (0x1000);
  ASSERT (OptionList != NULL);

  //
  // Flag update pending in FormSet
  //
  UpdateData->FormSetUpdate = TRUE;
  //
  // Register CallbackHandle data for FormSet
  //
  UpdateData->FormCallbackHandle = (EFI_PHYSICAL_ADDRESS) (UINTN) FrontPageCallbackHandle;
  UpdateData->FormUpdate  = FALSE;
  UpdateData->FormTitle   = 0;
  UpdateData->DataCount   = 1;

  //
  // Collect the languages from what our current Language support is based on our VFR
  //
  gHii->GetPrimaryLanguages (gHii, gFrontPageHandle, &LanguageString);

  OptionCount = 0;

  for (Index = 0; LanguageString[Index] != 0; Index += 3) {
    Token = 0;
    CopyMem (Lang, &LanguageString[Index], 6);
    Lang[3] = 0;

    if (!StrCmp (Lang, UnicodeLang)) {
      mLastSelection = (UINT16) OptionCount;
    }

    BufferSize = 0;
    Status = gHii->GetString (gHii, gStringPackHandle, 1, TRUE, Lang, &BufferSize, NULL);
    ASSERT (Status == EFI_BUFFER_TOO_SMALL);
    StringBuffer = AllocateZeroPool (BufferSize);
    ASSERT (StringBuffer != NULL);
    Status = gHii->GetString (gHii, gStringPackHandle, 1, TRUE, Lang, &BufferSize, StringBuffer);
    ASSERT_EFI_ERROR (Status);
    gHii->NewString (gHii, NULL, gStringPackHandle, &Token, StringBuffer);
    FreePool (StringBuffer);
    CopyMem (&OptionList[OptionCount].StringToken, &Token, sizeof (UINT16));
    CopyMem (&OptionList[OptionCount].Value, &OptionCount, sizeof (UINT16));
    Key = 0x1234;
    CopyMem (&OptionList[OptionCount].Key, &Key, sizeof (UINT16));
    OptionList[OptionCount].Flags = EFI_IFR_FLAG_INTERACTIVE | EFI_IFR_FLAG_NV_ACCESS;
    OptionCount++;
  }

  FreePool (LanguageString);

  if (ReInitializeStrings) {
    FreePool (OptionList);
    return EFI_SUCCESS;
  }

  Status = CreateOneOfOpCode (
            FRONT_PAGE_QUESTION_ID,                               // Question ID
            FRONT_PAGE_DATA_WIDTH,                                // Data Width
            (STRING_REF) STRING_TOKEN (STR_LANGUAGE_SELECT),      // Prompt Token
            (STRING_REF) STRING_TOKEN (STR_LANGUAGE_SELECT_HELP), // Help Token
            OptionList,       // List of Options
            OptionCount,      // Number of Options
            &UpdateData->Data // Data Buffer
            );

  //
  // Assign the number of options and the oneof and endoneof op-codes to count
  //
  UpdateData->DataCount = (UINT8) (OptionCount + 2);

  gHii->UpdateForm (gHii, gFrontPageHandle, (EFI_FORM_LABEL) 0x0002, TRUE, UpdateData);

  FreePool (UpdateData);
  FreePool (OptionList);
  return Status;
}

EFI_STATUS
CallFrontPage (
  VOID
  )
/*++

Routine Description:

  Call the browser and display the front page

Arguments:

  None

Returns:

--*/
{
  EFI_STATUS  Status;
  UINT8       FakeNvRamMap[1];
  BOOLEAN     FrontPageMenuResetRequired;

  //
  // Begin waiting for USER INPUT
  //
  REPORT_STATUS_CODE (
        EFI_PROGRESS_CODE,
        (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_INPUT_WAIT)
        );

  FakeNvRamMap[0] = (UINT8) mLastSelection;
  FrontPageMenuResetRequired = FALSE;
  Status = gBrowser->SendForm (
                      gBrowser,
                      TRUE,                     // Use the database
                      &gFrontPageHandle,        // The HII Handle
                      1,
                      NULL,
                      FrontPageCallbackHandle,  // This is the handle that the interface to the callback was installed on
                      FakeNvRamMap,
                      NULL,
                      &FrontPageMenuResetRequired
                      );
  //
  // Check whether user change any option setting which needs a reset to be effective
  //
  if (FrontPageMenuResetRequired) {
    EnableResetRequired ();
  }

  gHii->ResetStrings (gHii, gFrontPageHandle);

  return Status;
}

EFI_STATUS
GetStringFromToken (
  IN      EFI_GUID                  *ProducerGuid,
  IN      STRING_REF                Token,
  OUT     CHAR16                    **String
  )
/*++

Routine Description:

  Acquire the string associated with the ProducerGuid and return it.

Arguments:

  ProducerGuid - The Guid to search the HII database for
  Token - The token value of the string to extract
  String - The string that is extracted

Returns:

  EFI_SUCCESS - The function returns EFI_SUCCESS always.

--*/
{
  EFI_STATUS      Status;
  UINT16          HandleBufferLength;
  EFI_HII_HANDLE  *HiiHandleBuffer;
  UINTN           StringBufferLength;
  UINTN           NumberOfHiiHandles;
  UINTN           Index;
  UINT16          Length;
  EFI_GUID        HiiGuid;

  //
  // Initialize params.
  //
  HandleBufferLength  = 0;
  HiiHandleBuffer     = NULL;

  //
  // Get all the Hii handles
  //
  Status = BdsLibGetHiiHandles (gHii, &HandleBufferLength, &HiiHandleBuffer);
  ASSERT_EFI_ERROR (Status);

  //
  // Get the gHii Handle that matches the StructureNode->ProducerName
  //
  NumberOfHiiHandles = HandleBufferLength / sizeof (EFI_HII_HANDLE);
  for (Index = 0; Index < NumberOfHiiHandles; Index++) {
    Length = 0;
    Status = ExtractDataFromHiiHandle (
              HiiHandleBuffer[Index],
              &Length,
              NULL,
              &HiiGuid
              );
    if (CompareGuid (ProducerGuid, &HiiGuid)) {
      break;
    }
  }
  //
  // Find the string based on the current language
  //
  StringBufferLength  = 0x100;
  *String             = AllocateZeroPool (0x100);
  Status = gHii->GetString (
                  gHii,
                  HiiHandleBuffer[Index],
                  Token,
                  FALSE,
                  NULL,
                  &StringBufferLength,
                  *String
                  );

  if (EFI_ERROR (Status)) {
    FreePool (*String);
    *String = GetStringById (STRING_TOKEN (STR_MISSING_STRING));
  }

  FreePool (HiiHandleBuffer);
  return EFI_SUCCESS;
}

VOID
ConvertProcessorToString (
  IN  EFI_PROCESSOR_CORE_FREQUENCY_DATA *ProcessorFrequency,
  OUT CHAR16                            **String
  )
/*++

Routine Description:

  Convert Processor Frequency Data to a string

Arguments:

  ProcessorFrequency - The frequency data to process
  String - The string that is created

Returns:

--*/
{
  CHAR16  *StringBuffer;
  UINTN   Index;
  UINT32  FreqMhz;

  if (ProcessorFrequency->Exponent >= 6) {
    FreqMhz = ProcessorFrequency->Value;
    for (Index = 0; Index < (UINTN) (ProcessorFrequency->Exponent - 6); Index++) {
      FreqMhz *= 10;
    }
  } else {
    FreqMhz = 0;
  }

  StringBuffer = AllocateZeroPool (0x20);
  ASSERT (StringBuffer != NULL);
  Index = UnicodeValueToString (StringBuffer, LEFT_JUSTIFY, FreqMhz / 1000, 3);
  StrCat (StringBuffer, L".");
  UnicodeValueToString (StringBuffer + Index + 1, PREFIX_ZERO, (FreqMhz % 1000) / 10, 2);
  StrCat (StringBuffer, L" GHz");

  *String = (CHAR16 *) StringBuffer;

  return ;
}

VOID
ConvertMemorySizeToString (
  IN  UINT32          MemorySize,
  OUT CHAR16          **String
  )
/*++

Routine Description:

  Convert Memory Size to a string

Arguments:

  MemorySize - The size of the memory to process
  String - The string that is created

Returns:

--*/
{
  CHAR16  *StringBuffer;

  StringBuffer = AllocateZeroPool (0x20);
  ASSERT (StringBuffer != NULL);
  UnicodeValueToString (StringBuffer, LEFT_JUSTIFY, MemorySize, 6);
  StrCat (StringBuffer, L" MB RAM");

  *String = (CHAR16 *) StringBuffer;

  return ;
}

VOID
UpdateFrontPageStrings (
  VOID
  )
/*++

Routine Description:

  Update the banner information for the Front Page based on DataHub information

Arguments:

  None

Returns:

--*/
{
  EFI_STATUS                        Status;
  STRING_REF                        TokenToUpdate;
  CHAR16                            *NewString;
  UINT64                            MonotonicCount;
  EFI_DATA_HUB_PROTOCOL             *DataHub;
  EFI_DATA_RECORD_HEADER            *Record;
  EFI_SUBCLASS_TYPE1_HEADER         *DataHeader;
  EFI_MISC_BIOS_VENDOR_DATA         *BiosVendor;
  EFI_MISC_SYSTEM_MANUFACTURER_DATA *SystemManufacturer;
  EFI_PROCESSOR_VERSION_DATA        *ProcessorVersion;
  EFI_PROCESSOR_CORE_FREQUENCY_DATA *ProcessorFrequency;
  EFI_MEMORY_ARRAY_START_ADDRESS_DATA *MemoryArray;
  CHAR8                             LangCode[3];
  CHAR16                            Lang[3];
  UINTN                             Size;
  UINTN                             Index;
  BOOLEAN                           Find[5];

  ZeroMem (Find, sizeof (Find));

  //
  // Update Front Page strings
  //
  Status = gBS->LocateProtocol (
                  &gEfiDataHubProtocolGuid,
                  NULL,
                  &DataHub
                  );
  ASSERT_EFI_ERROR (Status);

  Size = 3;

  Status = gRT->GetVariable (
                  L"Lang",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &Size,
                  LangCode
                  );

  for (Index = 0; Index < 3; Index++) {
    Lang[Index] = (CHAR16) LangCode[Index];
  }

  MonotonicCount  = 0;
  Record          = NULL;
  do {
    Status = DataHub->GetNextRecord (DataHub, &MonotonicCount, NULL, &Record);
    if (Record->DataRecordClass == EFI_DATA_RECORD_CLASS_DATA) {
      DataHeader = (EFI_SUBCLASS_TYPE1_HEADER *) (Record + 1);
      if (CompareGuid (&Record->DataRecordGuid, &mMiscSubClass) &&
          (DataHeader->RecordType == EFI_MISC_BIOS_VENDOR_RECORD_NUMBER)
          ) {
        BiosVendor = (EFI_MISC_BIOS_VENDOR_DATA *) (DataHeader + 1);
        GetStringFromToken (&Record->ProducerName, BiosVendor->BiosVersion, &NewString);
        TokenToUpdate = (STRING_REF) STR_FRONT_PAGE_BIOS_VERSION;
        gHii->NewString (gHii, Lang, gFrontPageHandle, &TokenToUpdate, NewString);
        FreePool (NewString);
        Find[0] = TRUE;
      }

      if (CompareGuid (&Record->DataRecordGuid, &mMiscSubClass) &&
          (DataHeader->RecordType == EFI_MISC_SYSTEM_MANUFACTURER_RECORD_NUMBER)
          ) {
        SystemManufacturer = (EFI_MISC_SYSTEM_MANUFACTURER_DATA *) (DataHeader + 1);
        GetStringFromToken (&Record->ProducerName, SystemManufacturer->SystemProductName, &NewString);
        TokenToUpdate = (STRING_REF) STR_FRONT_PAGE_COMPUTER_MODEL;
        gHii->NewString (gHii, Lang, gFrontPageHandle, &TokenToUpdate, NewString);
        FreePool (NewString);
        Find[1] = TRUE;
      }

      if (CompareGuid (&Record->DataRecordGuid, &mProcessorSubClass) &&
          (DataHeader->RecordType == ProcessorVersionRecordType)
          ) {
        ProcessorVersion = (EFI_PROCESSOR_VERSION_DATA *) (DataHeader + 1);
        GetStringFromToken (&Record->ProducerName, *ProcessorVersion, &NewString);
        TokenToUpdate = (STRING_REF) STR_FRONT_PAGE_CPU_MODEL;
        gHii->NewString (gHii, Lang, gFrontPageHandle, &TokenToUpdate, NewString);
        FreePool (NewString);
        Find[2] = TRUE;
      }

      if (CompareGuid (&Record->DataRecordGuid, &mProcessorSubClass) &&
          (DataHeader->RecordType == ProcessorCoreFrequencyRecordType)
          ) {
        ProcessorFrequency = (EFI_PROCESSOR_CORE_FREQUENCY_DATA *) (DataHeader + 1);
        ConvertProcessorToString (ProcessorFrequency, &NewString);
        TokenToUpdate = (STRING_REF) STR_FRONT_PAGE_CPU_SPEED;
        gHii->NewString (gHii, Lang, gFrontPageHandle, &TokenToUpdate, NewString);
        FreePool (NewString);
        Find[3] = TRUE;
      }

      if (CompareGuid (&Record->DataRecordGuid, &mMemorySubClass) &&
          (DataHeader->RecordType == EFI_MEMORY_ARRAY_START_ADDRESS_RECORD_NUMBER)
          ) {
        MemoryArray = (EFI_MEMORY_ARRAY_START_ADDRESS_DATA *) (DataHeader + 1);
        ConvertMemorySizeToString((UINT32)(RShiftU64((MemoryArray->MemoryArrayEndAddress -
                                  MemoryArray->MemoryArrayStartAddress + 1), 20)),
                                  &NewString);
        TokenToUpdate = (STRING_REF) STR_FRONT_PAGE_MEMORY_SIZE;
        gHii->NewString (gHii, Lang, gFrontPageHandle, &TokenToUpdate, NewString);
        FreePool (NewString);
        Find[4] = TRUE;
      }
    }
  } while (!EFI_ERROR (Status) && (MonotonicCount != 0) && !(Find[0] && Find[1] && Find[2] && Find[3] && Find[4]));

  return ;
}

VOID
PlatformBdsEnterFrontPage (
  IN UINT16                       TimeoutDefault,
  IN BOOLEAN                      ConnectAllHappened
  )
/*++

Routine Description:
  This function is the main entry of the platform setup entry.
  The function will present the main menu of the system setup,
  this is the platform reference part and can be customize.

Arguments:
  TimeoutDefault     - The fault time out value before the system
                       continue to boot.
  ConnectAllHappened - The indicater to check if the connect all have
                       already happended.

Returns:
  None

--*/
{
  EFI_STATUS                    Status;
  EFI_HII_UPDATE_DATA           *UpdateData;
  EFI_CONSOLE_CONTROL_PROTOCOL  *ConsoleControl;

  //
  // Indicate if we need connect all in the platform setup
  //
  if (ConnectAllHappened) {
    gConnectAllHappened = TRUE;
  }
  //
  // Allocate space for creation of Buffer
  //
  UpdateData = AllocateZeroPool (0x1000);
  ASSERT (UpdateData != NULL);

  UpdateData->FormSetUpdate       = FALSE;
  UpdateData->FormCallbackHandle  = 0;
  UpdateData->FormUpdate          = FALSE;
  UpdateData->FormTitle           = 0;
  UpdateData->DataCount           = 1;

  //
  // Remove Banner Op-code if any at this label
  //
  gHii->UpdateForm (gHii, gFrontPageHandle, (EFI_FORM_LABEL) 0xFFFF, FALSE, UpdateData);

  //
  // Create Banner Op-code which reflects correct timeout value
  //
  CreateBannerOpCode (
    STRING_TOKEN (STR_TIME_OUT_PROMPT),
    TimeoutDefault,
    (UINT8) EFI_IFR_BANNER_TIMEOUT,
    &UpdateData->Data
    );

  //
  // Add Banner Op-code at this label
  //
  gHii->UpdateForm (gHii, gFrontPageHandle, (EFI_FORM_LABEL) 0xFFFF, TRUE, UpdateData);

  do {

    InitializeFrontPage (TRUE);

    //
    // Update Front Page strings
    //
    UpdateFrontPageStrings ();

    gCallbackKey = 0;
    PERF_START (0, "BdsTimeOut", "BDS", 0);
    Status = CallFrontPage ();
    PERF_END (0, "BdsTimeOut", "BDS", 0);

    //
    // If gCallbackKey is greater than 1 and less or equal to 5,
    // it will lauch configuration utilities.
    // 2 = set language
    // 3 = boot manager
    // 4 = device manager
    // 5 = boot maintainenance manager
    //
    if ((gCallbackKey > 0x0001) && (gCallbackKey <= 0x0005)) {
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
    case 0x0001:
      //
      // User hit continue
      //
      break;

    case 0x0002:
      //
      // User made a language setting change - display front page again
      //
      break;

    case 0x0003:
      //
      // User chose to run the Boot Manager
      //
      CallBootManager ();
      break;

    case 0x0004:
      //
      // Display the Device Manager
      //
      do {
        CallDeviceManager();
      } while (gCallbackKey == 4);
      break;

    case 0x0005:
      //
      // Display the Boot Maintenance Manager
      //
      BdsStartBootMaint ();
      break;
    }

  } while ((Status == EFI_SUCCESS) && (gCallbackKey != 1));

  //
  //Will leave browser, check any reset required change is applied? if yes, reset system
  //
  SetupResetReminder ();

  //
  // Automatically load current entry
  // Note: The following lines of code only execute when Auto boot
  // takes affect
  //
  Status = gBS->LocateProtocol (&gEfiConsoleControlProtocolGuid, NULL, &ConsoleControl);
  ConsoleControl->SetMode (ConsoleControl, EfiConsoleControlScreenText);

}
