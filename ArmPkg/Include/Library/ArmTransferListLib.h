/** @file
  Library that implements the helper functions to parse and pack a Transfer
  List as specified by the A-profile Firmware Handoff Specification.

  Copyright (c) 2022 - 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - https://github.com/FirmwareHandoff/firmware_handoff
**/

#ifndef ARM_TRANSFER_LIST_LIB_
#define ARM_TRANSFER_LIST_LIB_

#include <Base.h>
#include <Uefi.h>
#include <IndustryStandard/ArmTransferList.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

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
  );

/**
  Return the first Transfer Entry Node in the Transfer List.

  @param [in]   TransferListHeader     Pointer to the Transfer List Header.

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TransferListGetFirstEntry (
  IN TRANSFER_LIST_HEADER  *TransferListHeader
  );

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
  );

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
  );

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
  );

/**
  Return the data in Transfer Entry.

  @param [in]   TransferEntry          Pointer to a Transfer Entry Header

  @return Pointer to the Data of Transfer Entry Node if successful otherwise NULL

**/
VOID *
EFIAPI
TransferListGetEntryData (
  IN TRANSFER_ENTRY_HEADER  *TransferEntry
  );

/**
  Dump the transfer list to the debug output.

  @param [in]   TransferListHeader       Pointer to the Transfer List Header

**/
VOID
EFIAPI
TransferListDump (
  IN TRANSFER_LIST_HEADER  *TransferListHeader
  );

/**
  Verify the checksum of the transfer list.

  @param [in]   TransferListHeader       Pointer to the Transfer List Header

  @retval FALSE      Invalid Checksum
  @retval TRUE       Valid Checksum
**/
BOOLEAN
EFIAPI
TransferListVerifyChecksum (
  IN TRANSFER_LIST_HEADER  *TransferListHeader
  );

/**
  Check the header of the Transfer List.

  @param [in]   TransferListHeader       Pointer to the Transfer List Header

  @return TRANSFER_LIST_OPS code indicating the validity of the Transfer List
**/
TRANSFER_LIST_OPS
EFIAPI
TransferListCheckHeader (
  IN TRANSFER_LIST_HEADER  *TransferListHeader
  );

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
  );

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
  );

/**
  Ensure a valid Transfer List exists at the specified buffer.

  If a valid header is present it is returned; otherwise a new empty Transfer
  List is initialized in-place using the provided capacity.

  @param[in,out]  TransferListBase      Base address of the buffer/region.
  @param[in]      TransferListCapacity  Total capacity (bytes) of the buffer.

  @return Pointer to the Transfer List header on success; NULL on failure.
**/
TRANSFER_LIST_HEADER *
EFIAPI
TransferListEnsure (
  IN OUT VOID   *TransferListBase,
  IN     UINTN  TransferListCapacity
  );

/**
  Relocate an existing Transfer List to a new buffer with the given capacity.

  Useful when the Transfer List must grow beyond its current region (e.g., when
  backed by a fixed flash window).

  @param[in]   TransferListHeader    Pointer to the current Transfer List header.
  @param[out]  DestinationBase       Destination buffer address.
  @param[in]   DestinationCapacity   Capacity in bytes of the destination buffer.

  @return Pointer to the relocated Transfer List header; NULL on failure.
**/
TRANSFER_LIST_HEADER *
EFIAPI
TransferListRelocate (
  IN  TRANSFER_LIST_HEADER  *TransferListHeader,
  OUT VOID                  *DestinationBase,
  IN  UINTN                 DestinationCapacity
  );

/**
  Append a new Transfer Entry.

  @param[in,out]  TransferListHeader  Pointer to the Transfer List header.
  @param[in]      TagId               Full (up to 24-bit) tag identifier.
  @param[in]      DataSize            Payload size in bytes.
  @param[in]      Data                Optional source buffer (may be NULL).

  @retval NULL      Append failed (e.g., insufficient capacity).
  @retval non-NULL  Pointer to the newly added entry.
**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TransferListAdd (
  IN OUT TRANSFER_LIST_HEADER  *TransferListHeader,
  IN     UINT32                TagId,
  IN     UINT32                DataSize,
  IN     CONST VOID            *Data   OPTIONAL
  );

/**
  Resize an existing Transfer Entry payload.

  On growth, subsequent entries may be moved to make room. On shrink, space is
  reclaimed. Call TransferListUpdateChecksum() after modifications.

  @param[in,out]  TransferListHeader  Pointer to the Transfer List header.
  @param[in,out]  Entry               Entry to resize.
  @param[in]      NewDataSize         New payload size in bytes.

  @retval TRUE   Resize succeeded; Entry remains valid.
  @retval FALSE  Resize failed (e.g., insufficient capacity).
**/
BOOLEAN
EFIAPI
TransferListSetDataSize (
  IN OUT TRANSFER_LIST_HEADER   *TransferListHeader,
  IN OUT TRANSFER_ENTRY_HEADER  *Entry,
  IN     UINT32                 NewDataSize
  );

