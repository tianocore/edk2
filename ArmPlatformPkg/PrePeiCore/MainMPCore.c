/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/ArmMPCoreMailBoxLib.h>
#include <Chipset/ArmV7.h>
#include <Drivers/PL390Gic.h>

extern EFI_PEI_PPI_DESCRIPTOR *gSecPpiTable;

/*
 * This is the main function for secondary cores. They loop around until a non Null value is written to
 * SYS_FLAGS register.The SYS_FLAGS register is platform specific.
 * Note:The secondary cores, while executing secondary_main, assumes that:
 *      : SGI 0 is configured as Non-secure interrupt
 *      : Priority Mask is configured to allow SGI 0
 *      : Interrupt Distributor and CPU interfaces are enabled
 *
 */
VOID
EFIAPI
secondary_main(IN UINTN CoreId)
{
	//Function pointer to Secondary Core entry point
	VOID (*secondary_start)(VOID);
	UINTN secondary_entry_addr=0;

	//Clear Secondary cores MailBox
	ArmClearMPCoreMailbox();

	while (secondary_entry_addr = ArmGetMPCoreMailbox(), secondary_entry_addr == 0) {
		ArmCallWFI();
	  //Acknowledge the interrupt and send End of Interrupt signal.
		PL390GicAcknowledgeSgiFrom(PcdGet32(PcdGicInterruptInterfaceBase),0/*CoreId*/);
	}

	secondary_start = (VOID (*)())secondary_entry_addr;

	//Jump to secondary core entry point.
	secondary_start();

	//the secondaries shouldn't reach here
	ASSERT(FALSE);
}

VOID primary_main (
  IN  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint
  )
{
	EFI_SEC_PEI_HAND_OFF        SecCoreData;

	//Enable the GIC Distributor
	PL390GicEnableDistributor(PcdGet32(PcdGicDistributorBase));

	// If ArmVe has not been built as Standalone then we need to wake up the secondary cores
	if (FeaturePcdGet(PcdStandalone) == FALSE) {
		// Sending SGI to all the Secondary CPU interfaces
		PL390GicSendSgiTo (PcdGet32(PcdGicDistributorBase), GIC_ICDSGIR_FILTER_EVERYONEELSE, 0x0E);
	}

	//
	// Bind this information into the SEC hand-off state
	// Note: this must be in sync with the stuff in the asm file
	// Note also:  HOBs (pei temp ram) MUST be above stack
	//
	SecCoreData.DataSize               = sizeof(EFI_SEC_PEI_HAND_OFF);
	SecCoreData.BootFirmwareVolumeBase = (VOID *)(UINTN)PcdGet32 (PcdEmbeddedFdBaseAddress);
	SecCoreData.BootFirmwareVolumeSize = PcdGet32 (PcdEmbeddedFdSize);
	SecCoreData.TemporaryRamBase       = (VOID *)(UINTN)PcdGet32 (PcdCPUCoresNonSecStackBase); // We consider we run on the primary core (and so we use the first stack)
	SecCoreData.TemporaryRamSize       = (UINTN)(UINTN)PcdGet32 (PcdCPUCoresNonSecStackSize);
	SecCoreData.PeiTemporaryRamBase    = (VOID *)((UINTN)(SecCoreData.TemporaryRamBase) + (SecCoreData.TemporaryRamSize / 2));
	SecCoreData.PeiTemporaryRamSize    = SecCoreData.TemporaryRamSize / 2;
	SecCoreData.StackBase              = SecCoreData.TemporaryRamBase;
	SecCoreData.StackSize              = SecCoreData.TemporaryRamSize - SecCoreData.PeiTemporaryRamSize;

	// jump to pei core entry point
	(PeiCoreEntryPoint)(&SecCoreData, (VOID *)&gSecPpiTable);
}
