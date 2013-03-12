/** @file  
  TIS (TPM Interface Specification) functions used by TPM Dxe driver.
  
Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <IndustryStandard/Tpm12.h>
#include <Library/TimerLib.h>
#include <Library/TpmCommLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

STATIC UINT8                        TpmCommandBuf[TPMCMDBUFLENGTH];

/**
  Send command to TPM for execution.

  @param[in] TisReg     TPM register space base address.  
  @param[in] TpmBuffer  Buffer for TPM command data.  
  @param[in] DataLength TPM command data length.  
 
  @retval EFI_SUCCESS   Operation completed successfully.
  @retval EFI_TIMEOUT   The register can't run into the expected status in time.

**/
EFI_STATUS
TisPcSend (
  IN     TIS_PC_REGISTERS_PTR       TisReg,
  IN     UINT8                      *TpmBuffer,
  IN     UINT32                     DataLength
  )
{
  UINT16                            BurstCount;
  UINT32                            Index;
  EFI_STATUS                        Status;

  Status = TisPcPrepareCommand (TisReg);
  if (EFI_ERROR (Status)){
    DEBUG ((DEBUG_ERROR, "The Tpm not ready!\n"));
    return Status;
  }
  Index = 0;
  while (Index < DataLength) {
    Status = TisPcReadBurstCount (TisReg, &BurstCount);
    if (EFI_ERROR (Status)) {
      return EFI_TIMEOUT;
    }
    for (; BurstCount > 0 && Index < DataLength; BurstCount--) {
      MmioWrite8 ((UINTN) &TisReg->DataFifo, *(TpmBuffer + Index));
      Index++;
    }
  }
  //
  // Ensure the Tpm status STS_EXPECT change from 1 to 0
  //
  Status = TisPcWaitRegisterBits (
             &TisReg->Status,
             (UINT8) TIS_PC_VALID,
             TIS_PC_STS_EXPECT,
             TIS_TIMEOUT_C
             );
  return Status;
}

/**
  Receive response data of last command from TPM.

  @param[in]  TisReg            TPM register space base address.  
  @param[out] TpmBuffer         Buffer for response data.  
  @param[out] RespSize          Response data length.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_DEVICE_ERROR      Unexpected device status.
  @retval EFI_BUFFER_TOO_SMALL  Response data is too long.

**/
EFI_STATUS
TisPcReceive (
  IN      TIS_PC_REGISTERS_PTR      TisReg,
     OUT  UINT8                     *TpmBuffer,
     OUT  UINT32                    *RespSize
  )
{
  EFI_STATUS                        Status;
  UINT16                            BurstCount;
  UINT32                            Index;
  UINT32                            ResponseSize;
  UINT32                            Data32;

  //
  // Wait for the command completion
  //
  Status = TisPcWaitRegisterBits (
             &TisReg->Status,
             (UINT8) (TIS_PC_VALID | TIS_PC_STS_DATA),
             0,
             TIS_TIMEOUT_B
             );
  if (EFI_ERROR (Status)) {
    return EFI_TIMEOUT;
  }
  //
  // Read the response data header and check it
  //
  Index = 0;
  BurstCount = 0;
  while (Index < sizeof (TPM_RSP_COMMAND_HDR)) {
    Status = TisPcReadBurstCount (TisReg, &BurstCount);
    if (EFI_ERROR (Status)) {
      return EFI_TIMEOUT;
    }
    for (; BurstCount > 0 ; BurstCount--) {
      *(TpmBuffer + Index) = MmioRead8 ((UINTN) &TisReg->DataFifo);
      Index++;
      if (Index == sizeof (TPM_RSP_COMMAND_HDR))
        break;
    }
  }
  //
  // Check the reponse data header (tag,parasize and returncode )
  //
  CopyMem (&Data32, (TpmBuffer + 2), sizeof (UINT32));
  ResponseSize = SwapBytes32 (Data32);
  *RespSize =  ResponseSize;
  if (ResponseSize == sizeof (TPM_RSP_COMMAND_HDR)) {
    return EFI_SUCCESS;
  }
  if (ResponseSize < sizeof (TPM_RSP_COMMAND_HDR)) {
    return EFI_DEVICE_ERROR;
  }
  if (ResponseSize > TPMCMDBUFLENGTH) {
    return EFI_BUFFER_TOO_SMALL;
  }
  //
  // Continue reading the remaining data
  //
  while (Index < ResponseSize) {
    for (; BurstCount > 0 ; BurstCount--) {
      *(TpmBuffer + Index) = MmioRead8 ((UINTN) &TisReg->DataFifo);
      Index++;
      if (Index == ResponseSize) {
        return EFI_SUCCESS;
      }
    }
    Status = TisPcReadBurstCount (TisReg, &BurstCount);
    if (EFI_ERROR (Status) && (Index < ResponseSize)) {
      return EFI_DEVICE_ERROR;
    }
  }
  return EFI_SUCCESS;
}

