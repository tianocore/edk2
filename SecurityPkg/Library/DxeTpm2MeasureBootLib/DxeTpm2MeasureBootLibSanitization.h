/** @file
  This file includes the function prototypes for the sanitization functions.

  These are those functions:

  DxeTpm2MeasureBootLibImageRead() function will make sure the PE/COFF image content
  read is within the image buffer.

  Tcg2MeasureGptTable() function will receive untrusted GPT partition table, and parse
  partition data carefully.

  Tcg2MeasurePeImage() function will accept untrusted PE/COFF image and validate its
  data structure within this image buffer before use.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DXE_TPM2_MEASURE_BOOT_LIB_SANITATION_
#define DXE_TPM2_MEASURE_BOOT_LIB_SANITATION_

#include <Uefi.h>
#include <Uefi/UefiSpec.h>
#include <Protocol/BlockIo.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Protocol/Tcg2Protocol.h>

/**
  This function will validate the EFI_PARTITION_TABLE_HEADER structure is safe to parse
  However this function will not attempt to verify the validity of the GPT partition
  It will check the following:
    - Signature
    - Revision
    - AlternateLBA
    - FirstUsableLBA
    - LastUsableLBA
    - PartitionEntryLBA
    - NumberOfPartitionEntries
    - SizeOfPartitionEntry
    - BlockIo

  @param[in] PrimaryHeader
    Pointer to the EFI_PARTITION_TABLE_HEADER structure.

  @param[in] BlockIo
    Pointer to the EFI_BLOCK_IO_PROTOCOL structure.

  @retval EFI_SUCCESS
    The EFI_PARTITION_TABLE_HEADER structure is valid.

  @retval EFI_INVALID_PARAMETER
    The EFI_PARTITION_TABLE_HEADER structure is invalid.
**/
EFI_STATUS
EFIAPI
Tpm2SanitizeEfiPartitionTableHeader (
  IN CONST EFI_PARTITION_TABLE_HEADER  *PrimaryHeader,
  IN CONST EFI_BLOCK_IO_PROTOCOL       *BlockIo
  );

/**
  This function will validate that the allocation size from the primary header is sane
  It will check the following:
    - AllocationSize does not overflow

  @param[in] PrimaryHeader
    Pointer to the EFI_PARTITION_TABLE_HEADER structure.

  @param[out] AllocationSize
    Pointer to the allocation size.

  @retval EFI_SUCCESS
    The allocation size is valid.

  @retval EFI_OUT_OF_RESOURCES
    The allocation size is invalid.
**/
EFI_STATUS
EFIAPI
Tpm2SanitizePrimaryHeaderAllocationSize (
  IN CONST EFI_PARTITION_TABLE_HEADER  *PrimaryHeader,
  OUT UINT32                           *AllocationSize
  );

/**
  This function will validate that the Gpt Event Size calculated from the primary header is sane
  It will check the following:
    - EventSize does not overflow

  Important: This function includes the entire length of the allocated space, including
  (sizeof (EFI_TCG2_EVENT) - sizeof (Tcg2Event->Event)) . When hashing the buffer allocated with this
  size, the caller must subtract the size of the (sizeof (EFI_TCG2_EVENT) - sizeof (Tcg2Event->Event))
  from the size of the buffer before hashing.

  @param[in] PrimaryHeader - Pointer to the EFI_PARTITION_TABLE_HEADER structure.
  @param[in] NumberOfPartition - Number of partitions.
  @param[out] EventSize - Pointer to the event size.

  @retval EFI_SUCCESS
    The event size is valid.

  @retval EFI_OUT_OF_RESOURCES
    Overflow would have occurred.

  @retval EFI_INVALID_PARAMETER
    One of the passed parameters was invalid.
**/
EFI_STATUS
Tpm2SanitizePrimaryHeaderGptEventSize (
  IN  CONST EFI_PARTITION_TABLE_HEADER  *PrimaryHeader,
  IN  UINTN                             NumberOfPartition,
  OUT UINT32                            *EventSize
  );

/**
  This function will validate that the PeImage Event Size from the loaded image is sane
  It will check the following:
    - EventSize does not overflow

  @param[in] FilePathSize - Size of the file path.
  @param[out] EventSize - Pointer to the event size.

  @retval EFI_SUCCESS
    The event size is valid.

  @retval EFI_OUT_OF_RESOURCES
    Overflow would have occurred.

  @retval EFI_INVALID_PARAMETER
    One of the passed parameters was invalid.
**/
EFI_STATUS
Tpm2SanitizePeImageEventSize (
  IN  UINT32  FilePathSize,
  OUT UINT32  *EventSize
  );

#endif // DXE_TPM2_MEASURE_BOOT_LIB_VALIDATION_
