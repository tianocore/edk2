/** @file
  Implementation of the HII for the Opal UEFI Driver.

Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "OpalHii.h"
//
// Character definitions
//
#define UPPER_LOWER_CASE_OFFSET  0x20

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

EFI_HII_CONFIG_ACCESS_PROTOCOL  gHiiConfigAccessProtocol;

//
// Handle to the list of HII packages (forms and strings) for this driver
//
EFI_HII_HANDLE  gHiiPackageListHandle = NULL;

//
// Package List GUID containing all form and string packages
//
const EFI_GUID  gHiiPackageListGuid   = PACKAGE_LIST_GUID;
const EFI_GUID  gHiiSetupVariableGuid = SETUP_VARIABLE_GUID;

//
// Structure that contains state of the HII
// This structure is updated by Hii.cpp and its contents
// is rendered in the HII.
//
OPAL_HII_CONFIGURATION  gHiiConfiguration;

//
// The device path containing the VENDOR_DEVICE_PATH and EFI_DEVICE_PATH_PROTOCOL
//
HII_VENDOR_DEVICE_PATH  gHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
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
  Get saved OPAL request.

  @param[in]  OpalDisk      The disk needs to get the saved OPAL request.
  @param[out] OpalRequest   OPAL request got.

**/
VOID
GetSavedOpalRequest (
  IN OPAL_DISK      *OpalDisk,
  OUT OPAL_REQUEST  *OpalRequest
  )
{
  EFI_STATUS                Status;
  OPAL_REQUEST_VARIABLE     *TempVariable;
  OPAL_REQUEST_VARIABLE     *Variable;
  UINTN                     VariableSize;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInVariable;
  UINTN                     DevicePathSizeInVariable;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     DevicePathSize;

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __FUNCTION__));

  Variable     = NULL;
  VariableSize = 0;

  Status = GetVariable2 (
             OPAL_REQUEST_VARIABLE_NAME,
             &gHiiSetupVariableGuid,
             (VOID **)&Variable,
             &VariableSize
             );
  if (EFI_ERROR (Status) || (Variable == NULL)) {
    return;
  }

  TempVariable = Variable;
  while ((VariableSize > sizeof (OPAL_REQUEST_VARIABLE)) &&
         (VariableSize >= TempVariable->Length) &&
         (TempVariable->Length > sizeof (OPAL_REQUEST_VARIABLE)))
  {
    DevicePathInVariable     = (EFI_DEVICE_PATH_PROTOCOL *)((UINTN)TempVariable + sizeof (OPAL_REQUEST_VARIABLE));
    DevicePathSizeInVariable = GetDevicePathSize (DevicePathInVariable);
    DevicePath               = OpalDisk->OpalDevicePath;
    DevicePathSize           = GetDevicePathSize (DevicePath);
    if ((DevicePathSize == DevicePathSizeInVariable) &&
        (CompareMem (DevicePath, DevicePathInVariable, DevicePathSize) == 0))
    {
      //
      // Found the node for the OPAL device.
      // Get the OPAL request.
      //
      CopyMem (OpalRequest, &TempVariable->OpalRequest, sizeof (OPAL_REQUEST));
      DEBUG ((
        DEBUG_INFO,
        "OpalRequest got: 0x%x\n",
        *OpalRequest
        ));
      break;
    }

    VariableSize -= TempVariable->Length;
    TempVariable  = (OPAL_REQUEST_VARIABLE *)((UINTN)TempVariable + TempVariable->Length);
  }

  FreePool (Variable);

  DEBUG ((DEBUG_INFO, "%a() - exit\n", __FUNCTION__));
}

