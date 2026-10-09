/** @file
  UEFI Driver Binding callbacks for the UFDT dispatcher.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UfdtDispatcherInternal.h"

#include <Library/UfdtDispatcherLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

/** Check whether a driver matches an available device-tree node.

  Match compatible strings and check that the node and its ancestors are enabled.

  @param[in] Driver  Driver with a NULL-terminated compatible list.
  @param[in] Device  Non-NULL parsed node to match.

  @retval TRUE   Availability and compatible checks pass.
  @retval FALSE  The node is unavailable or no compatible string matches.
**/
BOOLEAN
EFIAPI
UfdtDriverIsCompatible (
  IN     UFDT_BINDING_DRIVER  *Driver,
  IN CONST UFDT_DEVICE_NODE   *Device
  )
{
  UINTN  Index;

  if (!UfdtDeviceNodeIsAvailable (Device)) {
    DEBUG ((DEBUG_INFO, "UfdtDriver: Node %a present but not available.\n", Device->Path));
    return FALSE;
  }

  // Check whether a driver compatible string appears in the node's list.
  for (Index = 0; Driver->Compatible[Index] != NULL; Index++) {
    if (UfdtNodeIsCompatible (Device, Driver->Compatible[Index])) {
      return TRUE;
    }
  }

  return FALSE;
}

/** Test whether this binding can claim a matching FDT node.

  @param[in] This                 Binding embedded in UFDT_BINDING_DRIVER.
  @param[in] ControllerHandle     Controller carrying the FDT node protocol.
  @param[in] RemainingDevicePath  Unused; discovery always considers the whole controller.

  @retval EFI_SUCCESS          Success.
  @retval EFI_UNSUPPORTED      The controller is unsupported.
  @retval EFI_ALREADY_STARTED  This binding already owns the node protocol.
  @retval Other                Other OpenProtocol() failures, including ownership by another driver.
**/
EFI_STATUS
EFIAPI
UfdtBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  UFDT_BINDING_DRIVER  *Driver;
  UFDT_DEVICE_NODE     *Device;
  EFI_STATUS           Status;

  Driver = BASE_CR (This, UFDT_BINDING_DRIVER, Binding);
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEdkiiUfdtNodeProtocolGuid,
                  (VOID **)&Device,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (ControllerHandle, &gEdkiiUfdtNodeProtocolGuid, This->DriverBindingHandle, ControllerHandle);

  if (!UfdtDriverIsCompatible (Driver, Device)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/** Claim a node and run the binding's configuration callback.

  @param[in] This                 Registered binding with a Configure callback.
  @param[in] ControllerHandle     Handle carrying the node protocol to claim.
  @param[in] RemainingDevicePath  Ignored; configuration covers the whole node.

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_ALREADY_STARTED   This binding already owns the node protocol.
  @retval EFI_NOT_READY         The callback requires an unavailable provider.
  @retval EFI_UNSUPPORTED       The node protocol is absent or the callback
                                does not support this device.
  @retval Other                 Protocol-open or configuration-callback errors.
**/
EFI_STATUS
EFIAPI
UfdtBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  UFDT_BINDING_DRIVER  *Driver;
  UFDT_DEVICE_NODE     *Device;
  EFI_STATUS           Status;
  UFDT_BINDING_DEVICE  *BindingDevice;

  Driver = BASE_CR (This, UFDT_BINDING_DRIVER, Binding);
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEdkiiUfdtNodeProtocolGuid,
                  (VOID **)&Device,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BindingDevice = AllocateZeroPool (sizeof (*BindingDevice));
  if (BindingDevice == NULL) {
    gBS->CloseProtocol (ControllerHandle, &gEdkiiUfdtNodeProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  BindingDevice->Device = Device;
  BindingDevice->Handle = ControllerHandle;
  Status                = Driver->Configure (BindingDevice);
  if (EFI_ERROR (Status)) {
    FreePool (BindingDevice);
    DEBUG ((DEBUG_ERROR, "UfdtDriver: %a failed on %a: %r\n", Driver->Name, Device->Path, Status));
    gBS->CloseProtocol (ControllerHandle, &gEdkiiUfdtNodeProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return Status;
  }

  InsertTailList (&Driver->Devices, &BindingDevice->Link);
  DEBUG ((DEBUG_INFO, "UfdtDriver: %a Start %a: %r\n", Driver->Name, Device->Path, Status));
  return EFI_SUCCESS;
}

/** Refuse removal of resource bindings.

  @param[in] This               Binding embedded in UFDT_BINDING_DRIVER.
  @param[in] ControllerHandle   Controller requested for disconnection.
  @param[in] NumberOfChildren   Number of requested child handles.
  @param[in] ChildHandleBuffer  Optional child handle array.

  @retval EFI_ACCESS_DENIED  Successful bindings remain until firmware exit.
**/
EFI_STATUS
EFIAPI
UfdtBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  return EFI_ACCESS_DENIED;
}
