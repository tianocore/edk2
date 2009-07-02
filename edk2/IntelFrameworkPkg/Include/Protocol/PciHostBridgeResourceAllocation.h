/** @file
  This file declares Pci Host Bridge Resource Allocation Protocol which 
  Provides the basic interfaces to abstract a PCI host bridge resource allocation. This protocol is
  mandatory if the system includes PCI devices.
  
  Copyright (c) 2007 - 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This protocol is defined in Framework of EFI Pci Host Bridge Resource Allocation Protocol Spec
  Version 0.9

**/

#ifndef _PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_H_
#define _PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_H_

#include <PiDxe.h>
#include <Protocol/PciRootBridgeIo.h>

#define EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_GUID \
  { 0xCF8034BE, 0x6768, 0x4d8b, {0xB7,0x39,0x7C,0xCE,0x68,0x3A,0x9F,0xBE }}


typedef struct _EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL;


//
// EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ATTRIBUTES
//

/// If this bit is set, then the PCI Root Bridge does not
/// support separate windows for Non-prefetchable and Prefetchable
/// memory. A PCI bus driver needs to include requests for Prefetchable
/// memory in the Non-prefetchable memory pool.
///
#define EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM  1

///
/// If this bit is set, then the PCI Root Bridge supports
/// 64 bit memory windows.  If this bit is not set,
/// the PCI bus driver needs to include requests for 64 bit
/// memory address in the corresponding 32 bit memory pool.
///
#define EFI_PCI_HOST_BRIDGE_MEM64_DECODE   2

typedef UINT64 EFI_RESOURCE_ALLOCATION_STATUS;

/// The request of this resource type could be fulfilled.
#define EFI_RESOURCE_SATISFIED      0x0000000000000000ULL

/// The request of this resource type could not be fulfilled for its
/// absence in the host bridge resource pool.
#define EFI_RESOURCE_NOT_SATISFIED  0xFFFFFFFFFFFFFFFFULL

//
// EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE
//
typedef enum {
  ///
  /// Reset the host bridge PCI apertures and internal data structures.
  /// PCI enumerator should issue this notification before starting fresh
  /// enumeration process. Enumeration cannot be restarted after sending
  /// any other notification such as EfiPciHostBridgeBeginBusAllocation.
  ///
  EfiPciHostBridgeBeginEnumeration,

  ///
  /// The bus allocation phase is about to begin. No specific action
  /// is required here. This notification can be used to perform any
  /// chipset specific programming.  
  ///
  EfiPciHostBridgeBeginBusAllocation,

  ///
  /// The bus allocation and bus programming phase is complete. No specific
  /// action is required here. This notification can be used to perform any
  /// chipset specific programming.  
  ///
  EfiPciHostBridgeEndBusAllocation,
  
  ///
  /// The resource allocation phase is about to begin.No specific action is
  /// required here. This notification can be used to perform any chipset specific programming.  
  ///
  EfiPciHostBridgeBeginResourceAllocation,
  
  ///
  /// Allocate resources per previously submitted requests for all the PCI Root
  /// Bridges. These resource settings are returned on the next call to
  /// GetProposedResources().  
  ///
  EfiPciHostBridgeAllocateResources,
  
  ///
  /// Program the Host Bridge hardware to decode previously allocated resources
  /// (proposed resources) for all the PCI Root Bridges.
  ///
  EfiPciHostBridgeSetResources,
  
  ///
  /// De-allocate previously allocated resources previously for all the PCI
  /// Root Bridges and reset the I/O and memory apertures to initial state.  
  ///
  EfiPciHostBridgeFreeResources,
  
  ///
  /// The resource allocation phase is completed.  No specific action is required
  /// here. This notification can be used to perform any chipset specific programming.  
  ///
  EfiPciHostBridgeEndResourceAllocation
} EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE;

