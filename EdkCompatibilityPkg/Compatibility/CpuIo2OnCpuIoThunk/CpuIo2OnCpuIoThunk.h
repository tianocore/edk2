/** @file
  Internal include file for the CPU I/O 2 Protocol thunk driver.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __CPU_IO2_ON_CPU_IO_H__
#define __CPU_IO2_ON_CPU_IO_H__

#include <Protocol/CpuIo2.h>
#include <Protocol/CpuIo.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Enables a driver to read memory-mapped registers in the PI System memory space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the memory operation.
  @param[in]       Address      The base address of the memory operation.
  @param[in]       Count        The number of memory operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[in, out]      Buffer       The destination buffer to store the results.

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
  IN OUT VOID                              *Buffer
  );

/**
  Enables a driver to write memory-mapped registers in the PI System memory space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the memory operation.
  @param[in]       Address      The base address of the memory operation.
  @param[in]       Count        The number of memory operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[in, out]       Buffer       The source buffer from which to write data.

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
  IN OUT VOID                              *Buffer
  );

/**
  Enables a driver to read registers in the PI CPU I/O space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the I/O operation.
  @param[in]       Address      The base address of the I/O operation. The caller is responsible
                                for aligning the Address if required. 
  @param[in]       Count        The number of I/O operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[in, out]      Buffer       The destination buffer to store the results.

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
  IN     UINT64                            Address,
  IN     UINTN                             Count,
  IN OUT VOID                              *Buffer
  );

/**
  Enables a driver to write registers in the PI CPU I/O space.

  @param[in]       This         A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]       Width        Signifies the width of the I/O operation.
  @param[in]       Address      The base address of the I/O operation. The caller is responsible
                                for aligning the Address if required. 
  @param[in]       Count        The number of I/O operations to perform. The number of bytes moved
                                is Width size * Count, starting at Address.
  @param[in, out]       Buffer       The source buffer from which to write data.

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
  IN     UINT64                            Address,
  IN     UINTN                             Count,
  IN OUT VOID                              *Buffer
  );

#endif
