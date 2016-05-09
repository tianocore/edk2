/** @file
  Implementation of the HII for the Opal UEFI Driver.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "OpalHii.h"
#include "OpalDriver.h"
#include "OpalHiiPrivate.h"

//
// This is the generated IFR binary Data for each formset defined in VFR.
// This Data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  OpalPasswordFormBin[];

//
// This is the generated String package Data for all .UNI files.
// This Data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  OpalPasswordDxeStrings[];

CHAR16  OpalPasswordStorageName[] = L"OpalHiiConfig";

EFI_HII_CONFIG_ACCESS_PROTOCOL gHiiConfigAccessProtocol;

//
// Handle to the list of HII packages (forms and strings) for this driver
//
EFI_HII_HANDLE gHiiPackageListHandle = NULL;

//
// Package List GUID containing all form and string packages
//
const EFI_GUID gHiiPackageListGuid = PACKAGE_LIST_GUID;
const EFI_GUID gHiiSetupVariableGuid = SETUP_VARIABLE_GUID;

//
// Structure that contains state of the HII
// This structure is updated by Hii.cpp and its contents
// is rendered in the HII.
//
OPAL_HII_CONFIGURATION gHiiConfiguration;

CHAR8 gHiiOldPassword[MAX_PASSWORD_CHARACTER_LENGTH] = {0};
UINT32 gHiiOldPasswordLength = 0;

//
// The device path containing the VENDOR_DEVICE_PATH and EFI_DEVICE_PATH_PROTOCOL
//
HII_VENDOR_DEVICE_PATH gHiiVendorDevicePath = {
    {
        {
            HARDWARE_DEVICE_PATH,
            HW_VENDOR_DP,
            {
                (UINT8)(sizeof(VENDOR_DEVICE_PATH)),
                (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8)
            }
        },
        OPAL_PASSWORD_CONFIG_GUID
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
  Sets the current system state of global config variables.

**/
VOID
HiiSetCurrentConfiguration(
  VOID
  )
{
  UINT32                                       PpStorageFlag;
  EFI_STRING                                   NewString;

  gHiiConfiguration.NumDisks = GetDeviceCount();

  //
  // Update the BlockSID status string.
  //
  PpStorageFlag = Tcg2PhysicalPresenceLibGetManagementFlags ();

  if ((PpStorageFlag & TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_ENABLE_BLOCK_SID) != 0) {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN(STR_ENABLED), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO,  "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  } else {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN(STR_DISABLED), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO,  "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  }
  HiiSetString(gHiiPackageListHandle, STRING_TOKEN(STR_BLOCKSID_STATUS1), NewString, NULL);
  FreePool (NewString);

  if ((PpStorageFlag & TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_ENABLE_BLOCK_SID) != 0) {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN(STR_DISK_INFO_ENABLE_BLOCKSID_TRUE), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO,  "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  } else {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN(STR_DISK_INFO_ENABLE_BLOCKSID_FALSE), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO,  "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  }
  HiiSetString(gHiiPackageListHandle, STRING_TOKEN(STR_BLOCKSID_STATUS2), NewString, NULL);
  FreePool (NewString);

  if ((PpStorageFlag & TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_DISABLE_BLOCK_SID) != 0) {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN(STR_DISK_INFO_DISABLE_BLOCKSID_TRUE), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO,  "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  } else {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN(STR_DISK_INFO_DISABLE_BLOCKSID_FALSE), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO,  "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  }
  HiiSetString(gHiiPackageListHandle, STRING_TOKEN(STR_BLOCKSID_STATUS3), NewString, NULL);
  FreePool (NewString);
}

/**
  Install the HII related resources.

  @retval  EFI_SUCCESS        Install all the resources success.
  @retval  other              Error occur when install the resources.
**/
EFI_STATUS
HiiInstall(
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_HANDLE                   DriverHandle;

  //
  // Clear the global configuration.
  //
  ZeroMem(&gHiiConfiguration, sizeof(gHiiConfiguration));

  //
  // Obtain the driver handle that the BIOS assigned us
  //
  DriverHandle = HiiGetDriverImageHandleCB();

  //
  // Populate the config access protocol with the three functions we are publishing
  //
  gHiiConfigAccessProtocol.ExtractConfig = ExtractConfig;
  gHiiConfigAccessProtocol.RouteConfig = RouteConfig;
  gHiiConfigAccessProtocol.Callback = DriverCallback;

  //
  // Associate the required protocols with our driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces(
               &DriverHandle,
               &gEfiHiiConfigAccessProtocolGuid,
               &gHiiConfigAccessProtocol,      // HII callback
               &gEfiDevicePathProtocolGuid,
               &gHiiVendorDevicePath,        // required for HII callback allow all disks to be shown in same hii
               NULL
           );

  if (EFI_ERROR(Status)) {
    return Status;
  }

  return OpalHiiAddPackages();
}

