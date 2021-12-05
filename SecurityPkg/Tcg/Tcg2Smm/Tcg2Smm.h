/** @file
  The header file for Tcg2 SMM driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TCG2_SMM_H__
#define __TCG2_SMM_H__

#include <PiMm.h>

#include <Guid/MemoryOverwriteControl.h>
#include <Guid/TpmInstance.h>
#include <Guid/TpmNvsMm.h>

#include <Protocol/MmReadyToLock.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/Tcg2Protocol.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/Tpm2DeviceLib.h>

#include <IndustryStandard/TpmPtp.h>

//
// The definition for TCG MOR
//
#define ACPI_FUNCTION_DSM_MEMORY_CLEAR_INTERFACE  1
#define ACPI_FUNCTION_PTS_CLEAR_MOR_BIT           2

//
// The return code for Memory Clear Interface Functions
//
#define MOR_REQUEST_SUCCESS          0
#define MOR_REQUEST_GENERAL_FAILURE  1

/**
  Notify the system that the SMM variable driver is ready.
**/
VOID
Tcg2NotifyMmReady (
  VOID
  );

/**
  This function is an abstraction layer for implementation specific Mm buffer validation routine.

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and not overlap with SMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlap with SMRAM.
**/
BOOLEAN
IsBufferOutsideMmValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

/**
  The driver's common initialization routine.

  It install callbacks for TPM physical presence and MemoryClear, and locate
  SMM variable to be used in the callback function.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval Others          Some error occurs when executing this entry point.

**/
EFI_STATUS
InitializeTcgCommon (
  VOID
  );

#endif // __TCG_SMM_H__
