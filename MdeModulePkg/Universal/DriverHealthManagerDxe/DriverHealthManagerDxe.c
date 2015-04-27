/** @file
  This module produces two driver health manager forms.
  One will be used by BDS core to configure the Configured Required
  driver health instances, the other will be automatically included by
  firmware setup (UI).

Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DriverHealthManagerDxe.h"
#include "DriverHealthManagerVfr.h"

EFI_HII_CONFIG_ACCESS_PROTOCOL mDriverHealthManagerConfigAccess     = {
  DriverHealthManagerFakeExtractConfig,
  DriverHealthManagerFakeRouteConfig,
  DriverHealthManagerCallback
};

EFI_GUID mDriverHealthManagerForm = DRIVER_HEALTH_MANAGER_FORMSET_GUID;

FORM_DEVICE_PATH  mDriverHealthManagerFormDevicePath = {
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

EFI_HII_HANDLE                       mDriverHealthManagerHiiHandle;
EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO  *mDriverHealthManagerHealthInfo     = NULL;
UINTN                                mDriverHealthManagerHealthInfoCount = 0;
EFI_HII_DATABASE_PROTOCOL            *mDriverHealthManagerDatabase;


extern UINT8 DriverHealthManagerVfrBin[];
extern UINT8 DriverHealthConfigureVfrBin[];

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
DriverHealthManagerFakeExtractConfig (
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
DriverHealthManagerFakeRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_NOT_FOUND;
}

/**

  Install the health manager forms.
  One will be used by BDS core to configure the Configured Required
  driver health instances, the other will be automatically included by
  firmware setup (UI).

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS   The health manager forms are successfully installed.

**/
EFI_STATUS
EFIAPI
InitializeDriverHealthManager (
  EFI_HANDLE                 ImageHandle,
  EFI_SYSTEM_TABLE           *SystemTable
  )
{
  EFI_STATUS                  Status;
  EFI_HANDLE                  Handle;

  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &mDriverHealthManagerDatabase
                  );
  ASSERT_EFI_ERROR (Status);

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiDevicePathProtocolGuid,
                  &mDriverHealthManagerFormDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mDriverHealthManagerConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);


  //
  // Publish Driver Health HII data.
  //
  mDriverHealthManagerHiiHandle = HiiAddPackages (
                                    &gEfiCallerIdGuid,
                                    Handle,
                                    DriverHealthManagerVfrBin,
                                    DriverHealthConfigureVfrBin,
                                    STRING_ARRAY_NAME,
                                    NULL
                                    );
  ASSERT (mDriverHealthManagerHiiHandle != NULL);

  return EFI_SUCCESS;
}

/**

  Select the best matching language according to front page policy for best user experience.

  This function supports both ISO 639-2 and RFC 4646 language codes, but language
  code types may not be mixed in a single call to this function.

  @param  SupportedLanguages   A pointer to a Null-terminated ASCII string that
                               contains a set of language codes in the format
                               specified by Iso639Language.
  @param  Iso639Language       If TRUE, then all language codes are assumed to be
                               in ISO 639-2 format.  If FALSE, then all language
                               codes are assumed to be in RFC 4646 language format.

  @retval NULL                 The best matching language could not be found in SupportedLanguages.
  @retval NULL                 There are not enough resources available to return the best matching
                               language.
  @retval Other                A pointer to a Null-terminated ASCII string that is the best matching
                               language in SupportedLanguages.
**/
CHAR8 *
DriverHealthManagerSelectBestLanguage (
  IN CHAR8        *SupportedLanguages,
  IN BOOLEAN      Iso639Language
  )
{
  CHAR8           *LanguageVariable;
  CHAR8           *BestLanguage;

  LanguageVariable =  GetEfiGlobalVariable (Iso639Language ? L"Lang" : L"PlatformLang");

  BestLanguage = GetBestLanguage(
                   SupportedLanguages,
                   Iso639Language,
                   (LanguageVariable != NULL) ? LanguageVariable : "",
                   Iso639Language ? "eng" : "en-US",
                   NULL
                   );
  if (LanguageVariable != NULL) {
    FreePool (LanguageVariable);
  }

  return BestLanguage;
}



