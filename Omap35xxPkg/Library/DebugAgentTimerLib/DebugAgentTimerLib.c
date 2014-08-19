/** @file
  Debug Agent timer lib for OMAP 35xx.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/OmapLib.h>
#include <Library/ArmLib.h>
#include <Library/PcdLib.h>

#include <Omap3530/Omap3530.h>


volatile UINT32 gVector;

// Cached registers
volatile UINT32 gTISR;
volatile UINT32 gTCLR;
volatile UINT32 gTLDR;
volatile UINT32 gTCRR;
volatile UINT32 gTIER;

VOID
EnableInterruptSource (
  VOID
  )
{
  UINTN Bank;
  UINTN Bit;

  // Map vector to FIQ, IRQ is default
  MmioWrite32 (INTCPS_ILR (gVector), 1);

  Bank = gVector / 32;
  Bit  = 1UL << (gVector % 32);

  MmioWrite32 (INTCPS_MIR_CLEAR(Bank), Bit);
}

VOID
DisableInterruptSource (
  VOID
  )
{
  UINTN Bank;
  UINTN Bit;

  Bank = gVector / 32;
  Bit  = 1UL << (gVector % 32);

  MmioWrite32 (INTCPS_MIR_SET(Bank), Bit);
}



/**
  Setup all the hardware needed for the debug agents timer.

  This function is used to set up debug enviroment. It may enable interrupts.

**/
VOID
EFIAPI
DebugAgentTimerIntialize (
  VOID
  )
{
  UINT32      TimerBaseAddress;
  UINT32      TimerNumber;

  TimerNumber = PcdGet32(PcdOmap35xxDebugAgentTimer);
  gVector = InterruptVectorForTimer (TimerNumber);

  // Set up the timer registers
  TimerBaseAddress = TimerBase (TimerNumber);
  gTISR = TimerBaseAddress + GPTIMER_TISR;
  gTCLR = TimerBaseAddress + GPTIMER_TCLR;
  gTLDR = TimerBaseAddress + GPTIMER_TLDR;
  gTCRR = TimerBaseAddress + GPTIMER_TCRR;
  gTIER = TimerBaseAddress + GPTIMER_TIER;

  if ((TimerNumber < 2) || (TimerNumber > 9)) {
    // This code assumes one the General Purpose timers is used
    // GPT2 - GPT9
    CpuDeadLoop ();
  }
  // Set source clock for GPT2 - GPT9 to SYS_CLK
  MmioOr32 (CM_CLKSEL_PER, 1 << (TimerNumber - 2));

}


/**
  Set the period for the debug agent timer. Zero means disable the timer.

  @param[in] TimerPeriodMilliseconds    Frequency of the debug agent timer.

**/
VOID
EFIAPI
DebugAgentTimerSetPeriod (
  IN  UINT32  TimerPeriodMilliseconds
  )
{
  UINT64      TimerCount;
  INT32       LoadValue;

  if (TimerPeriodMilliseconds == 0) {
    // Turn off GPTIMER3
    MmioWrite32 (gTCLR, TCLR_ST_OFF);

    DisableInterruptSource ();
  } else {
    // Calculate required timer count
    TimerCount = DivU64x32(TimerPeriodMilliseconds * 1000000, PcdGet32(PcdDebugAgentTimerFreqNanoSeconds));

    // Set GPTIMER5 Load register
    LoadValue = (INT32) -TimerCount;
    MmioWrite32 (gTLDR, LoadValue);
    MmioWrite32 (gTCRR, LoadValue);

    // Enable Overflow interrupt
    MmioWrite32 (gTIER, TIER_TCAR_IT_DISABLE | TIER_OVF_IT_ENABLE | TIER_MAT_IT_DISABLE);

    // Turn on GPTIMER3, it will reload at overflow
    MmioWrite32 (gTCLR, TCLR_AR_AUTORELOAD | TCLR_ST_ON);

    EnableInterruptSource ();
  }
}


/**
  Perform End Of Interrupt for the debug agent timer. This is called in the
  interrupt handler after the interrupt has been processed.

**/
VOID
EFIAPI
DebugAgentTimerEndOfInterrupt (
  VOID
  )
{
   // Clear all timer interrupts
  MmioWrite32 (gTISR, TISR_CLEAR_ALL);

  // Poll interrupt status bits to ensure clearing
  while ((MmioRead32 (gTISR) & TISR_ALL_INTERRUPT_MASK) != TISR_NO_INTERRUPTS_PENDING);

  MmioWrite32 (INTCPS_CONTROL, INTCPS_CONTROL_NEWFIQAGR);
  ArmDataSyncronizationBarrier ();

}

