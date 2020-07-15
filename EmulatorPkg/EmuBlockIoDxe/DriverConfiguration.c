/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  DriverConfiguration.c

Abstract:

**/

#include "EmuBlockIo.h"

//
// EFI Driver Configuration Functions
//
EFI_STATUS
EFIAPI
EmuBlockIoDriverConfigurationSetOptions (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL                      *This,
  IN  EFI_HANDLE                                             ControllerHandle,
  IN  EFI_HANDLE                                             ChildHandle  OPTIONAL,
  IN  CHAR8                                                  *Language,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED               *ActionRequired
  );

EFI_STATUS
EFIAPI
EmuBlockIoDriverConfigurationOptionsValid (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL               *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle  OPTIONAL
  );

EFI_STATUS
EFIAPI
EmuBlockIoDriverConfigurationForceDefaults (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL                      *This,
  IN  EFI_HANDLE                                             ControllerHandle,
  IN  EFI_HANDLE                                             ChildHandle  OPTIONAL,
  IN  UINT32                                                 DefaultType,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED               *ActionRequired
  );

//
// EFI Driver Configuration Protocol
//
EFI_DRIVER_CONFIGURATION_PROTOCOL gEmuBlockIoDriverConfiguration = {
  EmuBlockIoDriverConfigurationSetOptions,
  EmuBlockIoDriverConfigurationOptionsValid,
  EmuBlockIoDriverConfigurationForceDefaults,
  "eng"
};

/*++

  Routine Description:
    Allows the user to set controller specific options for a controller that a
    driver is currently managing.

  Arguments:
    This             - A pointer to the EFI_DRIVER_CONFIGURATION_ PROTOCOL instance.
    ControllerHandle - The handle of the controller to set options on.
    ChildHandle      - The handle of the child controller to set options on.  This
                       is an optional parameter that may be NULL.  It will be NULL
                       for device drivers, and for a bus drivers that wish to set
                       options for the bus controller.  It will not be NULL for a
                       bus driver that wishes to set options for one of its child
                       controllers.
    Language         - A pointer to a three character ISO 639-2 language identifier.
                       This is the language of the user interface that should be
                       presented to the user, and it must match one of the languages
                       specified in SupportedLanguages.  The number of languages
                       supported by a driver is up to the driver writer.
    ActionRequired   - A pointer to the action that the calling agent is required
                       to perform when this function returns.  See "Related
                       Definitions" for a list of the actions that the calling
                       agent is required to perform prior to accessing
                       ControllerHandle again.

  Returns:
    EFI_SUCCESS           - The driver specified by This successfully set the
                            configuration options for the controller specified
                            by ControllerHandle..
    EFI_INVALID_PARAMETER - ControllerHandle is not a valid EFI_HANDLE.
    EFI_INVALID_PARAMETER - ChildHandle is not NULL and it is not a valid EFI_HANDLE.
    EFI_INVALID_PARAMETER - ActionRequired is NULL.
    EFI_UNSUPPORTED       - The driver specified by This does not support setting
                            configuration options for the controller specified by
                            ControllerHandle and ChildHandle.
    EFI_UNSUPPORTED       - The driver specified by This does not support the
                            language specified by Language.
    EFI_DEVICE_ERROR      - A device error occurred while attempt to set the
                            configuration options for the controller specified
                            by ControllerHandle and ChildHandle.
    EFI_OUT_RESOURCES     - There are not enough resources available to set the
                            configuration options for the controller specified
                            by ControllerHandle and ChildHandle.

--*/
EFI_STATUS
EFIAPI
EmuBlockIoDriverConfigurationSetOptions (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL                      *This,
  IN  EFI_HANDLE                                             ControllerHandle,
  IN  EFI_HANDLE                                             ChildHandle  OPTIONAL,
  IN  CHAR8                                                  *Language,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED               *ActionRequired
  )
{
  EFI_STATUS            Status;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
  CHAR8                 *SupportedLanguage;

  SupportedLanguage = This->SupportedLanguages;

  Status            = EFI_UNSUPPORTED;
  while (*SupportedLanguage != 0) {
    if (AsciiStrnCmp (Language, SupportedLanguage, 3) == 0) {
      Status = EFI_SUCCESS;
    }

    SupportedLanguage += 3;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (ActionRequired == NULL || ControllerHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Validate controller handle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEmuIoThunkProtocolGuid,
                  (VOID **)&BlockIo,
                  gEmuBlockIoDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEmuIoThunkProtocolGuid,
          gEmuBlockIoDriverBinding.DriverBindingHandle,
          ControllerHandle
          );

    return EFI_UNSUPPORTED;
  }

  if (Status == EFI_UNSUPPORTED) {
    return Status;
  } else if (Status != EFI_ALREADY_STARTED) {
    return EFI_INVALID_PARAMETER;
  }

  *ActionRequired = EfiDriverConfigurationActionNone;
  return EFI_SUCCESS;
}

