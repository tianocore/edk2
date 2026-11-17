/** @file
  Brotli Decompress interfaces

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <BrotliDecompressLibInternal.h>

/**
  Allocation routine used by BROTLI decompression.

  @param Ptr              Pointer to the BROTLI_BUFF instance.
  @param Size             The size in bytes to be allocated.

  @return The allocated pointer address, or NULL on failure
**/
VOID *
BrAlloc (
  IN VOID    *Ptr,
  IN size_t  Size
  )
{
  VOID         *Addr;
  BROTLI_BUFF  *Private;

  Private = (BROTLI_BUFF *)Ptr;

  if (Private->BuffSize >= Size) {
    Addr               = Private->Buff;
    Private->Buff      = (VOID *)((UINT8 *)Addr + Size);
    Private->BuffSize -= Size;
    return Addr;
  } else {
    return NULL;
  }
}

/**
  Free routine used by BROTLI decompression.

  @param Ptr              Pointer to the BROTLI_BUFF instance
  @param Address          The address to be freed
**/
VOID
BrFree (
  IN VOID  *Ptr,
  IN VOID  *Address
  )
{
  //
  // We use the 'scratch buffer' for allocations, so there is no free
  // operation required.  The scratch buffer will be freed by the caller
  // of the decompression code.
  //
}

/**
  Decompresses a Brotli compressed source buffer.

  Extracts decompressed data to its original form.
  If the compressed source data specified by Source is successfully decompressed
  into Destination, then EFI_SUCCESS is returned. If the compressed source data
  specified by Source is not in a valid compressed data format,
  then EFI_INVALID_PARAMETER is returned.

  @param  Source      The source buffer containing the compressed data.
  @param  SourceSize  The size of source buffer.
  @param  Destination The destination buffer to store the decompressed data.
  @param  DestinationSize The expected size of the decompressed data.
  @param  BuffInfo    The pointer to the BROTLI_BUFF instance.

  @retval EFI_SUCCESS Decompression completed successfully, and
                      the uncompressed buffer is returned in Destination.
  @retval EFI_INVALID_PARAMETER
                      The source buffer specified by Source is corrupted
                      (not in a valid compressed format).
**/
STATIC
EFI_STATUS
BrotliDecompressInternal (
  IN CONST VOID  *Source,
  IN UINTN       SourceSize,
  IN OUT VOID    *Destination,
  IN UINTN       DestinationSize,
  IN VOID        *BuffInfo
  )
{
  UINT8                *Input;
  UINT8                *Output;
  const UINT8          *NextIn;
  UINT8                *NextOut;
  size_t               TotalOut;
  size_t               AvailableIn;
  size_t               AvailableOut;
  VOID                 *Temp;
  UINTN                CopySize;
  EFI_STATUS           Status;
  BrotliDecoderResult  Result;
  BrotliDecoderState   *BroState;

  TotalOut     = 0;
  AvailableOut = FILE_BUFFER_SIZE;
  Result       = BROTLI_DECODER_RESULT_ERROR;
  Status       = EFI_INVALID_PARAMETER;
  BroState     = BrotliDecoderCreateInstance (BrAlloc, BrFree, BuffInfo);
  Temp         = Destination;

  if (BroState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Input  = (UINT8 *)BrAlloc (BuffInfo, FILE_BUFFER_SIZE);
  Output = (UINT8 *)BrAlloc (BuffInfo, FILE_BUFFER_SIZE);
  if ((Input == NULL) || (Output == NULL)) {
    BrFree (BuffInfo, Input);
    BrFree (BuffInfo, Output);
    BrotliDecoderDestroyInstance (BroState);
    return EFI_INVALID_PARAMETER;
  }

  NextOut = Output;
  Result  = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
  while (1) {
    if (Result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
      if (SourceSize == 0) {
        break;
      }

      if (SourceSize >= FILE_BUFFER_SIZE) {
        AvailableIn = FILE_BUFFER_SIZE;
      } else {
        AvailableIn = SourceSize;
      }

      CopyMem (Input, Source, AvailableIn);
      Source      = (VOID *)((UINT8 *)Source + AvailableIn);
      SourceSize -= AvailableIn;
      NextIn      = Input;
    } else if (Result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
      if (DestinationSize < FILE_BUFFER_SIZE) {
        goto Done;
      }

      CopyMem (Temp, Output, FILE_BUFFER_SIZE);
      AvailableOut     = FILE_BUFFER_SIZE;
      Temp             = (VOID *)((UINT8 *)Temp +FILE_BUFFER_SIZE);
      DestinationSize -= FILE_BUFFER_SIZE;
      NextOut          = Output;
    } else {
      break; /* Error or success. */
    }

    Result = BrotliDecoderDecompressStream (
               BroState,
               &AvailableIn,
               &NextIn,
               &AvailableOut,
               &NextOut,
               &TotalOut
               );
  }

  if (NextOut != Output) {
    CopySize = (UINTN)(NextOut - Output);
    if (CopySize > DestinationSize) {
      goto Done;
    }

    CopyMem (Temp, Output, CopySize);
    DestinationSize -= CopySize;
  }

  if ((Result == BROTLI_DECODER_RESULT_SUCCESS) && (DestinationSize == 0)) {
    Status = EFI_SUCCESS;
  }

Done:
  BrFree (BuffInfo, Input);
  BrFree (BuffInfo, Output);
  BrotliDecoderDestroyInstance (BroState);
  return Status;
}

/**
  Get the size of the uncompressed buffer by parsing EncodeData header.

  @param EncodedData  Pointer to the compressed data.
  @param StartOffset  Start offset of the compressed data.
  @param EndOffset    End offset of the compressed data.

  @return The size of the uncompressed buffer.
**/
UINT64
BrGetDecodedSizeOfBuf (
  IN UINT8  *EncodedData,
  IN UINT8  StartOffset,
  IN UINT8  EndOffset
  )
{
  UINT64  DecodedSize;
  INTN    Index;

  /* Parse header */
  DecodedSize = 0;
  for (Index = EndOffset - 1; Index >= StartOffset; Index--) {
    DecodedSize = LShiftU64 (DecodedSize, 8) + EncodedData[Index];
  }

  return DecodedSize;
}

/**
  Given a Brotli compressed source buffer, this function retrieves the size of
  the uncompressed buffer and the size of the scratch buffer required
  to decompress the compressed source buffer.

  Retrieves the size of the uncompressed buffer and the temporary scratch buffer
  required to decompress the buffer specified by Source and SourceSize.
  The size of the uncompressed buffer is returned in DestinationSize,
  the size of the scratch buffer is returned in ScratchSize, and EFI_SUCCESS is returned.
  This function does not have scratch buffer available to perform a thorough
  checking of the validity of the source data. It just retrieves the "Original Size"
  field from the BROTLI_SCRATCH_MAX beginning bytes of the source data and output it as DestinationSize.
  And ScratchSize is specific to the decompression implementation.

  Reject buffers shorter than BROTLI_SCRATCH_MAX and sizes that do not fit
  in the UINT32 output parameters.

  @param  Source          The source buffer containing the compressed data.
  @param  SourceSize      The size, in bytes, of the source buffer.
  @param  DestinationSize A pointer to the size, in bytes, of the uncompressed buffer
                          that will be generated when the compressed buffer specified
                          by Source and SourceSize is decompressed.
  @param  ScratchSize     A pointer to the size, in bytes, of the scratch buffer that
                          is required to decompress the compressed buffer specified
                          by Source and SourceSize.

  @retval EFI_SUCCESS     The size of the uncompressed data was returned
                          in DestinationSize and the size of the scratch
                          buffer was returned in ScratchSize.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL or the source
                                buffer is too short.
  @retval EFI_UNSUPPORTED  A decoded or scratch size does not fit in UINT32.
                          Output parameters have not been modified.
**/
EFI_STATUS
EFIAPI
BrotliUefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  )
{
  UINT64  DecodedSize;
  UINT64  RequiredScratchSize;

  if ((Source == NULL) || (DestinationSize == NULL) || (ScratchSize == NULL) ||
      (SourceSize < BROTLI_SCRATCH_MAX))
  {
    return EFI_INVALID_PARAMETER;
  }

  DecodedSize         = BrGetDecodedSizeOfBuf ((UINT8 *)Source, 0, BROTLI_DECODE_MAX);
  RequiredScratchSize = BrGetDecodedSizeOfBuf ((UINT8 *)Source, BROTLI_DECODE_MAX, BROTLI_SCRATCH_MAX);
  if ((DecodedSize > MAX_UINT32) || (RequiredScratchSize > MAX_UINT32)) {
    return EFI_UNSUPPORTED;
  }

  *DestinationSize = (UINT32)DecodedSize;
  *ScratchSize     = (UINT32)RequiredScratchSize;
  return EFI_SUCCESS;
}

