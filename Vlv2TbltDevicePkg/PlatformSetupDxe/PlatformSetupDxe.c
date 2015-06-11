/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

**/

#include "PlatformSetupDxe.h"
#include "Guid/SetupVariable.h"
#include <Protocol/FormBrowserEx2.h>


#define EFI_CALLBACK_INFO_SIGNATURE SIGNATURE_32 ('C', 'l', 'b', 'k')
#define EFI_CALLBACK_INFO_FROM_THIS(a)  CR (a, EFI_CALLBACK_INFO, ConfigAccess, EFI_CALLBACK_INFO_SIGNATURE)

typedef struct {
  UINTN                           Signature;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_HANDLE                  RegisteredHandle;
  SYSTEM_CONFIGURATION            FakeNvData;
  SYSTEM_CONFIGURATION            BackupNvData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;
} EFI_CALLBACK_INFO;

#pragma pack(1)

//
// HII specific Vendor Device Path definition.
//
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

//
// uni string and Vfr Binary data.
//
extern UINT8  VfrBin[];
extern UINT8  PlatformSetupDxeStrings[];

EFI_HANDLE            mImageHandle;

//
// module global data
//
#define EFI_NORMAL_SETUP_GUID \
  { 0xec87d643, 0xeba4, 0x4bb5, 0xa1, 0xe5, 0x3f, 0x3e, 0x36, 0xb2, 0xd, 0xa9 }

EFI_GUID                     mNormalSetupGuid = EFI_NORMAL_SETUP_GUID;

EFI_GUID                     mSystemConfigGuid = SYSTEM_CONFIGURATION_GUID;
CHAR16                       mVariableName[] = L"Setup";
CHAR16                       mSetupName[] = L"Setup";
EFI_CALLBACK_INFO           *mCallbackInfo;
BOOLEAN                      GlobalReset=FALSE;

HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EFI_CALLER_ID_GUID
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

  @param  This         Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request      A null-terminated Unicode string in <ConfigRequest> format.
  @param  Progress     On return, points to a character in the Request string.
                       Points to the string's null terminator if request was successful.
                       Points to the most recent '&' before the first failing name/value
                       pair (or the beginning of the string if the failure is in the
                       first name/value pair) if the request was not successful.
  @param  Results      A null-terminated Unicode string in <ConfigAltResp> format which
                       has all values filled in for the names in the Request string.
                       String to be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/

VOID
CheckSystemConfigLoad(SYSTEM_CONFIGURATION *SystemConfigPtr);

VOID
CheckSystemConfigSave(SYSTEM_CONFIGURATION *SystemConfigPtr);

VOID
ConfirmSecureBootTest();

VOID
LoadLpssDefaultValues (
  IN EFI_CALLBACK_INFO                       *Private
  )
{
  //
  // Load LPSS and SCC defalut configurations for Android
  //
  Private->FakeNvData.LpsseMMCEnabled            = FALSE;
  Private->FakeNvData.LpssSdioEnabled            = TRUE;
  Private->FakeNvData.LpssSdcardEnabled          = TRUE;
  Private->FakeNvData.LpssSdCardSDR25Enabled     = FALSE;
  Private->FakeNvData.LpssSdCardDDR50Enabled     = TRUE;
  Private->FakeNvData.LpssMipiHsi                = FALSE;
  Private->FakeNvData.LpsseMMC45Enabled          = TRUE;
  Private->FakeNvData.LpsseMMC45DDR50Enabled     = TRUE;
  Private->FakeNvData.LpsseMMC45HS200Enabled     = FALSE;
  Private->FakeNvData.LpsseMMC45RetuneTimerValue = 8;
  Private->FakeNvData.eMMCBootMode               = 1;     // Auto Detect

  Private->FakeNvData.GOPEnable = TRUE;
  Private->FakeNvData.SecureBoot = TRUE;
  Private->FakeNvData.UsbAutoMode = TRUE;
  Private->FakeNvData.UsbXhciSupport = TRUE;
  Private->FakeNvData.PchUsb30Mode = TRUE;
  Private->FakeNvData.LegacyUSBBooting = FALSE;
  Private->FakeNvData.PchUsb20 = FALSE;
}


