/** @file
  Library that implements the helper functions to parse and pack a Transfer
  List as specified by the A-profile Firmware Handoff Specification.

  Copyright (c) 2022 - 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - https://github.com/FirmwareHandoff/firmware_handoff
**/

#include <Base.h>
#include <Library/ArmTransferListLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  This function verifies the checksum of the Transfer List.

  @param [in]   TransferListHeader       Pointer to the Transfer List Header

  @retval FALSE      Invalid Checksum
  @retval TRUE       Valid Checksum

**/
BOOLEAN
EFIAPI
TransferListVerifyChecksum (
  IN TRANSFER_LIST_HEADER  *TransferListHeader
  )
{
  if (TransferListHeader == NULL) {
    return FALSE;
  }

  if ((TransferListHeader->Flags & TRANSFER_LIST_FL_HAS_CHECKSUM) == 0) {
    return TRUE;
  }

  return (CalculateSum8 ((UINT8 *)TransferListHeader, TransferListHeader->UsedSize) == 0);
}

/**
  This function checks the header of the Transfer List.

  @param [in]   TransferListHeader       Pointer to the Transfer List Header

  @return TRANSFER_LIST_OPS code indicating the validity of the Transfer List

**/
TRANSFER_LIST_OPS
EFIAPI
TransferListCheckHeader (
  IN TRANSFER_LIST_HEADER  *TransferListHeader
  )
{
  if (TransferListHeader == NULL) {
    return TRANSFER_LIST_OPS_INVALID;
  }

  if (TransferListHeader->Signature != TRANSFER_LIST_SIGNATURE_64) {
    DEBUG ((DEBUG_ERROR, "Bad transfer list signature 0x%x\n", TransferListHeader->Signature));
    return TRANSFER_LIST_OPS_INVALID;
  }

  if (TransferListHeader->TotalSize == 0) {
    DEBUG ((DEBUG_ERROR, "Bad transfer list total size 0x%x\n", TransferListHeader->TotalSize));
    return TRANSFER_LIST_OPS_INVALID;
  }

  if (TransferListHeader->UsedSize > TransferListHeader->TotalSize) {
    DEBUG ((DEBUG_ERROR, "Bad transfer list used size 0x%x\n", TransferListHeader->UsedSize));
    return TRANSFER_LIST_OPS_INVALID;
  }

  if (TransferListHeader->HeaderSize != sizeof (TRANSFER_LIST_HEADER)) {
    DEBUG ((DEBUG_ERROR, "Bad transfer list header size 0x%x\n", TransferListHeader->HeaderSize));
    return TRANSFER_LIST_OPS_INVALID;
  }

  if (TransferListVerifyChecksum (TransferListHeader) == FALSE) {
    DEBUG ((DEBUG_ERROR, "Bad transfer list checksum 0x%x\n", TransferListHeader->Checksum));
    return TRANSFER_LIST_OPS_INVALID;
  }

  if (TransferListHeader->Version == 0) {
    DEBUG ((DEBUG_ERROR, "Transfer list version is invalid\n"));
    return TRANSFER_LIST_OPS_INVALID;
  } else if (TransferListHeader->Version == ARM_FW_HANDOFF_PROTOCOL_VERSION) {
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Transfer list version is valid for all operations\n"));
    return TRANSFER_LIST_OPS_ALL;
  } else if (TransferListHeader->Version > ARM_FW_HANDOFF_PROTOCOL_VERSION) {
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Transfer list version is valid for read-only\n"));
    return TRANSFER_LIST_OPS_RO;
  }

  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Old or custom transfer list version is detected\n"));
  return TRANSFER_LIST_OPS_CUSTOM;
}

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

/**
  Find a Transfer Entry Node in the Transfer List matched with the given tag-id.

  @param [in]   TransferListHeader       Pointer to the Transfer List Header
  @param [in]   TagId     Tag id

  @return Pointer to the Transfer Entry Node if successful otherwise NULL
**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TransferListFindEntry (
  IN TRANSFER_LIST_HEADER  *TransferListHeader,
  IN UINT16                TagId
  )
{
  TRANSFER_ENTRY_HEADER  *Entry = NULL;

  do {
    Entry = TransferListGetNextEntry (TransferListHeader, Entry);
  } while ((Entry != NULL) && (Entry->TagId != TagId));

  return Entry;
}

/**
  Dump the transfer list to the debug output.

  @param [in]   TransferListHeader       Pointer to the Transfer List Header

**/
VOID
EFIAPI
TransferListDump (
  IN TRANSFER_LIST_HEADER  *TransferListHeader
  )
{
  TRANSFER_ENTRY_HEADER  *Entry;
  UINTN                  Idx;

  Entry = NULL;
  Idx   = 0;

  if (TransferListHeader == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Dump transfer list:\n"));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "signature  0x%x\n", TransferListHeader->Signature));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "checksum   0x%x\n", TransferListHeader->Checksum));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "version    0x%x\n", TransferListHeader->Version));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "hdr_size   0x%x\n", TransferListHeader->HeaderSize));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "alignment  0x%x\n", TransferListHeader->Alignment));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "used_size  0x%x\n", TransferListHeader->UsedSize));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "total_size 0x%x\n", TransferListHeader->TotalSize));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "flags      0x%x\n", TransferListHeader->Flags));

  while (TRUE) {
    Entry = TransferListGetNextEntry (TransferListHeader, Entry);
    if (Entry == NULL) {
      break;
    }

    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Entry %d:\n", Idx++));
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "tag_id     0x%x\n", Entry->TagId));
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "hdr_size   0x%x\n", Entry->HeaderSize));
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "data_size  0x%x\n", Entry->DataSize));
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "data_addr  0x%lx\n", (UINTN)TransferListGetEntryData (Entry)));
  }
}
