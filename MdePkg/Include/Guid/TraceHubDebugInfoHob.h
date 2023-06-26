/** @file
This header file declares Trace Hub related structure.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TRACE_HUB_DEBUG_INFO_HOB_H_
#define TRACE_HUB_DEBUG_INFO_HOB_H_

#define TRACEHUB_DEBUG_INFO_HOB_REVISION  1

typedef struct {
  UINT16     Revision;            // Structure revision
  BOOLEAN    Flag;                // Flag to enable or disable Trace Hub debug message.
  UINT8      DebugLevel;          // Debug level for Trace Hub.
  UINT8      Rvsd[4];             // Reserved for future use
  UINT64     TraceHubMmioAddress; // MMIO address where Trace Hub debug message output to.
} TRACEHUB_DEBUG_INFO_HOB;

extern GUID  gTraceHubDebugInfoHobGuid;

#endif // TRACE_HUB_DEBUG_INFO_HOB_H_
