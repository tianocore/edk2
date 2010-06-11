/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  hob.c

Abstract:

  Support for hob operation

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"
#include "PeiHob.h"
#include EFI_GUID_DEFINITION (IoBaseHob)
#include EFI_GUID_DEFINITION (MemoryAllocationHob)

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

UINTN
GetHobListSize (
  IN VOID  *HobStart
  )
/*++

Routine Description:

  Get size of hob list.

Arguments:

  HobStart      - Start pointer of hob list

Returns:

  Size of hob list.

--*/
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 Size;

  Hob.Raw = HobStart;
  Size    = 0;

  while (Hob.Header->HobType != EFI_HOB_TYPE_END_OF_HOB_LIST) {
    Size += Hob.Header->HobLength;
    Hob.Raw += Hob.Header->HobLength;
  }

  Size += Hob.Header->HobLength;

  return Size;
}

UINT32
GetHobVersion (
  IN VOID  *HobStart
  )
/*++

Routine Description:

  Get hob version.

Arguments:

  HobStart      - Start pointer of hob list

Returns:

  Hob version.

--*/
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = HobStart;
  return Hob.HandoffInformationTable->Version;
}

EFI_STATUS
GetHobBootMode (
  IN  VOID           *HobStart,
  OUT EFI_BOOT_MODE  *BootMode
  )
/*++

Routine Description:

  Get current boot mode.

Arguments:

  HobStart      - Start pointer of hob list
  
  BootMode      - Current boot mode recorded in PHIT hob

Returns:

  EFI_NOT_FOUND     - Invalid hob header
  
  EFI_SUCCESS       - Boot mode found

--*/
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = HobStart;
  if (Hob.Header->HobType != EFI_HOB_TYPE_HANDOFF) {
    return EFI_NOT_FOUND;
  }

  *BootMode = Hob.HandoffInformationTable->BootMode;
  return EFI_SUCCESS;
}

EFI_STATUS
GetCpuHobInfo (
  IN  VOID   *HobStart,
  OUT UINT8  *SizeOfMemorySpace,
  OUT UINT8  *SizeOfIoSpace
  )
/*++

Routine Description:

  Get information recorded in CPU hob (Memory space size, Io space size)

Arguments:

  HobStart            - Start pointer of hob list
  
  SizeOfMemorySpace   - Size of memory size
  
  SizeOfIoSpace       - Size of IO size

Returns:

  EFI_NOT_FOUND     - CPU hob not found
  
  EFI_SUCCESS       - CPU hob found and information got.

--*/
{
  EFI_PEI_HOB_POINTERS  CpuHob;

  CpuHob.Raw  = HobStart;
  CpuHob.Raw  = GetHob (EFI_HOB_TYPE_CPU, CpuHob.Raw);
  if (CpuHob.Header->HobType != EFI_HOB_TYPE_CPU) {
    return EFI_NOT_FOUND;
  }

  *SizeOfMemorySpace  = CpuHob.Cpu->SizeOfMemorySpace;
  *SizeOfIoSpace      = CpuHob.Cpu->SizeOfIoSpace;
  return EFI_SUCCESS;
}

EFI_STATUS
GetDxeCoreHobInfo (
  IN  VOID                  *HobStart,
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                *Length,
  OUT VOID                  **EntryPoint,
  OUT EFI_GUID              **FileName
  )
/*++

Routine Description:

  Get memory allocation hob created for DXE core and extract its information

Arguments:

  HobStart        - Start pointer of the hob list
  BaseAddress     - Start address of memory allocated for DXE core
  Length          - Length of memory allocated for DXE core
  EntryPoint      - DXE core file name
  FileName        - File Name

Returns:

  EFI_NOT_FOUND   - DxeCoreHob not found  
  EFI_SUCCESS     - DxeCoreHob found and information got

--*/
{
  EFI_PEI_HOB_POINTERS  DxeCoreHob;
  
  
  DxeCoreHob.Raw  = HobStart;
  DxeCoreHob.Raw  = GetHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, DxeCoreHob.Raw);
  while (DxeCoreHob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION && 
         !EfiCompareGuid (&DxeCoreHob.MemoryAllocationModule->MemoryAllocationHeader.Name, 
                          &gEfiHobMemeryAllocModuleGuid)) {

    DxeCoreHob.Raw  = GET_NEXT_HOB (DxeCoreHob);
    DxeCoreHob.Raw  = GetHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, DxeCoreHob.Raw);

  }

  if (DxeCoreHob.Header->HobType != EFI_HOB_TYPE_MEMORY_ALLOCATION) {
    return EFI_NOT_FOUND;
  }

  *BaseAddress  = DxeCoreHob.MemoryAllocationModule->MemoryAllocationHeader.MemoryBaseAddress;
  *Length       = DxeCoreHob.MemoryAllocationModule->MemoryAllocationHeader.MemoryLength;
  *EntryPoint   = (VOID *) (UINTN) DxeCoreHob.MemoryAllocationModule->EntryPoint;
  *FileName     = &DxeCoreHob.MemoryAllocationModule->ModuleName;

  return EFI_SUCCESS;
}

EFI_STATUS
GetNextFirmwareVolumeHob (
  IN OUT VOID                  **HobStart,
  OUT    EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT    UINT64                *Length
  )