EFI_STATUS
EFIAPI
SystemConfigExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  EFI_CALLBACK_INFO                *Private;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;
  UINTN                            BufferSize;
  VOID                             *SystemConfigPtr;


  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &mSystemConfigGuid, mVariableName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  Size             = 0;
  AllocatedRequest = FALSE;

  Private          = EFI_CALLBACK_INFO_FROM_THIS (This);

  SetupInfo();

  HiiConfigRouting = Private->HiiConfigRouting;
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&mSystemConfigGuid, mVariableName, Private->DriverHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    BufferSize = sizeof (SYSTEM_CONFIGURATION);
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }
  SystemConfigPtr = GetVariable(mSetupName, &mNormalSetupGuid);


  if (SystemConfigPtr == NULL) {
    ZeroMem(&Private->FakeNvData, sizeof(SYSTEM_CONFIGURATION));
    ZeroMem(&Private->BackupNvData, sizeof(SYSTEM_CONFIGURATION));
  } else {
    CheckSystemConfigLoad(SystemConfigPtr);
    CopyMem(&Private->FakeNvData, SystemConfigPtr, sizeof(SYSTEM_CONFIGURATION));
    CopyMem(&Private->BackupNvData, SystemConfigPtr, sizeof(SYSTEM_CONFIGURATION));
    FreePool(SystemConfigPtr);
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  Status = HiiConfigRouting->BlockToConfig (
                               HiiConfigRouting,
                               ConfigRequest,
                               (UINT8 *) &Private->FakeNvData,
                               sizeof (SYSTEM_CONFIGURATION),
                               Results,
                               Progress
                               );

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }

  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param  This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration   A null-terminated Unicode string in <ConfigRequest> format.
  @param  Progress        A pointer to a string filled in with the offset of the most
                          recent '&' before the first failing name/value pair (or the
                          beginning of the string if the failure is in the first
                          name/value pair) or the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
SystemConfigRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_CALLBACK_INFO                         *Private;
  SYSTEM_CONFIGURATION                       *FakeNvData;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Configuration;

  if (!HiiIsConfigHdrMatch (Configuration, &mSystemConfigGuid, mVariableName)) {
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);
  Private    = EFI_CALLBACK_INFO_FROM_THIS (This);
  FakeNvData = &Private->FakeNvData;
  if (!HiiGetBrowserData (&mSystemConfigGuid, mVariableName, sizeof (SYSTEM_CONFIGURATION), (UINT8 *) FakeNvData)) {
    //
    // FakeNvData can't be got from SetupBrowser, which doesn't need to be set.
    //
    return EFI_SUCCESS;
  }

  if (Private->FakeNvData.ReservedO != Private->BackupNvData.ReservedO) {
    Private->BackupNvData.ReservedO = Private->FakeNvData.ReservedO;
    LoadLpssDefaultValues (Private);

    //
    // Pass changed uncommitted data back to Form Browser
    //
    HiiSetBrowserData (&mSystemConfigGuid, mVariableName, sizeof (SYSTEM_CONFIGURATION), (UINT8 *) FakeNvData, NULL);
  }

  gRT->SetVariable(
         mSetupName,
         &mNormalSetupGuid,
         EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
         sizeof(SYSTEM_CONFIGURATION),
         &Private->FakeNvData
         );

  CheckSystemConfigSave(&Private->FakeNvData);
  return EFI_SUCCESS;
}

