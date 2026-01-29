/** @file
  Header file defining a Transfer List and Transfer Entry as specified by the
  A-profile Firmware Handoff Protocol specification.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - https://github.com/FirmwareHandoff/firmware_handoff

  @par Glossary:
    - TL - Transfer list
    - TE - Transfer entry
    - HOB - Hand off block.
**/

#ifndef ARM_TRANSFER_LIST_
#define ARM_TRANSFER_LIST_

#include <Base.h>
#include <Uefi.h>

#define ARM_FW_HANDOFF_PROTOCOL_VERSION  1

#define TRANSFER_LIST_ALIGNMENT     8             // 8 byte alignment
#define TRANSFER_LIST_SIGNATURE_64  (0x4a0fb10b)
#define TRANSFER_LIST_SIGNATURE_32  (0xfb10b)

/*
 * For register convention, please see below:
 * https://github.com/FirmwareHandoff/firmware_handoff/blob/main/source/register_conventions.rst
 */
#define REGISTER_CONVENTION_VERSION_SHIFT_64  (32)
#define TRANSFER_LIST_SIGNATURE_MASK_64       \
  ((1ULL << REGISTER_CONVENTION_VERSION_SHIFT_64) - 1)

#define REGISTER_CONVENTION_VERSION_SHIFT_32  (24)
#define TRANSFER_LIST_SIGNATURE_MASK_32       \
  ((1UL << REGISTER_CONVENTION_VERSION_SHIFT_32) - 1)

#define REGISTER_CONVENTION_VERSION_MASK  (0xff)
#define REGISTER_CONVENTION_VERSION       (1)

#define CREATE_TRANSFER_LIST_HANDOFF_X1_VALUE(version)    \
  ((TRANSFER_LIST_SIGNATURE &                             \
    (REGISTER_CONVENTION_VERSION_SHIFT_64 - 1)) |         \
    ((version) << REGISTER_CONVENTION_VERSION_SHIFT_64))

#define CREATE_TRANSFER_LIST_HANDOFF_R1_VALUE(version)    \
  ((TRANSFER_LIST_SIGNATURE &                             \
    (REGISTER_CONVENTION_VERSION_SHIFT_32 - 1)) |         \
    ((version) << REGISTER_CONVENTION_VERSION_SHIFT_32))

/*
 * tag id identifies contents of transfer entry.
 * below is the standard tag id used in transfer entry.
 * please see:
 *    https://github.com/FirmwareHandoff/firmware_handoff/blob/main/source/transfer_list.rst
 *    "Standard transfer entries" Section.
 */
#define TRANSFER_ENTRY_TAG_ID_EMPTY                 0
#define TRANSFER_ENTRY_TAG_ID_FDT                   1
#define TRANSFER_ENTRY_TAG_ID_HOB                   2
#define TRANSFER_ENTRY_TAG_ID_HOB_LIST              3
#define TRANSFER_ENTRY_TAG_ID_ACPI_TABLE_AGGREGATE  4
#define TRANSFER_ENTRY_TAG_ID_TPM_EVENT_LOG         5
#define TRANSFER_ENTRY_TAG_ID_TPM_CRB_BASE          6

/*
 * Flag value in TransferListHeader->Flags.
 * please see:
 *    https://github.com/FirmwareHandoff/firmware_handoff/blob/main/source/transfer_list.rst
 */
#define TRANSFER_LIST_FL_HAS_CHECKSUM  BIT0

/*
 * Flag values for TPM event log table entry layout XFERLIST_EVLOG->Flags,
 * see https://github.com/FirmwareHandoff/firmware_handoff/blob/main/source/transfer_list.rst#tpm-event-log-table-entry-layout-xferlist_evlog
 */
#define TRANSFER_LIST_EVENTLOG_FL_NEED_TO_REPLAY  BIT0 /* Need to replay */

/*
 * Operation codes indicating the validity of the Transfer List.
 */
typedef enum {
  TRANSFER_LIST_OPS_INVALID,  /* invalid for any operation */
  TRANSFER_LIST_OPS_ALL,      /* valid for all operations */
  TRANSFER_LIST_OPS_RO,       /* valid for read only */
  TRANSFER_LIST_OPS_CUSTOM,   /* abort or switch to special code to interpret */
} TRANSFER_LIST_OPS;

/*
 * Transfer list starts with the following header.
 * Transfer entries followed after the following header.
 */
typedef struct TransferListHeader {
  /// Signature, must be TRANSFER_LIST_SIGNATURE
  UINT32    Signature;

  /// Checksum
  UINT8     Checksum;

  /// Version of the TL Header.
  UINT8     Version;

  /// The size of this TL header in bytes.
  UINT8     HeaderSize;

  /// The maximum alignment required by any transfer entry in the transfer list,
  /// specified as a power of two.
  UINT8     Alignment;

  /// The number of bytes occupied by the TL. This field
  /// accounts for the size of the TL header plus the size
  /// of all the entries contained in the TL.
  UINT32    UsedSize;

  /// The number of bytes occupied by the entire TL,
  /// including any spare space at the end after UsedSize.
  UINT32    TotalSize;

  /// Flags word.
  UINT32    Flags;

  /// Reserved.
  UINT32    Reserved;
} TRANSFER_LIST_HEADER;

/*
 * Transfer entry in transfer list starts with the following header.
 */
typedef struct TransferEntryHeader {
  /// The entry type identifier.
  UINT16    TagId;

  /// Reserved.
  UINT8     Reserved0;

  /// The size of this entry header in bytes.
  UINT8     HeaderSize;

  /// The size of the data content in bytes.
  UINT32    DataSize;
} TRANSFER_ENTRY_HEADER;

/*
 * TPM event log information entry,
 * see Section 'TPM event log table entry layout (XFERLIST_EVLOG)' in
 * the Firmware Handoff specification.
 */
typedef struct TransferListEventLog {
  /// See the TRANSFER_LIST_EVENT_LOG_FL_*
  UINT32    Flags;

  /// TPM event log as much as
  /// TRNASFER_ENTRY_HEADER->DataSize - sizeof (TRANSFER_LIST_EVENTLOG)->Flags
  UINT8     EventLog[];
} TRANSFER_LIST_EVENTLOG;

#endif // ARM_TRANSFER_LIST_