/**
  Decompresses a Brotli compressed source buffer.

  Extracts decompressed data to its original form.
  If the compressed source data specified by Source is successfully decompressed
  into Destination, then RETURN_SUCCESS is returned.  If the compressed source data
  specified by Source is not in a valid compressed data format,
  then RETURN_INVALID_PARAMETER is returned.

  Destination and Scratch must be allocated using the sizes returned by
  BrotliUefiDecompressGetInfo() for the same source buffer. The decoded data
  must exactly match the destination size advertised in the source header.

  @param  Source      The source buffer containing the compressed data.
  @param  SourceSize  The size of source buffer.
  @param  Destination The destination buffer to store the decompressed data
  @param  Scratch     A temporary scratch buffer that is used to perform the decompression.
                      This is an optional parameter that may be NULL if the
                      required scratch buffer size is 0.

  @retval EFI_SUCCESS Decompression completed successfully, and
                      the uncompressed buffer is returned in Destination.
  @retval EFI_INVALID_PARAMETER
                      The source buffer specified by Source is corrupted
                      (not in a valid compressed format), a required buffer
                      is NULL, or the decoded size does not match the header.
  @retval EFI_UNSUPPORTED
                      A source, decoded, or scratch size does not fit in UINT32.
**/
EFI_STATUS
EFIAPI
BrotliUefiDecompress (
  IN CONST VOID  *Source,
  IN UINTN       SourceSize,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  )
{
  EFI_STATUS   Status;
  BROTLI_BUFF  BroBuff;
  UINT32       DestinationSize;
  UINT32       ScratchSize;

  if (SourceSize > MAX_UINT32) {
    return EFI_UNSUPPORTED;
  }

  Status = BrotliUefiDecompressGetInfo (Source, (UINT32)SourceSize, &DestinationSize, &ScratchSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (((Destination == NULL) && (DestinationSize != 0)) ||
      ((Scratch == NULL) && (ScratchSize != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  BroBuff.Buff     = Scratch;
  BroBuff.BuffSize = ScratchSize;

  Status = BrotliDecompressInternal (
             (VOID *)((UINT8 *)Source + BROTLI_SCRATCH_MAX),
             SourceSize - BROTLI_SCRATCH_MAX,
             Destination,
             DestinationSize,
             (VOID *)(&BroBuff)
             );

  return Status;
}
