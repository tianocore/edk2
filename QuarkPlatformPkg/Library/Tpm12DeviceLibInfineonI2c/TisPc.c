/** @file
  Basic TIS (TPM Interface Specification) functions for Infineon I2C TPM.

  Copyright (c) 2013 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/Tpm12DeviceLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/I2cLib.h>

//
// Default TPM (Infineon SLB9645) I2C Slave Device Address on Crosshill board.
//
#define TPM_I2C_SLAVE_DEVICE_ADDRESS              0x20

//
// Default Infineon SLB9645 TPM I2C mapped registers (SLB9645 I2C Comm. Protocol Application Note).
//
#define INFINEON_TPM_ACCESS_0_ADDRESS_DEFAULT     0x0
#define INFINEON_TPM_STS_0_ADDRESS_DEFAULT        0x01
#define INFINEON_TPM_BURST0_COUNT_0_DEFAULT       0x02
#define INFINEON_TPM_BURST1_COUNT_0_DEFAULT       0x03
#define INFINEON_TPM_DATA_FIFO_0_ADDRESS_DEFAULT  0x05
#define INFINEON_TPM_DID_VID_0_DEFAULT            0x09

//
// Max. retry count for read transfers (as recommended by Infineon).
//
#define READ_RETRY  3

//
// Guard time of 250us between I2C read and next I2C write transfer (as recommended by Infineon).
//
#define GUARD_TIME  250

//
// Define bits of ACCESS and STATUS registers
//

///
/// This bit is a 1 to indicate that the other bits in this register are valid.
///
#define TIS_PC_VALID                BIT7
///
/// Indicate that this locality is active.
///
#define TIS_PC_ACC_ACTIVE           BIT5
///
/// Set to 1 to indicate that this locality had the TPM taken away while
/// this locality had the TIS_PC_ACC_ACTIVE bit set.
///
#define TIS_PC_ACC_SEIZED           BIT4
///
/// Set to 1 to indicate that TPM MUST reset the
/// TIS_PC_ACC_ACTIVE bit and remove ownership for localities less than the
/// locality that is writing this bit.
///
#define TIS_PC_ACC_SEIZE            BIT3
///
/// When this bit is 1, another locality is requesting usage of the TPM.
///
#define TIS_PC_ACC_PENDIND          BIT2
///
/// Set to 1 to indicate that this locality is requesting to use TPM.
///
#define TIS_PC_ACC_RQUUSE           BIT1
///
/// A value of 1 indicates that a T/OS has not been established on the platform
///
#define TIS_PC_ACC_ESTABLISH        BIT0

///
/// When this bit is 1, TPM is in the Ready state,
/// indicating it is ready to receive a new command.
///
#define TIS_PC_STS_READY            BIT6
///
/// Write a 1 to this bit to cause the TPM to execute that command.
///
#define TIS_PC_STS_GO               BIT5
///
/// This bit indicates that the TPM has data available as a response.
///
#define TIS_PC_STS_DATA             BIT4
///
/// The TPM sets this bit to a value of 1 when it expects another byte of data for a command.
///
#define TIS_PC_STS_EXPECT           BIT3
///
/// Writes a 1 to this bit to force the TPM to re-send the response.
///
#define TIS_PC_STS_RETRY            BIT1

//
// Default TimeOut values in microseconds
//
#define TIS_TIMEOUT_A               (750  * 1000)  // 750ms
#define TIS_TIMEOUT_B               (2000 * 1000)  // 2s
#define TIS_TIMEOUT_C               (750  * 1000)  // 750ms
#define TIS_TIMEOUT_D               (750  * 1000)  // 750ms

//
// Global variable to indicate if TPM I2C Read Transfer has previously occurred.
// NOTE: Given the GUARD_TIME requirement (TpmAccess.h), if this library loaded
// by PEI Drivers this global variable required to be resident in R/W memory
//
BOOLEAN mI2CPrevReadTransfer = FALSE;

/**
  Writes single byte data to TPM specified by I2C register address.

  @param[in]  TpmAddress  The register to write.
  @param[in]  Data        The data to write to the register.

**/
VOID
TpmWriteByte (
  IN UINTN  TpmAddress,
  IN UINT8  Data
  )
{
  EFI_STATUS              Status;
  UINTN                   WriteLength;
  UINT8                   WriteData[2];
  EFI_I2C_DEVICE_ADDRESS  I2CDeviceAddr;

  //
  // Setup I2C Slave device address and address mode (7-bit).
  //
  I2CDeviceAddr.I2CDeviceAddress = TPM_I2C_SLAVE_DEVICE_ADDRESS;

  //
  // As recommended by Infineon (SLB9645 I2C Communication protocol application
  // note revision 1.0) wait 250 microseconds between a read and a write transfer.
  //
  if (mI2CPrevReadTransfer) {
    MicroSecondDelay (GUARD_TIME);
  }

  //
  // Write to TPM register.
  //
  WriteLength = 2;
  WriteData[0] = (UINT8)TpmAddress;
  WriteData[1] = Data;

  Status = I2cWriteMultipleByte (
             I2CDeviceAddr,
             EfiI2CSevenBitAddrMode,
             &WriteLength,
             &WriteData
             );
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "TpmWriteByte(): I2C Write to TPM address %0x failed (%r)\n", TpmAddress, Status));
    ASSERT (FALSE);  // Writes to TPM should always succeed.
  }

  mI2CPrevReadTransfer = FALSE;
}

