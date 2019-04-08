/** @file
  Reset System Library functions for coreboot

  Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>

#include <Guid/AcpiBoardInfoGuid.h>

VOID
AcpiPmControl (
  UINTN SuspendType
  )
{
	EFI_HOB_GUID_TYPE  *GuidHob;
	ACPI_BOARD_INFO    *pAcpiBoardInfo;	
	UINTN PmCtrlReg = 0;
	
  ASSERT (SuspendType <= 7);  
  //
	// Find the acpi board information guid hob
	//
	GuidHob = GetFirstGuidHob (&gUefiAcpiBoardInfoGuid);
	ASSERT (GuidHob != NULL);
  pAcpiBoardInfo = (ACPI_BOARD_INFO *)GET_GUID_HOB_DATA (GuidHob); 
  
  PmCtrlReg = (UINTN)pAcpiBoardInfo->PmCtrlRegBase; 
  IoAndThenOr16 (PmCtrlReg, (UINT16) ~0x3c00, (UINT16) (SuspendType << 10));
  IoOr16 (PmCtrlReg, BIT13);
  CpuDeadLoop ();
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
  EFI_HOB_GUID_TYPE  *GuidHob;
	ACPI_BOARD_INFO    *pAcpiBoardInfo;	
		
	//
	// Find the acpi board information guid hob
	//
	GuidHob = GetFirstGuidHob (&gUefiAcpiBoardInfoGuid);
	ASSERT (GuidHob != NULL);
  pAcpiBoardInfo = (ACPI_BOARD_INFO *)GET_GUID_HOB_DATA (GuidHob); 
	
  IoWrite8 ((UINTN)pAcpiBoardInfo->ResetRegAddress, pAcpiBoardInfo->ResetValue);
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
	EFI_HOB_GUID_TYPE  *GuidHob;
	ACPI_BOARD_INFO    *pAcpiBoardInfo;	
		
	//
	// Find the acpi board information guid hob
	//
	GuidHob = GetFirstGuidHob (&gUefiAcpiBoardInfoGuid);
	ASSERT (GuidHob != NULL);
  pAcpiBoardInfo = (ACPI_BOARD_INFO *)GET_GUID_HOB_DATA (GuidHob); 
	
  IoWrite8 ((UINTN)pAcpiBoardInfo->ResetRegAddress, pAcpiBoardInfo->ResetValue);
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
  EFI_HOB_GUID_TYPE  *GuidHob;
  ACPI_BOARD_INFO    *pAcpiBoardInfo;
  UINTN              PmCtrlReg;

  //
  // Find the acpi board information guid hob
  //
  GuidHob = GetFirstGuidHob (&gUefiAcpiBoardInfoGuid);
  ASSERT (GuidHob != NULL);
  pAcpiBoardInfo = (ACPI_BOARD_INFO *)GET_GUID_HOB_DATA (GuidHob); 
  
  //
  // GPE0_EN should be disabled to avoid any GPI waking up the system from S5
  //
  IoWrite16 ((UINTN)pAcpiBoardInfo->PmGpeEnBase,  0);

  //
  // Clear Power Button Status
  //
  IoWrite16((UINTN) pAcpiBoardInfo->PmEvtBase, BIT8);
  
  //
  // Transform system into S5 sleep state
  //
  PmCtrlReg = (UINTN)pAcpiBoardInfo->PmCtrlRegBase; 
  IoAndThenOr16 (PmCtrlReg, (UINT16) ~0x3c00, (UINT16) (7 << 10));
  IoOr16 (PmCtrlReg, BIT13);
  CpuDeadLoop ();

  ASSERT (FALSE);
}

/**
  Calling this function causes the system to enter a power state for capsule
  update.

  Reset update should not return, if it returns, it means the system does
  not support capsule update.

**/
VOID
EFIAPI
EnterS3WithImmediateWake (
  VOID
  )
{
  AcpiPmControl (5);
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

/**
  The ResetSystem function resets the entire platform.

  @param[in] ResetType      The type of reset to perform.
  @param[in] ResetStatus    The status code for the reset.
  @param[in] DataSize       The size, in bytes, of ResetData.
  @param[in] ResetData      For a ResetType of EfiResetCold, EfiResetWarm, or EfiResetShutdown
                            the data buffer starts with a Null-terminated string, optionally
                            followed by additional binary data. The string is a description
                            that the caller may use to further indicate the reason for the
                            system reset.
**/
VOID
EFIAPI
ResetSystem (
  IN EFI_RESET_TYPE               ResetType,
  IN EFI_STATUS                   ResetStatus,
  IN UINTN                        DataSize,
  IN VOID                         *ResetData OPTIONAL
  )
{
  switch (ResetType) {
  case EfiResetWarm:
    ResetWarm ();
    break;

  case EfiResetCold:
    ResetCold ();
    break;

  case EfiResetShutdown:
    ResetShutdown ();
    return;

  case EfiResetPlatformSpecific:
    ResetPlatformSpecific (DataSize, ResetData);
    return;

  default:
    return;
  }
}
