/** @file
  Serial I/O Port library functions with no library constructor/destructor

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/SemihostLib.h>
#include <Library/SerialPortLib.h>


/*

  Programmed hardware of Serial port.

  @return    Always return EFI_UNSUPPORTED.

**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  if (SemihostConnectionSupported ()) {
    return RETURN_SUCCESS;
  } else {
    return RETURN_UNSUPPORTED;
  }
}

/**
  Write data to serial device.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes writed to serial device.

**/

#define PRINT_BUFFER_SIZE       512
#define PRINT_BUFFER_THRESHOLD  (PRINT_BUFFER_SIZE - 4)

UINTN
EFIAPI
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
)
{
  UINT8 PrintBuffer[PRINT_BUFFER_SIZE];
  UINTN SourceIndex      = 0;
  UINTN DestinationIndex = 0;
  UINT8 CurrentCharacter;

  while (SourceIndex < NumberOfBytes)
  {
      CurrentCharacter = Buffer[SourceIndex++];

      switch (CurrentCharacter)
      {
      case '\r':
          continue;

      case '\n':
          PrintBuffer[DestinationIndex++] = ' ';
          // fall through

      default:
          PrintBuffer[DestinationIndex++] = CurrentCharacter;
          break;
      }

      if (DestinationIndex > PRINT_BUFFER_THRESHOLD)
      {
          PrintBuffer[DestinationIndex] = '\0';
          SemihostWriteString ((CHAR8 *) PrintBuffer);

          DestinationIndex = 0;
      }
  }

  if (DestinationIndex > 0)
  {
      PrintBuffer[DestinationIndex] = '\0';
      SemihostWriteString ((CHAR8 *) PrintBuffer);
  }

  return NumberOfBytes;
}


/**
  Read data from serial device and save the datas in buffer.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval !0               Aactual number of bytes read from serial device.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
)
{
  *Buffer = SemihostReadCharacter ();
  return 1;
}



/**
  Check to see if any data is avaiable to be read from the debug device.

  @retval TRUE       At least one byte of data is avaiable to be read
  @retval FALSE      No data is avaiable to be read

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  // Since SemiHosting read character is blocking always say we have a char ready?
  return SemihostConnectionSupported ();
}