/**
  Format TPM command data according to the format control character.

  @param[in]      FmtChar       Format control character.  
  @param[in, out] ap            List of arguments.  
  @param[in]      TpmBuffer     Buffer for TPM command data.  
  @param[out]     DataLength    TPM command data length. 
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid format control character.
  @retval EFI_BUFFER_TOO_SMALL  Buffer too small for command data.

**/
EFI_STATUS
TisPcSendV (
  IN      UINT8                     FmtChar,
  IN OUT  VA_LIST                   *ap,
  UINT8                             *TpmBuffer,
  UINT32                            *DataLength
  )
{
  UINT8                             DataByte;
  UINT16                            DataWord;
  UINT32                            DataDword;
  TPM_RQU_COMMAND_HDR               TpmCmdHdr;
  TPM_RQU_COMMAND_HDR               *TpmCmdPtr;
  UINTN                             Size;
  UINT8                             *Raw;

  switch (FmtChar) {

    case 'b':
      DataByte  = VA_ARG (*ap, UINT8);
      Raw = &DataByte;
      Size = sizeof (DataByte);
      break;

    case 'w':
      DataWord  = VA_ARG (*ap, UINT16);
      DataWord  = SwapBytes16 (DataWord);
      Raw = (UINT8*)&DataWord;
      Size = sizeof (DataWord);
      break;

    case 'd':
      DataDword  = VA_ARG (*ap, UINT32);
      DataDword  = SwapBytes32 (DataDword);
      Raw = (UINT8*)&DataDword;
      Size = sizeof (DataDword);
      break;

    case 'h':
      TpmCmdPtr           = VA_ARG (*ap, TPM_RQU_COMMAND_HDR*);
      TpmCmdHdr.tag       = SwapBytes16 (TpmCmdPtr->tag);
      TpmCmdHdr.paramSize = SwapBytes32 (TpmCmdPtr->paramSize);
      TpmCmdHdr.ordinal   = SwapBytes32 (TpmCmdPtr->ordinal);
      Raw                 = (UINT8*) &TpmCmdHdr;
      Size                = sizeof (TpmCmdHdr);
      break;

    case 'r':
      Raw  = VA_ARG (*ap, UINT8*);
      Size = VA_ARG (*ap, UINTN);
      break;

    case '\0':
      return EFI_INVALID_PARAMETER;

    default:
      return EFI_INVALID_PARAMETER;
  }

  //
  // Check input to avoid overflow.
  //
  if ((UINT32) (~0)- *DataLength < (UINT32)Size) {
    return EFI_INVALID_PARAMETER;
  }

  if(*DataLength + (UINT32) Size > TPMCMDBUFLENGTH) {
    return EFI_BUFFER_TOO_SMALL;
  }
  CopyMem (TpmBuffer + *DataLength, Raw, Size);
  *DataLength += (UINT32) Size;
  return EFI_SUCCESS;
}

