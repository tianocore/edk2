/** @file
  PCI bus enumeration logic function declaration for PCI bus module.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_PCI_ENUMERATOR_H_
#define _EFI_PCI_ENUMERATOR_H_

#include "PciResourceSupport.h"

/**
  This routine is used to enumerate entire pci bus system
  in a given platform.

  @param Controller          Parent controller handle.
  @param HostBridgeHandle    Host bridge handle.

  @retval EFI_SUCCESS    PCI enumeration finished successfully.
  @retval other          Some error occurred when enumerating the pci bus system.

**/
EFI_STATUS
PciEnumerator (
  IN EFI_HANDLE                    Controller,
  IN EFI_HANDLE                    HostBridgeHandle
  );

/**
  Enumerate PCI root bridge.

  @param PciResAlloc   Pointer to protocol instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.
  @param RootBridgeDev Instance of root bridge device.

  @retval EFI_SUCCESS  Successfully enumerated root bridge.
  @retval other        Failed to enumerate root bridge.

**/
EFI_STATUS
PciRootBridgeEnumerator (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *PciResAlloc,
  IN PCI_IO_DEVICE                                     *RootBridgeDev
  );

/**
  This routine is used to process all PCI devices' Option Rom
  on a certain root bridge.

  @param Bridge     Given parent's root bridge.
  @param RomBase    Base address of ROM driver loaded from.
  @param MaxLength  Maximum rom size.

**/
VOID
ProcessOptionRom (
  IN PCI_IO_DEVICE *Bridge,
  IN UINT64        RomBase,
  IN UINT64        MaxLength
  );

/**
  This routine is used to assign bus number to the given PCI bus system

  @param Bridge             Parent root bridge instance.
  @param StartBusNumber     Number of beginning.
  @param SubBusNumber       The number of sub bus.

  @retval EFI_SUCCESS       Successfully assigned bus number.
  @retval EFI_DEVICE_ERROR  Failed to assign bus number.

**/
EFI_STATUS
PciAssignBusNumber (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber,
  OUT UINT8                             *SubBusNumber
  );

/**
  This routine is used to determine the root bridge attribute by interfacing
  the host bridge resource allocation protocol.

  @param PciResAlloc    Protocol instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
  @param RootBridgeDev  Root bridge instance

  @retval EFI_SUCCESS  Successfully got root bridge's attribute.
  @retval other        Failed to get attribute.

**/
EFI_STATUS
DetermineRootBridgeAttributes (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc,
  IN PCI_IO_DEVICE                                    *RootBridgeDev
  );

/**
  Get Max Option Rom size on specified bridge.

  @param Bridge    Given bridge device instance.

  @return Max size of option rom needed.

**/
UINT64
GetMaxOptionRomSize (
  IN PCI_IO_DEVICE   *Bridge
  );

/**
  Process attributes of devices on this host bridge

  @param PciResAlloc Protocol instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.

  @retval EFI_SUCCESS   Successfully process attribute.
  @retval EFI_NOT_FOUND Can not find the specific root bridge device.
  @retval other         Failed to determine the root bridge device's attribute.

**/
EFI_STATUS
PciHostBridgeDeviceAttribute (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  );

/**
  Get resource allocation status from the ACPI resource descriptor.

  @param AcpiConfig       Point to Acpi configuration table.
  @param IoResStatus      Return the status of I/O resource.
  @param Mem32ResStatus   Return the status of 32-bit Memory resource.
  @param PMem32ResStatus  Return the status of 32-bit Prefetchable Memory resource.
  @param Mem64ResStatus   Return the status of 64-bit Memory resource.
  @param PMem64ResStatus  Return the status of 64-bit Prefetchable Memory resource.

**/
VOID
GetResourceAllocationStatus (
  VOID        *AcpiConfig,
  OUT UINT64  *IoResStatus,
  OUT UINT64  *Mem32ResStatus,
  OUT UINT64  *PMem32ResStatus,
  OUT UINT64  *Mem64ResStatus,
  OUT UINT64  *PMem64ResStatus
  );

/**
  Remove a PCI device from device pool and mark its bar.

  @param PciDevice Instance of Pci device.

  @retval EFI_SUCCESS Successfully remove the PCI device.
  @retval EFI_ABORTED Pci device is a root bridge or a PCI-PCI bridge.

**/
EFI_STATUS
RejectPciDevice (
  IN PCI_IO_DEVICE       *PciDevice
  );

/**
  Determine whethter a PCI device can be rejected.

  @param  PciResNode Pointer to Pci resource node instance.

  @retval TRUE  The PCI device can be rejected.
  @retval TRUE  The PCI device cannot be rejected.

**/
BOOLEAN
IsRejectiveDevice (
  IN  PCI_RESOURCE_NODE   *PciResNode
  );

