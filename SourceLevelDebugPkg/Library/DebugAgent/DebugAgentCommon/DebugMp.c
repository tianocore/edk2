/** @file
  Multi-Processor support functions implementation.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DebugAgent.h"

DEBUG_MP_CONTEXT volatile  mDebugMpContext = {0,0,0,0,0,0,0,0,FALSE,FALSE};

DEBUG_CPU_DATA volatile  mDebugCpuData = {0};

/**
  Acquire access control on debug port.

  It will block in the function if cannot get the access control.

**/
VOID
AcquireDebugPortControl (
  VOID
  )
{
  if (!MultiProcessorDebugSupport) {
    return;
  }

  while (TRUE) {
    if (AcquireSpinLockOrFail (&mDebugMpContext.DebugPortSpinLock)) {
      break;
    }
    CpuPause ();
    continue;
  }
}

/**
  Release access control on debug port.

**/
VOID
ReleaseDebugPortControl (
  VOID
  )
{
  if (!MultiProcessorDebugSupport) {
    return;
  }

  ReleaseSpinLock (&mDebugMpContext.DebugPortSpinLock);
}

/**
  Acquire access control on MP context.

  It will block in the function if cannot get the access control.

**/
VOID
AcquireMpContextControl (
  VOID
  )
{
  while (TRUE) {
    if (AcquireSpinLockOrFail (&mDebugMpContext.MpContextSpinLock)) {
      break;
    }
    CpuPause ();
    continue;
  }
}

/**
  Release access control on MP context.

**/
VOID
ReleaseMpContextControl (
  VOID
  )
{
  ReleaseSpinLock (&mDebugMpContext.MpContextSpinLock);
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
  if (!IsBsp (CurrentProcessorIndex)) {
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

  AcquireMpContextControl ();

  for (Index = 0; Index < mDebugCpuData.CpuCount; Index ++) {
    if (mDebugCpuData.ApicID[Index] == LocalApicID) {
      break;
    }
  }

  if (Index == mDebugCpuData.CpuCount) {
    mDebugCpuData.ApicID[Index] = LocalApicID;
    mDebugCpuData.CpuCount ++ ;
  }

  ReleaseMpContextControl ();

  return Index;
}

/**
  Check if the specified processor is BSP or not.

  @param[in] ProcessorIndex Processor index value.

  @retval TRUE    It is BSP.
  @retval FALSE   It isn't BSP.

**/
BOOLEAN
IsBsp (
  IN UINT32             ProcessorIndex
  )
{
  if (AsmMsrBitFieldRead64 (MSR_IA32_APIC_BASE_ADDRESS, 8, 8) == 1) {
    if (mDebugMpContext.BspIndex != ProcessorIndex) {
      AcquireMpContextControl ();
      mDebugMpContext.BspIndex = ProcessorIndex;
      ReleaseMpContextControl ();
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

  AcquireMpContextControl ();

  Value = mDebugMpContext.CpuStopStatusMask[ProcessorIndex / 8];
  Index = ProcessorIndex % 8;
  if (StopFlag) {
    Value = BitFieldWrite8 (Value, Index, Index, 1);
  } else {
    Value = BitFieldWrite8 (Value, Index, Index, 0);
  }
  mDebugMpContext.CpuStopStatusMask[ProcessorIndex / 8] = Value;

  ReleaseMpContextControl ();
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

  AcquireMpContextControl ();

  Value = mDebugMpContext.CpuBreakMask[ProcessorIndex / 8];
  Index = ProcessorIndex % 8;
  if (BreakFlag) {
    Value = BitFieldWrite8 (Value, Index, Index, 1);
  } else {
    Value = BitFieldWrite8 (Value, Index, Index, 0);
  }
  mDebugMpContext.CpuBreakMask[ProcessorIndex / 8] = Value;

  ReleaseMpContextControl ();
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
  AcquireMpContextControl ();

  mDebugMpContext.RunCommandSet = RunningFlag;

  ReleaseMpContextControl ();
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
  AcquireMpContextControl ();

  mDebugMpContext.ViewPointIndex = ProcessorIndex;

  ReleaseMpContextControl ();
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
  AcquireMpContextControl ();

  mDebugMpContext.IpiSentByAp = IpiSentByApFlag;

  ReleaseMpContextControl ();
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
  if (MultiProcessorDebugSupport) {
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

