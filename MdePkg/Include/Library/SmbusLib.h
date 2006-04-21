/** @file
	SMBUS Functions

	Copyright (c) 2006, Intel Corporation
	All rights reserved. This program and the accompanying materials
	are licensed and made available under the terms and conditions of the BSD License
	which accompanies this distribution.  The full text of the license may be found at
	http://opensource.org/licenses/bsd-license.php

	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

	Module Name:	SmbusLib.h

**/

#ifndef __SMBUS_LIB__
#define __SMBUS_LIB__

/**
  Macro that converts SMBUS slave address, SMBUS command, SMBUS data length,
  and PEC to a value that can be passed to the SMBUS Library functions.

  Computes an address that is compatible with the SMBUS Library functions.
  The unused upper bits of SlaveAddress, Command, and Length are stripped
  prior to the generation of the address.
  
  @param  SlaveAddress    SMBUS Slave Address.  Range 0..127.
  @param  Command         SMBUS Command.  Range 0..255.
  @param  Length          SMBUS Data Length.  Range 0..32.
  @param  Pec             TRUE if Packet Error Checking is enabled.  Otherwise FALSE.

**/
#define SMBUS_LIB_ADDRESS(SlaveAddress,Command,Length,Pec)  \
  ( ((Pec) ? MAX_BIT : 0)          | \
    (((SlaveAddress) & 0x7f) << 1) | \
    (((Command) & 0xff) << 8)      | \
    (((Length) & 0x1f) << 16)        \
  )

/**
  Executes an SMBUS quick read command.

  Executes an SMBUS quick read command on the SMBUS device specified by SmBusAddress.
  Only the SMBUS slave address field of SmBusAddress is required.
  If Status is not NULL, then the status of the executed command is returned in Status.
  If PEC is set in SmBusAddress, then ASSERT().
  If Command in SmBusAddress is not zero, then ASSERT().
  If Length in SmBusAddress is not zero, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

**/
VOID
EFIAPI
SmBusQuickRead (
  IN  UINTN                     SmBusAddress,
  OUT RETURN_STATUS             *Status       OPTIONAL
  )
;

/**
  Executes an SMBUS quick write command.

  Executes an SMBUS quick write command on the SMBUS device specified by SmBusAddress.
  Only the SMBUS slave address field of SmBusAddress is required.
  If Status is not NULL, then the status of the executed command is returned in Status.
  If PEC is set in SmBusAddress, then ASSERT().
  If Command in SmBusAddress is not zero, then ASSERT().
  If Length in SmBusAddress is not zero, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

**/
BOOLEAN
EFIAPI
SmBusQuickWrite (
  IN  UINTN                     SmBusAddress,
  OUT RETURN_STATUS             *Status       OPTIONAL
  )
;

/**
  Executes an SMBUS receive byte command.

  Executes an SMBUS receive byte command on the SMBUS device specified by SmBusAddress.
  Only the SMBUS slave address field of SmBusAddress is required.
  The byte received from the SMBUS is returned.
  If Status is not NULL, then the status of the executed command is returned in Status.
  If Command in SmBusAddress is not zero, then ASSERT().
  If Length in SmBusAddress is not zero, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The byte received from the SMBUS.

**/
UINT8
EFIAPI
SmBusReceiveByte (
  IN  UINTN          SmBusAddress,
  OUT RETURN_STATUS  *Status        OPTIONAL
  )
;

/**
  Executes an SMBUS send byte command.

  Executes an SMBUS send byte command on the SMBUS device specified by SmBusAddress.
  The byte specified by Value is sent.
  Only the SMBUS slave address field of SmBusAddress is required.  Value is returned.
  If Status is not NULL, then the status of the executed command is returned in Status.
  If Command in SmBusAddress is not zero, then ASSERT().
  If Length in SmBusAddress is not zero, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Value           The 8-bit value to send.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The parameter of Value.

**/
UINT8
EFIAPI
SmBusSendByte (
  IN  UINTN          SmBusAddress,
  IN  UINT8          Value,
  OUT RETURN_STATUS  *Status        OPTIONAL
  )
;

/**
  Executes an SMBUS read data byte command.

  Executes an SMBUS read data byte command on the SMBUS device specified by SmBusAddress.
  Only the SMBUS slave address and SMBUS command fields of SmBusAddress are required.
  The 8-bit value read from the SMBUS is returned.
  If Status is not NULL, then the status of the executed command is returned in Status.
  If Length in SmBusAddress is not zero, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The byte read from the SMBUS.

**/
UINT8
EFIAPI
SmBusReadDataByte (
  IN  UINTN          SmBusAddress,
  OUT RETURN_STATUS  *Status        OPTIONAL
  )
