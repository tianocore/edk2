/** @file
  Produces the CPU I/O PPI.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuIoPei.h"

//
// Instance of CPU I/O PPI
//
EFI_PEI_CPU_IO_PPI  gCpuIoPpi = {
  {
    CpuMemoryServiceRead,
    CpuMemoryServiceWrite
  },
  {
    CpuIoServiceRead,
    CpuIoServiceWrite
  },
  CpuIoRead8,
  CpuIoRead16,
  CpuIoRead32,
  CpuIoRead64,
  CpuIoWrite8,
  CpuIoWrite16,
  CpuIoWrite32,
  CpuIoWrite64,
  CpuMemRead8,
  CpuMemRead16,
  CpuMemRead32,
  CpuMemRead64,
  CpuMemWrite8,
  CpuMemWrite16,
  CpuMemWrite32,
  CpuMemWrite64
};

//
// PPI Descriptor used to install the CPU I/O PPI
//
EFI_PEI_PPI_DESCRIPTOR  gPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiCpuIoPpiInstalledGuid,
  NULL
};

//
// Lookup table for increment values based on transfer widths
//
UINT8  mInStride[] = {
  1, // EfiPeiCpuIoWidthUint8
  2, // EfiPeiCpuIoWidthUint16
  4, // EfiPeiCpuIoWidthUint32
  8, // EfiPeiCpuIoWidthUint64
  0, // EfiPeiCpuIoWidthFifoUint8
  0, // EfiPeiCpuIoWidthFifoUint16
  0, // EfiPeiCpuIoWidthFifoUint32
  0, // EfiPeiCpuIoWidthFifoUint64
  1, // EfiPeiCpuIoWidthFillUint8
  2, // EfiPeiCpuIoWidthFillUint16
  4, // EfiPeiCpuIoWidthFillUint32
  8  // EfiPeiCpuIoWidthFillUint64
};

//
// Lookup table for increment values based on transfer widths
//
UINT8  mOutStride[] = {
  1, // EfiPeiCpuIoWidthUint8
  2, // EfiPeiCpuIoWidthUint16
  4, // EfiPeiCpuIoWidthUint32
  8, // EfiPeiCpuIoWidthUint64
  1, // EfiPeiCpuIoWidthFifoUint8
  2, // EfiPeiCpuIoWidthFifoUint16
  4, // EfiPeiCpuIoWidthFifoUint32
  8, // EfiPeiCpuIoWidthFifoUint64
  0, // EfiPeiCpuIoWidthFillUint8
  0, // EfiPeiCpuIoWidthFillUint16
  0, // EfiPeiCpuIoWidthFillUint32
  0  // EfiPeiCpuIoWidthFillUint64
};

/**
  Check parameters to a CPU I/O PPI service request.

  @param[in]  MmioOperation  TRUE for an MMIO operation, FALSE for I/O Port operation.
  @param[in]  Width          The width of the access. Enumerated in bytes.
  @param[in]  Address        The physical address of the access.
  @param[in]  Count          The number of accesses to perform.
  @param[in]  Buffer         A pointer to the buffer of data.

  @retval EFI_SUCCESS            The parameters for this request pass the checks.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this EFI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this EFI system.

**/
EFI_STATUS
CpuIoCheckParameter (
  IN BOOLEAN                   MmioOperation,
  IN EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN UINT64                    Address,
  IN UINTN                     Count,
  IN VOID                      *Buffer
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
  if ((UINT32)Width >= EfiPeiCpuIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // For FIFO type, the target address won't increase during the access,
  // so treat Count as 1
  //
  if ((Width >= EfiPeiCpuIoWidthFifoUint8) && (Width <= EfiPeiCpuIoWidthFifoUint64)) {
    Count = 1;
  }

  //
  // Check to see if Width is in the valid range for I/O Port operations
  //
  Width = (EFI_PEI_CPU_IO_PPI_WIDTH)(Width & 0x03);
  if (!MmioOperation && (Width == EfiPeiCpuIoWidthUint64)) {
    return EFI_INVALID_PARAMETER;
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

  return EFI_SUCCESS;
}

/**
  Reads memory-mapped registers.

  @param[in]  PeiServices  An indirect pointer to the PEI Services Table
                           published by the PEI Foundation.
  @param[in]  This         Pointer to local data for the interface.
  @param[in]  Width        The width of the access. Enumerated in bytes.
  @param[in]  Address      The physical address of the access.
  @param[in]  Count        The number of accesses to perform.
  @param[out] Buffer       A pointer to the buffer of data.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this EFI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceRead (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN  UINT64                    Address,
  IN  UINTN                     Count,
  OUT VOID                      *Buffer
  )
{
  EFI_STATUS                Status;
  UINT8                     InStride;
  UINT8                     OutStride;
  EFI_PEI_CPU_IO_PPI_WIDTH  OperationWidth;
  BOOLEAN                   Aligned;
  UINT8                     *Uint8Buffer;

  Status = CpuIoCheckParameter (TRUE, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select loop based on the width of the transfer
  //
  InStride       = mInStride[Width];
  OutStride      = mOutStride[Width];
  OperationWidth = (EFI_PEI_CPU_IO_PPI_WIDTH)(Width & 0x03);
  Aligned        = (BOOLEAN)(((UINTN)Buffer & (mInStride[OperationWidth] - 1)) == 0x00);
  for (Uint8Buffer = Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (OperationWidth == EfiPeiCpuIoWidthUint8) {
      *Uint8Buffer = MmioRead8 ((UINTN)Address);
    } else if (OperationWidth == EfiPeiCpuIoWidthUint16) {
      if (Aligned) {
        *((UINT16 *)Uint8Buffer) = MmioRead16 ((UINTN)Address);
      } else {
        WriteUnaligned16 ((UINT16 *)Uint8Buffer, MmioRead16 ((UINTN)Address));
      }
    } else if (OperationWidth == EfiPeiCpuIoWidthUint32) {
      if (Aligned) {
        *((UINT32 *)Uint8Buffer) = MmioRead32 ((UINTN)Address);
      } else {
        WriteUnaligned32 ((UINT32 *)Uint8Buffer, MmioRead32 ((UINTN)Address));
      }
    } else if (OperationWidth == EfiPeiCpuIoWidthUint64) {
      if (Aligned) {
        *((UINT64 *)Uint8Buffer) = MmioRead64 ((UINTN)Address);
      } else {
        WriteUnaligned64 ((UINT64 *)Uint8Buffer, MmioRead64 ((UINTN)Address));
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Writes memory-mapped registers.

  @param[in]  PeiServices  An indirect pointer to the PEI Services Table
                           published by the PEI Foundation.
  @param[in]  This         Pointer to local data for the interface.
  @param[in]  Width        The width of the access. Enumerated in bytes.
  @param[in]  Address      The physical address of the access.
  @param[in]  Count        The number of accesses to perform.
  @param[in]  Buffer       A pointer to the buffer of data.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this EFI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceWrite (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN UINT64                    Address,
  IN UINTN                     Count,
  IN VOID                      *Buffer
  )
{
  EFI_STATUS                Status;
  UINT8                     InStride;
  UINT8                     OutStride;
  EFI_PEI_CPU_IO_PPI_WIDTH  OperationWidth;
  BOOLEAN                   Aligned;
  UINT8                     *Uint8Buffer;

  Status = CpuIoCheckParameter (TRUE, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select loop based on the width of the transfer
  //
  InStride       = mInStride[Width];
  OutStride      = mOutStride[Width];
  OperationWidth = (EFI_PEI_CPU_IO_PPI_WIDTH)(Width & 0x03);
  Aligned        = (BOOLEAN)(((UINTN)Buffer & (mInStride[OperationWidth] - 1)) == 0x00);
  for (Uint8Buffer = Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (OperationWidth == EfiPeiCpuIoWidthUint8) {
      MmioWrite8 ((UINTN)Address, *Uint8Buffer);
    } else if (OperationWidth == EfiPeiCpuIoWidthUint16) {
      if (Aligned) {
        MmioWrite16 ((UINTN)Address, *((UINT16 *)Uint8Buffer));
      } else {
        MmioWrite16 ((UINTN)Address, ReadUnaligned16 ((UINT16 *)Uint8Buffer));
      }
    } else if (OperationWidth == EfiPeiCpuIoWidthUint32) {
      if (Aligned) {
        MmioWrite32 ((UINTN)Address, *((UINT32 *)Uint8Buffer));
      } else {
        MmioWrite32 ((UINTN)Address, ReadUnaligned32 ((UINT32 *)Uint8Buffer));
      }
    } else if (OperationWidth == EfiPeiCpuIoWidthUint64) {
      if (Aligned) {
        MmioWrite64 ((UINTN)Address, *((UINT64 *)Uint8Buffer));
      } else {
        MmioWrite64 ((UINTN)Address, ReadUnaligned64 ((UINT64 *)Uint8Buffer));
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Reads I/O registers.

  @param[in]  PeiServices  An indirect pointer to the PEI Services Table
                           published by the PEI Foundation.
  @param[in]  This         Pointer to local data for the interface.
  @param[in]  Width        The width of the access. Enumerated in bytes.
  @param[in]  Address      The physical address of the access.
  @param[in]  Count        The number of accesses to perform.
  @param[out] Buffer       A pointer to the buffer of data.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this EFI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuIoServiceRead (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI  *This,
  IN  EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN  UINT64                    Address,
  IN  UINTN                     Count,
  OUT VOID                      *Buffer
  )
{
  EFI_STATUS                Status;
  UINT8                     InStride;
  UINT8                     OutStride;
  EFI_PEI_CPU_IO_PPI_WIDTH  OperationWidth;
  BOOLEAN                   Aligned;
  UINT8                     *Uint8Buffer;

  Status = CpuIoCheckParameter (FALSE, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select loop based on the width of the transfer
  //
  InStride       = mInStride[Width];
  OutStride      = mOutStride[Width];
  OperationWidth = (EFI_PEI_CPU_IO_PPI_WIDTH)(Width & 0x03);

  //
  // Fifo operations supported for (mInStride[Width] == 0)
  //
  if (InStride == 0) {
    switch (OperationWidth) {
      case EfiPeiCpuIoWidthUint8:
        IoReadFifo8 ((UINTN)Address, Count, Buffer);
        return EFI_SUCCESS;
      case EfiPeiCpuIoWidthUint16:
        IoReadFifo16 ((UINTN)Address, Count, Buffer);
        return EFI_SUCCESS;
      case EfiPeiCpuIoWidthUint32:
        IoReadFifo32 ((UINTN)Address, Count, Buffer);
        return EFI_SUCCESS;
      default:
        //
        // The CpuIoCheckParameter call above will ensure that this
        // path is not taken.
        //
        ASSERT (FALSE);
        break;
    }
  }

  Aligned = (BOOLEAN)(((UINTN)Buffer & (mInStride[OperationWidth] - 1)) == 0x00);
  for (Uint8Buffer = Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (OperationWidth == EfiPeiCpuIoWidthUint8) {
      *Uint8Buffer = IoRead8 ((UINTN)Address);
    } else if (OperationWidth == EfiPeiCpuIoWidthUint16) {
      if (Aligned) {
        *((UINT16 *)Uint8Buffer) = IoRead16 ((UINTN)Address);
      } else {
        WriteUnaligned16 ((UINT16 *)Uint8Buffer, IoRead16 ((UINTN)Address));
      }
    } else if (OperationWidth == EfiPeiCpuIoWidthUint32) {
      if (Aligned) {
        *((UINT32 *)Uint8Buffer) = IoRead32 ((UINTN)Address);
      } else {
        WriteUnaligned32 ((UINT32 *)Uint8Buffer, IoRead32 ((UINTN)Address));
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Write I/O registers.

  @param[in]  PeiServices  An indirect pointer to the PEI Services Table
                           published by the PEI Foundation.
  @param[in]  This         Pointer to local data for the interface.
  @param[in]  Width        The width of the access. Enumerated in bytes.
  @param[in]  Address      The physical address of the access.
  @param[in]  Count        The number of accesses to perform.
  @param[in]  Buffer       A pointer to the buffer of data.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this EFI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this EFI system.

**/
EFI_STATUS
EFIAPI
CpuIoServiceWrite (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN EFI_PEI_CPU_IO_PPI_WIDTH  Width,
  IN UINT64                    Address,
  IN UINTN                     Count,
  IN VOID                      *Buffer
  )
{
  EFI_STATUS                Status;
  UINT8                     InStride;
  UINT8                     OutStride;
  EFI_PEI_CPU_IO_PPI_WIDTH  OperationWidth;
  BOOLEAN                   Aligned;
  UINT8                     *Uint8Buffer;

  //
  // Make sure the parameters are valid
  //
  Status = CpuIoCheckParameter (FALSE, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select loop based on the width of the transfer
  //
  InStride       = mInStride[Width];
  OutStride      = mOutStride[Width];
  OperationWidth = (EFI_PEI_CPU_IO_PPI_WIDTH)(Width & 0x03);

  //
  // Fifo operations supported for (mInStride[Width] == 0)
  //
  if (InStride == 0) {
    switch (OperationWidth) {
      case EfiPeiCpuIoWidthUint8:
        IoWriteFifo8 ((UINTN)Address, Count, Buffer);
        return EFI_SUCCESS;
      case EfiPeiCpuIoWidthUint16:
        IoWriteFifo16 ((UINTN)Address, Count, Buffer);
        return EFI_SUCCESS;
      case EfiPeiCpuIoWidthUint32:
        IoWriteFifo32 ((UINTN)Address, Count, Buffer);
        return EFI_SUCCESS;
      default:
        //
        // The CpuIoCheckParameter call above will ensure that this
        // path is not taken.
        //
        ASSERT (FALSE);
        break;
    }
  }

  Aligned = (BOOLEAN)(((UINTN)Buffer & (mInStride[OperationWidth] - 1)) == 0x00);
  for (Uint8Buffer = (UINT8 *)Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (OperationWidth == EfiPeiCpuIoWidthUint8) {
      IoWrite8 ((UINTN)Address, *Uint8Buffer);
    } else if (OperationWidth == EfiPeiCpuIoWidthUint16) {
      if (Aligned) {
        IoWrite16 ((UINTN)Address, *((UINT16 *)Uint8Buffer));
      } else {
        IoWrite16 ((UINTN)Address, ReadUnaligned16 ((UINT16 *)Uint8Buffer));
      }
    } else if (OperationWidth == EfiPeiCpuIoWidthUint32) {
      if (Aligned) {
        IoWrite32 ((UINTN)Address, *((UINT32 *)Uint8Buffer));
      } else {
        IoWrite32 ((UINTN)Address, ReadUnaligned32 ((UINT32 *)Uint8Buffer));
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  8-bit I/O read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  An 8-bit value returned from the I/O space.
**/
UINT8
EFIAPI
CpuIoRead8 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address
  )
{
  return IoRead8 ((UINTN)Address);
}

/**
  16-bit I/O read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 16-bit value returned from the I/O space.

**/
UINT16
EFIAPI
CpuIoRead16 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address
  )
{
  return IoRead16 ((UINTN)Address);
}

/**
  32-bit I/O read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 32-bit value returned from the I/O space.

**/
UINT32
EFIAPI
CpuIoRead32 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address
  )
{
  return IoRead32 ((UINTN)Address);
}

/**
  64-bit I/O read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 64-bit value returned from the I/O space.

**/
UINT64
EFIAPI
CpuIoRead64 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address
  )
{
  return IoRead64 ((UINTN)Address);
}

/**
  8-bit I/O write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuIoWrite8 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address,
  IN UINT8                     Data
  )
{
  IoWrite8 ((UINTN)Address, Data);
}

/**
  16-bit I/O write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuIoWrite16 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address,
  IN UINT16                    Data
  )
{
  IoWrite16 ((UINTN)Address, Data);
}

/**
  32-bit I/O write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuIoWrite32 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address,
  IN UINT32                    Data
  )
{
  IoWrite32 ((UINTN)Address, Data);
}

/**
  64-bit I/O write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuIoWrite64 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address,
  IN UINT64                    Data
  )
{
  IoWrite64 ((UINTN)Address, Data);
}

/**
  8-bit memory read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  An 8-bit value returned from the memory space.

**/
UINT8
EFIAPI
CpuMemRead8 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address
  )
{
  return MmioRead8 ((UINTN)Address);
}

/**
  16-bit memory read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 16-bit value returned from the memory space.

**/
UINT16
EFIAPI
CpuMemRead16 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address
  )
{
  return MmioRead16 ((UINTN)Address);
}

/**
  32-bit memory read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 32-bit value returned from the memory space.

**/
UINT32
EFIAPI
CpuMemRead32 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address
  )
{
  return MmioRead32 ((UINTN)Address);
}

