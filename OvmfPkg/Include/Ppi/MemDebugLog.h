/** @file
  Memory Debug Log PPI

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MEM_DEBUG_LOG_PPI_H_
#define __MEM_DEBUG_LOG_PPI_H_

#define MEM_DEBUG_LOG_PPI_SIGNATURE  SIGNATURE_32('M','D','B','L')

#define MEM_DEBUG_LOG_PPI_VERSION  (1)

typedef
EFI_STATUS
(EFIAPI *MEM_DEBUG_LOG_PPI_WRITE)(
  IN  CHAR8               *Buffer,
  IN  UINTN                Length
  );

typedef struct {
  UINT32                     Signature;
  UINT32                     Version;
  MEM_DEBUG_LOG_PPI_WRITE    MemDebugLogPpiWrite;
} MEM_DEBUG_LOG_PPI;

extern  EFI_GUID  gMemDebugLogPpiGuid;

#endif // __MEM_DEBUG_LOG_PPI_H_
