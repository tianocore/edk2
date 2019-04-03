/** @file
  Basic TIS (TPM Interface Specification) functions for Atmel I2C TPM.

  Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/Tpm12DeviceLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/I2cLib.h>

//
// Atmel I2C TPM slave address
//
#define ATMEL_I2C_TPM_SLAVE_ADDRESS      0x29

//
// Maximum I2C transfer size for Atmel I2C TPM
//
#define ATMEL_I2C_TPM_MAX_TRANSFER_SIZE  0x10

//
// Default TimeOut values in microseconds
//
#define TIS_TIMEOUT_A  ( 750 * 1000)  // 750ms
#define TIS_TIMEOUT_B  (2000 * 1000)  // 2s
#define TIS_TIMEOUT_C  ( 750 * 1000)  // 750ms
#define TIS_TIMEOUT_D  ( 750 * 1000)  // 750ms

/**
  Send command to Atmel I2c TPM breaking request up into multiple I2C transfers
  if required.

  @param[in] Buffer  Pointer to TPM command data.
  @param[in] Length  Number of bytes of TPM command data.

  @retval EFI_SUCCESS    TPM command sent.
  @retval EFI_NOT_FOUND  TPM chip doesn't exit.
  @retval EFI_TIMEOUT    Can't get the TPM control in time.
**/
EFI_STATUS
WriteTpmBufferMultiple (
  IN UINT8  *Buffer,
  IN UINTN  Length
  )
{
  EFI_STATUS              Status;
  EFI_I2C_DEVICE_ADDRESS  I2CDeviceAddr;
  UINTN                   Index;
  UINTN                   PartialLength;

  Status = EFI_SUCCESS;

  I2CDeviceAddr.I2CDeviceAddress = ATMEL_I2C_TPM_SLAVE_ADDRESS;

  DEBUG ((EFI_D_VERBOSE, "WriteTpmBufferMultiple: Addr=%02x  Length=%02x\n", I2CDeviceAddr.I2CDeviceAddress, Length));

  for (PartialLength = 0; Length > 0; Length -= PartialLength, Buffer += PartialLength) {
    //
    // Write data to TPM.
    //
    PartialLength = MIN (Length, ATMEL_I2C_TPM_MAX_TRANSFER_SIZE);
    Status = I2cWriteMultipleByte (
      I2CDeviceAddr,
      EfiI2CSevenBitAddrMode,
      &PartialLength,
      Buffer
      );
    DEBUG ((EFI_D_VERBOSE, "  "));
    for (Index = 0; Index < PartialLength; Index++) {
      DEBUG ((EFI_D_VERBOSE, "%02x ", Buffer[Index]));
    }
    DEBUG ((EFI_D_VERBOSE, "\n"));
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_VERBOSE, "  Status = %r\n", Status));
      return Status;
    }
  }

  DEBUG ((EFI_D_VERBOSE, "  Status = %r\n", Status));
  return Status;
}

/**
  Receive a response to a command from Atmel I2c TPM breaking response into
  multiple I2C transfers if required.

  @param[out] Buffer  Pointer to TPM response data.
  @param[in]  Length  Maximum number of bytes to receive.

  @retval EFI_SUCCESS    TPM response received.
  @retval EFI_NOT_FOUND  TPM chip doesn't exit.
  @retval EFI_TIMEOUT    Can't get the TPM control in time.
**/
EFI_STATUS
ReadTpmBufferMultiple (
  OUT UINT8  *Buffer,
  IN  UINTN  Length
  )
{
  EFI_STATUS              Status;
  EFI_I2C_DEVICE_ADDRESS  I2CDeviceAddr;
  UINTN                   WriteLength;
  UINTN                   Index;
  UINTN                   PartialLength;

  Status = EFI_SUCCESS;

  I2CDeviceAddr.I2CDeviceAddress = ATMEL_I2C_TPM_SLAVE_ADDRESS;
  WriteLength = 0;

  DEBUG ((EFI_D_VERBOSE, "ReadTpmBufferMultiple: Addr=%02x  Length=%02x\n", I2CDeviceAddr.I2CDeviceAddress, Length));

  for (PartialLength = 0; Length > 0; Length -= PartialLength, Buffer += PartialLength) {
    //
    // Read data from TPM.
    //
    PartialLength = MIN (Length, ATMEL_I2C_TPM_MAX_TRANSFER_SIZE);
    Status = I2cReadMultipleByte (
      I2CDeviceAddr,
      EfiI2CSevenBitAddrMode,
      &WriteLength,
      &PartialLength,
      Buffer
      );
    if (!EFI_ERROR (Status)) {
      DEBUG ((EFI_D_VERBOSE, "  "));
      for (Index = 0; Index < PartialLength; Index++) {
        DEBUG ((EFI_D_VERBOSE, "%02x ", Buffer[Index]));
      }
      DEBUG ((EFI_D_VERBOSE, "\n"));
    }
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_VERBOSE, "  Status = %r\n", Status));
      return Status;
    }
  }

  DEBUG ((EFI_D_VERBOSE, "  Status = %r\n", Status));
  return Status;
}