/*++

  Routine Description:
    Tests to see if a controller's current configuration options are valid.

  Arguments:
    This             - A pointer to the EFI_DRIVER_CONFIGURATION_PROTOCOL instance.
    ControllerHandle - The handle of the controller to test if it's current
                       configuration options are valid.
    ChildHandle      - The handle of the child controller to test if it's current
                       configuration options are valid.  This is an optional
                       parameter that may be NULL.  It will be NULL for device
                       drivers.  It will also be NULL for a bus drivers that wish
                       to test the configuration options for the bus controller.
                       It will not be NULL for a bus driver that wishes to test
                       configuration options for one of its child controllers.

  Returns:
    EFI_SUCCESS           - The controller specified by ControllerHandle and
                            ChildHandle that is being managed by the driver
                            specified by This has a valid set of  configuration
                            options.
    EFI_INVALID_PARAMETER - ControllerHandle is not a valid EFI_HANDLE.
    EFI_INVALID_PARAMETER - ChildHandle is not NULL and it is not a valid EFI_HANDLE.
    EFI_UNSUPPORTED       - The driver specified by This is not currently
                            managing the controller specified by ControllerHandle
                            and ChildHandle.
    EFI_DEVICE_ERROR      - The controller specified by ControllerHandle and
                            ChildHandle that is being managed by the driver
                            specified by This has an invalid set of configuration
                            options.

--*/
EFI_STATUS
EFIAPI
EmuBlockIoDriverConfigurationOptionsValid (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL               *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle  OPTIONAL
  )
{
  EFI_STATUS            Status;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;

  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }

  if (ControllerHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Validate controller handle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEmuIoThunkProtocolGuid,
                  (VOID **)&BlockIo,
                  gEmuBlockIoDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEmuIoThunkProtocolGuid,
          gEmuBlockIoDriverBinding.DriverBindingHandle,
          ControllerHandle
          );

    return EFI_UNSUPPORTED;
  }

  if (Status == EFI_UNSUPPORTED) {
    return Status;
  } else if (Status != EFI_ALREADY_STARTED) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/*++

  Routine Description:
    Forces a driver to set the default configuration options for a controller.

  Arguments:
    This             - A pointer to the EFI_DRIVER_CONFIGURATION_ PROTOCOL instance.
    ControllerHandle - The handle of the controller to force default configuration options on.
    ChildHandle      - The handle of the child controller to force default configuration options on  This is an optional parameter that may be NULL.  It will be NULL for device drivers.  It will also be NULL for a bus drivers that wish to force default configuration options for the bus controller.  It will not be NULL for a bus driver that wishes to force default configuration options for one of its child controllers.
    DefaultType      - The type of default configuration options to force on the controller specified by ControllerHandle and ChildHandle.  See Table 9-1 for legal values.  A DefaultType of 0x00000000 must be supported by this protocol.
    ActionRequired   - A pointer to the action that the calling agent is required to perform when this function returns.  See "Related Definitions" in Section 9.1for a list of the actions that the calling agent is required to perform prior to accessing ControllerHandle again.

  Returns:
    EFI_SUCCESS           - The driver specified by This successfully forced the default configuration options on the controller specified by ControllerHandle and ChildHandle.
    EFI_INVALID_PARAMETER - ControllerHandle is not a valid EFI_HANDLE.
    EFI_INVALID_PARAMETER - ChildHandle is not NULL and it is not a valid EFI_HANDLE.
    EFI_INVALID_PARAMETER - ActionRequired is NULL.
    EFI_UNSUPPORTED       - The driver specified by This does not support forcing the default configuration options on the controller specified by ControllerHandle and ChildHandle.
    EFI_UNSUPPORTED       - The driver specified by This does not support the configuration type specified by DefaultType.
    EFI_DEVICE_ERROR      - A device error occurred while attempt to force the default configuration options on the controller specified by  ControllerHandle and ChildHandle.
    EFI_OUT_RESOURCES     - There are not enough resources available to force the default configuration options on the controller specified by ControllerHandle and ChildHandle.

--*/
EFI_STATUS
EFIAPI
EmuBlockIoDriverConfigurationForceDefaults (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL                      *This,
  IN  EFI_HANDLE                                             ControllerHandle,
  IN  EFI_HANDLE                                             ChildHandle  OPTIONAL,
  IN  UINT32                                                 DefaultType,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED               *ActionRequired
  )
{
  EFI_STATUS            Status;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;

  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }

  if (ActionRequired == NULL || ControllerHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Validate controller handle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEmuIoThunkProtocolGuid,
                  (VOID **)&BlockIo,
                  gEmuBlockIoDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEmuIoThunkProtocolGuid,
          gEmuBlockIoDriverBinding.DriverBindingHandle,
          ControllerHandle
          );

    return EFI_UNSUPPORTED;
  }

  if (Status == EFI_UNSUPPORTED) {
    return Status;
  } else if (Status != EFI_ALREADY_STARTED) {
    return EFI_INVALID_PARAMETER;
  }

  *ActionRequired = EfiDriverConfigurationActionNone;
  return EFI_SUCCESS;
}
