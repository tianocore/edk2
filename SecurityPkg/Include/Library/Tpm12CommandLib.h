/** @file
  This library is used by other modules to send TPM12 command.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TPM12_COMMAND_LIB_H_
#define _TPM12_COMMAND_LIB_H_

#include <IndustryStandard/Tpm12.h>

/**
  Send Startup command to TPM1.2.

  @param TpmSt           Startup Type.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12Startup (
  IN TPM_STARTUP_TYPE          TpmSt
  );

/**
  Send SaveState command to TPM1.2.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12SaveState (
  VOID
  );

/**
  Send ForceClear command to TPM1.2.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12ForceClear (
  VOID
  );

#pragma pack(1)

typedef struct {
  UINT16                            sizeOfSelect;
  UINT8                             pcrSelect[3];
} TPM12_PCR_SELECTION;

typedef struct {
  TPM12_PCR_SELECTION               pcrSelection;
  TPM_LOCALITY_SELECTION            localityAtRelease;
  TPM_COMPOSITE_HASH                digestAtRelease;
} TPM12_PCR_INFO_SHORT;

typedef struct {
  TPM_STRUCTURE_TAG               tag;
  TPM_NV_INDEX                    nvIndex;
  TPM12_PCR_INFO_SHORT            pcrInfoRead;
  TPM12_PCR_INFO_SHORT            pcrInfoWrite;
  TPM_NV_ATTRIBUTES               permission;
  BOOLEAN                         bReadSTClear;
  BOOLEAN                         bWriteSTClear;
  BOOLEAN                         bWriteDefine;
  UINT32                          dataSize;
} TPM12_NV_DATA_PUBLIC;

#pragma pack()

/**
  Send NV DefineSpace command to TPM1.2.

  @param PubInfo           The public parameters of the NV area.
  @param EncAuth           The encrypted AuthData, only valid if the attributes require subsequent authorization.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12NvDefineSpace (
  IN TPM12_NV_DATA_PUBLIC  *PubInfo,
  IN TPM_ENCAUTH           *EncAuth
  );

/**
  Send NV ReadValue command to TPM1.2.

  @param NvIndex           The index of the area to set.
  @param Offset            The offset into the area.
  @param DataSize          The size of the data area.
  @param Data              The data to set the area to.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12NvReadValue (
  IN TPM_NV_INDEX   NvIndex,
  IN UINT32         Offset,
  IN OUT UINT32     *DataSize,
  OUT UINT8         *Data
  );

/**
  Send NV WriteValue command to TPM1.2.

  @param NvIndex           The index of the area to set.
  @param Offset            The offset into the NV Area.
  @param DataSize          The size of the data parameter.
  @param Data              The data to set the area to.

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm12NvWriteValue (
  IN TPM_NV_INDEX   NvIndex,
  IN UINT32         Offset,
  IN UINT32         DataSize,
  IN UINT8          *Data
  );

/**
Extend a TPM PCR.

@param[in]  DigestToExtend    The 160 bit value representing the event to be recorded.
@param[in]  PcrIndex          The PCR to be updated.
@param[out] NewPcrValue       New PCR value after extend.

@retval EFI_SUCCESS           Operation completed successfully.
@retval EFI_TIMEOUT           The register can't run into the expected status in time.
@retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
@retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
EFIAPI
Tpm12Extend (
  IN  TPM_DIGEST    *DigestToExtend,
  IN  TPM_PCRINDEX  PcrIndex,
  OUT TPM_DIGEST    *NewPcrValue
  );

/**
Send TSC_PhysicalPresence command to TPM.

@param[in] PhysicalPresence   The state to set the TPMs Physical Presence flags.

@retval EFI_SUCCESS           Operation completed successfully.
@retval EFI_TIMEOUT           The register can't run into the expected status in time.
@retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
@retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
EFIAPI
Tpm12PhysicalPresence (
  IN      TPM_PHYSICAL_PRESENCE     PhysicalPresence
  );

/**
Send TPM_ContinueSelfTest command to TPM.

@retval EFI_SUCCESS           Operation completed successfully.
@retval EFI_TIMEOUT           The register can't run into the expected status in time.
@retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
@retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
EFIAPI
Tpm12ContinueSelfTest (
  VOID
  );

/**
Get TPM capability permanent flags.

@param[out] TpmPermanentFlags   Pointer to the buffer for returned flag structure.

@retval EFI_SUCCESS           Operation completed successfully.
@retval EFI_TIMEOUT           The register can't run into the expected status in time.
@retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
@retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
EFIAPI
Tpm12GetCapabilityFlagPermanent (
  OUT TPM_PERMANENT_FLAGS  *TpmPermanentFlags
  );

/**
Get TPM capability volatile flags.

@param[out] VolatileFlags   Pointer to the buffer for returned flag structure.

@retval EFI_SUCCESS      Operation completed successfully.
@retval EFI_DEVICE_ERROR The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
Tpm12GetCapabilityFlagVolatile (
  OUT TPM_STCLEAR_FLAGS                 *VolatileFlags
  );
#endif
