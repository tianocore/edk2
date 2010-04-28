/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeiHobLib.h

Abstract:


--*/

#ifndef _EFI_PEI_HOB_LIB_H_
#define _EFI_PEI_HOB_LIB_H_

#include "PeiApi.h" // EFI_PEI_SERVICES definition
#define EFI_STACK_SIZE      0x20000
#define EFI_BSP_STORE_SIZE  0x4000

EFI_STATUS
BuildHobHandoffInfoTable (
  IN  VOID                    *HobStart,
  IN  UINT16                  Version,
  IN  EFI_BOOT_MODE           BootMode,
  IN  EFI_PHYSICAL_ADDRESS    EfiMemoryTop,
  IN  EFI_PHYSICAL_ADDRESS    EfiMemoryBottom,
  IN  EFI_PHYSICAL_ADDRESS    EfiFreeMemoryTop,
  IN  EFI_PHYSICAL_ADDRESS    EfiFreeMemoryBottom
  )
/*++

Routine Description:

  Builds a HandoffInformationTable Information Table HOB

Arguments:

  HobStart      - Start pointer of hob list
  Version       - The version number pertaining to the PHIT HOB definition.
  BootMode      - The system boot mode as determined during the HOB producer phase.
  EfiMemoryTop  - The highest address location of memory that is allocated for use by the HOB
                  producer phase.
  EfiMemoryBottom   - The lowest address location of memory that is allocated for use by the HOB
                      producer phase.
  EfiFreeMemoryTop  - The highest address location of free memory that is currently available for use
                      by the HOB producer phase.
  EfiFreeMemoryBottom   - The lowest address location of free memory that is available for 
                          use by the HOB producer phase.

Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
BuildHobModule (
  IN VOID                   *HobStart,
  IN EFI_GUID               *ModuleName,
  IN EFI_PHYSICAL_ADDRESS   Module,
  IN UINT64                 ModuleLength,
  IN EFI_PHYSICAL_ADDRESS   EntryPoint
  )
/*++

Routine Description:

  Builds a HOB for a loaded PE32 module

Arguments:

  HobStart                  - Start pointer of hob list

  ModuleName                - The GUID File Name of the HON from the Firmware Volume

  Module                    - The 64 bit physical address of the module

  ModuleLength              - The length of the module in bytes

  EntryPoint                - The 64 bit physical address of the entry point to the module

Returns:

  EFI_SUCCESS
  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
BuildHobResourceDescriptor (
  IN VOID                        *HobStart,
  IN EFI_RESOURCE_TYPE           ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS        PhysicalStart,
  IN UINT64                      NumberOfBytes
  )
/*++

Routine Description:

  Builds a HOB that describes a chunck of system memory

Arguments:

  HobStart          - Start pointer of hob list

  ResourceType      - The type of memory described by this HOB

  ResourceAttribute - The memory attributes of the memory described by this HOB

  PhysicalStart     - The 64 bit physical address of memory described by this HOB

  NumberOfBytes     - The length of the memoty described by this HOB in bytes

Returns:

  EFI_SUCCESS
  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
BuildHobGuidType (
  IN VOID                        *HobStart,
  IN EFI_GUID                    *Guid,
  IN VOID                        *Buffer,
  IN UINTN                       BufferSize
  )
/*++

Routine Description:

  Builds a custom HOB that is tagged with a GUID for identification

Arguments:

  HobStart    - Start pointer of hob list

  Guid        - The GUID of the custome HOB type

  Buffer      - A pointer to the data for the custom HOB type

  BufferSize  - The size in byte of BufferSize

Returns:

  EFI_SUCCESS
  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
BuildHobFvDescriptor (
  IN VOID                        *HobStart,
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  )
/*++

Routine Description:

  Builds a Firmware Volume HOB

Arguments:

  HobStart    - Start pointer of hob list

  BaseAddress - The base address of the Firmware Volume

  Length      - The size of the Firmware Volume in bytes

Returns:

  EFI_SUCCESS
  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
BuildHobCpu (
  IN VOID                        *HobStart,
  IN UINT8                       SizeOfMemorySpace,
  IN UINT8                       SizeOfIoSpace
  )
/*++

Routine Description:

  Builds a HOB for the CPU

Arguments:

  HobStart                  - Start pointer of hob list

  SizeOfMemorySpace         - Identifies the maximum 
                              physical memory addressibility of the processor.

  SizeOfIoSpace             - Identifies the maximum physical I/O addressibility 
                              of the processor.

Returns:

  EFI_SUCCESS
  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
BuildHobStack (
  IN VOID                        *HobStart,
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  )
/*++

Routine Description:

  Builds a HOB for the Stack

Arguments:

  HobStart                  - Start pointer of hob list

  BaseAddress               - The 64 bit physical address of the Stack

  Length                    - The length of the stack in bytes

Returns:

  EFI_SUCCESS
  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
BuildHobBspStore (
  IN VOID                        *HobStart,
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length,
  IN EFI_MEMORY_TYPE             MemoryType
  )
/*++

Routine Description:

  Builds a HOB for the bsp store

Arguments:

  HobStart                  - Start pointer of hob list

  BaseAddress               - The 64 bit physical address of bsp store

  Length                    - The length of the bsp store in bytes
  
  MemoryType                - Memory type of the bsp store

Returns:

  EFI_SUCCESS
  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
BuildMemoryAllocationHob (
  IN VOID                        *HobStart,
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length,
  IN EFI_GUID                    *Name,
  IN EFI_MEMORY_TYPE             MemoryType
  )
/*++

Routine Description:

  Builds a HOB for memory allocation

Arguments:

  HobStart                  - Start pointer of hob list

  BaseAddress               - The base address of memory allocated by this HOB.

  Length                    - The length in bytes of memory allocated by this HOB.

  Name                      - A GUID that defines the memory allocation region's type and purpose, 
                              as well as other fields within the memory allocation HOB.

  MemoryType                - Defines the type of memory allocated by this HOB.

Returns:

  EFI_SUCCESS
  EFI_NOT_AVAILABLE_YET

--*/
;

