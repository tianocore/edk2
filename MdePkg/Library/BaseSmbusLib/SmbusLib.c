/** @file
  Base SMBUS library implementation built upon I/O library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  SmbusLib.c

**/

#include "SmbusLibRegisters.h"

#define SMBUS_LIB_SLAVE_ADDRESS(SmBusAddress)      (((SmBusAddress) >> 1)  & 0x7f)
#define SMBUS_LIB_COMMAND(SmBusAddress)            (((SmBusAddress) >> 8)  & 0xff)
#define SMBUS_LIB_LENGTH(SmBusAddress)             (((SmBusAddress) >> 16) & 0x1f)
#define SMBUS_LIB_PEC(SmBusAddress)     ((BOOLEAN) (((SmBusAddress) & SMBUS_LIB_PEC_BIT) != 0))
#define SMBUS_LIB_RESEARVED(SmBusAddress)          ((SmBusAddress) & ~(((1 << 21) - 2) | SMBUS_LIB_PEC_BIT))

//
// Replaced by PCD
//
#define ICH_SMBUS_BASE_ADDRESS               0xEFA0

/**
  Reads an 8-bit SMBUS register on ICH.

  This internal function reads an SMBUS register specified by Offset.

  @param  Offset  The offset of SMBUS register.

  @return The value read.

**/
UINT8
InternalSmBusIoRead8 (
  IN UINTN      Offset
  )
{
  return IoRead8 (ICH_SMBUS_BASE_ADDRESS + Offset);
}

/**
  Writes an 8-bit SMBUS register on ICH.

  This internal function writes an SMBUS register specified by Offset.

  @param  Offset  The offset of SMBUS register.
  @param  Value   The value to write to SMBUS register.

  @return The value written the SMBUS register.

**/
UINT8
InternalSmBusIoWrite8 (
  IN UINTN      Offset,
  IN UINT8      Value
  )
{
  return IoWrite8 (ICH_SMBUS_BASE_ADDRESS + Offset, Value);
}

/**
  Acquires the ownership of SMBUS.

  This internal function reads the host state register.
  If the SMBUS is not available, RETURN_TIMEOUT is returned;
  Otherwise, it performs some basic initializations and returns
  RETURN_SUCCESS. 

  @retval RETURN_SUCCESS    The SMBUS command was executed successfully.
  @retval RETURN_TIMEOUT    A timeout occurred while executing the SMBUS command.

**/
RETURN_STATUS
InternalSmBusAcquire (
  VOID 
  )
{
  UINT8   HostStatus;

  HostStatus = InternalSmBusIoRead8 (SMBUS_R_HST_STS);
  if ((HostStatus & SMBUS_B_INUSE_STS) != 0) {
    return RETURN_TIMEOUT;
  } else if ((HostStatus & SMBUS_B_HOST_BUSY) != 0) {
    //
    // Clear Status Register and exit
    //
    InternalSmBusIoWrite8 (SMBUS_R_HST_STS, SMBUS_B_HSTS_ALL);
    return RETURN_TIMEOUT;
  }
  //
  // Clear byte pointer of 32-byte buffer.
  //
  InternalSmBusIoRead8 (SMBUS_R_HST_CTL);
  //
  // Clear BYTE_DONE status
  //
  InternalSmBusIoWrite8 (SMBUS_R_HST_STS, SMBUS_B_BYTE_DONE_STS);
  
  return RETURN_SUCCESS;
}

/**
  Waits until the completion of SMBUS transaction.

  This internal function waits until the transaction of SMBUS is over
  by polling the INTR bit of Host status register.
  If the SMBUS is not available, RETURN_TIMEOUT is returned;
  Otherwise, it performs some basic initializations and returns
  RETURN_SUCCESS. 

  @retval RETURN_SUCCESS      The SMBUS command was executed successfully.
  @retval RETURN_CRC_ERROR    The checksum is not correct (PEC is incorrect).
  @retval RETURN_DEVICE_ERROR The request was not completed because a failure reflected
                              in the Host Status Register bit.  Device errors are
                              a result of a transaction collision, illegal command field,
                              unclaimed cycle (host initiated), or bus errors (collisions).

**/
RETURN_STATUS
InternalSmBusWait (
  VOID 
  )
{
  UINT8   HostStatus;
  UINT8   AuxiliaryStatus;
  BOOLEAN First;
  First = TRUE;

  do {
    //
    // Poll INTR bit of host status register.
    //
    HostStatus = InternalSmBusIoRead8 (SMBUS_R_HST_STS);
  } while ((HostStatus & (SMBUS_B_INTR | SMBUS_B_ERROR | SMBUS_B_BYTE_DONE_STS)) == 0);
  
  if ((HostStatus & SMBUS_B_ERROR) == 0) {
    return RETURN_SUCCESS;
  }
  //
  // Clear error bits of host status register
  //
  InternalSmBusIoWrite8 (SMBUS_R_HST_STS, SMBUS_B_ERROR);
  //
  // Read auxiliary status register to judge CRC error.
  //
  AuxiliaryStatus = InternalSmBusIoRead8 (SMBUS_R_AUX_STS);
  if ((AuxiliaryStatus & SMBUS_B_CRCE) != 0) {
    return RETURN_CRC_ERROR;
  }

  return RETURN_DEVICE_ERROR;
}

