/** @file
  Supporting functions declaration for PCI devices management.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_PCI_DEVICE_SUPPORT_H_
#define _EFI_PCI_DEVICE_SUPPORT_H_

/**
  Initialize the PCI devices pool.

**/
VOID
InitializePciDevicePool (
  VOID
  );

/**
  Insert a root bridge into PCI device pool.

  @param RootBridge     A pointer to the PCI_IO_DEVICE.

**/
VOID
InsertRootBridge (
  IN PCI_IO_DEVICE  *RootBridge
  );

/**
  This function is used to insert a PCI device node under
  a bridge.

  @param Bridge         The PCI bridge.
  @param PciDeviceNode  The PCI device needs inserting.

**/
VOID
InsertPciDevice (
  IN PCI_IO_DEVICE  *Bridge,
  IN PCI_IO_DEVICE  *PciDeviceNode
  );

/**
  Destroy root bridge and remove it from device tree.

  @param RootBridge     The bridge want to be removed.

**/
VOID
DestroyRootBridge (
  IN PCI_IO_DEVICE  *RootBridge
  );

/**
  Destroy all the pci device node under the bridge.
  Bridge itself is not included.

  @param Bridge         A pointer to the PCI_IO_DEVICE.

**/
VOID
DestroyPciDeviceTree (
  IN PCI_IO_DEVICE  *Bridge
  );

/**
  Destroy all device nodes under the root bridge
  specified by Controller.

  The root bridge itself is also included.

  @param  Controller    Root bridge handle.

  @retval EFI_SUCCESS   Destroy all device nodes successfully.
  @retval EFI_NOT_FOUND Cannot find any PCI device under specified
                        root bridge.

**/
EFI_STATUS
DestroyRootBridgeByHandle (
  IN EFI_HANDLE  Controller
  );

/**
  This function registers the PCI IO device.

  It creates a handle for this PCI IO device (if the handle does not exist), attaches
  appropriate protocols onto the handle, does necessary initialization, and sets up
  parent/child relationship with its bus controller.

  @param Controller     An EFI handle for the PCI bus controller.
  @param PciIoDevice    A PCI_IO_DEVICE pointer to the PCI IO device to be registered.
  @param Handle         A pointer to hold the returned EFI handle for the PCI IO device.

  @retval EFI_SUCCESS   The PCI device is successfully registered.
  @retval other         An error occurred when registering the PCI device.

**/
EFI_STATUS
RegisterPciDevice (
  IN  EFI_HANDLE     Controller,
  IN  PCI_IO_DEVICE  *PciIoDevice,
  OUT EFI_HANDLE     *Handle      OPTIONAL
  );

/**
  This function is used to remove the whole PCI devices on the specified bridge from
  the root bridge.

  @param RootBridgeHandle   The root bridge device handle.
  @param Bridge             The bridge device to be removed.

**/
VOID
RemoveAllPciDeviceOnBridge (
  EFI_HANDLE     RootBridgeHandle,
  PCI_IO_DEVICE  *Bridge
  );

/**
  This function is used to de-register the PCI IO device.

  That includes un-installing PciIo protocol from the specified PCI
  device handle.

  @param Controller    An EFI handle for the PCI bus controller.
  @param Handle        PCI device handle.

  @retval EFI_SUCCESS  The PCI device is successfully de-registered.
  @retval other        An error occurred when de-registering the PCI device.

**/
EFI_STATUS
DeRegisterPciDevice (
  IN  EFI_HANDLE  Controller,
  IN  EFI_HANDLE  Handle
  );

/**
  Start to manage the PCI device on the specified root bridge or PCI-PCI Bridge.

  @param Controller          The root bridge handle.
  @param RootBridge          A pointer to the PCI_IO_DEVICE.
  @param RemainingDevicePath A pointer to the EFI_DEVICE_PATH_PROTOCOL.
  @param NumberOfChildren    Children number.
  @param ChildHandleBuffer   A pointer to the child handle buffer.

  @retval EFI_NOT_READY   Device is not allocated.
  @retval EFI_UNSUPPORTED Device only support PCI-PCI bridge.
  @retval EFI_NOT_FOUND   Can not find the specific device.
  @retval EFI_SUCCESS     Success to start Pci devices on bridge.

**/
EFI_STATUS
StartPciDevicesOnBridge (
  IN EFI_HANDLE                Controller,
  IN PCI_IO_DEVICE             *RootBridge,
  IN EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath,
  IN OUT UINT8                 *NumberOfChildren,
  IN OUT EFI_HANDLE            *ChildHandleBuffer
  );

/**
  Start to manage all the PCI devices it found previously under
  the entire host bridge.

  @param Controller          The root bridge handle.

  @retval EFI_NOT_READY   Device is not allocated.
  @retval EFI_SUCCESS     Success to start Pci device on host bridge.

**/
EFI_STATUS
StartPciDevices (
  IN EFI_HANDLE  Controller
  );

/**
  Create root bridge device.

  @param RootBridgeHandle    Specified root bridge handle.

  @return The crated root bridge device instance, NULL means no
          root bridge device instance created.

**/
PCI_IO_DEVICE *
CreateRootBridge (
  IN EFI_HANDLE  RootBridgeHandle
  );

/**
  Get root bridge device instance by specific root bridge handle.

  @param RootBridgeHandle    Given root bridge handle.

  @return The root bridge device instance, NULL means no root bridge
          device instance found.

**/
PCI_IO_DEVICE *
GetRootBridgeByHandle (
  EFI_HANDLE  RootBridgeHandle
  );

/**
  Judge whether Pci device existed.

  @param Bridge       Parent bridge instance.
  @param PciIoDevice  Device instance.

  @retval TRUE        Pci device existed.
  @retval FALSE       Pci device did not exist.

**/
BOOLEAN
PciDeviceExisted (
  IN PCI_IO_DEVICE  *Bridge,
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  Get the active VGA device on the specified Host Bridge.

  @param HostBridgeHandle    Host Bridge handle.

  @return The active VGA device on the specified Host Bridge.

**/
PCI_IO_DEVICE *
LocateVgaDeviceOnHostBridge (
  IN EFI_HANDLE  HostBridgeHandle
  );

/**
  Locate the active VGA device under the bridge.

  @param Bridge  PCI IO instance for the bridge.

  @return The active VGA device.

**/
PCI_IO_DEVICE *
LocateVgaDevice (
  IN PCI_IO_DEVICE  *Bridge
  );

/**
  Destroy a pci device node.

  All direct or indirect allocated resource for this node will be freed.

  @param PciIoDevice  A pointer to the PCI_IO_DEVICE to be destroyed.

**/
VOID
FreePciDevice (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

#endif
