/** @file
  Abstraction layer that contains Standalone MM specific implementation for
  Status Code Handler Driver.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StatusCodeHandlerMm.h"
#include <Guid/MmStatusCodeUseSerial.h>
#include <Library/HobLib.h>

/**
  Check if the status code is using serial port.

  This function determines whether the status code reporting mechanism
  is configured to use the serial port.

  @retval TRUE   Status code is using the serial port.
  @retval FALSE  Status code is not using the serial port.
**/
BOOLEAN
IsStatusCodeUsingSerialPort (
  VOID
  )
{
  VOID                       *Hob;
  MM_STATUS_CODE_USE_SERIAL  *StatusCodeUseSerialHob;

  Hob = GetFirstGuidHob (&gMmStatusCodeUseSerialHobGuid);
  ASSERT (Hob != NULL);

  StatusCodeUseSerialHob = (MM_STATUS_CODE_USE_SERIAL *)GET_GUID_HOB_DATA (Hob);

  return StatusCodeUseSerialHob->StatusCodeUseSerial;
}

/**
  Entry point of Standalone MM Status Code Driver.

  This function is the entry point of Standalone MM Status Code Driver.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI MM System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
StatusCodeHandlerStandaloneMmEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return StatusCodeHandlerCommonEntry ();
}
