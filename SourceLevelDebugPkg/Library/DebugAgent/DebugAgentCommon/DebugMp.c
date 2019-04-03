/** @file
  Multi-Processor support functions implementation.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DebugAgent.h"

GLOBAL_REMOVE_IF_UNREFERENCED DEBUG_MP_CONTEXT volatile  mDebugMpContext = {0,0,0,{0},{0},0,0,0,0,FALSE,FALSE};

GLOBAL_REMOVE_IF_UNREFERENCED DEBUG_CPU_DATA volatile  mDebugCpuData = {0};

/**
  Acquire a spin lock when Multi-processor supported.

  It will block in the function if cannot get the access control.
  If Multi-processor is not supported, return directly.

  @param[in, out] MpSpinLock      A pointer to the spin lock.

**/
VOID
AcquireMpSpinLock (
  IN OUT SPIN_LOCK           *MpSpinLock
  )
{
  if (!MultiProcessorDebugSupport()) {
    return;
  }

  while (TRUE) {
    if (AcquireSpinLockOrFail (MpSpinLock)) {
      break;
    }
    CpuPause ();
    continue;
  }
}

/**
  Release a spin lock when Multi-processor supported.

  @param[in, out] MpSpinLock      A pointer to the spin lock.

**/
VOID
ReleaseMpSpinLock (
  IN OUT SPIN_LOCK           *MpSpinLock
  )
{
  if (!MultiProcessorDebugSupport()) {
    return;
  }

  ReleaseSpinLock (MpSpinLock);
}

/**
  Break the other processor by send IPI.

  @param[in] CurrentProcessorIndex  Current processor index value.

**/
VOID
HaltOtherProcessors (
  IN UINT32             CurrentProcessorIndex
  )
{
  DebugAgentMsgPrint (DEBUG_AGENT_INFO, "processor[%x]:Try to halt other processors.\n", CurrentProcessorIndex);
  if (!DebugAgentIsBsp (CurrentProcessorIndex)) {
    SetIpiSentByApFlag (TRUE);;
  }

  mDebugMpContext.BreakAtCpuIndex = CurrentProcessorIndex;

  //
  // Set the debug viewpoint to the current breaking CPU.
  //
  SetDebugViewPoint (CurrentProcessorIndex);

  //
  // Send fixed IPI to other processors.
  //
  SendFixedIpiAllExcludingSelf (DEBUG_TIMER_VECTOR);

}

/**
  Get the current processor's index.

  @return Processor index value.

**/
UINT32
GetProcessorIndex (
  VOID
  )
{
  UINT32                Index;
  UINT16                LocalApicID;

  LocalApicID = (UINT16) GetApicId ();

  AcquireMpSpinLock (&mDebugMpContext.MpContextSpinLock);

  for (Index = 0; Index < mDebugCpuData.CpuCount; Index ++) {
    if (mDebugCpuData.ApicID[Index] == LocalApicID) {
      break;
    }
  }

  if (Index == mDebugCpuData.CpuCount) {
    mDebugCpuData.ApicID[Index] = LocalApicID;
    mDebugCpuData.CpuCount ++ ;
  }

  ReleaseMpSpinLock (&mDebugMpContext.MpContextSpinLock);

  return Index;
}