/**
  Save OPAL request.

  @param[in] OpalDisk       The disk has OPAL request to save.
  @param[in] OpalRequest    OPAL request to save.

**/
VOID
SaveOpalRequest (
  IN OPAL_DISK     *OpalDisk,
  IN OPAL_REQUEST  OpalRequest
  )
{
  EFI_STATUS                Status;
  OPAL_REQUEST_VARIABLE     *TempVariable;
  UINTN                     TempVariableSize;
  OPAL_REQUEST_VARIABLE     *Variable;
  UINTN                     VariableSize;
  OPAL_REQUEST_VARIABLE     *NewVariable;
  UINTN                     NewVariableSize;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInVariable;
  UINTN                     DevicePathSizeInVariable;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     DevicePathSize;

  DEBUG ((DEBUG_INFO, "%a() - enter\n", __FUNCTION__));

  DEBUG ((
    DEBUG_INFO,
    "OpalRequest to save: 0x%x\n",
    OpalRequest
    ));

  Variable        = NULL;
  VariableSize    = 0;
  NewVariable     = NULL;
  NewVariableSize = 0;

  Status = GetVariable2 (
             OPAL_REQUEST_VARIABLE_NAME,
             &gHiiSetupVariableGuid,
             (VOID **)&Variable,
             &VariableSize
             );
  if (!EFI_ERROR (Status) && (Variable != NULL)) {
    TempVariable     = Variable;
    TempVariableSize = VariableSize;
    while ((TempVariableSize > sizeof (OPAL_REQUEST_VARIABLE)) &&
           (TempVariableSize >= TempVariable->Length) &&
           (TempVariable->Length > sizeof (OPAL_REQUEST_VARIABLE)))
    {
      DevicePathInVariable     = (EFI_DEVICE_PATH_PROTOCOL *)((UINTN)TempVariable + sizeof (OPAL_REQUEST_VARIABLE));
      DevicePathSizeInVariable = GetDevicePathSize (DevicePathInVariable);
      DevicePath               = OpalDisk->OpalDevicePath;
      DevicePathSize           = GetDevicePathSize (DevicePath);
      if ((DevicePathSize == DevicePathSizeInVariable) &&
          (CompareMem (DevicePath, DevicePathInVariable, DevicePathSize) == 0))
      {
        //
        // Found the node for the OPAL device.
        // Update the OPAL request.
        //
        CopyMem (&TempVariable->OpalRequest, &OpalRequest, sizeof (OPAL_REQUEST));
        NewVariable     = Variable;
        NewVariableSize = VariableSize;
        break;
      }

      TempVariableSize -= TempVariable->Length;
      TempVariable      = (OPAL_REQUEST_VARIABLE *)((UINTN)TempVariable + TempVariable->Length);
    }

    if (NewVariable == NULL) {
      //
      // The node for the OPAL device is not found.
      // Create node for the OPAL device.
      //
      DevicePath      = OpalDisk->OpalDevicePath;
      DevicePathSize  = GetDevicePathSize (DevicePath);
      NewVariableSize = VariableSize + sizeof (OPAL_REQUEST_VARIABLE) + DevicePathSize;
      NewVariable     = AllocatePool (NewVariableSize);
      ASSERT (NewVariable != NULL);
      CopyMem (NewVariable, Variable, VariableSize);
      TempVariable         = (OPAL_REQUEST_VARIABLE *)((UINTN)NewVariable + VariableSize);
      TempVariable->Length = (UINT32)(sizeof (OPAL_REQUEST_VARIABLE) + DevicePathSize);
      CopyMem (&TempVariable->OpalRequest, &OpalRequest, sizeof (OPAL_REQUEST));
      DevicePathInVariable = (EFI_DEVICE_PATH_PROTOCOL *)((UINTN)TempVariable + sizeof (OPAL_REQUEST_VARIABLE));
      CopyMem (DevicePathInVariable, DevicePath, DevicePathSize);
    }
  } else {
    DevicePath      = OpalDisk->OpalDevicePath;
    DevicePathSize  = GetDevicePathSize (DevicePath);
    NewVariableSize = sizeof (OPAL_REQUEST_VARIABLE) + DevicePathSize;
    NewVariable     = AllocatePool (NewVariableSize);
    ASSERT (NewVariable != NULL);
    NewVariable->Length = (UINT32)(sizeof (OPAL_REQUEST_VARIABLE) + DevicePathSize);
    CopyMem (&NewVariable->OpalRequest, &OpalRequest, sizeof (OPAL_REQUEST));
    DevicePathInVariable = (EFI_DEVICE_PATH_PROTOCOL *)((UINTN)NewVariable + sizeof (OPAL_REQUEST_VARIABLE));
    CopyMem (DevicePathInVariable, DevicePath, DevicePathSize);
  }

  Status = gRT->SetVariable (
                  OPAL_REQUEST_VARIABLE_NAME,
                  (EFI_GUID *)&gHiiSetupVariableGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  NewVariableSize,
                  NewVariable
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OpalRequest variable set failed (%r)\n", Status));
  }

  if (NewVariable != Variable) {
    FreePool (NewVariable);
  }

  if (Variable != NULL) {
    FreePool (Variable);
  }

  DEBUG ((DEBUG_INFO, "%a() - exit\n", __FUNCTION__));
}

