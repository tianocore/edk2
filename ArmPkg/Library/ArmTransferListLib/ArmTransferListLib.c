/** @file
  Library that implements the helper functions to parse and pack a Transfer
  List as specified by the A-profile Firmware Handoff Specification.

  Copyright (c) 2022 - 2025, Arm Limited. All rights reserved.<BR>
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
  This function verifies the checksum of the Transfer List.

  @param [in]   Tlh       Pointer to the Transfer List Header

  @retval FALSE      Invalid Checksum
  @retval TRUE       Valid Checksum

**/
BOOLEAN
TlVerifyChecksum (
  IN TRANSFER_LIST_HEADER  *Tlh
  )
{
  if (Tlh == NULL) {
    return FALSE;
  }

  if (!(Tlh->Flags & TRANSFER_LIST_FL_HAS_CHECKSUM)) {
    return TRUE;
  }

  return (CalculateSum8 ((UINT8 *)Tlh, Tlh->UsedSize) == 0);
}

/**
  This function checks the header of the Transfer List.

  @param [in]   Tlh       Pointer to the Transfer List Header

  @return TRANSFER_LIST_OPS code indicating the validity of the Transfer List

**/
TRANSFER_LIST_OPS
EFIAPI
TlCheckHeader (
  IN TRANSFER_LIST_HEADER  *Tlh
  )
{
  if (Tlh == NULL) {
    return TL_OPS_INVALID;
  }

  if (Tlh->Signature != TRANSFER_LIST_SIGNATURE_64) {
    DEBUG ((DEBUG_ERROR, "Bad transfer list signature 0x%x\n", Tlh->Signature));
    return TL_OPS_INVALID;
  }

  if (Tlh->TotalSize <= 0) {
    DEBUG ((DEBUG_ERROR, "Bad transfer list total size 0x%x\n", Tlh->TotalSize));
    return TL_OPS_INVALID;
  }

  if (Tlh->UsedSize > Tlh->TotalSize) {
    DEBUG ((DEBUG_ERROR, "Bad transfer list used size 0x%x\n", Tlh->UsedSize));
    return TL_OPS_INVALID;
  }

  if (Tlh->HeaderSize != sizeof (TRANSFER_LIST_HEADER)) {
    DEBUG ((DEBUG_ERROR, "Bad transfer list header size 0x%x\n", Tlh->HeaderSize));
    return TL_OPS_INVALID;
  }

  if (TlVerifyChecksum (Tlh) == FALSE) {
    DEBUG ((DEBUG_ERROR, "Bad transfer list checksum 0x%x\n", Tlh->Checksum));
    return TL_OPS_INVALID;
  }

  if (Tlh->Version == 0) {
    DEBUG ((DEBUG_ERROR, "Transfer list version is invalid\n"));
    return TL_OPS_INVALID;
  } else if (Tlh->Version == ARM_FW_HANDOFF_PROTOCOL_VERSION) {
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Transfer list version is valid for all operations\n"));
    return TL_OPS_ALL;
  } else if (Tlh->Version > ARM_FW_HANDOFF_PROTOCOL_VERSION) {
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Transfer list version is valid for read-only\n"));
    return TL_OPS_RO;
  }

  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Old or custom transfer list version is detected\n"));
  return TL_OPS_CUSTOM;
}

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
  @param [in]   CurrentTe  Pointer to the Current Transfer Entry.
                           If this is NULL, the first Transfer Entry is returned.

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TlGetNextEntry (
  IN TRANSFER_LIST_HEADER   *Tlh,
  IN TRANSFER_ENTRY_HEADER  *CurrentTe
  )
{
  TRANSFER_ENTRY_HEADER  *Te;
  UINTN                  CurrentAddr;
  UINTN                  EndAddr;

  if (Tlh == NULL) {
    return NULL;
  }

  EndAddr = (UINTN)Tlh + Tlh->UsedSize;

  if (CurrentTe != NULL) {
    CurrentAddr = (UINTN)CurrentTe + CurrentTe->HeaderSize + CurrentTe->DataSize;
  } else {
    CurrentAddr = (UINTN)Tlh + Tlh->HeaderSize;
  }

  CurrentAddr = ALIGN_VALUE (CurrentAddr, (1 << Tlh->Alignment));

  Te = (TRANSFER_ENTRY_HEADER *)CurrentAddr;

  if (((CurrentAddr + sizeof (TRANSFER_LIST_HEADER)) < CurrentAddr) ||
      ((CurrentAddr + sizeof (TRANSFER_ENTRY_HEADER)) > EndAddr) ||
      ((CurrentAddr + Te->HeaderSize + Te->DataSize) < CurrentAddr) ||
      ((CurrentAddr + Te->HeaderSize + Te->DataSize) > EndAddr))
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

  while ((Te != NULL) && ((Te->TagId != TagId) || Te->Reserved0 != 0)) {
    Te = TlGetNextEntry (Tlh, Te);
  }

  return Te;
}