/**
  Install the HII form and string packages.

  @retval  EFI_SUCCESS           Install all the resources success.
  @retval  EFI_OUT_OF_RESOURCES  Out of resource error.
**/
EFI_STATUS
OpalHiiAddPackages(
  VOID
  )
{
  EFI_HANDLE                   DriverHandle;
  CHAR16                       *NewString;

  DriverHandle = HiiGetDriverImageHandleCB();

  //
  // Publish the HII form and HII string packages
  //
  gHiiPackageListHandle = HiiAddPackages(
                                &gHiiPackageListGuid,
                                DriverHandle,
                                OpalPasswordDxeStrings,
                                OpalPasswordFormBin,
                                (VOID*)NULL
                                );

  //
  // Make sure the packages installed successfully
  //
  if (gHiiPackageListHandle == NULL) {
    DEBUG ((DEBUG_INFO, "OpalHiiAddPackages failed\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Update Version String in main window
  //
  NewString = HiiGetDriverNameCB ();
  if (HiiSetString(gHiiPackageListHandle, STRING_TOKEN(STR_MAIN_OPAL_VERSION), NewString, NULL) == 0) {
    DEBUG ((DEBUG_INFO,  "OpalHiiAddPackages: HiiSetString( ) failed\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Uninstall the HII capability.

  @retval  EFI_SUCCESS           Uninstall all the resources success.
  @retval  others                Other errors occur when unistall the hii resource.
**/
EFI_STATUS
HiiUninstall(
  VOID
  )
{
  EFI_STATUS                   Status;

  //
  // Remove the packages we've provided to the BIOS
  //
  HiiRemovePackages(gHiiPackageListHandle);

  //
  // Remove the protocols from our driver handle
  //
  Status = gBS->UninstallMultipleProtocolInterfaces(
                          HiiGetDriverImageHandleCB(),
                          &gEfiHiiConfigAccessProtocolGuid,
                          &gHiiConfigAccessProtocol,        // HII callback
                          &gEfiDevicePathProtocolGuid,
                          &gHiiVendorDevicePath,            // required for HII callback
                          NULL
                      );
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_INFO, "Cannot uninstall Hii Protocols: %r\n", Status));
  }

  return Status;
}

/**
  Updates the main menu form.

  @retval  EFI_SUCCESS           update the main form success.
**/
EFI_STATUS
HiiPopulateMainMenuForm (
  VOID
  )
{
  UINT8         Index;
  CHAR8         *DiskName;
  EFI_STRING_ID DiskNameId;
  OPAL_DISK     *OpalDisk;

  HiiSetCurrentConfiguration();

  gHiiConfiguration.SupportedDisks = 0;

  for (Index = 0; Index < gHiiConfiguration.NumDisks; Index++) {
    OpalDisk = HiiGetOpalDiskCB (Index);
    if ((OpalDisk != NULL) && OpalFeatureSupported (&OpalDisk->SupportedAttributes)) {
      gHiiConfiguration.SupportedDisks |= (1 << Index);
      DiskNameId = GetDiskNameStringId (Index);
      DiskName = HiiDiskGetNameCB (Index);
      if ((DiskName == NULL) || (DiskNameId == 0)) {
        return EFI_UNSUPPORTED;
      }
      HiiSetFormString(DiskNameId, DiskName);
    }
  }

  OpalHiiSetBrowserData ();
  return EFI_SUCCESS;
}

/**
  Update the disk action info.

  @param     ActionString
  @param     SelectedAction

  @retval  EFI_SUCCESS           Uninstall all the resources success.
**/
EFI_STATUS
HiiSelectDiskAction (
  CHAR8           *ActionString,
  UINT8           SelectedAction
  )
{
  OPAL_DISK                     *OpalDisk;
  OPAL_DISK_ACTIONS              AvailActions;

  OpalHiiGetBrowserData ();

  HiiSetFormString(STRING_TOKEN(STR_DISK_ACTION_LBL), ActionString);
  HiiSetFormString(STRING_TOKEN(STR_ACTION_STATUS), " ");

  gHiiConfiguration.SelectedAction = SelectedAction;
  gHiiConfiguration.AvailableFields = 0;

  OpalDisk = HiiGetOpalDiskCB(gHiiConfiguration.SelectedDiskIndex);
  if (OpalDisk == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (OpalSupportGetAvailableActions (&OpalDisk->SupportedAttributes, &OpalDisk->LockingFeature, OpalDisk->Owner, &AvailActions) != TcgResultSuccess) {
    return EFI_DEVICE_ERROR;
  }

  switch (SelectedAction) {
    case HII_KEY_ID_GOTO_LOCK:
    case HII_KEY_ID_GOTO_UNLOCK:
    case HII_KEY_ID_GOTO_SET_ADMIN_PWD:
    case HII_KEY_ID_GOTO_SET_USER_PWD:
    case HII_KEY_ID_GOTO_SECURE_ERASE:
    case HII_KEY_ID_GOTO_DISABLE_USER:
    case HII_KEY_ID_GOTO_ENABLE_FEATURE:   // User is required to enter Password to enable Feature
      gHiiConfiguration.AvailableFields |= HII_FIELD_PASSWORD;
      break;

    case HII_KEY_ID_GOTO_PSID_REVERT:
      gHiiConfiguration.AvailableFields |= HII_FIELD_PSID;
      break;

    case HII_KEY_ID_GOTO_REVERT:
      gHiiConfiguration.AvailableFields |= HII_FIELD_PASSWORD;
      gHiiConfiguration.AvailableFields |= HII_FIELD_KEEP_USER_DATA;
      if (AvailActions.RevertKeepDataForced) {
        gHiiConfiguration.AvailableFields |= HII_FIELD_KEEP_USER_DATA_FORCED;
      }
      break;
  }

  OpalHiiSetBrowserData ();

  return EFI_SUCCESS;
}

/**
  Get disk name string id.

  @param   DiskIndex             The input disk index info.

  @retval  The disk name string id.

**/
EFI_STRING_ID
GetDiskNameStringId(
  UINT8 DiskIndex
  )
{
  switch (DiskIndex) {
    case 0: return STRING_TOKEN(STR_MAIN_GOTO_DISK_INFO_0);
    case 1: return STRING_TOKEN(STR_MAIN_GOTO_DISK_INFO_1);
    case 2: return STRING_TOKEN(STR_MAIN_GOTO_DISK_INFO_2);
    case 3: return STRING_TOKEN(STR_MAIN_GOTO_DISK_INFO_3);
    case 4: return STRING_TOKEN(STR_MAIN_GOTO_DISK_INFO_4);
    case 5: return STRING_TOKEN(STR_MAIN_GOTO_DISK_INFO_5);
  }
  return 0;
}

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
DriverCallback(
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  EFI_BROWSER_ACTION                      Action,
  EFI_QUESTION_ID                         QuestionId,
  UINT8                                   Type,
  EFI_IFR_TYPE_VALUE                      *Value,
  EFI_BROWSER_ACTION_REQUEST              *ActionRequest
  )
{
  HII_KEY    HiiKey;
  UINT8      HiiKeyId;
  UINT32     PpRequest;

  if (ActionRequest != NULL) {
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If QuestionId is an auto-generated key (label, empty line, etc.), ignore it.
  //
  if ((QuestionId & HII_KEY_FLAG) == 0) {
    return EFI_SUCCESS;
  }

  HiiKey.Raw = QuestionId;
  HiiKeyId   = (UINT8) HiiKey.KeyBits.Id;

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    switch (HiiKeyId) {
      case HII_KEY_ID_VAR_SUPPORTED_DISKS:
        DEBUG ((DEBUG_INFO,  "HII_KEY_ID_VAR_SUPPORTED_DISKS\n"));
        return HiiPopulateMainMenuForm ();

      case HII_KEY_ID_VAR_SELECTED_DISK_AVAILABLE_ACTIONS:
        return HiiPopulateDiskInfoForm();
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGING) {
    switch (HiiKeyId) {
      case HII_KEY_ID_GOTO_DISK_INFO:
        return HiiSelectDisk((UINT8)HiiKey.KeyBits.Index);

      case HII_KEY_ID_GOTO_LOCK:
        return HiiSelectDiskAction("Action: Lock", HiiKeyId);

      case HII_KEY_ID_GOTO_UNLOCK:
        return HiiSelectDiskAction("Action: Unlock", HiiKeyId);

      case HII_KEY_ID_GOTO_SET_ADMIN_PWD:
        return HiiSelectDiskAction("Action: Set Administrator Password", HiiKeyId);

      case HII_KEY_ID_GOTO_SET_USER_PWD:
        return HiiSelectDiskAction("Action: Set User Password", HiiKeyId);

      case HII_KEY_ID_GOTO_SECURE_ERASE:
        return HiiSelectDiskAction("Action: Secure Erase", HiiKeyId);

      case HII_KEY_ID_GOTO_PSID_REVERT:
        return HiiSelectDiskAction("Action: Revert to Factory Defaults with PSID", HiiKeyId);

      case HII_KEY_ID_GOTO_REVERT:
        return HiiSelectDiskAction("Action: Revert to Factory Defaults", HiiKeyId);

      case HII_KEY_ID_GOTO_DISABLE_USER:
        return HiiSelectDiskAction("Action: Disable User", HiiKeyId);

      case HII_KEY_ID_GOTO_ENABLE_FEATURE:
        return HiiSelectDiskAction("Action: Enable Feature", HiiKeyId);

      case HII_KEY_ID_ENTER_PASSWORD:
        return HiiPasswordEntered(Value->string);

      case HII_KEY_ID_ENTER_PSID:
        return HiiPsidRevert(Value->string);

    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    switch (HiiKeyId) {
      case HII_KEY_ID_BLOCKSID:
        switch (Value->u8) {
          case 0:
            PpRequest = TCG2_PHYSICAL_PRESENCE_NO_ACTION;
            break;

          case 1:
            PpRequest = TCG2_PHYSICAL_PRESENCE_ENABLE_BLOCK_SID;
            break;

          case 2:
            PpRequest = TCG2_PHYSICAL_PRESENCE_DISABLE_BLOCK_SID;
            break;

          case 3:
            PpRequest = TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_ENABLE_BLOCK_SID_FUNC_TRUE;
            break;

          case 4:
            PpRequest = TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_ENABLE_BLOCK_SID_FUNC_FALSE;
            break;

          case 5:
            PpRequest = TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_DISABLE_BLOCK_SID_FUNC_TRUE;
            break;

          case 6:
            PpRequest = TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_DISABLE_BLOCK_SID_FUNC_FALSE;
            break;

          default:
            PpRequest = TCG2_PHYSICAL_PRESENCE_NO_ACTION;
            DEBUG ((DEBUG_ERROR, "Invalid value input!\n"));
            break;
        }
        HiiSetBlockSidAction(PpRequest);

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        return EFI_SUCCESS;

      default:
        break;
    }
  }

  return EFI_UNSUPPORTED;
}

/**
  Update the global Disk index info.

  @param   Index             The input disk index info.

  @retval  EFI_SUCCESS       Update the disk index info success.

**/
EFI_STATUS
HiiSelectDisk(
  UINT8 Index
  )
{
  OpalHiiGetBrowserData();
  gHiiConfiguration.SelectedDiskIndex = Index;
  OpalHiiSetBrowserData ();

  return EFI_SUCCESS;
}

/**
  Draws the disk info form.

  @retval  EFI_SUCCESS       Draw the disk info success.

**/
EFI_STATUS
HiiPopulateDiskInfoForm(
  VOID
  )
{
  OPAL_DISK*                    OpalDisk;
  OPAL_DISK_ACTIONS             AvailActions;
  TCG_RESULT                    Ret;
  CHAR8                         *DiskName;

  OpalHiiGetBrowserData();

  DiskName = HiiDiskGetNameCB (gHiiConfiguration.SelectedDiskIndex);
  if (DiskName == NULL) {
    return EFI_UNSUPPORTED;
  }
  HiiSetFormString(STRING_TOKEN(STR_DISK_INFO_SELECTED_DISK_NAME), DiskName);

  ZeroMem(gHiiConfiguration.Psid, sizeof(gHiiConfiguration.Psid));

  gHiiConfiguration.SelectedDiskAvailableActions = HII_ACTION_NONE;

  OpalDisk = HiiGetOpalDiskCB(gHiiConfiguration.SelectedDiskIndex);

  if (OpalDisk != NULL) {
    OpalDiskUpdateStatus (OpalDisk);
    Ret = OpalSupportGetAvailableActions(&OpalDisk->SupportedAttributes, &OpalDisk->LockingFeature, OpalDisk->Owner, &AvailActions);
    if (Ret == TcgResultSuccess) {
      //
      // Update actions, always allow PSID Revert
      //
      gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.PsidRevert == 1) ? HII_ACTION_PSID_REVERT : HII_ACTION_NONE;

      //
      // Always allow unlock to handle device migration
      //
      gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.Unlock == 1) ? HII_ACTION_UNLOCK : HII_ACTION_NONE;

      if (!OpalFeatureEnabled (&OpalDisk->SupportedAttributes, &OpalDisk->LockingFeature)) {
        if (OpalDisk->Owner == OpalOwnershipNobody) {
          gHiiConfiguration.SelectedDiskAvailableActions |= HII_ACTION_ENABLE_FEATURE;

          //
          // Update strings
          //
          HiiSetFormString( STRING_TOKEN(STR_DISK_INFO_PSID_REVERT), "PSID Revert to factory default");
        } else {
          DEBUG ((DEBUG_INFO, "Feature disabled but ownership != nobody\n"));
        }
      } else {
        gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.Revert == 1) ? HII_ACTION_REVERT : HII_ACTION_NONE;
        gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.AdminPass == 1) ? HII_ACTION_SET_ADMIN_PWD : HII_ACTION_NONE;
        gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.UserPass == 1) ? HII_ACTION_SET_USER_PWD : HII_ACTION_NONE;
        gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.SecureErase == 1) ? HII_ACTION_SECURE_ERASE : HII_ACTION_NONE;
        gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.DisableUser == 1) ? HII_ACTION_DISABLE_USER : HII_ACTION_NONE;

        HiiSetFormString (STRING_TOKEN(STR_DISK_INFO_PSID_REVERT), "PSID Revert to factory default and Disable");

        //
        // Determine revert options for disk
        // Default initialize keep user Data to be true
        //
        gHiiConfiguration.KeepUserData = 1;
      }
    }
  }

  //
  // Pass the current configuration to the BIOS
  //
  OpalHiiSetBrowserData ();

  return EFI_SUCCESS;
}

