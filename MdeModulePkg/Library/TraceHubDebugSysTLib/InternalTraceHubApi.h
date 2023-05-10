/** @file
This header file declares functions that are not for common use.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef INTERNAL_TRACE_HUB_API_H_
#define INTERNAL_TRACE_HUB_API_H_

/**
  Count the total number of Trace Hub debug instance in the system.

  @retval UINT32      The total number of Trace Hub debug instance in the system.
**/
UINT32
EFIAPI
CountThDebugInstance (
  VOID
  );

/**
  Pack Trace Hub debug instances in the system.

  @param[in, out]  ThPtr     A pointer to TRACEHUB_DEBUG_INFO_HOB structure.
  @param[in]       Count     Number of Trace Hub HOBs.
**/
VOID
EFIAPI
PackThDebugInstance (
  IN OUT TRACEHUB_DEBUG_INFO_HOB  *ThPtr,
  IN     UINT32                   Count
  );

#endif // INTERNAL_TRACE_HUB_API_H_
