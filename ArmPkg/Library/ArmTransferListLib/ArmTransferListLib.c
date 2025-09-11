/** @file
  Library that implements the helper functions to parse and pack a Transfer
  List as specified by the A-profile Firmware Handoff Specification.

  Copyright (c) 2022 - 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - https://github.com/FirmwareHandoff/firmware_handoff
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <assert.h>
#include <libtl/include/transfer_list.h>
#include <Library/ArmTransferListLib.h>
#include <Library/HobLib.h>

STATIC_ASSERT (
  sizeof (TRANSFER_LIST_HEADER) == sizeof (struct transfer_list_header),
  "TRANSFER_LIST_HEADER size mismatch"
  );

STATIC_ASSERT (
  sizeof (TRANSFER_ENTRY_HEADER) == sizeof (struct transfer_list_entry),
  "TRANSFER_ENTRY_HEADER size mismatch"
  );

/**
  Get the TransferList from HOB list.

  @param[out] TransferList  TransferList

  @retval EFI_SUCCESS      TransferList is found.
  @retval EFI_NOT_FOUND    TransferList is not found.

**/
EFI_STATUS
EFIAPI
TransferListGetFromHobList (
  OUT TRANSFER_LIST_HEADER  **TransferList
  )
{
  VOID               *HobList;
  EFI_HOB_GUID_TYPE  *GuidHob;
  UINTN              *GuidHobData;

  *TransferList = NULL;

  HobList = GetHobList ();
  if (HobList == NULL) {
    return EFI_NOT_FOUND;
  }

  GuidHob = GetNextGuidHob (&gArmTransferListHobGuid, HobList);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  GuidHobData = GET_GUID_HOB_DATA (GuidHob);

  *TransferList = (TRANSFER_LIST_HEADER *)(*GuidHobData);

  return EFI_SUCCESS;
}

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
  return transfer_list_verify_checksum ((struct transfer_list_header *)TransferListHeader);
}

/**
  This helper maps Transfer List operation modes to corresponding libtl ops.

  @param[in]  ops                     libtl Operation modes.

  @retval TRANSFER_LIST_OPS_ALL       Header is valid and usable (read/write).
  @retval TRANSFER_LIST_OPS_RO        Header is valid but only supports read-only
                                      operations.
  @retval TRANSFER_LIST_OPS_CUSTOM    Header is valid but requires custom handling
                                      due to a version mismatch.
  @retval TRANSFER_LIST_OPS_INVALID   Header is invalid or unsupported.
**/
static TRANSFER_LIST_OPS
MapLibtlOps (
  enum transfer_list_ops  ops
  )
{
  TRANSFER_LIST_OPS  Status;

  switch (ops) {
    case TL_OPS_ALL:
      Status = TRANSFER_LIST_OPS_ALL;
      break;
    case TL_OPS_RO:
      Status = TRANSFER_LIST_OPS_RO;
      break;
    case TL_OPS_CUS:
      Status = TRANSFER_LIST_OPS_CUSTOM;
      break;
    default:
      Status = TRANSFER_LIST_OPS_INVALID;
  }

  return Status;
}

/**
  Check the Transfer List header and return an operation mode.

  This function forwards to transfer_list_check_header() (libtl) and converts
  the returned libtl operation code into the TRANSFER_LIST_OPS enum.

  @param[in]  TransferListHeader      Pointer to the Transfer List header.

  @retval TRANSFER_LIST_OPS_ALL       Header is valid and usable (read/write).
  @retval TRANSFER_LIST_OPS_RO        Header is valid but only supports read-only
                                      operations.
  @retval TRANSFER_LIST_OPS_CUSTOM    Header is valid but requires custom handling
                                      due to a version mismatch.
  @retval TRANSFER_LIST_OPS_INVALID   Header is invalid or unsupported.
**/
TRANSFER_LIST_OPS
EFIAPI
TransferListCheckHeader (
  IN TRANSFER_LIST_HEADER  *TransferListHeader
  )
{
  return MapLibtlOps (transfer_list_check_header ((const struct transfer_list_header *)TransferListHeader));
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
  return (TRANSFER_ENTRY_HEADER *)transfer_list_next ((struct transfer_list_header *)TransferListHeader, NULL);
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
  return (TRANSFER_ENTRY_HEADER *)transfer_list_next ((struct transfer_list_header *)TransferListHeader, (struct transfer_list_entry *)CurrentEntry);
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
  IN UINT32                TagId
  )
{
  return (TRANSFER_ENTRY_HEADER *)transfer_list_find ((struct transfer_list_header *)TransferListHeader, TagId);
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
  IN UINT32                 TagId
  )
{
  TRANSFER_ENTRY_HEADER  *Entry;

  if (CurrentEntry == NULL) {
    return TransferListFindFirstEntry (TransferListHeader, TagId);
  } else {
    Entry = TransferListGetNextEntry (TransferListHeader, CurrentEntry);
  }

  while ((Entry != NULL) && ((Entry->TagId & 0xFFFFFF) != (TagId & 0xFFFFFF))) {
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

  return transfer_list_entry_data ((struct transfer_list_entry *)TransferEntry);
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
  IN UINT32                TagId
  )
{
  TRANSFER_ENTRY_HEADER  *Entry = NULL;

  do {
    Entry = TransferListGetNextEntry (TransferListHeader, Entry);
  } while ((Entry != NULL) && ((Entry->TagId & 0xFFFFFF) != (TagId & 0xFFFFFF)));

  return Entry;
}

/**
  Get TPM event log from TransferList

  @param [in]   TransferListHeader       Pointer to the Transfer List Header
  @param [out]  EventLog                 Pointer to Eventlog in TransferList
  @param [out]  EventLogSize             Size of Event log
  @param [out]  EventLogFlags            Flags for Event log

  @return EFI_SUCCESS
  @return EFI_NOT_FOUND                  No Event log in TransferListHeader
  @return EFI_INVALID_PARAMETER          Invalid parameters

**/
EFI_STATUS
EFIAPI
TransferListGetEventLog (
  IN TRANSFER_LIST_HEADER  *TransferListHeader,
  OUT VOID                 **EventLog,
  OUT UINTN                *EventLogSize,
  OUT UINT32               *EventLogFlags       OPTIONAL
  )
{
  TRANSFER_ENTRY_HEADER   *Entry;
  TRANSFER_LIST_EVENTLOG  *EntryData;

  if ((TransferListHeader == NULL) || (EventLog == NULL) || (EventLogSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *EventLog     = NULL;
  *EventLogSize = 0;

  Entry = TransferListFindFirstEntry (TransferListHeader, TRANSFER_ENTRY_TAG_ID_TPM_EVENT_LOG);
  if ((Entry == NULL) || (Entry->DataSize == 0) ||
      ((Entry->DataSize - OFFSET_OF (TRANSFER_LIST_EVENTLOG, EventLog)) == 0))
  {
    return EFI_NOT_FOUND;
  }

  EntryData = TransferListGetEntryData (Entry);
  if (EventLogFlags != NULL) {
    *EventLogFlags = EntryData->Flags;
  }

  *EventLogSize = Entry->DataSize - OFFSET_OF (TRANSFER_LIST_EVENTLOG, EventLog);
  *EventLog     = (VOID *)&EntryData->EventLog;

  return EFI_SUCCESS;
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
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "TagId     0x%x\n", Entry->TagId));
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "hdr_size   0x%x\n", Entry->HeaderSize));
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "data_size  0x%x\n", Entry->DataSize));
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "data_addr  0x%lx\n", (UINTN)TransferListGetEntryData (Entry)));
  }
}