/**
  This is the function that is called to provide results data to the driver.  This data
  consists of a unique key which is used to identify what data is either being passed back
  or being asked for.

  @param  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action         A null-terminated Unicode string in <ConfigRequest> format.
  @param  KeyValue       A unique Goto OpCode callback value which record user's selection.
                         0x100 <= KeyValue <0x500 : user select a controller item in the first page;
                         KeyValue == 0x1234       : user select 'Refresh' in first page, or user select 'Go to Previous Menu' in second page
                         KeyValue == 0x1235       : user select 'Pci device filter' in first page
                         KeyValue == 0x1500       : user select 'order ... priority' item in second page
                         KeyValue == 0x1800       : user select 'commint changes' in third page
                         KeyValue == 0x2000       : user select 'Go to Previous Menu' in third page
  @param  Type           The type of value for the question.
  @param  Value          A pointer to the data being sent to the original exporting driver.
  @param  ActionRequest  On return, points to the action requested by the callback function.

  @retval EFI_SUCCESS    Always returned.

**/
EFI_STATUS
EFIAPI
SystemConfigCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        KeyValue,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_CALLBACK_INFO             *Private;
  SYSTEM_CONFIGURATION          *FakeNvData;
  SYSTEM_CONFIGURATION          *SetupData;
  UINTN                         SizeOfNvStore;
  EFI_INPUT_KEY                 Key;
  CHAR16                        *StringBuffer1;
  CHAR16                        *StringBuffer2;
  CHAR16                        *StringBuffer3;
  EFI_STATUS                    Status;
  UINTN                         DataSize;
  UINT8                         OsSelection;
  EDKII_FORM_BROWSER_EXTENSION2_PROTOCOL *FormBrowserEx2;

  StringBuffer1 = AllocateZeroPool (200 * sizeof (CHAR16));
  ASSERT (StringBuffer1 != NULL);
  StringBuffer2 = AllocateZeroPool (200 * sizeof (CHAR16));
  ASSERT (StringBuffer2 != NULL);
  StringBuffer3 = AllocateZeroPool (200 * sizeof (CHAR16));
  ASSERT (StringBuffer3 != NULL);

  switch (Action) {
  case EFI_BROWSER_ACTION_CHANGING:
  {
    if (KeyValue == 0x1235) {
      StrCpy (StringBuffer1, L"Will you disable PTT ? ");
      StrCpy (StringBuffer2, L"Enter (YES)  /   Esc (NO)");

      //
      // Popup a menu to notice user
      //
      do {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, NULL);
      } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

      //
      // If the user hits the YES Response key,
      //
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {

      }
    } else if (KeyValue == 0x1236) {
      StrCpy (StringBuffer1, L"Will you revoke trust ? ");
      StrCpy (StringBuffer2, L"Enter (YES)  /   Esc (NO)");

      //
      // Popup a menu to notice user
      //
      do {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, NULL);
      } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

      //
      // If the user hits the YES Response key,
      //
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {

      }
	 } else if (KeyValue == 0x1239) {
	   if (Value->u8 == 0x00) {
       StrCpy (StringBuffer1, L"WARNING: SOC may be damaged due to high temperature");
       StrCpy (StringBuffer2, L"when DPTF is disabled and IGD turbo is enabled.");
       StrCpy (StringBuffer3, L"Press Enter/ESC to continue...");

        //
        // Popup a menu to notice user
        //
        do {
          CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, StringBuffer3, NULL);
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));
    }
    } else if (KeyValue == 0x1240) { // secure erase feature of eMMC
	    //
      // Popup a menu to notice user
      //
      StrCpy (StringBuffer1, L"WARNING: All your data on the eMMC will be lost");
      StrCpy (StringBuffer2, L"Do you really want to enable secure erase on eMMC?");
      StrCpy (StringBuffer3, L"       Enter (YES)    /    Esc (NO)        ");

      do {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, StringBuffer3,NULL);
      } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

      //
      // If the user hits the ESC Response key,
      //
      if (Key.ScanCode == SCAN_ESC) {
        Private = EFI_CALLBACK_INFO_FROM_THIS (This);
        FakeNvData = &Private->FakeNvData;

        Status = HiiGetBrowserData (
		               &mSystemConfigGuid,
				           mVariableName,
				           sizeof (SYSTEM_CONFIGURATION),
				           (UINT8 *) FakeNvData
				           );
        if (!EFI_ERROR (Status)) {
             FakeNvData->SecureErase = 0;
             HiiSetBrowserData (
               &mSystemConfigGuid,
               mVariableName,
               sizeof (SYSTEM_CONFIGURATION),
               (UINT8 *) FakeNvData,
               NULL
               );
        }
        break;
      }

      //
      // If the user hits the YES Response key
      //
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        //
        // Save change
        //
        Private = EFI_CALLBACK_INFO_FROM_THIS (This);
        FakeNvData = &Private->FakeNvData;

        Status = HiiGetBrowserData (
		               &mSystemConfigGuid,
				           mVariableName,
				           sizeof (SYSTEM_CONFIGURATION),
				          (UINT8 *) FakeNvData
				          );
        if (!EFI_ERROR (Status)) {
          Status = gRT->SetVariable (
                          L"Setup",
                          &mNormalSetupGuid,
                          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                          sizeof(SYSTEM_CONFIGURATION),
                          &Private->FakeNvData
                          );
        }

        //
        // Reset system
        //
        gRT->ResetSystem(
		           EfiResetCold,
			         EFI_SUCCESS,
			         0,
			         NULL
			         );

    }


    }
    else if (KeyValue == 0xF001) {
      //
      // Popup a menu to notice user
      //
      StrCpy (StringBuffer1, L"Do you want to Commit Changes and Exit?");
      StrCpy (StringBuffer2, L"         Enter (YES) / Esc (NO)        ");

      do {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, NULL);
      } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

      //
      // If the user hits the YES Response key
      //
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        //
        // Save change
        //
        Private = EFI_CALLBACK_INFO_FROM_THIS (This);
        FakeNvData = &Private->FakeNvData;

        Status = HiiGetBrowserData (
		           &mSystemConfigGuid,
				   mVariableName,
				   sizeof (SYSTEM_CONFIGURATION),
				   (UINT8 *) FakeNvData
				   );
        if (!EFI_ERROR (Status)) {
          Status = gRT->SetVariable (
                          L"Setup",
                          &mNormalSetupGuid,
                          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                          sizeof(SYSTEM_CONFIGURATION),
                          &Private->FakeNvData
                          );
        }

		//
		// Update Secure Boot configuration changes
		//
        CheckSystemConfigSave(FakeNvData);

        //
        // Reset system
        //
        if (GlobalReset == TRUE) {
          //
          // Issue full reset
          //
          IoWrite8 (
            (UINTN) 0XCF9,
            (UINT8) 0x02
            );

          IoWrite8 (
            (UINTN) 0xCF9,
            (UINT8) 0x0E
            );
        } else {
        	gRT->ResetSystem(
			       EfiResetCold,
				   EFI_SUCCESS,
				   0,
				   NULL
				   );
        }
      }
    } else if (KeyValue == 0xF002) {
      //
      // Popup a menu to notice user
      //
      StrCpy (StringBuffer1, L"Do you want to Discard Changes and Exit?");
      StrCpy (StringBuffer2, L"         Enter (YES) / Esc (NO)         ");

      do {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, NULL);
      } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

      //
      // If the user hits the YES Response key
      //
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        //
        // Reset system
        //
        gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
      }
    } else if (KeyValue == 0xF003) {
      //
      // Popup a menu to notice user
      //
      StrCpy (StringBuffer1, L"Do you want to load setup defaults and Exit?");
      StrCpy (StringBuffer2, L"         Enter (YES) / Esc (NO)             ");

      do {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, NULL);
      } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

      //
      // If the user hits the YES Response key
      //
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {

        Status = gBS->LocateProtocol (&gEdkiiFormBrowserEx2ProtocolGuid, NULL, (VOID **) &FormBrowserEx2);
        FormBrowserEx2->ExecuteAction(BROWSER_ACTION_DEFAULT, EFI_HII_DEFAULT_CLASS_STANDARD);

        FakeNvData = AllocateZeroPool (sizeof(SYSTEM_CONFIGURATION));

        if (FakeNvData == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }
        
        Status = HiiGetBrowserData (
		           &mSystemConfigGuid,
				   mVariableName,
				   sizeof (SYSTEM_CONFIGURATION),
				   (UINT8 *) FakeNvData
				   );
        
        if (!EFI_ERROR (Status)) {
          Status = gRT->SetVariable (
                          L"Setup",
                          &mNormalSetupGuid,
                            EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                          sizeof(SYSTEM_CONFIGURATION),
                          FakeNvData
                          );
        }

        FreePool (FakeNvData);

        DataSize = sizeof(OsSelection);
        Status = gRT->GetVariable(
                        L"OsSelection",
                        &gOsSelectionVariableGuid,
                        NULL,
                        &DataSize,
                        &OsSelection
                        );

        if (EFI_ERROR(Status) || (OsSelection != FakeNvData->ReservedO)) {
          OsSelection = FakeNvData->ReservedO;
          Status = gRT->SetVariable (
                          L"OsSelection",
                          &gOsSelectionVariableGuid,
                          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                          sizeof(OsSelection),
                          &OsSelection
                          );
        }

        //
        // Reset system
        //
        gRT->ResetSystem(
		       EfiResetCold,
			   EFI_SUCCESS,
			   0,
			   NULL
			   );
      }
    } else if ((KeyValue == 0x123A) || (KeyValue == 0x123B) || (KeyValue == 0x123C)) {
        StrCpy (StringBuffer1, L"WARNING: Enable or disable USB Controllers will ");
        StrCpy (StringBuffer2, L"make global reset to restart system.");
        StrCpy (StringBuffer3, L"Press Enter/ESC to continue...");
        //
        // Popup a menu to notice user
        //
        do {
          CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, StringBuffer3, NULL);
        } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

        FakeNvData = AllocateZeroPool (sizeof(SYSTEM_CONFIGURATION));
        Status = HiiGetBrowserData (
		           &mSystemConfigGuid,
				   mVariableName,
				   sizeof (SYSTEM_CONFIGURATION),
				   (UINT8 *) FakeNvData
				   );
        //
        // Get variable data
        //
        SizeOfNvStore = sizeof(SYSTEM_CONFIGURATION);
        SetupData = AllocateZeroPool (sizeof(SYSTEM_CONFIGURATION));
        Status = gRT->GetVariable(
                        L"Setup",
                        &mNormalSetupGuid,
                        NULL,
                        &SizeOfNvStore,
                        SetupData
                        );
        if ((SetupData->UsbAutoMode != FakeNvData->UsbAutoMode) ||
        	   (SetupData->UsbXhciSupport != FakeNvData->UsbXhciSupport) ||
        	   (SetupData->PchUsb20 != FakeNvData->PchUsb20)) {
          GlobalReset = TRUE;
        } else {
          GlobalReset = FALSE;
        }

    }
  }
  break;

  default:
    break;
  }

  FreePool (StringBuffer1);
  FreePool (StringBuffer2);
  FreePool (StringBuffer3);

  //
  // Workaround for Load Default for "DPTF Enable"
  //
  if (Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD) {
    if (KeyValue == 0x1239) {
      return EFI_NOT_FOUND;
    }
  }

  if (Action == EFI_BROWSER_ACTION_FORM_CLOSE) {
    //
    // Do nothing for UEFI OPEN/CLOSE Action
    //
    return EFI_SUCCESS;
  }

  Private = EFI_CALLBACK_INFO_FROM_THIS (This);
  FakeNvData = &Private->FakeNvData;
  if (!HiiGetBrowserData (&mSystemConfigGuid, mVariableName, sizeof (SYSTEM_CONFIGURATION), (UINT8 *) FakeNvData)) {
    return EFI_NOT_FOUND;
  }

  if ((Action == EFI_BROWSER_ACTION_FORM_OPEN) && (Private->FakeNvData.ReservedO != Private->BackupNvData.ReservedO)) {
    Private->BackupNvData.ReservedO = Private->FakeNvData.ReservedO;
    LoadLpssDefaultValues (Private);
  }

  //
  // When user selected the secure erase, set it to disable
  //
  if((KeyValue == 0x1240) && (Action == EFI_BROWSER_ACTION_CHANGED)) {
    FakeNvData->SecureErase = 0;
  }

  if ((Action == EFI_BROWSER_ACTION_FORM_OPEN) || (Action == EFI_BROWSER_ACTION_CHANGED)) {
    //
    // If function 0 is disabled, function 1 ~ 7 also required to be disabled.
    //
    if (Private->FakeNvData.LpssDma0Enabled == 0) {
      Private->FakeNvData.LpssHsuart0Enabled = 0;
      Private->FakeNvData.LpssHsuart1Enabled = 0;
      Private->FakeNvData.LpssPwm0Enabled    = 0;
      Private->FakeNvData.LpssPwm1Enabled    = 0;
      Private->FakeNvData.LpssSpiEnabled     = 0;
    }


    //
    // If function 0 is disabled, function 1 ~ 7 also required to be disabled.
    //
    if (Private->FakeNvData.LpssDma1Enabled == 0) {
      Private->FakeNvData.LpssI2C0Enabled = 0;
      Private->FakeNvData.LpssI2C1Enabled = 0;
      Private->FakeNvData.LpssI2C2Enabled = 0;
      Private->FakeNvData.LpssI2C3Enabled = 0;
      Private->FakeNvData.LpssI2C4Enabled = 0;
      Private->FakeNvData.LpssI2C5Enabled = 0;
      Private->FakeNvData.LpssI2C6Enabled = 0;
    }
  }


  //
  // Pass changed uncommitted data back to Form Browser
  //
  HiiSetBrowserData (&mSystemConfigGuid, mVariableName, sizeof (SYSTEM_CONFIGURATION), (UINT8 *) FakeNvData, NULL);

  return EFI_SUCCESS;
}


