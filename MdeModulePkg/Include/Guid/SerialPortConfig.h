/** @file
  Provides serial port configuration.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SERIAL_PORT_CONFIG_H
#define _SERIAL_PORT_CONFIG_H

#include <Uefi/UefiBaseType.h>

#define SERIAL_PORT_CONFIG_MAX       3
#define SERIAL_PORT_CONFIG_SIZE_MAX  100

typedef union {
  struct {
    /**
    Enables eSPI interface for serial logging
    **/
    UINT8    EspiUartEnable  : 1;

    /**
    Enables local UART controler for serial logging
    **/
    UINT8    LocalUartEnable : 1;

    /**
      Following values are supported for COM selection:
      000 3F8h - 3FFh (COM 1)
      001 2F8h - 2FFh (COM 2)
      010 3E8h - 3EFh (COM 3)
      011 2E8h - 2EFh (COM 4)
    **/
    UINT8    EspiComSelect  : 3;
    UINT8    LocalComSelect : 3;
  } Fields;

  UINT8    Raw;
} PLATFORM_SERIAL_PORT_CONFIG;

typedef UINT8 SERIAL_PORT_DEVICE_CONFIG[SERIAL_PORT_CONFIG_SIZE_MAX];

typedef
RETURN_STATUS
(*SERIAL_PORT_INITIALIZE) (
  IN SERIAL_PORT_DEVICE_CONFIG  SerialPortDeviceConfig
  );

typedef
UINTN
(*SERIAL_PORT_WRITE) (
  IN SERIAL_PORT_DEVICE_CONFIG  SerialPortDeviceConfig,
  IN UINT8                      *Buffer,
  IN UINTN                      NumberOfBytes
  );

typedef
UINTN
(*SERIAL_PORT_READ) (
  IN SERIAL_PORT_DEVICE_CONFIG  SerialPortDeviceConfig,
  IN UINT8                      *Buffer,
  IN UINTN                      NumberOfBytes
  );

typedef
BOOLEAN
(*SERIAL_PORT_POLL) (
  IN SERIAL_PORT_DEVICE_CONFIG  SerialPortDeviceConfig
  );

typedef struct {
  /*
  Uart device enabled by platform.
  */
  BOOLEAN                      EnabledByPlatform;

  /*
  Uart device enabled by platform and available on current platform.
  */
  BOOLEAN                      Enabled;

  /*
  Uart device initialize hook.
  */
  SERIAL_PORT_INITIALIZE       Initialize;

  /*
  Uart device write hook.
  */
  SERIAL_PORT_WRITE            WriteBytes;

  /*
  Uart device read hook.
  */
  SERIAL_PORT_READ             ReadBytes;

  /*
  Uart device pool hook.
  */
  SERIAL_PORT_POLL             Poll;

  /*
  Uart device configuration data. Should be converted to device-specific type.
  */
  SERIAL_PORT_DEVICE_CONFIG    DeviceConfig;
} SERIAL_PORT_CONFIG_SINGLE;

typedef struct {
  /*
  Array with configurations of all UART devices.
  */
  SERIAL_PORT_CONFIG_SINGLE    Configurations[SERIAL_PORT_CONFIG_MAX];

  /*
  Count of existing UART devices.
  */
  UINT8                        Count;

  BOOLEAN                      InitializeDevices;
} SERIAL_PORT_CONFIG;

#endif //_SERIAL_PORT_CONFIG_H
