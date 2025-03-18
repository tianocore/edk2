/** @file
  Library that implements the helper functions to parse and pack a Transfer
  List as specified by the A-profile Firmware Handoff Specification.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - https://github.com/FirmwareHandoff/firmware_handoff
**/

#include <Base.h>
#include <Library/ArmTransferListLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  Return the first Transfer Entry Node in the Transfer List.

  @param [in]   TransferListHeader     TransferListHeader

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TransferListGetFirstEntry (
  IN TRANSFER_LIST_HEADER  *TransferListHeader
  )
{
  return TransferListGetNextEntry (TransferListHeader, NULL);
}

/**
  Return the next Transfer Entry Node in the Transfer List from
  last Transfer Entry Node.

  @param [in]   TransferListHeader     Pointer to the Transfer List Header.
  @param [in]   CurrentEntry           Pointer to the Current Transfer Entry.
                                       If this is NULL, the first Transfer Entry
                                       is returned.

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TransferListGetNextEntry (
  IN TRANSFER_LIST_HEADER   *TransferListHeader,
  IN TRANSFER_ENTRY_HEADER  *CurrentEntry
  )
{
  TRANSFER_ENTRY_HEADER  *Entry;
  UINTN                  CurrentAddr;
  UINTN                  EndAddr;

  if (TransferListHeader == NULL) {
    return NULL;
  }

  EndAddr = (UINTN)TransferListHeader + TransferListHeader->UsedSize;

  if (CurrentEntry != NULL) {
    CurrentAddr = (UINTN)CurrentEntry + CurrentEntry->HeaderSize + CurrentEntry->DataSize;
  } else {
    CurrentAddr = (UINTN)TransferListHeader + TransferListHeader->HeaderSize;
  }

  CurrentAddr = ALIGN_VALUE (CurrentAddr, (1 << TransferListHeader->Alignment));

  Entry = (TRANSFER_ENTRY_HEADER *)CurrentAddr;

  if (((CurrentAddr + sizeof (TRANSFER_LIST_HEADER)) < CurrentAddr) ||
      ((CurrentAddr + sizeof (TRANSFER_ENTRY_HEADER)) > EndAddr) ||
      ((CurrentAddr + Entry->HeaderSize + Entry->DataSize) < CurrentAddr) ||
      ((CurrentAddr + Entry->HeaderSize + Entry->DataSize) > EndAddr))
  {
    return NULL;
  }

  return Entry;
}

/**
  Return the first Transfer Entry Node in the Transfer List
  matched with given tag-id.

  @param [in]   TransferListHeader     Pointer to the Transfer List Header.
  @param [in]   TagId                  Tag id

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TransferListFindFirstEntry (
  IN TRANSFER_LIST_HEADER  *TransferListHeader,
  IN UINT16                TagId
  )
{
  TRANSFER_ENTRY_HEADER  *Entry;

  Entry = TransferListGetFirstEntry (TransferListHeader);

  while ((Entry != NULL) && ((Entry->TagId != TagId) || Entry->Reserved0 != 0)) {
    Entry = TransferListGetNextEntry (TransferListHeader, Entry);
  }

  return Entry;
}

/**
  Return the Next Transfer Entry Node in the Transfer List
  matched with given tag-id from last Transfer Entry Node.

  @param [in]   TransferListHeader     Pointer to the Transfer List Header.
  @param [in]   CurrentEntry           Pointer to the Current Transfer Entry.
                                       If this is NULL, the first Transfer Entry
                                       is returned.
  @param [in]   TagId                  Tag id

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TransferListFindNextEntry (
  IN TRANSFER_LIST_HEADER   *TransferListHeader,
  IN TRANSFER_ENTRY_HEADER  *CurrentEntry,
  IN UINT16                 TagId
  )
{
  TRANSFER_ENTRY_HEADER  *Entry;

  if (CurrentEntry == NULL) {
    return TransferListFindFirstEntry (TransferListHeader, TagId);
  } else {
    Entry = TransferListGetNextEntry (TransferListHeader, CurrentEntry);
  }

  while ((Entry != NULL) && ((Entry->TagId != TagId) || Entry->Reserved0 != 0)) {
    Entry = TransferListGetNextEntry (TransferListHeader, Entry);
  }

  return Entry;
}

/**
  Return the data in Transfer Entry.

  @param [in]   TransferEntry          Pointer to a Transfer Entry Header

  @return Pointer to the Data of Transfer Entry Node if successful otherwise NULL

**/
VOID *
EFIAPI
TransferListGetEntryData (
  IN TRANSFER_ENTRY_HEADER  *TransferEntry
  )
{
  if ((TransferEntry == NULL) || (TransferEntry->DataSize == 0)) {
    return NULL;
  }

  return (VOID *)((UINTN)TransferEntry + TransferEntry->HeaderSize);
}
