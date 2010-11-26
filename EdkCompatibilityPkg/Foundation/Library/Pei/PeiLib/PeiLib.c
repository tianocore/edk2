/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeiLib.c

Abstract:

  PEI Library Functions
 
--*/

#include "TianoCommon.h"
#include "PeiHob.h"
#include "Pei.h"
#include "PeiLib.h"
#include "EfiCommonLib.h"

VOID
PeiCopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  );

VOID
ZeroMem (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
/*++

Routine Description:

  Set Buffer to zero for Size bytes.

Arguments:

  Buffer  - Memory to set.

  Size    - Number of bytes to set

Returns:

  None

--*/
{
  EfiCommonLibZeroMem (Buffer, Size);
}

VOID
PeiCopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  )
/*++

Routine Description:

  Copy Length bytes from Source to Destination.

Arguments:

  Destination - Target of copy

  Source      - Place to copy from

  Length      - Number of bytes to copy

Returns:

  None

--*/
{
  EfiCommonLibCopyMem (Destination, Source, Length);
}

VOID
CopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  )
/*++

Routine Description:

  Copy Length bytes from Source to Destination.

Arguments:

  Destination - Target of copy

  Source      - Place to copy from

  Length      - Number of bytes to copy

Returns:

  None

--*/
{
  EfiCommonLibCopyMem (Destination, Source, Length);
}


BOOLEAN
CompareGuid (
  IN EFI_GUID     *Guid1,
  IN EFI_GUID     *Guid2
  )
/*++

Routine Description:

  Compares two GUIDs

Arguments:

  Guid1 - guid to compare
  Guid2 - guid to compare

Returns:
  = TRUE  if Guid1 == Guid2
  = FALSE if Guid1 != Guid2 

--*/
{
  if ((((INT32 *) Guid1)[0] - ((INT32 *) Guid2)[0]) == 0) {
    if ((((INT32 *) Guid1)[1] - ((INT32 *) Guid2)[1]) == 0) {
      if ((((INT32 *) Guid1)[2] - ((INT32 *) Guid2)[2]) == 0) {
        if ((((INT32 *) Guid1)[3] - ((INT32 *) Guid2)[3]) == 0) {
          return TRUE;
        }
      }
    }
  }

  return FALSE;
}


EFI_STATUS
EFIAPI 
PeiLibPciCfgModify (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_PCI_CFG_PPI          *PciCfg,
  IN PEI_PCI_CFG_PPI_WIDTH    Width,
  IN UINT64                   Address,
  IN UINTN                    SetBits,
  IN UINTN                    ClearBits
  )
/*++

Routine Description:

  PCI read-modify-write operations.

  PIWG's PI specification replaces Inte's EFI Specification 1.10.
  EFI_PEI_PCI_CFG_PPI defined in Inte's EFI Specification 1.10 is replaced by
  EFI_PEI_PCI_CFG2_PPI in PI 1.0. "Modify" function  in these two PPI are not 
  compatibile with each other.
  

  For Framework code that make the following call:

      PciCfg->Modify (
                       PeiServices,
                       PciCfg,
                       Width,
                       Address,
                       SetBits,
                       ClearBits
                       );
   it will be updated to the following code which call this library API:
      PeiLibPciCfgModify (
          PeiServices,
          PciCfg,
          Width,
          Address,
          SetBits,
          ClearBits
          );

   The 

Arguments:
  
  PeiServices     An indirect pointer to the PEI Services Table
                          published by the PEI Foundation.
  PciCfg          A pointer to the this pointer of EFI_PEI_PCI_CFG_PPI. 
                          This parameter is unused as a place holder to make
                          the parameter list identical to PEI_PCI_CFG_PPI_RW.
  Width           The width of the access. Enumerated in bytes. Type
                          EFI_PEI_PCI_CFG_PPI_WIDTH is defined in Read().

  Address         The physical address of the access.

  SetBits         Points to value to bitwise-OR with the read configuration value.

                          The size of the value is determined by Width.

  ClearBits       Points to the value to negate and bitwise-AND with the read configuration value.
                          The size of the value is determined by Width.


Returns:

  EFI_SUCCESS           The function completed successfully.

  EFI_DEVICE_ERROR      There was a problem with the transaction.

--*/
{
  EFI_STATUS            Status;
  EFI_PEI_PCI_CFG2_PPI  *PciCfg2;

  Status = (*PeiServices)->LocatePpi (
                             PeiServices,
                             &gPeiPciCfg2PpiGuid,
                             0,
                             NULL,
                             (VOID **) &PciCfg2
                             );
  ASSERT_PEI_ERROR ((CONST EFI_PEI_SERVICES **) PeiServices, Status);

  Status = PciCfg2->Modify (
                      (CONST EFI_PEI_SERVICES **) PeiServices,
                      PciCfg2,
                      (EFI_PEI_PCI_CFG_PPI_WIDTH) Width,
                      Address,
                      &SetBits,
                      &ClearBits
                      );

  return Status;
}


