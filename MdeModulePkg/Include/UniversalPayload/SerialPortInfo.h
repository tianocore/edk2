/** @file
  This file defines the structure for serial port info.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <UniversalPayload/UniversalPayload.h>

#pragma pack(1)
typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  BOOLEAN                             UseMmio;
  UINT8                               RegisterStride;
  UINT32                              BaudRate;
  EFI_PHYSICAL_ADDRESS                RegisterBase;
  UINT32                              InputHertz;
} UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO;
#pragma pack()

#define UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_REVISION  2

extern GUID  gUniversalPayloadSerialPortInfoGuid;