/**
  Sets the current system state of global config variables.

**/
VOID
HiiSetCurrentConfiguration (
  VOID
  )
{
  UINT32      PpStorageFlag;
  EFI_STRING  NewString;

  gHiiConfiguration.NumDisks = GetDeviceCount ();

  //
  // Update the BlockSID status string.
  //
  PpStorageFlag = Tcg2PhysicalPresenceLibGetManagementFlags ();

  if ((PpStorageFlag & TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_ENABLE_BLOCK_SID) != 0) {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN (STR_ENABLED), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO, "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  } else {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN (STR_DISABLED), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO, "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  }

  HiiSetString (gHiiPackageListHandle, STRING_TOKEN (STR_BLOCKSID_STATUS1), NewString, NULL);
  FreePool (NewString);

  if ((PpStorageFlag & TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_ENABLE_BLOCK_SID) != 0) {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN (STR_DISK_INFO_ENABLE_BLOCKSID_TRUE), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO, "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  } else {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN (STR_DISK_INFO_ENABLE_BLOCKSID_FALSE), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO, "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  }

  HiiSetString (gHiiPackageListHandle, STRING_TOKEN (STR_BLOCKSID_STATUS2), NewString, NULL);
  FreePool (NewString);

  if ((PpStorageFlag & TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_DISABLE_BLOCK_SID) != 0) {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN (STR_DISK_INFO_DISABLE_BLOCKSID_TRUE), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO, "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  } else {
    NewString = HiiGetString (gHiiPackageListHandle, STRING_TOKEN (STR_DISK_INFO_DISABLE_BLOCKSID_FALSE), NULL);
    if (NewString == NULL) {
      DEBUG ((DEBUG_INFO, "HiiSetCurrentConfiguration: HiiGetString( ) failed\n"));
      return;
    }
  }

  HiiSetString (gHiiPackageListHandle, STRING_TOKEN (STR_BLOCKSID_STATUS3), NewString, NULL);
  FreePool (NewString);
}

/**
  Install the HII related resources.

  @retval  EFI_SUCCESS        Install all the resources success.
  @retval  other              Error occur when install the resources.
**/
EFI_STATUS
HiiInstall (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DriverHandle;

  //
  // Clear the global configuration.
  //
  ZeroMem (&gHiiConfiguration, sizeof (gHiiConfiguration));

  //
  // Obtain the driver handle that the BIOS assigned us
  //
  DriverHandle = HiiGetDriverImageHandleCB ();

  //
  // Populate the config access protocol with the three functions we are publishing
  //
  gHiiConfigAccessProtocol.ExtractConfig = ExtractConfig;
  gHiiConfigAccessProtocol.RouteConfig   = RouteConfig;
  gHiiConfigAccessProtocol.Callback      = DriverCallback;

  //
  // Associate the required protocols with our driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &DriverHandle,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gHiiConfigAccessProtocol,   // HII callback
                  &gEfiDevicePathProtocolGuid,
                  &gHiiVendorDevicePath,     // required for HII callback allow all disks to be shown in same hii
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return OpalHiiAddPackages ();
}

