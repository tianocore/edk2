/* @file
	HOB related definitions in PI.

	Copyright (c) 2006 - 2007, Intel Corporation                                                         
	All rights reserved. This program and the accompanying materials                          
	are licensed and made available under the terms and conditions of the BSD License         
	which accompanies this distribution.  The full text of the license may be found at        
	http://opensource.org/licenses/bsd-license.php                                            

	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

	Module Name:  PiHob.h

	@par Revision Reference:
	Version 1.0.

**/

#ifndef __PI_HOB_H__
#define __PI_HOB_H__

//
// HobType of EFI_HOB_GENERIC_HEADER.
// 
#define EFI_HOB_TYPE_HANDOFF 							0x0001
#define EFI_HOB_TYPE_MEMORY_ALLOCATION 		0x0002
#define EFI_HOB_TYPE_RESOURCE_DESCRIPTOR 	0x0003
#define EFI_HOB_TYPE_GUID_EXTENSION 			0x0004
#define EFI_HOB_TYPE_FV 									0x0005
#define EFI_HOB_TYPE_CPU 									0x0006
#define EFI_HOB_TYPE_MEMORY_POOL 					0x0007
#define EFI_HOB_TYPE_FV2 									0x0009
#define EFI_HOB_TYPE_LOAD_PEIM 						0x000A
#define EFI_HOB_TYPE_UNUSED 							0xFFFE
#define EFI_HOB_TYPE_END_OF_HOB_LIST 			0xFFFF

//
// Describes the format and size of the data inside the HOB. 
// All HOBs must contain this generic HOB header.
// 
typedef struct _EFI_HOB_GENERIC_HEADER {
	UINT16 HobType;
	UINT16 HobLength;
	UINT32 Reserved;
} EFI_HOB_GENERIC_HEADER;


//
// Value of version ofinEFI_HOB_HANDOFF_INFO_TABLE.
// 
#define EFI_HOB_HANDOFF_TABLE_VERSION 0x0009
//
// Contains general state information used by the HOB producer phase. 
// This HOB must be the first one in the HOB list.
// 
typedef struct _EFI_HOB_HANDOFF_INFO_TABLE {
	EFI_HOB_GENERIC_HEADER  Header;
	UINT32                Version;
	EFI_BOOT_MODE         BootMode;
	EFI_PHYSICAL_ADDRESS  EfiMemoryTop;
	EFI_PHYSICAL_ADDRESS  EfiMemoryBottom;
	EFI_PHYSICAL_ADDRESS  EfiFreeMemoryTop;
	EFI_PHYSICAL_ADDRESS  EfiFreeMemoryBottom;
	EFI_PHYSICAL_ADDRESS  EfiEndOfHobList;
} EFI_HOB_HANDOFF_INFO_TABLE;


typedef struct _EFI_HOB_MEMORY_ALLOCATION_HEADER {
	EFI_GUID              Name;
	EFI_PHYSICAL_ADDRESS  MemoryBaseAddress;
	UINT64                MemoryLength;
	EFI_MEMORY_TYPE       MemoryType;

	//
	// Padding for Itanium processor family
	// 
	UINT8                 Reserved[4];
} EFI_HOB_MEMORY_ALLOCATION_HEADER;

//
// Describes all memory ranges used during the HOB producer 
// phase that exist outside the HOB list. This HOB type 
// describes how memory is used, 
// not the physical attributes of memory.
// 
typedef struct _EFI_HOB_MEMORY_ALLOCATION {
	EFI_HOB_GENERIC_HEADER            Header;
	EFI_HOB_MEMORY_ALLOCATION_HEADER  AllocDescriptor;
	//
	// Additional data pertaining to the ¡°Name¡± Guid memory
	// may go here.
	//
} EFI_HOB_MEMORY_ALLOCATION;


//
// Describes the memory stack that is produced by the HOB producer 
// phase and upon which all postmemory-installed executable
// content in the HOB producer phase is executing.
// 
typedef struct _EFI_HOB_MEMORY_ALLOCATION_STACK {
	EFI_HOB_GENERIC_HEADER            Header;
	EFI_HOB_MEMORY_ALLOCATION_HEADER  AllocDescriptor;
} EFI_HOB_MEMORY_ALLOCATION_STACK;

//
// Defines the location of the boot-strap 
// processor (BSP) BSPStore (¡°Backing Store Pointer Store¡±).
// This HOB is valid for the Itanium processor family only 
// register overflow store.
// 
typedef struct _EFI_HOB_MEMORY_ALLOCATION_BSP_STORE {
	EFI_HOB_GENERIC_HEADER            Header;
	EFI_HOB_MEMORY_ALLOCATION_HEADER  AllocDescriptor;
} EFI_HOB_MEMORY_ALLOCATION_BSP_STORE;

//
// Defines the location and entry point of the HOB consumer phase.
// 
typedef struct {
	EFI_HOB_GENERIC_HEADER            Header;
	EFI_HOB_MEMORY_ALLOCATION_HEADER  MemoryAllocationHeader;
	EFI_GUID ModuleName;
	EFI_PHYSICAL_ADDRESS EntryPoint;
} EFI_HOB_MEMORY_ALLOCATION_MODULE;

typedef UINT32 EFI_RESOURCE_TYPE;

