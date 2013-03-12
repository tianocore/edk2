/** @file
  The header file for TPM PEI driver.
  
Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TPM_COMM_H_
#define _TPM_COMM_H_

#include <IndustryStandard/Tpm12.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/TpmCommLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#pragma pack(1)

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
  TPM_STARTUP_TYPE      TpmSt;
} TPM_CMD_START_UP;

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
} TPM_CMD_SELF_TEST;

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
  UINT32                Capability;
  UINT32                CapabilityFlagSize;
  UINT32                CapabilityFlag;
} TPM_CMD_GET_CAPABILITY;

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
  TPM_PCRINDEX          PcrIndex;
  TPM_DIGEST            TpmDigest;
} TPM_CMD_EXTEND;

typedef struct {
  TPM_RQU_COMMAND_HDR   Hdr;
  TPM_PHYSICAL_PRESENCE PhysicalPresence;
} TPM_CMD_PHYSICAL_PRESENCE;

#pragma pack()

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
  );

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
  );

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
  );

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
  );


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
  );

#endif  // _TPM_COMM_H_