/**

  This is an internal worker function to get the Component Name (2) protocol interface
  and the language it supports.

  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ComponentName        A pointer to the Component Name (2) protocol interface.
  @param  SupportedLanguage    The best suitable language that matches the SupportedLangues interface for the
                               located Component Name (2) instance.

  @retval EFI_SUCCESS          The Component Name (2) protocol instance is successfully located and we find
                               the best matching language it support.
  @retval EFI_UNSUPPORTED      The input Language is not supported by the Component Name (2) protocol.
  @retval Other                Some error occurs when locating Component Name (2) protocol instance or finding
                               the supported language.

**/
EFI_STATUS
DriverHealthManagerGetComponentNameWorker (
  IN  EFI_GUID                    *ProtocolGuid,
  IN  EFI_HANDLE                  DriverBindingHandle,
  OUT EFI_COMPONENT_NAME_PROTOCOL **ComponentName,
  OUT CHAR8                       **SupportedLanguage
  )
{
  EFI_STATUS                      Status;

  //
  // Locate Component Name (2) protocol on the driver binging handle.
  //
  Status = gBS->OpenProtocol (
                 DriverBindingHandle,
                 ProtocolGuid,
                 (VOID **) ComponentName,
                 NULL,
                 NULL,
                 EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Apply shell policy to select the best language.
  //
  *SupportedLanguage = DriverHealthManagerSelectBestLanguage (
                         (*ComponentName)->SupportedLanguages,
                         (BOOLEAN) (ProtocolGuid == &gEfiComponentNameProtocolGuid)
                         );
  if (*SupportedLanguage == NULL) {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/**

  This is an internal worker function to get driver name from Component Name (2) protocol interface.

  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  DriverName           A pointer to the Unicode string to return. This Unicode string is the name
                               of the driver specified by This.

  @retval EFI_SUCCESS          The driver name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval Other                The driver name cannot be retrieved from Component Name (2) protocol
                               interface.

**/
EFI_STATUS
DriverHealthManagerGetDriverNameWorker (
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  DriverBindingHandle,
  OUT CHAR16      **DriverName
  )
{
  EFI_STATUS                     Status;
  CHAR8                          *BestLanguage;
  EFI_COMPONENT_NAME_PROTOCOL    *ComponentName;

  //
  // Retrieve Component Name (2) protocol instance on the driver binding handle and
  // find the best language this instance supports.
  //
  Status = DriverHealthManagerGetComponentNameWorker (
             ProtocolGuid,
             DriverBindingHandle,
             &ComponentName,
             &BestLanguage
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the driver name from Component Name (2) protocol instance on the driver binging handle.
  //
  Status = ComponentName->GetDriverName (
                            ComponentName,
                            BestLanguage,
                            DriverName
                            );
  FreePool (BestLanguage);

  return Status;
}

/**
  This function gets driver name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the driver name.
  If the attempt fails, it then gets the driver name from EFI 1.1 Component Name protocol for backward
  compatibility support.

  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.

  @return A pointer to the Unicode string to return. This Unicode string is the name of the controller
          specified by ControllerHandle and ChildHandle.


**/
CHAR16 *
DriverHealthManagerGetDriverName (
  IN  EFI_HANDLE  DriverBindingHandle
  )
{
  EFI_STATUS      Status;
  CHAR16          *DriverName;

  //
  // Get driver name from UEFI 2.0 Component Name 2 protocol interface.
  //
  Status = DriverHealthManagerGetDriverNameWorker (&gEfiComponentName2ProtocolGuid, DriverBindingHandle, &DriverName);
  if (EFI_ERROR (Status)) {
    //
    // If it fails to get the driver name from Component Name protocol interface, we should fall back on
    // EFI 1.1 Component Name protocol interface.
    //
    Status = DriverHealthManagerGetDriverNameWorker (&gEfiComponentNameProtocolGuid, DriverBindingHandle, &DriverName);
  }

  if (!EFI_ERROR (Status)) {
    return AllocateCopyPool (StrSize (DriverName), DriverName);
  } else {
    return ConvertDevicePathToText (DevicePathFromHandle (DriverBindingHandle), FALSE, TRUE);
  }
}



/**
  This function gets controller name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the controller name.
  If the attempt fails, it then gets the controller name from EFI 1.1 Component Name protocol for backward
  compatibility support.

  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ControllerHandle     The handle of a controller that the driver specified by This is managing.
                               This handle specifies the controller whose name is to be returned.
  @param  ChildHandle          The handle of the child controller to retrieve the name of. This is an
                               optional parameter that may be NULL. It will be NULL for device drivers.
                               It will also be NULL for bus drivers that attempt to retrieve the name
                               of the bus controller. It will not be NULL for a bus driver that attempts
                               to retrieve the name of a child controller.
  @param  ControllerName       A pointer to the Unicode string to return. This Unicode string
                               is the name of the controller specified by ControllerHandle and ChildHandle.

  @retval  EFI_SUCCESS         The controller name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval  Other               The controller name cannot be retrieved from Component Name (2) protocol.

**/
EFI_STATUS
DriverHealthManagerGetControllerNameWorker (
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ChildHandle,
  OUT CHAR16      **ControllerName
  )
{
  EFI_STATUS                     Status;
  CHAR8                          *BestLanguage;
  EFI_COMPONENT_NAME_PROTOCOL    *ComponentName;

  //
  // Retrieve Component Name (2) protocol instance on the driver binding handle and
  // find the best language this instance supports.
  //
  Status = DriverHealthManagerGetComponentNameWorker (
             ProtocolGuid,
             DriverBindingHandle,
             &ComponentName,
             &BestLanguage
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the controller name from Component Name (2) protocol instance on the driver binging handle.
  //
  Status = ComponentName->GetControllerName (
                            ComponentName,
                            ControllerHandle,
                            ChildHandle,
                            BestLanguage,
                            ControllerName
                            );
  FreePool (BestLanguage);

  return Status;
}

/**

  This function gets controller name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the controller name.
  If the attempt fails, it then gets the controller name from EFI 1.1 Component Name protocol for backward
  compatibility support.

  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ControllerHandle     The handle of a controller that the driver specified by DriverBindingHandle is managing.
                               This handle specifies the controller whose name is to be returned.
  @param  ChildHandle          The handle of the child controller to retrieve the name of. This is an
                               optional parameter that may be NULL. It will be NULL for device drivers.
                               It will also be NULL for bus drivers that attempt to retrieve the name
                               of the bus controller. It will not be NULL for a bus driver that attempts
                               to retrieve the name of a child controller.

  @return A pointer to the Unicode string to return. This Unicode string is the name of the controller
          specified by ControllerHandle and ChildHandle.
**/
CHAR16 *
DriverHealthManagerGetControllerName (
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ChildHandle
  )
{
  EFI_STATUS      Status;
  CHAR16          *ControllerName;

  //
  // Get controller name from UEFI 2.0 Component Name 2 protocol interface.
  //
  Status = DriverHealthManagerGetControllerNameWorker (
             &gEfiComponentName2ProtocolGuid,
             DriverBindingHandle,
             ControllerHandle,
             ChildHandle,
             &ControllerName
             );
  if (EFI_ERROR (Status)) {
    //
    // If it fails to get the controller name from Component Name protocol interface, we should fall back on
    // EFI 1.1 Component Name protocol interface.
    //
    Status = DriverHealthManagerGetControllerNameWorker (
               &gEfiComponentNameProtocolGuid,
               DriverBindingHandle,
               ControllerHandle,
               ChildHandle,
               &ControllerName
               );
  }

  if (!EFI_ERROR (Status)) {
    return AllocateCopyPool (StrSize (ControllerName), ControllerName);
  } else {
    return ConvertDevicePathToText (DevicePathFromHandle (ChildHandle != NULL ? ChildHandle : ControllerHandle), FALSE, TRUE);
  }
}

/**
  The repair notify function.
  @param Value  A value between 0 and Limit that identifies the current progress
                of the repair operation.
  @param Limit  The maximum value of Value for the current repair operation.
                If Limit is 0, then the completion progress is indeterminate.
                For example, a driver that wants to specify progress in percent
                would use a Limit value of 100.

  @retval EFI_SUCCESS  Successfully return from the notify function.
**/
EFI_STATUS
EFIAPI
DriverHealthManagerRepairNotify (
  IN UINTN        Value,
  IN UINTN        Limit
  )
{
  DEBUG ((EFI_D_INFO, "[DriverHealthManagement]RepairNotify: %d/%d\n", Value, Limit));
  return EFI_SUCCESS;
}

/**
  Look for the formset GUID which has the gEfiHiiDriverHealthFormsetGuid class GUID in the specified HII package list.

  @param Handle         Handle to the HII package list.
  @param FormsetGuid    Return the formset GUID.

  @retval EFI_SUCCESS   The formset is found successfully.
  @retval EFI_NOT_FOUND The formset cannot be found.
**/
EFI_STATUS
DriverHealthManagerGetFormsetId (
  IN  EFI_HII_HANDLE   Handle,
  OUT EFI_GUID         *FormsetGuid
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINTN                        BufferSize;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  UINT32                       Offset;
  UINT32                       Offset2;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  UINT8                        Index;
  UINT8                        NumberOfClassGuid;
  EFI_GUID                     *ClassGuid;

  //
  // Get HII PackageList
  //
  BufferSize     = 0;
  HiiPackageList = NULL;
  Status = mDriverHealthManagerDatabase->ExportPackageLists (mDriverHealthManagerDatabase, Handle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = AllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = mDriverHealthManagerDatabase->ExportPackageLists (mDriverHealthManagerDatabase, Handle, &BufferSize, HiiPackageList);
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ASSERT (HiiPackageList != NULL);

  //
  // Get Form package from this HII package List
  //
  for (Offset = sizeof (EFI_HII_PACKAGE_LIST_HEADER); Offset < ReadUnaligned32 (&HiiPackageList->PackageLength); Offset += PackageHeader.Length) {
    Package = ((UINT8 *) HiiPackageList) + Offset;
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));

    if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Search FormSet in this Form Package
      //
      
      for (Offset2 = sizeof (EFI_HII_PACKAGE_HEADER); Offset2 < PackageHeader.Length; Offset2 += ((EFI_IFR_OP_HEADER *) OpCodeData)->Length) {
        OpCodeData = Package + Offset2;

        if ((((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP) &&
            (((EFI_IFR_OP_HEADER *) OpCodeData)->Length > OFFSET_OF (EFI_IFR_FORM_SET, Flags))) {
          //
          // Try to compare against formset class GUID
          //
          NumberOfClassGuid = (UINT8) (((EFI_IFR_FORM_SET *) OpCodeData)->Flags & 0x3);
          ClassGuid         = (EFI_GUID *) (OpCodeData + sizeof (EFI_IFR_FORM_SET));
          for (Index = 0; Index < NumberOfClassGuid; Index++) {
            if (CompareGuid (&gEfiHiiDriverHealthFormsetGuid, &ClassGuid[Index])) {
              CopyMem (FormsetGuid, &((EFI_IFR_FORM_SET *) OpCodeData)->Guid, sizeof (EFI_GUID));
              FreePool (HiiPackageList);
              return EFI_SUCCESS;
            }
          }
        }
      }
    }
  }

  //
  // Form package not found in this Package List
  //
  FreePool (HiiPackageList);
  return EFI_NOT_FOUND;
}

/**
  Processes a single controller using the EFI Driver Health Protocol associated with
  that controller.

  @param DriverHealth       A pointer to the EFI_DRIVER_HEALTH_PROTOCOL instance.
  @param ControllerHandle   The class guid specifies which form set will be displayed.
  @param ChildHandle        The handle of the child controller to retrieve the health
                            status on.  This is an optional parameter that may be NULL.
  @param HealthStatus       The health status of the controller.
  @param MessageList        An array of warning or error messages associated
                            with the controller specified by ControllerHandle and
                            ChildHandle.  This is an optional parameter that may be NULL.
  @param FormHiiHandle      The HII handle for an HII form associated with the
                            controller specified by ControllerHandle and ChildHandle.
**/
VOID
DriverHealthManagerProcessSingleControllerHealth (
  IN  EFI_DRIVER_HEALTH_PROTOCOL         *DriverHealth,
  IN  EFI_HANDLE                         ControllerHandle, OPTIONAL
  IN  EFI_HANDLE                         ChildHandle,      OPTIONAL
  IN  EFI_DRIVER_HEALTH_STATUS           HealthStatus,
  IN  EFI_DRIVER_HEALTH_HII_MESSAGE      **MessageList,    OPTIONAL
  IN  EFI_HII_HANDLE                     FormHiiHandle
  )
{
  EFI_STATUS                         Status;

  ASSERT (HealthStatus != EfiDriverHealthStatusConfigurationRequired);
  //
  // If the module need to be repaired or reconfiguration,  will process it until
  // reach a terminal status. The status from EfiDriverHealthStatusRepairRequired after repair
  // will be in (Health, Failed, Configuration Required).
  //
  switch (HealthStatus) {

  case EfiDriverHealthStatusRepairRequired:
    Status = DriverHealth->Repair (
                             DriverHealth,
                             ControllerHandle,
                             ChildHandle,
                             DriverHealthManagerRepairNotify
                             );
    break;

  case EfiDriverHealthStatusRebootRequired:
    gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
    break;

  case EfiDriverHealthStatusReconnectRequired:
    Status = gBS->DisconnectController (ControllerHandle, NULL, NULL);
    if (EFI_ERROR (Status)) {
      //
      // Disconnect failed.  Need to promote reconnect to a reboot.
      //
      gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
    } else {
      gBS->ConnectController (ControllerHandle, NULL, NULL, TRUE);
    }
    break;

  default:
    break;
  }
}

/**
  Update the form to include the driver health instances.

  @param ConfigureOnly  Only include the configure required driver health instances
                        when TRUE, include all the driver health instances otherwise.
**/
VOID
DriverHealthManagerUpdateForm (
  BOOLEAN                     ConfigureOnly
  )
{
  EFI_STATUS                  Status;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *EndLabel;
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  UINTN                       Index;
  EFI_STRING_ID               Prompt;
  EFI_STRING_ID               Help;
  CHAR16                      String[512];
  UINTN                       StringCount;
  EFI_STRING                  TmpString;
  EFI_STRING                  DriverName;
  EFI_STRING                  ControllerName;
  UINTN                       MessageIndex;
  EFI_HANDLE                  DriverHandle;
  EFI_STRING_ID               DevicePath;
  EFI_GUID                    FormsetGuid;

  EfiBootManagerFreeDriverHealthInfo (mDriverHealthManagerHealthInfo, mDriverHealthManagerHealthInfoCount);
  mDriverHealthManagerHealthInfo = EfiBootManagerGetDriverHealthInfo (&mDriverHealthManagerHealthInfoCount);

  //
  // Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_BEGIN;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  for (Index = 0; Index < mDriverHealthManagerHealthInfoCount; Index++) {
    if (ConfigureOnly && mDriverHealthManagerHealthInfo[Index].HealthStatus != EfiDriverHealthStatusConfigurationRequired) {
      continue;
    }
    DriverName = DriverHealthManagerGetDriverName (mDriverHealthManagerHealthInfo[Index].DriverHealthHandle);
    ASSERT (DriverName != NULL);

    if (mDriverHealthManagerHealthInfo[Index].ControllerHandle == NULL) {
      //
      // The ControllerHandle is set to NULL and the HealthStatus is set to EfiDriverHealthStatusHealthy
      // if all the controllers managed by the driver are in healthy state.
      //
      ASSERT (mDriverHealthManagerHealthInfo[Index].HealthStatus == EfiDriverHealthStatusHealthy);
      UnicodeSPrint (String, sizeof (String), L"%s", DriverName);
    } else {
      ControllerName = DriverHealthManagerGetControllerName (
                         mDriverHealthManagerHealthInfo[Index].DriverHealthHandle,
                         mDriverHealthManagerHealthInfo[Index].ControllerHandle,
                         mDriverHealthManagerHealthInfo[Index].ChildHandle
                         );
      ASSERT (ControllerName != NULL);
      UnicodeSPrint (String, sizeof (String), L"%s    %s", DriverName, ControllerName);
      FreePool (ControllerName);
    }
    FreePool (DriverName);

    Prompt = HiiSetString (mDriverHealthManagerHiiHandle, 0, String, NULL);

    switch(mDriverHealthManagerHealthInfo[Index].HealthStatus) {
    case EfiDriverHealthStatusRepairRequired:
      TmpString = HiiGetString (mDriverHealthManagerHiiHandle, STRING_TOKEN (STR_REPAIR_REQUIRED), NULL);
      break;
    case EfiDriverHealthStatusConfigurationRequired:
      TmpString = HiiGetString (mDriverHealthManagerHiiHandle, STRING_TOKEN (STR_CONFIGURATION_REQUIRED), NULL);
      break;
    case EfiDriverHealthStatusFailed:
      TmpString = HiiGetString (mDriverHealthManagerHiiHandle, STRING_TOKEN (STR_FAILED), NULL);
      break;
    case EfiDriverHealthStatusReconnectRequired:
      TmpString = HiiGetString (mDriverHealthManagerHiiHandle, STRING_TOKEN (STR_RECONNECT_REQUIRED), NULL);
      break;
    case EfiDriverHealthStatusRebootRequired:
      TmpString = HiiGetString (mDriverHealthManagerHiiHandle, STRING_TOKEN (STR_REBOOT_REQUIRED), NULL);
      break;
    default:
      ASSERT (mDriverHealthManagerHealthInfo[Index].HealthStatus == EfiDriverHealthStatusHealthy);
      TmpString = HiiGetString (mDriverHealthManagerHiiHandle, STRING_TOKEN (STR_HEALTHY), NULL);
      break;
    }
    StringCount = UnicodeSPrint (String, sizeof (String), L"%s\n", TmpString);
    FreePool (TmpString);

    //
    // Add the message of the Module itself provided as the help.
    //
    if (mDriverHealthManagerHealthInfo[Index].MessageList != NULL) {
      for (MessageIndex = 0; mDriverHealthManagerHealthInfo[Index].MessageList[MessageIndex].HiiHandle != NULL; MessageIndex++) {
        TmpString = HiiGetString (
                      mDriverHealthManagerHealthInfo[Index].MessageList[MessageIndex].HiiHandle,
                      mDriverHealthManagerHealthInfo[Index].MessageList[MessageIndex].StringId,
                      NULL
                      );
        StringCount += UnicodeSPrint (String + StringCount, sizeof (String) - sizeof (String[0]) * StringCount, L"\n%s", TmpString);
        FreePool (TmpString);
      }
    }
    Help = HiiSetString (mDriverHealthManagerHiiHandle, 0, String, NULL);

    switch (mDriverHealthManagerHealthInfo[Index].HealthStatus) {
    case EfiDriverHealthStatusConfigurationRequired:
      Status = mDriverHealthManagerDatabase->GetPackageListHandle (
                                               mDriverHealthManagerDatabase,
                                               mDriverHealthManagerHealthInfo[Index].HiiHandle,
                                               &DriverHandle
                                               );
      ASSERT_EFI_ERROR (Status);
      TmpString  = ConvertDevicePathToText (DevicePathFromHandle (DriverHandle), FALSE, TRUE);
      DevicePath = HiiSetString (mDriverHealthManagerHiiHandle, 0, TmpString, NULL);
      FreePool (TmpString);

      Status = DriverHealthManagerGetFormsetId (mDriverHealthManagerHealthInfo[Index].HiiHandle, &FormsetGuid);
      ASSERT_EFI_ERROR (Status);

      HiiCreateGotoExOpCode (
        StartOpCodeHandle,
        0,
        Prompt,
        Help,
        0,
        0,
        0,
        &FormsetGuid,
        DevicePath
        );
      break;

    case EfiDriverHealthStatusRepairRequired:
    case EfiDriverHealthStatusReconnectRequired:
    case EfiDriverHealthStatusRebootRequired:
      HiiCreateActionOpCode (
        StartOpCodeHandle,
        (EFI_QUESTION_ID) (Index + QUESTION_ID_DRIVER_HEALTH_BASE),
        Prompt,
        Help,
        EFI_IFR_FLAG_CALLBACK,
        0
        );
      break;

    default:
      ASSERT (mDriverHealthManagerHealthInfo[Index].HealthStatus == EfiDriverHealthStatusHealthy ||
              mDriverHealthManagerHealthInfo[Index].HealthStatus == EfiDriverHealthStatusFailed);
      HiiCreateTextOpCode (
        StartOpCodeHandle,
        Prompt,
        Help,
        0
        );
      break;
    }
  }

  Status = HiiUpdateForm (
             mDriverHealthManagerHiiHandle,
             ConfigureOnly ? PcdGetPtr (PcdDriverHealthConfigureForm) : &mDriverHealthManagerForm,
             DRIVER_HEALTH_FORM_ID,
             StartOpCodeHandle,
             EndOpCodeHandle
             );
  ASSERT_EFI_ERROR (Status);

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}

/**
  Called when the form is closing to remove the dynamicly added string from the HII package list.
**/
VOID
DriverHealthManagerCleanDynamicString (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINTN                        BufferSize;
  EFI_HII_PACKAGE_HEADER       *PackageHeader;
  UINT32                       FixedStringSize;

  FixedStringSize = *(UINT32 *) &STRING_ARRAY_NAME - sizeof (UINT32);
  BufferSize      = sizeof (EFI_HII_PACKAGE_LIST_HEADER) + FixedStringSize + sizeof (EFI_HII_PACKAGE_HEADER);
  HiiPackageList  = AllocatePool (BufferSize);
  ASSERT (HiiPackageList != NULL);

  HiiPackageList->PackageLength = (UINT32) BufferSize;
  CopyMem (&HiiPackageList->PackageListGuid, &gEfiCallerIdGuid, sizeof (EFI_GUID));

  PackageHeader = (EFI_HII_PACKAGE_HEADER *) (HiiPackageList + 1);
  CopyMem (PackageHeader, STRING_ARRAY_NAME + sizeof (UINT32), FixedStringSize);

  PackageHeader = (EFI_HII_PACKAGE_HEADER *) ((UINT8 *) PackageHeader + PackageHeader->Length);
  PackageHeader->Type   = EFI_HII_PACKAGE_END;
  PackageHeader->Length = sizeof (EFI_HII_PACKAGE_HEADER);

  Status = mDriverHealthManagerDatabase->UpdatePackageList (
                                           mDriverHealthManagerDatabase,
                                           mDriverHealthManagerHiiHandle,
                                           HiiPackageList
                                           );
  ASSERT_EFI_ERROR (Status);

  //
  // Form package not found in this Package List
  //
  FreePool (HiiPackageList);
}

/**
  This function is invoked if user selected a interactive opcode from Driver Health's
  Formset.

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
DriverHealthManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  UINTN                                      Index;

  if (QuestionId == QUESTION_ID_REFRESH_MANAGER || QuestionId == QUESTION_ID_REFRESH_CONFIGURE) {
    if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
      DriverHealthManagerUpdateForm ((BOOLEAN) (QuestionId == QUESTION_ID_REFRESH_CONFIGURE));
    } else if (Action == EFI_BROWSER_ACTION_FORM_CLOSE) {
      DriverHealthManagerCleanDynamicString ();
    }
    return EFI_SUCCESS;
  }

  if (Action != EFI_BROWSER_ACTION_CHANGED) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed.
    //
    return EFI_UNSUPPORTED;
  }

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_ERROR, "QuestionId = %x\n", QuestionId));

  //
  // We will have returned from processing a callback - user either hit ESC to exit, or selected
  // a target to display.
  // Process the diver health status states here.
  //
  Index = QuestionId - QUESTION_ID_DRIVER_HEALTH_BASE;
  ASSERT (Index < mDriverHealthManagerHealthInfoCount);
  //
  // Process the driver's healthy status for the specify module
  //
  DriverHealthManagerProcessSingleControllerHealth (
    mDriverHealthManagerHealthInfo[Index].DriverHealth,
    mDriverHealthManagerHealthInfo[Index].ControllerHandle,
    mDriverHealthManagerHealthInfo[Index].ChildHandle,
    mDriverHealthManagerHealthInfo[Index].HealthStatus,
    &(mDriverHealthManagerHealthInfo[Index].MessageList),
    mDriverHealthManagerHealthInfo[Index].HiiHandle
    );

  DriverHealthManagerUpdateForm ((BOOLEAN) (QuestionId == QUESTION_ID_REFRESH_CONFIGURE));

  return EFI_SUCCESS;
}


