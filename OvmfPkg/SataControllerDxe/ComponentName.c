/** @file
  UEFI Component Name(2) protocol implementation for Sata Controller driver.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SataController.h"

//
/// EFI Component Name Protocol
///
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  gSataControllerComponentName = {
  SataControllerComponentNameGetDriverName,
  SataControllerComponentNameGetControllerName,
  "eng"
};

//
/// EFI Component Name 2 Protocol
///
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL  gSataControllerComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)SataControllerComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)SataControllerComponentNameGetControllerName,
  "en"
};

//
/// Driver Name Strings
///
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  mSataControllerDriverNameTable[] = {
  {
    "eng;en",
    (CHAR16 *)L"Sata Controller Init Driver"
  },
  {
    NULL,
    NULL
  }
};

///
/// Controller Name Strings
///
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  mSataControllerControllerNameTable[] = {
  {
    "eng;en",
    (CHAR16 *)L"Sata Controller"
  },
  {
    NULL,
    NULL
  }
};

/**
  Retrieves a Unicode string that is the user readable name of the UEFI Driver.

  @param This           A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param Language       A pointer to a three character ISO 639-2 language identifier.
                        This is the language of the driver name that that the caller
                        is requesting, and it must match one of the languages specified
                        in SupportedLanguages.  The number of languages supported by a
                        driver is up to the driver writer.
  @param DriverName     A pointer to the Unicode string to return.  This Unicode string
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
SataControllerComponentNameGetDriverName (
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN CHAR8                        *Language,
  OUT CHAR16                      **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mSataControllerDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gSataControllerComponentName)
           );
}

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by an UEFI Driver.

  @param This                   A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param ControllerHandle       The handle of a controller that the driver specified by
                                This is managing.  This handle specifies the controller
                                whose name is to be returned.
  @param ChildHandle OPTIONAL   The handle of the child controller to retrieve the name
                                of.  This is an optional parameter that may be NULL.  It
                                will be NULL for device drivers.  It will also be NULL
                                for a bus drivers that wish to retrieve the name of the
                                bus controller.  It will not be NULL for a bus driver
                                that wishes to retrieve the name of a child controller.
  @param Language               A pointer to a three character ISO 639-2 language
                                identifier.  This is the language of the controller name
                                that that the caller is requesting, and it must match one
                                of the languages specified in SupportedLanguages.  The
                                number of languages supported by a driver is up to the
                                driver writer.
  @param ControllerName         A pointer to the Unicode string to return.  This Unicode
                                string is the name of the controller specified by
                                ControllerHandle and ChildHandle in the language
                                specified by Language from the point of view of the
                                driver specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in the
                                language specified by Language for the driver
                                specified by This was returned in DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.
**/
EFI_STATUS
EFIAPI
SataControllerComponentNameGetControllerName (
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_HANDLE                   ChildHandle OPTIONAL,
  IN CHAR8                        *Language,
  OUT CHAR16                      **ControllerName
  )
{
  EFI_STATUS  Status;

  //
  // Make sure this driver is currently managing ControllHandle
  //
  Status = EfiTestManagedDevice (
             ControllerHandle,
             gSataControllerDriverBinding.DriverBindingHandle,
             &gEfiPciIoProtocolGuid
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mSataControllerControllerNameTable,
           ControllerName,
           (BOOLEAN)(This == &gSataControllerComponentName)
           );
}