/**
  The driver Entry Point. The funciton will export a disk device class formset and
  its callback function to hii database.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
PlatformSetupDxeInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                  Status;
  EFI_FORM_BROWSER2_PROTOCOL  *FormBrowser2;

  mImageHandle = ImageHandle;

  //
  // There should only be one Form Configuration protocol
  //
  Status = gBS->LocateProtocol (
                 &gEfiFormBrowser2ProtocolGuid,
                 NULL,
                 (VOID **) &FormBrowser2
                 );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mCallbackInfo = AllocateZeroPool (sizeof (EFI_CALLBACK_INFO));
  if (mCallbackInfo == NULL) {
    return EFI_BAD_BUFFER_SIZE;
  }

  mCallbackInfo->Signature = EFI_CALLBACK_INFO_SIGNATURE;
  mCallbackInfo->ConfigAccess.ExtractConfig = SystemConfigExtractConfig;
  mCallbackInfo->ConfigAccess.RouteConfig   = SystemConfigRouteConfig;
  mCallbackInfo->ConfigAccess.Callback      = SystemConfigCallback;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  // Install Platform Driver Override Protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mCallbackInfo->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mCallbackInfo->ConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Finish;
  }

  //
  // Publish our HII data
  //
  mCallbackInfo->RegisteredHandle = HiiAddPackages (
                                      &mSystemConfigGuid,
                                      mCallbackInfo->DriverHandle,
                                      VfrBin,
                                      PlatformSetupDxeStrings,
                                      NULL
                                      );
  if (mCallbackInfo->RegisteredHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }

  mHiiHandle = mCallbackInfo->RegisteredHandle;

  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **) &mCallbackInfo->HiiConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    goto Finish;
  }

  //
  // Clear all the globle variable
  //
  return EFI_SUCCESS;

Finish:
  if (mCallbackInfo->DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mCallbackInfo->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &mCallbackInfo->ConfigAccess,
           NULL
           );
  }

  if (mCallbackInfo->RegisteredHandle != NULL) {
    HiiRemovePackages (mCallbackInfo->RegisteredHandle);
  }

  if (mCallbackInfo != NULL) {
    FreePool (mCallbackInfo);
  }

  return Status;
}

/**
  Unload its installed protocol.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
PlatformSetupDxeUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (mCallbackInfo != NULL) {
    if (mCallbackInfo->DriverHandle != NULL) {
      gBS->UninstallMultipleProtocolInterfaces (
             mCallbackInfo->DriverHandle,
             &gEfiDevicePathProtocolGuid,
             &mHiiVendorDevicePath,
             &gEfiHiiConfigAccessProtocolGuid,
             &mCallbackInfo->ConfigAccess,
             NULL
             );
    }

    if (mCallbackInfo->RegisteredHandle != NULL) {
      HiiRemovePackages (mCallbackInfo->RegisteredHandle);
    }

    FreePool (mCallbackInfo);
  }

  return EFI_SUCCESS;
}

