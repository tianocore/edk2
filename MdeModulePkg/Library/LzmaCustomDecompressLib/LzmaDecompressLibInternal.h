/** @file
  LZMA Decompress Library internal header file declares Lzma decompress interfaces.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __LZMADECOMPRESSLIB_INTERNAL_H__
#define __LZMADECOMPRESSLIB_INTERNAL_H__

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Guid/LzmaDecompress.h>

/**
  Given a Lzma compressed source buffer, this function retrieves the size of
  the uncompressed buffer and the size of the scratch buffer required
  to decompress the compressed source buffer.

  Retrieves the size of the uncompressed buffer and the temporary scratch buffer
  required to decompress the buffer specified by Source and SourceSize.
  The size of the uncompressed buffer is returned in DestinationSize,
  the size of the scratch buffer is returned in ScratchSize, and RETURN_SUCCESS is returned.
  This function does not have scratch buffer available to perform a thorough
  checking of the validity of the source data. It just retrieves the "Original Size"
  field from the LZMA_HEADER_SIZE beginning bytes of the source data and output it as DestinationSize.
  And ScratchSize is specific to the decompression implementation.

  If SourceSize is less than LZMA_HEADER_SIZE, then ASSERT().

  @param  Source          The source buffer containing the compressed data.
  @param  SourceSize      The size, in bytes, of the source buffer.
  @param  DestinationSize A pointer to the size, in bytes, of the uncompressed buffer
                          that will be generated when the compressed buffer specified
                          by Source and SourceSize is decompressed.
  @param  ScratchSize     A pointer to the size, in bytes, of the scratch buffer that
                          is required to decompress the compressed buffer specified
                          by Source and SourceSize.

  @retval  RETURN_SUCCESS The size of the uncompressed data was returned
                          in DestinationSize and the size of the scratch
                          buffer was returned in ScratchSize.

**/
RETURN_STATUS
EFIAPI
LzmaUefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  );

/**
  Decompresses a Lzma compressed source buffer.

  Extracts decompressed data to its original form.
  If the compressed source data specified by Source is successfully decompressed
  into Destination, then RETURN_SUCCESS is returned.  If the compressed source data
  specified by Source is not in a valid compressed data format,
  then RETURN_INVALID_PARAMETER is returned.

  @param  Source      The source buffer containing the compressed data.
  @param  SourceSize  The size of source buffer.
  @param  Destination The destination buffer to store the decompressed data
  @param  Scratch     A temporary scratch buffer that is used to perform the decompression.
                      This is an optional parameter that may be NULL if the
                      required scratch buffer size is 0.

  @retval  RETURN_SUCCESS Decompression completed successfully, and
                          the uncompressed buffer is returned in Destination.
  @retval  RETURN_INVALID_PARAMETER
                          The source buffer specified by Source is corrupted
                          (not in a valid compressed format).
**/
RETURN_STATUS
EFIAPI
LzmaUefiDecompress (
  IN CONST VOID  *Source,
  IN UINTN       SourceSize,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  );

#endif

