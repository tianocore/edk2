/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <PiPei.h>

#include <Library/ArmLib.h>
#include <Library/ArmGicLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/ArmPlatformSecLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>

#include <Chipset/ArmV7.h>

// When the firmware is built as not Standalone, the secondary cores need to wait the firmware
// entirely written into DRAM. It is the firmware from DRAM which will wake up the secondary cores.
VOID
NonSecureWaitForFirmware (
  VOID
  )
{
  VOID (*secondary_start)(VOID);

  // The secondary cores will execute the firmware once wake from WFI.
  secondary_start = (VOID (*)())PcdGet32(PcdFvBaseAddress);

  ArmCallWFI();

  // Acknowledge the interrupt and send End of Interrupt signal.
  ArmGicAcknowledgeInterrupt (PcdGet32(PcdGicDistributorBase), PcdGet32(PcdGicInterruptInterfaceBase), NULL, NULL);

  // Jump to secondary core entry point.
  secondary_start ();

  // PEI Core should always load and never return
  ASSERT (FALSE);
}

/**
  Call before jumping to Normal World

  This function allows the firmware platform to do extra actions before
  jumping to the Normal World

**/
VOID
ArmPlatformSecExtraAction (
  IN  UINTN         MpId,
  OUT UINTN*        JumpAddress
  )
{
  CHAR8           Buffer[100];
  UINTN           CharCount;

  if (FeaturePcdGet (PcdStandalone) == FALSE) {

    //
    // Warning: This code assumes the DRAM has already been initialized by ArmPlatformSecLib
    //

    if (ArmPlatformIsPrimaryCore (MpId)) {
      UINTN*   StartAddress = (UINTN*)PcdGet32(PcdFvBaseAddress);

      // Patch the DRAM to make an infinite loop at the start address
      *StartAddress = 0xEAFFFFFE; // opcode for while(1)

      CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Waiting for firmware at 0x%08X ...\n\r",StartAddress);
      SerialPortWrite ((UINT8 *) Buffer, CharCount);

      *JumpAddress = PcdGet32(PcdFvBaseAddress);
    } else {
      // When the primary core is stopped by the hardware debugger to copy the firmware
      // into DRAM. The secondary cores are still running. As soon as the first bytes of
      // the firmware are written into DRAM, the secondary cores will start to execute the
      // code even if the firmware is not entirely written into the memory.
      // That's why the secondary cores need to be parked in WFI and wake up once the
      // firmware is ready.

      *JumpAddress = (UINTN)NonSecureWaitForFirmware;
    }
  } else if (FeaturePcdGet (PcdSystemMemoryInitializeInSec)) {

    //
    // Warning: This code assumes the DRAM has already been initialized by ArmPlatformSecLib
    //

    if (ArmPlatformIsPrimaryCore (MpId)) {
      // Signal the secondary cores they can jump to PEI phase
      ArmGicSendSgiTo (PcdGet32(PcdGicDistributorBase), ARM_GIC_ICDSGIR_FILTER_EVERYONEELSE, 0x0E, PcdGet32 (PcdGicSgiIntId));

      // To enter into Non Secure state, we need to make a return from exception
      *JumpAddress = PcdGet32(PcdFvBaseAddress);
    } else {
      // We wait for the primary core to finish to initialize the System Memory. Otherwise the secondary
      // cores would make crash the system by setting their stacks in DRAM before the primary core has not
      // finished to initialize the system memory.
      *JumpAddress = (UINTN)NonSecureWaitForFirmware;
    }
  } else {
    *JumpAddress = PcdGet32(PcdFvBaseAddress);
  }
}
