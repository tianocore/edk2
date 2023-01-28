/** @file
  Produces the CPU I/O 2 Protocol.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
Copyright (c) 2022, Ventana Micro Systems Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/CpuIo2.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define MAX_IO_PORT_ADDRESS  0xFFFF

//
// Handle for the CPU I/O 2 Protocol
//
STATIC EFI_HANDLE  mHandle = NULL;

//
// Lookup table for increment values based on transfer widths
//
STATIC CONST UINT8  mInStride[] = {
  1, // EfiCpuIoWidthUint8
  2, // EfiCpuIoWidthUint16
  4, // EfiCpuIoWidthUint32
  8, // EfiCpuIoWidthUint64
  0, // EfiCpuIoWidthFifoUint8
  0, // EfiCpuIoWidthFifoUint16
  0, // EfiCpuIoWidthFifoUint32
  0, // EfiCpuIoWidthFifoUint64
  1, // EfiCpuIoWidthFillUint8
  2, // EfiCpuIoWidthFillUint16
  4, // EfiCpuIoWidthFillUint32
  8  // EfiCpuIoWidthFillUint64
};

//
// Lookup table for increment values based on transfer widths
//
STATIC CONST UINT8  mOutStride[] = {
  1, // EfiCpuIoWidthUint8
  2, // EfiCpuIoWidthUint16
  4, // EfiCpuIoWidthUint32
  8, // EfiCpuIoWidthUint64
  1, // EfiCpuIoWidthFifoUint8
  2, // EfiCpuIoWidthFifoUint16
  4, // EfiCpuIoWidthFifoUint32
  8, // EfiCpuIoWidthFifoUint64
  0, // EfiCpuIoWidthFillUint8
  0, // EfiCpuIoWidthFillUint16
  0, // EfiCpuIoWidthFillUint32
  0  // EfiCpuIoWidthFillUint64
};

/**
  Check parameters to a CPU I/O 2 Protocol service request.

  The I/O operations are carried out exactly as requested. The caller is responsible
  for satisfying any alignment and I/O width restrictions that a PI System on a
  platform might require. For example on some platforms, width requests of
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will
  be handled by the driver.

  @param[in] MmioOperation  TRUE for an MMIO operation, FALSE for I/O Port operation.
  @param[in] Width          Signifies the width of the I/O or Memory operation.
  @param[in] Address        The base address of the I/O operation.
  @param[in] Count          The number of I/O operations to perform. The number of
                            bytes moved is Width size * Count, starting at Address.
  @param[in] Buffer         For read operations, the destination buffer to store the results.
                            For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The parameters for this request pass the checks.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this PI system.

**/
STATIC
EFI_STATUS
CpuIoCheckParameter (
  IN BOOLEAN                    MmioOperation,
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     Address,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  )
{
  UINT64  MaxCount;
  UINT64  Limit;

  //
  // Check to see if Buffer is NULL
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see if Width is in the valid range
  //
  if ((UINT32)Width >= EfiCpuIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // For FIFO type, the target address won't increase during the access,
  // so treat Count as 1
  //
  if ((Width >= EfiCpuIoWidthFifoUint8) && (Width <= EfiCpuIoWidthFifoUint64)) {
    Count = 1;
  }

  //
  // Check to see if Width is in the valid range for I/O Port operations
  //
  Width = (EFI_CPU_IO_PROTOCOL_WIDTH)(Width & 0x03);
  if (!MmioOperation && (Width == EfiCpuIoWidthUint64)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see if Address is aligned
  //
  if ((Address & (UINT64)(mInStride[Width] - 1)) != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check to see if any address associated with this transfer exceeds the maximum
  // allowed address.  The maximum address implied by the parameters passed in is
  // Address + Size * Count.  If the following condition is met, then the transfer
  // is not supported.
  //
  //    Address + Size * Count > (MmioOperation ? MAX_ADDRESS : MAX_IO_PORT_ADDRESS) + 1
  //
  // Since MAX_ADDRESS can be the maximum integer value supported by the CPU and Count
  // can also be the maximum integer value supported by the CPU, this range
  // check must be adjusted to avoid all overflow conditions.
  //
  // The following form of the range check is equivalent but assumes that
  // MAX_ADDRESS and MAX_IO_PORT_ADDRESS are of the form (2^n - 1).
  //
  Limit = (MmioOperation ? MAX_ADDRESS : MAX_IO_PORT_ADDRESS);
  if (Count == 0) {
    if (Address > Limit) {
      return EFI_UNSUPPORTED;
    }
  } else {
    MaxCount = RShiftU64 (Limit, Width);
    if (MaxCount < (Count - 1)) {
      return EFI_UNSUPPORTED;
    }

    if (Address > LShiftU64 (MaxCount - Count + 1, Width)) {
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Check to see if Buffer is aligned
  //
  if (((UINTN)Buffer & ((MIN (sizeof (UINTN), mInStride[Width])  - 1))) != 0) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Reads memory-mapped registers.

  The I/O operations are carried out exactly as requested. The caller is responsible
  for satisfying any alignment and I/O width restrictions that a PI System on a
  platform might require. For example on some platforms, width requests of
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will
  be handled by the driver.

  If Width is EfiCpuIoWidthUint8, EfiCpuIoWidthUint16, EfiCpuIoWidthUint32,
  or EfiCpuIoWidthUint64, then both Address and Buffer are incremented for
  each of the Count operations that is performed.

  If Width is EfiCpuIoWidthFifoUint8, EfiCpuIoWidthFifoUint16,
  EfiCpuIoWidthFifoUint32, or EfiCpuIoWidthFifoUint64, then only Buffer is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times on the same Address.

  If Width is EfiCpuIoWidthFillUint8, EfiCpuIoWidthFillUint16,
  EfiCpuIoWidthFillUint32, or EfiCpuIoWidthFillUint64, then only Address is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times from the first element of Buffer.

  @param[in]  This     A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O or Memory operation.
  @param[in]  Address  The base address of the I/O operation.
  @param[in]  Count    The number of I/O operations to perform. The number of
                       bytes moved is Width size * Count, starting at Address.
  @param[out] Buffer   For read operations, the destination buffer to store the results.
                       For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the PI system.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this PI system.

**/
STATIC
EFI_STATUS
EFIAPI
CpuMemoryServiceRead (
  IN  EFI_CPU_IO2_PROTOCOL       *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                     Address,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  )
{
  EFI_STATUS                 Status;
  UINT8                      InStride;
  UINT8                      OutStride;
  EFI_CPU_IO_PROTOCOL_WIDTH  OperationWidth;
  UINT8                      *Uint8Buffer;

  Status = CpuIoCheckParameter (TRUE, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select loop based on the width of the transfer
  //
  InStride       = mInStride[Width];
  OutStride      = mOutStride[Width];
  OperationWidth = (EFI_CPU_IO_PROTOCOL_WIDTH)(Width & 0x03);
  for (Uint8Buffer = Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (OperationWidth == EfiCpuIoWidthUint8) {
      *Uint8Buffer = MmioRead8 ((UINTN)Address);
    } else if (OperationWidth == EfiCpuIoWidthUint16) {
      *((UINT16 *)Uint8Buffer) = MmioRead16 ((UINTN)Address);
    } else if (OperationWidth == EfiCpuIoWidthUint32) {
      *((UINT32 *)Uint8Buffer) = MmioRead32 ((UINTN)Address);
    } else if (OperationWidth == EfiCpuIoWidthUint64) {
      *((UINT64 *)Uint8Buffer) = MmioRead64 ((UINTN)Address);
    }
  }

  return EFI_SUCCESS;
}

/**
  Writes memory-mapped registers.

  The I/O operations are carried out exactly as requested. The caller is responsible
  for satisfying any alignment and I/O width restrictions that a PI System on a
  platform might require. For example on some platforms, width requests of
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will
  be handled by the driver.

  If Width is EfiCpuIoWidthUint8, EfiCpuIoWidthUint16, EfiCpuIoWidthUint32,
  or EfiCpuIoWidthUint64, then both Address and Buffer are incremented for
  each of the Count operations that is performed.

  If Width is EfiCpuIoWidthFifoUint8, EfiCpuIoWidthFifoUint16,
  EfiCpuIoWidthFifoUint32, or EfiCpuIoWidthFifoUint64, then only Buffer is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times on the same Address.

  If Width is EfiCpuIoWidthFillUint8, EfiCpuIoWidthFillUint16,
  EfiCpuIoWidthFillUint32, or EfiCpuIoWidthFillUint64, then only Address is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times from the first element of Buffer.

  @param[in]  This     A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O or Memory operation.
  @param[in]  Address  The base address of the I/O operation.
  @param[in]  Count    The number of I/O operations to perform. The number of
                       bytes moved is Width size * Count, starting at Address.
  @param[in]  Buffer   For read operations, the destination buffer to store the results.
                       For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the PI system.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this PI system.

**/
STATIC
EFI_STATUS
EFIAPI
CpuMemoryServiceWrite (
  IN EFI_CPU_IO2_PROTOCOL       *This,
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     Address,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  )
{
  EFI_STATUS                 Status;
  UINT8                      InStride;
  UINT8                      OutStride;
  EFI_CPU_IO_PROTOCOL_WIDTH  OperationWidth;
  UINT8                      *Uint8Buffer;

  Status = CpuIoCheckParameter (TRUE, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select loop based on the width of the transfer
  //
  InStride       = mInStride[Width];
  OutStride      = mOutStride[Width];
  OperationWidth = (EFI_CPU_IO_PROTOCOL_WIDTH)(Width & 0x03);
  for (Uint8Buffer = Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (OperationWidth == EfiCpuIoWidthUint8) {
      MmioWrite8 ((UINTN)Address, *Uint8Buffer);
    } else if (OperationWidth == EfiCpuIoWidthUint16) {
      MmioWrite16 ((UINTN)Address, *((UINT16 *)Uint8Buffer));
    } else if (OperationWidth == EfiCpuIoWidthUint32) {
      MmioWrite32 ((UINTN)Address, *((UINT32 *)Uint8Buffer));
    } else if (OperationWidth == EfiCpuIoWidthUint64) {
      MmioWrite64 ((UINTN)Address, *((UINT64 *)Uint8Buffer));
    }
  }

  return EFI_SUCCESS;
}

/**
  Reads I/O registers.

  The I/O operations are carried out exactly as requested. The caller is responsible
  for satisfying any alignment and I/O width restrictions that a PI System on a
  platform might require. For example on some platforms, width requests of
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will
  be handled by the driver.

  If Width is EfiCpuIoWidthUint8, EfiCpuIoWidthUint16, EfiCpuIoWidthUint32,
  or EfiCpuIoWidthUint64, then both Address and Buffer are incremented for
  each of the Count operations that is performed.

  If Width is EfiCpuIoWidthFifoUint8, EfiCpuIoWidthFifoUint16,
  EfiCpuIoWidthFifoUint32, or EfiCpuIoWidthFifoUint64, then only Buffer is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times on the same Address.

  If Width is EfiCpuIoWidthFillUint8, EfiCpuIoWidthFillUint16,
  EfiCpuIoWidthFillUint32, or EfiCpuIoWidthFillUint64, then only Address is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times from the first element of Buffer.

  @param[in]  This     A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O or Memory operation.
  @param[in]  Address  The base address of the I/O operation.
  @param[in]  Count    The number of I/O operations to perform. The number of
                       bytes moved is Width size * Count, starting at Address.
  @param[out] Buffer   For read operations, the destination buffer to store the results.
                       For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the PI system.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this PI system.

**/
STATIC
EFI_STATUS
EFIAPI
CpuIoServiceRead (
  IN  EFI_CPU_IO2_PROTOCOL       *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                     Address,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  )
{
  EFI_STATUS                 Status;
  UINT8                      InStride;
  UINT8                      OutStride;
  EFI_CPU_IO_PROTOCOL_WIDTH  OperationWidth;
  UINT8                      *Uint8Buffer;

  Status = CpuIoCheckParameter (FALSE, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Address += PcdGet64 (PcdPciIoTranslation);

  //
  // Select loop based on the width of the transfer
  //
  InStride       = mInStride[Width];
  OutStride      = mOutStride[Width];
  OperationWidth = (EFI_CPU_IO_PROTOCOL_WIDTH)(Width & 0x03);

  for (Uint8Buffer = Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (OperationWidth == EfiCpuIoWidthUint8) {
      *Uint8Buffer = MmioRead8 ((UINTN)Address);
    } else if (OperationWidth == EfiCpuIoWidthUint16) {
      *((UINT16 *)Uint8Buffer) = MmioRead16 ((UINTN)Address);
    } else if (OperationWidth == EfiCpuIoWidthUint32) {
      *((UINT32 *)Uint8Buffer) = MmioRead32 ((UINTN)Address);
    }
  }

  return EFI_SUCCESS;
}

/**
  Write I/O registers.

  The I/O operations are carried out exactly as requested. The caller is responsible
  for satisfying any alignment and I/O width restrictions that a PI System on a
  platform might require. For example on some platforms, width requests of
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will
  be handled by the driver.

  If Width is EfiCpuIoWidthUint8, EfiCpuIoWidthUint16, EfiCpuIoWidthUint32,
  or EfiCpuIoWidthUint64, then both Address and Buffer are incremented for
  each of the Count operations that is performed.

  If Width is EfiCpuIoWidthFifoUint8, EfiCpuIoWidthFifoUint16,
  EfiCpuIoWidthFifoUint32, or EfiCpuIoWidthFifoUint64, then only Buffer is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times on the same Address.

  If Width is EfiCpuIoWidthFillUint8, EfiCpuIoWidthFillUint16,
  EfiCpuIoWidthFillUint32, or EfiCpuIoWidthFillUint64, then only Address is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times from the first element of Buffer.

  @param[in]  This     A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O or Memory operation.
  @param[in]  Address  The base address of the I/O operation.
  @param[in]  Count    The number of I/O operations to perform. The number of
                       bytes moved is Width size * Count, starting at Address.
  @param[in]  Buffer   For read operations, the destination buffer to store the results.
                       For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the PI system.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this PI system.

**/
STATIC
EFI_STATUS
EFIAPI
CpuIoServiceWrite (
  IN EFI_CPU_IO2_PROTOCOL       *This,
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     Address,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  )
{
  EFI_STATUS                 Status;
  UINT8                      InStride;
  UINT8                      OutStride;
  EFI_CPU_IO_PROTOCOL_WIDTH  OperationWidth;
  UINT8                      *Uint8Buffer;

  //
  // Make sure the parameters are valid
  //
  Status = CpuIoCheckParameter (FALSE, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Address += PcdGet64 (PcdPciIoTranslation);

  //
  // Select loop based on the width of the transfer
  //
  InStride       = mInStride[Width];
  OutStride      = mOutStride[Width];
  OperationWidth = (EFI_CPU_IO_PROTOCOL_WIDTH)(Width & 0x03);

  for (Uint8Buffer = (UINT8 *)Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (OperationWidth == EfiCpuIoWidthUint8) {
      MmioWrite8 ((UINTN)Address, *Uint8Buffer);
    } else if (OperationWidth == EfiCpuIoWidthUint16) {
      MmioWrite16 ((UINTN)Address, *((UINT16 *)Uint8Buffer));
    } else if (OperationWidth == EfiCpuIoWidthUint32) {
      MmioWrite32 ((UINTN)Address, *((UINT32 *)Uint8Buffer));
    }
  }

  return EFI_SUCCESS;
}

//
// CPU I/O 2 Protocol instance
//
STATIC EFI_CPU_IO2_PROTOCOL  mCpuIo2 = {
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
  The user Entry Point for module CpuIo2Dxe. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
PciCpuIo2Initialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiCpuIo2ProtocolGuid);
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEfiCpuIo2ProtocolGuid,
                  &mCpuIo2,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