/**
  Reverts the Opal disk to factory default.

  @param   PsidStringId      The string id for the PSID info.

  @retval  EFI_SUCCESS       Do the required action success.

**/
EFI_STATUS
HiiPsidRevert(
  EFI_STRING_ID         PsidStringId
  )
{
  CHAR8                         Response[DEFAULT_RESPONSE_SIZE];
  TCG_PSID                      Psid;
  OPAL_DISK                     *OpalDisk;
  TCG_RESULT                    Ret;
  OPAL_SESSION                  Session;
  CHAR16                        *UnicodeStr;
  UINT8                         TmpBuf[PSID_CHARACTER_STRING_END_LENGTH];

  Ret = TcgResultFailure;

  UnicodeStr = HiiGetString (gHiiPackageListHandle, PsidStringId, NULL);
  ZeroMem (TmpBuf, sizeof (TmpBuf));
  UnicodeStrToAsciiStrS (UnicodeStr, (CHAR8*)TmpBuf, PSID_CHARACTER_STRING_END_LENGTH);
  CopyMem (Psid.Psid, TmpBuf, PSID_CHARACTER_LENGTH);
  HiiSetString (gHiiPackageListHandle, PsidStringId, L"", NULL);
  ZeroMem (TmpBuf, sizeof (TmpBuf));
  ZeroMem (UnicodeStr, StrSize (UnicodeStr));
  FreePool (UnicodeStr);

  OpalDisk = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);
  if (OpalDisk != NULL) {
    ZeroMem(&Session, sizeof(Session));
    Session.Sscp = OpalDisk->Sscp;
    Session.MediaId = OpalDisk->MediaId;
    Session.OpalBaseComId = OpalDisk->OpalBaseComId;

    Ret = OpalSupportPsidRevert(&Session, Psid.Psid, (UINT32)sizeof(Psid.Psid), OpalDisk->OpalDevicePath);
  }

  ZeroMem (Psid.Psid, PSID_CHARACTER_LENGTH);

  if (Ret == TcgResultSuccess) {
    AsciiSPrint( Response, DEFAULT_RESPONSE_SIZE, "%a", "PSID Revert: Success" );
  } else {
    AsciiSPrint( Response, DEFAULT_RESPONSE_SIZE, "%a", "PSID Revert: Failure" );
  }

  HiiSetFormString(STRING_TOKEN(STR_ACTION_STATUS), Response);

  return EFI_SUCCESS;
}

