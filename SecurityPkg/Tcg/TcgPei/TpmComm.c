/** @file
  Utility functions used by TPM PEI driver.
  
Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "TpmComm.h"

/**
  Send a command to TPM for execution and return response data.

  @param[in]      PeiServices   Describes the list of possible PEI Services.
  @param[in]      TisReg        TPM register space base address.  
  @param[in]      BufferIn      Buffer for command data.  
  @param[in]      SizeIn        Size of command data.  
  @param[in, out] BufferOut     Buffer for response data.  
  @param[in, out] SizeOut       size of response data.  
 
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
  );

/**
  Send TPM_Startup command to TPM.

  @param[in] PeiServices        Describes the list of possible PEI Services.
  @param[in] TpmHandle          TPM handle.  
  @param[in] BootMode           Boot mode.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TpmCommStartup (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      TIS_TPM_HANDLE            TpmHandle,
  IN      EFI_BOOT_MODE             BootMode
  )
{
  EFI_STATUS                        Status;
  TPM_STARTUP_TYPE                  TpmSt;
  UINT32                            TpmRecvSize;
  UINT32                            TpmSendSize;
  TPM_CMD_START_UP                  SendBuffer;
  UINT8                             RecvBuffer[20];

  TpmSt = TPM_ST_CLEAR;
  if (BootMode == BOOT_ON_S3_RESUME) {
    TpmSt = TPM_ST_STATE;
  }
  //
  // send Tpm command TPM_ORD_Startup
  //
  TpmRecvSize               = 20;
  TpmSendSize               = sizeof (TPM_CMD_START_UP);
  SendBuffer.Hdr.tag        = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize  = SwapBytes32 (TpmSendSize);
  SendBuffer.Hdr.ordinal    = SwapBytes32 (TPM_ORD_Startup);
  SendBuffer.TpmSt          = SwapBytes16 (TpmSt);
  Status = TisTpmCommand (PeiServices, TpmHandle, (UINT8 *)&SendBuffer, TpmSendSize, RecvBuffer, &TpmRecvSize);
  return Status;
}

/**
  Send TPM_ContinueSelfTest command to TPM.

  @param[in] PeiServices        Describes the list of possible PEI Services.
  @param[in] TpmHandle          TPM handle.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TpmCommContinueSelfTest (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      TIS_TPM_HANDLE            TpmHandle
  )
{
  EFI_STATUS                        Status;
  UINT32                            TpmRecvSize;
  UINT32                            TpmSendSize;
  TPM_CMD_SELF_TEST                 SendBuffer;
  UINT8                             RecvBuffer[20];

  //
  // send Tpm command TPM_ORD_ContinueSelfTest
  //
  TpmRecvSize               = 20;
  TpmSendSize               = sizeof (TPM_CMD_SELF_TEST);
  SendBuffer.Hdr.tag        = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize  = SwapBytes32 (TpmSendSize);  
  SendBuffer.Hdr.ordinal    = SwapBytes32 (TPM_ORD_ContinueSelfTest);
  Status = TisTpmCommand (PeiServices, TpmHandle, (UINT8 *)&SendBuffer, TpmSendSize, RecvBuffer, &TpmRecvSize);
  return Status;
}

/**
  Get TPM capability flags.

  @param[in]  PeiServices       Describes the list of possible PEI Services.
  @param[in]  TpmHandle         TPM handle.  
  @param[out] Deactivated       Returns deactivated flag.
  @param[out] LifetimeLock      Returns physicalPresenceLifetimeLock permanent flag.  
  @param[out] CmdEnable         Returns physicalPresenceCMDEnable permanent flag.
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TpmCommGetCapability (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      TIS_TPM_HANDLE            TpmHandle,
     OUT  BOOLEAN                   *Deactivated, OPTIONAL
     OUT  BOOLEAN                   *LifetimeLock, OPTIONAL
     OUT  BOOLEAN                   *CmdEnable OPTIONAL
  )
{
  EFI_STATUS                        Status;
  UINT32                            TpmRecvSize;
  UINT32                            TpmSendSize;
  TPM_CMD_GET_CAPABILITY            SendBuffer;
  UINT8                             RecvBuffer[40];
  TPM_PERMANENT_FLAGS               *TpmPermanentFlags;

  //
  // send Tpm command TPM_ORD_GetCapability
  //
  TpmRecvSize                   = 40;
  TpmSendSize                   = sizeof (TPM_CMD_GET_CAPABILITY);
  SendBuffer.Hdr.tag            = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize      = SwapBytes32 (TpmSendSize);  
  SendBuffer.Hdr.ordinal        = SwapBytes32 (TPM_ORD_GetCapability);
  SendBuffer.Capability         = SwapBytes32 (TPM_CAP_FLAG);
  SendBuffer.CapabilityFlagSize = SwapBytes32 (sizeof (TPM_CAP_FLAG_PERMANENT));
  SendBuffer.CapabilityFlag     = SwapBytes32 (TPM_CAP_FLAG_PERMANENT);
  Status = TisTpmCommand (PeiServices, TpmHandle, (UINT8 *)&SendBuffer, TpmSendSize, RecvBuffer, &TpmRecvSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  TpmPermanentFlags = (TPM_PERMANENT_FLAGS *)&RecvBuffer[sizeof (TPM_RSP_COMMAND_HDR) + sizeof (UINT32)];
  if (Deactivated != NULL) {
    *Deactivated      = TpmPermanentFlags->deactivated;
  }

  if (LifetimeLock != NULL) {
    *LifetimeLock = TpmPermanentFlags->physicalPresenceLifetimeLock;
  }

  if (CmdEnable != NULL) {
    *CmdEnable = TpmPermanentFlags->physicalPresenceCMDEnable;
  }
  return Status;
}

/**
  Extend a TPM PCR.

  @param[in]  PeiServices       Describes the list of possible PEI Services.
  @param[in]  TpmHandle         TPM handle.  
  @param[in]  DigestToExtend    The 160 bit value representing the event to be recorded.  
  @param[in]  PcrIndex          The PCR to be updated.
  @param[out] NewPcrValue       New PCR value after extend.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TpmCommExtend (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      TIS_TPM_HANDLE            TpmHandle,
  IN      TPM_DIGEST                *DigestToExtend,
  IN      TPM_PCRINDEX              PcrIndex,
     OUT  TPM_DIGEST                *NewPcrValue
  )
{
  EFI_STATUS                        Status;
  UINT32                            TpmSendSize;
  UINT32                            TpmRecvSize;
  TPM_CMD_EXTEND                    SendBuffer;
  UINT8                             RecvBuffer[10 + sizeof(TPM_DIGEST)];

  //
  // send Tpm command TPM_ORD_Extend
  //
  TpmRecvSize               = sizeof (TPM_RSP_COMMAND_HDR) + sizeof (TPM_DIGEST);
  TpmSendSize               = sizeof (TPM_CMD_EXTEND);
  SendBuffer.Hdr.tag        = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize  = SwapBytes32 (TpmSendSize);
  SendBuffer.Hdr.ordinal    = SwapBytes32 (TPM_ORD_Extend);
  SendBuffer.PcrIndex       = SwapBytes32 (PcrIndex);
  CopyMem (&SendBuffer.TpmDigest, (UINT8 *)DigestToExtend, sizeof (TPM_DIGEST));
  Status = TisTpmCommand (PeiServices, TpmHandle, (UINT8 *)&SendBuffer, TpmSendSize, RecvBuffer, &TpmRecvSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if(NewPcrValue != NULL) {
    CopyMem ((UINT8*)NewPcrValue, &RecvBuffer[10], sizeof (TPM_DIGEST));
  }

  return Status;
}


/**
  Send TSC_PhysicalPresence command to TPM.

  @param[in] PeiServices        Describes the list of possible PEI Services.
  @param[in] TpmHandle          TPM handle.  
  @param[in] PhysicalPresence   The state to set the TPMs Physical Presence flags.  
 
  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
TpmCommPhysicalPresence (
  IN      EFI_PEI_SERVICES          **PeiServices,
  IN      TIS_TPM_HANDLE            TpmHandle,
  IN      TPM_PHYSICAL_PRESENCE     PhysicalPresence
  )
{
  EFI_STATUS                        Status;
  UINT32                            TpmSendSize;
  UINT32                            TpmRecvSize;
  TPM_CMD_PHYSICAL_PRESENCE         SendBuffer;
  UINT8                             RecvBuffer[10];

  //
  // send Tpm command TSC_ORD_PhysicalPresence
  //
  TpmRecvSize                 = 10;
  TpmSendSize                 = sizeof (TPM_CMD_PHYSICAL_PRESENCE);
  SendBuffer.Hdr.tag          = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  SendBuffer.Hdr.paramSize    = SwapBytes32 (TpmSendSize);
  SendBuffer.Hdr.ordinal      = SwapBytes32 (TSC_ORD_PhysicalPresence);
  SendBuffer.PhysicalPresence = SwapBytes16 (PhysicalPresence);
  Status = TisTpmCommand (PeiServices, TpmHandle, (UINT8 *)&SendBuffer, TpmSendSize, RecvBuffer, &TpmRecvSize);
  return Status;
}