/**
  Install the HII form and string packages.

  @retval  EFI_SUCCESS           Install all the resources success.
  @retval  EFI_OUT_OF_RESOURCES  Out of resource error.
**/
EFI_STATUS
OpalHiiAddPackages (
  VOID
  )
{
  EFI_HANDLE  DriverHandle;

  DriverHandle = HiiGetDriverImageHandleCB ();

  //
  // Publish the HII form and HII string packages
  //
  gHiiPackageListHandle = HiiAddPackages (
                            &gHiiPackageListGuid,
                            DriverHandle,
                            OpalPasswordDxeStrings,
                            OpalPasswordFormBin,
                            (VOID *)NULL
                            );

  //
  // Make sure the packages installed successfully
  //
  if (gHiiPackageListHandle == NULL) {
    DEBUG ((DEBUG_INFO, "OpalHiiAddPackages failed\n"));
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
HiiUninstall (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Remove the packages we've provided to the BIOS
  //
  HiiRemovePackages (gHiiPackageListHandle);

  //
  // Remove the protocols from our driver handle
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  HiiGetDriverImageHandleCB (),
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gHiiConfigAccessProtocol,                // HII callback
                  &gEfiDevicePathProtocolGuid,
                  &gHiiVendorDevicePath,                    // required for HII callback
                  NULL
                  );
  if (EFI_ERROR (Status)) {
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
  UINT8          Index;
  CHAR8          *DiskName;
  EFI_STRING_ID  DiskNameId;
  OPAL_DISK      *OpalDisk;

  HiiSetCurrentConfiguration ();

  gHiiConfiguration.SupportedDisks = 0;

  for (Index = 0; Index < gHiiConfiguration.NumDisks; Index++) {
    OpalDisk = HiiGetOpalDiskCB (Index);
    if ((OpalDisk != NULL) && OpalFeatureSupported (&OpalDisk->SupportedAttributes)) {
      gHiiConfiguration.SupportedDisks |= (1 << Index);
      DiskNameId                        = GetDiskNameStringId (Index);
      DiskName                          = HiiDiskGetNameCB (Index);
      if ((DiskName == NULL) || (DiskNameId == 0)) {
        return EFI_UNSUPPORTED;
      }

      HiiSetFormString (DiskNameId, DiskName);
    }
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
GetDiskNameStringId (
  UINT8  DiskIndex
  )
{
  switch (DiskIndex) {
    case 0: return STRING_TOKEN (STR_MAIN_GOTO_DISK_INFO_0);
    case 1: return STRING_TOKEN (STR_MAIN_GOTO_DISK_INFO_1);
    case 2: return STRING_TOKEN (STR_MAIN_GOTO_DISK_INFO_2);
    case 3: return STRING_TOKEN (STR_MAIN_GOTO_DISK_INFO_3);
    case 4: return STRING_TOKEN (STR_MAIN_GOTO_DISK_INFO_4);
    case 5: return STRING_TOKEN (STR_MAIN_GOTO_DISK_INFO_5);
  }

  return 0;
}

/**
  Confirm whether user truly want to do the revert action.

  @param     OpalDisk            The device which need to perform data removal action.
  @param     ActionString        Specifies the action name shown on pop up menu.

  @retval  EFI_SUCCESS           Confirmed user want to do the revert action.
**/
EFI_STATUS
HiiConfirmDataRemovalAction (
  IN OPAL_DISK  *OpalDisk,
  IN CHAR16     *ActionString

  )
{
  CHAR16         Unicode[512];
  EFI_INPUT_KEY  Key;
  CHAR16         ApproveResponse;
  CHAR16         RejectResponse;

  //
  // When the estimate cost time bigger than MAX_ACCEPTABLE_REVERTING_TIME, pop up dialog to let user confirm
  // the revert action.
  //
  if (OpalDisk->EstimateTimeCost < MAX_ACCEPTABLE_REVERTING_TIME) {
    return EFI_SUCCESS;
  }

  ApproveResponse = L'Y';
  RejectResponse  = L'N';

  UnicodeSPrint (Unicode, StrSize (L"WARNING: ############# action needs about ####### seconds"), L"WARNING: %s action needs about %d seconds", ActionString, OpalDisk->EstimateTimeCost);

  do {
    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      Unicode,
      L" System should not be powered off until action completion ",
      L" ",
      L" Press 'Y/y' to continue, press 'N/n' to cancel ",
      NULL
      );
  } while (
           ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (ApproveResponse | UPPER_LOWER_CASE_OFFSET)) &&
           ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (RejectResponse | UPPER_LOWER_CASE_OFFSET))
           );

  if ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (RejectResponse | UPPER_LOWER_CASE_OFFSET)) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
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
DriverCallback (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  EFI_BROWSER_ACTION                    Action,
  EFI_QUESTION_ID                       QuestionId,
  UINT8                                 Type,
  EFI_IFR_TYPE_VALUE                    *Value,
  EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  HII_KEY    HiiKey;
  UINT8      HiiKeyId;
  UINT32     PpRequest;
  OPAL_DISK  *OpalDisk;

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
  HiiKeyId   = (UINT8)HiiKey.KeyBits.Id;

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    switch (HiiKeyId) {
      case HII_KEY_ID_VAR_SUPPORTED_DISKS:
        DEBUG ((DEBUG_INFO, "HII_KEY_ID_VAR_SUPPORTED_DISKS\n"));
        return HiiPopulateMainMenuForm ();

      case HII_KEY_ID_VAR_SELECTED_DISK_AVAILABLE_ACTIONS:
        DEBUG ((DEBUG_INFO, "HII_KEY_ID_VAR_SELECTED_DISK_AVAILABLE_ACTIONS\n"));
        return HiiPopulateDiskInfoForm ();
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGING) {
    switch (HiiKeyId) {
      case HII_KEY_ID_GOTO_DISK_INFO:
        return HiiSelectDisk ((UINT8)HiiKey.KeyBits.Index);

      case HII_KEY_ID_REVERT:
      case HII_KEY_ID_PSID_REVERT:
        OpalDisk = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);
        if (OpalDisk != NULL) {
          return HiiConfirmDataRemovalAction (OpalDisk, L"Revert");
        } else {
          ASSERT (FALSE);
          return EFI_SUCCESS;
        }

      case HII_KEY_ID_SECURE_ERASE:
        OpalDisk = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);
        if (OpalDisk != NULL) {
          return HiiConfirmDataRemovalAction (OpalDisk, L"Secure erase");
        } else {
          ASSERT (FALSE);
          return EFI_SUCCESS;
        }
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

        HiiSetBlockSidAction (PpRequest);

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        return EFI_SUCCESS;

      case HII_KEY_ID_SET_ADMIN_PWD:
        DEBUG ((DEBUG_INFO, "HII_KEY_ID_SET_ADMIN_PWD\n"));
        gHiiConfiguration.OpalRequest.SetAdminPwd = Value->b;
        OpalDisk                                  = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);
        if (OpalDisk != NULL) {
          SaveOpalRequest (OpalDisk, gHiiConfiguration.OpalRequest);
        }

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        return EFI_SUCCESS;

      case HII_KEY_ID_SET_USER_PWD:
        DEBUG ((DEBUG_INFO, "HII_KEY_ID_SET_USER_PWD\n"));
        gHiiConfiguration.OpalRequest.SetUserPwd = Value->b;
        OpalDisk                                 = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);
        if (OpalDisk != NULL) {
          SaveOpalRequest (OpalDisk, gHiiConfiguration.OpalRequest);
        }

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        return EFI_SUCCESS;

      case HII_KEY_ID_SECURE_ERASE:
        DEBUG ((DEBUG_INFO, "HII_KEY_ID_SECURE_ERASE\n"));
        gHiiConfiguration.OpalRequest.SecureErase = Value->b;
        OpalDisk                                  = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);
        if (OpalDisk != NULL) {
          SaveOpalRequest (OpalDisk, gHiiConfiguration.OpalRequest);
        }

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        return EFI_SUCCESS;

      case HII_KEY_ID_REVERT:
        DEBUG ((DEBUG_INFO, "HII_KEY_ID_REVERT\n"));
        gHiiConfiguration.OpalRequest.Revert = Value->b;
        OpalDisk                             = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);
        if (OpalDisk != NULL) {
          SaveOpalRequest (OpalDisk, gHiiConfiguration.OpalRequest);
        }

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        return EFI_SUCCESS;
      case HII_KEY_ID_KEEP_USER_DATA:
        DEBUG ((DEBUG_INFO, "HII_KEY_ID_KEEP_USER_DATA\n"));
        gHiiConfiguration.OpalRequest.KeepUserData = Value->b;
        OpalDisk                                   = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);
        if (OpalDisk != NULL) {
          SaveOpalRequest (OpalDisk, gHiiConfiguration.OpalRequest);
        }

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        return EFI_SUCCESS;

      case HII_KEY_ID_PSID_REVERT:
        DEBUG ((DEBUG_INFO, "HII_KEY_ID_PSID_REVERT\n"));
        gHiiConfiguration.OpalRequest.PsidRevert = Value->b;
        OpalDisk                                 = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);
        if (OpalDisk != NULL) {
          SaveOpalRequest (OpalDisk, gHiiConfiguration.OpalRequest);
        }

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        return EFI_SUCCESS;

      case HII_KEY_ID_DISABLE_USER:
        DEBUG ((DEBUG_INFO, "HII_KEY_ID_DISABLE_USER\n"));
        gHiiConfiguration.OpalRequest.DisableUser = Value->b;
        OpalDisk                                  = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);
        if (OpalDisk != NULL) {
          SaveOpalRequest (OpalDisk, gHiiConfiguration.OpalRequest);
        }

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        return EFI_SUCCESS;

      case HII_KEY_ID_ENABLE_FEATURE:
        DEBUG ((DEBUG_INFO, "HII_KEY_ID_ENABLE_FEATURE\n"));
        gHiiConfiguration.OpalRequest.EnableFeature = Value->b;
        OpalDisk                                    = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);
        if (OpalDisk != NULL) {
          SaveOpalRequest (OpalDisk, gHiiConfiguration.OpalRequest);
        }

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
HiiSelectDisk (
  UINT8  Index
  )
{
  OpalHiiGetBrowserData ();
  gHiiConfiguration.SelectedDiskIndex = Index;
  OpalHiiSetBrowserData ();

  return EFI_SUCCESS;
}

