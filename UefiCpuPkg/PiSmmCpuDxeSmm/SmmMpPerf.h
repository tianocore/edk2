/** @file
SMM MP perf-logging implementation

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MP_PERF_H_
#define MP_PERF_H_

//
// The list of all MP procedures that need to be perf-logged.
//
#define  SMM_MP_PERF_PROCEDURE_LIST(_) \
  _(InitializeSmm), \
  _(SmmRendezvousEntry), \
  _(PlatformValidSmi), \
  _(SmmRendezvousExit), \
  _(SmmMpProcedureMax) // Add new entries above this line

//
// To perf-log MP procedures, call MpPerfBegin()/MpPerfEnd() with CpuIndex
// and SMM_MP_PERF_PROCEDURE_ID with entry name defined in the SMM_MP_PERF_PROCEDURE_LIST.
//
#define  SMM_MP_PERF_PROCEDURE_ID(procedure)  SmmMpProcedureId ## procedure
enum {
  SMM_MP_PERF_PROCEDURE_LIST (SMM_MP_PERF_PROCEDURE_ID)
};

typedef struct {
  UINT64    Begin[SMM_MP_PERF_PROCEDURE_ID (SmmMpProcedureMax)];
  UINT64    End[SMM_MP_PERF_PROCEDURE_ID (SmmMpProcedureMax)];
} SMM_PERF_AP_PROCEDURE_PERFORMANCE;

/**
  Initialize the perf-logging feature for APs.

  @param NumberofCpus    Number of processors in the platform.
**/
VOID
InitializeMpPerf (
  UINTN  NumberofCpus
  );

/**
  Migrate MP performance data to standardized performance database.

  @param NumberofCpus    Number of processors in the platform.
  @param BspIndex        The index of the BSP.
**/
VOID
MigrateMpPerf (
  UINTN  NumberofCpus,
  UINTN  BspIndex
  );

/**
  Save the performance counter value before running the MP procedure.

  @param CpuIndex        The index of the CPU.
  @param MpProcedureId   The ID of the MP procedure.
**/
VOID
MpPerfBegin (
  IN UINTN  CpuIndex,
  IN UINTN  MpProcedureId
  );

/**
  Save the performance counter value after running the MP procedure.

  @param CpuIndex        The index of the CPU.
  @param MpProcedureId   The ID of the MP procedure.
**/
VOID
MpPerfEnd (
  IN UINTN  CpuIndex,
  IN UINTN  MpProcedureId
  );

#endif