///
/// EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE
///
typedef enum {
  ///
  /// This notification is only applicable to PCI-PCI bridges and
  /// indicates that the PCI enumerator is about to begin enumerating
  /// the bus behind the PCI-PCI Bridge. This notification is sent after
  /// the primary bus number, the secondary bus number and the subordinate
  /// bus number registers in the PCI-PCI Bridge are programmed to valid
  /// (not necessary final) values
  ///
  EfiPciBeforeChildBusEnumeration,

  ///
  /// This notification is sent before the PCI enumerator probes BAR registers
  /// for every valid PCI function.  
  ///
  EfiPciBeforeResourceCollection
} EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE;

/**
  Enter a certain phase of the PCI enumeration process

  @param  This                  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL instance
  @param  Phase                 The phase during enumeration

  @retval EFI_SUCCESS           Success
  @retval EFI_OUT_OF_RESOURCES  If SubmitResources ( ) could not allocate resources
  @retval EFI_INVALID_PARAMETER The Phase is invalid
  @retval EFI_NOT_READY         This phase cannot be entered at this time
  @retval EFI_DEVICE_ERROR      SetResources failed due to HW error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_NOTIFY_PHASE)(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL         *This,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE            Phase
  );


/**
  Return the device handle of the next PCI root bridge that is associated with
  this Host Bridge

  @param  This                  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  @param  RootBridgeHandle      Returns the device handle of the next PCI Root Bridge.
                                On input, it holds the RootBridgeHandle returned by the most
                                recent call to GetNextRootBridge().The handle for the first
                                PCI Root Bridge is returned if RootBridgeHandle is NULL on input

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER RootBridgeHandle is invalid
  @retval EFI_NOT_FOUND        There are no more PCI root bridge device handles.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_GET_NEXT_ROOT_BRIDGE)(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL     *This,
  IN OUT EFI_HANDLE                                       *RootBridgeHandle
  );


/**
  Returns the attributes of a PCI Root Bridge.

  @param  This                  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  @param  RootBridgeHandle      The device handle of the PCI Root Bridge
                                that the caller is interested in
  @param  Attribute             The pointer to attributes of the PCI Root Bridge

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER RootBridgeHandle is invalid
  @retval EFI_INVALID_PARAMETER Attributes is NULL

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_GET_ATTRIBUTES)(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL           *This,
  IN  EFI_HANDLE                                                RootBridgeHandle,
  OUT UINT64                                                    *Attributes
  );


/**
  This is the request from the PCI enumerator to set up
  the specified PCI Root Bridge for bus enumeration process.

  @param  This                  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  @param  RootBridgeHandle      The PCI Root Bridge to be set up
  @param  Configuration         Pointer to the pointer to the PCI bus resource descriptor

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER RootBridgeHandle is invalid
  @retval EFI_DEVICE_ERROR      Request failed due to hardware error
  @retval EFI_OUT_OF_RESOURCES  Request failed due to lack of resources

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_START_BUS_ENUMERATION)(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL           *This,
  IN  EFI_HANDLE                                                RootBridgeHandle,
  OUT VOID                                                      **Configuration
  );


/**
  This function programs the PCI Root Bridge hardware so that
  it decodes the specified PCI bus range

  @param  This                  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  @param  RootBridgeHandle      The PCI Root Bridge whose bus range is to be programmed
  @param  Configuration         The pointer to the PCI bus resource descriptor

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER RootBridgeHandle is invalid
  @retval EFI_INVALID_PARAMETER Configuration is NULL
  @retval EFI_INVALID_PARAMETER Configuration does not point to a valid ACPI resource descriptor
  @retval EFI_INVALID_PARAMETER Configuration contains one or more memory or IO ACPI resource descriptor
  @retval EFI_INVALID_PARAMETER Address Range Minimum or Address Range Length fields in Configuration
                                are invalid for this Root Bridge.
  @retval EFI_INVALID_PARAMETER Configuration contains one or more invalid ACPI resource descriptor
  @retval EFI_DEVICE_ERROR      Request failed due to hardware error
  @retval EFI_OUT_OF_RESOURCES  Request failed due to lack of resources

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_SET_BUS_NUMBERS)(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL          *This,
  IN EFI_HANDLE                                                RootBridgeHandle,
  IN VOID                                                      *Configuration
  );


/**
  Submits the I/O and memory resource requirements for the specified PCI Root Bridge

  @param  This                  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  @param  RootBridgeHandle      The PCI Root Bridge whose I/O and memory resource requirements
                                are being submitted
  @param  Configuration         The pointer to the PCI I/O and PCI memory resource descriptor

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER RootBridgeHandle is invalid
  @retval EFI_INVALID_PARAMETER Configuration is NULL
  @retval EFI_INVALID_PARAMETER Configuration does not point to a valid ACPI resource descriptor
  @retval EFI_INVALID_PARAMETER Configuration includes a resource descriptor of unsupported type
  
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_SUBMIT_RESOURCES)(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL          *This,
  IN EFI_HANDLE                                                RootBridgeHandle,
  IN VOID                                                      *Configuration
  );


/**
  This function returns the proposed resource settings for the specified
  PCI Root Bridge

  @param  This                  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  @param  RootBridgeHandle      The PCI Root Bridge handle
  @param  Configuration         The pointer to the pointer to the PCI I/O
                                and memory resource descriptor

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER RootBridgeHandle is invalid
  @retval EFI_DEVICE_ERROR      Request failed due to hardware error
  @retval EFI_OUT_OF_RESOURCES  Request failed due to lack of resources

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_GET_PROPOSED_RESOURCES)(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL           *This,
  IN  EFI_HANDLE                                                RootBridgeHandle,
  OUT VOID                                                      **Configuration
  );



/**
  This function is called for all the PCI controllers that the PCI
  bus driver finds. Can be used to Preprogram the controller.

  @param  This                  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  @param  RootBridgeHandle      The PCI Root Bridge handle
  @param  PciBusAddress         Address of the controller on the PCI bus
  @param  Phase                 The Phase during resource allocation

  @retval EFI_SUCCESS           Success
  @retval EFI_INVALID_PARAMETER RootBridgeHandle is invalid
  @retval EFI_DEVICE_ERROR      Device pre-initialization failed due to hardware error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_PREPROCESS_CONTROLLER)(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL            *This,
  IN  EFI_HANDLE                                                 RootBridgeHandle,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS                PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE               Phase
  );

/**
  Provides the basic interfaces to abstract a PCI host bridge resource allocation.
**/
struct _EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL {
  ///
  /// The notification from the PCI bus enumerator that it is about to enter
  /// a certain phase during the enumeration process.
  ///
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_NOTIFY_PHASE           NotifyPhase;
  
