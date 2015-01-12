/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


**/

#ifndef __I2C_SLAVE_H__
#define __I2C_SLAVE_H__

#include <Protocol/I2cHostMcg.h>

/**
  Declare the forward references

**/
typedef struct _EFI_I2C_SLAVE_PROTOCOL    EFI_I2C_SLAVE_PROTOCOL;   ///<  I2C slave protocol

/**
  The I2C controller received a data byte from the
  I2C msster.

  @param[in] Context        The value passed to the slave enable routine.
  @param[in] NumberOfBytes  Number of data bytes received
  @param[in] Data           Buffer containing the received data bytes

  @retval EFI_SUCCESS       ACK the data byte
  @retval EFI_UNSUPPORTED   NACK the data byte

**/
typedef
EFI_STATUS
(EFIAPI *EFI_I2C_SLAVE_RECEIVE_DATA) (
  IN VOID *Context,
  IN UINTN NumberOfBytes,
  IN CONST UINT8 *Data
  );

/**
  The I2C controller received the start bit from the
  I2C master.

  @param[in] Context        The value passed to the slave enable routine.

**/
typedef
VOID
(EFIAPI *EFI_I2C_SLAVE_RECEIVE_START) (
  IN VOID *Context,
  IN UINTN BytesSent,
  IN EFI_STATUS Status
  );

/**
  The I2C controller received the stop bit from the
  I2C master.

  @param[in] Context        The value passed to the slave enable routine.
  @param[in] BytesSent      Number of bytes successfully ACKed by the
                            I2C master.  This is a hint, not all I2C
                            controllers support the ability to return
                            the number of bytes sent.  When it is not
                            possible, the port driver returns zero.
  @param[in] Status         <ul>
                              <li>EFI_SUCCESS - All of the data was successfully sent</li>
                              <li>EFI_ABORTED - The controller was reset</li>
                              <li>EFI_DEVICE_ERROR - A NACK was received when sending the data.</li>
                              <li>EFI_END_OF_FILE - The stop bit was received before all of
                                the data was sent.</li>
                            </ul>

**/
typedef
VOID
(EFIAPI *EFI_I2C_SLAVE_RECEIVE_STOP) (
  IN VOID *Context,
  IN UINTN BytesSent,
  IN EFI_STATUS Status
  );

/**
  Enable or disable I2C slave operation.

  The ReceiveData callback allows the port driver to return data
  to the driver or application handling slave mode operations.  This
  is data that a remote master has sent to the local I2C controller.
  The data may be returned one byte at a time if the controller supports
  the ability to ACK/NACK on each receive byte.  If not, a block of
  data may be returned by the I2C port driver and the ACK/NACK status
  is used only as a hint for the port driver.

  The slave mode driver or application should buffer the data until
  either ReceiveStart or ReceiveStop is called.  At that time all of
  the data is received and the command may be processed.

  ReceiveStart is called when the I2C master is expecting a response.
  After processing the command, but before sending the response the
  slave driver or application should mark the command as processed to
  avoid processing it a second time when ReceiveStop is called.  The
  slave driver or application then calls SendData to send to queue the
  response data for transmission.  The data must remain valid in the
  WriteBuffer until ReceiveStop is called.

  ReceiveStop is called when the stop bit is received on the I2C bus.
  The slave driver or application starts processing the command if an
  command data is pending in the slave driver's or application's buffer.
  The BytesSent value is a hint to the slave driver or application as
  to how much data was returned to the I2C master.  If the controller
  does not provide this level of support then this value is set to zero.

  @param[in] This           Address of an EFI_I2C_SLAVE_PROTOCOL
                            structure
  @param[in] SlaveAddress   Slave address for the I2C controller
  @param[in] Context        Address of a context structure for use when
                            calling ReceiveData or ReceiveStop
  @param[in] ReceiveData    Called by the I2C port driver as data bytes
                            are received from the master.  Response status
                            indicates if the byte is ACKed or NACKed. When
                            data is passed back a byte at a time, the port
                            driver must hold the clock until this callback
                            returns.
  @param[in] ReceiveStart   Called when the I2C controller receives a start bit.
  @param[in] ReceiveStop    Called after all of the data bytes are
                            received.

  @retval EFI_SUCCESS       Slave operation is enabled on the controller.
  @retval EFI_UNSUPPORTED   The controller does not support this frequency.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_I2C_SLAVE_ENABLE) (
  IN CONST EFI_I2C_SLAVE_PROTOCOL *This,
  IN UINT32 SlaveAddress,
  IN VOID *Context,
  IN EFI_I2C_SLAVE_RECEIVE_DATA ReceiveData,
  IN EFI_I2C_SLAVE_RECEIVE_START ReceiveStart,
  IN EFI_I2C_SLAVE_RECEIVE_STOP ReceiveStop
  );

/**
  Send data to the I2C master.

  Port drivers may implement this as a blocking or non-blocking call.
  The data in the write buffer must remain valid until ReceiveStop or
  ReceiveStart is called indicating that the I2C master has terminated
  the transfer.

  @param[in] This         Address of an EFI_I2C_SLAVE_PROTOCOL
                          structure
  @param[in] WriteBytes   Number of bytes to write
  @param[in] WriteBuffer  Buffer containing the data to send

  @retval EFI_SUCCESS           Data waiting for master access.
  @retval EFI_INVALID_PARAMETER WriteBuffer is NULL or WriteBytes
                                is zero.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_I2C_SLAVE_SEND) (
  IN CONST EFI_I2C_SLAVE_PROTOCOL *This,
  IN UINTN WriteBytes,
  IN CONST UINT8 *WriteBuffer
  );

///
/// I2C slave protocol
///
/// The port driver publishes this protocol when slave mode is
/// supported by the controller.
///
struct _EFI_I2C_SLAVE_PROTOCOL {
  ///
  /// Enable or disable I2C slave operation
  ///
  EFI_I2C_SLAVE_ENABLE SlaveEnable;

  ///
  /// Send data to the I2C master
  ///
  EFI_I2C_SLAVE_SEND SendData;
};

///
/// GUID for the EFI_I2C_SLAVE_PROTOCOL
///
extern EFI_GUID gEfiI2cSlaveProtocolGuid;

#endif  //  __I2C_SLAVE_H__
