/** @file
  Serial Port Lib that thunks back to Emulator services to write to StdErr.
  All read functions are stubed out. There is no constructor so this lib can
  be linked with PEI Core.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2011, Apple Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/SerialPortLib.h>
#include <Library/PeiServicesLib.h>

#include <Ppi/EmuThunk.h>
#include <Protocol/EmuThunk.h>

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
  return RETURN_SUCCESS;
}

/**
  Write data from buffer to serial device.

  Writes NumberOfBytes data bytes from Buffer to the serial device.
  The number of bytes actually written to the serial device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.
  If Buffer is NULL, then ASSERT().
  If NumberOfBytes is zero, then return 0.

  @param  Buffer           The pointer to the data buffer to be written.
  @param  NumberOfBytes    The number of bytes to written to the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the serial device.
                           If this value is less than NumberOfBytes, then the read operation failed.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  EMU_THUNK_PPI       *ThunkPpi;
  EFI_STATUS          Status;
  EMU_THUNK_PROTOCOL  *Thunk;

  //
  // Locate EmuThunkPpi for retrieving standard output handle
  //
  Status = PeiServicesLocatePpi (
             &gEmuThunkPpiGuid,
             0,
             NULL,
             (VOID **)&ThunkPpi
             );
  if (!EFI_ERROR (Status)) {
    Thunk = (EMU_THUNK_PROTOCOL *)ThunkPpi->Thunk ();
    return Thunk->WriteStdErr (Buffer, NumberOfBytes);
  }

  return 0;
}

/**
  Read data from serial device and save the datas in buffer.

  Reads NumberOfBytes data bytes from a serial device into the buffer
  specified by Buffer. The number of bytes actually read is returned.
  If the return value is less than NumberOfBytes, then the rest operation failed.
  If Buffer is NULL, then ASSERT().
  If NumberOfBytes is zero, then return 0.

  @param  Buffer           The pointer to the data buffer to store the data read from the serial device.
  @param  NumberOfBytes    The number of bytes which will be read.

  @retval 0                Read data failed; No data is to be read.
  @retval >0               The actual number of bytes read from serial device.

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

  Polls a serial device to see if there is any data waiting to be read.
  If there is data waiting to be read from the serial device, then TRUE is returned.
  If there is no data waiting to be read from the serial device, then FALSE is returned.

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
