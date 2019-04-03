/** @file
This file contains URB request, each request is warpped in a
URB (Usb Request Block).

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/



#include "OhcPeim.h"


/**

  Create a TD

  @Param  Ohc                   UHC private data

  @retval                       TD structure pointer

**/
TD_DESCRIPTOR *
OhciCreateTD (
  IN USB_OHCI_HC_DEV      *Ohc
  )
{
  TD_DESCRIPTOR           *Td;

  Td = UsbHcAllocateMem(Ohc->MemPool, sizeof(TD_DESCRIPTOR));
  if (Td == NULL) {
    return NULL;
  }
  Td->CurrBufferPointer = NULL;
  Td->NextTD = NULL;
  Td->BufferEndPointer = NULL;
  Td->NextTDPointer = NULL;

  return Td;
}


/**

  Free a TD

  @Param  Ohc                   UHC private data
  @Param  Td                    Pointer to a TD to free

  @retval  EFI_SUCCESS          TD freed

**/
EFI_STATUS
OhciFreeTD (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN TD_DESCRIPTOR        *Td
  )
{
  if (Td == NULL) {
    return EFI_SUCCESS;
  }
  UsbHcFreeMem(Ohc->MemPool, Td, sizeof(TD_DESCRIPTOR));

  return EFI_SUCCESS;
}


/**

  Create a ED

  @Param   Ohc                  Device private data

  @retval  ED                   descriptor pointer

**/
ED_DESCRIPTOR *
OhciCreateED (
  USB_OHCI_HC_DEV          *Ohc
  )
{
  ED_DESCRIPTOR   *Ed;
  Ed = UsbHcAllocateMem(Ohc->MemPool, sizeof (ED_DESCRIPTOR));
  if (Ed == NULL) {
    return NULL;
  }
  Ed->Word0.Skip = 1;
  Ed->TdTailPointer = NULL;
  Ed->Word2.TdHeadPointer = RIGHT_SHIFT_4 ((UINT32) NULL);
  Ed->NextED = NULL;

  return Ed;
}

/**

  Free a ED

  @Param  Ohc                   UHC private data
  @Param  Ed                    Pointer to a ED to free

  @retval  EFI_SUCCESS          ED freed

**/

EFI_STATUS
OhciFreeED (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN ED_DESCRIPTOR        *Ed
  )
{
  if (Ed == NULL) {
    return EFI_SUCCESS;
  }
  UsbHcFreeMem(Ohc->MemPool, Ed, sizeof(ED_DESCRIPTOR));

  return EFI_SUCCESS;
}

/**

  Free  ED

  @Param  Ohc                    Device private data
  @Param  Ed                     Pointer to a ED to free

  @retval  EFI_SUCCESS           ED freed

**/
EFI_STATUS
OhciFreeAllTDFromED (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN ED_DESCRIPTOR        *Ed
  )
{
  TD_DESCRIPTOR           *HeadTd;
  TD_DESCRIPTOR           *TailTd;
  TD_DESCRIPTOR           *Td;
  TD_DESCRIPTOR           *TempTd;

  if (Ed == NULL) {
    return EFI_SUCCESS;
  }

  HeadTd = TD_PTR (Ed->Word2.TdHeadPointer);
  TailTd = Ed->TdTailPointer;

  Td = HeadTd;
  while (Td != TailTd) {
    TempTd = Td;
    Td = Td->NextTDPointer;
    OhciFreeTD (Ohc, TempTd);
  }

  return EFI_SUCCESS;
}

