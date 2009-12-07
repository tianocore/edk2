/** @file
  Produces the CPU I/O 2 Protocol.

Copyright (c) 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuIo2Dxe.h"

EFI_HANDLE           mHandle = NULL;
EFI_CPU_IO2_PROTOCOL mCpuIo  = {
  {
    CpuMemoryServiceRead,
    CpuMemoryServiceWrite
  },
  {
    CpuIoServiceRead,
    CpuIoServiceWrite
  }
};

/**
  Worker function to check the validation of parameters for CPU I/O interface functions.

  This function check the validation of parameters for CPU I/O interface functions.

  @param  Width                 Width of the Mmio/Io operation
  @param  Address               Base address of the Mmio/Io operation
  @param  Count                 Count of the number of accesses to perform
  @param  Buffer                Pointer to the buffer to read from memory
  @param  Limit                 Maximum address supported

  @retval EFI_INVALID_PARAMETER Buffer is NULL
  @retval EFI_UNSUPPORTED       The address range specified by Width, Address and Count is invalid
  @retval EFI_UNSUPPORTED       The memory buffer is not aligned
  @retval EFI_SUCCESS           Parameters are valid

**/
EFI_STATUS
CpuIoCheckParameter (
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     Address,
  IN UINTN                      Count,
  IN VOID                       *Buffer,
  IN UINT64                     Limit
  )
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
  if (Address - 1 + (UINT32)(1 << Width) * Count > Limit) {
    return EFI_UNSUPPORTED;
  }

  AlignMask = (1 << Width) - 1;
  if ((UINTN) Buffer & AlignMask) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Worker function to update access width and count for access to the unaligned address.
  Unaligned Io/MmIo address access, break up the request into word by word or byte by byte.

  @param  Address               Base address of the Mmio/Io operation
  @param  PtrWidth              Pointer to width of the Mmio/Io operation
                                Out, this value will be updated for access to the unaligned address.
  @param  PtrCount              Pointer to count of the number of accesses to perform
                                Out, this value will be updated for access to the unaligned address.
**/
VOID
CpuIoUpdateWidthCount (
  IN     UINT64                     Address,
  IN OUT EFI_CPU_IO_PROTOCOL_WIDTH  *PtrWidth,
  IN OUT UINTN                      *PtrCount
  )
{
  EFI_CPU_IO_PROTOCOL_WIDTH  BufferWidth;
  UINTN                      BufferCount;

  BufferWidth = *PtrWidth;
  BufferCount = *PtrCount;
  
  switch (BufferWidth) {
  case EfiCpuIoWidthUint8:
    break;

  case EfiCpuIoWidthUint16:
    if ((Address & 0x01) == 0) {
      break;
    } else {
      BufferCount = BufferCount * 2;
      BufferWidth = EfiCpuIoWidthUint8;
    }
    break;

  case EfiCpuIoWidthUint32:
    if ((Address & 0x03) == 0) {
      break;
    } else if ((Address & 0x01) == 0) {
      BufferCount = BufferCount * 2;
      BufferWidth = EfiCpuIoWidthUint16;
    } else {
      BufferCount = BufferCount * 4;
      BufferWidth = EfiCpuIoWidthUint8;
    }
    break;

  case EfiCpuIoWidthUint64:
    if ((Address & 0x07) == 0) {
      break;
    } else if ((Address & 0x03) == 0) {
      BufferCount = BufferCount * 2;
      BufferWidth = EfiCpuIoWidthUint32;
    } else if ((Address & 0x01) == 0) {
      BufferCount = BufferCount * 4;
      BufferWidth = EfiCpuIoWidthUint16;
    } else {
      BufferCount = BufferCount * 8;
      BufferWidth = EfiCpuIoWidthUint8;
    }
    break;

  default:
    return;
  }

  *PtrWidth = BufferWidth;
  *PtrCount = BufferCount;

  return;
}

