/** @file
SMM MP perf-logging implementation

Copyright (c) 2023 - 2024, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"

#define  SMM_MP_PERF_PROCEDURE_NAME(procedure)  # procedure
GLOBAL_REMOVE_IF_UNREFERENCED
CHAR8  *gSmmMpPerfProcedureName[] = {
  SMM_MP_PERF_PROCEDURE_LIST (SMM_MP_PERF_PROCEDURE_NAME)
};
//
// Each element holds the performance data for one processor.
//
GLOBAL_REMOVE_IF_UNREFERENCED
SMM_PERF_AP_PROCEDURE_PERFORMANCE  *mSmmMpProcedurePerformance = NULL;

/**
  Initialize the perf-logging feature for APs.

  @param NumberofCpus    Number of processors in the platform.
**/
VOID
InitializeMpPerf (
  UINTN  NumberofCpus
  )
{
  mSmmMpProcedurePerformance = AllocateZeroPool (NumberofCpus * sizeof (*mSmmMpProcedurePerformance));
  ASSERT (mSmmMpProcedurePerformance != NULL);
}

/**
  Migrate MP performance data to standardized performance database.

  @param NumberofCpus    Number of processors in the platform.
  @param BspIndex        The index of the BSP.
**/
VOID
MigrateMpPerf (
  UINTN  NumberofCpus,
  UINTN  BspIndex
  )
{
  UINTN  CpuIndex;
  UINTN  MpProcecureId;

  for (CpuIndex = 0; CpuIndex < NumberofCpus; CpuIndex++) {
    if ((CpuIndex != BspIndex) && !FeaturePcdGet (PcdSmmApPerfLogEnable)) {
      //
      // Skip migrating AP performance data if AP perf-logging is disabled.
      //
      continue;
    }

    for (MpProcecureId = 0; MpProcecureId < SMM_MP_PERF_PROCEDURE_ID (SmmMpProcedureMax); MpProcecureId++) {
      if (mSmmMpProcedurePerformance[CpuIndex].Begin[MpProcecureId] != 0) {
        PERF_START (NULL, gSmmMpPerfProcedureName[MpProcecureId], NULL, mSmmMpProcedurePerformance[CpuIndex].Begin[MpProcecureId]);
        PERF_END (NULL, gSmmMpPerfProcedureName[MpProcecureId], NULL, mSmmMpProcedurePerformance[CpuIndex].End[MpProcecureId]);
      }
    }
  }

  ZeroMem (mSmmMpProcedurePerformance, NumberofCpus * sizeof (*mSmmMpProcedurePerformance));
}

/**
  Save the performance counter value before running the MP procedure.

  @param CpuIndex        The index of the CPU.
  @param MpProcedureId   The ID of the MP procedure.
**/
VOID
MpPerfBegin (
  IN UINTN  CpuIndex,
  IN UINTN  MpProcedureId
  )
{
  mSmmMpProcedurePerformance[CpuIndex].Begin[MpProcedureId] = GetPerformanceCounter ();
}

/**
  Save the performance counter value after running the MP procedure.

  @param CpuIndex        The index of the CPU.
  @param MpProcedureId   The ID of the MP procedure.
**/
VOID
MpPerfEnd (
  IN UINTN  CpuIndex,
  IN UINTN  MpProcedureId
  )
{
  mSmmMpProcedurePerformance[CpuIndex].End[MpProcedureId] = GetPerformanceCounter ();
}
