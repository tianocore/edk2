/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:


  IchTcoReset.c

Abstract:
  Implements the programming of events in TCO Reset


--*/

#include "PlatformDxe.h"
#include <Protocol/TcoReset.h>
#include <Protocol/HwWatchdogTimer.h>


EFI_STATUS
EFIAPI
EnableTcoReset (
  IN      UINT32            *RcrbGcsSaveValue
  );
  
EFI_STATUS
EFIAPI
DisableTcoReset (
  OUT     UINT32    RcrbGcsRestoreValue
  );

EFI_TCO_RESET_PROTOCOL  mTcoResetProtocol = {
  EnableTcoReset,
  DisableTcoReset
};

/**

  Enables the TCO timer to reset the system in case of a system hang.  This is
  used when writing the clock registers.

  @param RcrbGcsSaveValue   This is the value of the RCRB GCS register before it is
                            changed by this procedure.  This will be used to restore
                            the settings of this register in PpiDisableTcoReset.

  @retval  EFI_STATUS

**/
EFI_STATUS
EFIAPI
EnableTcoReset (
  IN      UINT32            *RcrbGcsSaveValue
  )
{
  UINT16          TmpWord;
  UINT16          AcpiBase;
  EFI_WATCHDOG_TIMER_DRIVER_PROTOCOL  *WatchdogTimerProtocol;
  EFI_STATUS          Status;
  UINTN           PbtnDisableInterval = 4;  //Default value

  //
  // Get Watchdog Timer protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiWatchdogTimerDriverProtocolGuid,
                  NULL,
                  (VOID **)&WatchdogTimerProtocol
                  );

  //
  // If the protocol is present, shut off the Timer as we enter BDS
  //
  if (!EFI_ERROR(Status)) {
    WatchdogTimerProtocol->RestartWatchdogTimer();
    WatchdogTimerProtocol->AllowKnownReset(TRUE);
  }

  if (*RcrbGcsSaveValue == 0) {
    PbtnDisableInterval = PcdGet32(PcdPBTNDisableInterval);
  } else {
    PbtnDisableInterval = *RcrbGcsSaveValue * 10 / 6;
  }

  //
  // Read ACPI Base Address
  //
  AcpiBase = PchLpcPciCfg16(R_PCH_LPC_ACPI_BASE) & B_PCH_LPC_ACPI_BASE_BAR;

  //
  // Stop TCO if not already stopped
  //
  TmpWord = IoRead16(AcpiBase + R_PCH_TCO_CNT);
  TmpWord |= B_PCH_TCO_CNT_TMR_HLT;
  IoWrite16(AcpiBase + R_PCH_TCO_CNT, TmpWord);

  //
  // Clear second TCO status
  //
  IoWrite32(AcpiBase + R_PCH_TCO_STS, B_PCH_TCO_STS_SECOND_TO);

  //
  // Enable reboot on TCO timeout
  //
  *RcrbGcsSaveValue = MmioRead32 (PMC_BASE_ADDRESS + R_PCH_PMC_PM_CFG);
  MmioAnd8 (PMC_BASE_ADDRESS + R_PCH_PMC_PM_CFG, (UINT8) ~B_PCH_PMC_PM_CFG_NO_REBOOT);

  //
  // Set TCO reload value (interval *.6s)
  //
  IoWrite32(AcpiBase + R_PCH_TCO_TMR, (UINT32)(PbtnDisableInterval<<16));

  //
  // Force TCO to load new value
  //
  IoWrite8(AcpiBase + R_PCH_TCO_RLD, 4);

  //
  // Clear second TCO status
  //
  IoWrite32(AcpiBase + R_PCH_TCO_STS, B_PCH_TCO_STS_SECOND_TO);

  //
  // Start TCO timer running
  //
  TmpWord = IoRead16(AcpiBase + R_PCH_TCO_CNT);
  TmpWord &= ~(B_PCH_TCO_CNT_TMR_HLT);
  IoWrite16(AcpiBase + R_PCH_TCO_CNT, TmpWord);

  return EFI_SUCCESS;
}

/**
  Disables the TCO timer.  This is used after writing the clock registers.

  @param RcrbGcsRestoreValue   Value saved in PpiEnableTcoReset so that it can
                               restored.

  @retval EFI_STATUS

**/
EFI_STATUS
EFIAPI
DisableTcoReset (
  OUT     UINT32    RcrbGcsRestoreValue
  )
{
  UINT16          TmpWord;
  UINT16          AcpiBase;
  EFI_WATCHDOG_TIMER_DRIVER_PROTOCOL  *WatchdogTimerProtocol;
  EFI_STATUS          Status;

  //
  // Read ACPI Base Address
  //
  AcpiBase = PchLpcPciCfg16(R_PCH_LPC_ACPI_BASE) & B_PCH_LPC_ACPI_BASE_BAR;

  //
  // Stop the TCO timer
  //
  TmpWord = IoRead16(AcpiBase + R_PCH_TCO_CNT);
  TmpWord |= B_PCH_TCO_CNT_TMR_HLT;
  IoWrite16(AcpiBase + R_PCH_TCO_CNT, TmpWord);

  //
  // Get Watchdog Timer protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiWatchdogTimerDriverProtocolGuid,
                  NULL,
                  (VOID **)&WatchdogTimerProtocol
                  );

  //
  // If the protocol is present, shut off the Timer as we enter BDS
  //
  if (!EFI_ERROR(Status)) {
    WatchdogTimerProtocol->AllowKnownReset(FALSE);
  }

  return EFI_SUCCESS;
}

/**

  Updates the feature policies according to the setup variable.

  @retval Returns   VOID

**/
VOID
InitTcoReset (
  )
{
  EFI_HANDLE                        Handle;
  EFI_STATUS                        Status;

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiTcoResetProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mTcoResetProtocol
                  );
  ASSERT_EFI_ERROR(Status);

}