/**
  Compare two resource nodes and get the larger resource consumer.

  @param PciResNode1  resource node 1 want to be compared
  @param PciResNode2  resource node 2 want to be compared

  @return Larger resource node.

**/
PCI_RESOURCE_NODE *
GetLargerConsumerDevice (
  IN  PCI_RESOURCE_NODE   *PciResNode1,
  IN  PCI_RESOURCE_NODE   *PciResNode2
  );

/**
  Get the max resource consumer in the host resource pool.

  @param ResPool  Pointer to resource pool node.

  @return The max resource consumer in the host resource pool.

**/
PCI_RESOURCE_NODE *
GetMaxResourceConsumerDevice (
  IN  PCI_RESOURCE_NODE   *ResPool
  );

/**
  Adjust host bridge allocation so as to reduce resource requirement

  @param IoPool           Pointer to instance of I/O resource Node.
  @param Mem32Pool        Pointer to instance of 32-bit memory resource Node.
  @param PMem32Pool       Pointer to instance of 32-bit Prefetchable memory resource node.
  @param Mem64Pool        Pointer to instance of 64-bit memory resource node.
  @param PMem64Pool       Pointer to instance of 64-bit Prefetchable memory resource node.
  @param IoResStatus      Status of I/O resource Node.
  @param Mem32ResStatus   Status of 32-bit memory resource Node.
  @param PMem32ResStatus  Status of 32-bit Prefetchable memory resource node.
  @param Mem64ResStatus   Status of 64-bit memory resource node.
  @param PMem64ResStatus  Status of 64-bit Prefetchable memory resource node.

  @retval EFI_SUCCESS     Successfully adjusted resource on host bridge.
  @retval EFI_ABORTED     Host bridge hasn't this resource type or no resource be adjusted.

**/
EFI_STATUS
PciHostBridgeAdjustAllocation (
  IN  PCI_RESOURCE_NODE   *IoPool,
  IN  PCI_RESOURCE_NODE   *Mem32Pool,
  IN  PCI_RESOURCE_NODE   *PMem32Pool,
  IN  PCI_RESOURCE_NODE   *Mem64Pool,
  IN  PCI_RESOURCE_NODE   *PMem64Pool,
  IN  UINT64              IoResStatus,
  IN  UINT64              Mem32ResStatus,
  IN  UINT64              PMem32ResStatus,
  IN  UINT64              Mem64ResStatus,
  IN  UINT64              PMem64ResStatus
  );

/**
  Summary requests for all resource type, and construct ACPI resource
  requestor instance.

  @param Bridge           detecting bridge
  @param IoNode           Pointer to instance of I/O resource Node
  @param Mem32Node        Pointer to instance of 32-bit memory resource Node
  @param PMem32Node       Pointer to instance of 32-bit Pmemory resource node
  @param Mem64Node        Pointer to instance of 64-bit memory resource node
  @param PMem64Node       Pointer to instance of 64-bit Pmemory resource node
  @param Config           Output buffer holding new constructed APCI resource requestor

  @retval EFI_SUCCESS           Successfully constructed ACPI resource.
  @retval EFI_OUT_OF_RESOURCES  No memory available.

**/
EFI_STATUS
ConstructAcpiResourceRequestor (
  IN PCI_IO_DEVICE      *Bridge,
  IN PCI_RESOURCE_NODE  *IoNode,
  IN PCI_RESOURCE_NODE  *Mem32Node,
  IN PCI_RESOURCE_NODE  *PMem32Node,
  IN PCI_RESOURCE_NODE  *Mem64Node,
  IN PCI_RESOURCE_NODE  *PMem64Node,
  OUT VOID              **Config
  );

/**
  Get resource base from an acpi configuration descriptor.

  @param Config       An acpi configuration descriptor.
  @param IoBase       Output of I/O resource base address.
  @param Mem32Base    Output of 32-bit memory base address.
  @param PMem32Base   Output of 32-bit prefetchable memory base address.
  @param Mem64Base    Output of 64-bit memory base address.
  @param PMem64Base   Output of 64-bit prefetchable memory base address.

**/
VOID
GetResourceBase (
  IN VOID     *Config,
  OUT UINT64  *IoBase,
  OUT UINT64  *Mem32Base,
  OUT UINT64  *PMem32Base,
  OUT UINT64  *Mem64Base,
  OUT UINT64  *PMem64Base
  );

