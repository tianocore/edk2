/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Coreboot.h>

#include <Library/BaseLib.h>
#include <Library/BlParseLib.h>
#include <Library/SerialPortLib.h>

//
// We can't use DebugLib due to a constructor dependency cycle between DebugLib
// and ourselves.
//
#define ASSERT(Expression)      \
  do {                          \
    if (!(Expression)) {        \
      CpuDeadLoop ();           \
    }                           \
  } while (FALSE)

#define CBMC_CURSOR_MASK  ((1 << 28) - 1)
#define CBMC_OVERFLOW     (1 << 31)

STATIC struct cbmem_console  *gCbConsole = NULL;
STATIC UINT32                STM_cursor  = 0;

// Try to find the coreboot memory table in the given coreboot table.
static void *
find_cb_subtable (
  struct cb_header  *cbh,
  UINT32            tag
  )
{
  char    *tbl  = (char *)cbh + sizeof (*cbh);
  UINT32  count = cbh->table_entries;
  int     i;

  for (i = 0; i < count; i++) {
    struct cb_memory  *cbm = (void *)tbl;
    tbl += cbm->size;
    if (cbm->tag == tag) {
      return cbm;
    }
  }

  return NULL;
}

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
  /* `FindCbTag` doesn't work because we need direct access to the memory addresses? */
  struct cb_header  *cbh = GetParameterBase ();

  if (!cbh) {
    return RETURN_DEVICE_ERROR;
  }

  struct cb_cbmem_ref  *cbref = find_cb_subtable (cbh, CB_TAG_CBMEM_CONSOLE);

  if (!cbref) {
    return RETURN_DEVICE_ERROR;
  }

  gCbConsole = (void *)(UINTN)cbref->cbmem_addr;        // Support PEI and DXE
  if (gCbConsole == NULL) {
    return RETURN_DEVICE_ERROR;
  }

  // set the cursor such that the STM console will not overwrite the
  // coreboot console output
  STM_cursor = gCbConsole->cursor & CBMC_CURSOR_MASK;

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
  UINTN   Sent;
  UINT32  cursor;
  UINT32  flags;

  ASSERT (Buffer != NULL);

  if (NumberOfBytes == 0) {
    return 0;
  }

  if (!gCbConsole) {
    return 0;
  }

  Sent = 0;
  do {
    cursor = gCbConsole->cursor & CBMC_CURSOR_MASK;
    flags  = gCbConsole->cursor & ~CBMC_CURSOR_MASK;

    if (cursor >= gCbConsole->size) {
      return 0; // Old coreboot version with legacy overflow mechanism.
    }

    gCbConsole->body[cursor++] = Buffer[Sent++];

    if (cursor >= gCbConsole->size) {
      cursor = STM_cursor;
      flags |= CBMC_OVERFLOW;
    }

    gCbConsole->cursor = flags | cursor;
  } while (Sent < NumberOfBytes);

  return Sent;
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