/**
  This service requests use TPM12.

  @retval EFI_SUCCESS       Get the control of TPM12 chip.
  @retval EFI_NOT_FOUND     TPM12 not found.
  @retval EFI_DEVICE_ERROR  Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12RequestUseTpm (
  VOID
  )
{
  EFI_STATUS                 Status;
  UINT8                      Data;
  UINT64                     Current;
  UINT64                     Previous;
  UINT64                     Total;
  UINT64                     Start;
  UINT64                     End;
  UINT64                     Timeout;
  INT64                      Cycle;
  INT64                      Delta;

  //
  // Get the current timer value
  //
  Current = GetPerformanceCounter();

  //
  // Initialize local variables
  //
  Start = 0;
  End   = 0;
  Total = 0;

  //
  // Retrieve the performance counter properties and compute the number of
  // performance counter ticks required to reach the maximum TIS timeout of
  // TIS_TIMEOUT_A.  TIS_TIMEOUT_A is in microseconds.
  //
  Timeout = DivU64x32 (
              MultU64x32 (
                GetPerformanceCounterProperties (&Start, &End),
                TIS_TIMEOUT_A
                ),
              1000000
              );
  Cycle = End - Start;
  if (Cycle < 0) {
    Cycle = -Cycle;
  }
  Cycle++;

  //
  // Attempt to read a byte from the Atmel I2C TPM
  //
  do {
    Status = ReadTpmBufferMultiple (&Data, sizeof(Data));

    Previous = Current;
    Current  = GetPerformanceCounter();
    Delta = (INT64) (Current - Previous);
    if (Start > End) {
      Delta = -Delta;
    }
    if (Delta < 0) {
      Delta += Cycle;
    }
    Total += Delta;
    if (Total >= Timeout) {
      Status = EFI_TIMEOUT;
      DEBUG ((EFI_D_ERROR, "Atmel I2C TPM failed to read: %r\n", Status));
      return Status;
    }
  } while (EFI_ERROR (Status));

  return EFI_SUCCESS;
}

/**
  This service enables the sending of commands to the TPM12.

  @param[in]     InputParameterBlockSize   Size of the TPM12 input parameter block.
  @param[in]     InputParameterBlock       Pointer to the TPM12 input parameter block.
  @param[in,out] OutputParameterBlockSize  Size of the TPM12 output parameter block.
  @param[in]     OutputParameterBlock      Pointer to the TPM12 output parameter block.

  @retval EFI_SUCCESS           The command byte stream was successfully sent to
                                the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR      The command was not successfully sent to the
                                device or a response was not successfully received
                                from the device.
  @retval EFI_BUFFER_TOO_SMALL  The output parameter block is too small.
**/
EFI_STATUS
EFIAPI
Tpm12SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  )
{
  EFI_STATUS           Status;
  UINT32               TpmOutSize;
  TPM_RSP_COMMAND_HDR  *ResponseHeader;
  UINT64               Current;
  UINT64               Previous;
  UINT64               Total;
  UINT64               Start;
  UINT64               End;
  UINT64               Timeout;
  INT64                Cycle;
  INT64                Delta;

  //
  // Initialize local variables
  //
  Start   = 0;
  End     = 0;
  Total   = 0;

  //
  // Make sure response buffer is big enough to hold a response header
  //
  if (*OutputParameterBlockSize < sizeof (TPM_RSP_COMMAND_HDR)) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }

  //
  // Get the current timer value
  //
  Current = GetPerformanceCounter();

  //
  // Retrieve the performance counter properties and compute the number of
  // performance counter ticks required to reach the maximum TIS timeout of
  // TIS_TIMEOUT_A.  TIS_TIMEOUT_A is in microseconds.
  //
  Timeout = DivU64x32 (
              MultU64x32 (
                GetPerformanceCounterProperties (&Start, &End),
                TIS_TIMEOUT_A
                ),
              1000000
              );
  Cycle = End - Start;
  if (Cycle < 0) {
    Cycle = -Cycle;
  }
  Cycle++;

  //
  // Send command
  //
  do {
    Status = WriteTpmBufferMultiple (InputParameterBlock, InputParameterBlockSize);

    Previous = Current;
    Current  = GetPerformanceCounter();
    Delta = (INT64) (Current - Previous);
    if (Start > End) {
      Delta = -Delta;
    }
    if (Delta < 0) {
      Delta += Cycle;
    }
    Total += Delta;
    if (Total >= Timeout) {
      Status = EFI_TIMEOUT;
      goto Done;
    }
  } while (EFI_ERROR (Status));

  //
  // Receive response header
  //
  do {
    Status = ReadTpmBufferMultiple (OutputParameterBlock, sizeof (TPM_RSP_COMMAND_HDR));

    Previous = Current;
    Current  = GetPerformanceCounter();
    Delta = (INT64) (Current - Previous);
    if (Start > End) {
      Delta = -Delta;
    }
    if (Delta < 0) {
      Delta += Cycle;
    }
    Total += Delta;
    if (Total >= Timeout) {
      Status = EFI_TIMEOUT;
      goto Done;
    }
  } while (EFI_ERROR (Status));

  //
  // Check the response data header (tag, parasize and returncode)
  //
  ResponseHeader = (TPM_RSP_COMMAND_HDR *)OutputParameterBlock;
  if (SwapBytes16 (ReadUnaligned16 (&ResponseHeader->tag)) != TPM_TAG_RSP_COMMAND) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  TpmOutSize = SwapBytes32 (ReadUnaligned32 (&ResponseHeader->paramSize));
  if (TpmOutSize == sizeof (TPM_RSP_COMMAND_HDR)) {
    *OutputParameterBlockSize = TpmOutSize;
    Status = EFI_SUCCESS;
    goto Done;
  }
  if (TpmOutSize < sizeof (TPM_RSP_COMMAND_HDR)) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }
  if (*OutputParameterBlockSize < TpmOutSize) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }
  *OutputParameterBlockSize = TpmOutSize;

  //
  // Receive the remaining data in the response header
  //
  do {
    Status = ReadTpmBufferMultiple (
               OutputParameterBlock + sizeof (TPM_RSP_COMMAND_HDR),
               TpmOutSize - sizeof (TPM_RSP_COMMAND_HDR)
               );

    Previous = Current;
    Current  = GetPerformanceCounter();
    Delta = (INT64) (Current - Previous);
    if (Start > End) {
      Delta = -Delta;
    }
    if (Delta < 0) {
      Delta += Cycle;
    }
    Total += Delta;
    if (Total >= Timeout) {
      Status = EFI_TIMEOUT;
      goto Done;
    }
  } while (EFI_ERROR (Status));

Done:
  DEBUG ((
    EFI_D_VERBOSE,
    "Tpm12SubmitCommand() Status = %r  Time = %ld ms\n",
    Status,
    DivU64x64Remainder (
      MultU64x32 (Total, 1000),
      GetPerformanceCounterProperties (NULL, NULL),
      NULL
      )
    ));

  return Status;
}