/**
  Set password for the disk.

  @param      OpalDisk       The disk need to set the password.
  @param      Password       The input password.
  @param      PassLength     The input password length.

  @retval  EFI_SUCCESS       Do the required action success.

**/
EFI_STATUS
HiiSetPassword(
  OPAL_DISK          *OpalDisk,
  VOID               *Password,
  UINT32             PassLength
  )
{
  CHAR8                         Response[DEFAULT_RESPONSE_SIZE];
  TCG_RESULT                    Ret;
  BOOLEAN                       ExistingPassword;
  OPAL_SESSION                  Session;

  ExistingPassword = FALSE;

  //
  // PassLength = 0 means check whether exist old password.
  //
  if (PassLength == 0) {
    ZeroMem(gHiiOldPassword, sizeof(gHiiOldPassword));
    gHiiOldPasswordLength = 0;

    if (gHiiConfiguration.SelectedAction == HII_KEY_ID_GOTO_ENABLE_FEATURE) {
      ExistingPassword = FALSE;
    } else if (gHiiConfiguration.SelectedAction == HII_KEY_ID_GOTO_SET_ADMIN_PWD) {
      ExistingPassword = OpalUtilAdminPasswordExists(OpalDisk->Owner, &OpalDisk->LockingFeature);
    } else if (gHiiConfiguration.SelectedAction == HII_KEY_ID_GOTO_SET_USER_PWD) {
      //
      // Set user Password option shall only be shown if an Admin Password exists
      // so a Password is always required (Admin or Existing User Password)
      //
      ExistingPassword = TRUE;
    }

    //
    // Return error if there is a previous Password
    // see UEFI 2.4 errata B, Figure 121. Password Flowchart
    //
    return ExistingPassword ? EFI_DEVICE_ERROR : EFI_SUCCESS;
  }

  ZeroMem(&Session, sizeof(Session));
  Session.Sscp = OpalDisk->Sscp;
  Session.MediaId = OpalDisk->MediaId;
  Session.OpalBaseComId = OpalDisk->OpalBaseComId;

  AsciiSPrint(Response, DEFAULT_RESPONSE_SIZE, "%a", "Set Password: Failure");
  //
  // Password entered.
  // No current Owner, so set new Password, must be admin Password
  //
  if (OpalDisk->Owner == OpalOwnershipNobody) {
    Ret = OpalSupportEnableOpalFeature (&Session, OpalDisk->Msid, OpalDisk->MsidLength,Password, PassLength, OpalDisk->OpalDevicePath);
    if (Ret == TcgResultSuccess) {
      AsciiSPrint(Response, DEFAULT_RESPONSE_SIZE, "%a", "Set Password: Success");
    }

    HiiSetFormString(STRING_TOKEN(STR_ACTION_STATUS), Response);
    return EFI_SUCCESS;
  }

  //
  // 1st Password entered
  //
  if (OpalDisk->Owner == OpalOwnershipUnknown && gHiiOldPasswordLength == 0) {

    //
    // Unknown ownership - prompt for old Password, then new
    // old Password is not set yet - first time through
    // assume authority provided is admin1, overwritten if user1 authority works below
    //
    if (gHiiConfiguration.SelectedAction == HII_KEY_ID_GOTO_SET_USER_PWD) {
      //
      // First try to login as USER1 to Locking SP to see if we're simply updating its Password
      //
      Ret =  OpalUtilVerifyPassword (&Session, Password, PassLength, OPAL_LOCKING_SP_USER1_AUTHORITY);
      if (Ret == TcgResultSuccess) {
        //
        // User1 worked so authority 1 means user 1
        //
        CopyMem(gHiiOldPassword, Password, PassLength);
        gHiiOldPasswordLength = PassLength;

        return EFI_SUCCESS;
      }
    }

    //
    // Else try admin1 below
    //
    Ret =  OpalUtilVerifyPassword (&Session, Password, PassLength, OPAL_LOCKING_SP_ADMIN1_AUTHORITY);
    if (Ret == TcgResultSuccess) {
      CopyMem(gHiiOldPassword, Password, PassLength);
      gHiiOldPasswordLength = PassLength;

      return EFI_SUCCESS;
    } else {
      DEBUG ((DEBUG_INFO, "start session with old PW failed - return EFI_NOT_READY - mistyped old PW\n"));
      HiiSetFormString(STRING_TOKEN(STR_ACTION_STATUS), "Authentication Failure");

      ZeroMem(gHiiOldPassword, sizeof(gHiiOldPassword));
      gHiiOldPasswordLength = 0;

      return EFI_NOT_READY;
    }
  }

  //
  // New Password entered
  //
  if (gHiiConfiguration.SelectedAction == HII_KEY_ID_GOTO_SET_USER_PWD) {
    Ret = OpalSupportSetPassword(
                            &Session,
                            gHiiOldPassword,
                            gHiiOldPasswordLength,
                            Password,
                            PassLength,
                            OpalDisk->OpalDevicePath,
                            FALSE
                            );
  } else {
    Ret = OpalSupportSetPassword(
                            &Session,
                            gHiiOldPassword,
                            gHiiOldPasswordLength,
                            Password,
                            PassLength,
                            OpalDisk->OpalDevicePath,
                            TRUE
                            );
  }

  if (Ret == TcgResultSuccess) {
    AsciiSPrint(Response, DEFAULT_RESPONSE_SIZE, "%a", "Set Password: Success");
  }

  //
  // Reset old Password storage
  //
  ZeroMem(gHiiOldPassword, sizeof(gHiiOldPassword));
  gHiiOldPasswordLength = 0;

  HiiSetFormString(STRING_TOKEN(STR_ACTION_STATUS), Response);
  return Ret == TcgResultSuccess ? EFI_SUCCESS : EFI_NOT_READY;
}

