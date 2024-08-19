/** @file
  Serial IO Abstraction for GDB stub. This allows an EFI consoles that shows up on the system
  running GDB. One console for error information and another console for user input/output.

  Basic packet format is $packet-data#checksum. So every command has 4 bytes of overhead: $,
  #, 0, 0. The 0 and 0 are the ascii characters for the checksum.


  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <GdbStubInternal.h>

//
// Set TRUE if F Reply package signals a ctrl-c. We can not process the Ctrl-c
// here we need to wait for the periodic callback to do this.
//
BOOLEAN  gCtrlCBreakFlag = FALSE;

//
// If the periodic callback is called while we are processing an F packet we need
// to let the callback know to not read from the serial stream as it could steal
// characters from the F response packet
//
BOOLEAN  gProcessingFPacket = FALSE;

/**
  Process a control-C break message.

  Currently a place holder, remove the ASSERT when it gets implemented.

  @param  ErrNo   Error information from the F reply packet or other source

**/
VOID
GdbCtrlCBreakMessage (
  IN  UINTN  ErrNo
  )
{
  // See D.10.5 of gdb.pdf
  // This should look like a break message. Should look like SIGINT

  /* TODO: Make sure if we should do anything with ErrNo */
  // Turn on the global Ctrl-C flag.
  gCtrlCBreakFlag = TRUE;
}

/**
  Parse the F reply packet and extract the return value and an ErrNo if it exists.

  @param  Packet  Packet to parse like an F reply packet
  @param  ErrNo   Buffer to hold Count bytes that were read

  @retval -1      Error, not a valid F reply packet
  @retval other   Return the return code from the F reply packet

**/
INTN
GdbParseFReplyPacket (
  IN  CHAR8  *Packet,
  OUT UINTN  *ErrNo
  )
{
  INTN  RetCode;

  if (Packet[0] != 'F') {
    // A valid response would be an F packet
    return -1;
  }

  RetCode = AsciiStrHexToUintn (&Packet[1]);

  // Find 1st comma
  for ( ; *Packet != '\0' && *Packet != ','; Packet++) {
  }

  if (*Packet == '\0') {
    *ErrNo = 0;
    return RetCode;
  }

  *ErrNo = AsciiStrHexToUintn (++Packet);

  // Find 2nd comma
  for ( ; *Packet != '\0' && *Packet != ','; Packet++) {
  }

  if (*Packet == '\0') {
    return RetCode;
  }

  if (*(++Packet) == 'C') {
    GdbCtrlCBreakMessage (*ErrNo);
  }

  return RetCode;
}

/**
  Read data from a FileDescriptor. On success number of bytes read is returned. Zero indicates
  the end of a file. On error -1 is returned. If count is zero, GdbRead returns zero.

  @param  FileDescriptor   Device to talk to.
  @param  Buffer           Buffer to hold Count bytes that were read
  @param  Count            Number of bytes to transfer.

  @retval -1               Error
  @retval {other}          Number of bytes read.

**/
INTN
GdbRead (
  IN  INTN   FileDescriptor,
  OUT VOID   *Buffer,
  IN  UINTN  Count
  )
{
  CHAR8    Packet[128];
  UINTN    Size;
  INTN     RetCode;
  UINTN    ErrNo;
  BOOLEAN  ReceiveDone = FALSE;

  // Send:
  // "Fread,XX,YYYYYYYY,XX
  //
  // XX - FileDescriptor in ASCII
  // YYYYYYYY - Buffer address in ASCII
  // XX - Count in ASCII
  // SS - check sum
  //
  Size = AsciiSPrint (Packet, sizeof (Packet), "Fread,%x,%x,%x", FileDescriptor, Buffer, Count);
  // Packet array is too small if you got this ASSERT
  ASSERT (Size < sizeof (Packet));

  gProcessingFPacket = TRUE;
  SendPacket (Packet);
  Print ((CHAR16 *)L"Packet sent..\n");

  do {
    // Reply:
    ReceivePacket (Packet, sizeof (Packet));
    Print ((CHAR16 *)L"Command received..%c\n", Packet[0]);

    // Process GDB commands
    switch (Packet[0]) {
      // Write memory command.
      // M addr,length:XX...
      case    'M':
        WriteToMemory (Packet);
        break;

      // Fretcode, errno, Ctrl-C flag
      // retcode - Count read
      case    'F':
        // Once target receives F reply packet that means the previous
        // transactions are finished.
        ReceiveDone = TRUE;
        break;

      // Send empty buffer
      default:
        SendNotSupported ();
        break;
    }
  } while (ReceiveDone == FALSE);

  RetCode = GdbParseFReplyPacket (Packet, &ErrNo);
  Print ((CHAR16 *)L"RetCode: %x..ErrNo: %x..\n", RetCode, ErrNo);

  if (ErrNo > 0) {
    // Send error to the host if there is any.
    SendError ((UINT8)ErrNo);
  }

  gProcessingFPacket = FALSE;

  return RetCode;
}

