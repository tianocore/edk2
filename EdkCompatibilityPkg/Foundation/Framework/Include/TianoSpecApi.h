/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  TianoSpecApi.h

Abstract:

  Tiano intrinsic definitions in Tiano spec.


--*/

#ifndef _TIANO_SPEC_API_H_
#define _TIANO_SPEC_API_H_


#if ((TIANO_RELEASE_VERSION != 0) && (EFI_SPECIFICATION_VERSION < 0x00020000))
//
// Prior to UEFI 2.0 Tiano extended these enums. This was replaced by
// CreateEventEx() Event Groups in UEFI 2.0
//
#define EFI_EVENT_NOTIFY_SIGNAL_ALL     0x00000400

#define EFI_EVENT_SIGNAL_READY_TO_BOOT  0x00000203
#define EFI_EVENT_SIGNAL_LEGACY_BOOT    0x00000204
#endif

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_ADD_MEMORY_SPACE) (
  IN EFI_GCD_MEMORY_TYPE   GcdMemoryType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Capabilities
  )
/*++

Routine Description:

  Adds reserved memory, system memory, or memory-mapped I/O resources to the
global coherency domain of the processor.

Arguments:
    
  GcdMemoryType     - Memory type of the memory space.
  
  BaseAddress       - Base address of the memory space.
  
  Length            - Length of the memory space.
  
  Capabilities      - alterable attributes of the memory space.

Returns:

  EFI_SUCCESS       - Merged this memory space into GCD map.  

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_MEMORY_SPACE) (
  IN     EFI_GCD_ALLOCATE_TYPE               GcdAllocateType,
  IN     EFI_GCD_MEMORY_TYPE                 GcdMemoryType,
  IN     UINTN                               Alignment,
  IN     UINT64                              Length,
  IN OUT EFI_PHYSICAL_ADDRESS                * BaseAddress,
  IN     EFI_HANDLE                          ImageHandle,
  IN     EFI_HANDLE                          DeviceHandle OPTIONAL
  )
/*++

Routine Description:

  Allocates nonexistent memory, reserved memory, system memory, or memorymapped
I/O resources from the global coherency domain of the processor.

Arguments:
  
  GcdAllocateType   - The type of allocate operation
  
  GcdMemoryType     - The desired memory type
  
  Alignment         - Align with 2^Alignment
  
  Length            - Length to allocate
  
  BaseAddress       - Base address to allocate
  
  Imagehandle       - The image handle consume the allocated space.
  
  DeviceHandle      - The device handle consume the allocated space.

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter.
  
  EFI_NOT_FOUND               - No descriptor contains the desired space.
  
  EFI_SUCCESS                 - Memory space successfully allocated.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_FREE_MEMORY_SPACE) (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:Routine Description:

  Frees nonexistent memory, reserved memory, system memory, or memory-mapped
I/O resources from the global coherency domain of the processor.

Arguments:
    
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.
  
Returns:

  EFI_SUCCESS       - Space successfully freed.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_REMOVE_MEMORY_SPACE) (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:Routine Description:

  Removes reserved memory, system memory, or memory-mapped I/O resources from
the global coherency domain of the processor.

Arguments:
    
  BaseAddress       - Base address of the memory space.
  
  Length            - Length of the memory space.
  
Returns:

  EFI_SUCCESS       - Successfully remove a segment of memory space.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_GET_MEMORY_SPACE_DESCRIPTOR) (
  IN  EFI_PHYSICAL_ADDRESS             BaseAddress,
  OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  * Descriptor
  )
/*++

Routine Description:

  Retrieves the descriptor for a memory region containing a specified address.

Arguments:

  BaseAddress       - Specified start address
  
  Descriptor        - Specified length

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter
  
  EFI_SUCCESS                 - Successfully get memory space descriptor.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_SET_MEMORY_SPACE_ATTRIBUTES) (
  IN EFI_PHYSICAL_ADDRESS         BaseAddress,
  IN UINT64                       Length,
  IN UINT64                       Attributes
  )
/*++

Routine Description:

  Modifies the attributes for a memory region in the global coherency domain of the
processor.

Arguments:

  BaseAddress       - Specified start address
  
  Length            - Specified length
  
  Attributes        - Specified attributes

Returns:

  EFI_SUCCESS       - Successfully set attribute of a segment of memory space.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_GET_MEMORY_SPACE_MAP) (
  OUT UINTN                            *NumberOfDescriptors,
  OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  **MemorySpaceMap
  )
/*++

Routine Description:

  Returns a map of the memory resources in the global coherency domain of the
processor.

Arguments:

  NumberOfDescriptors       - Number of descriptors.
  
  MemorySpaceMap            - Descriptor array

Returns:

  EFI_INVALID_PARAMETER     - Invalid parameter
  
  EFI_OUT_OF_RESOURCES      - No enough buffer to allocate
  
  EFI_SUCCESS               - Successfully get memory space map.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_ADD_IO_SPACE) (
  IN EFI_GCD_IO_TYPE       GcdIoType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:

  Adds reserved I/O or I/O resources to the global coherency domain of the processor.

Arguments:
    
  GcdIoType         - IO type of the segment.
  
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.

Returns:

  EFI_SUCCESS       - Merged this segment into GCD map.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_IO_SPACE) (
  IN     EFI_GCD_ALLOCATE_TYPE               GcdAllocateType,
  IN     EFI_GCD_IO_TYPE                     GcdIoType,
  IN     UINTN                               Alignment,
  IN     UINT64                              Length,
  IN OUT EFI_PHYSICAL_ADDRESS                * BaseAddress,
  IN     EFI_HANDLE                          ImageHandle,
  IN     EFI_HANDLE                          DeviceHandle OPTIONAL
  )
/*++

Routine Description:

  Allocates nonexistent I/O, reserved I/O, or I/O resources from the global coherency
domain of the processor.

Arguments:
  
  GcdAllocateType   - The type of allocate operation
  
  GcdIoType         - The desired IO type
  
  Alignment         - Align with 2^Alignment
  
  Length            - Length to allocate
  
  BaseAddress       - Base address to allocate
  
  Imagehandle       - The image handle consume the allocated space.
  
  DeviceHandle      - The device handle consume the allocated space.

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter.
  
  EFI_NOT_FOUND               - No descriptor contains the desired space.
  
  EFI_SUCCESS                 - IO space successfully allocated.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_FREE_IO_SPACE) (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:Routine Description:

  Frees nonexistent I/O, reserved I/O, or I/O resources from the global coherency
domain of the processor.

Arguments:
    
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.
  
Returns:

  EFI_SUCCESS       - Space successfully freed.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_REMOVE_IO_SPACE) (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:Routine Description:

  Removes reserved I/O or I/O resources from the global coherency domain of the
processor.

Arguments:
    
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.
  
Returns:

  EFI_SUCCESS       - Successfully removed a segment of IO space.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_GET_IO_SPACE_DESCRIPTOR) (
  IN  EFI_PHYSICAL_ADDRESS         BaseAddress,
  OUT EFI_GCD_IO_SPACE_DESCRIPTOR  * Descriptor
  )
/*++

Routine Description:

  Retrieves the descriptor for an I/O region containing a specified address.

Arguments:

  BaseAddress       - Specified start address
  
  Descriptor        - Specified length

Returns:

  EFI_INVALID_PARAMETER       - Descriptor is NULL.
  
  EFI_SUCCESS                 - Successfully get the IO space descriptor.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_GET_IO_SPACE_MAP) (
  OUT UINTN                        *NumberOfDescriptors,
  OUT EFI_GCD_IO_SPACE_DESCRIPTOR  **IoSpaceMap
  )
/*++

Routine Description:

  Returns a map of the I/O resources in the global coherency domain of the processor.

Arguments:

  NumberOfDescriptors       - Number of descriptors.
  
  MemorySpaceMap            - Descriptor array

Returns:

  EFI_INVALID_PARAMETER     - Invalid parameter
  
  EFI_OUT_OF_RESOURCES      - No enough buffer to allocate
  
  EFI_SUCCESS               - Successfully get IO space map.

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_DISPATCH) (VOID)
/*++

Routine Description:

  Loads and executed DXE drivers from firmware volumes.

Arguments:

  None

Returns:

  Status code

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_SCHEDULE) (
  IN EFI_HANDLE  FirmwareVolumeHandle,
  IN EFI_GUID    * DriverName
  )
/*++

Routine Description:

  Clears the Schedule on Request (SOR) flag for a component that is stored in a firmware volume.

Arguments:

  FirmwareVolumeHandle  - The handle of the firmware volume that contains the file specified by FileName.

  DriverName            - A pointer to the name of the file in a firmware volume. 
  
Returns:

  Status code

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_TRUST) (
  IN EFI_HANDLE  FirmwareVolumeHandle,
  IN EFI_GUID    * DriverName
  )
/*++

Routine Description:

  Promotes a file stored in a firmware volume from the untrusted to the trusted state.

Arguments:

  FirmwareVolumeHandle  - The handle of the firmware volume that contains the file specified by FileName.

  DriverName            - A pointer to the name of the file in a firmware volume. 
  
Returns:

  Status code

--*/
;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_PROCESS_FIRMWARE_VOLUME) (
  IN VOID                             *FvHeader,
  IN UINTN                            Size,
  OUT EFI_HANDLE                      * FirmwareVolumeHandle
  )
