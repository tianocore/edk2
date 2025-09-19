/** @file
  TIS (TPM Interface Specification) functions used by dTPM2.0 library.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Tpm20.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/TpmTis.h>

#include "Tpm2Ptp.h"

#define TIS_TIMEOUT_MAX  (90000 * 1000)             // 90s

//
// Max TPM command/response length
//
#define TPMCMDBUFLENGTH  0x500

/**
  Check whether TPM chip exist.

  @param[in] TisReg  Pointer to TIS register.

  @retval    TRUE    TPM chip exists.
  @retval    FALSE   TPM chip is not found.
**/
BOOLEAN
TisPcPresenceCheck (
  IN      TIS_PC_REGISTERS_PTR  TisReg
  )
{
  UINT8  RegRead;

  RegRead = MmioRead8 ((UINTN)&TisReg->Access);
  return (BOOLEAN)(RegRead != (UINT8)-1);
}

/**
  Check whether the value of a TPM chip register satisfies the input BIT setting.

  @param[in]  Register     Address port of register to be checked.
  @param[in]  BitSet       Check these data bits are set.
  @param[in]  BitClear     Check these data bits are clear.
  @param[in]  TimeOut      The max wait time (unit MicroSecond) when checking register.

  @retval     EFI_SUCCESS  The register satisfies the check bit.
  @retval     EFI_TIMEOUT  The register can't run into the expected status in time.
**/
EFI_STATUS
TisPcWaitRegisterBits (
  IN      UINT8   *Register,
  IN      UINT8   BitSet,
  IN      UINT8   BitClear,
  IN      UINT32  TimeOut
  )
{
  UINT8   RegRead;
  UINT32  WaitTime;

  for (WaitTime = 0; WaitTime < TimeOut; WaitTime += 30) {
    RegRead = MmioRead8 ((UINTN)Register);
    if (((RegRead & BitSet) == BitSet) && ((RegRead & BitClear) == 0)) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay (30);
  }

  return EFI_TIMEOUT;
}