#if (PI_SPECIFICATION_VERSION >= 0x00010000)

VOID *
EFIAPI
ScanGuid (
  IN VOID        *Buffer,
  IN UINTN       Length,
  IN EFI_GUID    *Guid
  )
/*++

Routine Description:

  Scans a target buffer for a GUID, and returns a pointer to the matching GUID
  in the target buffer.

  This function searches target the buffer specified by Buffer and Length from
  the lowest address to the highest address at 128-bit increments for the 128-bit
  GUID value that matches Guid.  If a match is found, then a pointer to the matching
  GUID in the target buffer is returned.  If no match is found, then NULL is returned.
  If Length is 0, then NULL is returned.
  If Length > 0 and Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 32-bit boundary, then ASSERT().
  If Length is not aligned on a 128-bit boundary, then ASSERT().
  If Length is greater than (EFI_MAX_ADDRESS ?Buffer + 1), then ASSERT(). 

Arguments:

  Buffer - Pointer to the target buffer to scan.
  Length - Number of bytes in Buffer to scan.
  Guid   - Value to search for in the target buffer.
  
Returns:
  A pointer to the matching Guid in the target buffer or NULL otherwise.

--*/
{
  EFI_GUID                 *GuidPtr;
  EFI_PEI_SERVICES         **PeiServices;
  
  PeiServices = GetPeiServicesTablePointer();
  PEI_ASSERT(PeiServices, (((UINTN)Buffer & (sizeof (Guid->Data1) - 1)) == 0));
  PEI_ASSERT(PeiServices, (Length <= (EFI_MAX_ADDRESS - (UINTN)Buffer + 1)));
  PEI_ASSERT(PeiServices, ((Length & (sizeof (*GuidPtr) - 1)) == 0));

  GuidPtr = (EFI_GUID*)Buffer;
  Buffer  = GuidPtr + Length / sizeof (*GuidPtr);
  while (GuidPtr < (EFI_GUID*)Buffer) {
    if (CompareGuid (GuidPtr, Guid)) {
      return (VOID*)GuidPtr;
    }
    GuidPtr++;
  }
  return NULL;
}


VOID *
EFIAPI
InvalidateInstructionCacheRange (
  IN      VOID                      *Address,
  IN      UINTN                     Length
  )
/*++

Routine Description:

  Invalidates a range of instruction cache lines in the cache coherency domain
  of the calling CPU.

  Invalidates the instruction cache lines specified by Address and Length. If
  Address is not aligned on a cache line boundary, then entire instruction
  cache line containing Address is invalidated. If Address + Length is not
  aligned on a cache line boundary, then the entire instruction cache line
  containing Address + Length -1 is invalidated. This function may choose to
  invalidate the entire instruction cache if that is more efficient than
  invalidating the specified range. If Length is 0, the no instruction cache
  lines are invalidated. Address is returned.

  If Length is greater than (EFI_MAX_ADDRESS - Address + 1), then ASSERT().

Arguments:

  Address   -     The base address of the instruction cache lines to
                  invalidate. If the CPU is in a physical addressing mode, then
                  Address is a physical address. If the CPU is in a virtual
                  addressing mode, then Address is a virtual address.

  Length    -      The number of bytes to invalidate from the instruction cache.

 Returns:
  Address

**/
{
  PEI_ASSERT(GetPeiServicesTablePointer() , (Length <= EFI_MAX_ADDRESS - (UINTN)Address + 1));
  return Address;
}


EFI_STATUS
EFIAPI
PeiLibFfsFindNextVolume (
  IN UINTN                          Instance,
  IN OUT EFI_PEI_FV_HANDLE          *VolumeHandle
  )
/*++

Routine Description:

  The wrapper of Pei Core Service function FfsFindNextVolume.

Arguments:

  Instance     - The Fv Volume Instance.
  VolumeHandle - Pointer to the current Fv Volume to search.

Returns:
  EFI_STATUS
  
--*/
  
