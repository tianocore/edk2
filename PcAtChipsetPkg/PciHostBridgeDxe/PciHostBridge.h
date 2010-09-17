/** @file
  The Header file of the Pci Host Bridge Driver 

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/ 

#ifndef _PCI_HOST_BRIDGE_H_
#define _PCI_HOST_BRIDGE_H_

#include <PiDxe.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>

#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/Metronome.h>
#include <Protocol/DevicePath.h>


#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>

//
// Hard code the host bridge number in the platform.
// In this chipset, there is only one host bridge.
//
#define HOST_BRIDGE_NUMBER  1

#define MAX_PCI_DEVICE_NUMBER      31
#define MAX_PCI_FUNCTION_NUMBER    7
#define MAX_PCI_REG_ADDRESS        0xFF

typedef enum {
  IoOperation,
  MemOperation,
  PciOperation
} OPERATION_TYPE;

#define PCI_HOST_BRIDGE_SIGNATURE  SIGNATURE_32('e', 'h', 's', 't')
typedef struct {
  UINTN                                             Signature;
  EFI_HANDLE                                        HostBridgeHandle;
  UINTN                                             RootBridgeNumber;
  LIST_ENTRY                                        Head;
  BOOLEAN                                           ResourceSubmited;  
  BOOLEAN                                           CanRestarted;  
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  ResAlloc;
} PCI_HOST_BRIDGE_INSTANCE;

#define INSTANCE_FROM_RESOURCE_ALLOCATION_THIS(a) \
  CR(a, PCI_HOST_BRIDGE_INSTANCE, ResAlloc, PCI_HOST_BRIDGE_SIGNATURE)
  
//
//  HostBridge Resource Allocation interface
//

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

   @param[in] This                The instance pointer of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
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
EFIAPI
NotifyPhase(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE    Phase
  );

/**
   Return the device handle of the next PCI root bridge that is associated with this Host Bridge.

   This function is called multiple times to retrieve the device handles of all the PCI root bridges that
   are associated with this PCI host bridge. Each PCI host bridge is associated with one or more PCI
   root bridges. On each call, the handle that was returned by the previous call is passed into the
   interface, and on output the interface returns the device handle of the next PCI root bridge. The
   caller can use the handle to obtain the instance of the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
   for that root bridge. When there are no more PCI root bridges to report, the interface returns
   EFI_NOT_FOUND. A PCI enumerator must enumerate the PCI root bridges in the order that they
   are returned by this function.
   For D945 implementation, there is only one root bridge in PCI host bridge.

   @param[in]       This              The instance pointer of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
   @param[in, out]  RootBridgeHandle  Returns the device handle of the next PCI root bridge.
   
   @retval EFI_SUCCESS            If parameter RootBridgeHandle = NULL, then return the first Rootbridge handle of the
                                  specific Host bridge and return EFI_SUCCESS. 
   @retval EFI_NOT_FOUND          Can not find the any more root bridge in specific host bridge.
   @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not an EFI_HANDLE that was
                                  returned on a previous call to GetNextRootBridge().
**/
EFI_STATUS
EFIAPI
GetNextRootBridge(
  IN       EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN OUT   EFI_HANDLE                                       *RootBridgeHandle
  );
  
/**
   Returns the allocation attributes of a PCI root bridge.

   The function returns the allocation attributes of a specific PCI root bridge. The attributes can vary
   from one PCI root bridge to another. These attributes are different from the decode-related
   attributes that are returned by the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.GetAttributes() member function. The
   RootBridgeHandle parameter is used to specify the instance of the PCI root bridge. The device
   handles of all the root bridges that are associated with this host bridge must be obtained by calling
   GetNextRootBridge(). The attributes are static in the sense that they do not change during or
   after the enumeration process. The hardware may provide mechanisms to change the attributes on
   the fly, but such changes must be completed before EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL is 
   installed. The permitted values of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ATTRIBUTES are defined in
   "Related Definitions" below. The caller uses these attributes to combine multiple resource requests.
   For example, if the flag EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM is set, the PCI bus enumerator needs to 
   include requests for the prefetchable memory in the nonprefetchable memory pool and not request any 
   prefetchable memory.
      Attribute                                 Description
   ------------------------------------         ----------------------------------------------------------------------
   EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM         If this bit is set, then the PCI root bridge does not support separate
                                                windows for nonprefetchable and prefetchable memory. A PCI bus
                                                driver needs to include requests for prefetchable memory in the
                                                nonprefetchable memory pool.

   EFI_PCI_HOST_BRIDGE_MEM64_DECODE             If this bit is set, then the PCI root bridge supports 64-bit memory
                                                windows. If this bit is not set, the PCI bus driver needs to include
                                                requests for a 64-bit memory address in the corresponding 32-bit
                                                memory pool.

   @param[in]   This               The instance pointer of EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
   @param[in]   RootBridgeHandle   The device handle of the PCI root bridge in which the caller is interested. Type
                                   EFI_HANDLE is defined in InstallProtocolInterface() in the UEFI 2.0 Specification.
   @param[out]  Attributes         The pointer to attribte of root bridge, it is output parameter
   
   @retval EFI_INVALID_PARAMETER   Attribute pointer is NULL
   @retval EFI_INVALID_PARAMETER   RootBridgehandle is invalid.
   @retval EFI_SUCCESS             Success to get attribute of interested root bridge.

**/  
EFI_STATUS
EFIAPI
GetAttributes(
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT UINT64                                           *Attributes
  );
  
