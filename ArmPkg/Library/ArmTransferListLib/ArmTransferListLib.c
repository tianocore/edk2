/** @file
  Library that implements the helper functions to parse and pack a Transfer
  List as specified by the A-profile Firmware Handoff Specification.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - https://github.com/FirmwareHandoff/firmware_handoff

  @par Glossary:
    - TL - Transfer list
    - TE - Transfer entry
    - Tlh - Transfer list header
**/

#include <Base.h>
#include <Library/ArmTransferListLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  Return the first Transfer Entry Node in the Transfer List.

  @param [in]   Tlh       TransferListHeader

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TlGetFirstEntry (
  IN TRANSFER_LIST_HEADER  *Tlh
  )
{
  return TlGetNextEntry (Tlh, NULL);
}

/**
  Return the next Transfer Entry Node in the Transfer List from
  last Transfer Entry Node.

  @param [in]   Tlh       TransferListHeader
  @param [in]   LastTe    NULL or Last searched Transfer Entry

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TlGetNextEntry (
  IN TRANSFER_LIST_HEADER   *Tlh,
  IN TRANSFER_ENTRY_HEADER  *LastTe
  )
{
  TRANSFER_ENTRY_HEADER  *Te;
  UINTN                  CurrentAddr;
  UINTN                  EndAddr;

  if (Tlh == NULL) {
    return NULL;
  }

  EndAddr = (UINTN)Tlh + Tlh->UsedSize;

  if (LastTe != NULL) {
    CurrentAddr = (UINTN)LastTe + LastTe->HeaderSize + LastTe->DataSize;
  } else {
    CurrentAddr = (UINTN)Tlh + Tlh->HeaderSize;
  }

  CurrentAddr = ALIGN_VALUE (CurrentAddr, (1 << Tlh->Alignment));

  Te = (TRANSFER_ENTRY_HEADER *)CurrentAddr;

  if ((CurrentAddr + sizeof (TRANSFER_LIST_HEADER) < CurrentAddr) ||
      (CurrentAddr + sizeof (TRANSFER_ENTRY_HEADER) > EndAddr) ||
      (CurrentAddr + Te->HeaderSize + Te->DataSize < CurrentAddr) ||
      (CurrentAddr + Te->HeaderSize + Te->DataSize > EndAddr))
  {
    return NULL;
  }

  return Te;
}

/**
  Return the first Transfer Entry Node in the Transfer List
  matched with given tag-id.

  @param [in]   Tlh       TransferListHeader
  @param [in]   TagId     Tag id

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TlFindFirstEntry (
  IN TRANSFER_LIST_HEADER  *Tlh,
  IN UINT16                TagId
  )
{
  TRANSFER_ENTRY_HEADER  *Te;

  Te = TlGetFirstEntry (Tlh);

  while (Te && ((Te->TagId != TagId) || Te->Reserved0 != 0)) {
    Te = TlGetNextEntry (Tlh, Te);
  }

  return Te;
}

/**
  Return the Next Transfer Entry Node in the Transfer List
  matched with given tag-id from last Transfer Entry Node.

  @param [in]   Tlh       TransferListHeader
  @param [in]   LastTe    NULL or Last searched Transfer Entry
  @param [in]   TagId     Tag id

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TlFindNextEntry (
  IN TRANSFER_LIST_HEADER   *Tlh,
  IN TRANSFER_ENTRY_HEADER  *LastTe,
  IN UINT16                 TagId
  )
{
  TRANSFER_ENTRY_HEADER  *Te;

  if (LastTe == NULL) {
    return TlFindFirstEntry (Tlh, TagId);
  } else {
    Te = TlGetNextEntry (Tlh, LastTe);
  }

  while (Te && ((Te->TagId != TagId) || Te->Reserved0 != 0)) {
    Te = TlGetNextEntry (Tlh, Te);
  }

  return Te;
}

/**
  Return the data in Transfer Entry.

  @param [in]   Te       TransferEntryHeader

  @return Pointer to the Data of Transfer Entry Node if successful otherwise NULL

**/
VOID *
EFIAPI
TlGetEntryData (
  IN TRANSFER_ENTRY_HEADER  *Te
  )
{
  if ((Te == NULL) || (Te->DataSize == 0)) {
    return NULL;
  }

  return (VOID *)((UINTN)Te + Te->HeaderSize);
}
