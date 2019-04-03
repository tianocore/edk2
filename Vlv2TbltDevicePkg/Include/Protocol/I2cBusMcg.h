/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

**/

#ifndef __I2C_BUS_H__
#define __I2C_BUS_H__

#include <Protocol/I2cHostMcg.h>

//
// I2C bus protocol
//
typedef struct _EFI_I2C_BUS_PROTOCOL  EFI_I2C_BUS_PROTOCOL;

/**
  Perform an I2C operation on the device

  This routine must be called at or below TPL_NOTIFY.  For synchronous
  requests this routine must be called at or below TPL_CALLBACK.

  N.B. The typical consumers of this API are the third party I2C
  drivers.  Extreme care must be taken by other consumers of this
  API to prevent confusing the third party I2C drivers due to a
  state change at the I2C device which the third party I2C drivers
  did not initiate.  I2C platform drivers may use this API within
  these guidelines.

  This routine queues an operation to the I2C controller for execution
  on the I2C bus.

  As an upper layer driver writer, the following need to be provided
  to the platform vendor:

  1.  ACPI CID value or string - this is used to connect the upper layer
      driver to the device.
  2.  Slave address array guidance when the I2C device uses more than one
      slave address.  This is used to access the blocks of hardware within
      the I2C device.

  @param[in] This               Address of an EFI_I2C_BUS_PROTOCOL
                                structure
  @param[in] SlaveAddressIndex  Index into an array of slave addresses for
                                the I2C device.  The values in the array are
                                specified by the board designer, with the
                                I2C device driver writer providing the slave
                                address order.

                                For devices that have a single slave address,
                                this value must be zero.  If the I2C device
                                uses more than one slave address then the third
                                party (upper level) I2C driver writer needs to
                                specify the order of entries in the slave address
                                array.

                                \ref ThirdPartyI2cDrivers "Third Party I2C Drivers"
                                section in I2cMaster.h.
  @param[in] Event              Event to set for asynchronous operations,
                                NULL for synchronous operations
  @param[in] RequestPacket      Address of an EFI_I2C_REQUEST_PACKET
                                structure describing the I2C operation
  @param[out] I2cStatus         Optional buffer to receive the I2C operation
                                completion status

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_ABORTED           The request did not complete because the driver
                                was shutdown.
  @retval EFI_ACCESS_DENIED     Invalid SlaveAddressIndex value
  @retval EFI_BAD_BUFFER_SIZE   The WriteBytes or ReadBytes buffer size is too large.
  @retval EFI_DEVICE_ERROR      There was an I2C error (NACK) during the operation.
                                This could indicate the slave device is not present.
  @retval EFI_INVALID_PARAMETER RequestPacket is NULL
  @retval EFI_INVALID_PARAMETER TPL is too high
  @retval EFI_NO_RESPONSE       The I2C device is not responding to the
                                slave address.  EFI_DEVICE_ERROR may also be
                                returned if the controller can not distinguish
                                when the NACK occurred.
  @retval EFI_NOT_FOUND         I2C slave address exceeds maximum address
  @retval EFI_NOT_READY         I2C bus is busy or operation pending, wait for
                                the event and then read status pointed to by
                                the request packet.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory for I2C operation
  @retval EFI_TIMEOUT           The transaction did not complete within an internally
                                specified timeout period.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_I2C_BUS_START_REQUEST) (
  IN CONST EFI_I2C_BUS_PROTOCOL *This,
  IN UINTN SlaveAddressIndex,
  IN EFI_EVENT Event OPTIONAL,
  IN CONST EFI_I2C_REQUEST_PACKET *RequestPacket,
  OUT EFI_STATUS *I2cStatus OPTIONAL
  );

//
// The I2C bus protocol enables access to a specific device on the I2C bus.
//
// Each I2C device is described as an ACPI node (HID, UID and CID) within the
// platform layer.  The I2C bus protocol enumerates the I2C devices in the
// platform and creates a unique handle and device path for each I2C device.
//
// I2C slave addressing is abstracted to validate addresses and limit operation
// to the specified I2C device.  The third party providing the I2C device support
// provides an ordered list of slave addresses for the I2C device to the team
// building the platform layer.  The platform team must preserve the order of the
// supplied list.  SlaveAddressCount is the number of entries in this list or
// array within the platform layer.  The third party device support references
// a slave address using an index into the list or array in the range of zero
// to SlaveAddressCount - 1.
//
struct _EFI_I2C_BUS_PROTOCOL {
  //
  // Start an I2C operation on the bus
  //
  EFI_I2C_BUS_START_REQUEST StartRequest;

  //
  // The maximum number of slave addresses for the I2C device.  The caller may
  // validate this value as a check on the platform layer's configuration.  Slave
  // address selection uses an index value in the range of zero to SlaveAddressCount - 1.
  //
  UINTN SlaveAddressCount;

  //
  // Hardware revision - Matches the ACPI _HRV value
  //
  // The HardwareRevision value allows a single driver to support multiple hardware
  // revisions and implement the necessary workarounds for limitations within the
  // hardware.
  //
  UINT32 HardwareRevision;

  //
  // The maximum number of bytes the I2C host controller
  // is able to receive from the I2C bus.
  ///
  UINT32 MaximumReceiveBytes;

  //
  // The maximum number of bytes the I2C host controller
  // is able to send on the I2C bus.
  //
  UINT32 MaximumTransmitBytes;

  //
  // The maximum number of bytes in the I2C bus transaction.
  //
  UINT32 MaximumTotalBytes;
};

//
// GUID for the I2C bus protocol
//
extern EFI_GUID gEfiI2cBusProtocolGuid;

#endif  //  __I2C_BUS_H__