/**
   Sets up the specified PCI root bridge for the bus enumeration process.

   This member function sets up the root bridge for bus enumeration and returns the PCI bus range
   over which the search should be performed in ACPI 2.0 resource descriptor format.

   @param[in]   This              The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
   @param[in]   RootBridgeHandle  The PCI Root Bridge to be set up.
   @param[out]  Configuration     Pointer to the pointer to the PCI bus resource descriptor.
   
   @retval EFI_INVALID_PARAMETER Invalid Root bridge's handle
   @retval EFI_OUT_OF_RESOURCES  Fail to allocate ACPI resource descriptor tag.
   @retval EFI_SUCCESS           Sucess to allocate ACPI resource descriptor.

**/
EFI_STATUS
EFIAPI
StartBusEnumeration(
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  );
  
/**
   Programs the PCI root bridge hardware so that it decodes the specified PCI bus range.

   This member function programs the specified PCI root bridge to decode the bus range that is
   specified by the input parameter Configuration.
   The bus range information is specified in terms of the ACPI 2.0 resource descriptor format.

   @param[in] This              The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
   @param[in] RootBridgeHandle  The PCI Root Bridge whose bus range is to be programmed
   @param[in] Configuration     The pointer to the PCI bus resource descriptor
   
   @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not a valid root bridge handle.
   @retval EFI_INVALID_PARAMETER  Configuration is NULL.
   @retval EFI_INVALID_PARAMETER  Configuration does not point to a valid ACPI 2.0 resource descriptor.
   @retval EFI_INVALID_PARAMETER  Configuration does not include a valid ACPI 2.0 bus resource descriptor.
   @retval EFI_INVALID_PARAMETER  Configuration includes valid ACPI 2.0 resource descriptors other than 
                                  bus descriptors.
   @retval EFI_INVALID_PARAMETER  Configuration contains one or more invalid ACPI resource descriptors.
   @retval EFI_INVALID_PARAMETER  "Address Range Minimum" is invalid for this root bridge.
   @retval EFI_INVALID_PARAMETER  "Address Range Length" is invalid for this root bridge.
   @retval EFI_DEVICE_ERROR       Programming failed due to a hardware error.
   @retval EFI_SUCCESS            The bus range for the PCI root bridge was programmed.

**/
EFI_STATUS
EFIAPI
SetBusNumbers(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  );
  
/**
   Submits the I/O and memory resource requirements for the specified PCI root bridge.

   This function is used to submit all the I/O and memory resources that are required by the specified
   PCI root bridge. The input parameter Configuration is used to specify the following:
   - The various types of resources that are required
   - The associated lengths in terms of ACPI 2.0 resource descriptor format

   @param[in] This              Pointer to the EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL instance.
   @param[in] RootBridgeHandle  The PCI root bridge whose I/O and memory resource requirements are being submitted.
   @param[in] Configuration     The pointer to the PCI I/O and PCI memory resource descriptor.
   
   @retval EFI_SUCCESS            The I/O and memory resource requests for a PCI root bridge were accepted.
   @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not a valid root bridge handle.
   @retval EFI_INVALID_PARAMETER  Configuration is NULL.
   @retval EFI_INVALID_PARAMETER  Configuration does not point to a valid ACPI 2.0 resource descriptor.
   @retval EFI_INVALID_PARAMETER  Configuration includes requests for one or more resource types that are 
                                  not supported by this PCI root bridge. This error will happen if the caller 
                                  did not combine resources according to Attributes that were returned by
                                  GetAllocAttributes().
   @retval EFI_INVALID_PARAMETER  Address Range Maximum" is invalid.
   @retval EFI_INVALID_PARAMETER  "Address Range Length" is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  "Address Space Granularity" is invalid for this PCI root bridge.

**/
EFI_STATUS
EFIAPI
SubmitResources(
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  );
  
