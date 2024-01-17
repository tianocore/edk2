/** @file
  The library instance provides security service of TPM measure boot and
  Confidential Computing (CC) measure boot.

  Caution: This file requires additional review when modified.
  This library will have external input - PE/COFF image and GPT partition.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  This file will pull out the validation logic from the following functions, in an
  attempt to validate the untrusted input in the form of unit tests

  These are those functions:

  DxeTpmMeasureBootLibImageRead() function will make sure the PE/COFF image content
  read is within the image buffer.

  Tcg2MeasureGptTable() function will receive untrusted GPT partition table, and parse
  partition data carefully.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <Uefi/UefiSpec.h>
#include <Library/SafeIntLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Protocol/BlockIo.h>
#include <Library/MemoryAllocationLib.h>

#include "DxeTpmMeasureBootLibSanitization.h"

#define GPT_HEADER_REVISION_V1  0x00010000

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
TpmSanitizeEfiPartitionTableHeader (
  IN CONST EFI_PARTITION_TABLE_HEADER  *PrimaryHeader,
  IN CONST EFI_BLOCK_IO_PROTOCOL       *BlockIo
  )
{
  // Verify that the input parameters are safe to use
  if (PrimaryHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "Invalid Partition Table Header!\n"));
    return EFI_INVALID_PARAMETER;
  }

  if ((BlockIo == NULL) || (BlockIo->Media == NULL)) {
    DEBUG ((DEBUG_ERROR, "Invalid BlockIo!\n"));
    return EFI_INVALID_PARAMETER;
  }

  // The signature must be EFI_PTAB_HEADER_ID ("EFI PART" in ASCII)
  if (PrimaryHeader->Header.Signature != EFI_PTAB_HEADER_ID) {
    DEBUG ((DEBUG_ERROR, "Invalid Partition Table Header!\n"));
    return EFI_DEVICE_ERROR;
  }

  // The version must be GPT_HEADER_REVISION_V1 (0x00010000)
  if (PrimaryHeader->Header.Revision != GPT_HEADER_REVISION_V1) {
    DEBUG ((DEBUG_ERROR, "Invalid Partition Table Header Revision!\n"));
    return EFI_DEVICE_ERROR;
  }

  // The HeaderSize must be greater than or equal to 92 and must be less than or equal to the logical block size
  if ((PrimaryHeader->Header.HeaderSize < sizeof (EFI_PARTITION_TABLE_HEADER)) || (PrimaryHeader->Header.HeaderSize > BlockIo->Media->BlockSize)) {
    DEBUG ((DEBUG_ERROR, "Invalid Partition Table Header HeaderSize!\n"));
    return EFI_DEVICE_ERROR;
  }

  // check that the PartitionEntryLBA greater than the Max LBA
  // This will be used later for multiplication
  if (PrimaryHeader->PartitionEntryLBA > DivU64x32 (MAX_UINT64, BlockIo->Media->BlockSize)) {
    DEBUG ((DEBUG_ERROR, "Invalid Partition Table Header PartitionEntryLBA!\n"));
    return EFI_DEVICE_ERROR;
  }

  // Check that the number of partition entries is greater than zero
  if (PrimaryHeader->NumberOfPartitionEntries == 0) {
    DEBUG ((DEBUG_ERROR, "Invalid Partition Table Header NumberOfPartitionEntries!\n"));
    return EFI_DEVICE_ERROR;
  }

  // SizeOfPartitionEntry must be 128, 256, 512... improper size may lead to accessing uninitialized memory
  if ((PrimaryHeader->SizeOfPartitionEntry < 128) || ((PrimaryHeader->SizeOfPartitionEntry & (PrimaryHeader->SizeOfPartitionEntry - 1)) != 0)) {
    DEBUG ((DEBUG_ERROR, "SizeOfPartitionEntry shall be set to a value of 128 x 2^n where n is an integer greater than or equal to zero (e.g., 128, 256, 512, etc.)!\n"));
    return EFI_DEVICE_ERROR;
  }

  // This check is to prevent overflow when calculating the allocation size for the partition entries
  // This check will be used later for multiplication
  if (PrimaryHeader->NumberOfPartitionEntries > DivU64x32 (MAX_UINT64, PrimaryHeader->SizeOfPartitionEntry)) {
    DEBUG ((DEBUG_ERROR, "Invalid Partition Table Header NumberOfPartitionEntries!\n"));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

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
TpmSanitizePrimaryHeaderAllocationSize (
  IN CONST EFI_PARTITION_TABLE_HEADER  *PrimaryHeader,
  OUT UINT32                           *AllocationSize
  )
{
  EFI_STATUS  Status;

  if (PrimaryHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (AllocationSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Replacing logic:
  // PrimaryHeader->NumberOfPartitionEntries * PrimaryHeader->SizeOfPartitionEntry;
  Status = SafeUint32Mult (PrimaryHeader->NumberOfPartitionEntries, PrimaryHeader->SizeOfPartitionEntry, AllocationSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Allocation Size would have overflowed!\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}

/**
  This function will validate that the Gpt Event Size calculated from the primary header is sane
  It will check the following:
    - EventSize does not overflow

  Important: This function includes the entire length of the allocated space, including the
  TCG_PCR_EVENT_HDR. When hashing the buffer allocated with this size, the caller must subtract
  the size of the TCG_PCR_EVENT_HDR from the size of the buffer before hashing.

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
TpmSanitizePrimaryHeaderGptEventSize (
  IN  CONST EFI_PARTITION_TABLE_HEADER  *PrimaryHeader,
  IN  UINTN                             NumberOfPartition,
  OUT UINT32                            *EventSize
  )
{
  EFI_STATUS  Status;
  UINT32      SafeNumberOfPartitions;

  if (PrimaryHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (EventSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // We shouldn't even attempt to perform the multiplication if the number of partitions is greater than the maximum value of UINT32
  Status = SafeUintnToUint32 (NumberOfPartition, &SafeNumberOfPartitions);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "NumberOfPartition would have overflowed!\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Replacing logic:
  // (UINT32)(sizeof (EFI_GPT_DATA) - sizeof (GptData->Partitions) + NumberOfPartition * PrimaryHeader.SizeOfPartitionEntry + sizeof (TCG_PCR_EVENT_HDR));
  Status = SafeUint32Mult (SafeNumberOfPartitions, PrimaryHeader->SizeOfPartitionEntry, EventSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Event Size would have overflowed!\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = SafeUint32Add (
             sizeof (TCG_PCR_EVENT_HDR) +
             OFFSET_OF (EFI_GPT_DATA, Partitions),
             *EventSize,
             EventSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Event Size would have overflowed because of GPTData!\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}

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
TpmSanitizePeImageEventSize (
  IN  UINT32  FilePathSize,
  OUT UINT32  *EventSize
  )
{
  EFI_STATUS  Status;

  // Replacing logic:
  // sizeof (*ImageLoad) - sizeof (ImageLoad->DevicePath) + FilePathSize;
  Status = SafeUint32Add (OFFSET_OF (EFI_IMAGE_LOAD_EVENT, DevicePath), FilePathSize, EventSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EventSize would overflow!\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  // Replacing logic:
  // EventSize + sizeof (TCG_PCR_EVENT_HDR)
  Status = SafeUint32Add (*EventSize, sizeof (TCG_PCR_EVENT_HDR), EventSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EventSize would overflow!\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}