/**
  Draws the disk info form.

  @retval  EFI_SUCCESS       Draw the disk info success.

**/
EFI_STATUS
HiiPopulateDiskInfoForm (
  VOID
  )
{
  OPAL_DISK          *OpalDisk;
  OPAL_DISK_ACTIONS  AvailActions;
  TCG_RESULT         Ret;
  CHAR8              *DiskName;

  OpalHiiGetBrowserData ();

  DiskName = HiiDiskGetNameCB (gHiiConfiguration.SelectedDiskIndex);
  if (DiskName == NULL) {
    return EFI_UNSUPPORTED;
  }

  HiiSetFormString (STRING_TOKEN (STR_DISK_INFO_SELECTED_DISK_NAME), DiskName);

  gHiiConfiguration.SelectedDiskAvailableActions = HII_ACTION_NONE;
  ZeroMem (&gHiiConfiguration.OpalRequest, sizeof (OPAL_REQUEST));
  gHiiConfiguration.KeepUserDataForced = FALSE;

  OpalDisk = HiiGetOpalDiskCB (gHiiConfiguration.SelectedDiskIndex);

  if (OpalDisk != NULL) {
    OpalDiskUpdateStatus (OpalDisk);
    Ret = OpalSupportGetAvailableActions (&OpalDisk->SupportedAttributes, &OpalDisk->LockingFeature, OpalDisk->Owner, &AvailActions);
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
          HiiSetFormString (STRING_TOKEN (STR_DISK_INFO_PSID_REVERT), "PSID Revert to factory default");
        } else {
          DEBUG ((DEBUG_INFO, "Feature disabled but ownership != nobody\n"));
        }
      } else {
        gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.Revert == 1) ? HII_ACTION_REVERT : HII_ACTION_NONE;
        gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.AdminPass == 1) ? HII_ACTION_SET_ADMIN_PWD : HII_ACTION_NONE;
        gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.UserPass == 1) ? HII_ACTION_SET_USER_PWD : HII_ACTION_NONE;
        gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.SecureErase == 1) ? HII_ACTION_SECURE_ERASE : HII_ACTION_NONE;
        gHiiConfiguration.SelectedDiskAvailableActions |= (AvailActions.DisableUser == 1) ? HII_ACTION_DISABLE_USER : HII_ACTION_NONE;

        HiiSetFormString (STRING_TOKEN (STR_DISK_INFO_PSID_REVERT), "PSID Revert to factory default and Disable");

        //
        // Determine revert options for disk
        // Default initialize keep user Data to be true
        //
        gHiiConfiguration.OpalRequest.KeepUserData = 1;
        if (AvailActions.RevertKeepDataForced) {
          gHiiConfiguration.KeepUserDataForced = TRUE;
        }
      }
    }

    GetSavedOpalRequest (OpalDisk, &gHiiConfiguration.OpalRequest);
  }

  //
  // Pass the current configuration to the BIOS
  //
  OpalHiiSetBrowserData ();

  return EFI_SUCCESS;
}