/**
  Remove a Transfer Entry.

  After removal, the Transfer List is compacted as required by the underlying
  implementation. Call TransferListUpdateChecksum() if not done automatically.

  @param[in,out]  TransferListHeader  Pointer to the Transfer List header.
  @param[in,out]  Entry               Entry to remove.

  @retval TRUE   Entry removed.
  @retval FALSE  Removal failed.
**/
BOOLEAN
EFIAPI
TransferListRemove (
  IN OUT TRANSFER_LIST_HEADER   *TransferListHeader,
  IN OUT TRANSFER_ENTRY_HEADER  *Entry
  );

/**
  Optional reverse iterator: return the previous entry.

  @param[in]  TransferListHeader  TL header.
  @param[in]  CurrentEntry        Current entry.

  @retval NULL      No previous entry.
  @retval non-NULL  Previous entry.
**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TransferListGetPrevEntry (
  TRANSFER_LIST_HEADER   *TransferListHeader,
  TRANSFER_ENTRY_HEADER  *CurrentEntry
  );

/**
  Find the first Transfer Entry whose full TagId (up to 24 bits) matches.

  This is a thin wrapper over the underlying implementation. It returns the
  first matching entry, or NULL if no match exists.

  @param[in]  TransferListHeader  Pointer to the Transfer List header.
  @param[in]  TagId               Full tag identifier to match.

  @retval NULL      No matching entry or TransferListHeader is NULL.
  @retval non-NULL  Pointer to the first matching entry.
**/

TRANSFER_ENTRY_HEADER *
EFIAPI
TransferListFindEntryByTag  (
  IN TRANSFER_LIST_HEADER  *TransferListHeader,
  IN UINT32                TagId
  );

/**
  Read the first entry matching the full TagId (up to 24-bit).

  @param[in]   TransferListHeader   Pointer to the Transfer List header.
  @param[in]   TagId                Full tag identifier to match.
  @param[out]  EntryOut             On success, receives the entry pointer.
  @param[out]  EntryDataOut         Optional; receives pointer to entry data.
  @param[out]  EntryDataSizeOut     Optional; receives data size in bytes.

  @retval EFI_SUCCESS      Match found and output parameters set.
  @retval EFI_NOT_FOUND    No entry with TagId.
  @retval EFI_INVALID_PARAMETER  TransferListHeader or EntryOut is NULL.
**/
EFI_STATUS
EFIAPI
TransferListReadEntryByTag (
  IN  TRANSFER_LIST_HEADER   *TransferListHeader,
  IN  UINT32                 TagId,
  OUT TRANSFER_ENTRY_HEADER  **EntryOut,
  OUT VOID                   **EntryDataOut     OPTIONAL,
  OUT UINT32                 *EntryDataSizeOut  OPTIONAL
  );

/**
  Update (or create) an entry by TagId.

  If an entry with TagId exists, it is resized if needed and the payload is
  overwritten with Data (if Data is not NULL). If no entry exists and
  CreateIfMissing is TRUE, a new entry is appended (aligned to Alignment if
  non-zero). The Transfer List checksum is updated on success.

  @param[in,out]  TransferListHeader   Pointer to the Transfer List header.
  @param[in]      TagId                Full tag identifier to update.
  @param[in]      Data                 Source buffer (may be NULL to only resize).
  @param[in]      DataSize             Size of Data in bytes.
  @param[in]      CreateIfMissing      If TRUE, create the entry when missing.
  @param[in]      AlignmentLog2        Optional power-of-two alignment (0 to use default).

  @retval EFI_SUCCESS           Entry updated or created.
  @retval EFI_NOT_FOUND         Entry not found and CreateIfMissing is FALSE.
  @retval EFI_BUFFER_TOO_SMALL  Not enough capacity to add/resize.
  @retval EFI_INVALID_PARAMETER TransferListHeader is NULL.
**/
EFI_STATUS
EFIAPI
TransferListUpdateEntryByTag (
  IN OUT TRANSFER_LIST_HEADER  *TransferListHeader,
  IN     UINT32                TagId,
  IN     CONST VOID            *Data        OPTIONAL,
  IN     UINT32                DataSize,
  IN     BOOLEAN               CreateIfMissing,
  IN     UINT8                 AlignmentLog2
  );

#endif // ARM_TRANSFER_LIST_LIB_