/**
  Return the Next Transfer Entry Node in the Transfer List
  matched with given tag-id from last Transfer Entry Node.

  @param [in]   Tlh        TransferListHeader
  @param [in]   CurrentTe  Pointer to the Current Transfer Entry.
                           If this is NULL, the first Transfer Entry is returned.
  @param [in]   TagId      Tag id

  @return Pointer to the Transfer Entry Node if successful otherwise NULL

**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TlFindNextEntry (
  IN TRANSFER_LIST_HEADER   *Tlh,
  IN TRANSFER_ENTRY_HEADER  *CurrentTe,
  IN UINT16                 TagId
  )
{
  TRANSFER_ENTRY_HEADER  *Te;

  if (CurrentTe == NULL) {
    return TlFindFirstEntry (Tlh, TagId);
  } else {
    Te = TlGetNextEntry (Tlh, CurrentTe);
  }

  while ((Te != NULL) && ((Te->TagId != TagId) || Te->Reserved0 != 0)) {
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

/**
  Find a Transfer Entry Node in the Transfer List matched with the given tag-id.

  @param [in]   Tlh       Pointer to the Transfer List Header
  @param [in]   TagId     Tag id

  @return Pointer to the Transfer Entry Node if successful otherwise NULL
**/
TRANSFER_ENTRY_HEADER *
EFIAPI
TlFindEntry (
  IN TRANSFER_LIST_HEADER  *Tlh,
  IN UINT16                TagId
  )
{
  TRANSFER_ENTRY_HEADER  *Te = NULL;

  do {
    Te = TlGetNextEntry (Tlh, Te);
  } while ((Te != NULL) && (Te->TagId != TagId));

  return Te;
}

/**
  Dump the transfer list to the debug output.

  @param [in]   Tlh       TransferListHeader

**/
VOID
EFIAPI
TlDump (
  IN TRANSFER_LIST_HEADER  *Tlh
  )
{
  TRANSFER_ENTRY_HEADER  *Te;
  UINTN                  Idx;

  Te  = NULL;
  Idx = 0;

  if (Tlh == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Dump transfer list:\n"));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "signature  0x%x\n", Tlh->Signature));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "checksum   0x%x\n", Tlh->Checksum));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "version    0x%x\n", Tlh->Version));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "hdr_size   0x%x\n", Tlh->HeaderSize));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "alignment  0x%x\n", Tlh->Alignment));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "used_size  0x%x\n", Tlh->UsedSize));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "total_size 0x%x\n", Tlh->TotalSize));
  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "flags      0x%x\n", Tlh->Flags));

  while (TRUE) {
    Te = TlGetNextEntry (Tlh, Te);
    if (Te == NULL) {
      break;
    }

    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Entry %d:\n", Idx++));
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "tag_id     0x%x\n", Te->TagId));
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "hdr_size   0x%x\n", Te->HeaderSize));
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "data_size  0x%x\n", Te->DataSize));
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "data_addr  0x%lx\n", (UINTN)TlGetEntryData (Te)));
  }
}
