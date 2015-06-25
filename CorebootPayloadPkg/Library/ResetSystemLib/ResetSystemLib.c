/** @file
  Reset System Library functions for coreboot

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
