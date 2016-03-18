/** @file
  I2c Bus byte read/write functions.

  Copyright (c) 2008 - 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CIRRUS_LOGIC_I2C_H_
#define _CIRRUS_LOGIC_I2C_H_

#include <Protocol/PciIo.h>

/**
  Read one byte data on I2C Bus.

  Read one byte data from the slave device connectet to I2C Bus.
  If Data is NULL, then ASSERT().

  @param  PciIo              The pointer to PCI_IO_PROTOCOL.
  @param  DeviceAddress      Slave device's address.
  @param  RegisterAddress    The register address on slave device.
  @param  Data               The pointer to returned data if EFI_SUCCESS returned.

  @retval EFI_DEVICE_ERROR
  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
I2cReadByte (
  EFI_PCI_IO_PROTOCOL    *PciIo,
  UINT8                  DeviceAddress,
  UINT8                  RegisterAddress,
  UINT8                  *Data
  );

/**
  Write one byte data onto I2C Bus.

  Write one byte data to the slave device connectet to I2C Bus.
  If Data is NULL, then ASSERT().

  @param  PciIo              The pointer to PCI_IO_PROTOCOL.
  @param  DeviceAddress      Slave device's address.
  @param  RegisterAddress    The register address on slave device.
  @param  Data               The pointer to write data.

  @retval EFI_DEVICE_ERROR
  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
I2cWriteByte (
  EFI_PCI_IO_PROTOCOL    *PciIo,
  UINT8                  DeviceAddress,
  UINT8                  RegisterAddress,
  UINT8                  *Data
  );

#endif