/**
  Format reponse data according to the format control character.

  @param[in]      FmtChar       Format control character.  
  @param[in, out] ap            List of arguments.  
  @param[out]     TpmBuffer     Buffer for reponse data.  
  @param[in, out] DataIndex     Data offset in reponse data buffer. 
  @param[in]      RespSize      Response data length.  
  @param[out]     DataFinished  Reach the end of Response data.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid format control character.
  @retval EFI_BUFFER_TOO_SMALL  Buffer too small for command data.

**/
EFI_STATUS
TisPcReceiveV (
  IN      UINT8                     FmtChar,
  IN OUT  VA_LIST                   *ap,
     OUT  UINT8                     *TpmBuffer,
  IN OUT  UINT32                    *DataIndex,
  IN      UINT32                    RespSize,
     OUT  BOOLEAN                   *DataFinished
  )
{
  UINT8                             *Raw;
  TPM_RSP_COMMAND_HDR               *TpmRspPtr;
  UINTN                             Size;

  Raw = VA_ARG (*ap, UINT8*);
  switch (FmtChar) {

    case 'b':
      Size = sizeof (UINT8);
      break;

    case 'w':
      Size = sizeof (UINT16);
      break;

    case 'd':
      Size = sizeof (UINT32);
      break;

    case 'h':
      Size = sizeof (*TpmRspPtr);
      break;

    case 'r':
      Size = VA_ARG (*ap, UINTN);
      //
      // If overflowed, which means Size is big enough for Response data. 
      // skip this check. Copy the whole data 
      //
      if ((UINT32) (~0)- *DataIndex >= (UINT32)Size) {
        if(*DataIndex + (UINT32) Size <= RespSize) {
          break;
        }
      }

      *DataFinished = TRUE;
      if (*DataIndex >= RespSize) {
        return EFI_SUCCESS;
      }
      CopyMem (Raw, TpmBuffer + *DataIndex, RespSize - *DataIndex);
      *DataIndex += RespSize - *DataIndex;
      return EFI_SUCCESS;

    case '\0':
      return EFI_INVALID_PARAMETER;

    default:
      return EFI_WARN_UNKNOWN_GLYPH;
  }

  if(*DataIndex + (UINT32) Size > RespSize) {
    *DataFinished = TRUE;
    return EFI_SUCCESS;
  }

  if( *DataIndex + (UINT32) Size > TPMCMDBUFLENGTH )
    return EFI_BUFFER_TOO_SMALL;

  CopyMem (Raw, TpmBuffer + *DataIndex, Size);
  *DataIndex += (UINT32) Size;

  switch (FmtChar) {

    case 'w':
      *(UINT16*)Raw = SwapBytes16 (*(UINT16*) Raw);
      break;

    case 'd':
      *(UINT32*)Raw = SwapBytes32 (*(UINT32*) Raw);
      break;

    case 'h':
      TpmRspPtr = (TPM_RSP_COMMAND_HDR*) Raw;
      TpmRspPtr->tag = SwapBytes16 (TpmRspPtr->tag);
      TpmRspPtr->paramSize = SwapBytes32 (TpmRspPtr->paramSize);
      TpmRspPtr->returnCode = SwapBytes32 (TpmRspPtr->returnCode);
      break;
  }
  return EFI_SUCCESS;
}

/**
  Send formatted command to TPM for execution and return formatted data from response.

  @param[in] TisReg    TPM Handle.  
  @param[in] Fmt       Format control string.  
  @param[in] ...       The variable argument list.
 
  @retval EFI_SUCCESS  Operation completed successfully.
  @retval EFI_TIMEOUT  The register can't run into the expected status in time.

**/
EFI_STATUS
EFIAPI
TisPcExecute (
  IN      TIS_TPM_HANDLE            TisReg,
  IN      CONST CHAR8               *Fmt,
  ...
  )
{
  EFI_STATUS                        Status;
  VA_LIST                           Ap;
  UINT32                            BufSize;
  UINT32                            ResponseSize;
  BOOLEAN                           DataFinished;

  VA_START (Ap, Fmt);

  //
  // Put the formatted command to the TpmCommandBuf
  //
  BufSize = 0;
  while (*Fmt != '\0') {
    if (*Fmt == '%') Fmt++;
    if (*Fmt == '/') break;
    Status = TisPcSendV (*Fmt, &Ap, TpmCommandBuf, &BufSize);
    if (EFI_ERROR( Status )) {
      goto Error;
    }
    Fmt++;
  }
  //
  // Send the command to TPM
  //
  Status = TisPcSend (TisReg, TpmCommandBuf, BufSize);
  if (EFI_ERROR (Status))  {
    //
    // Ensure the TPM state change from "Reception" to "Idle/Ready"
    //
    MmioWrite8 ((UINTN) &(((TIS_PC_REGISTERS_PTR) TisReg)->Status), TIS_PC_STS_READY);
    goto Error;
  }

  MmioWrite8 ((UINTN) &(((TIS_PC_REGISTERS_PTR) TisReg)->Status), TIS_PC_STS_GO);
  Fmt++;
  //
  // Receive the response data from TPM
  //
  ZeroMem (TpmCommandBuf, TPMCMDBUFLENGTH);
  Status = TisPcReceive (TisReg, TpmCommandBuf, &ResponseSize);
  //
  // Ensure the TPM state change from "Execution" or "Completion" to "Idle/Ready"
  //
  MmioWrite8 ((UINTN) &(((TIS_PC_REGISTERS_PTR) TisReg)->Status), TIS_PC_STS_READY);
  if (EFI_ERROR (Status)) {
    goto Error;
  }
  
  //
  // Get the formatted data from the TpmCommandBuf.
  //
  BufSize =0;
  DataFinished = FALSE;
  while (*Fmt != '\0') {
    if (*Fmt == '%') {
      Fmt++;
    }
    Status = TisPcReceiveV (*Fmt, &Ap, TpmCommandBuf, &BufSize, ResponseSize, &DataFinished);
    if (EFI_ERROR (Status)) {
      goto Error;
    }
    if (DataFinished) {
      VA_END (Ap);
      return EFI_SUCCESS;
    }
    Fmt++;
  }

Error:
  VA_END (Ap);
  return Status;
}