VOID *
GetHob (
  IN UINT16  Type,
  IN VOID    *HobStart
  )
/*++

Routine Description:

  This function returns the first instance of a HOB type in a HOB list.
  
Arguments:

  Type          The HOB type to return.
  HobStart      The first HOB in the HOB list.
    
Returns:

  HobStart      There were no HOBs found with the requested type.
  else          Returns the first HOB with the matching type.

--*/
;

EFI_STATUS
GetFirstGuidHob (
  IN     VOID      **HobStart,
  IN     EFI_GUID  *Guid,
  OUT    VOID      **Buffer,
  OUT    UINTN     *BufferSize  OPTIONAL
  ) 
/*++

Routine Description:

  This function searches the first instance of a HOB among the whole HOB list. 

Arguments:

  HobStart                  - A pointer to the start pointer of hob list.

  Guid                      - A pointer to the GUID to match with in the HOB list.

  Buffer                    - A pointer to the pointer to the data for the custom HOB type.

  BufferSize                - A Pointer to the size in byte of BufferSize.

Returns:
  EFI_SUCCESS
  The first instance of the matched GUID HOB among the whole HOB list

--*/
;

EFI_STATUS
GetNextGuidHob (
  IN OUT VOID      **HobStart,
  IN     EFI_GUID  * Guid,
  OUT    VOID      **Buffer,
  OUT    UINTN     *BufferSize OPTIONAL
  )
/*++

Routine Description:
  Get the next guid hob.
  
Arguments:
  HobStart        A pointer to the start hob.
  Guid            A pointer to a guid.
  Buffer          A pointer to the buffer.
  BufferSize      Buffer size.
  
Returns:
  Status code.

  EFI_NOT_FOUND          - Next Guid hob not found
  
  EFI_SUCCESS            - Next Guid hob found and data for this Guid got
  
  EFI_INVALID_PARAMETER  - invalid parameter

--*/
;

#endif
