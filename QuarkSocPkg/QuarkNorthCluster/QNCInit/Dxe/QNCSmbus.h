/** @file
Common definitons for SMBus PEIM/DXE driver. Smbus PEI and DXE
modules share the same version of this file.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _QNC_SMBUS_H_
#define _QNC_SMBUS_H_

#include "CommonHeader.h"

//
// Minimum and maximum length for SMBus bus block protocols defined in SMBus spec 2.0.
//
#define MIN_SMBUS_BLOCK_LEN               1
#define MAX_SMBUS_BLOCK_LEN               32
#define ADD_LENGTH(SmbusAddress, Length)  ((SmbusAddress) + SMBUS_LIB_ADDRESS (0, 0, (Length), FALSE))

/**
  Executes an SMBus operation to an SMBus controller. Returns when either the command has been
  executed or an error is encountered in doing the operation.

  The internal worker function provides a standard way to execute an operation as defined in the
  System Management Bus (SMBus) Specification. The resulting transaction will be either that the
  SMBus slave devices accept this transaction or that this function returns with error.

  @param  SlaveAddress            The SMBus slave address of the device with which to communicate.
  @param  Command                 This command is transmitted by the SMBus host controller to the
                                  SMBus slave device and the interpretation is SMBus slave device
                                  specific. It can mean the offset to a list of functions inside an
                                  SMBus slave device. Not all operations or slave devices support
                                  this command's registers.
  @param  Operation               Signifies which particular SMBus hardware protocol instance that
                                  it will use to execute the SMBus transactions. This SMBus
                                  hardware protocol is defined by the SMBus Specification and is
                                  not related to EFI.
  @param  PecCheck                Defines if Packet Error Code (PEC) checking is required for this
                                  operation.
  @param  Length                  Signifies the number of bytes that this operation will do. The
                                  maximum number of bytes can be revision specific and operation
                                  specific. This field will contain the actual number of bytes that
                                  are executed for this operation. Not all operations require this
                                  argument.
  @param  Buffer                  Contains the value of data to execute to the SMBus slave device.
                                  Not all operations require this argument. The length of this
                                  buffer is identified by Length.

  @retval EFI_SUCCESS             The last data that was returned from the access matched the poll
                                  exit criteria.
  @retval EFI_CRC_ERROR           Checksum is not correct (PEC is incorrect).
  @retval EFI_TIMEOUT             Timeout expired before the operation was completed. Timeout is
                                  determined by the SMBus host controller device.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR        The request was not completed because a failure that was
                                  reflected in the Host Status Register bit. Device errors are a
                                  result of a transaction collision, illegal command field,
                                  unclaimed cycle (host initiated), or bus errors (collisions).
  @retval EFI_INVALID_PARAMETER   Operation is not defined in EFI_SMBUS_OPERATION.
  @retval EFI_INVALID_PARAMETER   Length/Buffer is NULL for operations except for EfiSmbusQuickRead
                                  and EfiSmbusQuickWrite. Length is outside the range of valid
                                  values.
  @retval EFI_UNSUPPORTED         The SMBus operation or PEC is not supported.
  @retval EFI_BUFFER_TOO_SMALL    Buffer is not sufficient for this operation.

**/
EFI_STATUS
Execute (
  IN     EFI_SMBUS_DEVICE_ADDRESS SlaveAddress,
  IN     EFI_SMBUS_DEVICE_COMMAND Command,
  IN     EFI_SMBUS_OPERATION      Operation,
  IN     BOOLEAN                  PecCheck,
  IN OUT UINTN                    *Length,
  IN OUT VOID                     *Buffer
  );

#endif
