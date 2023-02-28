/** @file

  Copyright (c) 2020 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MTRR_SUPPORT_H_
#define _MTRR_SUPPORT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <time.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/MtrrLib.h>
#include <Library/UnitTestHostBaseLib.h>

#include <Register/ArchitecturalMsr.h>
#include <Register/Cpuid.h>
#include <Register/Msr.h>

#define UNIT_TEST_APP_NAME     "MtrrLib Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

#define SCRATCH_BUFFER_SIZE  SIZE_16KB

typedef struct {
  UINT8                     PhysicalAddressBits;
  BOOLEAN                   MtrrSupported;
  BOOLEAN                   FixedMtrrSupported;
  MTRR_MEMORY_CACHE_TYPE    DefaultCacheType;
  UINT32                    VariableMtrrCount;
  UINT8                     MkTmeKeyidBits;
} MTRR_LIB_SYSTEM_PARAMETER;

extern UINT32   mFixedMtrrsIndex[];
extern BOOLEAN  mRandomInput;

/**
  Initialize the MTRR registers.

  @param SystemParameter System parameter that controls the MTRR registers initialization.
**/
UNIT_TEST_STATUS
EFIAPI
InitializeMtrrRegs (
  IN MTRR_LIB_SYSTEM_PARAMETER  *SystemParameter
  );

/**
  Initialize the MTRR registers.

  @param Context System parameter that controls the MTRR registers initialization.
**/
UNIT_TEST_STATUS
EFIAPI
InitializeSystem (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Return a random memory cache type.
**/
MTRR_MEMORY_CACHE_TYPE
GenerateRandomCacheType (
  VOID
  );

/**
  Generate random MTRRs.

  @param PhysicalAddressBits  Physical address bits.
  @param RawMemoryRanges      Return the randomly generated MTRRs.
  @param UcCount              Count of Uncacheable MTRRs.
  @param WtCount              Count of Write Through MTRRs.
  @param WbCount              Count of Write Back MTRRs.
  @param WpCount              Count of Write Protected MTRRs.
  @param WcCount              Count of Write Combining MTRRs.
**/
VOID
GenerateValidAndConfigurableMtrrPairs (
  IN     UINT32             PhysicalAddressBits,
  IN OUT MTRR_MEMORY_RANGE  *RawMemoryRanges,
  IN     UINT32             UcCount,
  IN     UINT32             WtCount,
  IN     UINT32             WbCount,
  IN     UINT32             WpCount,
  IN     UINT32             WcCount
  );

/**
  Convert the MTRR BASE/MASK array to memory ranges.

  @param DefaultType          Default memory type.
  @param PhysicalAddressBits  Physical address bits.
  @param RawMemoryRanges      Raw memory ranges.
  @param RawMemoryRangeCount  Count of raw memory ranges.
  @param MemoryRanges         Memory ranges.
  @param MemoryRangeCount     Count of memory ranges.
**/
VOID
GetEffectiveMemoryRanges (
  IN MTRR_MEMORY_CACHE_TYPE  DefaultType,
  IN UINT32                  PhysicalAddressBits,
  IN MTRR_MEMORY_RANGE       *RawMemoryRanges,
  IN UINT32                  RawMemoryRangeCount,
  OUT MTRR_MEMORY_RANGE      *MemoryRanges,
  OUT UINTN                  *MemoryRangeCount
  );

/**
  Generate random MTRR BASE/MASK for a specified type.

  @param PhysicalAddressBits Physical address bits.
  @param CacheType           Cache type.
  @param MtrrPair            Return the random MTRR.
  @param MtrrMemoryRange     Return the random memory range.
**/
VOID
GenerateRandomMtrrPair (
  IN  UINT32                  PhysicalAddressBits,
  IN  MTRR_MEMORY_CACHE_TYPE  CacheType,
  OUT MTRR_VARIABLE_SETTING   *MtrrPair        OPTIONAL,
  OUT MTRR_MEMORY_RANGE       *MtrrMemoryRange OPTIONAL
  );

/**
  Collect the test result.

  @param DefaultType          Default memory type.
  @param PhysicalAddressBits  Physical address bits.
  @param VariableMtrrCount    Count of variable MTRRs.
  @param Mtrrs                MTRR settings to collect from.
  @param Ranges               Return the memory ranges.
  @param RangeCount           Return the count of memory ranges.
  @param MtrrCount            Return the count of variable MTRRs being used.
**/
VOID
CollectTestResult (
  IN     MTRR_MEMORY_CACHE_TYPE  DefaultType,
  IN     UINT32                  PhysicalAddressBits,
  IN     UINT32                  VariableMtrrCount,
  IN     MTRR_SETTINGS           *Mtrrs,
  OUT    MTRR_MEMORY_RANGE       *Ranges,
  IN OUT UINTN                   *RangeCount,
  OUT    UINT32                  *MtrrCount
  );

/**
  Return a 64bit random number.

  @param Start  Start of the random number range.
  @param Limit  Limit of the random number range.
  @return 64bit random number
**/
UINT64
Random64 (
  UINT64  Start,
  UINT64  Limit
  );

/**
  Return a 32bit random number.

  @param Start  Start of the random number range.
  @param Limit  Limit of the random number range.
  @return 32bit random number
**/
UINT32
Random32 (
  UINT32  Start,
  UINT32  Limit
  );

/**
  Generate Count random numbers in FilePath.

  @param FilePath  The file path to put the generated random numbers.
  @param Count     Count of random numbers.
**/
VOID
GenerateRandomNumbers (
  CHAR8  *FilePath,
  UINTN  Count
  );

#endif
