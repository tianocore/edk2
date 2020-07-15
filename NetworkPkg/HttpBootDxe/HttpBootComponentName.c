/** @file
  Implementation of EFI_COMPONENT_NAME_PROTOCOL and EFI_COMPONENT_NAME2_PROTOCOL protocol.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpBootDxe.h"

///
/// Component Name Protocol instance
///
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_COMPONENT_NAME_PROTOCOL  gHttpBootDxeComponentName = {
  (EFI_COMPONENT_NAME_GET_DRIVER_NAME)    HttpBootDxeComponentNameGetDriverName,
  (EFI_COMPONENT_NAME_GET_CONTROLLER_NAME)HttpBootDxeComponentNameGetControllerName,
  "eng"
};

///
/// Component Name 2 Protocol instance
///
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_COMPONENT_NAME2_PROTOCOL  gHttpBootDxeComponentName2 = {
  HttpBootDxeComponentNameGetDriverName,
  HttpBootDxeComponentNameGetControllerName,
  "en"
};

///
/// Table of driver names
///
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_UNICODE_STRING_TABLE mHttpBootDxeDriverNameTable[] = {
  { "eng;en", (CHAR16 *)L"UEFI HTTP Boot Driver" },
  { NULL, NULL }
};

///
/// Table of controller names
///
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_UNICODE_STRING_TABLE mHttpBootDxeControllerNameTable[] = {
  { "eng;en", (CHAR16 *)L"UEFI Http Boot Controller" },
  { NULL, NULL }
};

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
HttpBootDxeComponentNameGetDriverName (
  IN EFI_COMPONENT_NAME2_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mHttpBootDxeDriverNameTable,
           DriverName,
           (BOOLEAN) (This != &gHttpBootDxeComponentName2)
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
HttpBootDxeComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL  *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    ChildHandle        OPTIONAL,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **ControllerName
  )
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      NicHandle;
  UINT32                          *Id;

  if (ControllerHandle == NULL || ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }

  NicHandle = HttpBootGetNicByIp4Children (ControllerHandle);
  if (NicHandle == NULL) {
    NicHandle = HttpBootGetNicByIp6Children(ControllerHandle);
    if (NicHandle == NULL) {
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Try to retrieve the private data by caller ID GUID.
  //
  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &Id,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mHttpBootDxeControllerNameTable,
           ControllerName,
           (BOOLEAN)(This != &gHttpBootDxeComponentName2)
           );

}