/**

  Attach an ED

  @Param  Ed                    Ed to be attached
  @Param  NewEd                 Ed to attach

  @retval EFI_SUCCESS           NewEd attached to Ed
  @retval EFI_INVALID_PARAMETER Ed is NULL

**/
EFI_STATUS
OhciAttachED (
  IN ED_DESCRIPTOR        *Ed,
  IN ED_DESCRIPTOR        *NewEd
  )
{
  ED_DESCRIPTOR           *Temp;

  if (Ed == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Ed->NextED == NULL){
    Ed->NextED = NewEd;
  } else {
    Temp = Ed->NextED;
    Ed->NextED = NewEd;
    NewEd->NextED = Temp;
  }

  return EFI_SUCCESS;
}
/**

  Attach an ED to an ED list

  @Param  OHC                   UHC private data
  @Param  ListType              Type of the ED list
  @Param  Ed                    ED to attach
  @Param  EdList                ED list to be attached

  @retval  EFI_SUCCESS          ED attached to ED list

**/
EFI_STATUS
OhciAttachEDToList (
  IN USB_OHCI_HC_DEV       *Ohc,
  IN DESCRIPTOR_LIST_TYPE  ListType,
  IN ED_DESCRIPTOR         *Ed,
  IN ED_DESCRIPTOR         *EdList
  )
{
  ED_DESCRIPTOR            *HeadEd;

  switch(ListType) {
    case CONTROL_LIST:
      HeadEd = (ED_DESCRIPTOR *) OhciGetMemoryPointer (Ohc, HC_CONTROL_HEAD);
      if (HeadEd == NULL) {
        OhciSetMemoryPointer (Ohc, HC_CONTROL_HEAD, Ed);
      } else {
        OhciAttachED (HeadEd, Ed);
      }
    break;

    case BULK_LIST:
      HeadEd = (ED_DESCRIPTOR *) OhciGetMemoryPointer (Ohc, HC_BULK_HEAD);
      if (HeadEd == NULL) {
        OhciSetMemoryPointer (Ohc, HC_BULK_HEAD, Ed);
      } else {
        OhciAttachED (HeadEd, Ed);
      }
    break;

    case INTERRUPT_LIST:
      OhciAttachED (EdList, Ed);
      break;

    default:
      ASSERT (FALSE);
  }

  return EFI_SUCCESS;
}
/**

  Link Td2 to the end of Td1

  @Param Td1                    TD to be linked
  @Param Td2                    TD to link

  @retval EFI_SUCCESS           TD successfully linked
  @retval EFI_INVALID_PARAMETER Td1 is NULL

**/
EFI_STATUS
OhciLinkTD (
  IN TD_DESCRIPTOR        *Td1,
  IN TD_DESCRIPTOR        *Td2
  )
{
  TD_DESCRIPTOR           *TempTd;

  if (Td1 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Td1 == Td2) {
    return EFI_SUCCESS;
  }

  TempTd = Td1;
  while (TempTd->NextTD != NULL) {
    TempTd = TempTd->NextTD;
  }

  TempTd->NextTD = Td2;
  TempTd->NextTDPointer = Td2;

  return EFI_SUCCESS;
}


/**

  Attach TD list to ED

  @Param  Ed                    ED which TD list attach on
  @Param  HeadTd                Head of the TD list to attach

  @retval  EFI_SUCCESS          TD list attached on the ED

**/
EFI_STATUS
OhciAttachTDListToED (
  IN ED_DESCRIPTOR        *Ed,
  IN TD_DESCRIPTOR        *HeadTd
  )
{
  TD_DESCRIPTOR           *TempTd;

  TempTd = TD_PTR (Ed->Word2.TdHeadPointer);

  if (TempTd != NULL) {
    while (TempTd->NextTD != NULL) {
      TempTd = TempTd->NextTD;
    }
    TempTd->NextTD = HeadTd;
    TempTd->NextTDPointer = HeadTd;
  } else {
    Ed->Word2.TdHeadPointer = RIGHT_SHIFT_4 ((UINT32) HeadTd);
  }

  return EFI_SUCCESS;
}


