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
    - HOB - Hand off block.
**/

#ifndef ARM_TRANSFER_LIST_LIB_
#define ARM_TRANSFER_LIST_LIB_

#include <Base.h>
#include <Uefi.h>
#include <IndustryStandard/ArmTransferList.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

/**
  Return the first Transfer Entry Node in the Transfer List.

  @param [in]   Tlh       TransferListHeader

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TlGetFirstEntry (
  IN TRANSFER_LIST_HEADER  *Tlh
  );

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
  );

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
  );

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
  );

/**
  Return the data in Transfer Entry.

   @param [in]   Te       TransferEntryHeader

   @return Pointer to the Data of Transfer Entry Node if successful otherwise NULL

**/
VOID *
EFIAPI
TlGetEntryData (
  IN TRANSFER_ENTRY_HEADER  *Te
  );

#endif // ARM_TRANSFER_LIST_LIB_
