/** @file
  UEFI Component Name(2) protocol implementation for Dhcp6 driver.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Dhcp6Impl.h"

/**
  Retrieves a Unicode string that is the user-readable name of the driver.

  This function retrieves the user-readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user-readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]  Language          A pointer to a Null-terminated ASCII string
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
Dhcp6ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user-readable name of the controller
  that is being managed by a driver.

  This function retrieves the user-readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user-readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]  ControllerHandle  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param[in]  ChildHandle       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that attempt to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that attempts to retrieve the name of a
                                child controller.

  @param[in]  Language          A pointer to a Null-terminated ASCII string
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
                                Language, from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user-readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL, and it is not a valid
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
Dhcp6ComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

//
// EFI Component Name Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  gDhcp6ComponentName = {
  Dhcp6ComponentNameGetDriverName,
  Dhcp6ComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL  gDhcp6ComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)Dhcp6ComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)Dhcp6ComponentNameGetControllerName,
  "en"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  mDhcp6DriverNameTable[] = {
  {
    "eng;en",
    L"DHCP6 Protocol Driver"
  },
  {
    NULL,
    NULL
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  *gDhcp6ControllerNameTable = NULL;

CHAR16  *mDhcp6ControllerName[] = {
  L"DHCPv6 (State=0, Init)",
  L"DHCPv6 (State=1, Selecting)",
  L"DHCPv6 (State=2, Requesting)",
  L"DHCPv6 (State=3, Declining)",
  L"DHCPv6 (State=4, Confirming)",
  L"DHCPv6 (State=5, Releasing)",
  L"DHCPv6 (State=6, Bound)",
  L"DHCPv6 (State=7, Renewing)",
  L"DHCPv6 (State=8, Rebinding)"
};

/**
  Retrieves a Unicode string that is the user-readable name of the driver.

  This function retrieves the user-readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user-readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]  Language          A pointer to a Null-terminated ASCII string
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
Dhcp6ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDhcp6DriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gDhcp6ComponentName)
           );
}

/**
  Update the component name for the Dhcp6 child handle.

  @param  Dhcp6[in]                   A pointer to the EFI_DHCP6_PROTOCOL.


  @retval EFI_SUCCESS                 Update the ControllerNameTable of this instance successfully.
  @retval EFI_INVALID_PARAMETER       The input parameter is invalid.

**/
EFI_STATUS
UpdateName (
  IN   EFI_DHCP6_PROTOCOL  *Dhcp6
  )
{
  EFI_STATUS           Status;
  EFI_DHCP6_MODE_DATA  Dhcp6ModeData;
  CHAR16               *HandleName;

  if (Dhcp6 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Format the child name into the string buffer.
  //
  Status = Dhcp6->GetModeData (Dhcp6, &Dhcp6ModeData, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (gDhcp6ControllerNameTable != NULL) {
    FreeUnicodeStringTable (gDhcp6ControllerNameTable);
    gDhcp6ControllerNameTable = NULL;
  }

  if (Dhcp6ModeData.Ia == NULL) {
    HandleName = L"DHCPv6 (No configured IA)";
  } else {
    if (Dhcp6ModeData.Ia->State > Dhcp6Rebinding) {
      return EFI_DEVICE_ERROR;
    }

    HandleName = mDhcp6ControllerName[Dhcp6ModeData.Ia->State];
  }

  if (Dhcp6ModeData.Ia != NULL) {
    FreePool (Dhcp6ModeData.Ia);
  }

  if (Dhcp6ModeData.ClientId != NULL) {
    FreePool (Dhcp6ModeData.ClientId);
  }

  Status = AddUnicodeString2 (
             "eng",
             gDhcp6ComponentName.SupportedLanguages,
             &gDhcp6ControllerNameTable,
             HandleName,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AddUnicodeString2 (
           "en",
           gDhcp6ComponentName2.SupportedLanguages,
           &gDhcp6ControllerNameTable,
           HandleName,
           FALSE
           );
}

/**
  Retrieves a Unicode string that is the user-readable name of the controller
  that is being managed by a driver.

  This function retrieves the user-readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user-readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]  ControllerHandle  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param[in]  ChildHandle       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param[in]  Language          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in the
                                RFC 4646 or ISO 639-2 language code format.

  @param[out]  ControllerName   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language, from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user-readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL, and it is not a valid
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
Dhcp6ComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  )
{
  EFI_STATUS          Status;
  EFI_DHCP6_PROTOCOL  *Dhcp6;

  //
  // Only provide names for child handles.
  //
  if (ChildHandle == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Make sure this driver produced ChildHandle
  //
  Status = EfiTestChildHandle (
             ControllerHandle,
             ChildHandle,
             &gEfiUdp6ProtocolGuid
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Retrieve an instance of a produced protocol from ChildHandle
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiDhcp6ProtocolGuid,
                  (VOID **)&Dhcp6,
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
  Status = UpdateName (Dhcp6);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           gDhcp6ControllerNameTable,
           ControllerName,
           (BOOLEAN)(This == &gDhcp6ComponentName)
           );
}