/**

  Set value to ED specific field

  @Param  Ed                    ED to be set
  @Param  Field                 Field to be set
  @Param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/
EFI_STATUS
OhciSetEDField (
  IN ED_DESCRIPTOR        *Ed,
  IN UINT32               Field,
  IN UINT32               Value
  )
{
  if (Field & ED_FUNC_ADD) {
    Ed->Word0.FunctionAddress = Value;
  }
  if (Field & ED_ENDPT_NUM) {
    Ed->Word0.EndPointNum = Value;
  }
  if (Field & ED_DIR) {
    Ed->Word0.Direction = Value;
  }
  if (Field & ED_SPEED) {
    Ed->Word0.Speed = Value;
  }
  if (Field & ED_SKIP) {
    Ed->Word0.Skip = Value;
  }
  if (Field & ED_FORMAT) {
    Ed->Word0.Format = Value;
  }
  if (Field & ED_MAX_PACKET) {
    Ed->Word0.MaxPacketSize = Value;
  }
  if (Field & ED_PDATA) {
    Ed->Word0.FreeSpace = Value;
  }
  if (Field & ED_ZERO) {
    Ed->Word2.Zero = Value;
  }
  if (Field & ED_TDTAIL_PTR) {
    Ed->TdTailPointer = (VOID *) Value;
  }

  if (Field & ED_HALTED) {
    Ed->Word2.Halted = Value;
  }
  if (Field & ED_DTTOGGLE) {
    Ed->Word2.ToggleCarry = Value;
  }
  if (Field & ED_TDHEAD_PTR) {
    Ed->Word2.TdHeadPointer = RIGHT_SHIFT_4 (Value);
  }

  if (Field & ED_NEXT_EDPTR) {
    Ed->NextED = (VOID *) Value;
  }

  return EFI_SUCCESS;
}

/**

  Get value from an ED's specific field

  @Param  Ed                    ED pointer
  @Param  Field                 Field to get value from

  @retval                       Value of the field

**/
UINT32
OhciGetEDField (
  IN ED_DESCRIPTOR        *Ed,
  IN UINT32               Field
  )
{
  switch (Field) {
    case ED_FUNC_ADD:
      return Ed->Word0.FunctionAddress;
      break;
    case ED_ENDPT_NUM:
      return Ed->Word0.EndPointNum;
      break;
    case ED_DIR:
      return Ed->Word0.Direction;
      break;
    case ED_SPEED:
      return Ed->Word0.Speed;
      break;
    case ED_SKIP:
      return Ed->Word0.Skip;
      break;
    case ED_FORMAT:
      return Ed->Word0.Format;
      break;
    case ED_MAX_PACKET:
      return Ed->Word0.MaxPacketSize;
      break;

    case ED_TDTAIL_PTR:
      return (UINT32) Ed->TdTailPointer;
      break;

    case ED_HALTED:
      return Ed->Word2.Halted;
      break;

    case ED_DTTOGGLE:
      return Ed->Word2.ToggleCarry;
      break;

    case ED_TDHEAD_PTR:
      return Ed->Word2.TdHeadPointer << 4;
      break;

    case ED_NEXT_EDPTR:
      return (UINT32) Ed->NextED;
      break;

    default:
      ASSERT (FALSE);
  }

  return 0;
}


/**

  Set value to TD specific field

  @Param  Td                    TD to be set
  @Param  Field                 Field to be set
  @Param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/
EFI_STATUS
OhciSetTDField (
  IN TD_DESCRIPTOR        *Td,
  IN UINT32               Field,
  IN UINT32               Value
  )
{
  if (Field & TD_PDATA) {
    Td->Word0.Reserved = Value;
  }
  if (Field & TD_BUFFER_ROUND) {
    Td->Word0.BufferRounding = Value;
  }
  if (Field & TD_DIR_PID) {
    Td->Word0.DirPID = Value;
  }
  if (Field & TD_DELAY_INT) {
    Td->Word0.DelayInterrupt = Value;
  }
  if (Field & TD_DT_TOGGLE) {
    Td->Word0.DataToggle = Value | 0x2;
  }
  if (Field & TD_ERROR_CNT) {
    Td->Word0.ErrorCount = Value;
  }
  if (Field & TD_COND_CODE) {
    Td->Word0.ConditionCode = Value;
  }

  if (Field & TD_CURR_BUFFER_PTR) {
    Td->CurrBufferPointer = (VOID *) Value;
  }


  if (Field & TD_NEXT_PTR) {
    Td->NextTD = (VOID *) Value;
  }

  if (Field & TD_BUFFER_END_PTR) {
    Td->BufferEndPointer = (VOID *) Value;
  }

  return EFI_SUCCESS;
}


/**

  Get value from ED specific field

  @Param  Td                    TD pointer
  @Param  Field                 Field to get value from

  @retval                       Value of the field

**/

UINT32
OhciGetTDField (
  IN TD_DESCRIPTOR      *Td,
  IN UINT32             Field
  )
{
  switch (Field){
    case TD_BUFFER_ROUND:
      return Td->Word0.BufferRounding;
      break;
    case TD_DIR_PID:
      return Td->Word0.DirPID;
      break;
    case TD_DELAY_INT:
      return Td->Word0.DelayInterrupt;
      break;
    case TD_DT_TOGGLE:
      return Td->Word0.DataToggle;
      break;
    case TD_ERROR_CNT:
      return Td->Word0.ErrorCount;
      break;
    case TD_COND_CODE:
      return Td->Word0.ConditionCode;
      break;
    case TD_CURR_BUFFER_PTR:
      return (UINT32) Td->CurrBufferPointer;
      break;

    case TD_NEXT_PTR:
      return (UINT32) Td->NextTD;
      break;

    case TD_BUFFER_END_PTR:
      return (UINT32) Td->BufferEndPointer;
      break;

    default:
      ASSERT (FALSE);
  }

  return 0;
}
