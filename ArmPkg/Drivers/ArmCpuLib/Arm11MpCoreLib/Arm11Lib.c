/** @file

  Copyright (c) 2011, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/ArmCpuLib.h>
#include <Library/ArmGicLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>

VOID
ArmCpuSynchronizeSignal (
  IN ARM_CPU_SYNCHRONIZE_EVENT   Event
  )
{
  if (Event == ARM_CPU_EVENT_BOOT_MEM_INIT) {
    // Do nothing, Cortex A9 secondary cores are waiting for the SCU to be
    // enabled (done by ArmCpuSetup()) as a way to know when the Init Boot
    // Mem as been initialized
  } else {
    // Send SGI to all Secondary core to wake them up from WFI state.
    ArmGicSendSgiTo (PcdGet32(PcdGicDistributorBase), ARM_GIC_ICDSGIR_FILTER_EVERYONEELSE, 0x0E);
  }
}

VOID
CArmCpuSynchronizeWait (
  IN ARM_CPU_SYNCHRONIZE_EVENT   Event
  )
{
  // Waiting for the SGI from the primary core
  ArmCallWFI ();

  // Acknowledge the interrupt and send End of Interrupt signal.
  ArmGicAcknowledgeSgiFrom (PcdGet32(PcdGicInterruptInterfaceBase), PRIMARY_CORE_ID);
}

#if 0
VOID
ArmEnableScu (
  VOID
  )
{
  INTN          ScuBase;

  ScuBase = ArmGetScuBaseAddress();

  // Invalidate all: write -1 to SCU Invalidate All register
  MmioWrite32(ScuBase + A9_SCU_INVALL_OFFSET, 0xffffffff);
  // Enable SCU
  MmioWrite32(ScuBase + A9_SCU_CONTROL_OFFSET, 0x1);
}
#endif

VOID
ArmCpuSetup (
  IN  UINTN         MpId
  )
{
  /*AMP mode and SMP mode

   By default, the processor is in AMP mode (bit 5 reset to 0). To prevent coherent data corruption the sequence to turn on MP11 CPUs in SMP mode is:

   1.Write the SCU register to change CPU mode.
   2.Disable interrupts.
   3.Clean and invalidate all the D-cache.
   4.Write SMP/nAMP bit as 1.
   5.Enable interrupts.

  Source: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0360e/BIHHFGEC.html
  */

  // If MPCore then Enable the SCU
  if (ArmIsMpCore()) {
    //ArmEnableScu ();
  }
}


VOID
ArmCpuSetupSmpNonSecure (
  IN  UINTN         MpId
  )
{
#if 0
  INTN          ScuBase;

  ArmSetAuxCrBit (A9_FEATURE_SMP);

  // Make the SCU accessible in Non Secure world
  if (IS_PRIMARY_CORE(MpId)) {
    ScuBase = ArmGetScuBaseAddress();

    // Allow NS access to SCU register
    MmioOr32 (ScuBase + A9_SCU_SACR_OFFSET, 0xf);
    // Allow NS access to Private Peripherals
    MmioOr32 (ScuBase + A9_SCU_SSACR_OFFSET, 0xfff);
  }
#endif
}

