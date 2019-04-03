/** @file
Provides definition of entry point to the common I2C module that produces
common I2C Controller functions used by I2C library services.


Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef _I2CCOMMON_H_
#define _I2CCOMMON_H_

#include <Uefi.h>
#include <Base.h>

#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/I2cLib.h>
#include <IohAccess.h>
#include <IohCommonDefinitions.h>
#include "I2cRegs.h"

//
// Constants that define I2C Controller timeout and max. polling time.
//
#define MAX_T_POLL_COUNT         100
#define TI2C_POLL                25  // microseconds
#define MAX_STOP_DET_POLL_COUNT ((1000 * 1000) / TI2C_POLL)  // Extreme for expected Stop detect.

/**
  The GetI2CIoPortBaseAddress() function gets IO port base address of I2C Controller.

  Always reads PCI configuration space to get MMIO base address of I2C Controller.

  @return The IO port base address of I2C controller.

**/
UINTN
GetI2CIoPortBaseAddress (
  VOID
  );

/**
  The EnableI2CMmioSpace() function enables access to I2C MMIO space.

**/
VOID
EnableI2CMmioSpace (
  VOID
  );

/**
  The DisableI2CController() functions disables I2C Controller.

**/
VOID
DisableI2CController (
  VOID
  );

/**
  The EnableI2CController() function enables the I2C Controller.

**/
VOID
EnableI2CController (
  VOID
  );

/**
  The WaitForStopDet() function waits until I2C STOP Condition occurs,
  indicating transfer completion.

  @retval EFI_SUCCESS           Stop detected.
  @retval EFI_TIMEOUT           Timeout while waiting for stop condition.
  @retval EFI_ABORTED           Tx abort signaled in HW status register.
  @retval EFI_DEVICE_ERROR      Tx or Rx overflow detected.

**/
EFI_STATUS
WaitForStopDet (
  VOID
  );

/**

  The InitializeInternal() function initialises internal I2C Controller
  register values that are commonly required for I2C Write and Read transfers.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @retval EFI_SUCCESS           I2C Operation completed successfully.

**/
EFI_STATUS
InitializeInternal (
  IN EFI_I2C_ADDR_MODE  AddrMode
  );

/**

  The WriteByte() function provides a standard way to execute a
  standard single byte write to an IC2 device (without accessing
  sub-addresses), as defined in the I2C Specification.

  @param  I2CAddress      I2C Slave device address
  @param  Value           The 8-bit value to write.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.

**/
EFI_STATUS
EFIAPI
WriteByte (
  IN  UINTN          I2CAddress,
  IN  UINT8          Value
  );

/**

  The ReadByte() function provides a standard way to execute a
  standard single byte read to an IC2 device (without accessing
  sub-addresses), as defined in the I2C Specification.

  @param  I2CAddress      I2C Slave device address
  @param  ReturnDataPtr   Pointer to location to receive read byte.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.

**/
EFI_STATUS
EFIAPI
ReadByte (
  IN  UINTN          I2CAddress,
  OUT UINT8          *ReturnDataPtr
  );

/**

  The WriteMultipleByte() function provides a standard way to execute
  multiple byte writes to an IC2 device (e.g. when accessing sub-addresses or
  when writing block of data), as defined in the I2C Specification.

  @param I2CAddress   The I2C slave address of the device
                      with which to communicate.

  @param WriteBuffer  Contains the value of byte to be written to the
                      I2C slave device.

  @param Length       No. of bytes to be written.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Tx abort signaled in HW status register.
  @retval EFI_DEVICE_ERROR      Tx overflow detected.

**/
EFI_STATUS
EFIAPI
WriteMultipleByte (
  IN  UINTN          I2CAddress,
  IN  UINT8          *WriteBuffer,
  IN  UINTN          Length
  );

/**

  The ReadMultipleByte() function provides a standard way to execute
  multiple byte writes to an IC2 device (e.g. when accessing sub-addresses or
  when reading block of data), as defined in the I2C Specification (I2C combined
  write/read protocol).

  @param I2CAddress   The I2C slave address of the device
                      with which to communicate.

  @param Buffer       Contains the value of byte data written or read from the
                      I2C slave device.

  @param WriteLength  No. of bytes to be written. In this case data
                      written typically contains sub-address or sub-addresses
                      in Hi-Lo format, that need to be read (I2C combined
                      write/read protocol).

  @param ReadLength   No. of bytes to be read from I2C slave device.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Tx abort signaled in HW status register.
  @retval EFI_DEVICE_ERROR      Rx underflow or Rx/Tx overflow detected.

**/
EFI_STATUS
EFIAPI
ReadMultipleByte (
  IN  UINTN          I2CAddress,
  IN  OUT UINT8      *Buffer,
  IN  UINTN          WriteLength,
  IN  UINTN          ReadLength
  );

#endif