/**
  Send BlockSid request through TPM physical presence module.

  @param   PpRequest         TPM physical presence operation request.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Other error occur.

**/
EFI_STATUS
HiiSetBlockSidAction (
  IN UINT32  PpRequest
  )
{
  UINT32      ReturnCode;
  EFI_STATUS  Status;

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
RouteConfig (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  CONST EFI_STRING                      Configuration,
  EFI_STRING                            *Progress
  )
{
  if ((Configuration == NULL) || (Progress == NULL)) {
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
ExtractConfig (
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  CONST EFI_STRING                      Request,
  EFI_STRING                            *Progress,
  EFI_STRING                            *Results
  )
{
  EFI_STATUS  Status;
  EFI_STRING  ConfigRequest;
  EFI_STRING  ConfigRequestHdr;
  UINTN       BufferSize;
  UINTN       Size;
  BOOLEAN     AllocatedRequest;
  EFI_HANDLE  DriverHandle;

  //
  // Check for valid parameters
  //
  if ((Progress == NULL) || (Results == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  *Progress = Request;
  if ((Request != NULL) &&
      !HiiIsConfigHdrMatch (Request, &gHiiSetupVariableGuid, OpalPasswordStorageName))
  {
    return EFI_NOT_FOUND;
  }

  AllocatedRequest = FALSE;
  BufferSize       = sizeof (OPAL_HII_CONFIGURATION);
  ConfigRequest    = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    DriverHandle     = HiiGetDriverImageHandleCB ();
    ConfigRequestHdr = HiiConstructConfigHdr (&gHiiSetupVariableGuid, OpalPasswordStorageName, DriverHandle);
    Size             = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest    = AllocateZeroPool (Size);
    if (ConfigRequest == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  //
  // Convert Buffer Data to <ConfigResp> by helper function BlockToConfig( )
  //
  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *)&gHiiConfiguration,
                                sizeof (OPAL_HII_CONFIGURATION),
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
  HiiSetBrowserData (
    &gHiiSetupVariableGuid,
    (CHAR16 *)L"OpalHiiConfig",
    sizeof (gHiiConfiguration),
    (UINT8 *)&gHiiConfiguration,
    NULL
    );
}

/**

  Populate the hii_g_Configuration with the browser Data.

**/
VOID
OpalHiiGetBrowserData (
  VOID
  )
{
  HiiGetBrowserData (
    &gHiiSetupVariableGuid,
    (CHAR16 *)L"OpalHiiConfig",
    sizeof (gHiiConfiguration),
    (UINT8 *)&gHiiConfiguration
    );
}

/**
  Set a string Value in a form.

  @param      DestStringId   The stringid which need to update.
  @param      SrcAsciiStr    The string need to update.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Other error occur.

**/
EFI_STATUS
HiiSetFormString (
  EFI_STRING_ID  DestStringId,
  CHAR8          *SrcAsciiStr
  )
{
  UINT32  Len;
  UINT32  UniSize;
  CHAR16  *UniStr;

  //
  // Determine the Length of the sting
  //
  Len = (UINT32)AsciiStrLen (SrcAsciiStr);

  //
  // Allocate space for the unicode string, including terminator
  //
  UniSize = (Len + 1) * sizeof (CHAR16);
  UniStr  = (CHAR16 *)AllocateZeroPool (UniSize);

  //
  // Copy into unicode string, then copy into string id
  //
  AsciiStrToUnicodeStrS (SrcAsciiStr, UniStr, Len + 1);

  //
  // Update the string in the form
  //
  if (HiiSetString (gHiiPackageListHandle, DestStringId, UniStr, NULL) == 0) {
    DEBUG ((DEBUG_INFO, "HiiSetFormString( ) failed\n"));
    FreePool (UniStr);
    return (EFI_OUT_OF_RESOURCES);
  }

  //
  // Free the memory
  //
  FreePool (UniStr);

  return (EFI_SUCCESS);
}

/**
  Initialize the Opal disk base on the hardware info get from device.

  @param Dev                  The Opal device.

  @retval EFI_SUCCESS         Initialize the device success.
  @retval EFI_DEVICE_ERROR    Get info from device failed.

**/
EFI_STATUS
OpalDiskInitialize (
  IN OPAL_DRIVER_DEVICE  *Dev
  )
{
  TCG_RESULT    TcgResult;
  OPAL_SESSION  Session;
  UINT8         ActiveDataRemovalMechanism;
  UINT32        RemovalMechanishLists[ResearvedMechanism];

  ZeroMem (&Dev->OpalDisk, sizeof (OPAL_DISK));
  Dev->OpalDisk.Sscp           = Dev->Sscp;
  Dev->OpalDisk.MediaId        = Dev->MediaId;
  Dev->OpalDisk.OpalDevicePath = Dev->OpalDevicePath;

  ZeroMem (&Session, sizeof (Session));
  Session.Sscp    = Dev->Sscp;
  Session.MediaId = Dev->MediaId;

  TcgResult = OpalGetSupportedAttributesInfo (&Session, &Dev->OpalDisk.SupportedAttributes, &Dev->OpalDisk.OpalBaseComId);
  if (TcgResult != TcgResultSuccess) {
    return EFI_DEVICE_ERROR;
  }

  Session.OpalBaseComId = Dev->OpalDisk.OpalBaseComId;

  TcgResult = OpalUtilGetMsid (&Session, Dev->OpalDisk.Msid, OPAL_MSID_LENGTH, &Dev->OpalDisk.MsidLength);
  if (TcgResult != TcgResultSuccess) {
    return EFI_DEVICE_ERROR;
  }

  if (Dev->OpalDisk.SupportedAttributes.DataRemoval) {
    TcgResult = OpalUtilGetDataRemovalMechanismLists (&Session, RemovalMechanishLists);
    if (TcgResult != TcgResultSuccess) {
      return EFI_DEVICE_ERROR;
    }

    TcgResult = OpalUtilGetActiveDataRemovalMechanism (&Session, Dev->OpalDisk.Msid, Dev->OpalDisk.MsidLength, &ActiveDataRemovalMechanism);
    if (TcgResult != TcgResultSuccess) {
      return EFI_DEVICE_ERROR;
    }

    Dev->OpalDisk.EstimateTimeCost = RemovalMechanishLists[ActiveDataRemovalMechanism];
  }

  return OpalDiskUpdateStatus (&Dev->OpalDisk);
}

/**
  Update the device ownship

  @param OpalDisk                The Opal device.

  @retval EFI_SUCCESS            Get ownership success.
  @retval EFI_ACCESS_DENIED      Has send BlockSID command, can't change ownership.
  @retval EFI_INVALID_PARAMETER  Not get Msid info before get ownership info.

**/
EFI_STATUS
OpalDiskUpdateOwnerShip (
  OPAL_DISK  *OpalDisk
  )
{
  OPAL_SESSION  Session;

  if (OpalDisk->MsidLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (OpalDisk->SentBlockSID) {
    return EFI_ACCESS_DENIED;
  }

  ZeroMem (&Session, sizeof (Session));
  Session.Sscp          = OpalDisk->Sscp;
  Session.MediaId       = OpalDisk->MediaId;
  Session.OpalBaseComId = OpalDisk->OpalBaseComId;

  OpalDisk->Owner = OpalUtilDetermineOwnership (&Session, OpalDisk->Msid, OpalDisk->MsidLength);
  return EFI_SUCCESS;
}

/**
  Update the device info.

  @param OpalDisk                The Opal device.

  @retval EFI_SUCCESS            Initialize the device success.
  @retval EFI_DEVICE_ERROR       Get info from device failed.
  @retval EFI_INVALID_PARAMETER  Not get Msid info before get ownership info.
  @retval EFI_ACCESS_DENIED      Has send BlockSID command, can't change ownership.

**/
EFI_STATUS
OpalDiskUpdateStatus (
  OPAL_DISK  *OpalDisk
  )
{
  TCG_RESULT    TcgResult;
  OPAL_SESSION  Session;

  ZeroMem (&Session, sizeof (Session));
  Session.Sscp          = OpalDisk->Sscp;
  Session.MediaId       = OpalDisk->MediaId;
  Session.OpalBaseComId = OpalDisk->OpalBaseComId;

  TcgResult = OpalGetLockingInfo (&Session, &OpalDisk->LockingFeature);
  if (TcgResult != TcgResultSuccess) {
    return EFI_DEVICE_ERROR;
  }

  return OpalDiskUpdateOwnerShip (OpalDisk);
}