/**
  Executes an SMBUS quick read/write command.

  This internal function executes an SMBUS quick read/write command
  on the SMBUS device specified by SmBusAddress.
  Only the SMBUS slave address field of SmBusAddress is required.
  If Status is not NULL, then the status of the executed command is returned in Status.

  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

**/
VOID
EFIAPI
InternalSmBusQuick (
  IN  UINTN                     SmBusAddress,
  OUT RETURN_STATUS             *Status       OPTIONAL
  )
{
  RETURN_STATUS   ReturnStatus;

  ReturnStatus = InternalSmBusAcquire ();
  if (RETURN_ERROR (ReturnStatus)) {
    goto Done;
  }
 
  //
  // Set Command register
  //
  InternalSmBusIoWrite8 (SMBUS_R_HST_CMD, 0);
  //
  // Set Auxiliary Control register
  //
  InternalSmBusIoWrite8 (SMBUS_R_AUX_CTL, 0);
  //
  // Set SMBus slave address for the device to send/receive from
  //
  InternalSmBusIoWrite8 (SMBUS_R_XMIT_SLVA, (UINT8) SmBusAddress);
  //
  // Set Control Register (Initiate Operation, Interrupt disabled)
  //
  InternalSmBusIoWrite8 (SMBUS_R_HST_CTL, SMBUS_V_SMB_CMD_QUICK + SMBUS_B_START);

  //
  // Wait for the end
  //
  ReturnStatus = InternalSmBusWait ();

  //
  // Clear status register and exit
  //
  InternalSmBusIoWrite8 (SMBUS_R_HST_STS, SMBUS_B_HSTS_ALL);;

Done:
  if (Status != NULL) {
    *Status = ReturnStatus;
  }
}

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
{
  ASSERT (!SMBUS_LIB_PEC (SmBusAddress));
  ASSERT (SMBUS_LIB_COMMAND (SmBusAddress)   == 0);
  ASSERT (SMBUS_LIB_LENGTH (SmBusAddress)    == 0);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  InternalSmBusQuick (SmBusAddress | SMBUS_B_READ, Status);
}

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
VOID
EFIAPI
SmBusQuickWrite (
  IN  UINTN                     SmBusAddress,
  OUT RETURN_STATUS             *Status       OPTIONAL
  )
{
  ASSERT (!SMBUS_LIB_PEC (SmBusAddress));
  ASSERT (SMBUS_LIB_COMMAND (SmBusAddress)   == 0);
  ASSERT (SMBUS_LIB_LENGTH (SmBusAddress)    == 0);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  InternalSmBusQuick (SmBusAddress | SMBUS_B_WRITE, Status);
}

