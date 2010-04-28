/*++

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiDriverModelLib.c

Abstract:

  Light weight lib to support EFI drivers.

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"

EFI_STATUS
EfiLibInstallDriverBinding (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding,
  IN EFI_HANDLE                   DriverBindingHandle
  )
/*++

Routine Description:

  Intialize a driver by installing the Driver Binding Protocol onto the 
  driver's DriverBindingHandle.  This is typically the same as the driver's
  ImageHandle, but it can be different if the driver produces multiple
  DriverBinding Protocols.  This function also initializes the EFI Driver
  Library that initializes the global variables gST, gBS, gRT.

Arguments:

  ImageHandle         - The image handle of the driver

  SystemTable         - The EFI System Table that was passed to the driver's entry point

  DriverBinding       - A Driver Binding Protocol instance that this driver is producing

  DriverBindingHandle - The handle that DriverBinding is to be installe onto.  If this
                        parameter is NULL, then a new handle is created.

Returns: 

  EFI_SUCCESS is DriverBinding is installed onto DriverBindingHandle

  Otherwise, then return status from gBS->InstallProtocolInterface()

--*/
{
  EfiInitializeDriverLib (ImageHandle, SystemTable);

  DriverBinding->ImageHandle          = ImageHandle;

  DriverBinding->DriverBindingHandle  = DriverBindingHandle;

  return gBS->InstallProtocolInterface (
                &DriverBinding->DriverBindingHandle,
                &gEfiDriverBindingProtocolGuid,
                EFI_NATIVE_INTERFACE,
                DriverBinding
                );
}

EFI_STATUS
InstallAllDriverProtocolsWorker (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   * SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL        * DriverBinding,
  IN EFI_HANDLE                         DriverBindingHandle,
  IN EFI_COMPONENT_NAME_PROTOCOL        * ComponentName, OPTIONAL
  IN EFI_COMPONENT_NAME2_PROTOCOL       * ComponentName2, OPTIONAL
  IN EFI_DRIVER_CONFIGURATION_PROTOCOL  * DriverConfiguration, OPTIONAL
  IN EFI_DRIVER_CONFIGURATION2_PROTOCOL * DriverConfiguration2, OPTIONAL
  IN EFI_DRIVER_DIAGNOSTICS_PROTOCOL    * DriverDiagnostics, OPTIONAL
  IN EFI_DRIVER_DIAGNOSTICS2_PROTOCOL   * DriverDiagnostics2 OPTIONAL
  )
