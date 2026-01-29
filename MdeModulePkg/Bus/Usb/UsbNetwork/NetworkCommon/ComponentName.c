/** @file
  This file contains code for USB network common driver
  component name definitions

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DriverBinding.h"

extern EFI_DRIVER_BINDING_PROTOCOL  gNetworkCommonDriverBinding;

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  gNetworkCommonDriverNameTable[] = {
  {
    "eng;en",
    L"Network Common Driver"
  },
  {
    NULL,
    NULL
  }
};
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  *gNetworkCommonControllerNameTable = NULL;

EFI_STATUS
EFIAPI
NetworkCommonComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
NetworkCommonComponentNameGetControllerName (
  IN EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN CHAR8                        *Language,
  OUT CHAR16                      **ControllerName
  );

GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  gNetworkCommonComponentName = {
  NetworkCommonComponentNameGetDriverName,
  NetworkCommonComponentNameGetControllerName,
  "eng"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL  gNetworkCommonComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)NetworkCommonComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)NetworkCommonComponentNameGetControllerName,
  "en"
};

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
  @param[out] DriverName        A pointer to the Unicode string to return.
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
NetworkCommonComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           gNetworkCommonDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gNetworkCommonComponentName)
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
  @param[in]  Controller        The handle of a controller that the driver
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
  @param[out] ControllerName    A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
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
NetworkCommonComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  )
{
  EFI_STATUS                 Status;
  CHAR16                     *HandleName;
  EFI_USB_IO_PROTOCOL        *UsbIo;
  EFI_USB_DEVICE_DESCRIPTOR  DevDesc;

  if ((Language == NULL) || (ControllerName == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ChildHandle == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Make sure this driver is currently managing ControllerHandle
  //
  Status = EfiTestManagedDevice (
             Controller,
             gNetworkCommonDriverBinding.DriverBindingHandle,
             &gEdkIIUsbEthProtocolGuid
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure this driver produced ChildHandle
  //
  Status = EfiTestChildHandle (
             Controller,
             ChildHandle,
             &gEdkIIUsbEthProtocolGuid
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (Controller, &gEfiUsbIoProtocolGuid, (VOID **)&UsbIo);

  if (!EFI_ERROR (Status)) {
    Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = UsbIo->UsbGetStringDescriptor (UsbIo, 0x409, DevDesc.StrManufacturer, &HandleName);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    *ControllerName = HandleName;

    if (gNetworkCommonControllerNameTable != NULL) {
      FreeUnicodeStringTable (gNetworkCommonControllerNameTable);
      gNetworkCommonControllerNameTable = NULL;
    }

    Status = AddUnicodeString2 (
               "eng",
               gNetworkCommonComponentName.SupportedLanguages,
               &gNetworkCommonControllerNameTable,
               HandleName,
               TRUE
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = AddUnicodeString2 (
               "en",
               gNetworkCommonComponentName2.SupportedLanguages,
               &gNetworkCommonControllerNameTable,
               HandleName,
               FALSE
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    return LookupUnicodeString2 (
             Language,
             This->SupportedLanguages,
             gNetworkCommonControllerNameTable,
             ControllerName,
             (BOOLEAN)(This == &gNetworkCommonComponentName)
             );
  }

  return EFI_UNSUPPORTED;
}
