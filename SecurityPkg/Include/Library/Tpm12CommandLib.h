/** @file
  This library is used by other modules to send TPM12 command.

Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

#endif