/**
  Check if the specified processor is BSP or not.

  @param[in] ProcessorIndex Processor index value.

  @retval TRUE    It is BSP.
  @retval FALSE   It isn't BSP.

**/
BOOLEAN
DebugAgentIsBsp (
  IN UINT32  ProcessorIndex
  )
{
  MSR_IA32_APIC_BASE_REGISTER  MsrApicBase;

  //
  // If there are less than 2 CPUs detected, then the currently executing CPU
  // must be the BSP.  This avoids an access to an MSR that may not be supported
  // on single core CPUs.
  //
  if (mDebugCpuData.CpuCount < 2) {
    return TRUE;
  }

  MsrApicBase.Uint64 = AsmReadMsr64 (MSR_IA32_APIC_BASE);
  if (MsrApicBase.Bits.BSP == 1) {
    if (mDebugMpContext.BspIndex != ProcessorIndex) {
      AcquireMpSpinLock (&mDebugMpContext.MpContextSpinLock);
      mDebugMpContext.BspIndex = ProcessorIndex;
      ReleaseMpSpinLock (&mDebugMpContext.MpContextSpinLock);
    }
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Set processor stop flag bitmask in MP context.

  @param[in] ProcessorIndex Processor index value.
  @param[in] StopFlag       TRUE means set stop flag.
                            FALSE means clean break flag.

**/
VOID
SetCpuStopFlagByIndex (
  IN UINT32             ProcessorIndex,
  IN BOOLEAN            StopFlag
  )
{
  UINT8                 Value;
  UINTN                 Index;

  AcquireMpSpinLock (&mDebugMpContext.MpContextSpinLock);

  Value = mDebugMpContext.CpuStopStatusMask[ProcessorIndex / 8];
  Index = ProcessorIndex % 8;
  if (StopFlag) {
    Value = BitFieldWrite8 (Value, Index, Index, 1);
  } else {
    Value = BitFieldWrite8 (Value, Index, Index, 0);
  }
  mDebugMpContext.CpuStopStatusMask[ProcessorIndex / 8] = Value;

  ReleaseMpSpinLock (&mDebugMpContext.MpContextSpinLock);
}

/**
  Set processor break flag bitmask in MP context.

  @param[in] ProcessorIndex Processor index value.
  @param[in] BreakFlag      TRUE means set break flag.
                            FALSE means clean break flag.

**/
VOID
SetCpuBreakFlagByIndex (
  IN UINT32             ProcessorIndex,
  IN BOOLEAN            BreakFlag
  )
{
  UINT8                 Value;
  UINTN                 Index;

  AcquireMpSpinLock (&mDebugMpContext.MpContextSpinLock);

  Value = mDebugMpContext.CpuBreakMask[ProcessorIndex / 8];
  Index = ProcessorIndex % 8;
  if (BreakFlag) {
    Value = BitFieldWrite8 (Value, Index, Index, 1);
  } else {
    Value = BitFieldWrite8 (Value, Index, Index, 0);
  }
  mDebugMpContext.CpuBreakMask[ProcessorIndex / 8] = Value;

  ReleaseMpSpinLock (&mDebugMpContext.MpContextSpinLock);
}

/**
  Check if processor is stopped already.

  @param[in] ProcessorIndex   Processor index value.

  @retval TRUE        Processor is stopped already.
  @retval TRUE        Processor isn't stopped.

**/
BOOLEAN
IsCpuStopped (
  IN UINT32              ProcessorIndex
  )
{
  UINT8                 CpuMask;

  CpuMask = (UINT8) (1 << (ProcessorIndex % 8));

  if ((mDebugMpContext.CpuStopStatusMask[ProcessorIndex / 8] & CpuMask) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Set the run command flag.

  @param[in] RunningFlag   TRUE means run command flag is set.
                           FALSE means run command flag is cleared.

**/
VOID
SetCpuRunningFlag (
  IN BOOLEAN            RunningFlag
  )
{
  AcquireMpSpinLock (&mDebugMpContext.MpContextSpinLock);
  mDebugMpContext.RunCommandSet = RunningFlag;
  ReleaseMpSpinLock (&mDebugMpContext.MpContextSpinLock);
}

/**
  Set the current view point to be debugged.

  @param[in] ProcessorIndex   Processor index value.

**/
VOID
SetDebugViewPoint (
  IN UINT32             ProcessorIndex
  )
{
  AcquireMpSpinLock (&mDebugMpContext.MpContextSpinLock);
  mDebugMpContext.ViewPointIndex = ProcessorIndex;
  ReleaseMpSpinLock (&mDebugMpContext.MpContextSpinLock);
}

/**
  Set the IPI send by BPS/AP flag.

  @param[in] IpiSentByApFlag   TRUE means this IPI is sent by AP.
                               FALSE means this IPI is sent by BSP.

**/
VOID
SetIpiSentByApFlag (
  IN BOOLEAN            IpiSentByApFlag
  )
{
  AcquireMpSpinLock (&mDebugMpContext.MpContextSpinLock);
  mDebugMpContext.IpiSentByAp = IpiSentByApFlag;
  ReleaseMpSpinLock (&mDebugMpContext.MpContextSpinLock);
}

/**
  Check the next pending breaking CPU.

  @retval others      There is at least one processor broken, the minimum
                      index number of Processor returned.
  @retval -1          No any processor broken.

**/
UINT32
FindNextPendingBreakCpu (
  VOID
  )
{
  UINT32               Index;

  for (Index = 0; Index < DEBUG_CPU_MAX_COUNT / 8; Index ++) {
    if (mDebugMpContext.CpuBreakMask[Index] != 0) {
      return  (UINT32) LowBitSet32 (mDebugMpContext.CpuBreakMask[Index]) + Index * 8;
    }
  }
  return (UINT32)-1;
}

/**
  Check if all processors are in running status.

  @retval TRUE        All processors run.
  @retval FALSE       At least one processor does not run.

**/
BOOLEAN
IsAllCpuRunning (
  VOID
  )
{
  UINTN              Index;

  for (Index = 0; Index < DEBUG_CPU_MAX_COUNT / 8; Index ++) {
    if (mDebugMpContext.CpuStopStatusMask[Index] != 0) {
      return FALSE;
    }
  }
  return TRUE;
}

/**
  Check if the current processor is the first breaking processor.

  If yes, halt other processors.

  @param[in] ProcessorIndex   Processor index value.

  @return TRUE       This processor is the first breaking processor.
  @return FALSE      This processor is not the first breaking processor.

**/
BOOLEAN
IsFirstBreakProcessor (
  IN UINT32              ProcessorIndex
  )
{
  if (MultiProcessorDebugSupport()) {
    if (mDebugMpContext.BreakAtCpuIndex != (UINT32) -1) {
      //
      // The current processor is not the first breaking one.
      //
      SetCpuBreakFlagByIndex (ProcessorIndex, TRUE);
      return FALSE;
    } else {
      //
      // If no any processor breaks, try to halt other processors
      //
      HaltOtherProcessors (ProcessorIndex);
      return TRUE;
    }
  }
  return TRUE;
}

