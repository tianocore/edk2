/** @file
This file contains URB request, each request is warpped in a
URB (Usb Request Block).

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/



#include "Ohci.h"


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
    DEBUG ((EFI_D_INFO, "STV allocate TD fail !\r\n"));
    return NULL;
  }
  Td->CurrBufferPointer = 0;
  Td->NextTD = 0;
  Td->BufferEndPointer = 0;
  Td->NextTDPointer = 0;

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
    DEBUG ((EFI_D_INFO, "STV allocate ED fail !\r\n"));
    return NULL;
  }
  Ed->Word0.Skip = 1;
  Ed->TdTailPointer = 0;
  Ed->Word2.TdHeadPointer = 0;
  Ed->NextED = 0;

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
  TailTd = (TD_DESCRIPTOR *)(UINTN)(Ed->TdTailPointer);

  Td = HeadTd;
  while (Td != TailTd) {
    TempTd = Td;
    Td = (TD_DESCRIPTOR *)(UINTN)(Td->NextTDPointer);
    OhciFreeTD (Ohc, TempTd);
  }

  return EFI_SUCCESS;
}

/**

  Find a working ED match the requirement

  @Param  EdHead                Head of the ED list
  @Param  DeviceAddress         Device address to search
  @Param  EndPointNum           End point num to search
  @Param  EdDir                 ED Direction to search

  @retval   ED descriptor searched

**/

ED_DESCRIPTOR *
OhciFindWorkingEd (
  IN ED_DESCRIPTOR       *EdHead,
  IN UINT8               DeviceAddress,
  IN UINT8               EndPointNum,
  IN UINT8               EdDir
  )
{
  ED_DESCRIPTOR           *Ed;

  for (Ed = EdHead; Ed != NULL; Ed = (ED_DESCRIPTOR *)(UINTN)(Ed->NextED)) {
    if (Ed->Word2.Halted == 0 && Ed->Word0.Skip == 0 &&
        Ed->Word0.FunctionAddress == DeviceAddress && Ed->Word0.EndPointNum == EndPointNum &&
        Ed->Word0.Direction == EdDir) {
      break;
    }
  }

  return Ed;
}


