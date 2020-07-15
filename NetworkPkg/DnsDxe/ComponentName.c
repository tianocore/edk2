/** @file
Implementation of EFI_COMPONENT_NAME_PROTOCOL and EFI_COMPONENT_NAME2_PROTOCOL protocol.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DnsImpl.h"

//
// EFI Component Name Functions
//
/**
  Retrieves a Unicode string that is the user-readable name of the EFI Driver.

  @param  This       A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language   A pointer to a three-character ISO 639-2 language identifier.
                     This is the language of the driver name that that the caller
                     is requesting, and it must match one of the languages specified
                     in SupportedLanguages.  The number of languages supported by a
                     driver is up to the driver writer.
  @param  DriverName A pointer to the Unicode string to return.  This Unicode string
                     is the name of the driver specified by This in the language
                     specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by This
                                and the language specified by Language was returned
                                in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.

**/
EFI_STATUS
EFIAPI
DnsComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by an EFI Driver.

  @param  This             A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  ControllerHandle The handle of a controller that the driver specified by
                           This is managing.  This handle specifies the controller
                           whose name is to be returned.
  @param  ChildHandle      The handle of the child controller to retrieve the name
                           of.  This is an optional parameter that may be NULL.  It
                           will be NULL for device drivers.  It will also be NULL
                           for a bus drivers that wish to retrieve the name of the
                           bus controller.  It will not be NULL for a bus driver
                           that wishes to retrieve the name of a child controller.
  @param  Language         A pointer to a three character ISO 639-2 language
                           identifier.  This is the language of the controller name
                           that the caller is requesting, and it must match one
                           of the languages specified in SupportedLanguages.  The
                           number of languages supported by a driver is up to the
                           driver writer.
  @param  ControllerName   A pointer to the Unicode string to return.  This Unicode
                           string is the name of the controller specified by
                           ControllerHandle and ChildHandle in the language specified
                           by Language, from the point of view of the driver specified
                           by This.

  @retval EFI_SUCCESS           The Unicode string for the user-readable name in the
                                language specified by Language for the driver
                                specified by This was returned in DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently managing
                                the controller specified by ControllerHandle and
                                ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.

**/
EFI_STATUS
EFIAPI
DnsComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    ChildHandle        OPTIONAL,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **ControllerName
  );


///
/// Component Name Protocol instance
///
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_COMPONENT_NAME_PROTOCOL  gDnsComponentName = {
  DnsComponentNameGetDriverName,
  DnsComponentNameGetControllerName,
  "eng"
};

///
/// Component Name 2 Protocol instance
///
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_COMPONENT_NAME2_PROTOCOL  gDnsComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     DnsComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) DnsComponentNameGetControllerName,
  "en"
};

///
/// Table of driver names
///
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_UNICODE_STRING_TABLE mDnsDriverNameTable[] = {
  { "eng;en", (CHAR16 *)L"DNS Network Service Driver" },
  { NULL, NULL }
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE *gDnsControllerNameTable = NULL;

/**
  Retrieves a Unicode string that is the user-readable name of the EFI Driver.

  @param  This       A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language   A pointer to a three-character ISO 639-2 language identifier.
                     This is the language of the driver name that that the caller
                     is requesting, and it must match one of the languages specified
                     in SupportedLanguages.  The number of languages supported by a
                     driver is up to the driver writer.
  @param  DriverName A pointer to the Unicode string to return.  This Unicode string
                     is the name of the driver specified by This in the language
                     specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by This
                                and the language specified by Language was returned
                                in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.

**/
EFI_STATUS
EFIAPI
DnsComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDnsDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gDnsComponentName)
           );
}