/**
  Get BurstCount by reading the burstCount field of a TIS register
  in the time of default TIS_TIMEOUT_D.

  @param[in]  TisReg                Pointer to TIS register.
  @param[out] BurstCount            Pointer to a buffer to store the got BurstCount.

  @retval     EFI_SUCCESS           Get BurstCount.
  @retval     EFI_INVALID_PARAMETER TisReg is NULL or BurstCount is NULL.
  @retval     EFI_TIMEOUT           BurstCount can't be got in time.
**/
EFI_STATUS
TisPcReadBurstCount (
  IN      TIS_PC_REGISTERS_PTR  TisReg,
  OUT  UINT16                   *BurstCount
  )
{
  UINT32  WaitTime;
  UINT8   DataByte0;
  UINT8   DataByte1;

  if ((BurstCount == NULL) || (TisReg == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  WaitTime = 0;
  do {
    //
    // TIS_PC_REGISTERS_PTR->burstCount is UINT16, but it is not 2bytes aligned,
    // so it needs to use MmioRead8 to read two times
    //
    DataByte0   = MmioRead8 ((UINTN)&TisReg->BurstCount);
    DataByte1   = MmioRead8 ((UINTN)&TisReg->BurstCount + 1);
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

  @param[in] TisReg                Pointer to TIS register.

  @retval    EFI_SUCCESS           TPM chip enters into ready state.
  @retval    EFI_INVALID_PARAMETER TisReg is NULL.
  @retval    EFI_TIMEOUT           TPM chip can't be set to ready state in time.
**/
EFI_STATUS
TisPcPrepareCommand (
  IN      TIS_PC_REGISTERS_PTR  TisReg
  )
{
  EFI_STATUS  Status;

  if (TisReg == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MmioWrite8 ((UINTN)&TisReg->Status, TIS_PC_STS_READY);
  Status = TisPcWaitRegisterBits (
             &TisReg->Status,
             TIS_PC_STS_READY,
             0,
             TIS_TIMEOUT_B
             );
  return Status;
}

/**
  Get the control of TPM chip by sending requestUse command TIS_PC_ACC_RQUUSE
  to ACCESS Register in the time of default TIS_TIMEOUT_A.

  @param[in] TisReg                Pointer to TIS register.

  @retval    EFI_SUCCESS           Get the control of TPM chip.
  @retval    EFI_INVALID_PARAMETER TisReg is NULL.
  @retval    EFI_NOT_FOUND         TPM chip doesn't exit.
  @retval    EFI_TIMEOUT           Can't get the TPM control in time.
**/
EFI_STATUS
TisPcRequestUseTpm (
  IN      TIS_PC_REGISTERS_PTR  TisReg
  )
{
  EFI_STATUS  Status;

  if (TisReg == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!TisPcPresenceCheck (TisReg)) {
    return EFI_NOT_FOUND;
  }

  MmioWrite8 ((UINTN)&TisReg->Access, TIS_PC_ACC_RQUUSE);
  Status = TisPcWaitRegisterBits (
             &TisReg->Access,
             (UINT8)(TIS_PC_ACC_ACTIVE |TIS_PC_VALID),
             0,
             TIS_TIMEOUT_A
             );
  return Status;
}

/**
  Send a command to TPM for execution and return response data.

  @param[in]      TisReg        TPM register space base address.
  @param[in]      BufferIn      Buffer for command data.
  @param[in]      SizeIn        Size of command data.
  @param[in, out] BufferOut     Buffer for response data.
  @param[in, out] SizeOut       Size of response data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.
  @retval EFI_UNSUPPORTED       Unsupported TPM version

**/
EFI_STATUS
Tpm2TisTpmCommand (
  IN     TIS_PC_REGISTERS_PTR  TisReg,
  IN     UINT8                 *BufferIn,
  IN     UINT32                SizeIn,
  IN OUT UINT8                 *BufferOut,
  IN OUT UINT32                *SizeOut
  )
{
  EFI_STATUS  Status;
  UINT16      BurstCount;
  UINT32      Index;
  UINT32      TpmOutSize;
  UINT16      Data16;
  UINT32      Data32;
  UINT32      CommandCode;

  DEBUG_CODE_BEGIN ();
  DumpTpmInputBlock (SizeIn, BufferIn);
  DEBUG_CODE_END ();
  TpmOutSize = 0;

  Status = TisPcPrepareCommand (TisReg);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tpm2 is not ready for command!\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // Send the command data to Tpm
  //
  Index = 0;
  while (Index < SizeIn) {
    Status = TisPcReadBurstCount (TisReg, &BurstCount);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    for ( ; BurstCount > 0 && Index < SizeIn; BurstCount--) {
      MmioWrite8 ((UINTN)&TisReg->DataFifo, *(BufferIn + Index));
      Index++;
    }
  }

  //
  // Check the Tpm status STS_EXPECT change from 1 to 0
  //
  Status = TisPcWaitRegisterBits (
             &TisReg->Status,
             (UINT8)TIS_PC_VALID,
             TIS_PC_STS_EXPECT,
             TIS_TIMEOUT_C
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tpm2 The send buffer too small!\n"));
    Status = EFI_BUFFER_TOO_SMALL;
    goto Exit;
  }

  //
  // Executed the TPM command and waiting for the response data ready
  //
  MmioWrite8 ((UINTN)&TisReg->Status, TIS_PC_STS_GO);

  //
  // NOTE: That may take many seconds to minutes for certain commands, such as key generation.
  //
  Status = TisPcWaitRegisterBits (
             &TisReg->Status,
             (UINT8)(TIS_PC_VALID | TIS_PC_STS_DATA),
             0,
             TIS_TIMEOUT_MAX
             );
  if (EFI_ERROR (Status)) {
    //
    // dataAvail check timeout. Cancel the currently executing command by writing commandCancel,
    // Expect TPM_RC_CANCELLED or successfully completed response.
    //
    DEBUG ((DEBUG_ERROR, "Wait for Tpm2 response data time out. Trying to cancel the command!!\n"));

    MmioWrite32 ((UINTN)&TisReg->Status, TIS_PC_STS_CANCEL);
    Status = TisPcWaitRegisterBits (
               &TisReg->Status,
               (UINT8)(TIS_PC_VALID | TIS_PC_STS_DATA),
               0,
               TIS_TIMEOUT_B
               );
    //
    // Do not clear CANCEL bit here because Writes of 0 to this bit are ignored
    //
    if (EFI_ERROR (Status)) {
      //
      // Cancel executing command fail to get any response
      // Try to abort the command with write of a 1 to commandReady in Command Execution state
      //
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }
  }

  //
  // Get response data header
  //
  Index      = 0;
  BurstCount = 0;
  while (Index < sizeof (TPM2_RESPONSE_HEADER)) {
    Status = TisPcReadBurstCount (TisReg, &BurstCount);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    for ( ; BurstCount > 0; BurstCount--) {
      *(BufferOut + Index) = MmioRead8 ((UINTN)&TisReg->DataFifo);
      Index++;
      if (Index == sizeof (TPM2_RESPONSE_HEADER)) {
        break;
      }
    }
  }

  //
  // Check the response data header (tag, parameter size and return code )
  //
  CopyMem (&Data16, BufferOut, sizeof (UINT16));
  // TPM2 should not use this RSP_COMMAND
  if (SwapBytes16 (Data16) == TPM_ST_RSP_COMMAND) {
    DEBUG ((DEBUG_ERROR, "TPM2: TPM_ST_RSP error - %x\n", TPM_ST_RSP_COMMAND));
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  CopyMem (&Data32, (BufferOut + 2), sizeof (UINT32));
  TpmOutSize = SwapBytes32 (Data32);
  if (*SizeOut < TpmOutSize) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto Exit;
  }

  *SizeOut = TpmOutSize;
  //
  // Continue reading the remaining data
  //
  while ( Index < TpmOutSize ) {
    for ( ; BurstCount > 0; BurstCount--) {
      *(BufferOut + Index) = MmioRead8 ((UINTN)&TisReg->DataFifo);
      Index++;
      if (Index == TpmOutSize) {
        Status = EFI_SUCCESS;
        goto Exit;
      }
    }

    Status = TisPcReadBurstCount (TisReg, &BurstCount);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }
  }

Exit:
  DEBUG_CODE_BEGIN ();
  if (SizeIn >= sizeof (TPM2_COMMAND_HEADER)) {
    CommandCode = SwapBytes32 (((TPM2_COMMAND_HEADER *)BufferIn)->commandCode);
  } else {
    CommandCode = 0;
  }

  DumpTpmOutputBlock (TpmOutSize, BufferOut, CommandCode);
  DEBUG_CODE_END ();
  MmioWrite8 ((UINTN)&TisReg->Status, TIS_PC_STS_READY);
  return Status;
}

/**
  This service enables the sending of commands to the TPM2.

  @param[in]      InputParameterBlockSize  Size of the TPM2 input parameter block.
  @param[in]      InputParameterBlock      Pointer to the TPM2 input parameter block.
  @param[in,out]  OutputParameterBlockSize Size of the TPM2 output parameter block.
  @param[in]      OutputParameterBlock     Pointer to the TPM2 output parameter block.

  @retval EFI_SUCCESS            The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR       The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_BUFFER_TOO_SMALL   The output parameter block is too small.
**/
EFI_STATUS
EFIAPI
DTpm2TisSubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  )
{
  return Tpm2TisTpmCommand (
           (TIS_PC_REGISTERS_PTR)(UINTN)PcdGet64 (PcdTpmBaseAddress),
           InputParameterBlock,
           InputParameterBlockSize,
           OutputParameterBlock,
           OutputParameterBlockSize
           );
}

/**
  This service requests use TPM2.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
DTpm2TisRequestUseTpm (
  VOID
  )
{
  return TisPcRequestUseTpm ((TIS_PC_REGISTERS_PTR)(UINTN)PcdGet64 (PcdTpmBaseAddress));
}
