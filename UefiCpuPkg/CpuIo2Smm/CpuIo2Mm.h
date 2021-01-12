/** @file
  Internal include file for the SMM CPU I/O Protocol.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CPU_IO2_SMM_H_
#define _CPU_IO2_SMM_H_

#include <PiSmm.h>

#include <Protocol/SmmCpuIo2.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#define MAX_IO_PORT_ADDRESS   0xFFFF

/**
  Reads memory-mapped registers.

  The I/O operations are carried out exactly as requested.  The caller is
  responsible for any alignment and I/O width issues that the bus, device,
  platform, or type of I/O might require.

  @param[in]  This     The EFI_SMM_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O operations.
  @param[in]  Address  The base address of the I/O operations.  The caller is
                       responsible for aligning the Address if required.
  @param[in]  Count    The number of I/O operations to perform.
  @param[out] Buffer   For read operations, the destination buffer to store
                       the results.  For write operations, the source buffer
                       from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the device.
  @retval EFI_UNSUPPORTED        The Address is not valid for this system.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a
                                 lack of resources

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceRead (
  IN  CONST EFI_SMM_CPU_IO2_PROTOCOL  *This,
  IN  EFI_SMM_IO_WIDTH                Width,
  IN  UINT64                          Address,
  IN  UINTN                           Count,
  OUT VOID                            *Buffer
  );

/**
  Writes memory-mapped registers.

  The I/O operations are carried out exactly as requested.  The caller is
  responsible for any alignment and I/O width issues that the bus, device,
  platform, or type of I/O might require.

  @param[in]  This     The EFI_SMM_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O operations.
  @param[in]  Address  The base address of the I/O operations.  The caller is
                       responsible for aligning the Address if required.
  @param[in]  Count    The number of I/O operations to perform.
  @param[in]  Buffer   For read operations, the destination buffer to store
                       the results.  For write operations, the source buffer
                       from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the device.
  @retval EFI_UNSUPPORTED        The Address is not valid for this system.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a
                                 lack of resources

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceWrite (
  IN CONST EFI_SMM_CPU_IO2_PROTOCOL  *This,
  IN EFI_SMM_IO_WIDTH                Width,
  IN UINT64                          Address,
  IN UINTN                           Count,
  IN VOID                            *Buffer
  );

/**
  Reads I/O registers.

  The I/O operations are carried out exactly as requested.  The caller is
  responsible for any alignment and I/O width issues that the bus, device,
  platform, or type of I/O might require.

  @param[in]  This     The EFI_SMM_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O operations.
  @param[in]  Address  The base address of the I/O operations.  The caller is
                       responsible for aligning the Address if required.
  @param[in]  Count    The number of I/O operations to perform.
  @param[out] Buffer   For read operations, the destination buffer to store
                       the results.  For write operations, the source buffer
                       from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the device.
  @retval EFI_UNSUPPORTED        The Address is not valid for this system.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a
                                 lack of resources

**/
EFI_STATUS
EFIAPI
CpuIoServiceRead (
  IN  CONST EFI_SMM_CPU_IO2_PROTOCOL  *This,
  IN  EFI_SMM_IO_WIDTH                Width,
  IN  UINT64                          Address,
  IN  UINTN                           Count,
  OUT VOID                            *Buffer
  );

/**
  Write I/O registers.

  The I/O operations are carried out exactly as requested.  The caller is
  responsible for any alignment and I/O width issues that the bus, device,
  platform, or type of I/O might require.

  @param[in]  This     The EFI_SMM_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O operations.
  @param[in]  Address  The base address of the I/O operations.  The caller is
                       responsible for aligning the Address if required.
  @param[in]  Count    The number of I/O operations to perform.
  @param[in]  Buffer   For read operations, the destination buffer to store
                       the results.  For write operations, the source buffer
                       from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the device.
  @retval EFI_UNSUPPORTED        The Address is not valid for this system.
  @retval EFI_INVALID_PARAMETER  Width or Count, or both, were invalid.
  @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a
                                 lack of resources

**/
EFI_STATUS
EFIAPI
CpuIoServiceWrite (
  IN CONST EFI_SMM_CPU_IO2_PROTOCOL  *This,
  IN EFI_SMM_IO_WIDTH                Width,
  IN UINT64                          Address,
  IN UINTN                           Count,
  IN VOID                            *Buffer
  );

/**
  The module Entry Point SmmCpuIoProtocol driver

  @retval EFI_SUCCESS  The entry point is executed successfully.
  @retval Other        Some error occurs when executing this entry point.

**/
EFI_STATUS
CommonCpuIo2Initialize (
  VOID
  );

#endif
