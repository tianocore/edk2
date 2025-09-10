/** @file
  PTP (Platform TPM Profile) CRB (Command Response Buffer) interface used by DTPM2.0 library.

Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.<BR>
Copyright (c), Microsoft Corporation.
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

#include <IndustryStandard/TpmPtp.h>
#include <IndustryStandard/TpmTis.h>

#include "Tpm2DeviceLibDTpm.h"

#include "Tpm2Ptp.h"

//
// Execution of the command may take from several seconds to minutes for certain
// commands, such as key generation.
//
#define PTP_TIMEOUT_MAX  (90000 * 1000)             // 90s

//
// Max TPM command/response length
//
#define TPMCMDBUFLENGTH  0x500

//
// Max retry count according to Spec TCG PC Client Device Driver Design Principles
// for TPM2.0, Version 1.1, Revision 0.04, Section 7.2.1
//
#define RETRY_CNT_MAX  3

/**
  Check whether TPM PTP register exist.

  @param[in] Reg  Pointer to PTP register.

  @retval    TRUE    TPM PTP exists.
  @retval    FALSE   TPM PTP is not found.
**/
BOOLEAN
Tpm2IsPtpPresence (
  IN VOID  *Reg
  )
{
  UINT8  RegRead;

  RegRead = MmioRead8 ((UINTN)Reg);
  if (RegRead == 0xFF) {
    //
    // No TPM chip
    //
    return FALSE;
  }

  return TRUE;
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
PtpCrbWaitRegisterBits (
  IN      UINT32  *Register,
  IN      UINT32  BitSet,
  IN      UINT32  BitClear,
  IN      UINT32  TimeOut
  )
{
  UINT32  RegRead;
  UINT32  WaitTime;

  for (WaitTime = 0; WaitTime < TimeOut; WaitTime += 30) {
    RegRead = MmioRead32 ((UINTN)Register);
    if (((RegRead & BitSet) == BitSet) && ((RegRead & BitClear) == 0)) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay (30);
  }

  return EFI_TIMEOUT;
}

/**
  Get the control of TPM chip.

  @param[in] CrbReg                Pointer to CRB register.

  @retval    EFI_SUCCESS           Get the control of TPM chip.
  @retval    EFI_INVALID_PARAMETER CrbReg is NULL.
  @retval    EFI_NOT_FOUND         TPM chip doesn't exit.
  @retval    EFI_TIMEOUT           Can't get the TPM control in time.
**/
EFI_STATUS
PtpCrbRequestUseTpm (
  IN      PTP_CRB_REGISTERS_PTR  CrbReg
  )
{
  EFI_STATUS  Status;

  if (!Tpm2IsPtpPresence (CrbReg)) {
    return EFI_NOT_FOUND;
  }

  MmioWrite32 ((UINTN)&CrbReg->LocalityControl, PTP_CRB_LOCALITY_CONTROL_REQUEST_ACCESS);
  Status = PtpCrbWaitRegisterBits (
             &CrbReg->LocalityStatus,
             PTP_CRB_LOCALITY_STATUS_GRANTED,
             0,
             PTP_TIMEOUT_A
             );
  return Status;
}

/**
  Send a command to TPM for execution and return response data.

  @param[in]      CrbReg        TPM register space base address.
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
PtpCrbTpmCommand (
  IN     PTP_CRB_REGISTERS_PTR  CrbReg,
  IN     UINT8                  *BufferIn,
  IN     UINT32                 SizeIn,
  IN OUT UINT8                  *BufferOut,
  IN OUT UINT32                 *SizeOut
  )
{
  EFI_STATUS  Status;
  UINT32      Index;
  UINT32      TpmOutSize;
  UINT16      Data16;
  UINT32      Data32;
  UINT8       RetryCnt;

  DEBUG_CODE_BEGIN ();
  DumpTpmInputBlock (SizeIn, BufferIn);
  DEBUG_CODE_END ();
  TpmOutSize = 0;

  RetryCnt = 0;
  while (TRUE) {
    //
    // STEP 0:
    // if CapCRbIdelByPass == 0, enforce Idle state before sending command
    //
    if ((GetCachedIdleByPass () == 0) && ((MmioRead32 ((UINTN)&CrbReg->CrbControlStatus) & PTP_CRB_CONTROL_AREA_STATUS_TPM_IDLE) == 0)) {
      Status = PtpCrbWaitRegisterBits (
                 &CrbReg->CrbControlStatus,
                 PTP_CRB_CONTROL_AREA_STATUS_TPM_IDLE,
                 0,
                 PTP_TIMEOUT_C
                 );
      if (EFI_ERROR (Status)) {
        RetryCnt++;
        if (RetryCnt < RETRY_CNT_MAX) {
          MmioWrite32 ((UINTN)&CrbReg->CrbControlRequest, PTP_CRB_CONTROL_AREA_REQUEST_GO_IDLE);
          continue;
        } else {
          //
          // Try to goIdle to recover TPM
          //
          Status = EFI_DEVICE_ERROR;
          goto GoIdle_Exit;
        }
      }
    }

    //
    // STEP 1:
    // Ready is any time the TPM is ready to receive a command, following a write
    // of 1 by software to Request.cmdReady, as indicated by the Status field
    // being cleared to 0.
    //
    MmioWrite32 ((UINTN)&CrbReg->CrbControlRequest, PTP_CRB_CONTROL_AREA_REQUEST_COMMAND_READY);
    Status = PtpCrbWaitRegisterBits (
               &CrbReg->CrbControlRequest,
               0,
               PTP_CRB_CONTROL_AREA_REQUEST_COMMAND_READY,
               PTP_TIMEOUT_C
               );
    if (EFI_ERROR (Status)) {
      RetryCnt++;
      if (RetryCnt < RETRY_CNT_MAX) {
        MmioWrite32 ((UINTN)&CrbReg->CrbControlRequest, PTP_CRB_CONTROL_AREA_REQUEST_GO_IDLE);
        continue;
      } else {
        Status = EFI_DEVICE_ERROR;
        goto GoIdle_Exit;
      }
    }

    Status = PtpCrbWaitRegisterBits (
               &CrbReg->CrbControlStatus,
               0,
               PTP_CRB_CONTROL_AREA_STATUS_TPM_IDLE,
               PTP_TIMEOUT_C
               );
    if (EFI_ERROR (Status)) {
      RetryCnt++;
      if (RetryCnt < RETRY_CNT_MAX) {
        MmioWrite32 ((UINTN)&CrbReg->CrbControlRequest, PTP_CRB_CONTROL_AREA_REQUEST_GO_IDLE);
        continue;
      } else {
        Status = EFI_DEVICE_ERROR;
        goto GoIdle_Exit;
      }
    }

    break;
  }

  //
  // STEP 2:
  // Command Reception occurs following a Ready state between the write of the
  // first byte of a command to the Command Buffer and the receipt of a write
  // of 1 to Start.
  //
  for (Index = 0; Index < SizeIn; Index++) {
    MmioWrite8 ((UINTN)&CrbReg->CrbDataBuffer[Index], BufferIn[Index]);
  }

  MmioWrite32 ((UINTN)&CrbReg->CrbControlCommandAddressHigh, (UINT32)RShiftU64 ((UINTN)CrbReg->CrbDataBuffer, 32));
  MmioWrite32 ((UINTN)&CrbReg->CrbControlCommandAddressLow, (UINT32)(UINTN)CrbReg->CrbDataBuffer);
  MmioWrite32 ((UINTN)&CrbReg->CrbControlCommandSize, sizeof (CrbReg->CrbDataBuffer));

  MmioWrite64 ((UINTN)&CrbReg->CrbControlResponseAddrss, (UINT32)(UINTN)CrbReg->CrbDataBuffer);
  MmioWrite32 ((UINTN)&CrbReg->CrbControlResponseSize, sizeof (CrbReg->CrbDataBuffer));

  //
  // STEP 3:
  // Command Execution occurs after receipt of a 1 to Start and the TPM
  // clearing Start to 0.
  //
  MmioWrite32 ((UINTN)&CrbReg->CrbControlStart, PTP_CRB_CONTROL_START);
  Status = PtpCrbWaitRegisterBits (
             &CrbReg->CrbControlStart,
             0,
             PTP_CRB_CONTROL_START,
             PTP_TIMEOUT_MAX
             );
  if (EFI_ERROR (Status)) {
    //
    // Command Completion check timeout. Cancel the currently executing command by writing TPM_CRB_CTRL_CANCEL,
    // Expect TPM_RC_CANCELLED or successfully completed response.
    //
    MmioWrite32 ((UINTN)&CrbReg->CrbControlCancel, PTP_CRB_CONTROL_CANCEL);
    Status = PtpCrbWaitRegisterBits (
               &CrbReg->CrbControlStart,
               0,
               PTP_CRB_CONTROL_START,
               PTP_TIMEOUT_B
               );
    MmioWrite32 ((UINTN)&CrbReg->CrbControlCancel, 0);

    if (EFI_ERROR (Status)) {
      //
      // Still in Command Execution state. Try to goIdle, the behavior is agnostic.
      //
      Status = EFI_DEVICE_ERROR;
      goto GoIdle_Exit;
    }
  }

  //
  // STEP 4:
  // Command Completion occurs after completion of a command (indicated by the
  // TPM clearing TPM_CRB_CTRL_Start_x to 0) and before a write of a 1 by the
  // software to Request.goIdle.
  //

  //
  // Get response data header
  //
  for (Index = 0; Index < sizeof (TPM2_RESPONSE_HEADER); Index++) {
    BufferOut[Index] = MmioRead8 ((UINTN)&CrbReg->CrbDataBuffer[Index]);
  }

  //
  // Check the response data header (tag, parasize and returncode)
  //
  CopyMem (&Data16, BufferOut, sizeof (UINT16));
  // TPM2 should not use this RSP_COMMAND
  if (SwapBytes16 (Data16) == TPM_ST_RSP_COMMAND) {
    DEBUG ((DEBUG_ERROR, "TPM2: TPM_ST_RSP error - %x\n", TPM_ST_RSP_COMMAND));
    Status = EFI_UNSUPPORTED;
    goto GoIdle_Exit;
  }

  CopyMem (&Data32, (BufferOut + 2), sizeof (UINT32));
  TpmOutSize = SwapBytes32 (Data32);
  if (*SizeOut < TpmOutSize) {
    //
    // Command completed, but buffer is not enough
    //
    Status = EFI_BUFFER_TOO_SMALL;
    goto GoIdle_Exit;
  }

  *SizeOut = TpmOutSize;
  //
  // Continue reading the remaining data
  //
  for (Index = sizeof (TPM2_RESPONSE_HEADER); Index < TpmOutSize; Index++) {
    BufferOut[Index] = MmioRead8 ((UINTN)&CrbReg->CrbDataBuffer[Index]);
  }

  DEBUG_CODE_BEGIN ();
  DumpTpmOutputBlock (TpmOutSize, BufferOut);
  DEBUG_CODE_END ();

  //
  // Do not wait for state transition for TIMEOUT_C
  // This function will try to wait 2 TIMEOUT_C at the beginning in next call.
  //
GoIdle_Exit:

  //
  //  Return to Idle state by setting TPM_CRB_CTRL_STS_x.Status.goIdle to 1.
  //
  MmioWrite32 ((UINTN)&CrbReg->CrbControlRequest, PTP_CRB_CONTROL_AREA_REQUEST_GO_IDLE);

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
  );

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
  IN     TIS_PC_REGISTERS_PTR  TisReg
  );

/**
  Return PTP interface type.

  @param[in] Register                Pointer to PTP register.

  @return PTP interface type.
**/
TPM2_PTP_INTERFACE_TYPE
Tpm2GetPtpInterface (
  IN VOID  *Register
  )
{
  PTP_CRB_INTERFACE_IDENTIFIER   InterfaceId;
  PTP_FIFO_INTERFACE_CAPABILITY  InterfaceCapability;

  if (!Tpm2IsPtpPresence (Register)) {
    return Tpm2PtpInterfaceMax;
  }

  //
  // Check interface id
  //
  InterfaceId.Uint32         = MmioRead32 ((UINTN)&((PTP_CRB_REGISTERS *)Register)->InterfaceId);
  InterfaceCapability.Uint32 = MmioRead32 ((UINTN)&((PTP_FIFO_REGISTERS *)Register)->InterfaceCapability);

  if ((InterfaceId.Bits.InterfaceType == PTP_INTERFACE_IDENTIFIER_INTERFACE_TYPE_CRB) &&
      (InterfaceId.Bits.InterfaceVersion == PTP_INTERFACE_IDENTIFIER_INTERFACE_VERSION_CRB) &&
      (InterfaceId.Bits.CapCRB != 0))
  {
    return Tpm2PtpInterfaceCrb;
  }

  if ((InterfaceId.Bits.InterfaceType == PTP_INTERFACE_IDENTIFIER_INTERFACE_TYPE_FIFO) &&
      (InterfaceId.Bits.InterfaceVersion == PTP_INTERFACE_IDENTIFIER_INTERFACE_VERSION_FIFO) &&
      (InterfaceId.Bits.CapFIFO != 0) &&
      (InterfaceCapability.Bits.InterfaceVersion == INTERFACE_CAPABILITY_INTERFACE_VERSION_PTP))
  {
    return Tpm2PtpInterfaceFifo;
  }

  if (InterfaceId.Bits.InterfaceType == PTP_INTERFACE_IDENTIFIER_INTERFACE_TYPE_TIS) {
    return Tpm2PtpInterfaceTis;
  }

  return Tpm2PtpInterfaceMax;
}

/**
  Return PTP CRB interface IdleByPass state.

  @param[in] Register                Pointer to PTP register.

  @return PTP CRB interface IdleByPass state.
**/
UINT8
Tpm2GetIdleByPass (
  IN VOID  *Register
  )
{
  PTP_CRB_INTERFACE_IDENTIFIER  InterfaceId;

  //
  // Check interface id
  //
  InterfaceId.Uint32 = MmioRead32 ((UINTN)&((PTP_CRB_REGISTERS *)Register)->InterfaceId);

  return (UINT8)(InterfaceId.Bits.CapCRBIdleBypass);
}

/**
  Dump PTP register information.

  @param[in] Register                Pointer to PTP register.
**/
VOID
DumpPtpInfo (
  IN VOID  *Register
  )
{
  PTP_CRB_INTERFACE_IDENTIFIER   InterfaceId;
  PTP_FIFO_INTERFACE_CAPABILITY  InterfaceCapability;
  UINT8                          StatusEx;
  UINT16                         Vid;
  UINT16                         Did;
  UINT8                          Rid;
  TPM2_PTP_INTERFACE_TYPE        PtpInterface;

  if (!Tpm2IsPtpPresence (Register)) {
    return;
  }

  InterfaceId.Uint32         = MmioRead32 ((UINTN)&((PTP_CRB_REGISTERS *)Register)->InterfaceId);
  InterfaceCapability.Uint32 = MmioRead32 ((UINTN)&((PTP_FIFO_REGISTERS *)Register)->InterfaceCapability);
  StatusEx                   = MmioRead8 ((UINTN)&((PTP_FIFO_REGISTERS *)Register)->StatusEx);

  //
  // Dump InterfaceId Register for PTP
  //
  DEBUG ((DEBUG_INFO, "InterfaceId - 0x%08x\n", InterfaceId.Uint32));
  DEBUG ((DEBUG_INFO, "  InterfaceType    - 0x%02x\n", InterfaceId.Bits.InterfaceType));
  if (InterfaceId.Bits.InterfaceType != PTP_INTERFACE_IDENTIFIER_INTERFACE_TYPE_TIS) {
    DEBUG ((DEBUG_INFO, "  InterfaceVersion - 0x%02x\n", InterfaceId.Bits.InterfaceVersion));
    DEBUG ((DEBUG_INFO, "  CapFIFO          - 0x%x\n", InterfaceId.Bits.CapFIFO));
    DEBUG ((DEBUG_INFO, "  CapCRB           - 0x%x\n", InterfaceId.Bits.CapCRB));
  }

  //
  // Dump Capability Register for TIS and FIFO
  //
  DEBUG ((DEBUG_INFO, "InterfaceCapability - 0x%08x\n", InterfaceCapability.Uint32));
  if ((InterfaceId.Bits.InterfaceType == PTP_INTERFACE_IDENTIFIER_INTERFACE_TYPE_TIS) ||
      (InterfaceId.Bits.InterfaceType == PTP_INTERFACE_IDENTIFIER_INTERFACE_TYPE_FIFO))
  {
    DEBUG ((DEBUG_INFO, "  InterfaceVersion - 0x%x\n", InterfaceCapability.Bits.InterfaceVersion));
  }

  //
  // Dump StatusEx Register for PTP FIFO
  //
  DEBUG ((DEBUG_INFO, "StatusEx - 0x%02x\n", StatusEx));
  if (InterfaceCapability.Bits.InterfaceVersion == INTERFACE_CAPABILITY_INTERFACE_VERSION_PTP) {
    DEBUG ((DEBUG_INFO, "  TpmFamily - 0x%x\n", (StatusEx & PTP_FIFO_STS_EX_TPM_FAMILY) >> PTP_FIFO_STS_EX_TPM_FAMILY_OFFSET));
  }

  Vid          = 0xFFFF;
  Did          = 0xFFFF;
  Rid          = 0xFF;
  PtpInterface = GetCachedPtpInterface ();
  DEBUG ((DEBUG_INFO, "PtpInterface - %x\n", PtpInterface));
  switch (PtpInterface) {
    case Tpm2PtpInterfaceCrb:
      Vid = MmioRead16 ((UINTN)&((PTP_CRB_REGISTERS *)Register)->Vid);
      Did = MmioRead16 ((UINTN)&((PTP_CRB_REGISTERS *)Register)->Did);
      Rid = (UINT8)InterfaceId.Bits.Rid;
      break;
    case Tpm2PtpInterfaceFifo:
    case Tpm2PtpInterfaceTis:
      Vid = MmioRead16 ((UINTN)&((PTP_FIFO_REGISTERS *)Register)->Vid);
      Did = MmioRead16 ((UINTN)&((PTP_FIFO_REGISTERS *)Register)->Did);
      Rid = MmioRead8 ((UINTN)&((PTP_FIFO_REGISTERS *)Register)->Rid);
      break;
    default:
      break;
  }

  DEBUG ((DEBUG_INFO, "VID - 0x%04x\n", Vid));
  DEBUG ((DEBUG_INFO, "DID - 0x%04x\n", Did));
  DEBUG ((DEBUG_INFO, "RID - 0x%02x\n", Rid));
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
DTpm2SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  )
{
  TPM2_PTP_INTERFACE_TYPE  PtpInterface;

  PtpInterface = GetCachedPtpInterface ();
  switch (PtpInterface) {
    case Tpm2PtpInterfaceCrb:
      return PtpCrbTpmCommand (
               (PTP_CRB_REGISTERS_PTR)(UINTN)PcdGet64 (PcdTpmBaseAddress),
               InputParameterBlock,
               InputParameterBlockSize,
               OutputParameterBlock,
               OutputParameterBlockSize
               );
    case Tpm2PtpInterfaceFifo:
    case Tpm2PtpInterfaceTis:
      return Tpm2TisTpmCommand (
               (TIS_PC_REGISTERS_PTR)(UINTN)PcdGet64 (PcdTpmBaseAddress),
               InputParameterBlock,
               InputParameterBlockSize,
               OutputParameterBlock,
               OutputParameterBlockSize
               );
    default:
      return EFI_NOT_FOUND;
  }
}

/**
  This service requests use TPM2.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
DTpm2RequestUseTpm (
  VOID
  )
{
  TPM2_PTP_INTERFACE_TYPE  PtpInterface;

  PtpInterface = GetCachedPtpInterface ();
  switch (PtpInterface) {
    case Tpm2PtpInterfaceCrb:
      return PtpCrbRequestUseTpm ((PTP_CRB_REGISTERS_PTR)(UINTN)PcdGet64 (PcdTpmBaseAddress));
    case Tpm2PtpInterfaceFifo:
    case Tpm2PtpInterfaceTis:
      return TisPcRequestUseTpm ((TIS_PC_REGISTERS_PTR)(UINTN)PcdGet64 (PcdTpmBaseAddress));
    default:
      return EFI_NOT_FOUND;
  }
}
