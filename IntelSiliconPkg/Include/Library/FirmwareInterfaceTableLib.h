/** @file
  Firmware Interface Table (FIT) library

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FIRMWARE_INTERFACE_TABLE_LIB_H_
#define _FIRMWARE_INTERFACE_TABLE_LIB_H_

#include <Base.h>
#include <Guid/Fit.h>

/**
  Determines the Firmware Interface Table (FIT) base address.

  @param[out]   FitBase           The FIT base address.

  @retval       EFI_SUCCESS       The operation completed successfully.
  @retval       EFI_NOT_FOUND     The gNemMapStructureHobGuid was not found
  @retval       EFI_NOT_FOUND     The FIT table could not be found (or is no longer available)
  @retval       EFI_DEVICE_ERROR  The GUID in the NEM map structure is invalid.
**/
EFI_STATUS
EFIAPI
GetFitBase (
  OUT UINTN    *FitBase
  );

/**
  Checks if an existing FIT HOB exists before producing a new FIT HOB.

  @param[out]   FitHobStructure             A pointer to a pointer to the FIT_HOB_STRUCTURE. The pointer will be
                                            udpdated to point to a FIT_HOB_STRUCTURE instance.

  @retval       EFI_SUCCESS                 The FIT HOB structure was located and returned successfully.
  @retval       EFI_NOT_FOUND               The gFitStructureHobGuid was not found.
  @retval       EFI_DEVICE_ERROR            The GUID in the NEM map structure is invalid.
**/
EFI_STATUS
EFIAPI
CheckAndProduceFitHob (
  OUT FIT_HOB_STRUCTURE **FitHobStructure
  );

#endif