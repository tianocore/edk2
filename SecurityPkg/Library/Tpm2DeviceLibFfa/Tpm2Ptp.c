/** @file
  PTP (Platform TPM Profile) CRB (Command Response Buffer) interface used by dTPM2.0 library.

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
#include <Library/Tpm2DumpLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>

#include <IndustryStandard/TpmPtp.h>
#include <Guid/Tpm2ServiceFfa.h>

#include "Tpm2DeviceLibFfa.h"

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
  Status = Tpm2ServiceStart (
             TPM2_FFA_START_FUNC_QUALIFIER_LOCALITY,
             0
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PtpCrbRequestUseTpm: Request access failed - %r\n", Status));
    goto Exit;
  }

  // Double check to see if the locality we requested is granted or not.
  if ((MmioRead32 ((UINTN)&CrbReg->LocalityStatus) & PTP_CRB_LOCALITY_STATUS_GRANTED) == 0) {
    DEBUG ((DEBUG_ERROR, "PtpCrbRequestUseTpm: Locality not granted - %r\n", Status));
    Status = EFI_TIMEOUT;
    goto Exit;
  }

Exit:
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

  DEBUG_CODE_BEGIN ();
  DumpTpmInputBlock (SizeIn, BufferIn);
  DEBUG_CODE_END ();

  TpmOutSize = 0;

  //
  // STEP 1:
  // Ready is any time the TPM is ready to receive a command, following a write
  // of 1 by software to Request.cmdReady, as indicated by the Status field
  // being cleared to 0.
  //
  MmioWrite32 ((UINTN)&CrbReg->CrbControlRequest, PTP_CRB_CONTROL_AREA_REQUEST_COMMAND_READY);
  Status = Tpm2ServiceStart (
             TPM2_FFA_START_FUNC_QUALIFIER_COMMAND,
             0
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PtpCrbTpmCommand: Request command ready failed - %r\n", Status));
    goto Exit;
  }

  for (Index = 0; Index < SizeIn; Index++) {
    MmioWrite8 ((UINTN)&CrbReg->CrbDataBuffer[Index], BufferIn[Index]);
  }

  //
  // STEP 2:
  // Command Execution occurs after receipt of a 1 to Start and the TPM
  // clearing Start to 0.
  //
  MmioWrite32 ((UINTN)&CrbReg->CrbControlStart, PTP_CRB_CONTROL_START);
  Status = Tpm2ServiceStart (
             TPM2_FFA_START_FUNC_QUALIFIER_COMMAND,
             0
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PtpCrbTpmCommand: Control start failed - %r\n", Status));
    goto Exit;
  }

  //
  // STEP 3:
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
    goto Exit;
  }

  CopyMem (&Data32, (BufferOut + 2), sizeof (UINT32));
  TpmOutSize = SwapBytes32 (Data32);
  if (*SizeOut < TpmOutSize) {
    //
    // Command completed, but buffer is not enough
    //
    Status = EFI_BUFFER_TOO_SMALL;
    goto Exit;
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

Exit:

  //
  //  Return to Idle state by setting TPM_CRB_CTRL_STS_x.Status.goIdle to 1.
  //
  MmioWrite32 ((UINTN)&CrbReg->CrbControlRequest, PTP_CRB_CONTROL_AREA_REQUEST_GO_IDLE);
  Status = Tpm2ServiceStart (
             TPM2_FFA_START_FUNC_QUALIFIER_COMMAND,
             0
             );

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
FfaTpm2SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  )
{
  return PtpCrbTpmCommand (
           (PTP_CRB_REGISTERS_PTR)(UINTN)PcdGet64 (PcdTpmBaseAddress),
           InputParameterBlock,
           InputParameterBlockSize,
           OutputParameterBlock,
           OutputParameterBlockSize
           );
}

/**
  This service requests use TPM2 over FF-A.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
FfaTpm2RequestUseTpm (
  VOID
  )
{
  return PtpCrbRequestUseTpm ((PTP_CRB_REGISTERS_PTR)(UINTN)PcdGet64 (PcdTpmBaseAddress));
}
