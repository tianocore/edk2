/** @file
  Defines the HOB GUID, which is utilized for transferring essential
  information from the PEI to the DXE phase.

  Copyright (c) 2015 - 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MP_HANDOFF_H_
#define MP_HANDOFF_H_

#define MP_HANDOFF_GUID \
  { \
    0x11e2bd88, 0xed38, 0x4abd, {0xa3, 0x99, 0x21, 0xf2, 0x5f, 0xd0, 0x7a, 0x60 } \
  }

#define MP_HANDOFF_CONFIG_GUID \
  { \
    0xdabbd793, 0x7b46, 0x4144, {0x8a, 0xd4, 0x10, 0x1c, 0x7c, 0x08, 0xeb, 0xfa } \
  }

extern EFI_GUID  mMpHandOffGuid;
extern EFI_GUID  mMpHandOffConfigGuid;

//
// The information required to transfer from the PEI phase to the
// DXE phase is contained within the MP_HAND_OFF and PROCESSOR_HAND_OFF.
// If the SizeOfPointer (WaitLoopExecutionMode) of both phases are equal,
// and the APs is not in halt mode,
// then the APs can be awakened by triggering the start-up
// signal, rather than using INIT-SIPI-SIPI.
// To trigger the start-up signal, BSP writes the specified
// StartupSignalValue to the StartupSignalAddress of each processor.
// This address is monitored by the APs.
//
typedef struct {
  UINT32    ApicId;
  UINT32    Health;
  UINT64    StartupSignalAddress;
  UINT64    StartupProcedureAddress;
} PROCESSOR_HAND_OFF;

typedef struct {
  //
  // The ProcessorIndex indicates the range of processors. If it is set to 0, it signifies
  // processors from 0 to CpuCount - 1. Multiple instances in the HOB list describe
  // processors from ProcessorIndex to ProcessorIndex + CpuCount - 1.
  //
  UINT32                ProcessorIndex;
  UINT32                CpuCount;
  PROCESSOR_HAND_OFF    Info[];
} MP_HAND_OFF;

typedef struct {
  UINT32    WaitLoopExecutionMode;
  UINT32    StartupSignalValue;
} MP_HAND_OFF_CONFIG;
#endif