/**
  Reads single byte data from TPM specified by I2C register address.

  Due to stability issues when using I2C combined write/read transfers (with
  RESTART) to TPM (specifically read from status register), a single write is
  performed followed by single read (with STOP condition in between).

  @param[in] TpmAddress  Address of register  to read.

  @return  The value register read.

**/
UINT8
TpmReadByte (
  IN UINTN  TpmAddress
  )
{
  UINT8                   Data[1];
  UINT8                   ReadData;
  UINT8                   ReadCount;

  EFI_I2C_DEVICE_ADDRESS  I2CDeviceAddr;
  EFI_I2C_ADDR_MODE       I2CAddrMode;

  EFI_STATUS              Status;

  Status = EFI_SUCCESS;
  ReadData  = 0xFF;
  ReadCount = 0;

  //
  // Locate I2C protocol for TPM I2C access.
  //
  I2CDeviceAddr.I2CDeviceAddress = TPM_I2C_SLAVE_DEVICE_ADDRESS;
  I2CAddrMode = EfiI2CSevenBitAddrMode;

  //
  // As recommended by Infineon (SLB9645 I2C Communication protocol application
  // note revision 1.0) retry up to 3 times if TPM status, access or burst count
  // registers return 0xFF.
  //
  while ((ReadData == 0xFF) && (ReadCount < READ_RETRY)) {
    //
    // As recommended by Infineon (SLB9645 I2C Communication protocol application
    // note revision 1.0) wait 250 microseconds between a read and a write transfer.
    //
    if (mI2CPrevReadTransfer) {
      MicroSecondDelay (GUARD_TIME);
    }

    //
    // Write address to TPM.
    //
    Data[0] = (UINT8)TpmAddress;
    Status = I2cWriteByte (
               I2CDeviceAddr,
               I2CAddrMode,
               &Data
               );

    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_INFO, "TpmReadByte(): write to TPM address %0x failed (%r)\n", TpmAddress, Status));
    }

    mI2CPrevReadTransfer = FALSE;

    //
    // Read data from TPM.
    //
    Data[0] = (UINT8)TpmAddress;
    Status = I2cReadByte (
               I2CDeviceAddr,
               I2CAddrMode,
               &Data
               );

    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_INFO, "TpmReadByte(): read from TPM address %0x failed (%r)\n", TpmAddress, Status));
      ReadData = 0xFF;
    } else {
      ReadData = Data[0];
    }

    //
    // Only need to retry 3 times for TPM status, access, and burst count registers.
    // If read transfer is to TPM Data FIFO, do not retry, exit loop.
    //
    if (TpmAddress == INFINEON_TPM_DATA_FIFO_0_ADDRESS_DEFAULT) {
      ReadCount = READ_RETRY;
    } else {
      ReadCount++;
    }

    mI2CPrevReadTransfer = TRUE;
  }

  if (EFI_ERROR(Status)) {
    //
    //  Only reads to access register allowed to fail.
    //
    if (TpmAddress != INFINEON_TPM_ACCESS_0_ADDRESS_DEFAULT) {
      DEBUG ((EFI_D_ERROR, "TpmReadByte(): read from TPM address %0x failed\n", TpmAddress));
      ASSERT_EFI_ERROR (Status);
    }
  }
  return ReadData;
}

