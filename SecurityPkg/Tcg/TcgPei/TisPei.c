/** @file
  TIS (TPM Interface Specification) functions used by TPM PEI driver.
  
Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <IndustryStandard/Tpm12.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/TpmCommLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>

/**
  Send a command to TPM for execution and return response data.

  @param[in]      PeiServices   Describes the list of possible PEI Services.
  @param[in]      TisReg        TPM register space base address.  
  @param[in]      BufferIn      Buffer for command data.  
  @param[in]      SizeIn        Size of command data.  
  @param[in, out] BufferOut     Buffer for response data.  
  @param[in, out] SizeOut       Size of response data.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TisTpmCommand (
  IN     EFI_PEI_SERVICES           **PeiServices,
  IN     TIS_PC_REGISTERS_PTR       TisReg,
  IN     UINT8                      *BufferIn,
  IN     UINT32                     SizeIn,
  IN OUT UINT8                      *BufferOut,
  IN OUT UINT32                     *SizeOut
  )
{
  EFI_STATUS                        Status;
  UINT16                            BurstCount;
  UINT32                            Index;
  UINT32                            TpmOutSize;
  UINT16                            Data16;
  UINT32                            Data32;

  Status = TisPcPrepareCommand (TisReg);
  if (EFI_ERROR (Status)){
    DEBUG ((DEBUG_ERROR, "Tpm is not ready for command!\n"));
    return Status;
  }
  //
  // Send the command data to Tpm
  //
  Index = 0;
  while (Index < SizeIn) {
    Status = TisPcReadBurstCount (TisReg, &BurstCount);
    if (EFI_ERROR (Status)) {
      Status = EFI_TIMEOUT;
      goto Exit;
    }
    for (; BurstCount > 0 && Index < SizeIn; BurstCount--) {
      MmioWrite8((UINTN)&TisReg->DataFifo, *(BufferIn + Index));
      Index++;
    }
  }
  //
  // Check the Tpm status STS_EXPECT change from 1 to 0
  //
  Status = TisPcWaitRegisterBits (
             &TisReg->Status,
             (UINT8) TIS_PC_VALID,
             TIS_PC_STS_EXPECT,
             TIS_TIMEOUT_C
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "The send buffer too small!\n"));
    Status = EFI_BUFFER_TOO_SMALL;
    goto Exit;
  }
  //
  // Executed the TPM command and waiting for the response data ready
  //
  MmioWrite8((UINTN)&TisReg->Status, TIS_PC_STS_GO);
  Status = TisPcWaitRegisterBits (
             &TisReg->Status,
             (UINT8) (TIS_PC_VALID | TIS_PC_STS_DATA),
             0,
             TIS_TIMEOUT_B
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Wait for Tpm response data time out!!\n"));
    Status = EFI_TIMEOUT;
    goto Exit;
  }
  //
  // Get response data header
  //
  Index = 0;
  BurstCount = 0;
  while (Index < sizeof (TPM_RSP_COMMAND_HDR)) {
    Status = TisPcReadBurstCount (TisReg, &BurstCount);
    if (EFI_ERROR (Status)) {
      Status = EFI_TIMEOUT;
      goto Exit;
    }
    for (; BurstCount > 0; BurstCount--) {
      *(BufferOut + Index) = MmioRead8 ((UINTN)&TisReg->DataFifo);
      Index++;
      if (Index == sizeof (TPM_RSP_COMMAND_HDR)) break;
    }
  }
  //
  // Check the reponse data header (tag,parasize and returncode )
  //
  CopyMem (&Data16, BufferOut, sizeof (UINT16));
  if (SwapBytes16 (Data16) != TPM_TAG_RSP_COMMAND ) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }
  
  CopyMem (&Data32, (BufferOut + 2), sizeof (UINT32));
  TpmOutSize  = SwapBytes32 (Data32);
  if (*SizeOut < TpmOutSize) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto Exit;
  }
  *SizeOut = TpmOutSize;
  //
  // Continue reading the remaining data
  //
  while ( Index < TpmOutSize ) {
    for (; BurstCount > 0; BurstCount--) {
      *(BufferOut + Index) = MmioRead8 ((UINTN)&TisReg->DataFifo);
      Index++;
      if (Index == TpmOutSize) {
        Status = EFI_SUCCESS;
        goto Exit;
      }
    }
    Status = TisPcReadBurstCount (TisReg, &BurstCount);
    if (EFI_ERROR (Status)) {
      Status = EFI_TIMEOUT;
      goto Exit;
    }
  }
Exit:
  MmioWrite8((UINTN)&TisReg->Status, TIS_PC_STS_READY);
  return Status;
}

