/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  EfiCombinationLib.h

Abstract:

  Library functions that can be called in both PEI and DXE phase

--*/

#ifndef _EFI_COMBINATION_LIB_H_
#define _EFI_COMBINATION_LIB_H_

EFI_STATUS
EfiInitializeCommonDriverLib (
  IN EFI_HANDLE   ImageHandle,
  IN VOID         *SystemTable
  )
/*++

Routine Description:

  Initialize lib function calling phase: PEI or DXE
  
Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
	
  SystemTable     -	A pointer to the EFI System Table.

Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
;

EFI_STATUS
EfiCommonIoRead (
  IN  UINT8       Width,
  IN  UINTN       Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Io read operation.

Arguments:

  Width   - Width of read operation
  Address - Start IO address to read
  Count   - Read count
  Buffer  - Buffer to store result

Returns:

  Status code

--*/
;

EFI_STATUS
EfiCommonIoWrite (
  IN  UINT8       Width,
  IN  UINTN       Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Io write operation.

Arguments:

  Width   - Width of write operation
  Address - Start IO address to write
  Count   - Write count
  Buffer  - Buffer to write to the address

Returns:

  Status code

--*/
;

EFI_STATUS
EfiCommonPciRead (
  IN  UINT8       Width,
  IN  UINT64      Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Pci read operation

Arguments:

  Width   - Width of PCI read
  Address - PCI address to read
  Count   - Read count
  Buffer  - Output buffer for the read

Returns:

  Status code

--*/
;

EFI_STATUS
EfiCommonPciWrite (
  IN  UINT8       Width,
  IN  UINT64      Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Pci write operation

Arguments:

  Width   - Width of PCI write
  Address - PCI address to write
  Count   - Write count
  Buffer  - Buffer to write to the address

Returns:

  Status code

--*/
;

EFI_STATUS
EfiCommonStall (
  IN  UINTN      Microseconds
  )
/*++

Routine Description:

  Induces a fine-grained stall.

Arguments:

  Microseconds  - The number of microseconds to stall execution.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiCommonCopyMem (
  IN VOID       *Destination,
  IN VOID       *Source,
  IN UINTN      Length
  )
/*++

Routine Description:

  Copy Length bytes from Source to Destination.

Arguments:

  Destination - Target of copy

  Source      - Place to copy from

  Length      - Number of bytes to copy

Returns:

  Status code

--*/
;

EFI_STATUS
EfiCommonAllocatePages (
  IN EFI_ALLOCATE_TYPE          Type,
  IN EFI_MEMORY_TYPE            MemoryType,
  IN UINTN                      Pages,
  IN OUT EFI_PHYSICAL_ADDRESS   *Memory
  )
/*++

Routine Description:

  Allocates memory pages from the system.

Arguments:

  Type        - The type of allocation to perform.
  MemoryType  - The type of memory to allocate.
  Pages       - The number of contiguous pages to allocate.
  Memory      - Pointer to a physical address.

Returns:

  EFI_OUT_OF_RESOURCES			-	The pages could not be allocated.
  
  EFI_INVALID_PARAMETER			-	Invalid parameter
  
  EFI_NOT_FOUND							-	The requested pages could not be found.
  
  EFI_SUCCESS								-	The requested pages were allocated.

--*/
;

EFI_STATUS
EfiCommonLocateInterface (
  IN EFI_GUID                   *Guid,
  OUT VOID                      **Interface
  )
/*++

Routine Description:

  Returns the first protocol instance that matches the given protocol.

Arguments:

  Guid      - Provides the protocol to search for.
  Interface - On return, a pointer to the first interface that matches Protocol

Returns:

  Status code

--*/
;

EFI_STATUS
EfiCommonReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
/*++

Routine Description:

  Status Code reporter

Arguments:
  
  CodeType    - Type of Status Code.
  
  Value       - Value to output for Status Code.
  
  Instance    - Instance Number of this status code.
  
  CallerId    - ID of the caller of this status code.
  
  Data        - Optional data associated with this status code.

Returns:

	Status code
	
--*/
;

#endif