/**
  Write data to a FileDescriptor. On success number of bytes written is returned. Zero indicates
  nothing was written. On error -1 is returned.

  @param  FileDescriptor   Device to talk to.
  @param  Buffer           Buffer to hold Count bytes that are to be written
  @param  Count            Number of bytes to transfer.

  @retval -1               Error
  @retval {other}          Number of bytes written.

**/
INTN
GdbWrite (
  IN  INTN        FileDescriptor,
  OUT CONST VOID  *Buffer,
  IN  UINTN       Count
  )
{
  CHAR8    Packet[128];
  UINTN    Size;
  INTN     RetCode;
  UINTN    ErrNo;
  BOOLEAN  ReceiveDone = FALSE;

  // Send:
  // #Fwrite,XX,YYYYYYYY,XX$SS
  //
  // XX - FileDescriptor in ASCII
  // YYYYYYYY - Buffer address in ASCII
  // XX - Count in ASCII
  // SS - check sum
  //
  Size = AsciiSPrint (Packet, sizeof (Packet), "Fwrite,%x,%x,%x", FileDescriptor, Buffer, Count);
  // Packet array is too small if you got this ASSERT
  ASSERT (Size < sizeof (Packet));

  SendPacket (Packet);
  Print ((CHAR16 *)L"Packet sent..\n");

  do {
    // Reply:
    ReceivePacket (Packet, sizeof (Packet));
    Print ((CHAR16 *)L"Command received..%c\n", Packet[0]);

    // Process GDB commands
    switch (Packet[0]) {
      // Read memory command.
      // m addr,length.
      case    'm':
        ReadFromMemory (Packet);
        break;

      // Fretcode, errno, Ctrl-C flag
      // retcode - Count read
      case    'F':
        // Once target receives F reply packet that means the previous
        // transactions are finished.
        ReceiveDone = TRUE;
        break;

      // Send empty buffer
      default:
        SendNotSupported ();
        break;
    }
  } while (ReceiveDone == FALSE);

  RetCode = GdbParseFReplyPacket (Packet, &ErrNo);
  Print ((CHAR16 *)L"RetCode: %x..ErrNo: %x..\n", RetCode, ErrNo);

  // Send error to the host if there is any.
  if (ErrNo > 0) {
    SendError ((UINT8)ErrNo);
  }

  return RetCode;
}