/*++

Routine Description:

  Get next firmware volume hob from HobStart

Arguments:

  HobStart        - Start pointer of hob list
  
  BaseAddress     - Start address of next firmware volume
  
  Length          - Length of next firmware volume

Returns:

  EFI_NOT_FOUND   - Next firmware volume not found
  
  EFI_SUCCESS     - Next firmware volume found with address information

--*/
{
  EFI_PEI_HOB_POINTERS  FirmwareVolumeHob;

  FirmwareVolumeHob.Raw = *HobStart;
  if (END_OF_HOB_LIST (FirmwareVolumeHob)) {
    return EFI_NOT_FOUND;
  }

  FirmwareVolumeHob.Raw = GetHob (EFI_HOB_TYPE_FV, *HobStart);
  if (FirmwareVolumeHob.Header->HobType != EFI_HOB_TYPE_FV) {
    return EFI_NOT_FOUND;
  }

  *BaseAddress  = FirmwareVolumeHob.FirmwareVolume->BaseAddress;
  *Length       = FirmwareVolumeHob.FirmwareVolume->Length;

  *HobStart     = GET_NEXT_HOB (FirmwareVolumeHob);

  return EFI_SUCCESS;
}

#if (PI_SPECIFICATION_VERSION >= 0x00010000)
EFI_STATUS
GetNextFirmwareVolume2Hob (
  IN OUT VOID                  **HobStart,
  OUT    EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT    UINT64                *Length,
  OUT    EFI_GUID              *FileName
  )
/*++

Routine Description:

  Get next firmware volume2 hob from HobStart

Arguments:

  HobStart        - Start pointer of hob list
  
  BaseAddress     - Start address of next firmware volume
  
  Length          - Length of next firmware volume

Returns:

  EFI_NOT_FOUND   - Next firmware volume not found
  
  EFI_SUCCESS     - Next firmware volume found with address information

--*/
{
  EFI_PEI_HOB_POINTERS  FirmwareVolumeHob;

  FirmwareVolumeHob.Raw = *HobStart;
  if (END_OF_HOB_LIST (FirmwareVolumeHob)) {
    return EFI_NOT_FOUND;
  }

  FirmwareVolumeHob.Raw = GetHob (EFI_HOB_TYPE_FV2, *HobStart);
  if (FirmwareVolumeHob.Header->HobType != EFI_HOB_TYPE_FV2) {
    return EFI_NOT_FOUND;
  }

  *BaseAddress  = FirmwareVolumeHob.FirmwareVolume2->BaseAddress;
  *Length       = FirmwareVolumeHob.FirmwareVolume2->Length;
  EfiCommonLibCopyMem(FileName,&FirmwareVolumeHob.FirmwareVolume2->FileName,sizeof(EFI_GUID));

  *HobStart     = GET_NEXT_HOB (FirmwareVolumeHob);

  return EFI_SUCCESS;
}
#endif

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
      if (EfiCompareGuid (Guid, &GuidHob.Guid->Name)) {
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


#define PAL_ENTRY_HOB {0xe53cb8cc, 0xd62c, 0x4f74, {0xbd, 0xda, 0x31, 0xe5, 0x8d, 0xe5, 0x3e, 0x2}}
EFI_GUID  gPalEntryHob = PAL_ENTRY_HOB;

EFI_STATUS
GetPalEntryHobInfo (
  IN  VOID                  *HobStart,
  OUT EFI_PHYSICAL_ADDRESS  *PalEntry
  )
/*++

Routine Description:

  Get PAL entry from PalEntryHob

Arguments:

  HobStart      - Start pointer of hob list
  
  PalEntry      - Pointer to PAL entry

Returns:

  Status code.

--*/
{
  VOID        *Buffer;
  UINTN       BufferSize;
  EFI_STATUS  Status;
  VOID        *HobStart2;

  //
  // Initialize 'Buffer' to NULL before usage
  //
  Buffer = NULL;
  HobStart2 = HobStart;
  Status = GetNextGuidHob (
            &HobStart2,
            &gPalEntryHob,
            &Buffer,
            &BufferSize
            );
  if (EFI_ERROR (Status) || (Buffer == NULL)) {
    //
    // Failed to get HOB for gPalEntryHob
    //
    return EFI_NOT_FOUND;
  }
  *PalEntry = *((EFI_PHYSICAL_ADDRESS *) Buffer);
  return EFI_SUCCESS;
}


EFI_STATUS
GetIoPortSpaceAddressHobInfo (
  IN  VOID                  *HobStart,
  OUT EFI_PHYSICAL_ADDRESS  *IoPortSpaceAddress
  )
/*++

Routine Description:

  Get IO port space address from IoBaseHob.

Arguments:

  HobStart              - Start pointer of hob list
  
  IoPortSpaceAddress    - IO port space address

Returns:

  Status code

--*/
{

  VOID        *Buffer;
  UINTN       BufferSize;
  EFI_STATUS  Status;
  VOID        *HobStart2;

  //
  // Initialize 'Buffer' to NULL before usage
  //
  Buffer = NULL;
  HobStart2 = HobStart;
  Status = GetNextGuidHob (
            &HobStart2,
            &gEfiIoBaseHobGuid,
            &Buffer,
            &BufferSize
            );
  if (EFI_ERROR (Status) || (Buffer == NULL)) {
    //
    // Failed to get HOB for gEfiIoBaseHobGuid
    //
    return EFI_NOT_FOUND;
  }

  *IoPortSpaceAddress = *((EFI_PHYSICAL_ADDRESS *) Buffer);
  return EFI_SUCCESS;
}