;

/**
  Executes an SMBUS write data byte command.

  Executes an SMBUS write data byte command on the SMBUS device specified by SmBusAddress.
  The 8-bit value specified by Value is written.
  Only the SMBUS slave address and SMBUS command fields of SmBusAddress are required.
  Value is returned.
  If Status is not NULL, then the status of the executed command is returned in Status.
  If Length in SmBusAddress is not zero, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Value           The 8-bit value to write.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The parameter of Value.

**/
UINT8
EFIAPI
SmBusWriteDataByte (
  IN  UINTN          SmBusAddress,
  IN  UINT8          Value,
  OUT RETURN_STATUS  *Status        OPTIONAL
  )
;

/**
  Executes an SMBUS read data word command.

  Executes an SMBUS read data word command on the SMBUS device specified by SmBusAddress.
  Only the SMBUS slave address and SMBUS command fields of SmBusAddress are required.
  The 16-bit value read from the SMBUS is returned.
  If Status is not NULL, then the status of the executed command is returned in Status.
  If Length in SmBusAddress is not zero, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().
  
  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The byte read from the SMBUS.

**/
UINT16
EFIAPI
SmBusReadDataWord (
  IN  UINTN          SmBusAddress,
  OUT RETURN_STATUS  *Status        OPTIONAL
  )
;

/**
  Executes an SMBUS write data word command.

  Executes an SMBUS write data word command on the SMBUS device specified by SmBusAddress.
  The 16-bit value specified by Value is written.
  Only the SMBUS slave address and SMBUS command fields of SmBusAddress are required.
  Value is returned.
  If Status is not NULL, then the status of the executed command is returned in Status.
  If Length in SmBusAddress is not zero, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Value           The 16-bit value to write.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The parameter of Value.

**/
UINT16
EFIAPI
SmBusWriteDataWord (
  IN  UINTN          SmBusAddress,
  IN  UINT16         Value,
  OUT RETURN_STATUS  *Status        OPTIONAL
  )
;

/**
  Executes an SMBUS process call command.

  Executes an SMBUS process call command on the SMBUS device specified by SmBusAddress.
  The 16-bit value specified by Value is written.
  Only the SMBUS slave address and SMBUS command fields of SmBusAddress are required.
  The 16-bit value returned by the process call command is returned.
  If Status is not NULL, then the status of the executed command is returned in Status.
  If Length in SmBusAddress is not zero, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Value           The 16-bit value to write.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The 16-bit value returned by the process call command.

**/
UINT16
EFIAPI
SmBusProcessCall (
  IN  UINTN          SmBusAddress,
  IN  UINT16         Value,
  OUT RETURN_STATUS  *Status        OPTIONAL
  )
;

/**
  Executes an SMBUS read block command.

  Executes an SMBUS read block command on the SMBUS device specified by SmBusAddress.
  Only the SMBUS slave address and SMBUS command fields of SmBusAddress are required.
  Bytes are read from the SMBUS and stored in Buffer.
  The number of bytes read is returned, and will never return a value larger than 32-bytes.
  If Status is not NULL, then the status of the executed command is returned in Status.
  It is the caller¡¯s responsibility to make sure Buffer is large enough for the total number of bytes read.
  SMBUS supports a maximum transfer size of 32 bytes, so Buffer does not need to be any larger than 32 bytes.
  If Length in SmBusAddress is not zero, then ASSERT().
  If Buffer is NULL, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Buffer          Pointer to the buffer to store the bytes read from the SMBUS.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The number of bytes read.

**/
UINTN
EFIAPI
SmBusReadBlock (
  IN  UINTN          SmBusAddress,
  OUT VOID           *Buffer,
  OUT RETURN_STATUS  *Status        OPTIONAL
  )
;

/**
  Executes an SMBUS write block command.

  Executes an SMBUS write block command on the SMBUS device specified by SmBusAddress.
  The SMBUS slave address, SMBUS command, and SMBUS length fields of SmBusAddress are required.
  Bytes are written to the SMBUS from Buffer.
  The number of bytes written is returned, and will never return a value larger than 32-bytes.
  If Status is not NULL, then the status of the executed command is returned in Status.  
  If Length in SmBusAddress is zero or greater than 32, then ASSERT().
  If Buffer is NULL, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Buffer          Pointer to the buffer to store the bytes read from the SMBUS.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The number of bytes written.

**/
UINTN
EFIAPI
SmBusWriteBlock (
  IN  UINTN          SmBusAddress,
  OUT VOID           *Buffer,
  OUT RETURN_STATUS  *Status        OPTIONAL
  )
;

