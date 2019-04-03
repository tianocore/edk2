/** @file
This file contains the definination for host controller schedule routines.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/



#ifndef _OHCI_SCHED_H
#define _OHCI_SCHED_H

#include "Descriptor.h"

#define HCCA_MEM_SIZE     256
#define GRID_SIZE         16
#define GRID_SHIFT        4

/**

  Convert Error code from OHCI format to EFI format

  @Param  ErrorCode             ErrorCode in OHCI format

  @retval                       ErrorCode in EFI format

**/
UINT32
ConvertErrorCode (
  IN  UINT32              ErrorCode
  );
/**

  Check TDs Results

  @Param  Ohc                   UHC private data
  @Param  Td                    TD_DESCRIPTOR
  @Param  Result                Result to return

  @retval TRUE                  means OK
  @retval FLASE                 means Error or Short packet

**/
BOOLEAN
OhciCheckTDsResults (
  IN  USB_OHCI_HC_DEV     *Ohc,
  IN  TD_DESCRIPTOR       *Td,
  OUT UINT32              *Result
  );
/**

  Check the task status on an ED

  @Param  Ed                    Pointer to the ED task that TD hooked on
  @Param  HeadTd                TD header for current transaction

  @retval                       Task Status Code

**/

UINT32
CheckEDStatus (
  IN  ED_DESCRIPTOR       *Ed,
  IN  TD_DESCRIPTOR       *HeadTd
  );
/**

  Check the task status

  @Param  Ohc                   UHC private data
  @Param  ListType              Pipe type
  @Param  Ed                    Pointer to the ED task hooked on
  @Param  HeadTd                Head of TD corresponding to the task
  @Param  ErrorCode             return the ErrorCode

  @retval  EFI_SUCCESS          Task done
  @retval  EFI_NOT_READY        Task on processing
  @retval  EFI_DEVICE_ERROR     Some error occured

**/
EFI_STATUS
CheckIfDone (
  IN  USB_OHCI_HC_DEV       *Ohc,
  IN  DESCRIPTOR_LIST_TYPE  ListType,
  IN  ED_DESCRIPTOR         *Ed,
  IN  TD_DESCRIPTOR         *HeadTd,
  OUT UINT32                *ErrorCode
  );
/**

  Convert TD condition code to Efi Status

  @Param  ConditionCode         Condition code to convert

  @retval  EFI_SUCCESS          No error occured
  @retval  EFI_NOT_READY        TD still on processing
  @retval  EFI_DEVICE_ERROR     Error occured in processing TD

**/

EFI_STATUS
OhciTDConditionCodeToStatus (
  IN  UINT32              ConditionCode
  );

#endif
