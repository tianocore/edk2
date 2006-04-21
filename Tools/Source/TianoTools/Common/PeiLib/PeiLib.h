/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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
#include "peiHobLib.h"
#include EFI_PROTOCOL_DEFINITION (Decompress)
#include EFI_PROTOCOL_DEFINITION (TianoDecompress)
#include EFI_GUID_DEFINITION (PeiPeCoffLoader)

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

VOID
PeiPerfMeasure (
  EFI_PEI_SERVICES              **PeiServices,
  IN UINT16                     *Token,
  IN EFI_FFS_FILE_HEADER        *FileHeader,
  IN BOOLEAN                    EntryExit,
  IN UINT64                     Value
  )
/*++

Routine Description:

  Log a timestamp count.

Arguments:

  PeiServices - Pointer to the PEI Core Services table
  
  Token       - Pointer to Token Name
  
  FileHeader  - Pointer to the file header

  EntryExit   - Indicates start or stop measurement

  Value       - The start time or the stop time

Returns:

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

#ifdef EFI_PEI_PERFORMANCE
#define PEI_PERF_START(Ps, Token, FileHeader, Value)  PeiPerfMeasure (Ps, Token, FileHeader, FALSE, Value)
#define PEI_PERF_END(Ps, Token, FileHeader, Value)    PeiPerfMeasure (Ps, Token, FileHeader, TRUE, Value)
#else
#define PEI_PERF_START(Ps, Token, FileHeader, Value)
#define PEI_PERF_END(Ps, Token, FileHeader, Value)
#endif

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

#endif