/**
  Secure Erases Opal Disk.

  @param      OpalDisk       The disk need to erase data.
  @param      Password       The input password.
  @param      PassLength     The input password length.

  @retval  EFI_SUCCESS       Do the required action success.

**/
EFI_STATUS
HiiSecureErase(
  OPAL_DISK       *OpalDisk,
  const VOID      *Password,
  UINT32          PassLength
  )
{
  CHAR8                        Response[DEFAULT_RESPONSE_SIZE];
  BOOLEAN                      PasswordFailed;
  TCG_RESULT                   Ret;
  OPAL_SESSION                 AdminSpSession;

  if (PassLength == 0) {
    return EFI_DEVICE_ERROR; // return error to indicate there is an existing Password
  }

  ZeroMem(&AdminSpSession, sizeof(AdminSpSession));
  AdminSpSession.Sscp = OpalDisk->Sscp;
  AdminSpSession.MediaId = OpalDisk->MediaId;
  AdminSpSession.OpalBaseComId = OpalDisk->OpalBaseComId;

  Ret = OpalUtilSecureErase(&AdminSpSession, Password, PassLength, &PasswordFailed);
  if (Ret == TcgResultSuccess) {
    AsciiSPrint( Response, DEFAULT_RESPONSE_SIZE, "%a", "Secure Erase: Success" );
  } else {
    AsciiSPrint( Response, DEFAULT_RESPONSE_SIZE, "%a", "Secure Erase: Failure" );
  }
  HiiSetFormString(STRING_TOKEN(STR_ACTION_STATUS), Response);

  //
  // If Password failed, return invalid passowrd
  //
  if (PasswordFailed) {
    DEBUG ((DEBUG_INFO, "returning EFI_NOT_READY to indicate Password was not correct\n"));
    return EFI_NOT_READY;
  }

  //
  // Indicates Password was valid and is not changing to UEFI
  // Response string will indicate action error
  //
  return EFI_DEVICE_ERROR;
}


