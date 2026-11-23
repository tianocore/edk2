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
  Migrate MP performance data to the standardized performance database after AP
  dispatch has ended.

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
  UINTN  MpProcedureId;

  for (CpuIndex = 0; CpuIndex < NumberofCpus; CpuIndex++) {
    if ((CpuIndex != BspIndex) && !FeaturePcdGet (PcdSmmApPerfLogEnable)) {
      //
      // Skip migrating AP performance data if AP perf-logging is disabled.
      //
      continue;
    }

    for (MpProcedureId = 0;
         MpProcedureId < SMM_MP_PERF_PROCEDURE_ID (SmmMpProcedureMax);
         MpProcedureId++)
    {
      if (mSmmMpProcedurePerformance[CpuIndex].Begin[MpProcedureId] != 0) {
        if (MpProcedureId == SMM_MP_PERF_PROCEDURE_ID (SmmWaitForApArrival)) {
          if (LogPerformanceMeasurementEnabled (PERF_GENERAL_TYPE)) {
            PERF_START_EX (
              &gEfiCallerIdGuid,
              gSmmMpPerfProcedureName[MpProcedureId],
              NULL,
              mSmmMpProcedurePerformance[CpuIndex].Begin[MpProcedureId],
              PERF_FUNCTION_START_ID
              );
            PERF_END_EX (
              &gEfiCallerIdGuid,
              gSmmMpPerfProcedureName[MpProcedureId],
              NULL,
              mSmmMpProcedurePerformance[CpuIndex].End[MpProcedureId],
              PERF_FUNCTION_END_ID
              );
          }
        } else {
          PERF_START (
            NULL,
            gSmmMpPerfProcedureName[MpProcedureId],
            NULL,
            mSmmMpProcedurePerformance[CpuIndex].Begin[MpProcedureId]
            );
          PERF_END (
            NULL,
            gSmmMpPerfProcedureName[MpProcedureId],
            NULL,
            mSmmMpProcedurePerformance[CpuIndex].End[MpProcedureId]
            );
        }
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
