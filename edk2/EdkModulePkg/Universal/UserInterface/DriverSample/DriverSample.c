/*++
Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  DriverSample.c

Abstract:

  This is an example of how a driver might export data to the HII protocol to be 
  later utilized by the Setup Protocol

--*/

#include "DriverSample.h"

#define DISPLAY_ONLY_MY_ITEM  0x0001

#define STRING_PACK_GUID \
  { \
    0x8160a85f, 0x934d, 0x468b, { 0xa2, 0x35, 0x72, 0x89, 0x59, 0x14, 0xf6, 0xfc } \
  }

EFI_GUID  mFormSetGuid    = FORMSET_GUID;
EFI_GUID  mStringPackGuid = STRING_PACK_GUID; 

STATIC
EFI_STATUS
EFIAPI
DriverCallback (
  IN EFI_FORM_CALLBACK_PROTOCOL       *This,
  IN UINT16                           KeyValue,
  IN EFI_IFR_DATA_ARRAY               *Data,
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
  EFI_CALLBACK_INFO       *Private;
  EFI_HII_UPDATE_DATA     *UpdateData;
  UINT8                   *Location;
  EFI_HII_CALLBACK_PACKET *DataPacket;
  UINT16                  Value;
  CHAR16                  VariableName[40];
  STATIC UINT16           QuestionId = 0;
  IFR_OPTION              *OptionList;
  UINTN                   Index;
  MyIfrNVData             NVStruc;

  Private     = EFI_CALLBACK_INFO_FROM_THIS (This);

  //
  // This should tell me the first offset AFTER the end of the compiled NV map
  // If op-code results are not going to be saved to NV locations ensure the QuestionId
  // is beyond the end of the NVRAM mapping.
  //
  if (QuestionId == 0) {
    QuestionId = sizeof (MyIfrNVData);
  }

  ZeroMem (VariableName, (sizeof (CHAR16) * 40));

  switch (KeyValue) {
  case 0x0001:
    //
    // Create a small boot order list
    //
    QuestionId = (UINT16) ((UINTN) (&NVStruc.BootOrder) - (UINTN) (&NVStruc));

    //
    // Need some memory for OptionList. Allow for up to 8 options.
    //
    OptionList = AllocateZeroPool (sizeof (IFR_OPTION) * 8);
    ASSERT (OptionList != NULL);

    //
    // Allocate space for creation of Buffer
    //
    UpdateData = AllocateZeroPool (0x1000);
    ASSERT (UpdateData != NULL);

    //
    // Remove all the op-codes starting with Label 0x2222 to next Label (second label is for convenience
    // so we don't have to keep track of how many op-codes we added or subtracted.  The rules for removal
    // of op-codes are simply that the removal will always stop as soon as a label or the end of a form is
    // encountered.  Therefore, giving a large obnoxious count such as below takes care of other complexities.
    //
    UpdateData->DataCount = 0xFF;

    //
    // Delete set of op-codes
    //
    Private->Hii->UpdateForm (
                    Private->Hii,
                    Private->RegisteredHandle,
                    (EFI_FORM_LABEL) 0x2222,
                    FALSE,  // If we aren't adding, we are deleting
                    UpdateData
                    );

    //
    // Create 3 options
    //
    for (Index = 0; Index < 3; Index++) {
      OptionList[Index].StringToken = (UINT16) (STR_BOOT_OPTION1 + Index);
      OptionList[Index].Value       = (UINT16) (Index + 1);
      OptionList[Index].Flags       = RESET_REQUIRED;
    }

    CreateOrderedListOpCode (
      QuestionId,                               // Question ID
      8,                                        // Max Entries
      (UINT16) STRING_TOKEN (STR_BOOT_OPTIONS), // Token value for the Prompt
      (UINT16) STRING_TOKEN (STR_NULL_STRING),  // Token value for the Help
      OptionList,
      3,
      &UpdateData->Data                         // Buffer location to place op-codes
      );

    //
    // For one-of/ordered lists commands, they really consist of 2 op-codes (a header and a footer)
    // Each option within a one-of/ordered list is also an op-code
    // So this example has 5 op-codes it is adding since we have a one-of header + 3 options + one-of footer
    //
    UpdateData->DataCount = 0x5;

    //
    // Add one op-code
    //
    Private->Hii->UpdateForm (
                    Private->Hii,
                    Private->RegisteredHandle,
                    (EFI_FORM_LABEL) 0x2222,
                    TRUE,
                    UpdateData
                    );

    FreePool (UpdateData);
    FreePool (OptionList);
    break;

  case 0x0002:
    //
    // Create a large boot order list
    //
    QuestionId = (UINT16) ((UINTN) (&NVStruc.BootOrder) - (UINTN) (&NVStruc));

    //
    // Need some memory for OptionList. Allow for up to 8 options.
    //
    OptionList = AllocateZeroPool (sizeof (IFR_OPTION) * 8);
    ASSERT (OptionList != NULL);

    //
    // Allocate space for creation of Buffer
    //
    UpdateData = AllocateZeroPool (0x1000);
    ASSERT (UpdateData != NULL);

    //
    // Remove all the op-codes starting with Label 0x2222 to next Label (second label is for convenience
    // so we don't have to keep track of how many op-codes we added or subtracted
    //
    UpdateData->DataCount = 0xFF;

    //
    // Delete one op-code
    //
    Private->Hii->UpdateForm (
                    Private->Hii,
                    Private->RegisteredHandle,
                    (EFI_FORM_LABEL) 0x2222,
                    FALSE,
                    UpdateData
                    );

    //
    // Create 4 options
    //
    for (Index = 0; Index < 4; Index++) {
      OptionList[Index].StringToken = (UINT16) (STR_BOOT_OPTION1 + Index);
      OptionList[Index].Value       = (UINT16) (Index + 1);
      OptionList[Index].Flags       = RESET_REQUIRED;
    }

    CreateOrderedListOpCode (
      QuestionId,                               // Question ID
      8,                                        // Max Entries
      (UINT16) STRING_TOKEN (STR_BOOT_OPTIONS), // Token value for the Prompt
      (UINT16) STRING_TOKEN (STR_NULL_STRING),  // Token value for the Help
      OptionList,
      4,
      &UpdateData->Data                         // Buffer location to place op-codes
      );

    //
    // For one-of commands, they really consist of 2 op-codes (a header and a footer)
    // Each option within a one-of is also an op-code
    // So this example has 6 op-codes it is adding since we have a one-of header + 4 options + one-of footer
    //
    UpdateData->DataCount = 0x6;

    //
    // Add one op-code
    //
    Private->Hii->UpdateForm (
                    Private->Hii,
                    Private->RegisteredHandle,
                    (EFI_FORM_LABEL) 0x2222,
                    TRUE,
                    UpdateData
                    );

    FreePool (UpdateData);
    FreePool (OptionList);
    break;

  case 0x1234:
    //
    // Allocate space for creation of Buffer
    //
    QuestionId = (UINT16) ((UINTN) (&NVStruc.DynamicCheck) - (UINTN) (&NVStruc));
    UpdateData = AllocateZeroPool (0x1000);
    ASSERT (UpdateData != NULL);

    Location                        = (UINT8 *) &UpdateData->Data;

    UpdateData->FormSetUpdate       = TRUE;
    UpdateData->FormCallbackHandle  = (EFI_PHYSICAL_ADDRESS) (UINTN) Private->CallbackHandle;
    UpdateData->FormUpdate          = FALSE;
    UpdateData->FormTitle           = 0;
    UpdateData->DataCount           = 2;

    CreateGotoOpCode (
      1,
      STR_GOTO_FORM1,                                   // Token value for the Prompt
      0,                                                // Goto Help
      0,                                                // Flags
      0,                                                // Key
      &UpdateData->Data                                 // Buffer location to place op-codes
      );

    Location = Location + ((EFI_IFR_OP_HEADER *) &UpdateData->Data)->Length;

    CreateCheckBoxOpCode (
      QuestionId,                                       // Question ID
      1,                                                // Data width (BOOLEAN = 1)
      (UINT16) STRING_TOKEN (STR_CHECK_DYNAMIC_PROMPT), // Token value for the Prompt
      (UINT16) STRING_TOKEN (STR_CHECK_DYNAMIC_HELP),   // Token value for the Help
      EFI_IFR_FLAG_INTERACTIVE,                         // Flags
      0x1236,   // Key
      Location  // Buffer location to place op-codes
      );

    Private->Hii->UpdateForm (
                    Private->Hii,
                    Private->RegisteredHandle,
                    (EFI_FORM_LABEL) 0x1234,
                    TRUE,
                    UpdateData
                    );

    FreePool (UpdateData);
    QuestionId++;
    break;

  case 0x1235:
    //
    // Allocate space for creation of Buffer
    //
    UpdateData = AllocateZeroPool (0x1000);
    ASSERT (UpdateData != NULL);

    //
    // Initialize DataPacket with information intended to remove all
    // previously created op-codes in the dynamic page
    //
    UpdateData->FormSetUpdate       = FALSE;
    UpdateData->FormCallbackHandle  = 0;
    UpdateData->FormUpdate          = FALSE;
    UpdateData->FormTitle           = 0;
    //
    // Unlikely to be more than 0xff op-codes in the dynamic page to remove
    //
    UpdateData->DataCount           = 0xff;
    UpdateData->Data = NULL;

    //
    // Remove all op-codes from dynamic page
    //
    Private->Hii->UpdateForm (
                    Private->Hii,
                    Private->RegisteredHandle,
                    (EFI_FORM_LABEL) 0x1234,  // Label 0x1234
                    FALSE,                    // Remove Op-codes (will never remove form/endform)
                    UpdateData                // Significant value is UpdateData->DataCount
                    );

    UpdateData->FormSetUpdate       = FALSE;
    UpdateData->FormCallbackHandle  = 0;
    UpdateData->FormUpdate          = FALSE;
    UpdateData->FormTitle           = 0;
    UpdateData->DataCount           = 1;

    CreateGotoOpCode (
      1,
      STR_GOTO_FORM1,                         // Token value for the Prompt
      0,                                      // Goto Help
      0,                                      // Flags
      0,                                      // Key
      &UpdateData->Data                       // Buffer location to place op-codes
      );

    Private->Hii->UpdateForm (
                    Private->Hii,
                    Private->RegisteredHandle,
                    (EFI_FORM_LABEL) 0x1234,
                    TRUE,
                    UpdateData
                    );

    FreePool (UpdateData);
    break;

  case 0x1236:
    //
    // If I hit the checkbox, I enter this case statement...
    //
    //
    // Since I am returning an error (for test purposes) I need to pass in the string for the error
    // I will allocate space for the return value.  If an error occurs (which is the case) I can simply return
    // an error and fill in the string parameter, otherwise, I will return information in the DataArray structure.
    // The browser will free this packet structure
    //
    *Packet = AllocateZeroPool (sizeof (EFI_HII_CALLBACK_PACKET) + sizeof (SAMPLE_STRING) + 2);
    ASSERT (*Packet != NULL);

    //
    // Assign the buffer address to DataPacket
    //
    DataPacket = *Packet;

    StrCpy (DataPacket->String, (CHAR16 *) SAMPLE_STRING);
    return EFI_DEVICE_ERROR;

  case 0x1237:

    *Packet = AllocateZeroPool (sizeof (EFI_HII_CALLBACK_PACKET) + 2);
    ASSERT (*Packet != NULL);

    //
    // Assign the buffer address to DataPacket
    //
    DataPacket                        = *Packet;

    DataPacket->DataArray.EntryCount  = 1;
    DataPacket->DataArray.NvRamMap    = NULL;
    ((EFI_IFR_DATA_ENTRY *) (&DataPacket->DataArray + 1))->Flags = EXIT_REQUIRED;
    break;

  case 0x1555:
    Value = 0x0001;
    UnicodeSPrint (VariableName, 0x80, (CHAR16 *) L"%d", VAR_EQ_TEST_NAME);

    gRT->SetVariable (
          VariableName,
          &mFormSetGuid,
          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
          2,
          (VOID *) &Value
          );
    break;

  case 0x1556:
    Value = 0x1000;
    UnicodeSPrint (VariableName, 0x80, (CHAR16 *) L"%d", VAR_EQ_TEST_NAME);

    gRT->SetVariable (
          VariableName,
          &mFormSetGuid,
          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
          2,
          (VOID *) &Value
          );
    break;

  case 0x1557:
    Value = 0x0000;
    UnicodeSPrint (VariableName, 0x80, (CHAR16 *) L"%d", VAR_EQ_TEST_NAME);

    gRT->SetVariable (
          VariableName,
          &mFormSetGuid,
          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
          2,
          (VOID *) &Value
          );
    break;

  default:
    break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DriverSampleInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS          Status;
  EFI_HII_PROTOCOL    *Hii;
  //
  //  EFI_FORM_BROWSER_PROTOCOL       *FormConfig;
  //
  EFI_HII_PACKAGES    *PackageList;
  EFI_HII_HANDLE      HiiHandle;
  STRING_REF          TokenToUpdate;
  STRING_REF          TokenToUpdate2;
  STRING_REF          TokenToUpdate3;
  CHAR16              *NewString;
  EFI_HII_UPDATE_DATA *UpdateData;
  EFI_CALLBACK_INFO   *CallbackInfo;
  EFI_HANDLE          Handle;
  EFI_SCREEN_DESCRIPTOR   Screen;

  ZeroMem (&Screen, sizeof (EFI_SCREEN_DESCRIPTOR));

  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &Screen.RightColumn, &Screen.BottomRow);

  //
  // Remove 3 characters from top and bottom
  //
  Screen.TopRow     = 3;
  Screen.BottomRow  = Screen.BottomRow - 3;

  //
  // There should only be one HII protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiProtocolGuid,
                  NULL,
                  (VOID **) &Hii
                  );
  if (EFI_ERROR (Status)) {
    return Status;;
  }

  CallbackInfo = AllocatePool (sizeof (EFI_CALLBACK_INFO));
  if (CallbackInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CallbackInfo->Signature = EFI_CALLBACK_INFO_SIGNATURE;
  CallbackInfo->Hii       = Hii;

  //
  // This example does not implement worker functions for the NV accessor functions.  Only a callback evaluator
  //
  CallbackInfo->DriverCallback.NvRead   = NULL;
  CallbackInfo->DriverCallback.NvWrite  = NULL;
  CallbackInfo->DriverCallback.Callback = DriverCallback;

  //
  // Install protocol interface
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiFormCallbackProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &CallbackInfo->DriverCallback
                  );

  ASSERT_EFI_ERROR (Status);

  CallbackInfo->CallbackHandle  = Handle;

  PackageList                   = PreparePackages (1, &mStringPackGuid, DriverSampleStrings);
  Status                        = Hii->NewPack (Hii, PackageList, &HiiHandle);
  FreePool (PackageList);

  PackageList = PreparePackages (1, &mStringPackGuid, InventoryBin);
  Status      = Hii->NewPack (Hii, PackageList, &HiiHandle);
  FreePool (PackageList);

  PackageList = PreparePackages (1, &mStringPackGuid, VfrBin);
  Status      = Hii->NewPack (Hii, PackageList, &HiiHandle);
  FreePool (PackageList);

  CallbackInfo->RegisteredHandle = HiiHandle;

  //
  // Very simple example of how one would update a string that is already
  // in the HII database
  //
  TokenToUpdate = (STRING_REF) STR_CPU_STRING2;
  NewString     = (CHAR16 *) L"700 Mhz";

  Hii->NewString (Hii, NULL, HiiHandle, &TokenToUpdate, NewString);

  //
  // Add a string - if 0 will be updated with new Token number
  //
  TokenToUpdate = (STRING_REF) 0;

  //
  // Add a string - if 0 will be updated with new Token number
  //
  TokenToUpdate2 = (STRING_REF) 0;

  //
  // Add a string - if 0 will be updated with new Token number
  //
  TokenToUpdate3 = (STRING_REF) 0;

  Hii->NewString (Hii, NULL, HiiHandle, &TokenToUpdate, (CHAR16 *) L"Desired Speed");
  Hii->NewString (Hii, NULL, HiiHandle, &TokenToUpdate2, (CHAR16 *) L"5 Thz");
  Hii->NewString (Hii, NULL, HiiHandle, &TokenToUpdate3, (CHAR16 *) L"This is next year's desired speed - right?");

  //
  // Allocate space for creation of Buffer
  //
  UpdateData = AllocateZeroPool (0x1000);
  ASSERT (UpdateData != NULL);

  //
  // Flag update pending in FormSet
  //
  UpdateData->FormSetUpdate = TRUE;
  //
  // Register CallbackHandle data for FormSet
  //
  UpdateData->FormCallbackHandle = (EFI_PHYSICAL_ADDRESS) (UINTN) CallbackInfo->CallbackHandle;
  UpdateData->FormUpdate  = FALSE;
  UpdateData->FormTitle   = 0;
  UpdateData->DataCount   = 1;

  CreateTextOpCode (TokenToUpdate, TokenToUpdate2, TokenToUpdate3, 0, 0, &UpdateData->Data);

  Hii->UpdateForm (Hii, HiiHandle, (EFI_FORM_LABEL) 100, TRUE, UpdateData);

  FreePool (UpdateData);

  //
  // Example of how to display only the item we sent to HII
  //
  if (DISPLAY_ONLY_MY_ITEM == 0x0001) {
    //
    // Have the browser pull out our copy of the data, and only display our data
    //
    //    Status = FormConfig->SendForm (FormConfig, TRUE, HiiHandle, NULL, NULL, NULL, &Screen, NULL);
    //
  } else {
    //
    // Have the browser pull out all the data in the HII Database and display it.
    //
    //    Status = FormConfig->SendForm (FormConfig, TRUE, 0, NULL, NULL, NULL, NULL, NULL);
    //
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