/**
  Disables User for Opal Disk.

  @param      OpalDisk       The disk need to the action.
  @param      Password       The input password.
  @param      PassLength     The input password length.

  @retval  EFI_SUCCESS       Do the required action success.

**/
EFI_STATUS
HiiDisableUser(
  OPAL_DISK      *OpalDisk,
  VOID           *Password,
  UINT32         PassLength
  )
{
  CHAR8                        Response[ DEFAULT_RESPONSE_SIZE ];
  BOOLEAN                      PasswordFailed;
  TCG_RESULT                   Ret;
  OPAL_SESSION                 Session;

  if (PassLength == 0) {
    return EFI_DEVICE_ERROR; // return error to indicate there is an existing Password
  }

  ZeroMem(&Session, sizeof(Session));
  Session.Sscp = OpalDisk->Sscp;
  Session.MediaId = OpalDisk->MediaId;
  Session.OpalBaseComId = OpalDisk->OpalBaseComId;

  Ret = OpalSupportDisableUser(&Session, Password, PassLength, &PasswordFailed, OpalDisk->OpalDevicePath);
  if (Ret == TcgResultSuccess) {
    AsciiSPrint( Response, DEFAULT_RESPONSE_SIZE, "%a", "Disable User: Success" );
  } else {
    AsciiSPrint( Response, DEFAULT_RESPONSE_SIZE, "%a", "Disable User: Failure" );
  }
  HiiSetFormString (STRING_TOKEN(STR_ACTION_STATUS), Response);

  //
  // If Password failed, return invalid passowrd
  //
  if (PasswordFailed) {
    DEBUG ((DEBUG_INFO, "returning EFI_NOT_READY to indicate Password was not correct\n"));
    return EFI_NOT_READY;
  }

  //
  // Indicates Password was valid and is not changing to UEFI
  // Response string will indicate action error
  //
  return EFI_DEVICE_ERROR;
}

/**
  Revert Opal Disk as Admin1.

  @param      OpalDisk       The disk need to the action.
  @param      Password       The input password.
  @param      PassLength     The input password length.
  @param      KeepUserData   Whether need to keey user data.

  @retval  EFI_SUCCESS       Do the required action success.

**/
EFI_STATUS
HiiRevert(
  OPAL_DISK       *OpalDisk,
  VOID            *Password,
  UINT32          PassLength,
  BOOLEAN         KeepUserData
  )
{
  CHAR8                         Response[ DEFAULT_RESPONSE_SIZE ];
  BOOLEAN                      PasswordFailed;
  TCG_RESULT                    Ret;
  OPAL_SESSION                  Session;

  if (PassLength == 0) {
    DEBUG ((DEBUG_INFO, "Returning error to indicate there is an existing Password\n"));
    // return error to indicate there is an existing Password
    return EFI_DEVICE_ERROR;
  }

  ZeroMem(&Session, sizeof(Session));
  Session.Sscp = OpalDisk->Sscp;
  Session.MediaId = OpalDisk->MediaId;
  Session.OpalBaseComId = OpalDisk->OpalBaseComId;

  Ret = OpalSupportRevert(
                      &Session,
                      KeepUserData,
                      Password,
                      PassLength,
                      OpalDisk->Msid,
                      OpalDisk->MsidLength,
                      &PasswordFailed,
                      OpalDisk->OpalDevicePath
                      );
  if (Ret == TcgResultSuccess) {
    AsciiSPrint( Response, DEFAULT_RESPONSE_SIZE, "%a", "Revert: Success" );
  } else {
    AsciiSPrint( Response, DEFAULT_RESPONSE_SIZE, "%a", "Revert: Failure" );
  }
  HiiSetFormString(STRING_TOKEN(STR_ACTION_STATUS), Response);

  //
  // If Password failed, return invalid passowrd
  //
  if (PasswordFailed) {
    DEBUG ((DEBUG_INFO, "returning EFI_NOT_READY to indicate Password was not correct\n"));
    return EFI_NOT_READY;
  }

  //
  // Indicates Password was valid and is not changing to UEFI
  // Response string will indicate action error
  //
  return EFI_DEVICE_ERROR;
}