/**
  Executes an SMBUS byte or word command.

  This internal function executes an .
  Only the SMBUS slave address field of SmBusAddress is required.
  If Status is not NULL, then the status of the executed command is returned in Status.

  @param  HostControl     The value of Host Control Register to set.  
  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Value           The byte/word write to the SMBUS.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The byte/word read from the SMBUS.

**/
UINT16
InternalSmBusByteWord (
  IN  UINT8                     HostControl,
  IN  UINTN                     SmBusAddress,
  IN  UINT16                    Value,
  OUT RETURN_STATUS             *Status
  )
{
  RETURN_STATUS                 ReturnStatus;
  UINT8                         AuxiliaryControl;

  ReturnStatus = InternalSmBusAcquire ();
  if (RETURN_ERROR (ReturnStatus)) {
    goto Done;
  }

  AuxiliaryControl = 0;
  if (SMBUS_LIB_PEC (SmBusAddress)) {
    AuxiliaryControl |= SMBUS_B_AAC;
    HostControl      |= SMBUS_B_PEC_EN;
  }
 
  //
  // Set commond register
  //
  InternalSmBusIoWrite8 (SMBUS_R_HST_CMD, (UINT8) SMBUS_LIB_COMMAND (SmBusAddress));

  InternalSmBusIoWrite8 (SMBUS_R_HST_D0, (UINT8) Value);
  InternalSmBusIoWrite8 (SMBUS_R_HST_D1, (UINT8) (Value >> 8));

  //
  // Set Auxiliary Control Regiester.
  //
  InternalSmBusIoWrite8 (SMBUS_R_AUX_CTL, AuxiliaryControl);
  //
  // Set SMBus slave address for the device to send/receive from.
  //
  InternalSmBusIoWrite8 (SMBUS_R_XMIT_SLVA, (UINT8) SmBusAddress);
  //
  // Set Control Register (Initiate Operation, Interrupt disabled)
  //
  InternalSmBusIoWrite8 (SMBUS_R_HST_CTL, HostControl + SMBUS_B_START);

  //
  // Wait for the end
  //
  ReturnStatus = InternalSmBusWait ();
 
  Value  = InternalSmBusIoRead8 (SMBUS_R_HST_D1) << 8;
  Value |= InternalSmBusIoRead8 (SMBUS_R_HST_D0);

  //
  // Clear status register and exit
  //
  InternalSmBusIoWrite8 (SMBUS_R_HST_STS, SMBUS_B_HSTS_ALL);;

Done:
  if (Status != NULL) {
    *Status = ReturnStatus;
  }

  return Value;
}

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
{
  ASSERT (SMBUS_LIB_COMMAND (SmBusAddress)   == 0);
  ASSERT (SMBUS_LIB_LENGTH (SmBusAddress)    == 0);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  return (UINT8) InternalSmBusByteWord (
                   SMBUS_V_SMB_CMD_BYTE,
                   SmBusAddress | SMBUS_B_READ,
                   0,
                   Status
                   );
}

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
{
  ASSERT (SMBUS_LIB_COMMAND (SmBusAddress)   == 0);
  ASSERT (SMBUS_LIB_LENGTH (SmBusAddress)    == 0);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  return (UINT8) InternalSmBusByteWord (
                   SMBUS_V_SMB_CMD_BYTE,
                   SmBusAddress | SMBUS_B_WRITE,
                   Value,
                   Status
                   );
}

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
{
  ASSERT (SMBUS_LIB_LENGTH (SmBusAddress)    == 0);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  return (UINT8) InternalSmBusByteWord (
                   SMBUS_V_SMB_CMD_BYTE_DATA,
                   SmBusAddress | SMBUS_B_READ,
                   0,
                   Status
                   );
}

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
{
  ASSERT (SMBUS_LIB_LENGTH (SmBusAddress)    == 0);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  return (UINT8) InternalSmBusByteWord (
                   SMBUS_V_SMB_CMD_BYTE_DATA,
                   SmBusAddress | SMBUS_B_WRITE,
                   Value,
                   Status
                   );
}

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
{
  ASSERT (SMBUS_LIB_LENGTH (SmBusAddress)    == 0);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  return InternalSmBusByteWord (
           SMBUS_V_SMB_CMD_WORD_DATA,
           SmBusAddress | SMBUS_B_READ,
           0,
           Status
           );
}

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
{
  ASSERT (SMBUS_LIB_LENGTH (SmBusAddress)    == 0);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  return InternalSmBusByteWord (
           SMBUS_V_SMB_CMD_WORD_DATA,
           SmBusAddress | SMBUS_B_WRITE,
           Value,
           Status
           );
}

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
{
  ASSERT (SMBUS_LIB_LENGTH (SmBusAddress)    == 0);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  return InternalSmBusByteWord (
           SMBUS_V_SMB_CMD_PROCESS_CALL,
           SmBusAddress | SMBUS_B_WRITE,
           Value,
           Status
           );
}

