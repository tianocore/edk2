/** @file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _EFI_PCI_DEVICE_SUPPORT_H_
#define _EFI_PCI_DEVICE_SUPPORT_H_

/**
  Initialize the gPciDevicePool.
**/
EFI_STATUS
InitializePciDevicePool (
  VOID
  );

/**
  Insert a root bridge into PCI device pool

  @param RootBridge    - A pointer to the PCI_IO_DEVICE.

**/
EFI_STATUS
InsertRootBridge (
  PCI_IO_DEVICE *RootBridge
  );

/**
  This function is used to insert a PCI device node under
  a bridge

  @param Bridge         A pointer to the PCI_IO_DEVICE.
  @param PciDeviceNode  A pointer to the PCI_IO_DEVICE.

**/
EFI_STATUS
InsertPciDevice (
  PCI_IO_DEVICE *Bridge,
  PCI_IO_DEVICE *PciDeviceNode
  );

/**
  Destroy root bridge and remove it from deivce tree.
  
  @param RootBridge   The bridge want to be removed
  
**/
EFI_STATUS
DestroyRootBridge (
  IN PCI_IO_DEVICE *RootBridge
  );

/**
  Destroy all the pci device node under the bridge.
  Bridge itself is not included.

  @param Bridge   A pointer to the PCI_IO_DEVICE.

**/
EFI_STATUS
DestroyPciDeviceTree (
  IN PCI_IO_DEVICE *Bridge
  );

/**
  Destroy all device nodes under the root bridge
  specified by Controller.
  The root bridge itself is also included.

  @param Controller   An efi handle.

**/
EFI_STATUS
DestroyRootBridgeByHandle (
  EFI_HANDLE Controller
  );

/**
  This function registers the PCI IO device. It creates a handle for this PCI IO device
  (if the handle does not exist), attaches appropriate protocols onto the handle, does
  necessary initialization, and sets up parent/child relationship with its bus controller.

  @param Controller    - An EFI handle for the PCI bus controller.
  @param PciIoDevice   - A PCI_IO_DEVICE pointer to the PCI IO device to be registered.
  @param Handle        - A pointer to hold the EFI handle for the PCI IO device.

  @retval EFI_SUCCESS   - The PCI device is successfully registered.
  @retval Others        - An error occurred when registering the PCI device.

**/
EFI_STATUS
RegisterPciDevice (
  IN  EFI_HANDLE                     Controller,
  IN  PCI_IO_DEVICE                  *PciIoDevice,
  OUT EFI_HANDLE                     *Handle OPTIONAL
  );

/**
  This function is used to remove the whole PCI devices from the bridge.

  @param RootBridgeHandle   An efi handle.
  @param Bridge             A pointer to the PCI_IO_DEVICE.

  @retval EFI_SUCCESS
**/
EFI_STATUS
RemoveAllPciDeviceOnBridge (
  EFI_HANDLE               RootBridgeHandle,
  PCI_IO_DEVICE            *Bridge
  );

/**

  This function is used to de-register the PCI device from the EFI,
  That includes un-installing PciIo protocol from the specified PCI
  device handle.

  @param Controller   - controller handle
  @param Handle       - device handle

  @return Status of de-register pci device
**/
EFI_STATUS
DeRegisterPciDevice (
  IN  EFI_HANDLE                     Controller,
  IN  EFI_HANDLE                     Handle
  );

