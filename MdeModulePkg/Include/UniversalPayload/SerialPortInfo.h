/** @file
  This file defines the structure for serial port info.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - Universal Payload Specification 0.75 (https://universalpayload.github.io/documentation/)
**/

#ifndef UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_H_
#define UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_H_

#include <UniversalPayload/UniversalPayload.h>

#pragma pack(1)
typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  BOOLEAN                             UseMmio;
  UINT8                               RegisterStride;
  UINT32                              BaudRate;
  UINT32                              ClockRate;
  EFI_PHYSICAL_ADDRESS                RegisterBase;
} UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO;
#pragma pack()

#define UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_REVISION  1

extern GUID  gUniversalPayloadSerialPortInfoGuid;

#endif //  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_H_