/**
  Executes an SMBUS block command.

  Executes an SMBUS block read, block write and block write-block read command
  on the SMBUS device specified by SmBusAddress.
  Bytes are read from the SMBUS and stored in Buffer.
  The number of bytes read is returned, and will never return a value larger than 32-bytes.
  If Status is not NULL, then the status of the executed command is returned in Status.
  It is the caller¡¯s responsibility to make sure Buffer is large enough for the total number of bytes read.
  SMBUS supports a maximum transfer size of 32 bytes, so Buffer does not need to be any larger than 32 bytes.

  @param  HostControl     The value of Host Control Register to set.  
  @param  SmBusAddress    Address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  OutBuffer       Pointer to the buffer of bytes to write to the SMBUS.
  @param  InBuffer        Pointer to the buffer of bytes to read from the SMBUS.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The number of bytes read from the SMBUS.

**/
UINTN
InternalSmBusBlock (
  IN  UINT8                     HostControl,
  IN  UINTN                     SmBusAddress,
  IN  UINT8                     *OutBuffer,
  OUT UINT8                     *InBuffer,
  OUT RETURN_STATUS             *Status
  )
{
  RETURN_STATUS                 ReturnStatus;
  UINTN                         Index;
  UINTN                         BytesCount;
  UINT8                         AuxiliaryControl;

  BytesCount = SMBUS_LIB_LENGTH (SmBusAddress) + 1;

  ReturnStatus = InternalSmBusAcquire ();
  if (RETURN_ERROR (ReturnStatus)) {
    goto Done;
  }
  
  AuxiliaryControl = SMBUS_B_E32B;
  if (SMBUS_LIB_PEC (SmBusAddress)) {
    AuxiliaryControl |= SMBUS_B_AAC;
    HostControl      |= SMBUS_B_PEC_EN;
  }

  InternalSmBusIoWrite8 (SMBUS_R_HST_CMD, (UINT8) SMBUS_LIB_COMMAND (SmBusAddress));

  InternalSmBusIoWrite8 (SMBUS_R_HST_D0, (UINT8) BytesCount);

  if (OutBuffer != NULL) {
    for (Index = 0; Index < BytesCount; Index++) {
      InternalSmBusIoWrite8 (SMBUS_R_HOST_BLOCK_DB, OutBuffer[Index]);
    }
  }
  //
  // Set Auxiliary Control Regiester.
  //
  InternalSmBusIoWrite8 (SMBUS_R_AUX_CTL, AuxiliaryControl);
  //
  // Set SMBus slave address for the device to send/receive from
  //
  InternalSmBusIoWrite8 (SMBUS_R_XMIT_SLVA, (UINT8) SmBusAddress);
  //
  // Set Control Register (Initiate Operation, Interrupt disabled)
  //
  InternalSmBusIoWrite8 (SMBUS_R_HST_CTL, HostControl + SMBUS_B_START);

  //
  // Wait for the end
  //
  ReturnStatus = InternalSmBusWait ();
  if (RETURN_ERROR (ReturnStatus)) {
    goto Done;
  }

  BytesCount = InternalSmBusIoRead8 (SMBUS_R_HST_D0);
  if (InBuffer != NULL) {
    for (Index = 0; Index < BytesCount; Index++) {
      InBuffer[Index] = InternalSmBusIoRead8 (SMBUS_R_HOST_BLOCK_DB);
    }
  }

  //
  // Clear status register and exit
  //
  InternalSmBusIoWrite8 (SMBUS_R_HST_STS, SMBUS_B_HSTS_ALL);

Done:
  if (Status != NULL) {
    *Status = ReturnStatus;
  }

  return BytesCount;
}

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
{
  ASSERT (Buffer != NULL);
  ASSERT (SMBUS_LIB_LENGTH (SmBusAddress) == 0);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  return InternalSmBusBlock (
           SMBUS_V_SMB_CMD_BLOCK,
           SmBusAddress | SMBUS_B_READ,
           NULL,
           Buffer,
           Status
           );
}

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
{
  ASSERT (Buffer != NULL);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  return InternalSmBusBlock (
           SMBUS_V_SMB_CMD_BLOCK,
           SmBusAddress | SMBUS_B_WRITE,
           Buffer,
           NULL,
           Status
           );
}

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
  IN  VOID           *OutBuffer,
  OUT VOID           *InBuffer,
  OUT RETURN_STATUS  *Status        OPTIONAL
  )
{
  ASSERT (InBuffer  != NULL);
  ASSERT (OutBuffer != NULL);
  ASSERT (SMBUS_LIB_RESEARVED (SmBusAddress) == 0);

  return InternalSmBusBlock (
           SMBUS_V_SMB_CMD_BLOCK_PROCESS,
           SmBusAddress | SMBUS_B_WRITE,
           OutBuffer,
           InBuffer,
           Status
           );
}