{
  EFI_PEI_SERVICES  **PeiServices;
  
  PeiServices = GetPeiServicesTablePointer();
  return (*PeiServices)->FfsFindNextVolume (PeiServices, Instance, VolumeHandle);
}

EFI_STATUS
EFIAPI
PeiLibFfsFindNextFile (
  IN EFI_FV_FILETYPE            SearchType,
  IN EFI_PEI_FV_HANDLE          FvHandle,
  IN OUT EFI_PEI_FILE_HANDLE    *FileHandle
  )
/*++

Routine Description:

  The wrapper of Pei Core Service function FfsFindNextFile.

Arguments:

  SearchType   - Filter to find only file of this type.
  FvHandle     - Pointer to the current FV to search.
  FileHandle   - Pointer to the file matching SearchType in FwVolHeader.
               - NULL if file not found

Returns:
  EFI_STATUS
  
--*/  
{
  EFI_PEI_SERVICES  **PeiServices;
  
  PeiServices = GetPeiServicesTablePointer();
  return (*PeiServices)->FfsFindNextFile (PeiServices, SearchType, FvHandle, FileHandle);
}


EFI_STATUS
EFIAPI
PeiLibFfsFindFileByName (
  IN  EFI_GUID              *FileName,
  IN  EFI_PEI_FV_HANDLE     VolumeHandle,
  OUT EFI_PEI_FILE_HANDLE   *FileHandle
  )
/*++

Routine Description:

  The wrapper of Pei Core Service function FfsFindFileByName.

Arguments:

  FileName      - File name to search.
  VolumeHandle  - The current FV to search.
  FileHandle    - Pointer to the file matching name in VolumeHandle.
                - NULL if file not found

Returns:
   EFI_STATUS
   
--*/  
{
  EFI_PEI_SERVICES  **PeiServices;
  
  PeiServices = GetPeiServicesTablePointer();
  return (*PeiServices)->FfsFindFileByName (FileName, VolumeHandle, FileHandle);
}



EFI_STATUS
EFIAPI
PeiLibFfsFindSectionData (
  IN EFI_SECTION_TYPE           SectionType,
  IN EFI_FFS_FILE_HEADER        *FfsFileHeader,
  IN OUT VOID                   **SectionData
  )
/*++

Routine Description:

  The wrapper of Pei Core Service function FfsFindSectionData.

Arguments:

  SearchType      - Filter to find only sections of this type.
  FileHandle      - Pointer to the current file to search.
  SectionData     - Pointer to the Section matching SectionType in FfsFileHeader.
                  - NULL if section not found

Returns:
  EFI_STATUS
--*/
{
  EFI_PEI_SERVICES  **PeiServices;
  
  PeiServices = GetPeiServicesTablePointer();
  return (*PeiServices)->FfsFindSectionData (PeiServices, SectionType, (EFI_PEI_FILE_HANDLE)FfsFileHeader, SectionData);
}

EFI_STATUS
EFIAPI
PeiLibFfsGetVolumeInfo (
  IN EFI_PEI_FV_HANDLE  VolumeHandle,
  OUT EFI_FV_INFO       *VolumeInfo
  )
/*++

Routine Description:

  The wrapper of Pei Core Service function FfsGetVolumeInfo.

Arguments:

  VolumeHandle    - The handle to Fv Volume.
  VolumeInfo      - The pointer to volume information.
  
Returns:
  EFI_STATUS
--*/  
{
  EFI_PEI_SERVICES  **PeiServices;
  
  PeiServices = GetPeiServicesTablePointer();
  return (*PeiServices)->FfsGetVolumeInfo (VolumeHandle, VolumeInfo);
}



VOID
EFIAPI
BuildFvHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  )
/*++

Routine Description:

  Build FvHob.

Arguments:

  BaseAddress    - Fv base address.
  Length         - Fv Length.

Returns:
  NONE.
--*/  
{

  EFI_STATUS               Status;  
  EFI_HOB_FIRMWARE_VOLUME  *Hob;
  EFI_PEI_SERVICES  **PeiServices;

  PeiServices = GetPeiServicesTablePointer();

  //
  // Check FV Signature
  //
  PEI_ASSERT (PeiServices, ((EFI_FIRMWARE_VOLUME_HEADER*)((UINTN)BaseAddress))->Signature == EFI_FVH_SIGNATURE);


  Status = (*PeiServices)->CreateHob (
                             PeiServices,
                             EFI_HOB_TYPE_FV,
                             sizeof (EFI_HOB_FIRMWARE_VOLUME),
                             &Hob
                             );
  Hob->BaseAddress = BaseAddress;
  Hob->Length      = Length;
}