/**
  Worker function to perform memory mapped I/O read/write

  This function provides private services to perform memory mapped I/O read/write.

  @param  Width                 Width of the memory mapped I/O operation
  @param  Count                 Count of the number of accesses to perform
  @param  DestinationStrideFlag Boolean flag indicates if the destination is to be incremented
  @param  Destination           Destination of the memory mapped I/O operation
  @param  SourceStrideFlag      Boolean flag indicates if the source is to be incremented
  @param  Source                Source of the memory mapped I/O operation

  @retval EFI_SUCCESS           Successful operation
  @retval EFI_INVALID_PARAMETER Width is invalid

**/
EFI_STATUS
CpuIoMemRW (
  IN  EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN  UINTN                      Count,
  IN  BOOLEAN                    DestinationStrideFlag,
  OUT PTR                        Destination,
  IN  BOOLEAN                    SourceStrideFlag,
  IN  PTR                        Source
  )
{
  UINTN Stride;
  UINTN DestinationStride;
  UINTN SourceStride;

  Width             = (EFI_CPU_IO_PROTOCOL_WIDTH) (Width & 0x03);
  Stride            = (UINT32)(1 << Width);
  DestinationStride = DestinationStrideFlag ? Stride : 0;
  SourceStride      = SourceStrideFlag ? Stride : 0;

  //
  // Loop for each iteration and move the data
  //
  switch (Width) {
  case EfiCpuIoWidthUint8:
    for (; Count > 0; Count--, Destination.Buf += DestinationStride, Source.Buf += SourceStride) {
      MmioWrite8((UINTN)Destination.Ui8 , MmioRead8((UINTN)Source.Ui8));
    }
    break;

  case EfiCpuIoWidthUint16:
    for (; Count > 0; Count--, Destination.Buf += DestinationStride, Source.Buf += SourceStride) {
      MmioWrite16((UINTN)Destination.Ui16 , MmioRead16((UINTN)Source.Ui16));
    }
    break;

  case EfiCpuIoWidthUint32:
    for (; Count > 0; Count--, Destination.Buf += DestinationStride, Source.Buf += SourceStride) {
      MmioWrite32((UINTN)Destination.Ui32 , MmioRead32((UINTN)Source.Ui32));
    }
    break;

  case EfiCpuIoWidthUint64:
    for (; Count > 0; Count--, Destination.Buf += DestinationStride, Source.Buf += SourceStride) {
      MmioWrite64((UINTN)Destination.Ui64 , MmioRead64((UINTN)Source.Ui64));
    }
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Enables a driver to read memory-mapped registers in the PI System memory space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the memory operation.
  @param[in]       Address      The base address of the memory operation.
  @param[in]       Count        The number of memory operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[out]      Buffer       The destination buffer to store the results.

  @retval EFI_SUCCESS           The data was read from or written to the EFI system.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI system. Or Buffer is NULL.
  @retval EFI_UNSUPPORTED       The Buffer is not aligned for the given Width.
                                Or,The address range specified by Address, Width, and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceRead (
  IN     EFI_CPU_IO2_PROTOCOL              *This,
  IN     EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN     UINT64                            Address,
  IN     UINTN                             Count,
  OUT    VOID                              *Buffer
  )
{
  PTR                           Source;
  PTR                           Destination;
  EFI_STATUS                    Status;
  EFI_CPU_IO_PROTOCOL_WIDTH     BufferWidth;

  Status = CpuIoCheckParameter (Width, Address, Count, Buffer, MAX_ADDRESS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Destination.Buf = Buffer;
  Source.Buf      = (VOID *) (UINTN) Address;
  
  //
  // Support access to unaligned mmio address.
  // Break up the request into byte by byte
  //
  BufferWidth     = (EFI_CPU_IO_PROTOCOL_WIDTH) (Width & 0x03);
  CpuIoUpdateWidthCount (Address, &BufferWidth, &Count);

  if (Width >= EfiCpuIoWidthUint8 && Width <= EfiCpuIoWidthUint64) {
    return CpuIoMemRW (BufferWidth, Count, TRUE, Destination, TRUE, Source);
  }

  if (Width >= EfiCpuIoWidthFifoUint8 && Width <= EfiCpuIoWidthFifoUint64) {
    return CpuIoMemRW (BufferWidth, Count, TRUE, Destination, FALSE, Source);
  }

  if (Width >= EfiCpuIoWidthFillUint8 && Width <= EfiCpuIoWidthFillUint64) {
    return CpuIoMemRW (BufferWidth, Count, FALSE, Destination, TRUE, Source);
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Enables a driver to write memory-mapped registers in the PI System memory space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the memory operation.
  @param[in]       Address      The base address of the memory operation.
  @param[in]       Count        The number of memory operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[in]       Buffer       The source buffer from which to write data.

  @retval EFI_SUCCESS           The data was read from or written to the EFI system.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI system. Or Buffer is NULL.
  @retval EFI_UNSUPPORTED       The Buffer is not aligned for the given Width.
                                Or,The address range specified by Address, Width, and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceWrite (
  IN     EFI_CPU_IO2_PROTOCOL              *This,
  IN     EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN     UINT64                            Address,
  IN     UINTN                             Count,
  IN     VOID                              *Buffer
  )
{
  PTR                        Source;
  PTR                        Destination;
  EFI_STATUS                 Status;
  EFI_CPU_IO_PROTOCOL_WIDTH  BufferWidth;

  Status = CpuIoCheckParameter (Width, Address, Count, Buffer, MAX_ADDRESS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Destination.Buf = (VOID *) (UINTN) Address;
  Source.Buf      = Buffer;

  //
  // Support access to unaligned mmio address.
  // Break up the request into byte by byte
  //
  BufferWidth     = (EFI_CPU_IO_PROTOCOL_WIDTH) (Width & 0x03);
  CpuIoUpdateWidthCount (Address, &BufferWidth, &Count);

  if (Width >= EfiCpuIoWidthUint8 && Width <= EfiCpuIoWidthUint64) {
    return CpuIoMemRW (BufferWidth, Count, TRUE, Destination, TRUE, Source);
  }

  if (Width >= EfiCpuIoWidthFifoUint8 && Width <= EfiCpuIoWidthFifoUint64) {
    return CpuIoMemRW (BufferWidth, Count, FALSE, Destination, TRUE, Source);
  }

  if (Width >= EfiCpuIoWidthFillUint8 && Width <= EfiCpuIoWidthFillUint64) {
    return CpuIoMemRW (BufferWidth, Count, TRUE, Destination, FALSE, Source);
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Enables a driver to read registers in the PI CPU I/O space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the I/O operation.
  @param[in]       UserAddress  The base address of the I/O operation. The caller is responsible
                                for aligning the Address if required. 
  @param[in]       Count        The number of I/O operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[out]      UserBuffer   The destination buffer to store the results.

  @retval EFI_SUCCESS           The data was read from or written to the EFI system.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI system. Or Buffer is NULL.
  @retval EFI_UNSUPPORTED       The Buffer is not aligned for the given Width.
                                Or,The address range specified by Address, Width, and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuIoServiceRead (
  IN     EFI_CPU_IO2_PROTOCOL              *This,
  IN     EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN     UINT64                            UserAddress,
  IN     UINTN                             Count,
  OUT    VOID                              *UserBuffer
  )
{
  UINTN                         InStride;
  UINTN                         OutStride;
  UINTN                         Address;
  PTR                           Buffer;
  EFI_STATUS                    Status;
  EFI_CPU_IO_PROTOCOL_WIDTH     BufferWidth;  

  Buffer.Buf = (UINT8 *) UserBuffer;

  if (Width >= EfiCpuIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = CpuIoCheckParameter (Width, UserAddress, Count, UserBuffer, IA32_MAX_IO_ADDRESS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Support access to unaligned IO address.
  // Break up the request into byte by byte
  //
  BufferWidth     = (EFI_CPU_IO_PROTOCOL_WIDTH) (Width & 0x03);
  CpuIoUpdateWidthCount (UserAddress, &BufferWidth, &Count);

  Address   = (UINTN) UserAddress;
  InStride  = (UINT32)(1 << (BufferWidth & 0x03));
  OutStride = InStride;
  if (Width >= EfiCpuIoWidthFifoUint8 && Width <= EfiCpuIoWidthFifoUint64) {
    InStride = 0;
  }

  if (Width >= EfiCpuIoWidthFillUint8 && Width <= EfiCpuIoWidthFillUint64) {
    OutStride = 0;
  }

  //
  // Loop for each iteration and move the data
  //
  switch (BufferWidth) {
  case EfiCpuIoWidthUint8:
    for (; Count > 0; Count--, Buffer.Buf += OutStride, Address += InStride) {
      *Buffer.Ui8 = IoRead8 (Address);
    }
    break;

  case EfiCpuIoWidthUint16:
    for (; Count > 0; Count--, Buffer.Buf += OutStride, Address += InStride) {
      *Buffer.Ui16 = IoRead16 (Address);
    }
    break;

  case EfiCpuIoWidthUint32:
    for (; Count > 0; Count--, Buffer.Buf += OutStride, Address += InStride) {
      *Buffer.Ui32 = IoRead32 (Address);
    }
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Enables a driver to write registers in the PI CPU I/O space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the I/O operation.
  @param[in]       UserAddress  The base address of the I/O operation. The caller is responsible
                                for aligning the Address if required. 
  @param[in]       Count        The number of I/O operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[in]       UserBuffer   The source buffer from which to write data.

  @retval EFI_SUCCESS           The data was read from or written to the EFI system.
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI system. Or Buffer is NULL.
  @retval EFI_UNSUPPORTED       The Buffer is not aligned for the given Width.
                                Or,The address range specified by Address, Width, and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuIoServiceWrite (
  IN     EFI_CPU_IO2_PROTOCOL              *This,
  IN     EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN     UINT64                            UserAddress,
  IN     UINTN                             Count,
  IN     VOID                              *UserBuffer
  )
{
  UINTN                         InStride;
  UINTN                         OutStride;
  UINTN                         Address;
  PTR                           Buffer;
  EFI_STATUS                    Status;
  EFI_CPU_IO_PROTOCOL_WIDTH     BufferWidth;

  Buffer.Buf = (UINT8 *) UserBuffer;

  if (Width >= EfiCpuIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = CpuIoCheckParameter (Width, UserAddress, Count, UserBuffer, IA32_MAX_IO_ADDRESS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Support access to unaligned IO address.
  // Break up the request into byte by byte
  //
  BufferWidth = (EFI_CPU_IO_PROTOCOL_WIDTH) (Width & 0x03);
  CpuIoUpdateWidthCount (UserAddress, &BufferWidth, &Count);

  Address   = (UINTN) UserAddress;
  InStride  = (UINT32)(1 << (BufferWidth & 0x03));
  OutStride = InStride;
  if (Width >= EfiCpuIoWidthFifoUint8 && Width <= EfiCpuIoWidthFifoUint64) {
    InStride = 0;
  }

  if (Width >= EfiCpuIoWidthFillUint8 && Width <= EfiCpuIoWidthFillUint64) {
    OutStride = 0;
  }

  //
  // Loop for each iteration and move the data
  //
  switch (BufferWidth) {
  case EfiCpuIoWidthUint8:
    for (; Count > 0; Count--, Buffer.Buf += OutStride, Address += InStride) {
      IoWrite8 (Address, *Buffer.Ui8);
    }
    break;

  case EfiCpuIoWidthUint16:
    for (; Count > 0; Count--, Buffer.Buf += OutStride, Address += InStride) {
      IoWrite16 (Address, *Buffer.Ui16);
    }
    break;

  case EfiCpuIoWidthUint32:
    for (; Count > 0; Count--, Buffer.Buf += OutStride, Address += InStride) {
      IoWrite32 (Address, *Buffer.Ui32);
    }
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Entrypoint of CPU I/O 2 DXE module.
  
  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
CpuIo2Initialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS      Status;

  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiCpuIo2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mCpuIo
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