//
// Value of ResourceType in EFI_HOB_RESOURCE_DESCRIPTOR.
// 
#define EFI_RESOURCE_SYSTEM_MEMORY 					0x00000000
#define EFI_RESOURCE_MEMORY_MAPPED_IO 			0x00000001
#define EFI_RESOURCE_IO 										0x00000002
#define EFI_RESOURCE_FIRMWARE_DEVICE 				0x00000003
#define EFI_RESOURCE_MEMORY_MAPPED_IO_PORT 	0x00000004
#define EFI_RESOURCE_MEMORY_RESERVED 				0x00000005
#define EFI_RESOURCE_IO_RESERVED 						0x00000006
#define EFI_RESOURCE_MAX_MEMORY_TYPE 				0x00000007


typedef UINT32 EFI_RESOURCE_ATTRIBUTE_TYPE;

//
// These types can be ORed together as needed.
//
// The first three enumerations describe settings
//
#define EFI_RESOURCE_ATTRIBUTE_PRESENT 							0x00000001
#define EFI_RESOURCE_ATTRIBUTE_INITIALIZED 					0x00000002
#define EFI_RESOURCE_ATTRIBUTE_TESTED 							0x00000004
//
// The rest of the settings describe capabilities
//
#define EFI_RESOURCE_ATTRIBUTE_SINGLE_BIT_ECC 			0x00000008
#define EFI_RESOURCE_ATTRIBUTE_MULTIPLE_BIT_ECC 		0x00000010
#define EFI_RESOURCE_ATTRIBUTE_ECC_RESERVED_1 			0x00000020
#define EFI_RESOURCE_ATTRIBUTE_ECC_RESERVED_2 			0x00000040
#define EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED 			0x00000080
#define EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED 			0x00000100
#define EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED 	0x00000200
#define EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE 					0x00000400
#define EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE 		0x00000800
#define EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE	0x00001000
#define EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE 0x00002000
#define EFI_RESOURCE_ATTRIBUTE_16_BIT_IO 						0x00004000
#define EFI_RESOURCE_ATTRIBUTE_32_BIT_IO 						0x00008000
#define EFI_RESOURCE_ATTRIBUTE_64_BIT_IO 						0x00010000
#define EFI_RESOURCE_ATTRIBUTE_UNCACHED_EXPORTED 		0x00020000

//
// Describes the resource properties of all fixed, 
// nonrelocatable resource ranges found on the processor
// host bus during the HOB producer phase.
// 
typedef struct _EFI_HOB_RESOURCE_DESCRIPTOR {
	EFI_HOB_GENERIC_HEADER      Header;
	EFI_GUID                    Owner;
	EFI_RESOURCE_TYPE           ResourceType;
	EFI_RESOURCE_ATTRIBUTE_TYPE ResourceAttribute;
	EFI_PHYSICAL_ADDRESS        PhysicalStart;
	UINT64                      ResourceLength;
} EFI_HOB_RESOURCE_DESCRIPTOR;

//
// Allows writers of executable content in the HOB producer phase to 
// maintain and manage HOBs with specific GUID.
// 
typedef struct _EFI_HOB_GUID_TYPE {
	EFI_HOB_GENERIC_HEADER      Header;
	EFI_GUID                    Name;

	//
	// Guid specific data goes here
	//
} EFI_HOB_GUID_TYPE;

//
// Details the location of firmware volumes that contain firmware files.
// 
typedef struct {
	EFI_HOB_GENERIC_HEADER Header;
	EFI_PHYSICAL_ADDRESS BaseAddress;
	UINT64 Length;
} EFI_HOB_FIRMWARE_VOLUME;

//
// Details the location of a firmware volume which was extracted 
// from a file within another firmware volume.
// 
typedef struct {
	EFI_HOB_GENERIC_HEADER  Header;
	EFI_PHYSICAL_ADDRESS    BaseAddress;
	UINT64                  Length;
	EFI_GUID                FvName;
	EFI_GUID                FileName;
} EFI_HOB_FIRMWARE_VOLUME2;


//
// Describes processor information, such as address space and I/O space capabilities.
// 
typedef struct _EFI_HOB_CPU {
	EFI_HOB_GENERIC_HEADER 	Header;
	UINT8 									SizeOfMemorySpace;
	UINT8 									SizeOfIoSpace;
	UINT8 									Reserved[6];
} EFI_HOB_CPU;


//
// Describes pool memory allocations.
// 
typedef struct _EFI_HOB_MEMORY_POOL {
	EFI_HOB_GENERIC_HEADER 	Header;
} EFI_HOB_MEMORY_POOL;

//
// Union of all the possible HOB Types
//
typedef union {
	EFI_HOB_GENERIC_HEADER 							*Header;
	EFI_HOB_HANDOFF_INFO_TABLE 					*HandoffInformationTable;
	EFI_HOB_MEMORY_ALLOCATION 					*MemoryAllocation;
	EFI_HOB_MEMORY_ALLOCATION_BSP_STORE *MemoryAllocationBspStore;
	EFI_HOB_MEMORY_ALLOCATION_STACK 		*MemoryAllocationStack;
	EFI_HOB_MEMORY_ALLOCATION_MODULE 		*MemoryAllocationModule;
	EFI_HOB_RESOURCE_DESCRIPTOR 				*ResourceDescriptor;
	EFI_HOB_GUID_TYPE 									*Guid;
	EFI_HOB_FIRMWARE_VOLUME 						*FirmwareVolume;
	EFI_HOB_CPU 												*Cpu;
	EFI_HOB_MEMORY_POOL 								*Pool;
	UINT8 															*Raw;
} EFI_PEI_HOB_POINTERS;


#endif 
