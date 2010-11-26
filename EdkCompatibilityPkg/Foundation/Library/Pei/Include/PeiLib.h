/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeiLib.h

Abstract:

  PEI Library Functions
 
--*/

#ifndef _PEI_LIB_H_
#define _PEI_LIB_H_

#include "Tiano.h"
#include "Pei.h"
#include "PeiHobLib.h"
#include "PeiPerf.h"
#include EFI_PROTOCOL_DEFINITION (Decompress)
#include EFI_PROTOCOL_DEFINITION (TianoDecompress)
#include EFI_GUID_DEFINITION (PeiPeCoffLoader)
#include EFI_PPI_DEFINITION (FindFv)

#if defined(__GNUC__) && defined(ECP_CPU_IPF)

VOID
EFIAPI
EcpEfiBreakPoint (
  VOID
  )
/*++

Routine Description:

  Generates a breakpoint on the CPU.

  Generates a breakpoint on the CPU. The breakpoint must be implemented such
  that code can resume normal execution after the breakpoint.

Arguments:

  VOID

Returns: 

  VOID

--*/
;

VOID
EFIAPI
EcpMemoryFence (
  VOID
  )
/*++

Routine Description:

  Used to serialize load and store operations.

  All loads and stores that proceed calls to this function are guaranteed to be
  globally visible when this function returns.

Arguments:

  VOID

Returns: 

  VOID

--*/
;

#endif


#if (PI_SPECIFICATION_VERSION >= 0x00010000)

typedef struct {
  UINT32   PeiServiceTable;
} PEI_IDT_TABLE;


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
;

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
;

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
;

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
;

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
;

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
;

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
;

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
;

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
;

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
;


