/** @file
  Implementation of UEFI Driver Configuration Protocol for IDE bus driver which 
  provides ability to set IDE bus controller specific options.
  
  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include "IdeBus.h"

CHAR16 *OptionString[4] = {
  L"Enable Primary Master    (Y/N)? -->",
  L"Enable Primary Slave     (Y/N)? -->",
  L"Enable Secondary Master  (Y/N)? -->",
  L"Enable Secondary Slave   (Y/N)? -->"
};

//
// EFI Driver Configuration Protocol
//
EFI_DRIVER_CONFIGURATION_PROTOCOL gIDEBusDriverConfiguration = {
  IDEBusDriverConfigurationSetOptions,
  IDEBusDriverConfigurationOptionsValid,
  IDEBusDriverConfigurationForceDefaults,
  "eng"
};

/**
  Interprete keyboard input.

  @retval  EFI_ABORTED  Get an 'ESC' key inputed.
  @retval  EFI_SUCCESS  Get an 'Y' or 'y' inputed.
  @retval  EFI_NOT_FOUND Get an 'N' or 'n' inputed..

**/
EFI_STATUS
GetResponse (
  VOID
  )
{
  EFI_STATUS    Status;
  EFI_INPUT_KEY Key;

  while (TRUE) {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (!EFI_ERROR (Status)) {
      if (Key.ScanCode == SCAN_ESC) {
        return EFI_ABORTED;
      }

      switch (Key.UnicodeChar) {

      //
      // fall through
      //
      case L'y':
      case L'Y':
        gST->ConOut->OutputString (gST->ConOut, L"Y\n");
        return EFI_SUCCESS;

      //
      // fall through
      //
      case L'n':
      case L'N':
        gST->ConOut->OutputString (gST->ConOut, L"N\n");
        return EFI_NOT_FOUND;
      }

    }
  }
}

/**
  Allows the user to set controller specific options for a controller that a 
  driver is currently managing.

  @param  This              A pointer to the EFI_DRIVER_CONFIGURATION_ PROTOCOL instance.
  @param  ControllerHandle  The handle of the controller to set options on.
  @param  ChildHandle       The handle of the child controller to set options on.
                            This is an optional parameter that may be NULL.
                            It will be NULL for device drivers, and for a bus drivers
                            that wish to set options for the bus controller.
                            It will not be NULL for a bus driver that wishes to set
                            options for one of its child controllers.
  @param  Language          A pointer to a three character ISO 639-2 language identifier. 
                            This is the language of the user interface that should be presented 
                            to the user, and it must match one of the languages specified in 
                            SupportedLanguages. The number of languages supported by a driver is up to
                            the driver writer.
  @param  ActionRequired    A pointer to the action that the calling agent is required 
                            to perform when this function returns.
  

  @retval  EFI_SUCCESS           The driver specified by This successfully set the configuration 
                                 options for the controller specified by ControllerHandle..
  @retval  EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval  EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval  EFI_INVALID_PARAMETER ActionRequired is NULL.
  @retval  EFI_UNSUPPORTED       The driver specified by This does not support setting configuration options for 
                                 the controller specified by ControllerHandle and ChildHandle.
  @retval  EFI_UNSUPPORTED       The driver specified by This does not support the language specified by Language.
  @retval  EFI_DEVICE_ERROR      A device error occurred while attempt to set the configuration options for the 
                                 controller specified by ControllerHandle and ChildHandle.
  @retval  EFI_OUT_RESOURCES     There are not enough resources available to set the configuration options for the 
                                 controller specified by ControllerHandle and ChildHandle
**/
EFI_STATUS
EFIAPI
IDEBusDriverConfigurationSetOptions (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL                      *This,
  IN  EFI_HANDLE                                             ControllerHandle,
  IN  EFI_HANDLE                                             ChildHandle  OPTIONAL,
  IN  CHAR8                                                  *Language,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED               *ActionRequired
  )
{
  EFI_STATUS  Status;
  UINT8       Value;
  UINT8       NewValue;
  UINTN       DataSize;
  UINTN       Index;

  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }

  *ActionRequired = EfiDriverConfigurationActionNone;

  DataSize        = sizeof (Value);
  Status = gRT->GetVariable (
                  L"Configuration",
                  &gEfiCallerIdGuid,
                  NULL,
                  &DataSize,
                  &Value
                  );

  gST->ConOut->OutputString (gST->ConOut, L"IDE Bus Driver Configuration\n");
  gST->ConOut->OutputString (gST->ConOut, L"===============================\n");

  NewValue = 0;
  for (Index = 0; Index < 4; Index++) {
    gST->ConOut->OutputString (gST->ConOut, OptionString[Index]);

    Status = GetResponse ();
    if (Status == EFI_ABORTED) {
      return EFI_SUCCESS;
    }

    if (!EFI_ERROR (Status)) {
      NewValue = (UINT8) (NewValue | (1 << Index));
    }
  }

  if (EFI_ERROR (Status) || (NewValue != Value)) {
    gRT->SetVariable (
          L"Configuration",
          &gEfiCallerIdGuid,
          EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
          sizeof (NewValue),
          &NewValue
          );

    *ActionRequired = EfiDriverConfigurationActionRestartController;
  } else {
    *ActionRequired = EfiDriverConfigurationActionNone;
  }

  return EFI_SUCCESS;
}

