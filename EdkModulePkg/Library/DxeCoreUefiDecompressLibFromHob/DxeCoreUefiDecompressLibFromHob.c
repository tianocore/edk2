/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  DxeCoreUefiDecompressLibFromHob.c

Abstract:

  UEFI Decompress Library from HOBs

--*/

static DECOMPRESS_LIBRARY  mEfiDecompress;

EFI_STATUS
EFIAPI
DxeCoreUefiDecompressLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = GetFirstGuidHob (&gEfiDecompressProtocolGuid);
  ASSERT (GuidHob != NULL);
  CopyMem (&mEfiDecompress, GET_GUID_HOB_DATA (GuidHob), sizeof (mEfiDecompress));
  return EFI_SUCCESS;
}

RETURN_STATUS
EFIAPI
UefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  )
/*++

Routine Description:

  The internal implementation of *_DECOMPRESS_PROTOCOL.GetInfo().

Arguments:

  Source          - The source buffer containing the compressed data.
  SourceSize      - The size of source buffer
  DestinationSize - The size of destination buffer.
  ScratchSize     - The size of scratch buffer.

Returns:

  RETURN_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  RETURN_INVALID_PARAMETER - The source data is corrupted

--*/
{
  return mEfiDecompress.GetInfo (Source, SourceSize, DestinationSize, ScratchSize);
}

RETURN_STATUS
EFIAPI
UefiDecompress (
  IN CONST VOID  *Source,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  )
/*++

Routine Description:

  The internal implementation of *_DECOMPRESS_PROTOCOL.Decompress().

Arguments:

  Source          - The source buffer containing the compressed data.
  Destination     - The destination buffer to store the decompressed data
  Scratch         - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.

Returns:

  RETURN_SUCCESS           - Decompression is successfull
  RETURN_INVALID_PARAMETER - The source data is corrupted

--*/
{
  return mEfiDecompress.Decompress (Source, Destination, Scratch);
}
