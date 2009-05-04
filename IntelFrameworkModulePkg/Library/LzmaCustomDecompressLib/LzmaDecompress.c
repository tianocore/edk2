/** @file
  LZMA Decompress routines for edk2

  Portions based on LZMA SDK 4.65:
    LzmaUtil.c -- Test application for LZMA compression
    2008-11-23 : Igor Pavlov : Public domain

  Copyright (c) 2006 - 2009, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDecompressLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Guid/LzmaDecompress.h>

#include "Sdk/C/Types.h"
#include "Sdk/C/7zVersion.h"
#include "Sdk/C/LzmaDec.h"

/**
  Allocation routine used by LZMA decompression.

  @param p                Pointer to the ISzAlloc instance
  @param size             The size in bytes to be allocated

  @return The allocated pointer address, or NULL on failure
**/
STATIC
VOID *
SzAlloc (
  void *p,
  size_t size
  )
{
  return AllocatePool (size);
}

/**
  Free routine used by LZMA decompression.

  @param p                Pointer to the ISzAlloc instance
  @param address          The address to be freed
**/
STATIC
VOID
SzFree (
  void *p,
  void *address
  )
{
  if (address != NULL) {
    FreePool (address);
  }
}

STATIC ISzAlloc g_Alloc = { SzAlloc, SzFree };

#define LZMA_HEADER_SIZE (LZMA_PROPS_SIZE + 8)

STATIC
UINT64
GetDecodedSizeOfBuf(
  UINT8 *encodedData
  )
{
  UINT64 DecodedSize;
  INTN   Index;

  /* Parse header */
  DecodedSize = 0;
  for (Index = LZMA_PROPS_SIZE + 7; Index >= LZMA_PROPS_SIZE; Index--)
    DecodedSize = LShiftU64(DecodedSize, 8) + encodedData[Index];

  return DecodedSize;
}

//
// LZMA functions and data as defined in local LzmaDecompress.h
//

STATIC CONST VOID  *mSourceLastUsedWithGetInfo;
STATIC UINT32      mSizeOfLastSource;
STATIC UINT32      mDecompressedSizeForLastSource;

/**
  The internal implementation of *_DECOMPRESS_PROTOCOL.GetInfo().
  
  @param Source           The source buffer containing the compressed data.
  @param SourceSize       The size of source buffer
  @param DestinationSize  The size of destination buffer.
  @param ScratchSize      The size of scratch buffer.

  @retval RETURN_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  @retval RETURN_INVALID_PARAMETER - The source data is corrupted
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

  mSourceLastUsedWithGetInfo = Source;
  mSizeOfLastSource = SourceSize;
  mDecompressedSizeForLastSource = (UInt32)DecodedSize;
  *DestinationSize = mDecompressedSizeForLastSource;
  *ScratchSize = 0x10;
  return RETURN_SUCCESS;
}


/**
  The internal implementation of *_DECOMPRESS_PROTOCOL.Decompress().
  
  @param Source          - The source buffer containing the compressed data.
  @param Destination     - The destination buffer to store the decompressed data
  @param Scratch         - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.

  @retval RETURN_SUCCESS           - Decompression is successfull
  @retval RETURN_INVALID_PARAMETER - The source data is corrupted  
**/
RETURN_STATUS
EFIAPI
LzmaUefiDecompress (
  IN CONST VOID  *Source,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  )
{
  SRes        lzmaResult;
  ELzmaStatus status;
  SizeT       decodedBufSize;
  SizeT       encodedDataSize;

  if (Source != mSourceLastUsedWithGetInfo) {
    return RETURN_INVALID_PARAMETER;
  }

  decodedBufSize = (SizeT)mDecompressedSizeForLastSource;
  encodedDataSize = (SizeT)(mSizeOfLastSource - LZMA_HEADER_SIZE);

  lzmaResult = LzmaDecode(
    Destination,
    &decodedBufSize,
    (Byte*)((UINT8*)Source + LZMA_HEADER_SIZE),
    &encodedDataSize,
    Source,
    LZMA_PROPS_SIZE,
    LZMA_FINISH_END,
    &status,
    &g_Alloc
    );

  if (lzmaResult == SZ_OK) {
    return RETURN_SUCCESS;
  } else {
    return RETURN_INVALID_PARAMETER;
  }
}

