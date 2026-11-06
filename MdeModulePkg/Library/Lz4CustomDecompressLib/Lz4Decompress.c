/** @file
  LZ4 Decompress implementation.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Lz4DecompressLibInternal.h"

/**
  Reads and validates the serialized LZ4 header.

  Copies the header into an aligned local object before accessing its fields.
  Both size fields are validated against the available payload and the signed
  integer range accepted by the upstream LZ4 decompressor.

  @param[in]  Source      The source buffer containing the serialized header.
  @param[in]  SourceSize  The size, in bytes, of the source buffer.
  @param[out] Header      The aligned, validated LZ4 header.

  @retval EFI_SUCCESS           The header was read and validated.
  @retval EFI_INVALID_PARAMETER The source, header, or size fields are invalid.
**/
STATIC
EFI_STATUS
Lz4ReadAndValidateHeader (
  IN  CONST VOID             *Source,
  IN  UINTN                  SourceSize,
  OUT LZ4_DECOMPRESS_HEADER  *Header
  )
{
  if ((Source == NULL) ||
      (Header == NULL) ||
      (SourceSize < LZ4_INFO_SIZE))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Copy the serialized header to avoid unaligned field accesses.
  //
  CopyMem (Header, Source, sizeof (*Header));

  if ((Header->CompressedSize > (SourceSize - LZ4_INFO_SIZE)) ||
      (Header->CompressedSize > MAX_INT32) ||
      (Header->OriginalSize > MAX_INT32))
  {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Get the size of the uncompressed buffer by parsing the LZ4 header.

  @param[in]  Source      The source buffer containing the compressed data.
  @param[in]  SourceSize  The size of the source buffer.
  @param[out] DestinationSize The size of the uncompressed buffer.
  @param[out] ScratchSize The size of the scratch buffer required.

  @retval  EFI_SUCCESS           The size of the uncompressed data was returned
                                 in DestinationSize and the size of the scratch
                                 buffer was returned in ScratchSize.
  @retval  EFI_INVALID_PARAMETER The source buffer is NULL, the source size is too
                                 small, an output pointer is NULL, or the header is corrupt.
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
  LZ4_DECOMPRESS_HEADER  Header;
  EFI_STATUS             Status;

  if ((DestinationSize == NULL) || (ScratchSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Lz4ReadAndValidateHeader (Source, SourceSize, &Header);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Return the original size from the header.
  //
  *DestinationSize = Header.OriginalSize;

  //
  // Keep the existing scratch contract used by the guided section interface.
  //
  *ScratchSize = LZ4_SCRATCH_MAX;

  return EFI_SUCCESS;
}

/**
  Decompresses a LZ4 compressed source buffer.

  @param[in]  Source      The source buffer containing the compressed data.
  @param[in]  SourceSize  The size of the source buffer.
  @param[in, out] Destination The destination buffer to store the decompressed data.
  @param[in, out] Scratch     A temporary scratch buffer that is used to perform the decompression.
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
  LZ4_DECOMPRESS_HEADER  Header;
  CONST UINT8            *CompressedData;
  INT32                  DecompressedSize;
  EFI_STATUS             Status;

  if (Destination == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Lz4ReadAndValidateHeader (Source, SourceSize, &Header);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CompressedData = (CONST UINT8 *)Source + LZ4_INFO_SIZE;

  //
  // Decompress the data using LZ4.
  //
  DecompressedSize = LZ4_decompress_safe (
                       (const char *)CompressedData,
                       (char *)Destination,
                       (int)Header.CompressedSize,
                       (int)Header.OriginalSize
                       );

  //
  // Check if decompression was successful.
  //
  if ((DecompressedSize < 0) || ((UINT32)DecompressedSize != Header.OriginalSize)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
