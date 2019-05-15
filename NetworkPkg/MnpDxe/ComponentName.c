/** @file
  UEFI Component Name(2) protocol implementation for MnpDxe driver.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include  "MnpImpl.h"

//
// EFI Component Name Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL   gMnpComponentName = {
  MnpComponentNameGetDriverName,
  MnpComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL  gMnpComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME) MnpComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) MnpComponentNameGetControllerName,
  "en"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE      mMnpDriverNameTable[] = {
  {
    "eng;en",
    L"MNP Network Service Driver"
  },
  {
    NULL,
    NULL
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE    *gMnpControllerNameTable = NULL;

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param[in]   This             A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]   Language         A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param[out]  DriverName       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
MnpComponentNameGetDriverName (
  IN     EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN     CHAR8                         *Language,
     OUT CHAR16                        **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mMnpDriverNameTable,
           DriverName,
           (BOOLEAN) (This == &gMnpComponentName)
           );
}

/**
  Update the component name for the MNP child handle.

  @param  Mnp[in]                 A pointer to the EFI_MANAGED_NETWORK_PROTOCOL.


  @retval EFI_SUCCESS             Update the ControllerNameTable of this instance successfully.
  @retval EFI_INVALID_PARAMETER   The input parameter is invalid.

**/
EFI_STATUS
UpdateName (
  IN   EFI_MANAGED_NETWORK_PROTOCOL     *Mnp
  )
{
  EFI_STATUS                       Status;
  MNP_INSTANCE_DATA                *Instance;
  CHAR16                           HandleName[80];
  EFI_MANAGED_NETWORK_CONFIG_DATA  MnpConfigData;
  EFI_SIMPLE_NETWORK_MODE          SnpModeData;
  UINTN                            OffSet;
  UINTN                            Index;

  if (Mnp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = MNP_INSTANCE_DATA_FROM_THIS (Mnp);
  //
  // Format the child name into the string buffer as:
  // MNP (MAC=FF-FF-FF-FF-FF-FF, ProtocolType=0x0800, VlanId=0)
  //
  Status = Mnp->GetModeData (Mnp, &MnpConfigData, &SnpModeData);
  if (!EFI_ERROR (Status)) {
    OffSet = 0;
    //
    // Print the MAC address.
    //
    OffSet += UnicodeSPrint (
                HandleName,
                sizeof (HandleName),
                L"MNP (MAC="
                );
    for (Index = 0; Index < SnpModeData.HwAddressSize; Index++) {
      OffSet += UnicodeSPrint (
                  HandleName + OffSet,
                  sizeof (HandleName) - OffSet * sizeof (CHAR16),
                  L"%02X-",
                  SnpModeData.CurrentAddress.Addr[Index]
                  );
    }
    ASSERT (OffSet > 0);
    //
    // Remove the last '-'
    //
    OffSet--;
    //
    // Print the ProtocolType and VLAN ID for this instance.
    //
    OffSet += UnicodeSPrint (
                HandleName + OffSet,
                sizeof (HandleName) - OffSet * sizeof (CHAR16),
                L", ProtocolType=0x%X, VlanId=%d)",
                MnpConfigData.ProtocolTypeFilter,
                Instance->MnpServiceData->VlanId
                );
  } else if (Status == EFI_NOT_STARTED) {
    UnicodeSPrint (
      HandleName,
      sizeof (HandleName),
      L"MNP (Not started)"
      );
  } else {
    return Status;
  }

  if (gMnpControllerNameTable != NULL) {
    FreeUnicodeStringTable (gMnpControllerNameTable);
    gMnpControllerNameTable = NULL;
  }

  Status = AddUnicodeString2 (
             "eng",
             gMnpComponentName.SupportedLanguages,
             &gMnpControllerNameTable,
             HandleName,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AddUnicodeString2 (
           "en",
           gMnpComponentName2.SupportedLanguages,
           &gMnpControllerNameTable,
           HandleName,
           FALSE
           );
}

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.
  Currently not implemented.

  @param[in]   This             A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]   ControllerHandle The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param[in]   ChildHandle      The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param[in]   Language         A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param[out]  ControllerName   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name
                                specified by This, ControllerHandle, ChildHandle,
                                and Language was returned in ControllerName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
MnpComponentNameGetControllerName (
  IN     EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN     EFI_HANDLE                    ControllerHandle,
  IN     EFI_HANDLE                    ChildHandle        OPTIONAL,
  IN     CHAR8                         *Language,
     OUT CHAR16                        **ControllerName
  )
{
  EFI_STATUS                    Status;
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;

  //
  // Only provide names for MNP child handles.
  //
  if (ChildHandle == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Make sure this driver is currently managing ControllerHandle
  //
  Status = EfiTestManagedDevice (
             ControllerHandle,
             gMnpDriverBinding.DriverBindingHandle,
             &gEfiSimpleNetworkProtocolGuid
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure this driver produced ChildHandle
  //
  Status = EfiTestChildHandle (
             ControllerHandle,
             ChildHandle,
             &gEfiManagedNetworkServiceBindingProtocolGuid
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Retrieve an instance of a produced protocol from ChildHandle
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **)&Mnp,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Update the component name for this child handle.
  //
  Status = UpdateName (Mnp);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           gMnpControllerNameTable,
           ControllerName,
           (BOOLEAN)(This == &gMnpComponentName)
           );
}
