
/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

MemoryAllocationModule Name:

  Peihoblib.c

Abstract:

  PEI Library Functions
 
--*/

#include "Tiano.h"
#include "PeiHob.h"
#include "PeiHobLib.h"
#include "PeiLib.h"
#include EFI_GUID_DEFINITION(MemoryAllocationHob)


EFI_PEI_HOB_POINTERS
BuildHobEndOfHobList (
  IN  VOID  *HobStart
  )
/*++

Routine Description:

  Builds an end of HOB list HOB

Arguments:

  HobStart    - The HOB to build

Returns:

  A pointer to the next HOB

--*/
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = HobStart;
  
  Hob.Header->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  Hob.Header->HobLength = (UINT16) sizeof (EFI_HOB_GENERIC_HEADER);

  Hob.Header++;
  return Hob;
}

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
{
  EFI_PEI_HOB_POINTERS        HandOffHob;
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PEI_HOB_POINTERS        HobEnd;
  

  HandOffHob.Raw     = HobStart;
  Hob.Raw            = HobStart;
  Hob.Header->HobType   = EFI_HOB_TYPE_HANDOFF;
  Hob.Header->HobLength = (UINT16) sizeof (EFI_HOB_HANDOFF_INFO_TABLE);

  Hob.HandoffInformationTable->Version        = Version;
  Hob.HandoffInformationTable->BootMode       = BootMode;
  
  Hob.HandoffInformationTable->EfiMemoryTop     = EfiMemoryTop;
  Hob.HandoffInformationTable->EfiMemoryBottom  = EfiMemoryBottom;
  Hob.HandoffInformationTable->EfiFreeMemoryTop = EfiFreeMemoryTop;
  Hob.HandoffInformationTable->EfiFreeMemoryBottom = EfiFreeMemoryBottom;
 
  HobEnd.Raw = (VOID*)(Hob.HandoffInformationTable + 1);
  Hob.HandoffInformationTable->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS) (UINTN) HobEnd.Raw;
  Hob = BuildHobEndOfHobList (HobEnd.Raw);
  HandOffHob.HandoffInformationTable->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  return EFI_SUCCESS;
}



EFI_STATUS
BuildHobModule (
  IN VOID                   *HobStart,
  IN EFI_GUID               *ModuleName,
  IN EFI_PHYSICAL_ADDRESS   MemoryAllocationModule,
  IN UINT64                 ModuleLength,
  IN EFI_PHYSICAL_ADDRESS   EntryPoint
  )
/*++

Routine Description:

  Builds a HOB for a loaded PE32 module

Arguments:

  HobStart                  - Start pointer of hob list

  ModuleName                - The GUID File Name of the HON from the Firmware Volume

  MemoryAllocationModule                    - The 64 bit physical address of the module

  ModuleLength              - The length of the module in bytes

  EntryPoint                - The 64 bit physical address of the entry point to the module

Returns:

  EFI_SUCCESS
  EFI_NOT_AVAILABLE_YET

--*/
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PEI_HOB_POINTERS        HandOffHob;
 
  HandOffHob.Raw = HobStart;
  Hob.Raw = (VOID*)(UINTN)(HandOffHob.HandoffInformationTable->EfiEndOfHobList);

  Hob.Header->HobType   = EFI_HOB_TYPE_MEMORY_ALLOCATION;
  Hob.Header->HobLength = (UINT16) sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE);
  
  CopyMem(&(Hob.MemoryAllocationModule->ModuleName), ModuleName, sizeof(EFI_GUID));
  CopyMem(&(Hob.MemoryAllocationModule->MemoryAllocationHeader.Name), &gEfiHobMemeryAllocModuleGuid, sizeof(EFI_GUID));
  Hob.MemoryAllocationModule->MemoryAllocationHeader.MemoryBaseAddress = MemoryAllocationModule;
  Hob.MemoryAllocationModule->MemoryAllocationHeader.MemoryLength      = ModuleLength;
  Hob.MemoryAllocationModule->MemoryAllocationHeader.MemoryType  = EfiConventionalMemory;

  Hob.MemoryAllocationModule->EntryPoint        = EntryPoint;

  Hob.MemoryAllocationModule++;
  HandOffHob.HandoffInformationTable->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  Hob = BuildHobEndOfHobList(Hob.Raw);
  HandOffHob.HandoffInformationTable->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  return EFI_SUCCESS;
}

