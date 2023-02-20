/** @file

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TRACE_HUB_DEBUG_LIB_H_
#define TRACE_HUB_DEBUG_LIB_H_

#include <Uefi.h>

typedef enum {
  SeverityNone    = 0,
  SeverityFatal   = 1,
  SeverityError   = 2,
  SeverityWarning = 3,
  SeverityNormal  = 4,
  SeverityUser1   = 5,
  SeverityUser2   = 6,
  SeverityUser3   = 7,
  SeverityMax
} TRACE_HUB_SEVERITY_TYPE;

/**
  Write debug string to specified Trace Hub MMIO address.

  @param[in]  SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]  Buffer           A pointer to the data buffer.
  @param[in]  NumberOfBytes    Number of bytes to be written.

  @retval EFI_SUCCESS      Data was written to Trace Hub.
  @retval Other            Failed to output Trace Hub message.
**/
EFI_STATUS
EFIAPI
TraceHubDebugWrite (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT8                    *Buffer,
  IN UINTN                    NumberOfBytes
  );

/**
  Write catalog status code message to specified Trace Hub MMIO address.

  @param[in]  SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]  Id               Catalog ID.
  @param[in]  Guid             Driver Guid.

  @retval EFI_SUCCESS      Data was written to Trace Hub.
  @retval Other            Failed to output Trace Hub message.
**/
EFI_STATUS
EFIAPI
TraceHubWriteCataLog64StatusCode (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT64                   Id,
  IN EFI_GUID                 *Guid
  );

/**
  Write catalog message to specified Trace Hub MMIO address.

  @param[in]  SeverityType   An error level to decide whether to enable Trace Hub data.
  @param[in]  Id             Catalog ID.
  @param[in]  NumberOfParams Number of parameters in argument list.
  @param[in]  ...            Argument list that pass to Trace Hub.

  @retval EFI_SUCCESS      Data was written to Trace Hub.
  @retval Other            Failed to output Trace Hub message.
**/
EFI_STATUS
EFIAPI
TraceHubWriteCataLog64 (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT64                   Id,
  IN UINTN                    NumberOfParams,
  ...
  );

#endif // TRACE_HUB_DEBUG_LIB_H_
