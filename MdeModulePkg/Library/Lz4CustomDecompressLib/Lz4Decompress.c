/**
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-3-Clause-Clear
  @file
  LZ4 Decompress implementation.
**/

#include "Lz4DecompressLibInternal.h"


/**
  Get the size of the uncompressed buffer by parsing the LZ4 header.

  @param[in]  Source      The source buffer containing the compressed data.
  @param[in]  SourceSize  The size of the source buffer.
  @param[out] DestinationSize The size of the uncompressed buffer.
  @param[out] ScratchSize The size of the scratch buffer required.

  @retval  EFI_SUCCESS           The size of the uncompressed data was returned
                                 in DestinationSize and the size of the scratch
                                 buffer was returned in ScratchSize.
  @retval  EFI_INVALID_PARAMETER The source buffer is NULL or the source size is 0.
**/
EFI_STATUS
EFIAPI
Lz4UefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  )
{
  LZ4_DECOMPRESS_HEADER  *Header;

  // Check for invalid parameters
  if (Source == NULL || SourceSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  // Check if the source buffer is big enough to contain the header
  if (SourceSize < LZ4_INFO_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  Header = (LZ4_DECOMPRESS_HEADER *)Source;

  // Return the original size from the header
  *DestinationSize = Header->OriginalSize;

  // Keep the existing scratch contract used by the guided section interface.
  *ScratchSize = LZ4_SCRATCH_MAX;

  return EFI_SUCCESS;
}

/**
  Decompresses a LZ4 compressed source buffer.

  @param[in]  Source      The source buffer containing the compressed data.
  @param[in]  SourceSize  The size of the source buffer.
  @param[out] Destination The destination buffer to store the decompressed data.
  @param[out] Scratch     A temporary scratch buffer that is used to perform the decompression.
                          This is an optional parameter that may be NULL if the
                          required scratch buffer size is 0.

  @retval  EFI_SUCCESS           Decompression completed successfully.
  @retval  EFI_INVALID_PARAMETER The source buffer specified by Source is corrupted
                                 (not in a valid LZ4 format).
**/
EFI_STATUS
EFIAPI
Lz4UefiDecompress (
  IN CONST VOID  *Source,
  IN UINTN       SourceSize,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch  OPTIONAL
  )
{
  LZ4_DECOMPRESS_HEADER  *Header;
  UINT8                  *CompressedData;
  int                    DecompressedSize;

  // Check for invalid parameters
  if (Source == NULL || Destination == NULL || SourceSize < LZ4_INFO_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  Header = (LZ4_DECOMPRESS_HEADER *)Source;
  CompressedData = (UINT8 *)Source + LZ4_INFO_SIZE;

  // Verify the compressed size
  if (Header->CompressedSize > (SourceSize - LZ4_INFO_SIZE)) {
    return EFI_INVALID_PARAMETER;
  }

  // LZ4_decompress_safe() accepts signed int sizes.
  if ((Header->CompressedSize > MAX_INT32) ||
      (Header->OriginalSize > MAX_INT32)) {
    return EFI_INVALID_PARAMETER;
  }

  // Decompress the data using LZ4
  DecompressedSize = LZ4_decompress_safe (
                       (const char *)CompressedData,
                       (char *)Destination,
                       (int)Header->CompressedSize,
                       (int)Header->OriginalSize
                       );

  // Check if decompression was successful
  if (DecompressedSize < 0 || (UINT32)DecompressedSize != Header->OriginalSize) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
