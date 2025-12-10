/** @file
  GUID and structure used for debug status code policy.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _STATUS_CODE_DATA_TYPE_EX_DEBUG_GUID_H_
#define _STATUS_CODE_DATA_TYPE_EX_DEBUG_GUID_H_

#include <Library/SemaphoreLib.h>

#define STATUS_CODE_DATA_TYPE_EX_DEBUG_GUID \
  { 0x7859daa2, 0x926e, 0x4b01,{0x85, 0x86, 0xc6, 0x2d, 0x45, 0x64, 0x21, 0xd2} }

#define MAX_EX_DEBUG_SIZE     0x200 // Inherited from PEI status code max
#define MAX_EX_DEBUG_STR_LEN  (MAX_EX_DEBUG_SIZE - sizeof(EX_DEBUG_INFO))

typedef
CHAR8 *
(EFIAPI *PROCESS_BUFFER)(
  IN     VOID   *ProcessDataPtr,
  IN     CHAR8  *Buffer,
  IN OUT UINTN  *BufferSize
  );

typedef
EFI_STATUS
(EFIAPI *PRINT_SYNC_ACQUIRE)(
  VOID
  );

typedef
EFI_STATUS
(EFIAPI *PRINT_SYNC_RELEASE)(
  VOID
  );

typedef struct {
  PROCESS_BUFFER        ProcessBuffer;    // Buffer processing function
  VOID                  *ProcessDataPtr;  // Data needed for processing
  PRINT_SYNC_ACQUIRE    PrintSyncAcquire; // Acquire sync function
  PRINT_SYNC_RELEASE    PrintSyncRelease; // Release sync function
  UINT32                DebugStringLen;
  CHAR8                 *DebugString; // Provided debug string
} EX_DEBUG_INFO;

extern EFI_GUID  gStatusCodeDataTypeExDebugGuid;

#endif // _STATUS_CODE_DATA_TYPE_EX_DEBUG_GUID_H_
