/** @file
  Hand Off Block (HOB) definition.

  The HOB is a memory data structure used to hand-off system information from
  PEI to DXE (the next phase).

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  Hob.h

  @par Revision Reference:
  These definitions are from Hand Off Block (HOB) Spec Version 0.9.

**/

#ifndef __HOB_H__
#define __HOB_H__


//
// Every Hob must start with this data structure.
//
typedef struct {
  UINT16  HobType;
  UINT16  HobLength;
  UINT32  Reserved;
} EFI_HOB_GENERIC_HEADER;

//
// End of HOB List HOB
//
#define EFI_HOB_TYPE_END_OF_HOB_LIST  0xffff

//
// Handoff Information Table HOB
//
#define EFI_HOB_TYPE_HANDOFF          0x0001

#define EFI_HOB_HANDOFF_TABLE_VERSION 0x0009

typedef UINT32  EFI_BOOT_MODE;

typedef struct {
  EFI_HOB_GENERIC_HEADER  Header;
  UINT32                  Version;
  EFI_BOOT_MODE           BootMode;
  EFI_PHYSICAL_ADDRESS    EfiMemoryTop;
  EFI_PHYSICAL_ADDRESS    EfiMemoryBottom;
  EFI_PHYSICAL_ADDRESS    EfiFreeMemoryTop;
  EFI_PHYSICAL_ADDRESS    EfiFreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS    EfiEndOfHobList;
} EFI_HOB_HANDOFF_INFO_TABLE;

//
// Memory Descriptor HOB
//
#define EFI_HOB_TYPE_MEMORY_ALLOCATION  0x0002

typedef struct {
  EFI_GUID              Name;
  EFI_PHYSICAL_ADDRESS  MemoryBaseAddress;
  UINT64                MemoryLength;
  EFI_MEMORY_TYPE       MemoryType;
  UINT8                 Reserved[4];
} EFI_HOB_MEMORY_ALLOCATION_HEADER;

typedef struct {
  EFI_HOB_GENERIC_HEADER            Header;
  EFI_HOB_MEMORY_ALLOCATION_HEADER  AllocDescriptor;
  //
  // Additional data pertaining to the "Name" Guid memory
  // may go here.
  //
} EFI_HOB_MEMORY_ALLOCATION;

typedef struct {
  EFI_HOB_GENERIC_HEADER            Header;
  EFI_HOB_MEMORY_ALLOCATION_HEADER  AllocDescriptor;
} EFI_HOB_MEMORY_ALLOCATION_BSP_STORE;

typedef struct {
  EFI_HOB_GENERIC_HEADER            Header;
  EFI_HOB_MEMORY_ALLOCATION_HEADER  AllocDescriptor;
} EFI_HOB_MEMORY_ALLOCATION_STACK;

typedef struct {
  EFI_HOB_GENERIC_HEADER            Header;
  EFI_HOB_MEMORY_ALLOCATION_HEADER  MemoryAllocationHeader;
  EFI_GUID                          ModuleName;
  EFI_PHYSICAL_ADDRESS              EntryPoint;
} EFI_HOB_MEMORY_ALLOCATION_MODULE;

#define EFI_HOB_TYPE_RESOURCE_DESCRIPTOR  0x0003

typedef UINT32  EFI_RESOURCE_TYPE;

#define EFI_RESOURCE_SYSTEM_MEMORY          0
#define EFI_RESOURCE_MEMORY_MAPPED_IO       1
#define EFI_RESOURCE_IO                     2
#define EFI_RESOURCE_FIRMWARE_DEVICE        3
#define EFI_RESOURCE_MEMORY_MAPPED_IO_PORT  4
#define EFI_RESOURCE_MEMORY_RESERVED        5
#define EFI_RESOURCE_IO_RESERVED            6
#define EFI_RESOURCE_MAX_MEMORY_TYPE        7

typedef UINT32  EFI_RESOURCE_ATTRIBUTE_TYPE;

#define EFI_RESOURCE_ATTRIBUTE_PRESENT                  0x00000001
#define EFI_RESOURCE_ATTRIBUTE_INITIALIZED              0x00000002
#define EFI_RESOURCE_ATTRIBUTE_TESTED                   0x00000004
#define EFI_RESOURCE_ATTRIBUTE_SINGLE_BIT_ECC           0x00000008
#define EFI_RESOURCE_ATTRIBUTE_MULTIPLE_BIT_ECC         0x00000010
#define EFI_RESOURCE_ATTRIBUTE_ECC_RESERVED_1           0x00000020
#define EFI_RESOURCE_ATTRIBUTE_ECC_RESERVED_2           0x00000040
#define EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED           0x00000080
#define EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED          0x00000100
#define EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED      0x00000200
#define EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE              0x00000400
#define EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE        0x00000800
#define EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE  0x00001000
#define EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE     0x00002000
#define EFI_RESOURCE_ATTRIBUTE_16_BIT_IO                0x00004000
#define EFI_RESOURCE_ATTRIBUTE_32_BIT_IO                0x00008000
#define EFI_RESOURCE_ATTRIBUTE_64_BIT_IO                0x00010000
#define EFI_RESOURCE_ATTRIBUTE_UNCACHED_EXPORTED        0x00020000

