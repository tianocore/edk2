/*++

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  hob.c

Abstract:

  PEI Library Functions
 
--*/

#include "Tiano.h"
#include "Pei.h"
#include "PeiLib.h"
#include EFI_GUID_DEFINITION (MemoryAllocationHob)


EFI_STATUS
PeiBuildHobModule (
  IN EFI_PEI_SERVICES       **PeiServices,
  IN EFI_GUID               *ModuleName,
  IN EFI_PHYSICAL_ADDRESS   MemoryAllocationModule,
  IN UINT64                 ModuleLength,
  IN EFI_PHYSICAL_ADDRESS   EntryPoint
  )
/*++

Routine Description:

  Builds a HOB for a loaded PE32 module

Arguments:

  PeiServices               - The PEI core services table.
  ModuleName                - The GUID File Name of the module
  MemoryAllocationModule    - The 64 bit physical address of the module
  ModuleLength              - The length of the module in bytes
  EntryPoint                - The 64 bit physical address of the entry point
                              to the module

Returns:

  EFI_SUCCESS               - Hob is successfully built.
  Others                    - Errors occur while creating new Hob

--*/
{
  EFI_STATUS                        Status;  
  EFI_HOB_MEMORY_ALLOCATION_MODULE  *Hob;

  Status = (*PeiServices)->CreateHob (
                             PeiServices,
                             EFI_HOB_TYPE_MEMORY_ALLOCATION,
                             sizeof (EFI_HOB_MEMORY_ALLOCATION_MODULE),
                             (VOID **) &Hob
                             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Hob->MemoryAllocationHeader.Name = gEfiHobMemeryAllocModuleGuid;
  Hob->MemoryAllocationHeader.MemoryBaseAddress = MemoryAllocationModule;
  Hob->MemoryAllocationHeader.MemoryLength = ModuleLength;
  Hob->MemoryAllocationHeader.MemoryType = EfiBootServicesCode;
  (*PeiServices)->SetMem (
                    Hob->MemoryAllocationHeader.Reserved, 
                    sizeof (Hob->MemoryAllocationHeader.Reserved),
                    0
                    );

  Hob->ModuleName = *ModuleName;
  Hob->EntryPoint = EntryPoint;

  return EFI_SUCCESS;
}


EFI_STATUS
PeiBuildHobResourceDescriptor (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes
  )
/*++

Routine Description:

  Builds a HOB that describes a chunck of system memory

Arguments:

  PeiServices        - The PEI core services table.
 
  ResourceType       - The type of resource described by this HOB

  ResourceAttribute  - The resource attributes of the memory described by this HOB

  PhysicalStart      - The 64 bit physical address of memory described by this HOB

  NumberOfBytes      - The length of the memoty described by this HOB in bytes

Returns:

  EFI_SUCCESS     - Hob is successfully built.
  Others          - Errors occur while creating new Hob

--*/
{
  EFI_STATUS                   Status; 
  EFI_HOB_RESOURCE_DESCRIPTOR  *Hob;

  Status = (*PeiServices)->CreateHob (
                             PeiServices,
                             EFI_HOB_TYPE_RESOURCE_DESCRIPTOR,
                             sizeof (EFI_HOB_RESOURCE_DESCRIPTOR),
                             (VOID **) &Hob
                             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Hob->ResourceType      = ResourceType;
  Hob->ResourceAttribute = ResourceAttribute;
  Hob->PhysicalStart     = PhysicalStart;
  Hob->ResourceLength    = NumberOfBytes;

  return EFI_SUCCESS;
}


EFI_STATUS
PeiBuildHobGuid (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  EFI_GUID          *Guid,
  IN  UINTN             DataLength,
  OUT VOID              **Hob
  )
/*++

Routine Description:

  Builds a custom HOB that is tagged with a GUID for identification

Arguments:

  PeiServices - The PEI core services table.
  Guid        - The GUID of the custome HOB type
  DataLength  - The size of the data payload for the GUIDed HOB
  Hob         - Pointer to pointer to the created Hob

Returns:

  EFI_SUCCESS - Hob is successfully built.
  Others      - Errors occur while creating new Hob

--*/
{
  EFI_STATUS         Status;

  Status = (*PeiServices)->CreateHob (
                             PeiServices,
                             EFI_HOB_TYPE_GUID_EXTENSION,
                             (UINT16) (sizeof (EFI_HOB_GUID_TYPE) + DataLength),
                             Hob
                             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ((EFI_HOB_GUID_TYPE *)(*Hob))->Name = *Guid;
  
  return EFI_SUCCESS;
}


EFI_STATUS
PeiBuildHobGuidData (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_GUID                    *Guid,
  IN VOID                        *Data,
  IN UINTN                       DataLength
  )
/*++

Routine Description:

  Builds a custom HOB that is tagged with a GUID for identification

Arguments:

  PeiServices - The PEI core services table.

  Guid        - The GUID of the custome HOB type

  Data        - The data to be copied into the GUIDed HOB data field.

  DataLength  - The data field length.

Returns:

  EFI_SUCCESS   - Hob is successfully built.
  Others        - Errors occur while creating new Hob

--*/
{
  EFI_STATUS         Status;
  
  EFI_HOB_GUID_TYPE  *Hob;

  Status = PeiBuildHobGuid (
             PeiServices,
             Guid,
             DataLength,
             (VOID **) &Hob
             );

  if (EFI_ERROR (Status)) {
    return Status;
  } 

  Hob++;
  (*PeiServices)->CopyMem (Hob, Data, DataLength);
  
  return EFI_SUCCESS;
}


EFI_STATUS
PeiBuildHobFv (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  )
/*++

Routine Description:

  Builds a Firmware Volume HOB

Arguments:

  PeiServices - The PEI core services table.

  BaseAddress - The base address of the Firmware Volume

  Length      - The size of the Firmware Volume in bytes

Returns:

  EFI_SUCCESS   - Hob is successfully built.
  Others        - Errors occur while creating new Hob

--*/
{
  EFI_STATUS               Status;  
  EFI_HOB_FIRMWARE_VOLUME  *Hob;
  
  //
  // Check FV Signature
  //
  PEI_ASSERT ((CONST EFI_PEI_SERVICES **) PeiServices, ((EFI_FIRMWARE_VOLUME_HEADER*)((UINTN)BaseAddress))->Signature == EFI_FVH_SIGNATURE);
  
  Status = (*PeiServices)->CreateHob (
                             PeiServices,
                             EFI_HOB_TYPE_FV,
                             sizeof (EFI_HOB_FIRMWARE_VOLUME),
                             (VOID **) &Hob
                             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Hob->BaseAddress = BaseAddress;
  Hob->Length      = Length;

  return EFI_SUCCESS;
}


EFI_STATUS
PeiBuildHobCpu (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN UINT8                       SizeOfMemorySpace,
  IN UINT8                       SizeOfIoSpace
  )
/*++

Routine Description:

  Builds a HOB for the CPU

Arguments:

  PeiServices               - The PEI core services table.

  SizeOfMemorySpace         - Identifies the maximum 
                              physical memory addressibility of the processor.

  SizeOfIoSpace             - Identifies the maximum physical I/O addressibility 
                              of the processor.

Returns:

  EFI_SUCCESS               - Hob is successfully built.
  Others                    - Errors occur while creating new Hob

--*/
{
  EFI_STATUS   Status;  
  EFI_HOB_CPU  *Hob;

  Status = (*PeiServices)->CreateHob (
                             PeiServices,
                             EFI_HOB_TYPE_CPU,
                             sizeof (EFI_HOB_CPU),
                             (VOID **) &Hob
                             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Hob->SizeOfMemorySpace = SizeOfMemorySpace;
  Hob->SizeOfIoSpace     = SizeOfIoSpace;
  (*PeiServices)->SetMem (
                    Hob->Reserved, 
                    sizeof (Hob->Reserved), 
                    0
                    );
  
  return EFI_SUCCESS;
}



EFI_STATUS
PeiBuildHobStack (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  )
/*++

Routine Description:

  Builds a HOB for the Stack

Arguments:

  PeiServices               - The PEI core services table.

  BaseAddress               - The 64 bit physical address of the Stack

  Length                    - The length of the stack in bytes

Returns:

  EFI_SUCCESS               - Hob is successfully built.
  Others                    - Errors occur while creating new Hob

--*/
{
  EFI_STATUS                       Status;  
  EFI_HOB_MEMORY_ALLOCATION_STACK  *Hob;

  Status = (*PeiServices)->CreateHob (
                             PeiServices,
                             EFI_HOB_TYPE_MEMORY_ALLOCATION,
                             sizeof (EFI_HOB_MEMORY_ALLOCATION_STACK),
                             (VOID **) &Hob
                             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
 
  Hob->AllocDescriptor.Name = gEfiHobMemeryAllocStackGuid;
  Hob->AllocDescriptor.MemoryBaseAddress = BaseAddress;
  Hob->AllocDescriptor.MemoryLength = Length;
  Hob->AllocDescriptor.MemoryType = EfiBootServicesData;
  (*PeiServices)->SetMem (
                    Hob->AllocDescriptor.Reserved, 
                    sizeof (Hob->AllocDescriptor.Reserved), 
                    0
                    );

  return EFI_SUCCESS;
}



EFI_STATUS
PeiBuildHobBspStore (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length,
  IN EFI_MEMORY_TYPE             MemoryType
  )
/*++

Routine Description:

  Builds a HOB for the bsp store

Arguments:

  PeiServices               - The PEI core services table.

  BaseAddress               - The 64 bit physical address of the bsp

  Length                    - The length of the bsp store in bytes

  MemoryType                - Memory type

Returns:

  EFI_SUCCESS               - Hob is successfully built.
  Others                    - Errors occur while creating new Hob

--*/
{
  EFI_STATUS                           Status;  
  EFI_HOB_MEMORY_ALLOCATION_BSP_STORE  *Hob;

  Status = (*PeiServices)->CreateHob (
                             PeiServices,
                             EFI_HOB_TYPE_MEMORY_ALLOCATION,
                             sizeof (EFI_HOB_MEMORY_ALLOCATION_BSP_STORE),
                             (VOID **) &Hob
                             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
 
  Hob->AllocDescriptor.Name = gEfiHobMemeryAllocBspStoreGuid;
  Hob->AllocDescriptor.MemoryBaseAddress = BaseAddress;
  Hob->AllocDescriptor.MemoryLength = Length;
  Hob->AllocDescriptor.MemoryType = MemoryType;
  (*PeiServices)->SetMem (
                    Hob->AllocDescriptor.Reserved, 
                    sizeof (Hob->AllocDescriptor.Reserved), 
                    0
                    );

  return EFI_SUCCESS;
}


EFI_STATUS
PeiBuildHobMemoryAllocation (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length,
  IN EFI_GUID                    *Name,
  IN EFI_MEMORY_TYPE             MemoryType
  )
/*++

Routine Description:

  Builds a HOB for the memory allocation.

Arguments:

  PeiServices               - The PEI core services table.

  BaseAddress               - The 64 bit physical address of the memory

  Length                    - The length of the memory allocation in bytes

  Name                      - Name for Hob

  MemoryType                - Memory type

Returns:

  EFI_SUCCESS               - Hob is successfully built.
  Others                    - Errors occur while creating new Hob

--*/
{
  EFI_STATUS                 Status; 
  EFI_HOB_MEMORY_ALLOCATION  *Hob;

  Status = (*PeiServices)->CreateHob (
                             PeiServices,
                             EFI_HOB_TYPE_MEMORY_ALLOCATION,
                             sizeof (EFI_HOB_MEMORY_ALLOCATION),
                             (VOID **) &Hob
                             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Name != NULL) {
    Hob->AllocDescriptor.Name = *Name;
  } else {
    (*PeiServices)->SetMem(&(Hob->AllocDescriptor.Name), sizeof (EFI_GUID), 0);
  }

  Hob->AllocDescriptor.MemoryBaseAddress = BaseAddress;
  Hob->AllocDescriptor.MemoryLength = Length;
  Hob->AllocDescriptor.MemoryType = MemoryType;
  (*PeiServices)->SetMem (
                    Hob->AllocDescriptor.Reserved, 
                    sizeof (Hob->AllocDescriptor.Reserved), 
                    0
                    );

  return EFI_SUCCESS;
}
