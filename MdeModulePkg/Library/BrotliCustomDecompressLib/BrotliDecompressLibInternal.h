/** @file
  BROTLI UEFI header file

  Allows BROTLI code to build under UEFI (edk2) build environment

  Copyright (c) 2017 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BROTLI_DECOMPRESS_INTERNAL_H__
#define __BROTLI_DECOMPRESS_INTERNAL_H__

#include <PiPei.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <brotli/c/include/brotli/types.h>
#include <brotli/c/include/brotli/decode.h>

typedef struct
{
  VOID     *Buff;
  UINTN    BuffSize;
} BROTLI_BUFF;

#define FILE_BUFFER_SIZE     65536
#define BROTLI_INFO_SIZE     8
#define BROTLI_DECODE_MAX    8
#define BROTLI_SCRATCH_MAX   16

EFI_STATUS
EFIAPI
BrotliUefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  );

EFI_STATUS
EFIAPI
BrotliUefiDecompress (
  IN CONST VOID  *Source,
  IN UINTN       SourceSize,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  );

#endif
