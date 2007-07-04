/** @file
  Include file matches things in PI.

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  Version 1.0.

**/

#ifndef __PI_DXECIS_H__
#define __PI_DXECIS_H__

#include <Pi/PiMultiPhase.h>

//
// Global Coherencey Domain types
//
typedef enum {
  EfiGcdMemoryTypeNonExistent,
  EfiGcdMemoryTypeReserved,
  EfiGcdMemoryTypeSystemMemory,
  EfiGcdMemoryTypeMemoryMappedIo,
  EfiGcdMemoryTypeMaximum
} EFI_GCD_MEMORY_TYPE;


typedef enum {
  EfiGcdIoTypeNonExistent,
  EfiGcdIoTypeReserved,
  EfiGcdIoTypeIo,
  EfiGcdIoTypeMaximum
} EFI_GCD_IO_TYPE;


typedef enum {
  EfiGcdAllocateAnySearchBottomUp,
  EfiGcdAllocateMaxAddressSearchBottomUp,
  EfiGcdAllocateAddress,
  EfiGcdAllocateAnySearchTopDown,
  EfiGcdAllocateMaxAddressSearchTopDown,
  EfiGcdMaxAllocateType
} EFI_GCD_ALLOCATE_TYPE;


typedef struct {
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  UINT64                Length;
  UINT64                Capabilities;
  UINT64                Attributes;
  EFI_GCD_MEMORY_TYPE   GcdMemoryType;
  EFI_HANDLE            ImageHandle;
  EFI_HANDLE            DeviceHandle;
} EFI_GCD_MEMORY_SPACE_DESCRIPTOR;


typedef struct {
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  UINT64                Length;
  EFI_GCD_IO_TYPE       GcdIoType;
  EFI_HANDLE            ImageHandle;
  EFI_HANDLE            DeviceHandle;
} EFI_GCD_IO_SPACE_DESCRIPTOR;


