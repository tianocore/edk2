/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  CpuIo.h

Abstract:
  *.h file for the driver

  Note: the EFIAPI on the CpuIo functions is used to glue MASM (assembler) code
  into C code. By making the MASM functions EFIAPI it ensures that a standard
  C calling convention is assumed by the compiler, reguardless of the compiler
  flags.


--*/

#ifndef _CPU_IO_H
#define _CPU_IO_H

#include <PiDxe.h>

#include <Protocol/CpuIo.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#define VOLATILE  volatile

typedef union {
  UINT8  VOLATILE  *buf;
  UINT8  VOLATILE  *ui8;
  UINT16 VOLATILE  *ui16;
  UINT32 VOLATILE  *ui32;
  UINT64 VOLATILE  *ui64;
  UINTN  VOLATILE  ui;
} PTR;

EFI_STATUS
EFIAPI
CpuIoInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  CpuIo driver entry point.

Arguments:

  ImageHandle - The firmware allocated handle for the EFI image.
  SystemTable - A pointer to the EFI System Table.

Returns:

  EFI_SUCCESS          - The driver was initialized.
  EFI_OUT_OF_RESOURCES - The request could not be completed due to a lack of resources.

--*/
;

EFI_STATUS
EFIAPI
CpuMemoryServiceRead (
  IN  EFI_CPU_IO_PROTOCOL        *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                     Address,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Perform the memory mapped I/O read service

Arguments:

  This    - Pointer to an instance of the CPU I/O Protocol
  Width   - Width of the memory mapped I/O operation
  Address - Base address of the memory mapped I/O operation
  Count   - Count of the number of accesses to perform
  Buffer  - Pointer to the destination buffer to store the results

Returns:

  EFI_SUCCESS           - The data was read.
  EFI_INVALID_PARAMETER - Width is invalid.
  EFI_INVALID_PARAMETER - Buffer is NULL.
  EFI_UNSUPPORTED       - The Buffer is not aligned for the given Width.
  EFI_UNSUPPORTED       - The address range specified by Address, Width,
                          and Count is not valid.

--*/
;

EFI_STATUS
EFIAPI
CpuMemoryServiceWrite (
  IN EFI_CPU_IO_PROTOCOL        *This,
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     Address,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  )
/*++

Routine Description:

  Perform the memory mapped I/O write service

Arguments:

  This    - Pointer to an instance of the CPU I/O Protocol
  Width   - Width of the memory mapped I/O operation
  Address - Base address of the memory mapped I/O operation
  Count   - Count of the number of accesses to perform
  Buffer  - Pointer to the source buffer from which to write data

Returns:

  EFI_SUCCESS           - The data was written.
  EFI_INVALID_PARAMETER - Width is invalid.
  EFI_INVALID_PARAMETER - Buffer is NULL.
  EFI_UNSUPPORTED       - The Buffer is not aligned for the given Width.
  EFI_UNSUPPORTED       - The address range specified by Address, Width,
                          and Count is not valid.

--*/
;

EFI_STATUS
EFIAPI
CpuIoServiceRead (
  IN  EFI_CPU_IO_PROTOCOL        *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                     UserAddress,
  IN  UINTN                      Count,
  OUT VOID                       *UserBuffer
  )
/*++

Routine Description:

  Perform the port I/O read service

Arguments:

  This    - Pointer to an instance of the CPU I/O Protocol
  Width   - Width of the port I/O operation
  Address - Base address of the port I/O operation
  Count   - Count of the number of accesses to perform
  Buffer  - Pointer to the destination buffer to store the results

Returns:

  EFI_SUCCESS           - The data was read.
  EFI_INVALID_PARAMETER - Width is invalid.
  EFI_INVALID_PARAMETER - Buffer is NULL.
  EFI_UNSUPPORTED       - The Buffer is not aligned for the given Width.
  EFI_UNSUPPORTED       - The address range specified by Address, Width,
                          and Count is not valid.

--*/
;

EFI_STATUS
EFIAPI
CpuIoServiceWrite (
  IN EFI_CPU_IO_PROTOCOL        *This,
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     UserAddress,
  IN UINTN                      Count,
  IN VOID                       *UserBuffer
  )
/*++

Routine Description:

  Perform the port I/O write service

Arguments:

  This    - Pointer to an instance of the CPU I/O Protocol
  Width   - Width of the port I/O operation
  Address - Base address of the port I/O operation
  Count   - Count of the number of accesses to perform
  Buffer  - Pointer to the source buffer from which to write data

Returns:

  EFI_SUCCESS           - The data was written.
  EFI_INVALID_PARAMETER - Width is invalid.
  EFI_INVALID_PARAMETER - Buffer is NULL.
  EFI_UNSUPPORTED       - The Buffer is not aligned for the given Width.
  EFI_UNSUPPORTED       - The address range specified by Address, Width,
                          and Count is not valid.

--*/
;

EFI_STATUS
CpuIoCheckParameter (
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     Address,
  IN UINTN                      Count,
  IN VOID                       *Buffer,
  IN UINT64                     Limit
  )
/*++

Routine Description:

  Check the validation of parameters for CPU I/O interface functions.

Arguments:

  Width   - Width of the Memory Access
  Address - Address of the Memory access
  Count   - Count of the number of accesses to perform
  Buffer  - Pointer to the buffer to read from memory
  Buffer  - Memory buffer for the I/O operation
  Limit   - Maximum address supported

Returns:

  EFI_INVALID_PARAMETER - Buffer is NULL
  EFI_UNSUPPORTED       - The address range specified by Width, Address and Count is invalid
  EFI_UNSUPPORTED       - The memory buffer is not aligned
  EFI_SUCCESS           - Parameters are OK

--*/
;

#endif