/**
  Start to manage the PCI device on specified the root bridge or PCI-PCI Bridge

  @param Controller          An efi handle.
  @param RootBridge          A pointer to the PCI_IO_DEVICE.
  @param RemainingDevicePath A pointer to the EFI_DEVICE_PATH_PROTOCOL.
  @param NumberOfChildren    Children number.
  @param ChildHandleBuffer   A pointer to the child handle buffer.

  @retval EFI_NOT_READY   Device is not allocated
  @retval EFI_UNSUPPORTED Device only support PCI-PCI bridge.
  @retval EFI_NOT_FOUND   Can not find the specific device
  @retval EFI_SUCCESS     Success to start Pci device on bridge

**/
EFI_STATUS
StartPciDevicesOnBridge (
  IN EFI_HANDLE                          Controller,
  IN PCI_IO_DEVICE                       *RootBridge,
  IN EFI_DEVICE_PATH_PROTOCOL            *RemainingDevicePath,
  IN OUT UINT8                           *NumberOfChildren,
  IN OUT EFI_HANDLE                      *ChildHandleBuffer
  );

/**
  Start to manage all the PCI devices it found previously under 
  the entire host bridge.

  @param Controller          - root bridge handle.

**/
EFI_STATUS
StartPciDevices (
  IN EFI_HANDLE                         Controller
  );

/**
  Create root bridge device

  @param RootBridgeHandle   - Parent bridge handle.

  @return pointer to new root bridge 
**/
PCI_IO_DEVICE *
CreateRootBridge (
  IN EFI_HANDLE RootBridgeHandle
  );

/**
  Get root bridge device instance by specific handle.

  @param RootBridgeHandle    Given root bridge handle.

  @return root bridge device instance.
**/
PCI_IO_DEVICE *
GetRootBridgeByHandle (
  EFI_HANDLE RootBridgeHandle
  );

/**
  Check root bridge device is existed or not.

  @param RootBridgeHandle    Given root bridge handle.

  @return root bridge device is existed or not.
**/
BOOLEAN
RootBridgeExisted (
  IN EFI_HANDLE RootBridgeHandle
  );

/**
  Judege whether Pci device existed.
  
  @param Bridge       Parent bridege instance. 
  @param PciIoDevice  Device instance.
  
  @return whether Pci device existed.
**/
BOOLEAN
PciDeviceExisted (
  IN PCI_IO_DEVICE    *Bridge,
  IN PCI_IO_DEVICE    *PciIoDevice
  );

/**
  Active VGA device.
  
  @param VgaDevice device instance for VGA.
  
  @return device instance.
**/
PCI_IO_DEVICE *
ActiveVGADeviceOnTheSameSegment (
  IN PCI_IO_DEVICE        *VgaDevice
  );

/**
  Active VGA device on root bridge.
  
  @param RootBridge  Root bridge device instance.
  
  @return VGA device instance.
**/
PCI_IO_DEVICE *
ActiveVGADeviceOnTheRootBridge (
  IN PCI_IO_DEVICE        *RootBridge
  );

/**
  Get HPC PCI address according to its device path.
  @param PciRootBridgeIo   Root bridege Io instance.
  @param HpcDevicePath     Given searching device path.
  @param PciAddress        Buffer holding searched result.
  
  @retval EFI_NOT_FOUND Can not find the specific device path.
  @retval EFI_SUCCESS   Success to get the device path.
**/
EFI_STATUS
GetHpcPciAddress (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN  EFI_DEVICE_PATH_PROTOCOL         *HpcDevicePath,
  OUT UINT64                           *PciAddress
  );

/**
  Get HPC PCI address according to its device path.
  @param RootBridge           Root bridege Io instance.
  @param RemainingDevicePath  Given searching device path.
  @param PciAddress           Buffer holding searched result.
  
  @retval EFI_NOT_FOUND Can not find the specific device path.
**/
EFI_STATUS
GetHpcPciAddressFromRootBridge (
  IN  PCI_IO_DEVICE                    *RootBridge,
  IN  EFI_DEVICE_PATH_PROTOCOL         *RemainingDevicePath,
  OUT UINT64                           *PciAddress
  );

/**
  Destroy a pci device node.
  Also all direct or indirect allocated resource for this node will be freed.

  @param PciIoDevice  A pointer to the PCI_IO_DEVICE.

**/
EFI_STATUS
FreePciDevice (
  IN PCI_IO_DEVICE *PciIoDevice
  );

#endif