/**
  Tests to see if a controller's current configuration options are valid.

  @param  This             A pointer to the EFI_DRIVER_CONFIGURATION_PROTOCOL instance.
  @param  ControllerHandle The handle of the controller to test if it's current configuration options 
                           are valid.
  @param  ChildHandle      The handle of the child controller to test if it's current configuration 
                           options are valid.  This is an optional parameter that may be NULL. It will 
                           be NULL for device drivers.  It will also be NULL for a bus drivers that
                           wish to test the configuration options for the bus controller. It will 
                           not be NULL for a bus driver that wishes to test configuration options for 
                           one of its child controllers.
  @retval  EFI_SUCCESS           The controller specified by ControllerHandle and ChildHandle that is being
                                 managed by the driver specified by This has a valid set of  configuration
                                 options.
  @retval  EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval  EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval  EFI_UNSUPPORTED       The driver specified by This is not currently  managing the controller 
                                 specified by ControllerHandle and ChildHandle.
  @retval  EFI_DEVICE_ERROR      The controller specified by ControllerHandle and ChildHandle that is being
                                 managed by the driver specified by This has an invalid set of configuration
                                 options.
**/
EFI_STATUS
EFIAPI
IDEBusDriverConfigurationOptionsValid (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL               *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT8       Value;
  UINTN       DataSize;

  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }

  DataSize = sizeof (Value);
  Status = gRT->GetVariable (
                  L"Configuration",
                  &gEfiCallerIdGuid,
                  NULL,
                  &DataSize,
                  &Value
                  );
  if (EFI_ERROR (Status) || Value > 0x0f) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
/**
  Forces a driver to set the default configuration options for a controller.

  @param  This             A pointer to the EFI_DRIVER_CONFIGURATION_ PROTOCOL instance.
  @param  ControllerHandle The handle of the controller to force default configuration options on.
  @param  ChildHandle      The handle of the child controller to force default configuration 
                           options on  This is an optional parameter that may be NULL.  It 
                           will be NULL for device drivers. It will also be NULL for a bus 
                           drivers that wish to force default configuration options for the bus
                           controller.  It will not be NULL for a bus driver that wishes to force
                           default configuration options for one of its child controllers.
  @param  DefaultType      The type of default configuration options to force on the controller 
                           specified by ControllerHandle and ChildHandle. 
  @param  ActionRequired   A pointer to the action that the calling agent is required to perform 
                           when this function returns.

  @retval  EFI_SUCCESS           The driver specified by This successfully forced the 
                                 default configuration options on the controller specified by 
                                 ControllerHandle and ChildHandle.
  @retval  EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval  EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval  EFI_INVALID_PARAMETER ActionRequired is NULL.
  @retval  EFI_UNSUPPORTED       The driver specified by This does not support forcing the default 
                                 configuration options on the controller specified by ControllerHandle
                                 and ChildHandle.
  @retval  EFI_UNSUPPORTED       The driver specified by This does not support the configuration type 
                                 specified by DefaultType.
  @retval  EFI_DEVICE_ERROR      A device error occurred while attempt to force the default configuration 
                                 options on the controller specified by  ControllerHandle and ChildHandle.
  @retval  EFI_OUT_RESOURCES     There are not enough resources available to force the default configuration 
                                 options on the controller specified by ControllerHandle and ChildHandle.
**/
EFI_STATUS
EFIAPI
IDEBusDriverConfigurationForceDefaults (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL                      *This,
  IN  EFI_HANDLE                                             ControllerHandle,
  IN  EFI_HANDLE                                             ChildHandle  OPTIONAL,
  IN  UINT32                                                 DefaultType,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED               *ActionRequired
  )
{
  UINT8 Value;

  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }

  Value = 0x0f;
  gRT->SetVariable (
        L"Configuration",
        &gEfiCallerIdGuid,
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
        sizeof (Value),
        &Value
        );
  *ActionRequired = EfiDriverConfigurationActionRestartController;
  return EFI_SUCCESS;
}
