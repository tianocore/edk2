/** @file
  UEFI Component Name(2) protocol implementation for Mtftp4Dxe driver.
  
Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Mtftp4Impl.h"

//
// EFI Component Name Functions
//
/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
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
Mtftp4ComponentNameGetDriverName (
  IN     EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN     CHAR8                        *Language,
     OUT CHAR16                       **DriverName
  );


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

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

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
Mtftp4ComponentNameGetControllerName (
  IN     EFI_COMPONENT_NAME_PROTOCOL               *This,
  IN     EFI_HANDLE                                ControllerHandle,
  IN     EFI_HANDLE                                ChildHandle        OPTIONAL,
  IN     CHAR8                                     *Language,
     OUT CHAR16                                    **ControllerName
  );


///
/// EFI Component Name Protocol
///
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  gMtftp4ComponentName = {
  Mtftp4ComponentNameGetDriverName,
  Mtftp4ComponentNameGetControllerName,
  "eng"
};

///
/// EFI Component Name 2 Protocol
///
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL gMtftp4ComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME) Mtftp4ComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) Mtftp4ComponentNameGetControllerName,
  "en"
};


GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE mMtftp4DriverNameTable[] = {
  {
    "eng;en",
    L"MTFTP4 Network Service"
  },
  {
    NULL,
    NULL
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE *gMtftp4ControllerNameTable = NULL;

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
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
Mtftp4ComponentNameGetDriverName (
  IN     EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN     CHAR8                        *Language,
     OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mMtftp4DriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gMtftp4ComponentName)
           );
}

/**
  Update the component name for the Mtftp4 child handle.

  @param  Mtftp4[in]                A pointer to the EFI_MTFTP4_PROTOCOL.

  
  @retval EFI_SUCCESS               Update the ControllerNameTable of this instance successfully.
  @retval EFI_INVALID_PARAMETER     The input parameter is invalid.
  
**/
EFI_STATUS
UpdateName (
  IN   EFI_MTFTP4_PROTOCOL             *Mtftp4
  )
{
  EFI_STATUS                       Status;
  CHAR16                           HandleName[80];
  EFI_MTFTP4_MODE_DATA             ModeData;

  if (Mtftp4 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Format the child name into the string buffer as:
  // MTFTPv4 (ServerIp=192.168.1.10, ServerPort=69)
  //
  Status = Mtftp4->GetModeData (Mtftp4, &ModeData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UnicodeSPrint (HandleName, sizeof (HandleName),
    L"MTFTPv4 (ServerIp=%d.%d.%d.%d, ServerPort=%d)",
    ModeData.ConfigData.ServerIp.Addr[0],
    ModeData.ConfigData.ServerIp.Addr[1],
    ModeData.ConfigData.ServerIp.Addr[2],
    ModeData.ConfigData.ServerIp.Addr[3],
    ModeData.ConfigData.InitialServerPort
    );

  if (gMtftp4ControllerNameTable != NULL) {
    FreeUnicodeStringTable (gMtftp4ControllerNameTable);
    gMtftp4ControllerNameTable = NULL;
  }
  
  Status = AddUnicodeString2 (
             "eng",
             gMtftp4ComponentName.SupportedLanguages,
             &gMtftp4ControllerNameTable,
             HandleName,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  return AddUnicodeString2 (
           "en",
           gMtftp4ComponentName2.SupportedLanguages,
           &gMtftp4ControllerNameTable,
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

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

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
Mtftp4ComponentNameGetControllerName (
  IN     EFI_COMPONENT_NAME_PROTOCOL               *This,
  IN     EFI_HANDLE                                ControllerHandle,
  IN     EFI_HANDLE                                ChildHandle        OPTIONAL,
  IN     CHAR8                                     *Language,
     OUT CHAR16                                    **ControllerName
  )
{
  EFI_STATUS                    Status;
  EFI_MTFTP4_PROTOCOL           *Mtftp4;

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
             &gEfiUdp4ProtocolGuid
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // 
  // Retrieve an instance of a produced protocol from ChildHandle
  // 
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiMtftp4ProtocolGuid,
                  (VOID **)&Mtftp4,
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
  Status = UpdateName (Mtftp4);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           gMtftp4ControllerNameTable,
           ControllerName,
           (BOOLEAN)(This == &gMtftp4ComponentName)
           );
}