/**
  Enumerate pci bridge, allocate resource and determine attribute
  for devices on this bridge.

  @param BridgeDev    Pointer to instance of bridge device.

  @retval EFI_SUCCESS Successfully enumerated PCI bridge.
  @retval other       Failed to enumerate.

**/
EFI_STATUS
PciBridgeEnumerator (
  IN PCI_IO_DEVICE                                     *BridgeDev
  );

/**
  Allocate all kinds of resource for PCI bridge.

  @param  Bridge      Pointer to bridge instance.

  @retval EFI_SUCCESS Successfully allocated resource for PCI bridge.
  @retval other       Failed to allocate resource for bridge.

**/
EFI_STATUS
PciBridgeResourceAllocator (
  IN PCI_IO_DEVICE  *Bridge
  );

/**
  Get resource base address for a pci bridge device.

  @param Bridge     Given Pci driver instance.
  @param IoBase     Output for base address of I/O type resource.
  @param Mem32Base  Output for base address of 32-bit memory type resource.
  @param PMem32Base Ooutput for base address of 32-bit Pmemory type resource.
  @param Mem64Base  Output for base address of 64-bit memory type resource.
  @param PMem64Base Output for base address of 64-bit Pmemory type resource.

  @retval EFI_SUCCESS           Successfully got resource base address.
  @retval EFI_OUT_OF_RESOURCES  PCI bridge is not available.

**/
EFI_STATUS
GetResourceBaseFromBridge (
  IN  PCI_IO_DEVICE *Bridge,
  OUT UINT64        *IoBase,
  OUT UINT64        *Mem32Base,
  OUT UINT64        *PMem32Base,
  OUT UINT64        *Mem64Base,
  OUT UINT64        *PMem64Base
  );

/**
  Process Option Rom on this host bridge

  @param PciResAlloc Pointer to instance of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.

  @retval EFI_NOT_FOUND Can not find the root bridge instance.
  @retval EFI_SUCCESS   Success process.
**/
EFI_STATUS
PciHostBridgeP2CProcess (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc
  );

/**
   These are the notifications from the PCI bus driver that it is about to enter a certain
   phase of the PCI enumeration process.

   This member function can be used to notify the host bridge driver to perform specific actions,
   including any chipset-specific initialization, so that the chipset is ready to enter the next phase.
   Eight notification points are defined at this time. See belows:
   EfiPciHostBridgeBeginEnumeration       Resets the host bridge PCI apertures and internal data
                                          structures. The PCI enumerator should issue this notification
                                          before starting a fresh enumeration process. Enumeration cannot
                                          be restarted after sending any other notification such as
                                          EfiPciHostBridgeBeginBusAllocation.
   EfiPciHostBridgeBeginBusAllocation     The bus allocation phase is about to begin. No specific action is
                                          required here. This notification can be used to perform any
                                          chipset-specific programming.
   EfiPciHostBridgeEndBusAllocation       The bus allocation and bus programming phase is complete. No
                                          specific action is required here. This notification can be used to
                                          perform any chipset-specific programming.
   EfiPciHostBridgeBeginResourceAllocation
                                          The resource allocation phase is about to begin. No specific
                                          action is required here. This notification can be used to perform
                                          any chipset-specific programming.
   EfiPciHostBridgeAllocateResources      Allocates resources per previously submitted requests for all the PCI
                                          root bridges. These resource settings are returned on the next call to
                                          GetProposedResources(). Before calling NotifyPhase() with a Phase of
                                          EfiPciHostBridgeAllocateResource, the PCI bus enumerator is responsible
                                          for gathering I/O and memory requests for
                                          all the PCI root bridges and submitting these requests using
                                          SubmitResources(). This function pads the resource amount
                                          to suit the root bridge hardware, takes care of dependencies between
                                          the PCI root bridges, and calls the Global Coherency Domain (GCD)
                                          with the allocation request. In the case of padding, the allocated range
                                          could be bigger than what was requested.
   EfiPciHostBridgeSetResources           Programs the host bridge hardware to decode previously allocated
                                          resources (proposed resources) for all the PCI root bridges. After the
                                          hardware is programmed, reassigning resources will not be supported.
                                          The bus settings are not affected.
   EfiPciHostBridgeFreeResources          Deallocates resources that were previously allocated for all the PCI
                                          root bridges and resets the I/O and memory apertures to their initial
                                          state. The bus settings are not affected. If the request to allocate
                                          resources fails, the PCI enumerator can use this notification to
                                          deallocate previous resources, adjust the requests, and retry
                                          allocation.
   EfiPciHostBridgeEndResourceAllocation  The resource allocation phase is completed. No specific action is
                                          required here. This notification can be used to perform any chipsetspecific
                                          programming.

   @param[in] PciResAlloc         The instance pointer of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
   @param[in] Phase               The phase during enumeration

   @retval EFI_NOT_READY          This phase cannot be entered at this time. For example, this error
                                  is valid for a Phase of EfiPciHostBridgeAllocateResources if
                                  SubmitResources() has not been called for one or more
                                  PCI root bridges before this call
   @retval EFI_DEVICE_ERROR       Programming failed due to a hardware error. This error is valid
                                  for a Phase of EfiPciHostBridgeSetResources.
   @retval EFI_INVALID_PARAMETER  Invalid phase parameter
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.
                                  This error is valid for a Phase of EfiPciHostBridgeAllocateResources if the
                                  previously submitted resource requests cannot be fulfilled or
                                  were only partially fulfilled.
   @retval EFI_SUCCESS            The notification was accepted without any errors.

**/
EFI_STATUS
NotifyPhase (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *PciResAlloc,
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE       Phase
  );