/**
  Update the component name for the Dns4 child handle.

  @param  Dns4                       A pointer to the EFI_DNS4_PROTOCOL.


  @retval EFI_SUCCESS                Update the ControllerNameTable of this instance successfully.
  @retval EFI_INVALID_PARAMETER      The input parameter is invalid.

**/
EFI_STATUS
UpdateDns4Name (
  EFI_DNS4_PROTOCOL             *Dns4
  )
{
  EFI_STATUS                       Status;
  CHAR16                           HandleName[80];
  EFI_DNS4_MODE_DATA               ModeData;

  if (Dns4 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Format the child name into the string buffer as:
  // DNSv4 (StationIp=?, LocalPort=?)
  //
  Status = Dns4->GetModeData (Dns4, &ModeData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UnicodeSPrint (
    HandleName,
    sizeof (HandleName),
    L"DNSv4 (StationIp=%d.%d.%d.%d, LocalPort=%d)",
    ModeData.DnsConfigData.StationIp.Addr[0],
    ModeData.DnsConfigData.StationIp.Addr[1],
    ModeData.DnsConfigData.StationIp.Addr[2],
    ModeData.DnsConfigData.StationIp.Addr[3],
    ModeData.DnsConfigData.LocalPort
    );

  if (ModeData.DnsCacheList != NULL) {
    FreePool (ModeData.DnsCacheList);
  }
  if (ModeData.DnsServerList != NULL) {
    FreePool (ModeData.DnsServerList);
  }

  if (gDnsControllerNameTable != NULL) {
    FreeUnicodeStringTable (gDnsControllerNameTable);
    gDnsControllerNameTable = NULL;
  }

  Status = AddUnicodeString2 (
             "eng",
             gDnsComponentName.SupportedLanguages,
             &gDnsControllerNameTable,
             HandleName,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AddUnicodeString2 (
           "en",
           gDnsComponentName2.SupportedLanguages,
           &gDnsControllerNameTable,
           HandleName,
           FALSE
           );
}

/**
  Update the component name for the Dns6 child handle.

  @param  Dns6                       A pointer to the EFI_DNS6_PROTOCOL.


  @retval EFI_SUCCESS                Update the ControllerNameTable of this instance successfully.
  @retval EFI_INVALID_PARAMETER      The input parameter is invalid.

**/
EFI_STATUS
UpdateDns6Name (
  EFI_DNS6_PROTOCOL             *Dns6
  )
{
  EFI_STATUS                       Status;
  CHAR16                           HandleName[128];
  EFI_DNS6_MODE_DATA               ModeData;
  CHAR16                           Address[sizeof"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"];

  if (Dns6 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Format the child name into the string buffer as:
  // DNSv6 (StationIp=?, LocalPort=?)
  //
  Status = Dns6->GetModeData (Dns6, &ModeData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = NetLibIp6ToStr (&ModeData.DnsConfigData.StationIp, Address, sizeof (Address));
  if (EFI_ERROR (Status)) {
    return Status;
  }
  UnicodeSPrint (
    HandleName,
    sizeof (HandleName),
    L"DNSv6 (StationIp=%s, LocalPort=%d)",
    Address,
    ModeData.DnsConfigData.LocalPort
    );

  if (ModeData.DnsCacheList != NULL) {
    FreePool (ModeData.DnsCacheList);
  }
  if (ModeData.DnsServerList != NULL) {
    FreePool (ModeData.DnsServerList);
  }

  if (gDnsControllerNameTable != NULL) {
    FreeUnicodeStringTable (gDnsControllerNameTable);
    gDnsControllerNameTable = NULL;
  }

  Status = AddUnicodeString2 (
             "eng",
             gDnsComponentName.SupportedLanguages,
             &gDnsControllerNameTable,
             HandleName,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AddUnicodeString2 (
           "en",
           gDnsComponentName2.SupportedLanguages,
           &gDnsControllerNameTable,
           HandleName,
           FALSE
           );
}

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by an EFI Driver.

  @param  This             A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  ControllerHandle The handle of a controller that the driver specified by
                           This is managing.  This handle specifies the controller
                           whose name is to be returned.
  @param  ChildHandle      The handle of the child controller to retrieve the name
                           of.  This is an optional parameter that may be NULL.  It
                           will be NULL for device drivers.  It will also be NULL
                           for a bus drivers that wish to retrieve the name of the
                           bus controller.  It will not be NULL for a bus driver
                           that wishes to retrieve the name of a child controller.
  @param  Language         A pointer to a three character ISO 639-2 language
                           identifier.  This is the language of the controller name
                           that the caller is requesting, and it must match one
                           of the languages specified in SupportedLanguages.  The
                           number of languages supported by a driver is up to the
                           driver writer.
  @param  ControllerName   A pointer to the Unicode string to return.  This Unicode
                           string is the name of the controller specified by
                           ControllerHandle and ChildHandle in the language specified
                           by Language, from the point of view of the driver specified
                           by This.

  @retval EFI_SUCCESS           The Unicode string for the user-readable name in the
                                language specified by Language for the driver
                                specified by This was returned in DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently managing
                                the controller specified by ControllerHandle and
                                ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.

**/
EFI_STATUS
EFIAPI
DnsComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    ChildHandle        OPTIONAL,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **ControllerName
  )
{
  EFI_STATUS                    Status;
  EFI_DNS4_PROTOCOL             *Dns4;
  EFI_DNS6_PROTOCOL             *Dns6;

  //
  // ChildHandle must be NULL for a Device Driver
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
  if (!EFI_ERROR (Status)) {
    //
    // Retrieve an instance of a produced protocol from ChildHandle
    //
    Status = gBS->OpenProtocol (
                    ChildHandle,
                    &gEfiDns6ProtocolGuid,
                    (VOID **)&Dns6,
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
    Status = UpdateDns6Name (Dns6);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Make sure this driver produced ChildHandle
  //
  Status = EfiTestChildHandle (
             ControllerHandle,
             ChildHandle,
             &gEfiUdp4ProtocolGuid
             );
  if (!EFI_ERROR (Status)) {
    //
    // Retrieve an instance of a produced protocol from ChildHandle
    //
    Status = gBS->OpenProtocol (
                    ChildHandle,
                    &gEfiDns4ProtocolGuid,
                    (VOID **)&Dns4,
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
    Status = UpdateDns4Name (Dns4);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           gDnsControllerNameTable,
           ControllerName,
           (BOOLEAN)(This == &gDnsComponentName)
           );
}