/**
  Adds reserved memory, system memory, or memory-mapped I/O resources to the
  global coherency domain of the processor.

  @param  GcdMemoryType    Memory type of the memory space.
  @param  BaseAddress      Base address of the memory space.
  @param  Length           Length of the memory space.
  @param  Capabilities     alterable attributes of the memory space.

  @retval EFI_SUCCESS           Merged this memory space into GCD map.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ADD_MEMORY_SPACE) (
  IN EFI_GCD_MEMORY_TYPE   GcdMemoryType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Capabilities
  )
;

/**
  Allocates nonexistent memory, reserved memory, system memory, or memorymapped
  I/O resources from the global coherency domain of the processor.

  @param  GcdAllocateType  The type of allocate operation
  @param  GcdMemoryType    The desired memory type
  @param  Alignment        Align with 2^Alignment
  @param  Length           Length to allocate
  @param  BaseAddress      Base address to allocate
  @param  Imagehandle      The image handle consume the allocated space.
  @param  DeviceHandle     The device handle consume the allocated space.

  @retval EFI_INVALID_PARAMETER Invalid parameter.
  @retval EFI_NOT_FOUND         No descriptor contains the desired space.
  @retval EFI_SUCCESS           Memory space successfully allocated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_MEMORY_SPACE) (
  IN     EFI_GCD_ALLOCATE_TYPE               GcdAllocateType,
  IN     EFI_GCD_MEMORY_TYPE                 GcdMemoryType,
  IN     UINTN                               Alignment,
  IN     UINT64                              Length,
  IN OUT EFI_PHYSICAL_ADDRESS                *BaseAddress,
  IN     EFI_HANDLE                          ImageHandle,
  IN     EFI_HANDLE                          DeviceHandle OPTIONAL
  )
;

/**
  Frees nonexistent memory, reserved memory, system memory, or memory-mapped
  I/O resources from the global coherency domain of the processor.

  @param  BaseAddress      Base address of the segment.
  @param  Length           Length of the segment.

  @retval EFI_SUCCESS           Space successfully freed.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FREE_MEMORY_SPACE) (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
;

/**
  Removes reserved memory, system memory, or memory-mapped I/O resources from
  the global coherency domain of the processor.

  @param  BaseAddress      Base address of the memory space.
  @param  Length           Length of the memory space.

  @retval EFI_SUCCESS           Successfully remove a segment of memory space.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_REMOVE_MEMORY_SPACE) (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
;

/**
  Retrieves the descriptor for a memory region containing a specified address.

  @param  BaseAddress      Specified start address
  @param  Descriptor       Specified length

  @retval EFI_INVALID_PARAMETER Invalid parameter
  @retval EFI_SUCCESS           Successfully get memory space descriptor.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_MEMORY_SPACE_DESCRIPTOR) (
  IN  EFI_PHYSICAL_ADDRESS             BaseAddress,
  OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor
  )
;

/**
  Modifies the attributes for a memory region in the global coherency domain of the
  processor.

  @param  BaseAddress      Specified start address
  @param  Length           Specified length
  @param  Attributes       Specified attributes

  @retval EFI_SUCCESS           Successfully set attribute of a segment of memory space.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SET_MEMORY_SPACE_ATTRIBUTES) (
  IN EFI_PHYSICAL_ADDRESS         BaseAddress,
  IN UINT64                       Length,
  IN UINT64                       Attributes
  )
;

/**
  Returns a map of the memory resources in the global coherency domain of the
  processor.

  @param  NumberOfDescriptors Number of descriptors.
  @param  MemorySpaceMap      Descriptor array

  @retval EFI_INVALID_PARAMETER Invalid parameter
  @retval EFI_OUT_OF_RESOURCES  No enough buffer to allocate
  @retval EFI_SUCCESS           Successfully get memory space map.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_MEMORY_SPACE_MAP) (
  OUT UINTN                            *NumberOfDescriptors,
  OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  **MemorySpaceMap
  )
;

/**
  Adds reserved I/O or I/O resources to the global coherency domain of the processor.

  @param  GcdIoType        IO type of the segment.
  @param  BaseAddress      Base address of the segment.
  @param  Length           Length of the segment.

  @retval EFI_SUCCESS           Merged this segment into GCD map.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ADD_IO_SPACE) (
  IN EFI_GCD_IO_TYPE       GcdIoType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
;

/**
  Allocates nonexistent I/O, reserved I/O, or I/O resources from the global coherency
  domain of the processor.

  @param  GcdAllocateType  The type of allocate operation
  @param  GcdIoType        The desired IO type
  @param  Alignment        Align with 2^Alignment
  @param  Length           Length to allocate
  @param  BaseAddress      Base address to allocate
  @param  Imagehandle      The image handle consume the allocated space.
  @param  DeviceHandle     The device handle consume the allocated space.

  @retval EFI_INVALID_PARAMETER Invalid parameter.
  @retval EFI_NOT_FOUND         No descriptor contains the desired space.
  @retval EFI_SUCCESS           IO space successfully allocated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_IO_SPACE) (
  IN     EFI_GCD_ALLOCATE_TYPE               GcdAllocateType,
  IN     EFI_GCD_IO_TYPE                     GcdIoType,
  IN     UINTN                               Alignment,
  IN     UINT64                              Length,
  IN OUT EFI_PHYSICAL_ADDRESS                *BaseAddress,
  IN     EFI_HANDLE                          ImageHandle,
  IN     EFI_HANDLE                          DeviceHandle OPTIONAL
  )
;

/**
  Frees nonexistent I/O, reserved I/O, or I/O resources from the global coherency
  domain of the processor.

  @param  BaseAddress      Base address of the segment.
  @param  Length           Length of the segment.

  @retval EFI_SUCCESS           Space successfully freed.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_FREE_IO_SPACE) (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
;

/**
  Removes reserved I/O or I/O resources from the global coherency domain of the
  processor.

  @param  BaseAddress      Base address of the segment.
  @param Length Length of the segment.

  @retval EFI_SUCCESS           Successfully removed a segment of IO space.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_REMOVE_IO_SPACE) (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
;

/**
  Retrieves the descriptor for an I/O region containing a specified address.

  @param  BaseAddress      Specified start address
  @param  Descriptor       Specified length

  @retval EFI_INVALID_PARAMETER Descriptor is NULL.
  @retval EFI_SUCCESS           Successfully get the IO space descriptor.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_IO_SPACE_DESCRIPTOR) (
  IN  EFI_PHYSICAL_ADDRESS         BaseAddress,
  OUT EFI_GCD_IO_SPACE_DESCRIPTOR  *Descriptor
  )
;

/**
  Returns a map of the I/O resources in the global coherency domain of the processor.

  @param  NumberOfDescriptors Number of descriptors.
  @param  MemorySpaceMap      Descriptor array

  @retval EFI_INVALID_PARAMETER Invalid parameter
  @retval EFI_OUT_OF_RESOURCES  No enough buffer to allocate
  @retval EFI_SUCCESS           Successfully get IO space map.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_IO_SPACE_MAP) (
  OUT UINTN                        *NumberOfDescriptors,
  OUT EFI_GCD_IO_SPACE_DESCRIPTOR  **IoSpaceMap
  )
;



/**
  Loads and executed DXE drivers from firmware volumes.

  @return Status code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DISPATCH) (
  VOID
  )
;

/**
  Clears the Schedule on Request (SOR) flag for a component that is stored in a firmware volume.

  @param  FirmwareVolumeHandle The handle of the firmware volume that contains the file specified by FileName.
  @param  DriverName           A pointer to the name of the file in a firmware volume.

  @return Status code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SCHEDULE) (
  IN EFI_HANDLE  FirmwareVolumeHandle,
  IN EFI_GUID    *DriverName
  )
;

/**
  Promotes a file stored in a firmware volume from the untrusted to the trusted state.

  @param  FirmwareVolumeHandle The handle of the firmware volume that contains the file specified by FileName.
  @param  DriverName           A pointer to the name of the file in a firmware volume.

  @return Status code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TRUST) (
  IN EFI_HANDLE  FirmwareVolumeHandle,
  IN EFI_GUID    *DriverName
  )
;

/**
  Creates a firmware volume handle for a firmware volume that is present in system memory.

  @param  FirmwareVolumeHeader A pointer to the header of the firmware volume.
  @param  Size                 The size, in bytes, of the firmware volume.
  @param  FirmwareVolumeHandle On output, a pointer to the created handle.

  @return Status code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PROCESS_FIRMWARE_VOLUME) (
  IN VOID                             *FvHeader,
  IN UINTN                            Size,
  OUT EFI_HANDLE                      *FirmwareVolumeHandle
  )
;

//
// DXE Services Table
//
#define DXE_SERVICES_SIGNATURE  0x565245535f455844
#define DXE_SERVICES_REVISION   ((1<<16) | (00))

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
} DXE_SERVICES;

typedef DXE_SERVICES EFI_DXE_SERVICES;

#endif
