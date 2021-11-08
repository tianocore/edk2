/** @file
  Reset System Library functions for bootloader

  Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/AcpiBoardInfoGuid.h>

ACPI_BOARD_INFO    mAcpiBoardInfo;

/**
  The constructor function to initialize mAcpiBoardInfo.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
RETURN_STATUS
EFIAPI
ResetSystemLibConstructor (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  ACPI_BOARD_INFO    *AcpiBoardInfoPtr;

  //
  // Find the acpi board information guid hob
  //
  GuidHob = GetFirstGuidHob (&gUefiAcpiBoardInfoGuid);
  ASSERT (GuidHob != NULL);

  AcpiBoardInfoPtr = (ACPI_BOARD_INFO *)GET_GUID_HOB_DATA (GuidHob);
  CopyMem (&mAcpiBoardInfo, AcpiBoardInfoPtr, sizeof (ACPI_BOARD_INFO));

  return EFI_SUCCESS;
}

/**
  Calling this function causes a system-wide reset. This sets
  all circuitry within the system to its initial state. This type of reset
  is asynchronous to system operation and operates without regard to
  cycle boundaries.

  System reset should not return, if it returns, it means the system does
  not support cold reset.
**/
VOID
EFIAPI
ResetCold (
  VOID
  )
{
  IoWrite8 ((UINTN)mAcpiBoardInfo.ResetRegAddress, mAcpiBoardInfo.ResetValue);
  CpuDeadLoop ();
}

/**
  Calling this function causes a system-wide initialization. The processors
  are set to their initial state, and pending cycles are not corrupted.

  System reset should not return, if it returns, it means the system does
  not support warm reset.
**/
VOID
EFIAPI
ResetWarm (
  VOID
  )
{
  IoWrite8 ((UINTN)mAcpiBoardInfo.ResetRegAddress, mAcpiBoardInfo.ResetValue);
  CpuDeadLoop ();
}

/**
  Calling this function causes the system to enter a power state equivalent
  to the ACPI G2/S5 or G3 states.

  System shutdown should not return, if it returns, it means the system does
  not support shut down reset.
**/
VOID
EFIAPI
ResetShutdown (
  VOID
  )
{
  UINTN              PmCtrlReg;

  //
  // GPE0_EN should be disabled to avoid any GPI waking up the system from S5
  //
  IoWrite16 ((UINTN)mAcpiBoardInfo.PmGpeEnBase,  0);

  //
  // Clear Power Button Status
  //
  IoWrite16((UINTN) mAcpiBoardInfo.PmEvtBase, BIT8);

  //
  // Transform system into S5 sleep state
  //
  PmCtrlReg = (UINTN)mAcpiBoardInfo.PmCtrlRegBase;
  IoAndThenOr16 (PmCtrlReg, (UINT16) ~0x3c00, (UINT16) (7 << 10));
  IoOr16 (PmCtrlReg, BIT13);
  CpuDeadLoop ();

  ASSERT (FALSE);
}

/**
  This function causes a systemwide reset. The exact type of the reset is
  defined by the EFI_GUID that follows the Null-terminated Unicode string passed
  into ResetData. If the platform does not recognize the EFI_GUID in ResetData
  the platform must pick a supported reset type to perform.The platform may
  optionally log the parameters from any non-normal reset that occurs.

  @param[in]  DataSize   The size, in bytes, of ResetData.
  @param[in]  ResetData  The data buffer starts with a Null-terminated string,
                         followed by the EFI_GUID.
**/
VOID
EFIAPI
ResetPlatformSpecific (
  IN UINTN   DataSize,
  IN VOID    *ResetData
  )
{
  ResetCold ();
}