/**

  Initialize interrupt list.

  @Param Ohc                    Device private data

  @retval  EFI_SUCCESS          Initialization done

**/
EFI_STATUS
OhciInitializeInterruptList (
  USB_OHCI_HC_DEV          *Ohc
  )
{
  static UINT32     Leaf[32] = {0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30, 1, 17,
                                9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31};
  UINT32            *HccaInterruptTable;
  UINTN             Index;
  UINTN             Level;
  UINTN             Count;
  ED_DESCRIPTOR     *NewEd;

  HccaInterruptTable = Ohc->HccaMemoryBlock->HccaInterruptTable;

  for (Index = 0; Index < 32; Index++) {
    NewEd = OhciCreateED (Ohc);
    if (NewEd == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    HccaInterruptTable[Index] = (UINT32)(UINTN)NewEd;
  }

  for (Index = 0; Index < 32; Index++) {
    Ohc->IntervalList[0][Index] = (ED_DESCRIPTOR *)(UINTN)HccaInterruptTable[Leaf[Index]];
  }

  Count = 32;
  for (Level = 1; Level <= 5; Level++) {
    Count = Count >> 1;

    for (Index = 0; Index < Count; Index++) {
      Ohc->IntervalList[Level][Index] = OhciCreateED (Ohc);
      if (HccaInterruptTable[Index] == 0) {
        return EFI_OUT_OF_RESOURCES;
      }
      Ohc->IntervalList[Level - 1][Index * 2    ]->NextED = (UINT32)(UINTN)Ohc->IntervalList[Level][Index];
      Ohc->IntervalList[Level - 1][Index * 2 + 1]->NextED = (UINT32)(UINTN)Ohc->IntervalList[Level][Index];
    }
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

  if (Ed->NextED == 0){
    Ed->NextED = (UINT32)(UINTN)NewEd;
  } else {
    Temp = (ED_DESCRIPTOR *)(UINTN)(Ed->NextED);
    Ed->NextED = (UINT32)(UINTN)NewEd;
    NewEd->NextED = (UINT32)(UINTN)Temp;
  }

  return EFI_SUCCESS;
}


/**

  Count ED number on a ED chain

  @Param  Ed                    Head of the ED chain

  @retval                       ED number on the chain

**/

UINTN
CountEdNum (
  IN ED_DESCRIPTOR      *Ed
  )
{
  UINTN     Count;

  Count = 0;

  while (Ed) {
    Ed = (ED_DESCRIPTOR *)(UINTN)(Ed->NextED);
    Count++;
  }

  return Count;
}

/**

  Find the minimal burn ED list on a specific depth level

  @Param  Ohc                   Device private data
  @Param  Depth                 Depth level

  @retval                       ED list found

**/

ED_DESCRIPTOR *
OhciFindMinInterruptEDList (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Depth
  )
{
  UINTN                   EdNum;
  UINTN                   MinEdNum;
  ED_DESCRIPTOR           *TempEd;
  ED_DESCRIPTOR           *HeadEd;
  UINTN                   Index;

  if (Depth > 5) {
    return NULL;
  }

  MinEdNum = 0xFFFFFFFF;
  TempEd = NULL;
  for (Index = 0; Index < (UINTN)(32 >> Depth); Index++) {
    HeadEd = Ohc->IntervalList[Depth][Index];
    EdNum = CountEdNum (HeadEd);
    if (EdNum < MinEdNum) {
      MinEdNum = EdNum;
      TempEd = HeadEd;
    }
  }

  ASSERT (TempEd != NULL);

  return TempEd;
}


/**

  Attach an ED to an ED list

  @Param  OHC                   UHC private data
  @Param  ListType              Type of the ED list
  @Param  Ed                    ED to attach
  @Param  EdList                ED list to be attached

  @retval  EFI_SUCCESS          ED attached to ED list

**/
ED_DESCRIPTOR *
OhciAttachEDToList (
  IN USB_OHCI_HC_DEV       *Ohc,
  IN DESCRIPTOR_LIST_TYPE  ListType,
  IN ED_DESCRIPTOR         *Ed,
  IN ED_DESCRIPTOR         *EdList
  )
{
  ED_DESCRIPTOR            *HeadEd;

  HeadEd = NULL;
  switch(ListType) {
    case CONTROL_LIST:
      HeadEd = (ED_DESCRIPTOR *) OhciGetMemoryPointer (Ohc, HC_CONTROL_HEAD);
      if (HeadEd == NULL) {
        OhciSetMemoryPointer (Ohc, HC_CONTROL_HEAD, Ed);
        HeadEd = Ed;
      } else {
        OhciAttachED (HeadEd, Ed);
      }
    break;

    case BULK_LIST:
      HeadEd = (ED_DESCRIPTOR *) OhciGetMemoryPointer (Ohc, HC_BULK_HEAD);
      if (HeadEd == NULL) {
        OhciSetMemoryPointer (Ohc, HC_BULK_HEAD, Ed);
        HeadEd = Ed;
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

  return HeadEd;
}

/**

  Remove interrupt EDs that match requirement

  @Param  Ohc                   UHC private data
  @Param  IntEd                 The address of Interrupt endpoint

  @retval  EFI_SUCCESS          EDs match requirement removed

**/

EFI_STATUS
OhciFreeInterruptEdByEd (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN ED_DESCRIPTOR        *IntEd
  )
{
  ED_DESCRIPTOR           *Ed;
  ED_DESCRIPTOR           *TempEd;
  UINTN                   Index;

  if (IntEd == NULL)
    return EFI_SUCCESS;

  for (Index = 0; Index < 32; Index++) {
    Ed = (ED_DESCRIPTOR *)(UINTN)Ohc->HccaMemoryBlock->HccaInterruptTable[Index];
    if (Ed == NULL) {
      continue;
    }
    while (Ed->NextED != 0) {
      if (Ed->NextED == (UINT32)(UINTN)IntEd ) {
        TempEd = (ED_DESCRIPTOR *)(UINTN)(Ed->NextED);
        Ed->NextED = TempEd->NextED;
        OhciFreeED (Ohc, TempEd);
      } else {
        Ed = (ED_DESCRIPTOR *)(UINTN)(Ed->NextED);
      }
    }
  }
  return EFI_SUCCESS;
}

/**

  Remove interrupt EDs that match requirement

  @Param  Ohc                   UHC private data
  @Param  FunctionAddress       Requirement on function address
  @Param  EndPointNum           Requirement on end point number

  @retval  EFI_SUCCESS          EDs match requirement removed

**/
EFI_STATUS
OhciFreeInterruptEdByAddr (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT8                FunctionAddress,
  IN UINT8                EndPointNum
  )
{
  ED_DESCRIPTOR           *Ed;
  ED_DESCRIPTOR           *TempEd;
  UINTN                   Index;

  for (Index = 0; Index < 32; Index++) {
    Ed = (ED_DESCRIPTOR *)(UINTN)Ohc->HccaMemoryBlock->HccaInterruptTable[Index];
    if (Ed == NULL) {
      continue;
    }

    while (Ed->NextED != 0) {
      TempEd = (ED_DESCRIPTOR *)(UINTN)(Ed->NextED);
      if (TempEd->Word0.FunctionAddress == FunctionAddress &&
          TempEd->Word0.EndPointNum     == EndPointNum        ) {
        Ed->NextED = TempEd->NextED;
        OhciFreeED (Ohc, TempEd);
      } else {
        Ed = (ED_DESCRIPTOR *)(UINTN)(Ed->NextED);
      }
    }
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
  while (TempTd->NextTD != 0) {
    TempTd = (TD_DESCRIPTOR *)(UINTN)(TempTd->NextTD);
  }

  TempTd->NextTD = (UINT32)(UINTN)Td2;
  TempTd->NextTDPointer = (UINT32)(UINTN)Td2;

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
    while (TempTd->NextTD != 0) {
      TempTd = (TD_DESCRIPTOR *)(UINTN)(TempTd->NextTD);
    }
    TempTd->NextTD = (UINT32)(UINTN)HeadTd;
    TempTd->NextTDPointer = (UINT32)(UINTN)HeadTd;
  } else {
    Ed->Word2.TdHeadPointer = RIGHT_SHIFT_4 ((UINT32)(UINTN)HeadTd);
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
    Ed->TdTailPointer = Value;
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
    Ed->NextED = Value;
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
      return Ed->TdTailPointer;
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
      return Ed->NextED;
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
    Td->CurrBufferPointer = Value;
  }


  if (Field & TD_NEXT_PTR) {
    Td->NextTD = Value;
  }

  if (Field & TD_BUFFER_END_PTR) {
    Td->BufferEndPointer = Value;
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
      return Td->CurrBufferPointer;
      break;

    case TD_NEXT_PTR:
      return Td->NextTD;
      break;

    case TD_BUFFER_END_PTR:
      return Td->BufferEndPointer;
      break;

    default:
      ASSERT (FALSE);
  }

  return 0;
}

/**

  Free the Ed,Td,buffer that were created during transferring

  @Param  Ohc                   Device private data
**/

VOID
OhciFreeDynamicIntMemory(
  IN USB_OHCI_HC_DEV      *Ohc
  )
{
  INTERRUPT_CONTEXT_ENTRY *Entry;
  if (Ohc != NULL) {
    while (Ohc->InterruptContextList != NULL) {
      Entry = Ohc->InterruptContextList;
      Ohc->InterruptContextList = Ohc->InterruptContextList->NextEntry;
      OhciFreeInterruptEdByEd (Ohc, Entry->Ed);
      OhciFreeInterruptContextEntry (Ohc, Entry);
    }
  }
}
/**

  Free the Ed that were initilized during driver was starting,
  those memory were used as interrupt ED head

  @Param  Ohc                   Device private data


**/
VOID
OhciFreeFixedIntMemory (
  IN USB_OHCI_HC_DEV      *Ohc
  )
{
  static UINT32           Leaf[] = {32,16,8,4,2,1};
  UINTN                   Index;
  UINTN                   Level;

  for (Level = 0; Level < 6; Level++) {
    for (Index = 0; Index < Leaf[Level]; Index++) {
      if (Ohc->IntervalList[Level][Index] != NULL) {
        UsbHcFreeMem(Ohc->MemPool, Ohc->IntervalList[Level][Index], sizeof(ED_DESCRIPTOR));
      }
    }
  }
}
/**

  Release all OHCI used memory when OHCI going to quit

  @Param  Ohc                   Device private data

  @retval EFI_SUCCESS          Memory released

**/

EFI_STATUS
OhciFreeIntTransferMemory (
  IN USB_OHCI_HC_DEV           *Ohc
  )
{
  //
  // Free the Ed,Td,buffer that were created during transferring
  //
  OhciFreeDynamicIntMemory (Ohc);
  //
  // Free the Ed that were initilized during driver was starting
  //
  OhciFreeFixedIntMemory (Ohc);
  return EFI_SUCCESS;
}


