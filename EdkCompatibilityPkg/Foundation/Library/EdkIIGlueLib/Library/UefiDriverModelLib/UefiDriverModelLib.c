/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  #define EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL_GUID  gEfiComponentName2ProtocolGuid
  #define EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL       EFI_COMPONENT_NAME2_PROTOCOL
#else
  #define EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL_GUID  gEfiComponentNameProtocolGuid
  #define EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL       EFI_COMPONENT_NAME_PROTOCOL
#endif

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
  EFI_HANDLE                   DriverBindingHandle;
  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding;

  //
  // Install the first Driver Bindng Protocol onto ImageHandle
  //
  DriverBindingHandle = ImageHandle;

  //
  // See if onle one Driver Binding Protocol is advertised by the driver
  // EdkIIGlueLib: _gDriverModelProtocolListEntries is always 1
  //

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
                          &EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL_GUID, (EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
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
                          &EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL_GUID,      (EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
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
                          &EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL_GUID,    (EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
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
                          &EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL_GUID,      (EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
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
                          &EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL_GUID, (EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
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
                          &EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL_GUID,      (EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
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
                          &EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL_GUID,    (EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
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
                          &EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL_GUID,      (EDKII_GLUE_SELECTED_COMPONENT_NAME_PROTOCOL *)_gDriverModelProtocolList[0].ComponentName,
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
  
  return EFI_SUCCESS;
}