/**
  Reset the serial device.

  @param  This              Protocol instance pointer.

  @retval EFI_SUCCESS       The device was reset.
  @retval EFI_DEVICE_ERROR  The serial device could not be reset.

**/
EFI_STATUS
EFIAPI
GdbSerialReset (
  IN EFI_SERIAL_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receive time out, parity,
  data buts, and stop bits on a serial device.

  @param  This             Protocol instance pointer.
  @param  BaudRate         The requested baud rate. A BaudRate value of 0 will use the
                           device's default interface speed.
  @param  ReceiveFifoDepth The requested depth of the FIFO on the receive side of the
                           serial interface. A ReceiveFifoDepth value of 0 will use
                           the device's default FIFO depth.
  @param  Timeout          The requested time out for a single character in microseconds.
                           This timeout applies to both the transmit and receive side of the
                           interface. A Timeout value of 0 will use the device's default time
                           out value.
  @param  Parity           The type of parity to use on this serial device. A Parity value of
                           DefaultParity will use the device's default parity value.
  @param  DataBits         The number of data bits to use on the serial device. A DataBits
                           value of 0 will use the device's default data bit setting.
  @param  StopBits         The number of stop bits to use on this serial device. A StopBits
                           value of DefaultStopBits will use the device's default number of
                           stop bits.

  @retval EFI_SUCCESS      The device was reset.
  @retval EFI_DEVICE_ERROR The serial device could not be reset.

**/
EFI_STATUS
EFIAPI
GdbSerialSetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT64                  BaudRate,
  IN UINT32                  ReceiveFifoDepth,
  IN UINT32                  Timeout,
  IN EFI_PARITY_TYPE         Parity,
  IN UINT8                   DataBits,
  IN EFI_STOP_BITS_TYPE      StopBits
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Set the control bits on a serial device

  @param  This             Protocol instance pointer.
  @param  Control          Set the bits of Control that are settable.

  @retval EFI_SUCCESS      The new control bits were set on the serial device.
  @retval EFI_UNSUPPORTED  The serial device does not support this operation.
  @retval EFI_DEVICE_ERROR The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
GdbSerialSetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT32                  Control
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Retrieves the status of the control bits on a serial device

  @param  This              Protocol instance pointer.
  @param  Control           A pointer to return the current Control signals from the serial device.

  @retval EFI_SUCCESS       The control bits were read from the serial device.
  @retval EFI_DEVICE_ERROR  The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
GdbSerialGetControl (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  OUT UINT32                 *Control
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Writes data to a serial device.

  @param  This              Protocol instance pointer.
  @param  BufferSize        On input, the size of the Buffer. On output, the amount of
                            data actually written.
  @param  Buffer            The buffer of data to write

  @retval EFI_SUCCESS       The data was written.
  @retval EFI_DEVICE_ERROR  The device reported an error.
  @retval EFI_TIMEOUT       The data write was stopped due to a timeout.

**/
EFI_STATUS
EFIAPI
GdbSerialWrite (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  IN VOID                    *Buffer
  )
{
  GDB_SERIAL_DEV  *SerialDev;
  UINTN           Return;

  SerialDev = GDB_SERIAL_DEV_FROM_THIS (This);

  Return = GdbWrite (SerialDev->OutFileDescriptor, Buffer, *BufferSize);
  if (Return == (UINTN)-1) {
    return EFI_DEVICE_ERROR;
  }

  if (Return != *BufferSize) {
    *BufferSize = Return;
  }

  return EFI_SUCCESS;
}

/**
  Writes data to a serial device.

  @param  This              Protocol instance pointer.
  @param  BufferSize        On input, the size of the Buffer. On output, the amount of
                            data returned in Buffer.
  @param  Buffer            The buffer to return the data into.

  @retval EFI_SUCCESS       The data was read.
  @retval EFI_DEVICE_ERROR  The device reported an error.
  @retval EFI_TIMEOUT       The data write was stopped due to a timeout.

**/
EFI_STATUS
EFIAPI
GdbSerialRead (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  OUT VOID                   *Buffer
  )
{
  GDB_SERIAL_DEV  *SerialDev;
  UINTN           Return;

  SerialDev = GDB_SERIAL_DEV_FROM_THIS (This);

  Return = GdbRead (SerialDev->InFileDescriptor, Buffer, *BufferSize);
  if (Return == (UINTN)-1) {
    return EFI_DEVICE_ERROR;
  }

  if (Return != *BufferSize) {
    *BufferSize = Return;
  }

  return EFI_SUCCESS;
}

//
// Template used to initialize the GDB Serial IO protocols
//
GDB_SERIAL_DEV  gdbSerialDevTemplate = {
  GDB_SERIAL_DEV_SIGNATURE,
  NULL,

  { // SerialIo
    SERIAL_IO_INTERFACE_REVISION,
    GdbSerialReset,
    GdbSerialSetAttributes,
    GdbSerialSetControl,
    GdbSerialGetControl,
    GdbSerialWrite,
    GdbSerialRead,
    NULL
  },
  {    // SerialMode
    0, // ControlMask
    0, // Timeout
    0, // BaudRate
    1, // ReceiveFifoDepth
    0, // DataBits
    0, // Parity
    0  // StopBits
  },
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof (VENDOR_DEVICE_PATH) + sizeof (UINT32)),
          (UINT8)((sizeof (VENDOR_DEVICE_PATH) + sizeof (UINT32)) >> 8)
        },
      },
      EFI_SERIAL_IO_PROTOCOL_GUID
    },
    0,
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      {
        (UINT8)(sizeof (EFI_DEVICE_PATH_PROTOCOL)),
        (UINT8)(sizeof (EFI_DEVICE_PATH_PROTOCOL) >> 8)
      }
    },
  },
  GDB_STDIN,
  GDB_STDOUT
};

/**
  Make two serial consoles: 1) StdIn and StdOut via GDB. 2) StdErr via GDB.

  These console show up on the remote system running GDB

**/
VOID
GdbInitializeSerialConsole (
  VOID
  )
{
  EFI_STATUS      Status;
  GDB_SERIAL_DEV  *StdOutSerialDev;
  GDB_SERIAL_DEV  *StdErrSerialDev;

  // Use the template to make a copy of the Serial Console private data structure.
  StdOutSerialDev = AllocateCopyPool (sizeof (GDB_SERIAL_DEV), &gdbSerialDevTemplate);
  ASSERT (StdOutSerialDev != NULL);

  // Fixup pointer after the copy
  StdOutSerialDev->SerialIo.Mode = &StdOutSerialDev->SerialMode;

  StdErrSerialDev = AllocateCopyPool (sizeof (GDB_SERIAL_DEV), &gdbSerialDevTemplate);
  ASSERT (StdErrSerialDev != NULL);

  // Fixup pointer and modify stuff that is different for StdError
  StdErrSerialDev->SerialIo.Mode     = &StdErrSerialDev->SerialMode;
  StdErrSerialDev->DevicePath.Index  = 1;
  StdErrSerialDev->OutFileDescriptor = GDB_STDERR;

  // Make a new handle with Serial IO protocol and its device path on it.
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &StdOutSerialDev->Handle,
                  &gEfiSerialIoProtocolGuid,
                  &StdOutSerialDev->SerialIo,
                  &gEfiDevicePathProtocolGuid,
                  &StdOutSerialDev->DevicePath,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  // Make a new handle with Serial IO protocol and its device path on it.
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &StdErrSerialDev->Handle,
                  &gEfiSerialIoProtocolGuid,
                  &StdErrSerialDev->SerialIo,
                  &gEfiDevicePathProtocolGuid,
                  &StdErrSerialDev->DevicePath,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}
