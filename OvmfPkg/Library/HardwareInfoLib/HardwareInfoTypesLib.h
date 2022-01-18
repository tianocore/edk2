/**@file
  Hardware info types' definitions.
  General hardware info types to parse the binary data

  Copyright 2021 - 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HARDWARE_INFO_TYPES_LIB_H__
#define __HARDWARE_INFO_TYPES_LIB_H__

// Specific hardware types:
#include "HardwareInfoPciHostBridgeLib.h"

//
// Hardware info types enumeration listing the supported
// types which have an associated type definition
//
typedef enum {
  HardwareInfoTypeUndefined  = 0,
  HardwareInfoTypeHostBridge = 1,

  HardwareInfoTypeMax
} HARDWARE_INFO_TYPE;

//
// Header format preceding and describing an associated hardware
// info element
//
#pragma pack(1)
typedef struct {
  union {
    UINT64                Uint64;
    HARDWARE_INFO_TYPE    Value;
  } Type;
  UINT64    Size;
} HARDWARE_INFO_HEADER;
#pragma pack()

//
// Generic data structure to access any supported hardware type
// resource definition
//
#pragma pack(1)
typedef struct {
  LIST_ENTRY              Link;
  HARDWARE_INFO_HEADER    Header;
  union {
    UINT8               *Raw;
    HOST_BRIDGE_INFO    *PciHostBridge;
  } Data;
} HARDWARE_INFO;
#pragma pack()

#define HARDWARE_INFO_FROM_LINK(a) \
  BASE_CR (a, HARDWARE_INFO, Link)

#endif // __HARDWARE_INFO_TYPES_LIB_H__