/**
   Returns the proposed resource settings for the specified PCI root bridge.

   This member function returns the proposed resource settings for the specified PCI root bridge. The
   proposed resource settings are prepared when NotifyPhase() is called with a Phase of
   EfiPciHostBridgeAllocateResources. The output parameter Configuration
   specifies the following:
   - The various types of resources, excluding bus resources, that are allocated
   - The associated lengths in terms of ACPI 2.0 resource descriptor format

   @param[in]  This              Pointer to the EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL instance.
   @param[in]  RootBridgeHandle  The PCI root bridge handle. Type EFI_HANDLE is defined in InstallProtocolInterface() in the UEFI 2.0 Specification.
   @param[out] Configuration     The pointer to the pointer to the PCI I/O and memory resource descriptor.
   
   @retval EFI_SUCCESS            The requested parameters were returned.
   @retval EFI_INVALID_PARAMETER  RootBridgeHandle is not a valid root bridge handle.
   @retval EFI_DEVICE_ERROR       Programming failed due to a hardware error.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
GetProposedResources(
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  );

/**
   Provides the hooks from the PCI bus driver to every PCI controller (device/function) at various
   stages of the PCI enumeration process that allow the host bridge driver to preinitialize individual
   PCI controllers before enumeration.

   This function is called during the PCI enumeration process. No specific action is expected from this
   member function. It allows the host bridge driver to preinitialize individual PCI controllers before
   enumeration.

   @param This              Pointer to the EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL instance.
   @param RootBridgeHandle  The associated PCI root bridge handle. Type EFI_HANDLE is defined in
                            InstallProtocolInterface() in the UEFI 2.0 Specification.
   @param PciAddress        The address of the PCI device on the PCI bus. This address can be passed to the
                            EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL member functions to access the PCI
                            configuration space of the device. See Table 12-1 in the UEFI 2.0 Specification for
                            the definition of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS.
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
EFIAPI
PreprocessController (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *This,
  IN  EFI_HANDLE                                        RootBridgeHandle,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS       PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE      Phase
  );


//
// Define resource status constant 
//
#define EFI_RESOURCE_NONEXISTENT   0xFFFFFFFFFFFFFFFFULL
#define EFI_RESOURCE_LESS          0xFFFFFFFFFFFFFFFEULL


//
// Driver Instance Data Prototypes
//

typedef struct {
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation;
  UINTN                                      NumberOfBytes;
  UINTN                                      NumberOfPages;
  EFI_PHYSICAL_ADDRESS                       HostAddress;
  EFI_PHYSICAL_ADDRESS                       MappedHostAddress;
} MAP_INFO;

typedef struct {
  ACPI_HID_DEVICE_PATH              AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;

typedef struct {
  UINT64          BusBase;
  UINT64          BusLimit;     
  
  UINT64          MemBase;     
  UINT64          MemLimit;    
  
  UINT64          IoBase; 
  UINT64          IoLimit;     
} PCI_ROOT_BRIDGE_RESOURCE_APPETURE;

typedef enum {
  TypeIo = 0,
  TypeMem32,
  TypePMem32,
  TypeMem64,
  TypePMem64,
  TypeBus,
  TypeMax
} PCI_RESOURCE_TYPE;

typedef enum {
  ResNone = 0,
  ResSubmitted,
  ResRequested,
  ResAllocated,
  ResStatusMax
} RES_STATUS;

typedef struct {
  PCI_RESOURCE_TYPE Type;
  UINT64            Base;
  UINT64            Length;
  UINT64            Alignment;
  RES_STATUS        Status;
} PCI_RES_NODE;

#define PCI_ROOT_BRIDGE_SIGNATURE  SIGNATURE_32('e', '2', 'p', 'b')

typedef struct {
  UINT32                 Signature;
  LIST_ENTRY             Link;
  EFI_HANDLE             Handle;
  UINT64                 RootBridgeAttrib;
  UINT64                 Attributes;
  UINT64                 Supports;
  
  //
  // Specific for this memory controller: Bus, I/O, Mem
  //
  PCI_RES_NODE           ResAllocNode[6];
  
  //
  // Addressing for Memory and I/O and Bus arrange
  //
  UINT64                 BusBase;
  UINT64                 MemBase;     
  UINT64                 IoBase; 
  UINT64                 BusLimit;     
  UINT64                 MemLimit;    
  UINT64                 IoLimit;     

  UINTN                  PciAddress;
  UINTN                  PciData;
  
  EFI_DEVICE_PATH_PROTOCOL                *DevicePath;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL         Io;

} PCI_ROOT_BRIDGE_INSTANCE;


//
// Driver Instance Data Macros
//
#define DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(a) \
  CR(a, PCI_ROOT_BRIDGE_INSTANCE, Io, PCI_ROOT_BRIDGE_SIGNATURE)


#define DRIVER_INSTANCE_FROM_LIST_ENTRY(a) \
  CR(a, PCI_ROOT_BRIDGE_INSTANCE, Link, PCI_ROOT_BRIDGE_SIGNATURE)

/**

  Construct the Pci Root Bridge Io protocol

  @param Protocol         Point to protocol instance
  @param HostBridgeHandle Handle of host bridge
  @param Attri            Attribute of host bridge
  @param ResAppeture      ResourceAppeture for host bridge

  @retval EFI_SUCCESS Success to initialize the Pci Root Bridge.

**/
EFI_STATUS
RootBridgeConstructor (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL    *Protocol,
  IN EFI_HANDLE                         HostBridgeHandle,
  IN UINT64                             Attri,
  IN PCI_ROOT_BRIDGE_RESOURCE_APPETURE  *ResAppeture
  );

#endif