/**
  Unlocks Opal Disk.

  @param      OpalDisk       The disk need to the action.
  @param      Password       The input password.
  @param      PassLength     The input password length.

  @retval  EFI_SUCCESS       Do the required action success.

**/
EFI_STATUS
HiiUnlock(
  OPAL_DISK         *OpalDisk,
  VOID              *Password,
  UINT32            PassLength
  )
{
  CHAR8                         Response[DEFAULT_RESPONSE_SIZE];
  TCG_RESULT                    Ret;
  OPAL_SESSION                  Session;

  if (PassLength == 0) {
    DEBUG ((DEBUG_INFO, "Returning error to indicate there is an existing Password\n"));
    return EFI_DEVICE_ERROR; // return error to indicate there is an existing Password
  }

  ZeroMem(&Session, sizeof(Session));
  Session.Sscp = OpalDisk->Sscp;
  Session.MediaId = OpalDisk->MediaId;
  Session.OpalBaseComId = OpalDisk->OpalBaseComId;

  Ret = OpalSupportUnlock(&Session, Password, PassLength, OpalDisk->OpalDevicePath);
  if (Ret == TcgResultSuccess) {
    AsciiSPrint( Response, DEFAULT_RESPONSE_SIZE, "%a", "Unlock: Success" );
  } else {
    AsciiSPrint( Response, DEFAULT_RESPONSE_SIZE, "%a", "Unlock: Failure" );
  }

  HiiSetFormString(STRING_TOKEN(STR_ACTION_STATUS), Response);

  if (Ret == TcgResultSuccess) {
    DEBUG ((DEBUG_INFO, "returning error to indicate Password was correct but is not changing\n"));
    return EFI_DEVICE_ERROR;
  } else {
    DEBUG ((DEBUG_INFO, "returning EFI_NOT_READY to indicate Password was not correct\n"));
    return EFI_NOT_READY;
  }
}

