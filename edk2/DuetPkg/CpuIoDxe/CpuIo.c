/*++

Copyright (c) 2004 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  CpuIo.c

Abstract:

  This is the code that publishes the CPU I/O Protocol.
  The intent herein is to have a single I/O service that can load
  as early as possible, extend into runtime, and be layered upon by
  the implementations of architectural protocols and the PCI Root
  Bridge I/O Protocol.

--*/

#include "CpuIo.h"
#include "CpuIoAccess.h"

#define IA32_MAX_IO_ADDRESS   0xFFFF

EFI_CPU_IO_PROTOCOL mCpuIo = {
  {
    CpuMemoryServiceRead,
    CpuMemoryServiceWrite
  },
  {
    CpuIoServiceRead,
    CpuIoServiceWrite
  }
};

EFI_STATUS
CpuIoMemRW (
  IN  EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN  UINTN                      Count,
  IN  BOOLEAN                    DestinationStrideFlag,
  OUT PTR                        Destination,
  IN  BOOLEAN                    SourceStrideFlag,
  IN  PTR                        Source
  )
/*++

Routine Description:

  Private service to perform memory mapped I/O read/write

Arguments:

  Width                 - Width of the memory mapped I/O operation
  Count                 - Count of the number of accesses to perform
  DestinationStrideFlag - Boolean flag indicates if the destination is to be incremented
  Destination           - Destination of the memory mapped I/O operation
  SourceStrideFlag      - Boolean flag indicates if the source is to be incremented
  Source                - Source of the memory mapped I/O operation

Returns:

  EFI_SUCCESS           - Successful operation
  EFI_INVALID_PARAMETER - Width is invalid

--*/
{
  UINTN Stride;
  UINTN DestinationStride;
  UINTN SourceStride;

  Width             = (EFI_CPU_IO_PROTOCOL_WIDTH) (Width & 0x03);
  Stride            = (UINTN)1 << Width;
  DestinationStride = DestinationStrideFlag ? Stride : 0;
  SourceStride      = SourceStrideFlag ? Stride : 0;

  //
  // Loop for each iteration and move the data
  //
  switch (Width) {
  case EfiCpuIoWidthUint8:
    for (; Count > 0; Count--, Destination.buf += DestinationStride, Source.buf += SourceStride) {
      MemoryFence();
      *Destination.ui8 = *Source.ui8;
      MemoryFence();
    }
    break;

  case EfiCpuIoWidthUint16:
    for (; Count > 0; Count--, Destination.buf += DestinationStride, Source.buf += SourceStride) {
      MemoryFence ();
      *Destination.ui16 = *Source.ui16;
      MemoryFence ();
    }
    break;

  case EfiCpuIoWidthUint32:
    for (; Count > 0; Count--, Destination.buf += DestinationStride, Source.buf += SourceStride) {
      MemoryFence ();
      *Destination.ui32 = *Source.ui32;
      MemoryFence ();
    }
    break;

  case EfiCpuIoWidthUint64:
    for (; Count > 0; Count--, Destination.buf += DestinationStride, Source.buf += SourceStride) {
      MemoryFence ();
      *Destination.ui64 = *Source.ui64;
      MemoryFence ();
    }
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

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
{
  PTR        Source;
  PTR        Destination;
  EFI_STATUS Status;

  Status = CpuIoCheckParameter (Width, Address, Count, Buffer, EFI_MAX_ADDRESS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Destination.buf = Buffer;
  Source.buf      = (VOID *) (UINTN) Address;

  if (Width >= EfiCpuIoWidthUint8 && Width <= EfiCpuIoWidthUint64) {
    return CpuIoMemRW (Width, Count, TRUE, Destination, TRUE, Source);
  }

  if (Width >= EfiCpuIoWidthFifoUint8 && Width <= EfiCpuIoWidthFifoUint64) {
    return CpuIoMemRW (Width, Count, TRUE, Destination, FALSE, Source);
  }

  if (Width >= EfiCpuIoWidthFillUint8 && Width <= EfiCpuIoWidthFillUint64) {
    return CpuIoMemRW (Width, Count, FALSE, Destination, TRUE, Source);
  }

  return EFI_INVALID_PARAMETER;
}

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
{
  PTR        Source;
  PTR        Destination;
  EFI_STATUS Status;

  Status = CpuIoCheckParameter (Width, Address, Count, Buffer, EFI_MAX_ADDRESS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Destination.buf = (VOID *) (UINTN) Address;
  Source.buf      = Buffer;

  if (Width >= EfiCpuIoWidthUint8 && Width <= EfiCpuIoWidthUint64) {
    return CpuIoMemRW (Width, Count, TRUE, Destination, TRUE, Source);
  }

  if (Width >= EfiCpuIoWidthFifoUint8 && Width <= EfiCpuIoWidthFifoUint64) {
    return CpuIoMemRW (Width, Count, FALSE, Destination, TRUE, Source);
  }

  if (Width >= EfiCpuIoWidthFillUint8 && Width <= EfiCpuIoWidthFillUint64) {
    return CpuIoMemRW (Width, Count, TRUE, Destination, FALSE, Source);
  }

  return EFI_INVALID_PARAMETER;
}

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
{
  UINTN      InStride;
  UINTN      OutStride;
  UINTN      Address;
  PTR        Buffer;
  EFI_STATUS Status;

  Buffer.buf = (UINT8 *) UserBuffer;

  if (Width >= EfiCpuIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = CpuIoCheckParameter (Width, UserAddress, Count, UserBuffer, IA32_MAX_IO_ADDRESS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Address   = (UINTN) UserAddress;
  InStride  = (UINTN)1 << (Width & 0x03);
  OutStride = InStride;
  if (Width >= EfiCpuIoWidthFifoUint8 && Width <= EfiCpuIoWidthFifoUint64) {
    InStride = 0;
  }

  if (Width >= EfiCpuIoWidthFillUint8 && Width <= EfiCpuIoWidthFillUint64) {
    OutStride = 0;
  }

  Width = (EFI_CPU_IO_PROTOCOL_WIDTH) (Width & 0x03);

  //
  // Loop for each iteration and move the data
  //
  switch (Width) {
  case EfiCpuIoWidthUint8:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      *Buffer.ui8 = CpuIoRead8 ((UINT16) Address);
    }
    break;

  case EfiCpuIoWidthUint16:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      *Buffer.ui16 = CpuIoRead16 ((UINT16) Address);
    }
    break;

  case EfiCpuIoWidthUint32:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      *Buffer.ui32 = CpuIoRead32 ((UINT16) Address);
    }
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

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
{
  UINTN      InStride;
  UINTN      OutStride;
  UINTN      Address;
  PTR        Buffer;
  EFI_STATUS Status;

  Buffer.buf = (UINT8 *) UserBuffer;

  if (Width >= EfiCpuIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = CpuIoCheckParameter (Width, UserAddress, Count, UserBuffer, IA32_MAX_IO_ADDRESS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Address   = (UINTN) UserAddress;
  InStride  = (UINTN)1 << (Width & 0x03);
  OutStride = InStride;
  if (Width >= EfiCpuIoWidthFifoUint8 && Width <= EfiCpuIoWidthFifoUint64) {
    InStride = 0;
  }

  if (Width >= EfiCpuIoWidthFillUint8 && Width <= EfiCpuIoWidthFillUint64) {
    OutStride = 0;
  }

  Width = (EFI_CPU_IO_PROTOCOL_WIDTH) (Width & 0x03);

  //
  // Loop for each iteration and move the data
  //
  switch (Width) {
  case EfiCpuIoWidthUint8:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      CpuIoWrite8 ((UINT16) Address, *Buffer.ui8);
    }
    break;

  case EfiCpuIoWidthUint16:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      CpuIoWrite16 ((UINT16) Address, *Buffer.ui16);
    }
    break;

  case EfiCpuIoWidthUint32:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      CpuIoWrite32 ((UINT16) Address, *Buffer.ui32);
    }
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

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
{
  EFI_STATUS Status;
  EFI_HANDLE Handle;

  Handle = NULL;
  Status = SystemTable->BootServices->InstallProtocolInterface (
                                        &Handle,
                                        &gEfiCpuIoProtocolGuid,
                                        EFI_NATIVE_INTERFACE,
                                        &mCpuIo
                                        );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

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
{
  UINTN AlignMask;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Address > Limit) {
    return EFI_UNSUPPORTED;
  }

  //
  // For FiFo type, the target address won't increase during the access,
  // so treat count as 1
  //
  if (Width >= EfiCpuIoWidthFifoUint8 && Width <= EfiCpuIoWidthFifoUint64) {
    Count = 1;
  }

  Width = (EFI_CPU_IO_PROTOCOL_WIDTH) (Width & 0x03);
  if (Address - 1 + ((UINTN)1 << Width) * Count > Limit) {
    return EFI_UNSUPPORTED;
  }

  AlignMask = ((UINTN)1 << Width) - 1;
  if ((UINTN) Buffer & AlignMask) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}
