/** @file
File to contain all the hardware specific stuff for the Periodical Timer dispatch protocol.

Copyright (c) 2013-2016 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "QNCSmmHelpers.h"

typedef enum {
  PERIODIC_TIMER = 0,
  NUM_TIMERS
} SUPPORTED_TIMER;

typedef struct _TIMER_INTERVAL
{
  UINT64    Interval;
  UINT8     AssociatedTimer;
} TIMER_INTERVAL;

//
// Time constants, in 100 nano-second units
//
#define TIME_64s   640000000 /* 64   s  */
#define TIME_32s   320000000 /* 32   s  */
#define TIME_16s   160000000 /* 16   s  */
#define TIME_8s     80000000 /*  8   s  */
#define TIME_64ms     640000 /* 64   ms */
#define TIME_32ms     320000 /* 32   ms */
#define TIME_16ms     160000 /* 16   ms */
#define TIME_1_5ms     15000 /*  1.5 ms */

// PMCW (GPE+28h) [2:0] Periodic SMI Rate selection
// 000 1.5ms
// 001 16ms
// 010 32ms
// 011 64ms
// 100 8s
// 101 16s
// 110 32s
// 111 64s

typedef enum {
  INDEX_TIME_1_5ms = 0,
  INDEX_TIME_16ms,
  INDEX_TIME_32ms,
  INDEX_TIME_64ms,
  INDEX_TIME_8s,
  INDEX_TIME_16s,
  INDEX_TIME_32s,
  INDEX_TIME_64s,
  INDEX_TIME_MAX
} TIMER_INTERVAL_INDEX;

TIMER_INTERVAL mSmmPeriodicTimerIntervals[INDEX_TIME_MAX] = {
  {TIME_1_5ms, PERIODIC_TIMER},
  {TIME_16ms,  PERIODIC_TIMER},
  {TIME_32ms,  PERIODIC_TIMER},
  {TIME_64ms,  PERIODIC_TIMER},
  { TIME_8s,    PERIODIC_TIMER },
  {TIME_16s,   PERIODIC_TIMER},
  {TIME_32s,   PERIODIC_TIMER},
  {TIME_64s,   PERIODIC_TIMER}
};

typedef struct _TIMER_INFO {
  UINTN     NumChildren;      // number of children using this timer
  UINT64    MinReqInterval;   // minimum interval required by children
  UINTN     CurrentSetting;   // interval this timer is set at right now (index into interval table)
} TIMER_INFO;

TIMER_INFO  mTimers[NUM_TIMERS];

QNC_SMM_SOURCE_DESC mTIMER_SOURCE_DESCS[NUM_TIMERS] = {
  {
    QNC_SMM_NO_FLAGS,
    {
      {{GPE_ADDR_TYPE, {R_QNC_GPE0BLK_SMIE}}, S_QNC_GPE0BLK_SMIE, N_QNC_GPE0BLK_SMIE_SWT},
      NULL_BIT_DESC_INITIALIZER
    },
    {
      {{GPE_ADDR_TYPE, {R_QNC_GPE0BLK_SMIS}}, S_QNC_GPE0BLK_SMIS, N_QNC_GPE0BLK_SMIS_SWT}
    }
  }
};

VOID
QNCSmmPeriodicTimerProgramTimers(
  VOID
  );


TIMER_INTERVAL *
ContextToTimerInterval (
  IN  QNC_SMM_CONTEXT     *RegisterContext
  )
{
  UINTN loopvar;

  //
  // Determine which timer this child is using
  //
  for (loopvar = 0; loopvar < INDEX_TIME_MAX; loopvar++) {
    if (((RegisterContext->PeriodicTimer.SmiTickInterval == 0) && (RegisterContext->PeriodicTimer.Period >= mSmmPeriodicTimerIntervals[loopvar].Interval)) ||
        (RegisterContext->PeriodicTimer.SmiTickInterval == mSmmPeriodicTimerIntervals[loopvar].Interval)
       ) {
        return &mSmmPeriodicTimerIntervals[loopvar];
      }
  }

  //
  // If this assertion fires, then either:
  //    (1) the context contains an invalid interval
  //    (2) the timer interval table is corrupt
  //
  // ASSERT (FALSE);

  return NULL;
}

