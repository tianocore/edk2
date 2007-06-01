/** @file
  EFI Driver Model Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  UefiDriverModelLib.c

**/



/**
  The constructor function installs the standard EFI Driver Model Protocols.

  @param[in] ImageHandle The firmware allocated handle for the EFI image.
  @param[in] SystemTable A pointer to the EFI System Table.

  @retval EFI_SUCCESS The constructor always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
UefiDriverModelLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                   Status;
  UINTN                        Index;
  EFI_HANDLE                   DriverBindingHandle;
  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding;

  //
  // If no Driver Binding Protocols are advertised by the driver then simply return
  //
  if (_gDriverModelProtocolListEntries == 0) {
    return EFI_SUCCESS;
  }

  //
  // Install the first Driver Bindng Protocol onto ImageHandle
  //
  DriverBindingHandle = ImageHandle;

  //
  // See if onle one Driver Binding Protocol is advertised by the driver
  //
  if (_gDriverModelProtocolListEntries == 1) {
    //
    // The Driver Binding Protocol must never be NULL
    //
    ASSERT(_gDriverModelProtocolList[0].DriverBinding != NULL);

    //
    // Check for all 8 possible combinations of the ComponentName, DriverConfiguration, and DriverDiagnostics Protocol
    // These are all checks against const pointers, so the optimizing compiler will only select one of the
    // calls to InstallMultipleProtocolInterfaces()
    //
    if (_gDriverModelProtocolList[0].DriverDiagnostics == NULL) {
      if (_gDriverModelProtocolList[0].DriverConfiguration == NULL) {
        if (_gDriverModelProtocolList[0].ComponentName == NULL) {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid, (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid, (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentNameProtocolGuid, (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
                          NULL
                          );
        }
      } else {
        if (_gDriverModelProtocolList[0].ComponentName == NULL) {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiDriverConfigurationProtocolGuid, (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentNameProtocolGuid,       (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
                          &gEfiDriverConfigurationProtocolGuid, (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration,
                          NULL
                          );
        }
      }
    } else {
      if (_gDriverModelProtocolList[0].DriverConfiguration == NULL) {
        if (_gDriverModelProtocolList[0].ComponentName == NULL) {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,     (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiDriverDiagnosticsProtocolGuid, (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,     (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentNameProtocolGuid,     (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
                          &gEfiDriverDiagnosticsProtocolGuid, (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        }
      } else {
        if (_gDriverModelProtocolList[0].ComponentName == NULL) {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiDriverConfigurationProtocolGuid, (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration,
                          &gEfiDriverDiagnosticsProtocolGuid,   (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentNameProtocolGuid,       (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
                          &gEfiDriverConfigurationProtocolGuid, (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration,
                          &gEfiDriverDiagnosticsProtocolGuid,   (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        }
      }
    }

    //
    // ASSERT if the call to InstallMultipleProtocolInterfaces() failed
    //
    ASSERT_EFI_ERROR (Status);

    //
    // Update the ImageHandle and DriverBindingHandle fields of the Driver Binding Protocol
    //
    DriverBinding = (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding;
    DriverBinding->ImageHandle         = ImageHandle;
    DriverBinding->DriverBindingHandle = DriverBindingHandle;

  } else {
    for (Index = 0; Index < _gDriverModelProtocolListEntries; Index++) {
      //
      // The Driver Binding Protocol must never be NULL
      //
      ASSERT(_gDriverModelProtocolList[Index].DriverBinding != NULL);

      //
      // Install the Driver Binding Protocol and ASSERT() if the installation fails
      //
      Status = gBS->InstallProtocolInterface (
                      &DriverBindingHandle,
                      &gEfiDriverBindingProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[Index].DriverBinding
                      );
      ASSERT_EFI_ERROR (Status);

      //
      // Update the ImageHandle and DriverBindingHandle fields of the Driver Binding Protocol
      //
      DriverBinding = (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[Index].DriverBinding;
      DriverBinding->ImageHandle         = ImageHandle;
      DriverBinding->DriverBindingHandle = DriverBindingHandle;

      //
      // If Component Name Protocol is specified then install it and ASSERT() if the installation fails
      //
      if ((_gDriverModelProtocolBitmask & UEFI_DRIVER_MODEL_LIBRARY_COMPONENT_NAME_PROTOCOL_ENABLED) != 0) {
        if (_gDriverModelProtocolList[Index].ComponentName != NULL) {
          Status = gBS->InstallProtocolInterface (
                          &DriverBindingHandle,
                          &gEfiComponentNameProtocolGuid,
                          EFI_NATIVE_INTERFACE,
                          (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[Index].ComponentName
                          );
          ASSERT_EFI_ERROR (Status);
        }
      }

      //
      // If Driver Configuration Protocol is specified then install it and ASSERT() if the installation fails
      //
      if ((_gDriverModelProtocolBitmask & UEFI_DRIVER_MODEL_LIBRARY_DRIVER_CONFIGURATION_PROTOCOL_ENABLED) != 0) {
        if (_gDriverModelProtocolList[Index].DriverConfiguration != NULL) {
          Status = gBS->InstallProtocolInterface (
                          &DriverBindingHandle,
                          &gEfiDriverConfigurationProtocolGuid,
                          EFI_NATIVE_INTERFACE,
                          (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[Index].DriverConfiguration
                          );
          ASSERT_EFI_ERROR (Status);
        }
      }

      //
      // If Driver Diagnostics Protocol is specified then install it and ASSERT() if the installation fails
      //
      if ((_gDriverModelProtocolBitmask & UEFI_DRIVER_MODEL_LIBRARY_DRIVER_DIAGNOSTICS_PROTOCOL_ENABLED) != 0) {
        if (_gDriverModelProtocolList[Index].DriverDiagnostics != NULL) {
          Status = gBS->InstallProtocolInterface (
                          &DriverBindingHandle,
                          &gEfiDriverDiagnosticsProtocolGuid,
                          EFI_NATIVE_INTERFACE,
                          (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[Index].DriverDiagnostics
                          );
          ASSERT_EFI_ERROR (Status);
        }
      }

      //
      // Install subsequent Driver Bindng Protocols onto new handles
      //
      DriverBindingHandle = NULL;
    }
  }
  return EFI_SUCCESS;
}

/**
  The destructor function uninstalls the standard EFI Driver Model Protocols.

  @param[in] ImageHandle The firmware allocated handle for the EFI image.
  @param[in] SystemTable A pointer to the EFI System Table.

  @retval EFI_SUCCESS The destructor always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
UefiDriverModelLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  EFI_HANDLE  DriverBindingHandle;

  //
  // If no Driver Binding Protocols are advertised by the driver then simply return
  //
  if (_gDriverModelProtocolListEntries == 0) {
    return EFI_SUCCESS;
  }

  //
  // See if onle one Driver Binding Protocol is advertised by the driver
  //
  if (_gDriverModelProtocolListEntries == 1) {
    //
    // The Driver Binding Protocol must never be NULL
    //
    ASSERT(_gDriverModelProtocolList[0].DriverBinding != NULL);

    //
    // Retrieve the DriverBindingHandle from the Driver Binding Protocol
    //
    DriverBindingHandle = _gDriverModelProtocolList[0].DriverBinding->DriverBindingHandle;

    //
    // Check for all 8 possible combinations of the ComponentName, DriverConfiguration, and DriverDiagnostics Protocol
    // These are all checks against const pointers, so the optimizing compiler will only select one of the
    // calls to InstallMultipleProtocolInterfaces()
    //
    if (_gDriverModelProtocolList[0].DriverDiagnostics == NULL) {
      if (_gDriverModelProtocolList[0].DriverConfiguration == NULL) {
        if (_gDriverModelProtocolList[0].ComponentName == NULL) {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid, (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          NULL
                          );
        } else {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid, (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentNameProtocolGuid, (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
                          NULL
                          );
        }
      } else {
        if (_gDriverModelProtocolList[0].ComponentName == NULL) {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiDriverConfigurationProtocolGuid, (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration,
                          NULL
                          );
        } else {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentNameProtocolGuid,       (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
                          &gEfiDriverConfigurationProtocolGuid, (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration,
                          NULL
                          );
        }
      }
    } else {
      if (_gDriverModelProtocolList[0].DriverConfiguration == NULL) {
        if (_gDriverModelProtocolList[0].ComponentName == NULL) {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,     (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiDriverDiagnosticsProtocolGuid, (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        } else {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,     (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentNameProtocolGuid,     (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
                          &gEfiDriverDiagnosticsProtocolGuid, (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        }
      } else {
        if (_gDriverModelProtocolList[0].ComponentName == NULL) {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiDriverConfigurationProtocolGuid, (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration,
                          &gEfiDriverDiagnosticsProtocolGuid,   (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        } else {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentNameProtocolGuid,       (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
                          &gEfiDriverConfigurationProtocolGuid, (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration,
                          &gEfiDriverDiagnosticsProtocolGuid,   (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        }
      }
    }

    //
    // ASSERT if the call to UninstallMultipleProtocolInterfaces() failed
    //
    ASSERT_EFI_ERROR (Status);
  } else {
    for (Index = 0; Index < _gDriverModelProtocolListEntries; Index++) {
      //
      // The Driver Binding Protocol must never be NULL
      //
      ASSERT(_gDriverModelProtocolList[Index].DriverBinding != NULL);

      //
      // Retrieve the DriverBindingHandle from the Driver Binding Protocol
      //
      DriverBindingHandle = _gDriverModelProtocolList[0].DriverBinding->DriverBindingHandle;

      //
      // Uninstall the Driver Binding Protocol and ASSERT() if the installation fails
      //
      Status = gBS->UninstallProtocolInterface (
                      DriverBindingHandle,
                      &gEfiDriverBindingProtocolGuid,
                      (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[Index].DriverBinding
                      );
      ASSERT_EFI_ERROR (Status);

      //
      // If Component Name Protocol is specified then uninstall it and ASSERT() if the uninstallation fails
      //
      if ((_gDriverModelProtocolBitmask & UEFI_DRIVER_MODEL_LIBRARY_COMPONENT_NAME_PROTOCOL_ENABLED) != 0) {
        if (_gDriverModelProtocolList[Index].ComponentName != NULL) {
          Status = gBS->UninstallProtocolInterface (
                          DriverBindingHandle,
                          &gEfiComponentNameProtocolGuid,
                          (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[Index].ComponentName
                          );
          ASSERT_EFI_ERROR (Status);
        }
      }

      //
      // If Driver Configuration Protocol is specified then uninstall it and ASSERT() if the uninstallation fails
      //
      if ((_gDriverModelProtocolBitmask & UEFI_DRIVER_MODEL_LIBRARY_DRIVER_CONFIGURATION_PROTOCOL_ENABLED) != 0) {
        if (_gDriverModelProtocolList[Index].DriverConfiguration != NULL) {
          Status = gBS->UninstallProtocolInterface (
                          DriverBindingHandle,
                          &gEfiDriverConfigurationProtocolGuid,
                          (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[Index].DriverConfiguration
                          );
          ASSERT_EFI_ERROR (Status);
        }
      }

      //
      // If Driver Diagnostics Protocol is specified then uninstall it and ASSERT() if the uninstallation fails
      //
      if ((_gDriverModelProtocolBitmask & UEFI_DRIVER_MODEL_LIBRARY_DRIVER_DIAGNOSTICS_PROTOCOL_ENABLED) != 0) {
        if (_gDriverModelProtocolList[Index].DriverDiagnostics != NULL) {
          Status = gBS->UninstallProtocolInterface (
                          DriverBindingHandle,
                          &gEfiDriverDiagnosticsProtocolGuid,
                          (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[Index].DriverDiagnostics
                          );
          ASSERT_EFI_ERROR (Status);
        }
      }
    }
  }
  return EFI_SUCCESS;
}
