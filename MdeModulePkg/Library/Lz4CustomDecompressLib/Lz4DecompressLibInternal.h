/** @file
  LZ4 UEFI internal integration header.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Allows LZ4 code to build under the UEFI (edk2) build environment.
**/

#pragma once

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Pi/PiFirmwareFile.h>
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