EFI_STATUS
MapPeriodicTimerToSrcDesc (
  IN  QNC_SMM_CONTEXT             *RegisterContext,
  OUT QNC_SMM_SOURCE_DESC         *SrcDesc
  )
{
  TIMER_INTERVAL  *TimerInterval;

  //
  // Figure out which timer the child is requesting and
  // send back the source description
  //
  TimerInterval = ContextToTimerInterval (RegisterContext);
  if (TimerInterval == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  CopyMem (SrcDesc, &mTIMER_SOURCE_DESCS[TimerInterval->AssociatedTimer], sizeof (QNC_SMM_SOURCE_DESC));;

  //
  // Program the value of the interval into hardware
  //
  QNCSmmPeriodicTimerProgramTimers ();

  return EFI_SUCCESS;
}

VOID
PeriodicTimerGetContext (
  IN  DATABASE_RECORD    *Record,
  OUT QNC_SMM_CONTEXT    *HwContext
  )
{
  TIMER_INTERVAL    *TimerInterval;

  ASSERT (Record->ProtocolType == PeriodicTimerType);

  TimerInterval = ContextToTimerInterval (&Record->ChildContext);

  if (TimerInterval != NULL) {
    //
    // Ignore the hardware context. It's not required for this protocol.
    // Instead, just increment the child's context.
    // Update the elapsed time w/ the data from our tables
    //
    Record->CommBuffer.PeriodicTimer.ElapsedTime += TimerInterval->Interval;
    CopyMem (HwContext, &Record->ChildContext, sizeof (QNC_SMM_CONTEXT));
  }
}

BOOLEAN
PeriodicTimerCmpContext (
  IN QNC_SMM_CONTEXT     *HwContext,
  IN QNC_SMM_CONTEXT     *ChildContext
  )
{
  DATABASE_RECORD    *Record;

  Record = DATABASE_RECORD_FROM_CONTEXT (ChildContext);

  if (Record->CommBuffer.PeriodicTimer.ElapsedTime >= ChildContext->PeriodicTimer.Period) {
    //
    // This child should be dispatched
    // The timer will be restarted on the "ClearSource" call.
    //
    return TRUE;
  } else {
    return FALSE;
  }
}

VOID
PeriodicTimerGetBuffer (
  IN  DATABASE_RECORD     * Record
  )
{
  //
  // CommBuffer has been updated by PeriodicTimerGetContext, so return directly
  //
  return;
}

VOID
QNCSmmPeriodicTimerProgramTimers (
  VOID
  )
{
  UINT32            GpePmcwValue;
  SUPPORTED_TIMER   Timer;
  DATABASE_RECORD   *RecordInDb;
  LIST_ENTRY        *LinkInDb;
  TIMER_INTERVAL    *TimerInterval;

  //
  // Find the minimum required interval for each timer
  //
  for (Timer = (SUPPORTED_TIMER)0; Timer < NUM_TIMERS; Timer++) {
    mTimers[Timer].MinReqInterval = ~(UINT64)0x0;
    mTimers[Timer].NumChildren = 0;
  }
  LinkInDb = GetFirstNode (&mPrivateData.CallbackDataBase);
  while (!IsNull (&mPrivateData.CallbackDataBase, LinkInDb)) {
    RecordInDb = DATABASE_RECORD_FROM_LINK (LinkInDb);
    if (RecordInDb->ProtocolType == PeriodicTimerType) {
      //
      // This child is registerd with the PeriodicTimer protocol
      //
      TimerInterval = ContextToTimerInterval (&RecordInDb->ChildContext);

      if(TimerInterval != NULL) {
        Timer = (SUPPORTED_TIMER)((TIMER_INTERVAL *) (TimerInterval))->AssociatedTimer;

        ASSERT (Timer >= 0 && Timer < NUM_TIMERS);

        if (mTimers[Timer].MinReqInterval > RecordInDb->ChildContext.PeriodicTimer.SmiTickInterval) {
          mTimers[Timer].MinReqInterval = RecordInDb->ChildContext.PeriodicTimer.SmiTickInterval;
        }
        mTimers[Timer].NumChildren++;
      }
    }
    LinkInDb = GetNextNode (&mPrivateData.CallbackDataBase, &RecordInDb->Link);
  }

  //
  // Program the hardware
  //
  GpePmcwValue = 0;
  if (mTimers[PERIODIC_TIMER].NumChildren > 0) {
    switch (mTimers[PERIODIC_TIMER].MinReqInterval) {

    case TIME_64s:
      GpePmcwValue = INDEX_TIME_64s;
      mTimers[PERIODIC_TIMER].CurrentSetting = INDEX_TIME_64s;
      break;

    case TIME_32s:
      GpePmcwValue = INDEX_TIME_32s;
      mTimers[PERIODIC_TIMER].CurrentSetting = INDEX_TIME_32s;
      break;

    case TIME_16s:
      GpePmcwValue = INDEX_TIME_16s;
      mTimers[PERIODIC_TIMER].CurrentSetting = INDEX_TIME_16s;
      break;

    case TIME_8s:
      GpePmcwValue = INDEX_TIME_8s;
      mTimers[PERIODIC_TIMER].CurrentSetting = INDEX_TIME_8s;
      break;

    case TIME_64ms:
      GpePmcwValue = INDEX_TIME_64ms;
      mTimers[PERIODIC_TIMER].CurrentSetting = INDEX_TIME_64ms;
      break;

    case TIME_32ms:
      GpePmcwValue = INDEX_TIME_32ms;
      mTimers[PERIODIC_TIMER].CurrentSetting = INDEX_TIME_32ms;
      break;

    case TIME_16ms:
      GpePmcwValue = INDEX_TIME_16ms;
      mTimers[PERIODIC_TIMER].CurrentSetting = INDEX_TIME_16ms;
      break;

    case TIME_1_5ms:
      GpePmcwValue = INDEX_TIME_1_5ms;
      mTimers[PERIODIC_TIMER].CurrentSetting = INDEX_TIME_1_5ms;
      break;

    default:
      ASSERT (FALSE);
      break;
    };

    GpePmcwValue |= B_QNC_GPE0BLK_PMCW_PSE;

    IoOr32(((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_PMCW), GpePmcwValue);

    //
    // Restart the timer here, just need to clear the SMI
    //
    QNCSmmClearSource (&mTIMER_SOURCE_DESCS[PERIODIC_TIMER]);
  } else {
    QNCSmmDisableSource (&mTIMER_SOURCE_DESCS[PERIODIC_TIMER]);
  }
}

EFI_STATUS
QNCSmmPeriodicTimerDispatchGetNextShorterInterval (
  IN CONST EFI_SMM_PERIODIC_TIMER_DISPATCH2_PROTOCOL  *This,
  IN OUT   UINT64                                    **SmiTickInterval
  )
/*++

Routine Description:

  This services returns the next SMI tick period that is supported by the chipset.
  The order returned is from longest to shortest interval period.

Arguments:

  This              - Pointer to the EFI_SMM_PERIODIC_TIMER_DISPATCH2_PROTOCOL instance.
  SmiTickInterval   - Pointer to pointer of the next shorter SMI interval period that is supported by the child.

Returns:

  EFI_SUCCESS              - The service returned successfully.
  EFI_INVALID_PARAMETER   - The parameter SmiTickInterval is invalid.

--*/
{
  TIMER_INTERVAL    *IntervalPointer;

  ASSERT (SmiTickInterval != NULL);

  IntervalPointer = (TIMER_INTERVAL*)*SmiTickInterval;

  if (IntervalPointer == NULL) {
    //
    // The first time child requesting an interval
    //
    IntervalPointer = &mSmmPeriodicTimerIntervals[0];
  } else if (IntervalPointer == &mSmmPeriodicTimerIntervals[INDEX_TIME_MAX - 1]) {
    //
    // At end of the list
    //
    IntervalPointer = NULL;
  } else {
    if ((IntervalPointer >= &mSmmPeriodicTimerIntervals[0]) &&
        (IntervalPointer < &mSmmPeriodicTimerIntervals[INDEX_TIME_MAX - 1])) {
      //
      // Get the next interval in the list
      //
      IntervalPointer++;
    } else {
      //
      // Input is out of range
      //
      return EFI_INVALID_PARAMETER;
    }
  }

  if (IntervalPointer != NULL) {
  *SmiTickInterval = &IntervalPointer->Interval;
  } else {
    *SmiTickInterval = NULL;
  }

  return EFI_SUCCESS;
}

VOID
QNCSmmPeriodicTimerClearSource (
  IN QNC_SMM_SOURCE_DESC     *SrcDesc
  )
/*++

Routine Description:

  This function is responsible for calculating and enabling any timers that are required
  to dispatch messages to children. The SrcDesc argument isn't acutally used.

Arguments:

  SrcDesc - Pointer to the QNC_SMM_SOURCE_DESC instance.

Returns:

  None.

--*/
{
  DATABASE_RECORD   *RecordInDb;
  LIST_ENTRY        *LinkInDb;

  QNCSmmPeriodicTimerProgramTimers ();

  //
  // Reset Elapsed time
  //
  LinkInDb = GetFirstNode (&mPrivateData.CallbackDataBase);
  while (!IsNull (&mPrivateData.CallbackDataBase, LinkInDb)) {
    RecordInDb = DATABASE_RECORD_FROM_LINK (LinkInDb);
    if (RecordInDb->ProtocolType == PeriodicTimerType) {
      //
      // This child is registerd with the PeriodicTimer protocol and Callback
      // has been invoked, so reset the ElapsedTime to 0
      //
      if (RecordInDb->CommBuffer.PeriodicTimer.ElapsedTime >= RecordInDb->ChildContext.PeriodicTimer.Period) {
        RecordInDb->CommBuffer.PeriodicTimer.ElapsedTime = 0;
      }
    }
    LinkInDb = GetNextNode (&mPrivateData.CallbackDataBase, &RecordInDb->Link);
  }
}

