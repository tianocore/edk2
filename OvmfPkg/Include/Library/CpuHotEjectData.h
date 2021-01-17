/** @file
  Definition for a structure sharing state for CPU hot-eject.

  Copyright (C) 2021, Oracle Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _CPU_HOT_EJECT_DATA_H_
#define _CPU_HOT_EJECT_DATA_H_

typedef
VOID
(EFIAPI *CPU_HOT_EJECT_FN)(
  IN UINTN  ProcessorNum
  );

#define CPU_EJECT_INVALID               (MAX_UINT64)
#define CPU_EJECT_WORKER                (MAX_UINT64-1)

#define  CPU_HOT_EJECT_DATA_REVISION_1  0x00000001

typedef struct {
  UINT32           Revision;          // Used for version identification for this structure
  UINT32           ArrayLength;       // Number of entries for the ApicIdMap array

  UINT64           *ApicIdMap;        // Pointer to CpuIndex->ApicId map for pending ejects
  CPU_HOT_EJECT_FN Handler;           // Handler to do the CPU ejection
  UINT64           Reserved;
} CPU_HOT_EJECT_DATA;

#endif /* _CPU_HOT_EJECT_DATA_H_ */
