/** @file
  LZMA Decompress interfaces

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LzmaDecompressLibInternal.h"
#include "Sdk/C/7zTypes.h"
#include "Sdk/C/7zVersion.h"
#include "Sdk/C/LzmaDec.h"

#define SCRATCH_BUFFER_REQUEST_SIZE SIZE_64KB

typedef struct
{
  ISzAlloc Functions;
  VOID     *Buffer;
  UINTN    BufferSize;
} ISzAllocWithData;

/**
  Allocation routine used by LZMA decompression.

  @param P                Pointer to the ISzAlloc instance
  @param Size             The size in bytes to be allocated

  @return The allocated pointer address, or NULL on failure
**/
VOID *
SzAlloc (
  CONST ISzAlloc *P,
  size_t Size
  )
{
  VOID *Addr;
  ISzAllocWithData *Private;

  Private = (ISzAllocWithData*) P;

  if (Private->BufferSize >= Size) {
    Addr = Private->Buffer;
    Private->Buffer = (VOID*) ((UINT8*)Addr + Size);
    Private->BufferSize -= Size;
    return Addr;
  } else {
    ASSERT (FALSE);
    return NULL;
  }
}

/**
  Free routine used by LZMA decompression.

  @param P                Pointer to the ISzAlloc instance
  @param Address          The address to be freed
**/
VOID
SzFree (
  CONST ISzAlloc *P,
  VOID *Address
  )
{
  //
  // We use the 'scratch buffer' for allocations, so there is no free
  // operation required.  The scratch buffer will be freed by the caller
  // of the decompression code.
  //
}

#define LZMA_HEADER_SIZE (LZMA_PROPS_SIZE + 8)

/**
  Get the size of the uncompressed buffer by parsing EncodeData header.

  @param EncodedData  Pointer to the compressed data.

  @return The size of the uncompressed buffer.
**/
UINT64
GetDecodedSizeOfBuf(
  UINT8 *EncodedData
  )
{
  UINT64 DecodedSize;
  INTN   Index;

  /* Parse header */
  DecodedSize = 0;
  for (Index = LZMA_PROPS_SIZE + 7; Index >= LZMA_PROPS_SIZE; Index--)
    DecodedSize = LShiftU64(DecodedSize, 8) + EncodedData[Index];

  return DecodedSize;
}

//
// LZMA functions and data as defined in local LzmaDecompressLibInternal.h
//

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

  @retval RETURN_UNSUPPORTED  DestinationSize cannot be output because the
                              uncompressed buffer size (in bytes) does not fit
                              in a UINT32. Output parameters have not been
                              modified.
**/
RETURN_STATUS
EFIAPI
LzmaUefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  )
{
  UInt64  DecodedSize;

  ASSERT(SourceSize >= LZMA_HEADER_SIZE);

  DecodedSize = GetDecodedSizeOfBuf((UINT8*)Source);
  if (DecodedSize > MAX_UINT32) {
    return RETURN_UNSUPPORTED;
  }

  *DestinationSize = (UINT32)DecodedSize;
  *ScratchSize = SCRATCH_BUFFER_REQUEST_SIZE;
  return RETURN_SUCCESS;
}

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
  )
{
  SRes              LzmaResult;
  ELzmaStatus       Status;
  SizeT             DecodedBufSize;
  SizeT             EncodedDataSize;
  ISzAllocWithData  AllocFuncs;

  AllocFuncs.Functions.Alloc  = SzAlloc;
  AllocFuncs.Functions.Free   = SzFree;
  AllocFuncs.Buffer           = Scratch;
  AllocFuncs.BufferSize       = SCRATCH_BUFFER_REQUEST_SIZE;

  DecodedBufSize = (SizeT)GetDecodedSizeOfBuf((UINT8*)Source);
  EncodedDataSize = (SizeT) (SourceSize - LZMA_HEADER_SIZE);

  LzmaResult = LzmaDecode(
    Destination,
    &DecodedBufSize,
    (Byte*)((UINT8*)Source + LZMA_HEADER_SIZE),
    &EncodedDataSize,
    Source,
    LZMA_PROPS_SIZE,
    LZMA_FINISH_END,
    &Status,
    &(AllocFuncs.Functions)
    );

  if (LzmaResult == SZ_OK) {
    return RETURN_SUCCESS;
  } else {
    return RETURN_INVALID_PARAMETER;
  }
}