/**
  Executes an SMBUS block process call command.

  Executes an SMBUS block process call command on the SMBUS device specified by SmBusAddress.
  The SMBUS slave address, SMBUS command, and SMBUS length fields of SmBusAddress are required.
  Bytes are written to the SMBUS from OutBuffer.  Bytes are then read from the SMBUS into InBuffer.
  If Status is not NULL, then the status of the executed command is returned in Status.
  It is the caller¡¯s responsibility to make sure InBuffer is large enough for the total number of bytes read.
  SMBUS supports a maximum transfer size of 32 bytes, so Buffer does not need to be any larger than 32 bytes.
  If OutBuffer is NULL, then ASSERT().
  If InBuffer is NULL, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().


  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  OutBuffer       Pointer to the buffer of bytes to write to the SMBUS.
  @param  InBuffer        Pointer to the buffer of bytes to read from the SMBUS.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The number of bytes written.

**/
UINTN
EFIAPI
SmBusBlockProcessCall (
  IN  UINTN          SmBusAddress,
  OUT VOID           *OutBuffer,
  OUT VOID           *InBuffer,
  OUT RETURN_STATUS  *Status        OPTIONAL
  )
;

/**
  Enumerates the SMBUS and assigns slave addresses.

  Executes the SMBUS enumeration algorithm and assigns a valid address to all SMBUS slave devices.
  The total number of SMBUS slave devices detected is returned.
  The status of the executed command is returned.
  If Slave Address in SmBusAddress is not zero, then ASSERT().
  If Command in SmBusAddress is not zero, then ASSERT().
  If Length in SmBusAddress is not zero, then ASSERT().
  If PEC in SmBusAddress is set, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress        Address that encodes the SMBUS Slave Address,
                              SMBUS Command, SMBUS Data Length, and PEC.

  @retval RETURN_SUCCESS      The SMBUS command was executed.
  @retval RETURN_TIMEOUT      A timeout occurred while executing the SMBUS command.
  @retval RETURN_DEVICE_ERROR The request was not completed because a failure reflected
                              in the Host Status Register bit.
                              Device errors are a result of a transaction collision, illegal command field,
                              unclaimed cycle (host initiated), or bus errors (collisions).

**/
RETURN_STATUS
EFIAPI
SmBusArpAll (
  IN UINTN  SmBusAddress
  )
;

/**
  Assigns an SMBUS slave addresses.

  Assigns the SMBUS device specified by Uuid the slave address specified by SmBusAddress.
  The status of the executed command is returned.
  If Command in SmBusAddress is not zero, then ASSERT().
  If Length in SmBusAddress is not zero, then ASSERT().
  If PEC in SmBusAddress is set, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress        Address that encodes the SMBUS Slave Address,
                              SMBUS Command, SMBUS Data Length, and PEC.
  @param  Uuid                Pointer to the UUID of the device to assign a slave address.

  @retval RETURN_SUCCESS      The SMBUS command was executed.
  @retval RETURN_TIMEOUT      A timeout occurred while executing the SMBUS command.
  @retval RETURN_DEVICE_ERROR The request was not completed because a failure reflected
                              in the Host Status Register bit.
                              Device errors are a result of a transaction collision, illegal command field,
                              unclaimed cycle (host initiated), or bus errors (collisions).

**/
RETURN_STATUS
EFIAPI
SmBusArpDevice (
  IN UINTN       SmBusAddress,
  IN CONST GUID  *Uuid
  )
;

/**
  Retrieves the UUID associated with an SMBUS slave device.

  Retrieves the UUID associated with the slave address specified
  by SmBusAddress and returns the UUID in Uuid.
  The status of the executed command is returned.
  If Command in SmBusAddress is not zero, then ASSERT().
  If Length in SmBusAddress is not zero, then ASSERT().
  If PEC in SmBusAddress is set, then ASSERT().
  If Uuid is NULL, then ASSERT().
  If any reserved bits of SmBusAddress are set, then ASSERT().

  @param  SmBusAddress        Address that encodes the SMBUS Slave Address,
                              SMBUS Command, SMBUS Data Length, and PEC.
  @param  Uuid                Pointer to the UUID retrieved from the SMBUS slave device.

  @retval RETURN_SUCCESS      The SMBUS command was executed.
  @retval RETURN_TIMEOUT      A timeout occurred while executing the SMBUS command.
  @retval RETURN_DEVICE_ERROR The request was not completed because a failure reflected
                              in the Host Status Register bit.
                              Device errors are a result of a transaction collision, illegal command field,
                              unclaimed cycle (host initiated), or bus errors (collisions).

**/
RETURN_STATUS
EFIAPI
SmBusGetUuid (
  IN  UINTN  SmBusAddress,
  OUT GUID   *Uuid
  )
;

#endif
