/**
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-3-Clause-Clear
  @file
  LZ4 UEFI internal integration header.
  Allows LZ4 code to build under the UEFI (edk2) build environment.
**/

#ifndef __LZ4_DECOMPRESS_LIB_INTERNAL_H__
#define __LZ4_DECOMPRESS_LIB_INTERNAL_H__

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Guid/Lz4Decompress.h>
#include "UefiLz4Compat.h"
#include "Lz4Format.h"
#include <lz4/lib/lz4.h>

EFI_STATUS
EFIAPI
Lz4UefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  );

EFI_STATUS
EFIAPI
Lz4UefiDecompress (
  IN CONST VOID  *Source,
  IN UINTN       SourceSize,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch  OPTIONAL
  );

#endif
