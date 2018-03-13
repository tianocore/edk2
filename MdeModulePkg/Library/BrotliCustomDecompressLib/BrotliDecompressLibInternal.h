/** @file
  BROTLI UEFI header file

  Allows BROTLI code to build under UEFI (edk2) build environment

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __BROTLI_DECOMPRESS_INTERNAL_H__
#define __BROTLI_DECOMPRESS_INTERNAL_H__

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <common/types.h>
#include <dec/decode.h>

typedef struct
{
  VOID     *Buff;
  UINTN    BuffSize;
} BROTLI_BUFF;

#define FILE_BUFFER_SIZE     65536
#define BROTLI_INFO_SIZE     8
#define BROTLI_DECODE_MAX    8
#define BROTLI_SCRATCH_MAX   16

#define memcpy                      CopyMem
#define memmove                     CopyMem
#define memset(dest,ch,count)       SetMem(dest,(UINTN)(count),(UINT8)(ch))

VOID *
BrDummyMalloc (
  IN size_t   Size
  );

VOID
BrDummyFree (
  IN VOID *   Ptr
  );

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
