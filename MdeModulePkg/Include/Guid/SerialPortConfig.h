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
      Enables the eSPI interface for serial logging.
    **/
    UINT8    EspiUartEnable  : 1;

    /**
      Enables the local UART controller for serial logging.
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
    UART device enabled by the platform.
  */
  BOOLEAN                      EnabledByPlatform;

  /*
    UART device enabled by the platform and available on the current platform.
  */
  BOOLEAN                      Enabled;

  /*
    UART device initialization hook.
  */
  SERIAL_PORT_INITIALIZE       Initialize;

  /*
    UART device write hook.
  */
  SERIAL_PORT_WRITE            WriteBytes;

  /*
    UART device read hook.
  */
  SERIAL_PORT_READ             ReadBytes;

  /*
    UART device poll hook.
  */
  SERIAL_PORT_POLL             Poll;

  /*
    UART device configuration data. Convert to a device-specific type before
    use.
  */
  SERIAL_PORT_DEVICE_CONFIG    DeviceConfig;
} SERIAL_PORT_CONFIG_SINGLE;

typedef struct {
  /*
    Array of all UART device configurations.
  */
  SERIAL_PORT_CONFIG_SINGLE    Configurations[SERIAL_PORT_CONFIG_MAX];

  /*
    Number of UART devices present.
  */
  UINT8                        Count;

  BOOLEAN                      InitializeDevices;
} SERIAL_PORT_CONFIG;

#endif // _SERIAL_PORT_CONFIG_H