/**
  Provides the hooks from the PCI bus driver to every PCI controller (device/function) at various
  stages of the PCI enumeration process that allow the host bridge driver to preinitialize individual
  PCI controllers before enumeration.

  This function is called during the PCI enumeration process. No specific action is expected from this
  member function. It allows the host bridge driver to preinitialize individual PCI controllers before
  enumeration.

  @param Bridge            Pointer to the EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL instance.
  @param Bus               The bus number of the pci device.
  @param Device            The device number of the pci device.
  @param Func              The function number of the pci device.
  @param Phase             The phase of the PCI device enumeration.

  @retval EFI_SUCCESS              The requested parameters were returned.
  @retval EFI_INVALID_PARAMETER    RootBridgeHandle is not a valid root bridge handle.
  @retval EFI_INVALID_PARAMETER    Phase is not a valid phase that is defined in
                                   EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE.
  @retval EFI_DEVICE_ERROR         Programming failed due to a hardware error. The PCI enumerator should
                                    not enumerate this device, including its child devices if it is a PCI-to-PCI
                                    bridge.

**/
EFI_STATUS
PreprocessController (
  IN PCI_IO_DEVICE                                  *Bridge,
  IN UINT8                                          Bus,
  IN UINT8                                          Device,
  IN UINT8                                          Func,
  IN EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE   Phase
  );

/**
  This function allows the PCI bus driver to be notified to act as requested when a hot-plug event has
  happened on the hot-plug controller. Currently, the operations include add operation and remove operation..

  @param This                 A pointer to the hot plug request protocol.
  @param Operation            The operation the PCI bus driver is requested to make.
  @param Controller           The handle of the hot-plug controller.
  @param RemainingDevicePath  The remaining device path for the PCI-like hot-plug device.
  @param NumberOfChildren     The number of child handles.
                              For a add operation, it is an output parameter.
                              For a remove operation, it's an input parameter.
  @param ChildHandleBuffer    The buffer which contains the child handles.

  @retval EFI_INVALID_PARAMETER  Operation is not a legal value.
                                 Controller is NULL or not a valid handle.
                                 NumberOfChildren is NULL.
                                 ChildHandleBuffer is NULL while Operation is add.
  @retval EFI_OUT_OF_RESOURCES   There are no enough resources to start the devices.
  @retval EFI_NOT_FOUND          Can not find bridge according to controller handle.
  @retval EFI_SUCCESS            The handles for the specified device have been created or destroyed
                                 as requested, and for an add operation, the new handles are
                                 returned in ChildHandleBuffer.
**/
EFI_STATUS
EFIAPI
PciHotPlugRequestNotify (
  IN EFI_PCI_HOTPLUG_REQUEST_PROTOCOL * This,
  IN EFI_PCI_HOTPLUG_OPERATION        Operation,
  IN EFI_HANDLE                       Controller,
  IN EFI_DEVICE_PATH_PROTOCOL         * RemainingDevicePath OPTIONAL,
  IN OUT UINT8                        *NumberOfChildren,
  IN OUT EFI_HANDLE                   * ChildHandleBuffer
  );

/**
  Search hostbridge according to given handle

  @param RootBridgeHandle  Host bridge handle.

  @retval TRUE             Found host bridge handle.
  @retval FALSE            Not found hot bridge handle.

**/
BOOLEAN
SearchHostBridgeHandle (
  IN EFI_HANDLE RootBridgeHandle
  );

/**
  Add host bridge handle to global variable for enumerating.

  @param HostBridgeHandle   Host bridge handle.

  @retval EFI_SUCCESS       Successfully added host bridge.
  @retval EFI_ABORTED       Host bridge is NULL, or given host bridge
                            has been in host bridge list.

**/
EFI_STATUS
AddHostBridgeEnumerator (
  IN EFI_HANDLE HostBridgeHandle
  );

#endif
