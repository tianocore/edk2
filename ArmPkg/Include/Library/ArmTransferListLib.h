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
  IN UINT16                TagId
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
  IN UINT16                 TagId
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
  IN UINT16                TagId
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

#endif // ARM_TRANSFER_LIST_LIB_