typedef struct {
  EFI_HOB_GENERIC_HEADER      Header;
  EFI_GUID                    Owner;
  EFI_RESOURCE_TYPE           ResourceType;
  EFI_RESOURCE_ATTRIBUTE_TYPE ResourceAttribute;
  EFI_PHYSICAL_ADDRESS        PhysicalStart;
  UINT64                      ResourceLength;
} EFI_HOB_RESOURCE_DESCRIPTOR;

//
// GUID Extension HOB
// The HobLength is variable as it includes the GUID specific data.
//
#define EFI_HOB_TYPE_GUID_EXTENSION 0x0004

typedef struct {
  EFI_HOB_GENERIC_HEADER  Header;
  EFI_GUID                Name;

  //
  // Guid specific data goes here
  //
} EFI_HOB_GUID_TYPE;

//
// Firmware Volume HOB
//
#define EFI_HOB_TYPE_FV 0x0005

typedef struct {
  EFI_HOB_GENERIC_HEADER  Header;
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  UINT64                  Length;
} EFI_HOB_FIRMWARE_VOLUME;

//
// CPU HOB
//
#define EFI_HOB_TYPE_CPU  0x0006

typedef struct {
  EFI_HOB_GENERIC_HEADER  Header;
  UINT8                   SizeOfMemorySpace;
  UINT8                   SizeOfIoSpace;
  UINT8                   Reserved[6];
} EFI_HOB_CPU;

//
// PEI Core Memory Pool HOB
// The HobLength is variable as the HOB contains pool allocations by
// the PeiServices AllocatePool function
//
#define EFI_HOB_TYPE_PEI_MEMORY_POOL  0x0007

typedef struct {
  EFI_HOB_GENERIC_HEADER  Header;
} EFI_HOB_MEMORY_POOL;

//
// Capsule volume HOB -- identical to a firmware volume
//
#define EFI_HOB_TYPE_CV 0x0008

typedef struct {
  EFI_HOB_GENERIC_HEADER  Header;
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  UINT64                  Length;
} EFI_HOB_CAPSULE_VOLUME;

#define EFI_HOB_TYPE_UNUSED 0xFFFE

//
// Union of all the possible HOB Types
//
typedef union {
  EFI_HOB_GENERIC_HEADER              *Header;
  EFI_HOB_HANDOFF_INFO_TABLE          *HandoffInformationTable;
  EFI_HOB_MEMORY_ALLOCATION           *MemoryAllocation;
  EFI_HOB_MEMORY_ALLOCATION_BSP_STORE *MemoryAllocationBspStore;
  EFI_HOB_MEMORY_ALLOCATION_STACK     *MemoryAllocationStack;
  EFI_HOB_MEMORY_ALLOCATION_MODULE    *MemoryAllocationModule;
  EFI_HOB_RESOURCE_DESCRIPTOR         *ResourceDescriptor;
  EFI_HOB_GUID_TYPE                   *Guid;
  EFI_HOB_FIRMWARE_VOLUME             *FirmwareVolume;
  EFI_HOB_CPU                         *Cpu;
  EFI_HOB_MEMORY_POOL                 *Pool;
  EFI_HOB_CAPSULE_VOLUME              *CapsuleVolume;
  UINT8                               *Raw;
} EFI_PEI_HOB_POINTERS;

#define GET_HOB_TYPE(Hob)     ((Hob).Header->HobType)
#define GET_HOB_LENGTH(Hob)   ((Hob).Header->HobLength)
#define GET_NEXT_HOB(Hob)     ((Hob).Raw + GET_HOB_LENGTH (Hob))
#define END_OF_HOB_LIST(Hob)  (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_END_OF_HOB_LIST)

//
// Get the data and data size field of GUID 
//
#define GET_GUID_HOB_DATA(GuidHob)      ((VOID *) (((UINT8 *) &((GuidHob)->Name)) + sizeof (EFI_GUID)))
#define GET_GUID_HOB_DATA_SIZE(GuidHob) (((GuidHob)->Header).HobLength - sizeof (EFI_HOB_GUID_TYPE))

#endif
