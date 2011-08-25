/** @file
  Defines the interface to convey performance information from SEC phase to PEI.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_SEC_PERFORMANCE_PPI_H_
#define _PEI_SEC_PERFORMANCE_PPI_H_

#define PEI_SEC_PERFORMANCE_PPI_GUID \
  { \
    0x0ecc666b, 0x4662, 0x47f9, {0x9d, 0xd5, 0xd0, 0x96, 0xff, 0x7d, 0xa4, 0x9e } \
  }

typedef struct _PEI_SEC_PERFORMANCE_PPI PEI_SEC_PERFORMANCE_PPI;

///
/// Performance data collected in SEC phase.
///
typedef struct {
  UINT64         ResetEnd; ///< Timer value logged at the beginning of firmware image execution, in unit of nanosecond.
} FIRMWARE_SEC_PERFORMANCE;

/**
  This interface conveys performance information out of the Security (SEC) phase into PEI.

  This service is published by the SEC phase. The SEC phase handoff has an optional
  EFI_PEI_PPI_DESCRIPTOR list as its final argument when control is passed from SEC into the
  PEI Foundation. As such, if the platform supports collecting performance data in SEC,
  this information is encapsulated into the data structure abstracted by this service.
  This information is collected for the boot-strap processor (BSP) on IA-32.

  @param[in]  PeiServices  The pointer to the PEI Services Table.
  @param[in]  This         The pointer to this instance of the PEI_SEC_PERFORMANCE_PPI.
  @param[out] Performance  The pointer to performance data collected in SEC phase.

  @retval EFI_SUCCESS      The performance data was successfully returned.

**/
typedef
EFI_STATUS
(EFIAPI *GET_SEC_PERFORMANCE) (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN       PEI_SEC_PERFORMANCE_PPI   *This,
  OUT      FIRMWARE_SEC_PERFORMANCE  *Performance
  );

///
/// This PPI provides function to get performance data collected in SEC phase.
///
struct _PEI_SEC_PERFORMANCE_PPI {
  GET_SEC_PERFORMANCE         GetPerformance;
};

extern EFI_GUID gPeiSecPerformancePpiGuid;

#endif
