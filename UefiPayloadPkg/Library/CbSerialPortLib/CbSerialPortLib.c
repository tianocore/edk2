/** @file
  CBMEM console SerialPortLib instance

  Copyright (c) 2022, Baruch Binyamin Doron
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Coreboot.h>

#include <Library/BaseMemoryLib.h>
#include <Library/BlParseLib.h>
#include <Library/SerialPortLib.h>

// Upper nibble contains flags
#define CBMC_CURSOR_MASK  ((1 << 28) - 1)
#define CBMC_OVERFLOW     (1 << 31)

STATIC struct cbmem_console  *mCbConsole = NULL;

/**
  Find coreboot record with given Tag.
  NOTE: This coreboot-specific function definition is absent
         from the common BlParseLib header.

  @param  Tag                The tag id to be found

  @retval NULL              The Tag is not found.
  @retval Others            The pointer to the record found.

**/
VOID *
FindCbTag (
  IN  UINT32  Tag
  );

/**
  Initialize the serial device hardware.

  If no initialization is required, then return RETURN_SUCCESS.
  If the serial device was successfully initialized, then return RETURN_SUCCESS.
  If the serial device could not be initialized, then return RETURN_DEVICE_ERROR.

  @retval RETURN_SUCCESS        The serial device was initialized.
  @retval RETURN_DEVICE_ERROR   The serial device could not be initialized.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  struct cb_cbmem_ref  *CbMemRef;

  // The coreboot table contains large structures as references
  CbMemRef = FindCbTag (CB_TAG_CBMEM_CONSOLE);
  if (CbMemRef == NULL) {
    return RETURN_DEVICE_ERROR;
  }

  mCbConsole = (VOID *)(UINTN)CbMemRef->cbmem_addr;  // Support PEI and DXE
  if (mCbConsole == NULL) {
    return RETURN_DEVICE_ERROR;
  }

  return RETURN_SUCCESS;
}

/**
  Write data from buffer to serial device.

  Writes NumberOfBytes data bytes from Buffer to the serial device.
  The number of bytes actually written to the serial device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.
  If Buffer is NULL, then ASSERT().
  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the serial device.
                           If this value is less than NumberOfBytes, then the write operation failed.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  UINT32  Cursor;
  UINT32  Flags;

  if ((Buffer == NULL) || (NumberOfBytes == 0)) {
    return 0;
  }

  if (mCbConsole == NULL) {
    return 0;
  }

  Cursor = mCbConsole->cursor & CBMC_CURSOR_MASK;
  Flags  = mCbConsole->cursor & ~CBMC_CURSOR_MASK;
  if (Cursor >= mCbConsole->size) {
    // Already overflowed; bail out. TODO: Is this unnecessarily cautious?
    // - Supports old coreboot version with legacy overflow mechanism.
    return 0;
  }

  if (Cursor + NumberOfBytes > mCbConsole->size) {
    // Will overflow, zero cursor and set flag.
    Cursor = 0;
    Flags |= CBMC_OVERFLOW;
  }

  if (NumberOfBytes > mCbConsole->size) {
    // This one debug message is longer than the entire buffer. Truncate it.
    // - TODO: Is this unnecessarily cautious?
    NumberOfBytes = mCbConsole->size;
  }

  CopyMem (&mCbConsole->body[Cursor], Buffer, NumberOfBytes);
  Cursor += NumberOfBytes;

  if (Cursor == mCbConsole->size) {
    // Next message will overflow, zero cursor.
    // - Set flag preemptively. This could not be determined later.
    Cursor = 0;
    Flags |= CBMC_OVERFLOW;
  }

  mCbConsole->cursor = Flags | Cursor;

  return NumberOfBytes;
}

/**
  Read data from serial device and save the datas in buffer.

  Reads NumberOfBytes data bytes from a serial device into the buffer
  specified by Buffer. The number of bytes actually read is returned.
  If Buffer is NULL, then ASSERT().
  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to store the data read from the serial device.
  @param  NumberOfBytes    Number of bytes which will be read.

  @retval 0                Read data failed, no data is to be read.
  @retval >0               Actual number of bytes read from serial device.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8  *Buffer,
  IN  UINTN  NumberOfBytes
  )
{
  return 0;
}

/**
  Polls a serial device to see if there is any data waiting to be read.

  @retval TRUE             Data is waiting to be read from the serial device.
  @retval FALSE            There is no data waiting to be read from the serial device.

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  return FALSE;
}

/**
  Sets the control bits on a serial device.

  @param Control                Sets the bits of Control that are settable.

  @retval RETURN_SUCCESS        The new control bits were set on the serial device.
  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.
  @retval RETURN_DEVICE_ERROR   The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32  Control
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  Retrieve the status of the control bits on a serial device.

  @param Control                A pointer to return the current control signals from the serial device.

  @retval RETURN_SUCCESS        The control bits were read from the serial device.
  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.
  @retval RETURN_DEVICE_ERROR   The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32  *Control
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receive time out, parity,
  data bits, and stop bits on a serial device.

  @param BaudRate           The requested baud rate. A BaudRate value of 0 will use the
                            device's default interface speed.
                            On output, the value actually set.
  @param ReceiveFifoDepth   The requested depth of the FIFO on the receive side of the
                            serial interface. A ReceiveFifoDepth value of 0 will use
                            the device's default FIFO depth.
                            On output, the value actually set.
  @param Timeout            The requested time out for a single character in microseconds.
                            This timeout applies to both the transmit and receive side of the
                            interface. A Timeout value of 0 will use the device's default time
                            out value.
                            On output, the value actually set.
  @param Parity             The type of parity to use on this serial device. A Parity value of
                            DefaultParity will use the device's default parity value.
                            On output, the value actually set.
  @param DataBits           The number of data bits to use on the serial device. A DataBits
                            value of 0 will use the device's default data bit setting.
                            On output, the value actually set.
  @param StopBits           The number of stop bits to use on this serial device. A StopBits
                            value of DefaultStopBits will use the device's default number of
                            stop bits.
                            On output, the value actually set.

  @retval RETURN_SUCCESS            The new attributes were set on the serial device.
  @retval RETURN_UNSUPPORTED        The serial device does not support this operation.
  @retval RETURN_INVALID_PARAMETER  One or more of the attributes has an unsupported value.
  @retval RETURN_DEVICE_ERROR       The serial device is not functioning correctly.

**/
RETURN_STATUS
EFIAPI
SerialPortSetAttributes (
  IN OUT UINT64              *BaudRate,
  IN OUT UINT32              *ReceiveFifoDepth,
  IN OUT UINT32              *Timeout,
  IN OUT EFI_PARITY_TYPE     *Parity,
  IN OUT UINT8               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE  *StopBits
  )
{
  return RETURN_UNSUPPORTED;
}