  ///
  /// Retrieves the device handle for the next PCI root bridge that is produced by the
  /// host bridge to which this instance of the EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL is attached.  
  ///
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_GET_NEXT_ROOT_BRIDGE   GetNextRootBridge;
  
  ///
  /// Retrieves the allocation-related attributes of a PCI root bridge.
  ///
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_GET_ATTRIBUTES         GetAllocAttributes;
  
  ///
  /// Sets up a PCI root bridge for bus enumeration.
  ///
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_START_BUS_ENUMERATION  StartBusEnumeration;
  
  ///
  /// Sets up the PCI root bridge so that it decodes a specific range of bus numbers.
  ///
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_SET_BUS_NUMBERS        SetBusNumbers;
  
  ///
  /// Submits the resource requirements for the specified PCI root bridge.
  ///
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_SUBMIT_RESOURCES       SubmitResources;
  
  ///
  /// Returns the proposed resource assignment for the specified PCI root bridges.
  ///
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_GET_PROPOSED_RESOURCES GetProposedResources;
  
  ///
  /// Provides hooks from the PCI bus driver to every PCI controller
  /// (device/function) at various stages of the PCI enumeration process that
  /// allow the host bridge driver to preinitialize individual PCI controllers
  /// before enumeration.  
  ///
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_PREPROCESS_CONTROLLER  PreprocessController;
};

extern EFI_GUID gEfiPciHostBridgeResourceAllocationProtocolGuid;

#endif
