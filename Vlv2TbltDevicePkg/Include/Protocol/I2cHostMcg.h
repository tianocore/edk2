/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#ifndef __I2C_HOST_H__
#define __I2C_HOST_H__

#include <Protocol/I2cMasterMcg.h>

/**
  Declare the forward references

**/
typedef struct _EFI_I2C_HOST_PROTOCOL EFI_I2C_HOST_PROTOCOL;
typedef struct _EFI_I2C_HOST_CALLBACKS EFI_I2C_HOST_CALLBACKS;


/**
  Queue an I2C operation for execution on the I2C controller.

  This routine must be called at or below TPL_NOTIFY.  For synchronous
  requests this routine must be called at or below TPL_CALLBACK.

  N.B. The typical consumers of this API are the I2C bus driver and
  on rare occasions the I2C test application.  Extreme care must be
  taken by other consumers of this API to prevent confusing the
  third party I2C drivers due to a state change at the I2C device
  which the third party I2C drivers did not initiate.  I2C platform
  drivers may use this API within these guidelines.

  This layer uses the concept of I2C bus configurations to describe
  the I2C bus.  An I2C bus configuration is defined as a unique
  setting of the multiplexers and switches in the I2C bus which
  enable access to one or more I2C devices.  When using a switch
  to divide a bus, due to speed differences, the I2C platform layer
  would define an I2C bus configuration for the I2C devices on each
  side of the switch.  When using a multiplexer, the I2C platform
  layer defines an I2C bus configuration for each of the selector
  values required to control the multiplexer.  See Figure 1 in the
  <a href="http://www.nxp.com/documents/user_manual/UM10204.pdf">I<sup>2</sup>C
  Specification</a> for a complex I2C bus configuration.

  The I2C host driver processes all operations in FIFO order.  Prior to
  performing the operation, the I2C host driver calls the I2C platform
  driver to reconfigure the switches and multiplexers in the I2C bus
  enabling access to the specified I2C device.  The I2C platform driver
  also selects the maximum bus speed for the device.  After the I2C bus
  is configured, the I2C host driver calls the I2C port driver to
  initialize the I2C controller and start the I2C operation.

  @param[in] This             Address of an EFI_I2C_HOST_PROTOCOL instance.
  @param[in] I2cBusConfiguration  I2C bus configuration to access the I2C
                                  device.
  @param[in] SlaveAddress     Address of the device on the I2C bus.
  @param[in] Event            Event to set for asynchronous operations,
                              NULL for synchronous operations
  @param[in] RequestPacket    Address of an EFI_I2C_REQUEST_PACKET
                              structure describing the I2C operation
  @param[out] I2cStatus       Optional buffer to receive the I2C operation
                              completion status

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_ABORTED           The request did not complete because the driver
                                was shutdown.
  @retval EFI_BAD_BUFFER_SIZE   The WriteBytes or ReadBytes buffer size is too large.
  @retval EFI_DEVICE_ERROR      There was an I2C error (NACK) during the operation.
                                This could indicate the slave device is not present.
  @retval EFI_INVALID_PARAMETER RequestPacket is NULL
  @retval EFI_INVALID_PARAMETER TPL is too high
  @retval EFI_NO_MAPPING        Invalid I2cBusConfiguration value
  @retval EFI_NO_RESPONSE       The I2C device is not responding to the
                                slave address.  EFI_DEVICE_ERROR may also be
                                returned if the controller cannot distinguish
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
(EFIAPI *EFI_I2C_HOST_QUEUE_REQUEST) (
  IN CONST EFI_I2C_HOST_PROTOCOL *This,
  IN UINTN I2cBusConfiguration,
  IN UINTN SlaveAddress,
  IN EFI_EVENT Event OPTIONAL,
  IN CONST EFI_I2C_REQUEST_PACKET *RequestPacket,
  OUT EFI_STATUS *I2cStatus OPTIONAL
  );

///
/// Host access to the I2C bus.
///
struct _EFI_I2C_HOST_PROTOCOL {
  ///
  /// Queue an operation for execution on the I2C bus
  ///
  EFI_I2C_HOST_QUEUE_REQUEST QueueRequest;

  ///
  /// The maximum number of I2C bus configurations
  ///
  UINTN I2cBusConfigurationCount;

  ///
  /// The maximum number of bytes the I2C host controller
  /// is able to receive from the I2C bus.
  ///
  UINT32 MaximumReceiveBytes;

  ///
  /// The maximum number of bytes the I2C host controller
  /// is able to send on the I2C bus.
  ///
  UINT32 MaximumTransmitBytes;

  ///
  /// The maximum number of bytes in the I2C bus transaction.
  ///
  UINT32 MaximumTotalBytes;
};

///
/// GUID for the EFI_I2C_HOST_PROTOCOL
///
extern EFI_GUID gEfiI2cHostProtocolGuid;

#endif  //  __I2C_HOST_H__
