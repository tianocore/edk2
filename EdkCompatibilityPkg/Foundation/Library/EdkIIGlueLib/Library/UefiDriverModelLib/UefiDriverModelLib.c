/*++

Copyright (c) 2004 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.              


Module Name:

  UefiDriverModelLib.c
  
Abstract: 

  UEFI Driver Model Library.

--*/

#include "EdkIIGlueUefi.h"


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
  EFI_STATUS                   Status = EFI_UNSUPPORTED;
  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding;

  //
  // The Driver Binding Protocol must never be NULL
  //
  DriverBinding = (EFI_DRIVER_BINDING_PROTOCOL *) _gDriverModelProtocolList[0].DriverBinding;
  ASSERT(DriverBinding != NULL);

  //
  // Update the ImageHandle and DriverBindingHandle fields of the Driver Binding Protocol
  // Install the first Driver Bindng Protocol onto ImageHandle
  //
  DriverBinding->ImageHandle         = ImageHandle;
  DriverBinding->DriverBindingHandle = ImageHandle;

  //
  // See if onle one Driver Binding Protocol is advertised by the driver
  // EdkIIGlueLib: _gDriverModelProtocolListEntries is always 1
  //


  //
  // Check for all 8 possible combinations of the ComponentName, DriverConfiguration, and DriverDiagnostics Protocol
  // These are all checks against const pointers, so the optimizing compiler will only select one of the
  // calls to InstallMultipleProtocolInterfaces()
  //
  if ((_gEdkIIGlueDriverModelProtocolSelection == 1) || (_gEdkIIGlueDriverModelProtocolSelection == 3)) {
    if (_gDriverModelProtocolList[0].DriverDiagnostics == NULL) {
      if (_gDriverModelProtocolList[0].DriverConfiguration == NULL) {
        if (_gDriverModelProtocolList[0].ComponentName == NULL) {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid, DriverBinding,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid, DriverBinding,
                          &gEfiComponentNameProtocolGuid, (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
                          NULL
                          );
        }
      } else {
        if (_gDriverModelProtocolList[0].ComponentName == NULL) {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       DriverBinding,
                          &gEfiDriverConfigurationProtocolGuid, (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       DriverBinding,
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
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,     DriverBinding,
                          &gEfiDriverDiagnosticsProtocolGuid, (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,     DriverBinding,
                          &gEfiComponentNameProtocolGuid,     (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
                          &gEfiDriverDiagnosticsProtocolGuid, (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        }
      } else {
        if (_gDriverModelProtocolList[0].ComponentName == NULL) {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       DriverBinding,
                          &gEfiDriverConfigurationProtocolGuid, (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration,
                          &gEfiDriverDiagnosticsProtocolGuid,   (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,       DriverBinding,
                          &gEfiComponentNameProtocolGuid,       (EFI_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
                          &gEfiDriverConfigurationProtocolGuid, (EFI_DRIVER_CONFIGURATION_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration,
                          &gEfiDriverDiagnosticsProtocolGuid,   (EFI_DRIVER_DIAGNOSTICS_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics,
                          NULL
                          );
        }
      }
    }
  } // selection == 1 or 3

  if ((_gEdkIIGlueDriverModelProtocolSelection == 2) || (_gEdkIIGlueDriverModelProtocolSelection == 3)) {
    if (_gDriverModelProtocolList[0].DriverDiagnostics2== NULL) {
      if (_gDriverModelProtocolList[0].DriverConfiguration2== NULL) {
        if (_gDriverModelProtocolList[0].ComponentName2== NULL) {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid, DriverBinding,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,  DriverBinding,
                          &gEfiComponentName2ProtocolGuid, (EFI_COMPONENT_NAME2_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName2,
                          NULL
                          );
        }
      } else {
        if (_gDriverModelProtocolList[0].ComponentName2 == NULL) {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,        DriverBinding,
                          &gEfiDriverConfiguration2ProtocolGuid, (EFI_DRIVER_CONFIGURATION2_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration2,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,        DriverBinding,
                          &gEfiComponentName2ProtocolGuid,       (EFI_COMPONENT_NAME2_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName2,
                          &gEfiDriverConfiguration2ProtocolGuid, (EFI_DRIVER_CONFIGURATION2_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration2,
                          NULL
                          );
        }
      }
    } else {
      if (_gDriverModelProtocolList[0].DriverConfiguration2 == NULL) {
        if (_gDriverModelProtocolList[0].ComponentName2 == NULL) {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,      DriverBinding,
                          &gEfiDriverDiagnostics2ProtocolGuid, (EFI_DRIVER_DIAGNOSTICS2_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics2,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,      DriverBinding,
                          &gEfiComponentName2ProtocolGuid,     (EFI_COMPONENT_NAME2_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName2,
                          &gEfiDriverDiagnostics2ProtocolGuid, (EFI_DRIVER_DIAGNOSTICS2_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics2,
                          NULL
                          );
        }
      } else {
        if (_gDriverModelProtocolList[0].ComponentName2 == NULL) {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,        DriverBinding,
                          &gEfiDriverConfiguration2ProtocolGuid, (EFI_DRIVER_CONFIGURATION2_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration2,
                          &gEfiDriverDiagnostics2ProtocolGuid,   (EFI_DRIVER_DIAGNOSTICS2_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics2,
                          NULL
                          );
        } else {
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &DriverBinding->DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,        DriverBinding,
                          &gEfiComponentName2ProtocolGuid,       (EFI_COMPONENT_NAME2_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName2,
                          &gEfiDriverConfiguration2ProtocolGuid, (EFI_DRIVER_CONFIGURATION2_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration2,
                          &gEfiDriverDiagnostics2ProtocolGuid,   (EFI_DRIVER_DIAGNOSTICS2_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics2,
                          NULL
                          );
        }
      }
    }
  } // selection == 2 or 3

  //
  // ASSERT if the call to InstallMultipleProtocolInterfaces() failed
  //
  ASSERT_EFI_ERROR (Status);
  return Status;
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
  EFI_STATUS  Status = EFI_UNSUPPORTED;
  EFI_HANDLE  DriverBindingHandle;

  //
  // See if onle one Driver Binding Protocol is advertised by the driver
  // EdkIIGlueLib: _gDriverModelProtocolListEntries is always 1
  //

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
  if ((_gEdkIIGlueDriverModelProtocolSelection == 1) || (_gEdkIIGlueDriverModelProtocolSelection == 3)) {
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
  } // selection == 1 or 3

  if ((_gEdkIIGlueDriverModelProtocolSelection == 2) || (_gEdkIIGlueDriverModelProtocolSelection == 3)) {
    if (_gDriverModelProtocolList[0].DriverDiagnostics2 == NULL) {
      if (_gDriverModelProtocolList[0].DriverConfiguration2 == NULL) {
        if (_gDriverModelProtocolList[0].ComponentName2 == NULL) {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid, (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          NULL
                          );
        } else {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid, (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentName2ProtocolGuid, (EFI_COMPONENT_NAME2_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName2,
                          NULL
                          );
        }
      } else {
        if (_gDriverModelProtocolList[0].ComponentName2 == NULL) {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,        (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiDriverConfiguration2ProtocolGuid, (EFI_DRIVER_CONFIGURATION2_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration2,
                          NULL
                          );
        } else {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,        (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentName2ProtocolGuid,       (EFI_COMPONENT_NAME2_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName2,
                          &gEfiDriverConfiguration2ProtocolGuid, (EFI_DRIVER_CONFIGURATION2_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration2,
                          NULL
                          );
        }
      }
    } else {
      if (_gDriverModelProtocolList[0].DriverConfiguration2 == NULL) {
        if (_gDriverModelProtocolList[0].ComponentName2 == NULL) {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,      (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiDriverDiagnostics2ProtocolGuid, (EFI_DRIVER_DIAGNOSTICS2_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics2,
                          NULL
                          );
        } else {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,      (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentName2ProtocolGuid,     (EFI_COMPONENT_NAME2_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName2,
                          &gEfiDriverDiagnostics2ProtocolGuid, (EFI_DRIVER_DIAGNOSTICS2_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics2,
                          NULL
                          );
        }
      } else {
        if (_gDriverModelProtocolList[0].ComponentName2 == NULL) {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,        (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiDriverConfiguration2ProtocolGuid, (EFI_DRIVER_CONFIGURATION2_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration2,
                          &gEfiDriverDiagnostics2ProtocolGuid,   (EFI_DRIVER_DIAGNOSTICS2_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics2,
                          NULL
                          );
        } else {
          Status = gBS->UninstallMultipleProtocolInterfaces (
                          &DriverBindingHandle,
                          &gEfiDriverBindingProtocolGuid,        (EFI_DRIVER_BINDING_PROTOCOL *)_gDriverModelProtocolList[0].DriverBinding,
                          &gEfiComponentName2ProtocolGuid,       (EFI_COMPONENT_NAME2_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName2,
                          &gEfiDriverConfiguration2ProtocolGuid, (EFI_DRIVER_CONFIGURATION2_PROTOCOL *)_gDriverModelProtocolList[0].DriverConfiguration2,
                          &gEfiDriverDiagnostics2ProtocolGuid,   (EFI_DRIVER_DIAGNOSTICS2_PROTOCOL *)_gDriverModelProtocolList[0].DriverDiagnostics2,
                          NULL
                          );
        }
      }
    }
  } // selection == 2 or 3

  //
  // ASSERT if the call to UninstallMultipleProtocolInterfaces() failed
  //
  ASSERT_EFI_ERROR (Status);
  
  return EFI_SUCCESS;
}