/*++

Routine Description:

  Creates a firmware volume handle for a firmware volume that is present in system memory.

Arguments:

  FirmwareVolumeHeader    - A pointer to the header of the firmware volume.
  Size                    - The size, in bytes, of the firmware volume.
  FirmwareVolumeHandle    - On output, a pointer to the created handle.
  
Returns:

  Status code

--*/
;

//
// DXE Services Table
//
#define EFI_DXE_SERVICES_SIGNATURE  0x565245535f455844
#if (PI_SPECIFICATION_VERSION < 0x00010000)
#define EFI_DXE_SERVICES_REVISION   ((0 << 16) | (90))
#else
#define EFI_DXE_SERVICES_REVISION   ((1 << 16) | (00))
#endif

typedef struct {
  EFI_TABLE_HEADER                Hdr;

  //
  // Global Coherency Domain Services
  //
  EFI_ADD_MEMORY_SPACE            AddMemorySpace;
  EFI_ALLOCATE_MEMORY_SPACE       AllocateMemorySpace;
  EFI_FREE_MEMORY_SPACE           FreeMemorySpace;
  EFI_REMOVE_MEMORY_SPACE         RemoveMemorySpace;
  EFI_GET_MEMORY_SPACE_DESCRIPTOR GetMemorySpaceDescriptor;
  EFI_SET_MEMORY_SPACE_ATTRIBUTES SetMemorySpaceAttributes;
  EFI_GET_MEMORY_SPACE_MAP        GetMemorySpaceMap;
  EFI_ADD_IO_SPACE                AddIoSpace;
  EFI_ALLOCATE_IO_SPACE           AllocateIoSpace;
  EFI_FREE_IO_SPACE               FreeIoSpace;
  EFI_REMOVE_IO_SPACE             RemoveIoSpace;
  EFI_GET_IO_SPACE_DESCRIPTOR     GetIoSpaceDescriptor;
  EFI_GET_IO_SPACE_MAP            GetIoSpaceMap;

  //
  // Dispatcher Services
  //
  EFI_DISPATCH                    Dispatch;
  EFI_SCHEDULE                    Schedule;
  EFI_TRUST                       Trust;
  //
  // Service to process a single firmware volume found in a capsule
  //
  EFI_PROCESS_FIRMWARE_VOLUME     ProcessFirmwareVolume;
} EFI_DXE_SERVICES;

#endif
