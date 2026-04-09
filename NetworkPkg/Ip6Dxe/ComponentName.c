/** @file
  Implementation of EFI_COMPONENT_NAME_PROTOCOL and
  EFI_COMPONENT_NAME2_PROTOCOL protocol.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip6Impl.h"

//
// EFI Component Name Functions
//

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
Ip6ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user-readable name of the controller
  that is managed by a driver.

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
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param[out]  ControllerName   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
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
Ip6ComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

//
// EFI Component Name Protocol.
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  gIp6ComponentName = {
  Ip6ComponentNameGetDriverName,
  Ip6ComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol.
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL  gIp6ComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)Ip6ComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)Ip6ComponentNameGetControllerName,
  "en"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  mIp6DriverNameTable[] = {
  {
    "eng;en",
    L"IP6 Network Service Driver"
  },
  {
    NULL,
    NULL
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  *gIp6ControllerNameTable = NULL;

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
Ip6ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mIp6DriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gIp6ComponentName)
           );
}

/**
  Update the component name for the IP6 child handle.

  @param  Ip6[in]                   A pointer to the EFI_IP6_PROTOCOL.


  @retval EFI_SUCCESS               Update the ControllerNameTable of this instance successfully.
  @retval EFI_INVALID_PARAMETER     The input parameter is invalid.

**/
EFI_STATUS
UpdateName (
  IN    EFI_IP6_PROTOCOL  *Ip6
  )
{
  EFI_STATUS         Status;
  CHAR16             HandleName[128];
  EFI_IP6_MODE_DATA  Ip6ModeData;
  UINTN              Offset;
  CHAR16             Address[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"];

  if (Ip6 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Format the child name into the string buffer.
  //
  Offset = 0;
  Status = Ip6->GetModeData (Ip6, &Ip6ModeData, NULL, NULL);
  if (!EFI_ERROR (Status)) {
    if (Ip6ModeData.AddressList != NULL) {
      FreePool (Ip6ModeData.AddressList);
    }

    if (Ip6ModeData.GroupTable != NULL) {
      FreePool (Ip6ModeData.GroupTable);
    }

    if (Ip6ModeData.RouteTable != NULL) {
      FreePool (Ip6ModeData.RouteTable);
    }

    if (Ip6ModeData.NeighborCache != NULL) {
      FreePool (Ip6ModeData.NeighborCache);
    }

    if (Ip6ModeData.PrefixTable != NULL) {
      FreePool (Ip6ModeData.PrefixTable);
    }

    if (Ip6ModeData.IcmpTypeList != NULL) {
      FreePool (Ip6ModeData.IcmpTypeList);
    }
  }

  if (!EFI_ERROR (Status) && Ip6ModeData.IsStarted) {
    Status = NetLibIp6ToStr (&Ip6ModeData.ConfigData.StationAddress, Address, sizeof (Address));
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Offset += UnicodeSPrint (
                HandleName,
                sizeof (HandleName),
                L"IPv6(StationAddress=%s, ",
                Address
                );
    Status = NetLibIp6ToStr (&Ip6ModeData.ConfigData.DestinationAddress, Address, sizeof (Address));
    if (EFI_ERROR (Status)) {
      return Status;
    }

    UnicodeSPrint (
      HandleName + Offset,
      sizeof (HandleName) - Offset * sizeof (CHAR16),
      L"DestinationAddress=%s)",
      Address
      );
  } else if (!Ip6ModeData.IsStarted) {
    UnicodeSPrint (HandleName, sizeof (HandleName), L"IPv6(Not started)");
  } else {
    UnicodeSPrint (HandleName, sizeof (HandleName), L"IPv6(%r)", Status);
  }

  if (gIp6ControllerNameTable != NULL) {
    FreeUnicodeStringTable (gIp6ControllerNameTable);
    gIp6ControllerNameTable = NULL;
  }

  Status = AddUnicodeString2 (
             "eng",
             gIp6ComponentName.SupportedLanguages,
             &gIp6ControllerNameTable,
             HandleName,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AddUnicodeString2 (
           "en",
           gIp6ComponentName2.SupportedLanguages,
           &gIp6ControllerNameTable,
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
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param[out]  ControllerName   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
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
Ip6ComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  )
{
  EFI_STATUS        Status;
  EFI_IP6_PROTOCOL  *Ip6;

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
             &gEfiManagedNetworkProtocolGuid
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Retrieve an instance of a produced protocol from ChildHandle
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiIp6ProtocolGuid,
                  (VOID **)&Ip6,
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
  Status = UpdateName (Ip6);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           gIp6ControllerNameTable,
           ControllerName,
           (BOOLEAN)(This == &gIp6ComponentName)
           );
}