/*++

Routine Description:

  Intialize a driver by installing the Driver Binding Protocol onto the 
  driver's DriverBindingHandle.  This is typically the same as the driver's
  ImageHandle, but it can be different if the driver produces multiple
  DriverBinding Protocols.  This function also initializes the EFI Driver
  Library that initializes the global variables gST, gBS, gRT.

Arguments:

  ImageHandle         - The image handle of the driver

  SystemTable         - The EFI System Table that was passed to the driver's entry point

  DriverBinding       - A Driver Binding Protocol instance that this driver is producing

  DriverBindingHandle - The handle that DriverBinding is to be installe onto.  If this
                        parameter is NULL, then a new handle is created.

  ComponentName       - A Component Name Protocol instance that this driver is producing

  ComponentName2      - A Component Name2 Protocol instance that this driver is producing

  DriverConfiguration - A Driver Configuration Protocol instance that this driver is producing

  DriverConfiguration2- A Driver Configuration2 Protocol instance that this driver is producing
  
  DriverDiagnostics   - A Driver Diagnostics Protocol instance that this driver is producing

  DriverDiagnostics2  - A Driver Diagnostics2 Protocol instance that this driver is producing

Returns: 

  EFI_SUCCESS if all the protocols were installed onto DriverBindingHandle

  Otherwise, then return status from gBS->InstallProtocolInterface()

--*/
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBinding (ImageHandle, SystemTable, DriverBinding, DriverBindingHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (ComponentName != NULL) {
    Status = gBS->InstallProtocolInterface (
                    &DriverBinding->DriverBindingHandle,
                    &gEfiComponentNameProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    ComponentName
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (ComponentName2 != NULL) {
    Status = gBS->InstallProtocolInterface (
                    &DriverBinding->DriverBindingHandle,
                    &gEfiComponentName2ProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    ComponentName2
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (DriverConfiguration != NULL) {
    Status = gBS->InstallProtocolInterface (
                    &DriverBinding->DriverBindingHandle,
                    &gEfiDriverConfigurationProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    DriverConfiguration
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (DriverConfiguration2 != NULL) {
    Status = gBS->InstallProtocolInterface (
                    &DriverBinding->DriverBindingHandle,
                    &gEfiDriverConfiguration2ProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    DriverConfiguration2
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (DriverDiagnostics != NULL) {
    Status = gBS->InstallProtocolInterface (
                    &DriverBinding->DriverBindingHandle,
                    &gEfiDriverDiagnosticsProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    DriverDiagnostics
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (DriverDiagnostics2 != NULL) {
    Status = gBS->InstallProtocolInterface (
                    &DriverBinding->DriverBindingHandle,
                    &gEfiDriverDiagnostics2ProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    DriverDiagnostics2
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EfiLibInstallAllDriverProtocols (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   * SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL        * DriverBinding,
  IN EFI_HANDLE                         DriverBindingHandle,
  IN EFI_COMPONENT_NAME_PROTOCOL        * ComponentName, OPTIONAL
  IN EFI_DRIVER_CONFIGURATION_PROTOCOL  * DriverConfiguration, OPTIONAL
  IN EFI_DRIVER_DIAGNOSTICS_PROTOCOL    * DriverDiagnostics OPTIONAL
  )
/*++

Routine Description:

  Intialize a driver by installing the Driver Binding Protocol onto the 
  driver's DriverBindingHandle.  This is typically the same as the driver's
  ImageHandle, but it can be different if the driver produces multiple
  DriverBinding Protocols.  This function also initializes the EFI Driver
  Library that initializes the global variables gST, gBS, gRT.

Arguments:

  ImageHandle         - The image handle of the driver

  SystemTable         - The EFI System Table that was passed to the driver's entry point

  DriverBinding       - A Driver Binding Protocol instance that this driver is producing

  DriverBindingHandle - The handle that DriverBinding is to be installe onto.  If this
                        parameter is NULL, then a new handle is created.

  ComponentName       - A Component Name Protocol instance that this driver is producing

  DriverConfiguration - A Driver Configuration Protocol instance that this driver is producing
  
  DriverDiagnostics   - A Driver Diagnostics Protocol instance that this driver is producing

Returns: 

  EFI_SUCCESS if all the protocols were installed onto DriverBindingHandle

  Otherwise, then return status from gBS->InstallProtocolInterface()

--*/
{
  return InstallAllDriverProtocolsWorker (
           ImageHandle,
           SystemTable,
           DriverBinding,
           DriverBindingHandle,
           ComponentName,
           NULL,
           DriverConfiguration,
           NULL,
           DriverDiagnostics,
           NULL
           );
}

EFI_STATUS
EfiLibInstallAllDriverProtocols2 (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   * SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL        * DriverBinding,
  IN EFI_HANDLE                         DriverBindingHandle,
  IN EFI_COMPONENT_NAME2_PROTOCOL       * ComponentName2, OPTIONAL
  IN EFI_DRIVER_CONFIGURATION2_PROTOCOL * DriverConfiguration2, OPTIONAL
  IN EFI_DRIVER_DIAGNOSTICS2_PROTOCOL   * DriverDiagnostics2 OPTIONAL
  )
/*++

Routine Description:

  Intialize a driver by installing the Driver Binding Protocol onto the 
  driver's DriverBindingHandle.  This is typically the same as the driver's
  ImageHandle, but it can be different if the driver produces multiple
  DriverBinding Protocols.  This function also initializes the EFI Driver
  Library that initializes the global variables gST, gBS, gRT.

Arguments:

  ImageHandle         - The image handle of the driver

  SystemTable         - The EFI System Table that was passed to the driver's entry point

  DriverBinding       - A Driver Binding Protocol instance that this driver is producing

  DriverBindingHandle - The handle that DriverBinding is to be installe onto.  If this
                        parameter is NULL, then a new handle is created.

  ComponentName2      - A Component Name2 Protocol instance that this driver is producing

  DriverConfiguration2- A Driver Configuration2 Protocol instance that this driver is producing
  
  DriverDiagnostics2  - A Driver Diagnostics2 Protocol instance that this driver is producing

Returns: 

  EFI_SUCCESS if all the protocols were installed onto DriverBindingHandle

  Otherwise, then return status from gBS->InstallProtocolInterface()

--*/
{
  return InstallAllDriverProtocolsWorker (
           ImageHandle,
           SystemTable,
           DriverBinding,
           DriverBindingHandle,
           NULL,
           ComponentName2,
           NULL,
           DriverConfiguration2,
           NULL,
           DriverDiagnostics2
           );
}

EFI_STATUS
EfiLibTestManagedDevice (
  IN EFI_HANDLE       ControllerHandle,
  IN EFI_HANDLE       DriverBindingHandle,
  IN EFI_GUID         *ManagedProtocolGuid
  )
/*++

Routine Description:

  Test to see if the controller is managed by a specific driver.

Arguments:

  ControllerHandle          - Handle for controller to test

  DriverBindingHandle       - Driver binding handle for controller

  ManagedProtocolGuid       - The protocol guid the driver opens on controller

Returns: 

  EFI_SUCCESS     - The controller is managed by the driver

  EFI_UNSUPPORTED - The controller is not managed by the driver

--*/
{
  EFI_STATUS     Status;
  VOID           *ManagedInterface;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  ManagedProtocolGuid,
                  &ManagedInterface,
                  DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           ControllerHandle,
           ManagedProtocolGuid,
           DriverBindingHandle,
           ControllerHandle
           );
    return EFI_UNSUPPORTED;
  }

  if (Status != EFI_ALREADY_STARTED) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EfiLibTestChildHandle (
  IN EFI_HANDLE       ControllerHandle,
  IN EFI_HANDLE       ChildHandle,
  IN EFI_GUID         *ConsumedGuid
  )
/*++

Routine Description:

  Test to see if the child handle is the child of the controller

Arguments:

  ControllerHandle          - Handle for controller (parent)

  ChildHandle               - Child handle to test

  ConsumsedGuid             - Protocol guid consumed by child from controller

Returns: 

  EFI_SUCCESS     - The child handle is the child of the controller

  EFI_UNSUPPORTED - The child handle is not the child of the controller

--*/
{
  EFI_STATUS                            Status;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY   *OpenInfoBuffer;
  UINTN                                 EntryCount;
  UINTN                                 Index;

  //
  // Retrieve the list of agents that are consuming one of the protocols
  // on ControllerHandle that the children consume
  //
  Status = gBS->OpenProtocolInformation (
                  ControllerHandle,
                  ConsumedGuid,
                  &OpenInfoBuffer,
                  &EntryCount
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // See if one of the agents is ChildHandle
  //
  Status = EFI_UNSUPPORTED;
  for (Index = 0; Index < EntryCount; Index++) {
    if (OpenInfoBuffer[Index].ControllerHandle == ChildHandle &&
        OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) {
      Status = EFI_SUCCESS;
    }
  }
  gBS->FreePool (OpenInfoBuffer);
  return Status;
}