/**
  Check whether the value of a TPM chip register satisfies the input BIT setting.

  @param[in] Register  TPM register to be checked.
  @param[in] BitSet    Check these data bits are set.
  @param[in] BitClear  Check these data bits are clear.
  @param[in] TimeOut   The max wait time (unit MicroSecond) when checking register.

  @retval EFI_SUCCESS  The register satisfies the check bit.
  @retval EFI_TIMEOUT  The register can't run into the expected status in time.
**/
EFI_STATUS
TisPcWaitRegisterBits (
  IN UINTN   Register,
  IN UINT8   BitSet,
  IN UINT8   BitClear,
  IN UINT32  TimeOut
  )
{
  UINT8   RegRead;
  UINT32  WaitTime;

  for (WaitTime = 0; WaitTime < TimeOut; WaitTime += 30){
    RegRead = TpmReadByte (Register);
    if ((RegRead & BitSet) == BitSet && (RegRead & BitClear) == 0)
      return EFI_SUCCESS;
    MicroSecondDelay (30);
  }
  return EFI_TIMEOUT;
}

/**
  Get BurstCount by reading the burstCount field of a TIS register
  in the time of default TIS_TIMEOUT_D.

  @param[out] BurstCount  Pointer to a buffer to store the got BurstConut.

  @retval EFI_SUCCESS            Get BurstCount.
  @retval EFI_INVALID_PARAMETER  BurstCount is NULL.
  @retval EFI_TIMEOUT            BurstCount can't be got in time.
**/
EFI_STATUS
TisPcReadBurstCount (
  OUT UINT16  *BurstCount
  )
{
  UINT32  WaitTime;
  UINT8   DataByte0;
  UINT8   DataByte1;

  if (BurstCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  WaitTime = 0;
  do {
    //
    // BurstCount is UINT16, but it is not 2bytes aligned,
    // so it needs to use TpmReadByte to read two times
    //
    DataByte0   = TpmReadByte (INFINEON_TPM_BURST0_COUNT_0_DEFAULT);
    DataByte1   = TpmReadByte (INFINEON_TPM_BURST1_COUNT_0_DEFAULT);
    *BurstCount = (UINT16)((DataByte1 << 8) + DataByte0);
    if (*BurstCount != 0) {
      return EFI_SUCCESS;
    }
    MicroSecondDelay (30);
    WaitTime += 30;
  } while (WaitTime < TIS_TIMEOUT_D);

  return EFI_TIMEOUT;
}

/**
  Set TPM chip to ready state by sending ready command TIS_PC_STS_READY
  to Status Register in time.

  @retval EFI_SUCCESS  TPM chip enters into ready state.
  @retval EFI_TIMEOUT  TPM chip can't be set to ready state in time.
**/
EFI_STATUS
TisPcPrepareCommand (
  VOID
  )
{
  EFI_STATUS  Status;

  TpmWriteByte (INFINEON_TPM_STS_0_ADDRESS_DEFAULT, TIS_PC_STS_READY);
  Status = TisPcWaitRegisterBits (
             INFINEON_TPM_STS_0_ADDRESS_DEFAULT,
             TIS_PC_STS_READY,
             0,
             TIS_TIMEOUT_B
             );
  return Status;
}

/**
  This service requests use TPM12.

  @retval EFI_SUCCESS      Get the control of TPM12 chip.
  @retval EFI_NOT_FOUND    TPM12 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12RequestUseTpm (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Check to see if TPM exists
  //
  if (TpmReadByte (INFINEON_TPM_ACCESS_0_ADDRESS_DEFAULT) == 0xFF) {
    return EFI_NOT_FOUND;
  }

  TpmWriteByte (INFINEON_TPM_ACCESS_0_ADDRESS_DEFAULT, TIS_PC_ACC_RQUUSE);

  //
  // No locality set before, ACCESS_X.activeLocality MUST be valid within TIMEOUT_A
  //
  Status = TisPcWaitRegisterBits (
             INFINEON_TPM_ACCESS_0_ADDRESS_DEFAULT,
             (UINT8)(TIS_PC_ACC_ACTIVE |TIS_PC_VALID),
             0,
             TIS_TIMEOUT_A
             );
  return Status;
}

/**
  Send command to TPM for execution.

  @param[in] TpmBuffer   Buffer for TPM command data.
  @param[in] DataLength  TPM command data length.

  @retval EFI_SUCCESS  Operation completed successfully.
  @retval EFI_TIMEOUT  The register can't run into the expected status in time.

**/
EFI_STATUS
TisPcSend (
  IN UINT8   *TpmBuffer,
  IN UINT32  DataLength
  )
{
  UINT16      BurstCount;
  UINT32      Index;
  EFI_STATUS  Status;

  Status = TisPcPrepareCommand ();
  if (EFI_ERROR (Status)){
    DEBUG ((DEBUG_ERROR, "The TPM is not ready!\n"));
    goto Done;
  }
  Index = 0;
  while (Index < DataLength) {
    Status = TisPcReadBurstCount (&BurstCount);
    if (EFI_ERROR (Status)) {
      Status = EFI_TIMEOUT;
      goto Done;
    }
    for (; BurstCount > 0 && Index < DataLength; BurstCount--) {
      TpmWriteByte (INFINEON_TPM_DATA_FIFO_0_ADDRESS_DEFAULT, *(TpmBuffer + Index));
      Index++;
    }
  }
  //
  // Ensure the TPM status STS_EXPECT change from 1 to 0
  //
  Status = TisPcWaitRegisterBits (
             INFINEON_TPM_STS_0_ADDRESS_DEFAULT,
             (UINT8) TIS_PC_VALID,
             TIS_PC_STS_EXPECT,
             TIS_TIMEOUT_C
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Start the command
  //
  TpmWriteByte (INFINEON_TPM_STS_0_ADDRESS_DEFAULT, TIS_PC_STS_GO);

Done:
  if (EFI_ERROR (Status))  {
    //
    // Ensure the TPM state change from "Reception" to "Idle/Ready"
    //
    TpmWriteByte (INFINEON_TPM_STS_0_ADDRESS_DEFAULT, TIS_PC_STS_READY);
  }

  return Status;
}

/**
  Receive response data of last command from TPM.

  @param[out] TpmBuffer  Buffer for response data.
  @param[out] RespSize   Response data length.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_DEVICE_ERROR      Unexpected device status.
  @retval EFI_BUFFER_TOO_SMALL  Response data is too long.

**/
EFI_STATUS
TisPcReceive (
  OUT UINT8   *TpmBuffer,
  OUT UINT32  *RespSize
  )
{
  EFI_STATUS           Status;
  UINT16               BurstCount;
  UINT32               Index;
  UINT32               ResponseSize;
  TPM_RSP_COMMAND_HDR  *ResponseHeader;

  //
  // Wait for the command completion
  //
  Status = TisPcWaitRegisterBits (
             INFINEON_TPM_STS_0_ADDRESS_DEFAULT,
             (UINT8) (TIS_PC_VALID | TIS_PC_STS_DATA),
             0,
             TIS_TIMEOUT_B
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_TIMEOUT;
    goto Done;
  }
  //
  // Read the response data header and check it
  //
  Index = 0;
  BurstCount = 0;
  while (Index < sizeof (TPM_RSP_COMMAND_HDR)) {
    Status = TisPcReadBurstCount (&BurstCount);
    if (EFI_ERROR (Status)) {
      Status = EFI_TIMEOUT;
      goto Done;
    }
    for (; BurstCount > 0 ; BurstCount--) {
      *(TpmBuffer + Index) = TpmReadByte (INFINEON_TPM_DATA_FIFO_0_ADDRESS_DEFAULT);
      Index++;
      if (Index == sizeof (TPM_RSP_COMMAND_HDR))
        break;
    }
  }

  //
  // Check the response data header (tag, parasize and returncode)
  //
  ResponseHeader = (TPM_RSP_COMMAND_HDR *)TpmBuffer;
  if (SwapBytes16 (ReadUnaligned16 (&ResponseHeader->tag)) != TPM_TAG_RSP_COMMAND) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  ResponseSize = SwapBytes32 (ReadUnaligned32 (&ResponseHeader->paramSize));
  if (ResponseSize == sizeof (TPM_RSP_COMMAND_HDR)) {
    Status = EFI_SUCCESS;
    goto Done;
  }
  if (ResponseSize < sizeof (TPM_RSP_COMMAND_HDR)) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }
  if (*RespSize < ResponseSize) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }
  *RespSize = ResponseSize;

  //
  // Continue reading the remaining data
  //
  while (Index < ResponseSize) {
    for (; BurstCount > 0 ; BurstCount--) {
      *(TpmBuffer + Index) = TpmReadByte (INFINEON_TPM_DATA_FIFO_0_ADDRESS_DEFAULT);
      Index++;
      if (Index == ResponseSize) {
        Status = EFI_SUCCESS;
        goto Done;
      }
    }
    Status = TisPcReadBurstCount (&BurstCount);
    if (EFI_ERROR (Status) && (Index < ResponseSize)) {
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }
  }

Done:
  //
  // Ensure the TPM state change from "Execution" or "Completion" to "Idle/Ready"
  //
  TpmWriteByte (INFINEON_TPM_STS_0_ADDRESS_DEFAULT, TIS_PC_STS_READY);

  return Status;
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
  EFI_STATUS  Status;

  Status = TisPcSend (InputParameterBlock, InputParameterBlockSize);
  if (!EFI_ERROR (Status)) {
    Status = TisPcReceive (OutputParameterBlock, OutputParameterBlockSize);
  }
  return Status;
}