VOID 
EFIAPI
BuildGuidDataHob (
  IN EFI_GUID                   *Guid,
  IN VOID                       *Data,
  IN UINTN                      DataLength
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
;

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
  Address if successful to allocate memory. 
  NULL    if fail to allocate memory.

--*/  
;

VOID
SetPeiServicesTablePointer (
  IN EFI_PEI_SERVICES  **PeiServices
  )
/*++

Routine Description:

  Save PeiService pointer so that it can be retrieved anywhere.

Arguments:

  PeiServices     - The direct pointer to PeiServiceTable.
  
Returns:
  NONE
  
--*/      
;

EFI_PEI_SERVICES **
GetPeiServicesTablePointer (
  VOID 
  )
/*++

Routine Description:

  Get PeiService pointer.

Arguments:

  NONE.
  
Returns:
  The direct pointer to PeiServiceTable.
  
--*/      
;

VOID
MigrateIdtTable (
  IN EFI_PEI_SERVICES  **PeiServices
  )
/*++

Routine Description:

  Migrate IDT from CAR to real memory where preceded with 4 bytes for
  storing PeiService pointer.

Arguments:

  PeiServices   - The direct pointer to PeiServiceTable.
  
Returns:

  NONE.
  
--*/   
;


UINTN
ReadIdtBase (
  VOID
  )
/*++

Routine Description:

  Read IDT Register BaseAddress.

Arguments:

  NONE
  
Returns:
  IDT Register BaseAddress.

--*/  
;


UINT16
ReadIdtLimit (
  VOID
  )
/*++

Routine Description:

  Read IDT Register Limit.

Arguments:

  NONE
  
Returns:
  IDT Register Limit.

--*/  
;


VOID
SetIdtBase (
  UINT32  IdtBase,
  UINT16  IdtLimit 
  )
/*++

Routine Description:

  Set IDT Register BaseAddress.

Arguments:

  IdtBase   - IDT.BaseAddress
  IdtLimit  - IDT.Limit
  
Returns:
  NONE
--*/    
;

VOID
AsmWriteKr7 (
  UINT64 Address
  )
/*++

Routine Description:

  Write 64 bit into Kernel Register7 on IPF.

Arguments:

  Address   - Data to write into kr7.
  
Returns:
  NONE
  
--*/    
;


UINT64
AsmReadKr7 (
  VOID
  )
/*++

Routine Description:

  Read 64 bit from Kernel Register7 on IPF.

Arguments:

  NONE  
  
Returns:
  Data in kr7.
  
--*/    
;

#endif

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
;

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
;

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
;

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
;

EFI_STATUS
InstallEfiPeiPeCoffLoader (
  IN EFI_PEI_SERVICES                     **PeiServices,
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL      **This,
  IN EFI_PEI_PPI_DESCRIPTOR               *ThisPpi
  )
/*++

Routine Description:

  Install EFI Pei PE coff loader protocol.
  
Arguments:

  PeiServices - The PEI core services table.
  
  This        - Pointer to get Pei PE coff loader protocol as output
  
  ThisPpi     - Passed in as EFI_NT_LOAD_AS_DLL_PPI on NT_EMULATOR platform

Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
InstallEfiPeiPeCoffLoader64 (
  IN EFI_PEI_SERVICES                          **PeiServices,
  IN OUT  EFI_PEI_PE_COFF_LOADER_PROTOCOL      **This,
  IN EFI_PEI_PPI_DESCRIPTOR                    *ThisPpi
  );



EFI_STATUS
InstallEfiDecompress (
  EFI_DECOMPRESS_PROTOCOL  **This
  )
/*++

Routine Description:

  Install EFI decompress protocol.

Arguments:

  This  - Pointer to get decompress protocol as output

Returns:

  EFI_SUCCESS - EFI decompress protocol successfully installed.

--*/
;

EFI_STATUS
InstallTianoDecompress (
  EFI_TIANO_DECOMPRESS_PROTOCOL  **This
  )
/*++

Routine Description:

  Install Tiano decompress protocol.

Arguments:

  This  - Pointer to get decompress protocol as output

Returns:

  EFI_SUCCESS - Tiano decompress protocol successfully installed.

--*/
;

EFI_STATUS
GetTimerValue (
  OUT UINT64    *TimerValue
  )
/*++

Routine Description:

  Get timer value.

Arguments:

  TimerValue  - Pointer to the returned timer value

Returns:

  EFI_SUCCESS - Successfully got timer value

--*/
;

#ifdef EFI_NT_EMULATOR
EFI_STATUS
PeCoffLoaderWinNtLoadAsDll (
  IN  CHAR8  *PdbFileName,
  IN  VOID   **ImageEntryPoint,
  OUT VOID   **ModHandle
  )
/*++

Routine Description:

  Loads the .DLL file is present when a PE/COFF file is loaded.  This provides source level
  debugging for drivers that have cooresponding .DLL files on the local system.

Arguments:

  PdbFileName     - The name of the .PDB file.  This was found from the PE/COFF
                    file's debug directory entry.

  ImageEntryPoint - A pointer to the DLL entry point of the .DLL file was loaded.

  ModHandle       - Pointer to loaded library.

Returns:

  EFI_SUCCESS     - The .DLL file was loaded, and the DLL entry point is returned in ImageEntryPoint

  EFI_NOT_FOUND   - The .DLL file could not be found

  EFI_UNSUPPORTED - The .DLL file was loaded, but the entry point to the .DLL file could not
                    determined.

--*/
;

#endif
//
// hob.c
//
EFI_STATUS
PeiBuildHobModule (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_GUID                    *ModuleName,
  IN EFI_PHYSICAL_ADDRESS        Module,
  IN UINT64                      ModuleLength,
  IN EFI_PHYSICAL_ADDRESS        EntryPoint
  )
/*++

Routine Description:

  Builds a HOB for a loaded PE32 module

Arguments:

  PeiServices               - The PEI core services table.
  ModuleName                - The GUID File Name of the module
  Memory                    - The 64 bit physical address of the module
  ModuleLength              - The length of the module in bytes
  EntryPoint                - The 64 bit physical address of the entry point
                              to the module

Returns:

  EFI_SUCCESS               - Hob is successfully built.
  Others                    - Errors occur while creating new Hob

--*/
;

EFI_STATUS
PeiBuildHobResourceDescriptor (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_RESOURCE_TYPE           ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS        PhysicalStart,
  IN UINT64                      NumberOfBytes
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
;

EFI_STATUS
PeiBuildHobGuid (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_GUID                    *Guid,
  IN UINTN                       DataLength,
  IN OUT VOID                    **Hob
  )
/*++

Routine Description:

  Builds a custom HOB that is tagged with a GUID for identification

Arguments:

  PeiServices - The PEI core services table.

  Guid        - The GUID of the custome HOB type

  DataLength  - The size of the data payload for the GUIDed HOB

  Hob         - Pointer to the Hob

Returns:

  EFI_SUCCESS   - Hob is successfully built.
  Others        - Errors occur while creating new Hob

--*/
;

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
;

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
;

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
;

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
;

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

  BaseAddress               - The 64 bit physical address of the bsp store

  Length                    - The length of the bsp store in bytes

  MemoryType                - Memory type

Returns:

  EFI_SUCCESS               - Hob is successfully built.
  Others                    - Errors occur while creating new Hob

--*/
;

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

  Builds a HOB for the memory allocation

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
;

//
// print.c
//
UINTN
AvSPrint (
  OUT CHAR8       *StartOfBuffer,
  IN  UINTN       StrSize,
  IN  CONST CHAR8 *Format,
  IN  VA_LIST     Marker
  )
/*++

Routine Description:

  AvSPrint function to process format and place the results in Buffer. Since a 
  VA_LIST is used this rountine allows the nesting of Vararg routines. Thus 
  this is the main print working routine

Arguments:

  StartOfBuffer - Ascii buffer to print the results of the parsing of Format into.

  StrSize       - Maximum number of characters to put into buffer. Zero means 
                  no limit.

  FormatString  - Ascii format string see file header for more details.

  Marker        - Vararg list consumed by processing Format.

Returns: 

  Number of characters printed.

--*/
;

UINTN
ASPrint (
  OUT CHAR8       *Buffer,
  IN UINTN        BufferSize,
  IN CONST CHAR8  *Format,
  ...
  )
/*++

Routine Description:

  ASPrint function to process format and place the results in Buffer.

Arguments:

  Buffer     - Ascii buffer to print the results of the parsing of Format into.

  BufferSize - Maximum number of characters to put into buffer. Zero means no 
               limit.

  Format - Ascii format string see file header for more details.

  ...    - Vararg list consumed by processing Format.

Returns: 

  Number of characters printed.

--*/
;

//
// math.c
//
UINT64
MultU64x32 (
  IN  UINT64  Multiplicand,
  IN  UINTN   Multiplier
  )
/*++  
  
Routine Description:

  This routine allows a 64 bit value to be multiplied with a 32 bit 
  value returns 64bit result.
  No checking if the result is greater than 64bits

Arguments:

  Multiplicand  - multiplicand
  Multiplier    - multiplier

Returns:

  Multiplicand * Multiplier
  
--*/
;

UINT64
DivU64x32 (
  IN  UINT64  Dividend,
  IN  UINTN   Divisor,
  OUT UINTN   *Remainder OPTIONAL
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be divided with a 32 bit value returns 
  64bit result and the Remainder.
  N.B. only works for 31bit divisors!!

Arguments:

  Dividend  - dividend
  Divisor   - divisor
  Remainder - buffer for remainder
 
Returns:

  Dividend  / Divisor
  Remainder = Dividend mod Divisor

--*/
;

UINT64
RShiftU64 (
  IN  UINT64  Operand,
  IN  UINTN   Count
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be right shifted by 32 bits and returns the 
  shifted value.
  Count is valid up 63. (Only Bits 0-5 is valid for Count)

Arguments:

  Operand - Value to be shifted
  Count   - Number of times to shift right.
 
Returns:

  Value shifted right identified by the Count.

--*/
;

UINT64
LShiftU64 (
  IN  UINT64  Operand,
  IN  UINTN   Count
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be left shifted by 32 bits and 
  returns the shifted value.
  Count is valid up 63. (Only Bits 0-5 is valid for Count)

Arguments:

  Operand - Value to be shifted
  Count   - Number of times to shift left.

Returns:

  Value shifted left identified by the Count.

--*/
;

VOID
RegisterNativeCpuIo (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN VOID                     *CpuIo
  )
/*++

Routine Description:

  Register a native Cpu IO

Arguments:

  PeiServices - Calling context
  CpuIo       - CpuIo instance to register

Returns:

  None

--*/
;

VOID
GetNativeCpuIo (
  IN EFI_PEI_SERVICES         **PeiServices,
  OUT VOID                    **CpuIo
  )
/*++

Routine Description:

  Get registered Cpu IO.

Arguments:

  PeiServices - Calling context
  CpuIo       - CpuIo instance registered before

Returns:

  None

--*/
;

EFI_STATUS
EFIAPI
FindFv (
  IN     EFI_FIND_FV_PPI             *This,
  IN     EFI_PEI_SERVICES            **PeiServices,
  IN OUT UINT8                       *FvNumber,
  IN OUT EFI_FIRMWARE_VOLUME_HEADER  **FVAddress
  )
/*++

Routine Description:

  Search Fv in Hob.

Arguments:
  
  This        - Interface pointer that implement the Find Fv PPI
  
  PeiServices - Pointer to the PEI Service Table
  
  FvNumber    - On input,  the number of the fireware volume which supports FFS to locate
                On output, the next FV number which supports FFS.
  
  FVAddress   - The address of the volume which supports FFS to discover

Returns:

  EFI_SUCCESS           - An addtional FV which supports FFS found
  EFI_OUT_OF_RESOURCES  - There are no fireware volume which supports FFS for given fvnumber
  EFI_INVALID_PARAMETER - FvAddress is NULL

--*/
;



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
;

#endif
