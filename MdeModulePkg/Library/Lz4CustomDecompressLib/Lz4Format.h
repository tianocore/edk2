/**
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent
  @file
  LZ4 custom firmware format definitions.
  The edk2 LZ4 payload format is a custom wrapper and not a standard LZ4
  frame stream. The serialized layout is:
    [UINT32 OriginalSize][UINT32 CompressedSize][LZ4 block payload]
**/

#ifndef __LZ4_FORMAT_H__
#define __LZ4_FORMAT_H__

#include <Base.h>

typedef struct {
  UINT32    OriginalSize;
  UINT32    CompressedSize;
} LZ4_DECOMPRESS_HEADER;

#define LZ4_INFO_SIZE    ((UINT32)sizeof (LZ4_DECOMPRESS_HEADER))
#define LZ4_SCRATCH_MAX  SIZE_128KB

#endif
