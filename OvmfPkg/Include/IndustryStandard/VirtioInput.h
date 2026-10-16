/** @file

  Virtio Input Device specific type and macro definitions corresponding to
  the virtio 1.1 specification.
  https://docs.oasis-open.org/virtio/virtio/v1.1/cs01/virtio-v1.1-cs01.html#x1-3390008

  Copyright (C) 2026, Advanced Micro Devices, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <IndustryStandard/Virtio.h>

#pragma pack(1)

//
// Device configuration
//
typedef enum {
  VirtioInputCfg         = 0x00,
  VirtioInputCfgIdName   = 0x01,
  VirtioInputCfgIdSerial = 0x02,
  VirtioInputCfgIdDevids = 0x03,
  VirtioInputCfgPropBits = 0x10,
  VirtioInputCfgEvBits   = 0x11,
  VirtioInputCfgAbsInfo  = 0x12,
} VIRTIO_INPUT_CONFIG_SELECT;

typedef struct {
  UINT32    Min;
  UINT32    Max;
  UINT32    Fuzz;
  UINT32    Flat;
  UINT32    Res;
} VIRTIO_INPUT_ABS_INFO;

typedef struct {
  UINT16    Bustype;
  UINT16    Vendor;
  UINT16    Product;
  UINT16    Version;
} VIRTIO_INPUT_DEVIDS;

typedef struct {
  UINT8    Select;
  UINT8    Subsel;
  UINT8    Size;
  UINT8    Reserved[5];
  union {
    CHAR8                    String[128];
    UINT8                    Bitmap[128];
    VIRTIO_INPUT_ABS_INFO    Abs;
    VIRTIO_INPUT_DEVIDS      Ids;
  } Data;
} VIRTIO_INPUT_CONFIG;

#define OFFSET_OF_VINPUT(Field)  OFFSET_OF (VIRTIO_INPUT_CONFIG, Field)
#define SIZE_OF_VINPUT(Field)    (sizeof ((VIRTIO_INPUT_CONFIG *) 0)->Field)

//
// Device Operation
//
typedef struct {
  UINT16    Type;
  UINT16    Code;
  UINT32    Value;
} VIRTIO_INPUT_EVENT;

#pragma pack ()