/**
  Use the input password to do the specified action.

  @param      Str            The input password saved in.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Other error occur.

**/
EFI_STATUS
HiiPasswordEntered(
  EFI_STRING_ID            Str
  )
{
  OPAL_DISK*                   OpalDisk;
  CHAR8                        Password[MAX_PASSWORD_CHARACTER_LENGTH + 1];
  CHAR16*                      UniStr;
  UINT32                       PassLength;
  EFI_STATUS                   Status;

  OpalHiiGetBrowserData();

  OpalDisk = HiiGetOpalDiskCB(gHiiConfiguration.SelectedDiskIndex);
  if (OpalDisk == NULL) {
    DEBUG ((DEBUG_INFO, "ERROR: disk %u not found\n", gHiiConfiguration.SelectedDiskIndex));
    return EFI_NOT_FOUND;
  }

  if (Str == 0) {
    DEBUG ((DEBUG_INFO, "ERROR: str=NULL\n"));
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem(Password, sizeof(Password));

  UniStr = HiiGetString(gHiiPackageListHandle, Str, NULL);
  if (UniStr == NULL) {
    return EFI_NOT_FOUND;
  }

  HiiSetString(gHiiPackageListHandle, Str, L"", NULL);

  PassLength = (UINT32) StrLen (UniStr);
  if (PassLength >= sizeof(Password)) {
    HiiSetFormString(STRING_TOKEN(STR_ACTION_STATUS), "Password too long");
    ZeroMem (UniStr, StrSize (UniStr));
    FreePool(UniStr);
    return EFI_BUFFER_TOO_SMALL;
  }

  UnicodeStrToAsciiStrS (UniStr, Password, sizeof (Password));
  ZeroMem (UniStr, StrSize (UniStr));
  FreePool(UniStr);

  if (gHiiConfiguration.SelectedAction == HII_KEY_ID_GOTO_UNLOCK) {
    Status = HiiUnlock (OpalDisk, Password, PassLength);
  } else if (gHiiConfiguration.SelectedAction == HII_KEY_ID_GOTO_SECURE_ERASE) {
    Status = HiiSecureErase (OpalDisk, Password, PassLength);
  } else if (gHiiConfiguration.SelectedAction == HII_KEY_ID_GOTO_DISABLE_USER) {
    Status = HiiDisableUser (OpalDisk, Password, PassLength);
  } else if (gHiiConfiguration.SelectedAction == HII_KEY_ID_GOTO_REVERT) {
    if (OpalDisk->SupportedAttributes.PyriteSsc == 1 && OpalDisk->LockingFeature.MediaEncryption == 0) {
      //
      // For pyrite type device which also not supports media encryption, it not accept "Keep User Data" parameter.
      // So here hardcode a FALSE for this case.
      //
      Status = HiiRevert(OpalDisk, Password, PassLength, FALSE);
    } else {
      Status = HiiRevert(OpalDisk, Password, PassLength, gHiiConfiguration.KeepUserData);
    }
  } else {
    Status = HiiSetPassword(OpalDisk, Password, PassLength);
  }

  ZeroMem (Password, sizeof (Password));

  OpalHiiSetBrowserData ();

  return Status;
}

/**
  Update block sid info.

  @param      Enable         Enable/disable BlockSid.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Other error occur.

**/
EFI_STATUS
HiiSetBlockSidAction (
  IN UINT32          PpRequest
  )
{
  UINT32                           ReturnCode;
  EFI_STATUS                       Status;

  ReturnCode = Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunction (PpRequest, 0);
  if (ReturnCode == TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS) {
    Status = EFI_SUCCESS;
  } else if (ReturnCode == TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE) {
    Status = EFI_OUT_OF_RESOURCES;
  } else if (ReturnCode == TCG_PP_SUBMIT_REQUEST_TO_PREOS_NOT_IMPLEMENTED) {
    Status = EFI_UNSUPPORTED;
  } else {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration          A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param  Progress               A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
RouteConfig(
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  CONST EFI_STRING                        Configuration,
  EFI_STRING                              *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  *Progress = Configuration;
  if (!HiiIsConfigHdrMatch (Configuration, &gHiiSetupVariableGuid, OpalPasswordStorageName)) {
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);

  return EFI_SUCCESS;
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request                A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param  Progress               On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param  Results                A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
ExtractConfig(
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  CONST EFI_STRING                        Request,
  EFI_STRING                              *Progress,
  EFI_STRING                              *Results
  )
{
  EFI_STATUS                              Status;

  //
  // Check for valid parameters
  //
  if (Progress == NULL || Results == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  *Progress = Request;
  if ((Request != NULL) &&
    !HiiIsConfigHdrMatch (Request, &gHiiSetupVariableGuid, OpalPasswordStorageName)) {
    return EFI_NOT_FOUND;
  }

  //
  // Convert Buffer Data to <ConfigResp> by helper function BlockToConfig( )
  //
  Status = gHiiConfigRouting->BlockToConfig(
               gHiiConfigRouting,
               Request,
               (UINT8*)&gHiiConfiguration,
               sizeof(OPAL_HII_CONFIGURATION),
               Results,
               Progress
           );

  return (Status);
}


/**

  Pass the current system state to the bios via the hii_G_Configuration.

**/
VOID
OpalHiiSetBrowserData (
  VOID
  )
{
  HiiSetBrowserData(
      &gHiiSetupVariableGuid,
      (CHAR16*)L"OpalHiiConfig",
      sizeof(gHiiConfiguration),
      (UINT8*)&gHiiConfiguration,
      NULL
  );
}


/**

  Populate the hii_g_Configuraton with the browser Data.

**/
VOID
OpalHiiGetBrowserData (
  VOID
  )
{
  HiiGetBrowserData(
      &gHiiSetupVariableGuid,
      (CHAR16*)L"OpalHiiConfig",
      sizeof(gHiiConfiguration),
      (UINT8*)&gHiiConfiguration
  );
}

/**
  Set a string Value in a form.

  @param      DestStringId   The stringid which need to update.
  @param      SrcAsciiStr    The string nned to update.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Other error occur.

**/
EFI_STATUS
HiiSetFormString(
  EFI_STRING_ID       DestStringId,
  CHAR8               *SrcAsciiStr
  )
{
  UINT32                    Len;
  UINT32                    UniSize;
  CHAR16*                   UniStr;

  //
  // Determine the Length of the sting
  //
  Len = ( UINT32 )AsciiStrLen( SrcAsciiStr );

  //
  // Allocate space for the unicode string, including terminator
  //
  UniSize = (Len + 1) * sizeof(CHAR16);
  UniStr = (CHAR16*)AllocateZeroPool(UniSize);

  //
  // Copy into unicode string, then copy into string id
  //
  AsciiStrToUnicodeStrS ( SrcAsciiStr, UniStr, Len + 1);

  //
  // Update the string in the form
  //
  if (HiiSetString(gHiiPackageListHandle, DestStringId, UniStr, NULL) == 0) {
    DEBUG ((DEBUG_INFO,  "HiiSetFormString( ) failed\n"));
    FreePool(UniStr);
    return (EFI_OUT_OF_RESOURCES);
  }

  //
  // Free the memory
  //
  FreePool(UniStr);

  return (EFI_SUCCESS);
}

/**
  Initialize the Opal disk base on the hardware info get from device.

  @param Dev                  The Opal device.

  @retval EFI_SUCESS          Initialize the device success.
  @retval EFI_DEVICE_ERROR    Get info from device failed.

**/
EFI_STATUS
OpalDiskInitialize (
  IN OPAL_DRIVER_DEVICE          *Dev
  )
{
  TCG_RESULT                  TcgResult;
  OPAL_SESSION                Session;

  ZeroMem(&Dev->OpalDisk, sizeof(OPAL_DISK));
  Dev->OpalDisk.Sscp = Dev->Sscp;
  Dev->OpalDisk.MediaId = Dev->MediaId;
  Dev->OpalDisk.OpalDevicePath = Dev->OpalDevicePath;

  ZeroMem(&Session, sizeof(Session));
  Session.Sscp = Dev->Sscp;
  Session.MediaId = Dev->MediaId;

  TcgResult = OpalGetSupportedAttributesInfo (&Session, &Dev->OpalDisk.SupportedAttributes, &Dev->OpalDisk.OpalBaseComId);
  if (TcgResult != TcgResultSuccess) {
    return EFI_DEVICE_ERROR;
  }
  Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;

  TcgResult = OpalUtilGetMsid (&Session, Dev->OpalDisk.Msid, OPAL_MSID_LENGHT, &Dev->OpalDisk.MsidLength);
  if (TcgResult != TcgResultSuccess) {
    return EFI_DEVICE_ERROR;
  }

  return OpalDiskUpdateStatus (&Dev->OpalDisk);
}

/**
  Update the device info.

  @param OpalDisk                The Opal device.

  @retval EFI_SUCESS             Initialize the device success.
  @retval EFI_DEVICE_ERROR       Get info from device failed.
  @retval EFI_INVALID_PARAMETER  Not get Msid info before get ownership info.

**/
EFI_STATUS
OpalDiskUpdateStatus (
  OPAL_DISK        *OpalDisk
  )
{
  TCG_RESULT                  TcgResult;
  OPAL_SESSION                Session;

  ZeroMem(&Session, sizeof(Session));
  Session.Sscp = OpalDisk->Sscp;
  Session.MediaId = OpalDisk->MediaId;
  Session.OpalBaseComId = OpalDisk->OpalBaseComId;

  TcgResult = OpalGetLockingInfo(&Session, &OpalDisk->LockingFeature);
  if (TcgResult != TcgResultSuccess) {
    return EFI_DEVICE_ERROR;
  }

  if (OpalDisk->MsidLength == 0) {
    return EFI_INVALID_PARAMETER;
  } else {
    //
    // Base on the Msid info to get the ownership, so Msid info must get first.
    //
    OpalDisk->Owner = OpalUtilDetermineOwnership(&Session, OpalDisk->Msid, OpalDisk->MsidLength);
  }

  return EFI_SUCCESS;
}