EFI_STATUS
BuildHobResourceDescriptor (
  IN VOID *                       HobStart,
  IN EFI_RESOURCE_TYPE             ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE   ResourceAttribute,
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

  PhysicalStart   - The 64 bit physical address of memory described by this HOB

  NumberOfBytes   - The length of the memoty described by this HOB in bytes

Returns:

  EFI_SUCCESS
  EFI_NOT_AVAILABLE_YET

--*/
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PEI_HOB_POINTERS        HandOffHob;
  
  HandOffHob.Raw = HobStart;
  Hob.Raw = (VOID *)(UINTN)(HandOffHob.HandoffInformationTable->EfiEndOfHobList);
  
  Hob.Header->HobType   = EFI_HOB_TYPE_RESOURCE_DESCRIPTOR;
  Hob.Header->HobLength = (UINT16) sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);

  Hob.ResourceDescriptor->ResourceType          = ResourceType;
  Hob.ResourceDescriptor->ResourceAttribute     = ResourceAttribute;
  Hob.ResourceDescriptor->PhysicalStart = PhysicalStart;
  Hob.ResourceDescriptor->ResourceLength = NumberOfBytes; 

  Hob.ResourceDescriptor++;
  HandOffHob.HandoffInformationTable->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  Hob = BuildHobEndOfHobList(Hob.Raw);
  HandOffHob.HandoffInformationTable->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  return EFI_SUCCESS;
}

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
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PEI_HOB_POINTERS        HandOffHob;
  UINTN                   Length;
     
  
  HandOffHob.Raw = HobStart;
  Hob.Raw = (VOID *)(UINTN)HandOffHob.HandoffInformationTable->EfiEndOfHobList;  


  Hob.Header->HobType = EFI_HOB_TYPE_GUID_EXTENSION;
  Length              = sizeof(EFI_HOB_GUID_TYPE) + BufferSize;
  Length              = (Length + 0x7) & (~0x7);
  Hob.Header->HobLength  = (UINT16)Length;
  CopyMem(&Hob.Guid->Name, Guid, sizeof(EFI_GUID));
  CopyMem(Hob.Raw + sizeof(EFI_HOB_GUID_TYPE), Buffer, BufferSize);
  Hob.Raw += Length;

  HandOffHob.HandoffInformationTable->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  Hob = BuildHobEndOfHobList(Hob.Raw);
  HandOffHob.HandoffInformationTable->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  return EFI_SUCCESS;
}

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
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PEI_HOB_POINTERS        HandOffHob;
  
  HandOffHob.Raw = HobStart;
  Hob.Raw = (VOID*)(UINTN)(HandOffHob.HandoffInformationTable->EfiEndOfHobList);  

  Hob.Header->HobType   = EFI_HOB_TYPE_FV;
  Hob.Header->HobLength = (UINT16) sizeof (EFI_HOB_FIRMWARE_VOLUME);

  Hob.FirmwareVolume->BaseAddress = BaseAddress;
  Hob.FirmwareVolume->Length      = Length;

  Hob.FirmwareVolume++;

  HandOffHob.HandoffInformationTable->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  Hob = BuildHobEndOfHobList(Hob.Raw);
  HandOffHob.HandoffInformationTable->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  return EFI_SUCCESS;
}

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
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PEI_HOB_POINTERS        HandOffHob;

  HandOffHob.Raw = HobStart;
  Hob.Raw = (VOID*)(UINTN)HandOffHob.HandoffInformationTable->EfiEndOfHobList;

  Hob.Header->HobType   = EFI_HOB_TYPE_CPU;
  Hob.Header->HobLength = (UINT16) sizeof (EFI_HOB_CPU);

  Hob.Cpu->SizeOfMemorySpace = SizeOfMemorySpace;
  Hob.Cpu->SizeOfIoSpace     = SizeOfIoSpace;

  Hob.Cpu++;
  HandOffHob.HandoffInformationTable->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  Hob = BuildHobEndOfHobList(Hob.Raw);
  HandOffHob.HandoffInformationTable->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  return EFI_SUCCESS;
}



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
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PEI_HOB_POINTERS        HandOffHob;
    
  HandOffHob.Raw = HobStart;
  Hob.Raw = (VOID*)(UINTN)HandOffHob.HandoffInformationTable->EfiEndOfHobList;

  Hob.Header->HobType   = EFI_HOB_TYPE_MEMORY_ALLOCATION;
  Hob.Header->HobLength = (UINT16) sizeof (EFI_HOB_MEMORY_ALLOCATION_STACK);

  CopyMem(&(Hob.MemoryAllocationStack->AllocDescriptor.Name), &gEfiHobMemeryAllocStackGuid, sizeof(EFI_GUID));
  (Hob.MemoryAllocationStack->AllocDescriptor).MemoryBaseAddress = BaseAddress;
  (Hob.MemoryAllocationStack->AllocDescriptor).MemoryLength      = Length;
  (Hob.MemoryAllocationStack->AllocDescriptor).MemoryType  = EfiBootServicesData;

  Hob.MemoryAllocationStack++;
  HandOffHob.HandoffInformationTable->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  Hob = BuildHobEndOfHobList(Hob.Raw);
  HandOffHob.HandoffInformationTable->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  return EFI_SUCCESS;
}



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
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PEI_HOB_POINTERS        HandOffHob;

  HandOffHob.Raw = HobStart;
  Hob.Raw = (VOID *)(UINTN)HandOffHob.HandoffInformationTable->EfiEndOfHobList;
  Hob.Header->HobType   = EFI_HOB_TYPE_MEMORY_ALLOCATION;
  Hob.Header->HobLength = (UINT16) sizeof (EFI_HOB_MEMORY_ALLOCATION_BSP_STORE);

  (Hob.MemoryAllocationBspStore->AllocDescriptor).MemoryBaseAddress = BaseAddress;
  (Hob.MemoryAllocationBspStore->AllocDescriptor).MemoryLength = Length;
  (Hob.MemoryAllocationBspStore->AllocDescriptor).MemoryType = MemoryType;
  CopyMem(&(Hob.MemoryAllocationBspStore->AllocDescriptor).Name, &gEfiHobMemeryAllocBspStoreGuid, sizeof(EFI_GUID));
  Hob.MemoryAllocationBspStore++;

  HandOffHob.HandoffInformationTable->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  Hob = BuildHobEndOfHobList(Hob.Raw);
  HandOffHob.HandoffInformationTable->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  return EFI_SUCCESS;
}


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
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PEI_HOB_POINTERS        HandOffHob;
  
    
  HandOffHob.Raw = HobStart;
  Hob.Raw = (VOID*)(UINTN)HandOffHob.HandoffInformationTable->EfiEndOfHobList;

  Hob.Header->HobType   = EFI_HOB_TYPE_MEMORY_ALLOCATION;
  Hob.Header->HobLength = (UINT16) sizeof (EFI_HOB_MEMORY_ALLOCATION);

  if (Name != NULL) {
    CopyMem (&(Hob.MemoryAllocation->AllocDescriptor.Name), &Name, sizeof (EFI_GUID));
  } else {
    ZeroMem (&Hob.MemoryAllocation->AllocDescriptor.Name, sizeof (EFI_GUID));
  }

  (Hob.MemoryAllocation->AllocDescriptor).MemoryBaseAddress = BaseAddress;
  (Hob.MemoryAllocation->AllocDescriptor).MemoryLength = Length;
  (Hob.MemoryAllocation->AllocDescriptor).MemoryType = MemoryType;

  Hob.MemoryAllocation++;
  HandOffHob.HandoffInformationTable->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  Hob = BuildHobEndOfHobList(Hob.Raw);
  HandOffHob.HandoffInformationTable->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS) (UINTN) Hob.Raw;
  return EFI_SUCCESS;
}

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
{
  EFI_STATUS        Status;
  EFI_PEI_HOB_POINTERS  GuidHob;
  
  GuidHob.Raw = *HobStart;

  for (Status = EFI_NOT_FOUND; EFI_ERROR (Status); ) {
    
    if (END_OF_HOB_LIST (GuidHob)) {
      return EFI_NOT_FOUND;
    }
    
    if (GuidHob.Header->HobType == EFI_HOB_TYPE_GUID_EXTENSION) {            
      if ( ((INT32 *)Guid)[0] == ((INT32 *)&GuidHob.Guid->Name)[0] &&
           ((INT32 *)Guid)[1] == ((INT32 *)&GuidHob.Guid->Name)[1] &&
           ((INT32 *)Guid)[2] == ((INT32 *)&GuidHob.Guid->Name)[2] &&
           ((INT32 *)Guid)[3] == ((INT32 *)&GuidHob.Guid->Name)[3] ) {
        Status  = EFI_SUCCESS;
        *Buffer = (VOID *)((UINT8 *)(&GuidHob.Guid->Name) + sizeof (EFI_GUID));
        if (BufferSize) {
          *BufferSize = GuidHob.Header->HobLength - sizeof (EFI_HOB_GUID_TYPE);
        }
      }
    }

    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
  }

  return Status;
}

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
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = HobStart;
  //
  // Return input if not found
  //
  if (HobStart == NULL) {
    return HobStart;
  }

  //
  // Parse the HOB list, stop if end of list or matching type found.
  //
  while (!END_OF_HOB_LIST (Hob)) {

    if (Hob.Header->HobType == Type) {
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  
  //
  // Return input if not found
  //
  if (END_OF_HOB_LIST (Hob)) {
    return HobStart;
  }

  return (VOID *) (Hob.Raw);
}

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
{
  EFI_STATUS            Status;
  EFI_PEI_HOB_POINTERS  GuidHob;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Status = EFI_NOT_FOUND; EFI_ERROR (Status);) {

    GuidHob.Raw = *HobStart;
    if (END_OF_HOB_LIST (GuidHob)) {
      return EFI_NOT_FOUND;
    }

    GuidHob.Raw = GetHob (EFI_HOB_TYPE_GUID_EXTENSION, *HobStart);
    if (GuidHob.Header->HobType == EFI_HOB_TYPE_GUID_EXTENSION) {
      if ( ((INT32 *)Guid)[0] == ((INT32 *)&GuidHob.Guid->Name)[0] &&
           ((INT32 *)Guid)[1] == ((INT32 *)&GuidHob.Guid->Name)[1] &&
           ((INT32 *)Guid)[2] == ((INT32 *)&GuidHob.Guid->Name)[2] &&
           ((INT32 *)Guid)[3] == ((INT32 *)&GuidHob.Guid->Name)[3] ) {
        Status  = EFI_SUCCESS;
        *Buffer = (VOID *) ((UINT8 *) (&GuidHob.Guid->Name) + sizeof (EFI_GUID));
        if (BufferSize != NULL) {
          *BufferSize = GuidHob.Header->HobLength - sizeof (EFI_HOB_GUID_TYPE);
        }
      }
    }

    *HobStart = GET_NEXT_HOB (GuidHob);
  }

  return Status;
}
