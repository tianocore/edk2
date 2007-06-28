/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PciDeviceSupport.c

Abstract:

  This file provides routine to support Pci device node manipulation

Revision History

--*/

#include "pcibus.h"
#include "PciDeviceSupport.h"

//
// This device structure is serviced as a header.
// Its Next field points to the first root bridge device node
//
LIST_ENTRY  gPciDevicePool;

EFI_STATUS
InitializePciDevicePool (
  VOID
  )
/*++

Routine Description:

  Initialize the gPciDevicePool

Arguments:

Returns:

  None

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
{
  InitializeListHead (&gPciDevicePool);

  return EFI_SUCCESS;
}

EFI_STATUS
InsertRootBridge (
  PCI_IO_DEVICE *RootBridge
  )
/*++

Routine Description:

  Insert a root bridge into PCI device pool

Arguments:

  RootBridge    - A pointer to the PCI_IO_DEVICE.

Returns:

  None

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
{

  InsertTailList (&gPciDevicePool, &(RootBridge->Link));

  return EFI_SUCCESS;
}

EFI_STATUS
InsertPciDevice (
  PCI_IO_DEVICE *Bridge,
  PCI_IO_DEVICE *PciDeviceNode
  )
/*++

Routine Description:

  This function is used to insert a PCI device node under
  a bridge

Arguments:
  Bridge        - A pointer to the PCI_IO_DEVICE.
  PciDeviceNode - A pointer to the PCI_IO_DEVICE.

Returns:

  None

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
{

  InsertTailList (&Bridge->ChildList, &(PciDeviceNode->Link));
  PciDeviceNode->Parent = Bridge;

  return EFI_SUCCESS;
}

EFI_STATUS
DestroyRootBridge (
  IN PCI_IO_DEVICE *RootBridge
  )
/*++

Routine Description:


Arguments:

  RootBridge   - A pointer to the PCI_IO_DEVICE.

Returns:

  None

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
{
  DestroyPciDeviceTree (RootBridge);

  FreePciDevice (RootBridge);

  return EFI_SUCCESS;
}

EFI_STATUS
FreePciDevice (
  IN PCI_IO_DEVICE *PciIoDevice
  )
/*++

Routine Description:

  Destroy a pci device node.
  Also all direct or indirect allocated resource for this node will be freed.

Arguments:

  PciIoDevice   - A pointer to the PCI_IO_DEVICE.

Returns:

  None

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
{

  //
  // Assume all children have been removed underneath this device
  //
  if (PciIoDevice->ResourcePaddingDescriptors != NULL) {
    gBS->FreePool (PciIoDevice->ResourcePaddingDescriptors);
  }

  if (PciIoDevice->DevicePath != NULL) {
    gBS->FreePool (PciIoDevice->DevicePath);
  }

  gBS->FreePool (PciIoDevice);

  return EFI_SUCCESS;
}

EFI_STATUS
DestroyPciDeviceTree (
  IN PCI_IO_DEVICE *Bridge
  )
/*++

Routine Description:

  Destroy all the pci device node under the bridge.
  Bridge itself is not included.

Arguments:

  Bridge   - A pointer to the PCI_IO_DEVICE.

Returns:

  None

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
{
  LIST_ENTRY      *CurrentLink;
  PCI_IO_DEVICE   *Temp;

  while (!IsListEmpty (&Bridge->ChildList)) {

    CurrentLink = Bridge->ChildList.ForwardLink;

    //
    // Remove this node from the linked list
    //
    RemoveEntryList (CurrentLink);

    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);

    if (!IsListEmpty (&Temp->ChildList)) {
      DestroyPciDeviceTree (Temp);
    }

    FreePciDevice (Temp);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DestroyRootBridgeByHandle (
  EFI_HANDLE Controller
  )
/*++

Routine Description:

  Destroy all device nodes under the root bridge
  specified by Controller.
  The root bridge itself is also included.

Arguments:

  Controller   - An efi handle.

Returns:

  None

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
{

  LIST_ENTRY      *CurrentLink;
  PCI_IO_DEVICE   *Temp;

  CurrentLink = gPciDevicePool.ForwardLink;

  while (CurrentLink && CurrentLink != &gPciDevicePool) {
    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);

    if (Temp->Handle == Controller) {

      RemoveEntryList (CurrentLink);

      DestroyPciDeviceTree (Temp);

      FreePciDevice (Temp);

      return EFI_SUCCESS;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
RegisterPciDevice (
  IN  EFI_HANDLE                     Controller,
  IN  PCI_IO_DEVICE                  *PciIoDevice,
  OUT EFI_HANDLE                     *Handle OPTIONAL
  )
/*++

Routine Description:

  This function registers the PCI IO device. It creates a handle for this PCI IO device
  (if the handle does not exist), attaches appropriate protocols onto the handle, does
  necessary initialization, and sets up parent/child relationship with its bus controller.

Arguments:

  Controller    - An EFI handle for the PCI bus controller.
  PciIoDevice   - A PCI_IO_DEVICE pointer to the PCI IO device to be registered.
  Handle        - A pointer to hold the EFI handle for the PCI IO device.

Returns:

  EFI_SUCCESS   - The PCI device is successfully registered.
  Others        - An error occurred when registering the PCI device.

--*/
{
  EFI_STATUS          Status;
  VOID                *PlatformOpRomBuffer;
  UINTN               PlatformOpRomSize;
  UINT8               PciExpressCapRegOffset;
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT8               Data8;

  //
  // Install the pciio protocol, device path protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &PciIoDevice->Handle,
                  &gEfiDevicePathProtocolGuid,
                  PciIoDevice->DevicePath,
                  &gEfiPciIoProtocolGuid,
                  &PciIoDevice->PciIo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Detect if PCI Express Device
  //
  PciExpressCapRegOffset = 0;
  Status = LocateCapabilityRegBlock (
             PciIoDevice,
             EFI_PCI_CAPABILITY_ID_PCIEXP,
             &PciExpressCapRegOffset,
             NULL
             );
  if (!EFI_ERROR (Status)) {
    PciIoDevice->IsPciExp = TRUE;
  }
  
  //
  // Force Interrupt line to "Unknown" or "No Connection"
  //
  PciIo = &(PciIoDevice->PciIo);
  Data8 = PCI_INT_LINE_UNKNOWN;
  PciIoWrite (PciIo, EfiPciIoWidthUint8, 0x3C, 1, &Data8);

  //
  // Process Platform OpRom
  //
  if (gPciPlatformProtocol != NULL && !PciIoDevice->AllOpRomProcessed) {
    PciIoDevice->AllOpRomProcessed = TRUE;

    Status = gPciPlatformProtocol->GetPciRom (
                                     gPciPlatformProtocol,
                                     PciIoDevice->Handle,
                                     &PlatformOpRomBuffer,
                                     &PlatformOpRomSize
                                     );

    if (!EFI_ERROR (Status)) {

      //
      // Have Platform OpRom
      //
      PciIoDevice->RomSize        = PlatformOpRomSize;
      PciIoDevice->PciIo.RomSize  = PlatformOpRomSize;
      PciIoDevice->PciIo.RomImage = PlatformOpRomBuffer;

      //
      // Process Image
      //
      ProcessOpRomImage (PciIoDevice);
    }
  }

  if (PciIoDevice->BusOverride) {
    //
    // Install BusSpecificDriverOverride Protocol
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &PciIoDevice->Handle,
                    &gEfiBusSpecificDriverOverrideProtocolGuid,
                    &PciIoDevice->PciDriverOverride,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      gBS->UninstallMultipleProtocolInterfaces (
             &PciIoDevice->Handle,
             &gEfiDevicePathProtocolGuid,
             PciIoDevice->DevicePath,
             &gEfiPciIoProtocolGuid,
             &PciIoDevice->PciIo,
             NULL
             );

      return Status;
    }
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **) &(PciIoDevice->PciRootBridgeIo),
                  gPciBusDriverBinding.DriverBindingHandle,
                  PciIoDevice->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Pccard Hotplug GUID for Pccard device so that
  // to notify CardBus driver to stop the device when de-register happens
  //
  InstallPciHotplugGuid (PciIoDevice);

  if (Handle != NULL) {
    *Handle = PciIoDevice->Handle;
  }

  //
  // Indicate the pci device is registered
  //
  PciIoDevice->Registered = TRUE;

  return EFI_SUCCESS;
}

