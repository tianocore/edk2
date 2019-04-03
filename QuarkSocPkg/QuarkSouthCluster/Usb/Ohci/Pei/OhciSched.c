/** @file
OHCI transfer scheduling routines.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "OhcPeim.h"

/**

  Convert Error code from OHCI format to EFI format

  @Param  ErrorCode             ErrorCode in OHCI format

  @retval                       ErrorCode in EFI format

**/
UINT32
ConvertErrorCode (
  IN  UINT32              ErrorCode
  )
{
  UINT32                  TransferResult;

  switch (ErrorCode) {
    case TD_NO_ERROR:
      TransferResult = EFI_USB_NOERROR;
      break;

    case TD_TOBE_PROCESSED:
    case TD_TOBE_PROCESSED_2:
      TransferResult = EFI_USB_ERR_NOTEXECUTE;
      break;

    case TD_DEVICE_STALL:
      TransferResult = EFI_USB_ERR_STALL;
      break;

    case TD_BUFFER_OVERRUN:
    case TD_BUFFER_UNDERRUN:
      TransferResult = EFI_USB_ERR_BUFFER;
      break;

    case TD_CRC_ERROR:
      TransferResult = EFI_USB_ERR_CRC;
      break;

    case TD_NO_RESPONSE:
      TransferResult = EFI_USB_ERR_TIMEOUT;
      break;

    case TD_BITSTUFFING_ERROR:
      TransferResult = EFI_USB_ERR_BITSTUFF;
      break;

    default:
      TransferResult = EFI_USB_ERR_SYSTEM;
  }

  return TransferResult;
}


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
  )
{
  UINT32                  TdCompletionCode;

  *Result   = EFI_USB_NOERROR;

  while (Td) {
    TdCompletionCode = Td->Word0.ConditionCode;

    *Result |= ConvertErrorCode(TdCompletionCode);
    //
    // if any error encountered, stop processing the left TDs.
    //
    if (*Result) {
      return FALSE;
    }

    Td = Td->NextTDPointer;
  }
  return TRUE;

}


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
  )
{
  while(HeadTd != NULL) {
    if (HeadTd->Word0.ConditionCode != 0) {
      return HeadTd->Word0.ConditionCode;
    }
    HeadTd = HeadTd->NextTDPointer;
  }

  if (OhciGetEDField (Ed, ED_TDHEAD_PTR) != OhciGetEDField (Ed, ED_TDTAIL_PTR)) {
    return TD_TOBE_PROCESSED;
  }

  return TD_NO_ERROR;
}

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
  )
{
  *ErrorCode = TD_TOBE_PROCESSED;

  switch (ListType) {
    case CONTROL_LIST:
      if (OhciGetHcCommandStatus (Ohc, CONTROL_LIST_FILLED) != 0) {
        return EFI_NOT_READY;
      }
      break;

    case BULK_LIST:
      if (OhciGetHcCommandStatus (Ohc, BULK_LIST_FILLED) != 0) {
        return EFI_NOT_READY;
      }
      break;

    default:
      break;
  }

  *ErrorCode = CheckEDStatus (Ed, HeadTd);


  if (*ErrorCode == TD_NO_ERROR) {
    return EFI_SUCCESS;
  } else if (*ErrorCode == TD_TOBE_PROCESSED) {
    return EFI_NOT_READY;
  } else {
    return EFI_DEVICE_ERROR;
  }
}


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
  )
{
  if (ConditionCode == TD_NO_ERROR) {
    return EFI_SUCCESS;
  }

  if (ConditionCode == TD_TOBE_PROCESSED) {
    return EFI_NOT_READY;
  }

  return EFI_DEVICE_ERROR;
}