/**
  64-bit memory read operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.

  @return  A 64-bit value returned from the memory space.

**/
UINT64
EFIAPI
CpuMemRead64 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address
  )
{
  return MmioRead64 ((UINTN)Address);
}

/**
  8-bit memory write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuMemWrite8 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address,
  IN  UINT8                    Data
  )
{
  MmioWrite8 ((UINTN)Address, Data);
}

/**
  16-bit memory write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuMemWrite16 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address,
  IN UINT16                    Data
  )
{
  MmioWrite16 ((UINTN)Address, Data);
}

/**
  32-bit memory write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuMemWrite32 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address,
  IN UINT32                    Data
  )
{
  MmioWrite32 ((UINTN)Address, Data);
}

/**
  64-bit memory write operations.

  @param[in] PeiServices  An indirect pointer to the PEI Services Table published
                          by the PEI Foundation.
  @param[in] This         Pointer to local data for the interface.
  @param[in] Address      The physical address of the access.
  @param[in] Data         The data to write.

**/
VOID
EFIAPI
CpuMemWrite64 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN CONST EFI_PEI_CPU_IO_PPI  *This,
  IN UINT64                    Address,
  IN UINT64                    Data
  )
{
  MmioWrite64 ((UINTN)Address, Data);
}

/**
  The Entry point of the CPU I/O PEIM

  This function is the Entry point of the CPU I/O PEIM which installs CpuIoPpi.

  @param[in]  FileHandle   Pointer to image file handle.
  @param[in]  PeiServices  Pointer to PEI Services Table

  @retval EFI_SUCCESS  CPU I/O PPI successfully installed

**/
EFI_STATUS
EFIAPI
CpuIoInitialize (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;

  //
  // Register so it will be automatically shadowed to memory
  //
  Status = PeiServicesRegisterForShadow (FileHandle);

  //
  // Make CpuIo pointer in PeiService table point to gCpuIoPpi
  //
  (*((EFI_PEI_SERVICES **)PeiServices))->CpuIo = &gCpuIoPpi;

  if (Status == EFI_ALREADY_STARTED) {
    //
    // Shadow completed and running from memory
    //
    DEBUG ((DEBUG_INFO, "CpuIO PPI has been loaded into memory.  Reinstalled PPI=0x%x\n", &gCpuIoPpi));
  } else {
    Status = PeiServicesInstallPpi (&gPpiList);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