EFI_STATUS
RemoveAllPciDeviceOnBridge (
  EFI_HANDLE               RootBridgeHandle,
  PCI_IO_DEVICE            *Bridge
  )
/*++

Routine Description:

  This function is used to remove the whole PCI devices from the bridge.

Arguments:

  RootBridgeHandle   - An efi handle.
  Bridge             - A pointer to the PCI_IO_DEVICE.

Returns:

  None

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
{

  LIST_ENTRY      *CurrentLink;
  PCI_IO_DEVICE   *Temp;

  while (!IsListEmpty (&Bridge->ChildList)) {

    CurrentLink = Bridge->ChildList.ForwardLink;
    Temp        = PCI_IO_DEVICE_FROM_LINK (CurrentLink);

    //
    // Check if the current node has been deregistered before
    // If it is not, then deregister it
    //
    if (Temp->Registered) {
      DeRegisterPciDevice (RootBridgeHandle, Temp->Handle);
    }

    //
    // Remove this node from the linked list
    //
    RemoveEntryList (CurrentLink);

    if (!IsListEmpty (&Temp->ChildList)) {
      RemoveAllPciDeviceOnBridge (RootBridgeHandle, Temp);
    }

    FreePciDevice (Temp);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DeRegisterPciDevice (
  IN  EFI_HANDLE                     Controller,
  IN  EFI_HANDLE                     Handle
  )
/*++

Routine Description:

  This function is used to de-register the PCI device from the EFI,
  That includes un-installing PciIo protocol from the specified PCI
  device handle.

Arguments:

  Controller   - An efi handle.
  Handle       - An efi handle.

Returns:

  None

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_PCI_IO_PROTOCOL             *PciIo;
  EFI_STATUS                      Status;
  PCI_IO_DEVICE                   *PciIoDevice;
  PCI_IO_DEVICE                   *Node;
  LIST_ENTRY                      *CurrentLink;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  gPciBusDriverBinding.DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (PciIo);

    //
    // If it is already de-registered
    //
    if (!PciIoDevice->Registered) {
      return EFI_SUCCESS;
    }

    //
    // If it is PPB, first de-register its children
    //

    if (!IsListEmpty (&PciIoDevice->ChildList)) {

      CurrentLink = PciIoDevice->ChildList.ForwardLink;

      while (CurrentLink && CurrentLink != &PciIoDevice->ChildList) {
        Node    = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
        Status  = DeRegisterPciDevice (Controller, Node->Handle);

        if (EFI_ERROR (Status)) {
          return Status;
        }

        CurrentLink = CurrentLink->ForwardLink;
      }
    }
    //
    // Uninstall Pccard Hotplug GUID for Pccard device
    //
    UninstallPciHotplugGuid (PciIoDevice);

    //
    // Close the child handle
    //
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiPciRootBridgeIoProtocolGuid,
                    gPciBusDriverBinding.DriverBindingHandle,
                    Handle
                    );

    //
    // Un-install the device path protocol and pci io protocol
    //
    if (PciIoDevice->BusOverride) {
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      Handle,
                      &gEfiDevicePathProtocolGuid,
                      PciIoDevice->DevicePath,
                      &gEfiPciIoProtocolGuid,
                      &PciIoDevice->PciIo,
                      &gEfiBusSpecificDriverOverrideProtocolGuid,
                      &PciIoDevice->PciDriverOverride,
                      NULL
                      );
    } else {
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      Handle,
                      &gEfiDevicePathProtocolGuid,
                      PciIoDevice->DevicePath,
                      &gEfiPciIoProtocolGuid,
                      &PciIoDevice->PciIo,
                      NULL
                      );
    }

    if (EFI_ERROR (Status)) {
      gBS->OpenProtocol (
            Controller,
            &gEfiPciRootBridgeIoProtocolGuid,
            (VOID **) &PciRootBridgeIo,
            gPciBusDriverBinding.DriverBindingHandle,
            Handle,
            EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
            );
      return Status;
    }

    //
    // The Device Driver should disable this device after disconnect
    // so the Pci Bus driver will not touch this device any more.
    // Restore the register field to the original value
    //
    PciIoDevice->Registered = FALSE;
    PciIoDevice->Handle     = NULL;
  } else {

    //
    // Handle may be closed before
    //
    return EFI_SUCCESS;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
StartPciDevicesOnBridge (
  IN EFI_HANDLE                          Controller,
  IN PCI_IO_DEVICE                       *RootBridge,
  IN EFI_DEVICE_PATH_PROTOCOL            *RemainingDevicePath,
  IN OUT UINT8                           *NumberOfChildren,
  IN OUT EFI_HANDLE                      *ChildHandleBuffer
  )
/*++

Routine Description:

  Start to manage the PCI device on specified the root bridge or PCI-PCI Bridge

Arguments:

  Controller          - An efi handle.
  RootBridge          - A pointer to the PCI_IO_DEVICE.
  RemainingDevicePath - A pointer to the EFI_DEVICE_PATH_PROTOCOL.
  NumberOfChildren    - Children number.
  ChildHandleBuffer   - A pointer to the child handle buffer.

Returns:

  None

--*/
// TODO:    EFI_NOT_READY - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
{
  PCI_IO_DEVICE             *Temp;
  PCI_IO_DEVICE             *PciIoDevice;
  EFI_DEV_PATH_PTR          Node;
  EFI_DEVICE_PATH_PROTOCOL  *CurrentDevicePath;
  EFI_STATUS                Status;
  LIST_ENTRY                *CurrentLink;
  UINT64                    Supports;

  CurrentLink = RootBridge->ChildList.ForwardLink;

  while (CurrentLink && CurrentLink != &RootBridge->ChildList) {

    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
    if (RemainingDevicePath != NULL) {

      Node.DevPath = RemainingDevicePath;

      if (Node.Pci->Device != Temp->DeviceNumber ||
          Node.Pci->Function != Temp->FunctionNumber) {
        CurrentLink = CurrentLink->ForwardLink;
        continue;
      }

      //
      // Check if the device has been assigned with required resource
      //
      if (!Temp->Allocated) {
        return EFI_NOT_READY;
      }

      //
      // Check if the current node has been registered before
      // If it is not, register it
      //
      if (!Temp->Registered) {
        PciIoDevice = Temp;

        Status = RegisterPciDevice (
                  Controller,
                  PciIoDevice,
                  NULL
                  );

      }

      if (NumberOfChildren != NULL && ChildHandleBuffer != NULL && Temp->Registered) {
        ChildHandleBuffer[*NumberOfChildren] = Temp->Handle;
        (*NumberOfChildren)++;
      }

      //
      // Get the next device path
      //
      CurrentDevicePath = EfiNextDevicePathNode (RemainingDevicePath);
      if (EfiIsDevicePathEnd (CurrentDevicePath)) {
        return EFI_SUCCESS;
      }

      //
      // If it is a PPB
      //
      if (!IsListEmpty (&Temp->ChildList)) {
        Status = StartPciDevicesOnBridge (
                  Controller,
                  Temp,
                  CurrentDevicePath,
                  NumberOfChildren,
                  ChildHandleBuffer
                  );

        Temp->PciIo.Attributes (
                      &(Temp->PciIo),
                      EfiPciIoAttributeOperationSupported,
                      0,
                      &Supports
                      );
        Supports &= EFI_PCI_DEVICE_ENABLE;
        Temp->PciIo.Attributes (
                      &(Temp->PciIo),
                      EfiPciIoAttributeOperationEnable,
                      Supports,
                      NULL
                      );

        return Status;
      } else {

        //
        // Currently, the PCI bus driver only support PCI-PCI bridge
        //
        return EFI_UNSUPPORTED;
      }

    } else {

      //
      // If remaining device path is NULL,
      // try to enable all the pci devices under this bridge
      //

      if (!Temp->Registered && Temp->Allocated) {

        PciIoDevice = Temp;

        Status = RegisterPciDevice (
                  Controller,
                  PciIoDevice,
                  NULL
                  );

      }

      if (NumberOfChildren != NULL && ChildHandleBuffer != NULL && Temp->Registered) {
        ChildHandleBuffer[*NumberOfChildren] = Temp->Handle;
        (*NumberOfChildren)++;
      }

      if (!IsListEmpty (&Temp->ChildList)) {
        Status = StartPciDevicesOnBridge (
                  Controller,
                  Temp,
                  RemainingDevicePath,
                  NumberOfChildren,
                  ChildHandleBuffer
                  );

        Temp->PciIo.Attributes (
                      &(Temp->PciIo),
                      EfiPciIoAttributeOperationSupported,
                      0,
                      &Supports
                      );
        Supports &= EFI_PCI_DEVICE_ENABLE;
        Temp->PciIo.Attributes (
                      &(Temp->PciIo),
                      EfiPciIoAttributeOperationEnable,
                      Supports,
                      NULL
                      );

      }

      CurrentLink = CurrentLink->ForwardLink;
      continue;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
StartPciDevices (
  IN EFI_HANDLE                         Controller,
  IN EFI_DEVICE_PATH_PROTOCOL           *RemainingDevicePath
  )
/*++

Routine Description:

  Start to manage the PCI device according to RemainingDevicePath
  If RemainingDevicePath == NULL, the PCI bus driver will start
  to manage all the PCI devices it found previously

Arguments:
  Controller          - An efi handle.
  RemainingDevicePath - A pointer to the EFI_DEVICE_PATH_PROTOCOL.

Returns:

  None

--*/
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_DEV_PATH_PTR  Node;
  PCI_IO_DEVICE     *RootBridge;
  LIST_ENTRY        *CurrentLink;

  if (RemainingDevicePath != NULL) {

    //
    // Check if the RemainingDevicePath is valid
    //
    Node.DevPath = RemainingDevicePath;
    if ((Node.DevPath->Type != HARDWARE_DEVICE_PATH) ||
        ((Node.DevPath->SubType != HW_PCI_DP)         &&
         (DevicePathNodeLength (Node.DevPath) != sizeof (PCI_DEVICE_PATH)))
        ) {
      return EFI_UNSUPPORTED;
    }
  }

  CurrentLink = gPciDevicePool.ForwardLink;

  while (CurrentLink && CurrentLink != &gPciDevicePool) {

    RootBridge = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
    //
    // Locate the right root bridge to start
    //
    if (RootBridge->Handle == Controller) {
      StartPciDevicesOnBridge (
        Controller,
        RootBridge,
        RemainingDevicePath,
        NULL,
        NULL
        );
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_SUCCESS;
}

PCI_IO_DEVICE *
CreateRootBridge (
  IN EFI_HANDLE RootBridgeHandle
  )
/*++

Routine Description:


Arguments:
  RootBridgeHandle   - An efi handle.

Returns:

  None

--*/
{

  EFI_STATUS                      Status;
  PCI_IO_DEVICE                   *Dev;
  EFI_DEVICE_PATH_PROTOCOL        *ParentDevicePath;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;

  Dev = NULL;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (PCI_IO_DEVICE),
                  (VOID **) &Dev
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ZeroMem (Dev, sizeof (PCI_IO_DEVICE));
  Dev->Signature  = PCI_IO_DEVICE_SIGNATURE;
  Dev->Handle     = RootBridgeHandle;
  InitializeListHead (&Dev->ChildList);

  Status = gBS->OpenProtocol (
                  RootBridgeHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  gPciBusDriverBinding.DriverBindingHandle,
                  RootBridgeHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (Dev);
    return NULL;
  }

  //
  // Record the root bridge parent device path
  //
  Dev->DevicePath = DuplicateDevicePath (ParentDevicePath);

  //
  // Get the pci root bridge io protocol
  //
  Status = gBS->OpenProtocol (
                  RootBridgeHandle,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **) &PciRootBridgeIo,
                  gPciBusDriverBinding.DriverBindingHandle,
                  RootBridgeHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    FreePciDevice (Dev);
    return NULL;
  }

  Dev->PciRootBridgeIo = PciRootBridgeIo;

  //
  // Initialize the PCI I/O instance structure
  //
  Status  = InitializePciIoInstance (Dev);
  Status  = InitializePciDriverOverrideInstance (Dev);

  //
  // Initialize reserved resource list and
  // option rom driver list
  //
  InitializeListHead (&Dev->ReservedResourceList);
  InitializeListHead (&Dev->OptionRomDriverList);

  return Dev;
}

PCI_IO_DEVICE *
GetRootBridgeByHandle (
  EFI_HANDLE RootBridgeHandle
  )
/*++

Routine Description:


Arguments:

  RootBridgeHandle    - An efi handle.

Returns:

  None

--*/
{
  PCI_IO_DEVICE   *RootBridgeDev;
  LIST_ENTRY      *CurrentLink;

  CurrentLink = gPciDevicePool.ForwardLink;

  while (CurrentLink && CurrentLink != &gPciDevicePool) {

    RootBridgeDev = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
    if (RootBridgeDev->Handle == RootBridgeHandle) {
      return RootBridgeDev;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return NULL;
}

BOOLEAN
RootBridgeExisted (
  IN EFI_HANDLE RootBridgeHandle
  )
/*++

Routine Description:

  This function searches if RootBridgeHandle has already existed
  in current device pool.

  If so, it means the given root bridge has been already enumerated.

Arguments:

  RootBridgeHandle   - An efi handle.

Returns:

  None

--*/
{
  PCI_IO_DEVICE *Bridge;

  Bridge = GetRootBridgeByHandle (RootBridgeHandle);

  if (Bridge != NULL) {
    return TRUE;
  }

  return FALSE;
}

BOOLEAN
PciDeviceExisted (
  IN PCI_IO_DEVICE    *Bridge,
  IN PCI_IO_DEVICE    *PciIoDevice
  )
/*++

Routine Description:

Arguments:

  Bridge       - A pointer to the PCI_IO_DEVICE.
  PciIoDevice  - A pointer to the PCI_IO_DEVICE.

Returns:

  None

--*/
{

  PCI_IO_DEVICE   *Temp;
  LIST_ENTRY      *CurrentLink;

  CurrentLink = Bridge->ChildList.ForwardLink;

  while (CurrentLink && CurrentLink != &Bridge->ChildList) {

    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);

    if (Temp == PciIoDevice) {
      return TRUE;
    }

    if (!IsListEmpty (&Temp->ChildList)) {
      if (PciDeviceExisted (Temp, PciIoDevice)) {
        return TRUE;
      }
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return FALSE;
}

PCI_IO_DEVICE *
ActiveVGADeviceOnTheSameSegment (
  IN PCI_IO_DEVICE        *VgaDevice
  )
/*++

Routine Description:

Arguments:

  VgaDevice    - A pointer to the PCI_IO_DEVICE.

Returns:

  None

--*/
{
  LIST_ENTRY      *CurrentLink;
  PCI_IO_DEVICE   *Temp;

  CurrentLink = gPciDevicePool.ForwardLink;

  while (CurrentLink && CurrentLink != &gPciDevicePool) {

    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);

    if (Temp->PciRootBridgeIo->SegmentNumber == VgaDevice->PciRootBridgeIo->SegmentNumber) {

      Temp = ActiveVGADeviceOnTheRootBridge (Temp);

      if (Temp != NULL) {
        return Temp;
      }
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return NULL;
}

PCI_IO_DEVICE *
ActiveVGADeviceOnTheRootBridge (
  IN PCI_IO_DEVICE        *RootBridge
  )
/*++

Routine Description:

Arguments:

  RootBridge    - A pointer to the PCI_IO_DEVICE.

Returns:

  None

--*/
{
  LIST_ENTRY      *CurrentLink;
  PCI_IO_DEVICE   *Temp;

  CurrentLink = RootBridge->ChildList.ForwardLink;

  while (CurrentLink && CurrentLink != &RootBridge->ChildList) {

    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);

    if (IS_PCI_VGA(&Temp->Pci) &&
        (Temp->Attributes &
         (EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY |
          EFI_PCI_IO_ATTRIBUTE_VGA_IO     |
          EFI_PCI_IO_ATTRIBUTE_VGA_IO_16))) {
      return Temp;
    }

    if (IS_PCI_BRIDGE (&Temp->Pci)) {

      Temp = ActiveVGADeviceOnTheRootBridge (Temp);

      if (Temp != NULL) {
        return Temp;
      }
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return NULL;
}

EFI_STATUS
GetHpcPciAddress (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN  EFI_DEVICE_PATH_PROTOCOL         *HpcDevicePath,
  OUT UINT64                           *PciAddress
  )
/*++

Routine Description:

Arguments:

  PciRootBridgeIo       - A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  HpcDevicePath         - A pointer to the EFI_DEVICE_PATH_PROTOCL.
  PciAddress            - A pointer to the pci address.

Returns:

  None

--*/
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
{
  EFI_DEVICE_PATH_PROTOCOL  *CurrentDevicePath;
  EFI_DEV_PATH_PTR          Node;
  LIST_ENTRY                *CurrentLink;
  PCI_IO_DEVICE             *RootBridge;
  EFI_STATUS                Status;

  CurrentDevicePath = HpcDevicePath;

  //
  // Get the remaining device path for this PCI device, if it is a PCI device
  //
  while (!EfiIsDevicePathEnd (CurrentDevicePath)) {

    Node.DevPath = CurrentDevicePath;

    //
    // Check if it is PCI device Path?
    //
    if ((Node.DevPath->Type != HARDWARE_DEVICE_PATH) ||
        ((Node.DevPath->SubType != HW_PCI_DP)         &&
         (DevicePathNodeLength (Node.DevPath) != sizeof (PCI_DEVICE_PATH)))) {
      CurrentDevicePath = EfiNextDevicePathNode (CurrentDevicePath);
      continue;
    }

    break;
  }

  //
  // Check if it is not PCI device path
  //
  if (EfiIsDevicePathEnd (CurrentDevicePath)) {
    return EFI_NOT_FOUND;
  }

  CurrentLink = gPciDevicePool.ForwardLink;

  while (CurrentLink && CurrentLink != &gPciDevicePool) {

    RootBridge = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
    //
    // Locate the right root bridge to start
    //
    if (RootBridge->PciRootBridgeIo == PciRootBridgeIo) {
      Status = GetHpcPciAddressFromRootBridge (
                RootBridge,
                CurrentDevicePath,
                PciAddress
                );
      if (EFI_ERROR (Status)) {
        return EFI_NOT_FOUND;
      }

      return EFI_SUCCESS;

    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
GetHpcPciAddressFromRootBridge (
  IN  PCI_IO_DEVICE                    *RootBridge,
  IN  EFI_DEVICE_PATH_PROTOCOL         *RemainingDevicePath,
  OUT UINT64                           *PciAddress
  )
/*++

Routine Description:

Arguments:

  PciRootBridgeIo       - A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  HpcDevicePath         - A pointer to the EFI_DEVICE_PATH_PROTOCL.
  PciAddress            - A pointer to the pci address.

Returns:

  None

--*/
// TODO:    RootBridge - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_DEV_PATH_PTR          Node;
  PCI_IO_DEVICE             *Temp;
  EFI_DEVICE_PATH_PROTOCOL  *CurrentDevicePath;
  LIST_ENTRY                *CurrentLink;
  BOOLEAN                   MisMatch;

  MisMatch          = FALSE;

  CurrentDevicePath = RemainingDevicePath;
  Node.DevPath      = CurrentDevicePath;
  Temp              = NULL;

  while (!EfiIsDevicePathEnd (CurrentDevicePath)) {

    CurrentLink   = RootBridge->ChildList.ForwardLink;
    Node.DevPath  = CurrentDevicePath;

    while (CurrentLink && CurrentLink != &RootBridge->ChildList) {
      Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);

      if (Node.Pci->Device   == Temp->DeviceNumber &&
          Node.Pci->Function == Temp->FunctionNumber) {
        RootBridge = Temp;
        break;
      }

      CurrentLink = CurrentLink->ForwardLink;
    }

    //
    // Check if we find the bridge
    //
    if (CurrentLink == &RootBridge->ChildList) {

      MisMatch = TRUE;
      break;

    }

    CurrentDevicePath = EfiNextDevicePathNode (CurrentDevicePath);
  }

  if (MisMatch) {

    CurrentDevicePath = EfiNextDevicePathNode (CurrentDevicePath);

    if (EfiIsDevicePathEnd (CurrentDevicePath)) {
      *PciAddress = EFI_PCI_ADDRESS (RootBridge->BusNumber, Node.Pci->Device, Node.Pci->Function, 0);
      return EFI_SUCCESS;
    }

    return EFI_NOT_FOUND;
  }

  *PciAddress = EFI_PCI_ADDRESS (Temp->BusNumber, Temp->DeviceNumber, Temp->FunctionNumber, 0);

  return EFI_SUCCESS;

}