VOID
EFIAPI
BuildFvHob2 (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length,
  IN EFI_GUID                    *FvNameGuid,
  IN EFI_GUID                    *FileNameGuid
  )
/*++

Routine Description:

  Build FvHob2.

Arguments:

  BaseAddress  - Fv base address.
  Length       - Fv length.
  FvNameGuid   - Fv name.
  FileNameGuid - File name which contians encapsulated Fv.

Returns:
   NONE.
--*/  
{

  EFI_STATUS               Status;  
  EFI_HOB_FIRMWARE_VOLUME2  *Hob;
  EFI_PEI_SERVICES  **PeiServices;

  PeiServices = GetPeiServicesTablePointer();

  //
  // Check FV Signature
  //
  PEI_ASSERT (PeiServices, ((EFI_FIRMWARE_VOLUME_HEADER*)((UINTN)BaseAddress))->Signature == EFI_FVH_SIGNATURE);

  Status = (*PeiServices)->CreateHob (
                             PeiServices,
                             EFI_HOB_TYPE_FV2,
                             sizeof (EFI_HOB_FIRMWARE_VOLUME2),
                             &Hob
                             );
  Hob->BaseAddress = BaseAddress;
  Hob->Length      = Length;
  CopyMem ((VOID*)&Hob->FvName, FvNameGuid, sizeof(EFI_GUID));
  CopyMem ((VOID*)&Hob->FileName, FileNameGuid, sizeof(EFI_GUID));
}

EFI_STATUS
EFIAPI
PeiServicesLocatePpi (
  IN EFI_GUID                   *Guid,
  IN UINTN                      Instance,
  IN OUT EFI_PEI_PPI_DESCRIPTOR **PpiDescriptor,
  IN OUT VOID                   **Ppi
  )
/*++

Routine Description:

  The wrapper of Pei Core Service function LocatePpi.

Arguments:

  Guid          - Pointer to GUID of the PPI.
  Instance      - Instance Number to discover.
  PpiDescriptor - Pointer to reference the found descriptor. If not NULL,
                returns a pointer to the descriptor (includes flags, etc)
  Ppi           - Pointer to reference the found PPI

Returns:

  Status -  EFI_SUCCESS   if the PPI is in the database           
            EFI_NOT_FOUND if the PPI is not in the database
--*/  
{
  EFI_PEI_SERVICES  **PeiServices;
  
  PeiServices = GetPeiServicesTablePointer();
  return (*PeiServices)->LocatePpi (PeiServices, Guid, Instance, PpiDescriptor, Ppi);
}


VOID 
EFIAPI
BuildGuidDataHob (
  IN EFI_GUID                   *Guid,
  IN VOID                        *Data,
  IN UINTN                       DataLength
  )
/*++

Routine Description:

  Build Guid data Hob.

Arguments:

  Guid        - guid to build data hob.
  Data        - data to build data hob.
  DataLength  - the length of data.

Returns:
  NONE
--*/  
{
  VOID              *HobData;
  EFI_HOB_GUID_TYPE *Hob;
  EFI_PEI_SERVICES  **PeiServices;

  PeiServices = GetPeiServicesTablePointer();
  (*PeiServices)->CreateHob (
                     PeiServices,
                     EFI_HOB_TYPE_GUID_EXTENSION,
                     (UINT16) (sizeof (EFI_HOB_GUID_TYPE) + DataLength),
                     &Hob
                     );
  CopyMem ((VOID*)&Hob->Name, (VOID*)Guid, sizeof(EFI_GUID));

  HobData =  Hob + 1;

  CopyMem (HobData, Data, DataLength);
}


VOID *
EFIAPI
AllocatePages (
  IN UINTN  Pages
  )
/*++

Routine Description:

  Allocate Memory.

Arguments:

  Pages - Pages to allocate.

Returns:
  = Address if successful to allocate memory. 
  = NULL    if fail to allocate memory.

--*/  
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory; 
  EFI_PEI_SERVICES      **PeiServices;

  if (Pages == 0) {
    return NULL;
  }

  PeiServices = GetPeiServicesTablePointer();
  Status = (*PeiServices)->AllocatePages (PeiServices, EfiBootServicesData, Pages, &Memory);
  if (EFI_ERROR (Status)) {
    Memory = 0;
  }
  return (VOID *) (UINTN) Memory;

}

#endif
