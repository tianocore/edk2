/** @file
  Set TPM device type

  This module initializes the TPM device type based on a CRB over FF-A
  interface

  Copyright (C) 2025, Arm Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - https://developer.arm.com/documentation/den0138/latest/
**/

#include <PiPei.h>

#include <Guid/TpmInstance.h>
#include <Guid/Tpm2ServiceFfa.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/ArmFfaLib.h>

#define TPM2_FFA_SERVICE_MAJOR_VERSION  (1)
#define TPM2_FFA_SERVICE_MINOR_VERSION  (0)

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mTpmSelectedPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiTpmDeviceSelectedGuid,
  NULL
};

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mTpmInitializationDonePpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiTpmInitializationDonePpiGuid,
  NULL
};

/**
  Check the Tpm Service Interface version.

  See the CRB over FF-A spec 6.1.
  all of arguments' value based on the specification.

  @param [in]  TpmPartId        TPM service partition id.

  @retval EFI_SUCCESS
  @retval EFI_DEVICE_ERROR      Failed to communicate TPM SP.
  @retval EFI_UNSUPPORTED       Unsupported version of TPM service

**/
STATIC
EFI_STATUS
EFIAPI
Tpm2FfaCheckInterfaceVersion (
  IN  UINT16  TpmPartId
  )
{
  EFI_STATUS       Status;
  DIRECT_MSG_ARGS  TpmArgs;
  UINT16           MajorVer;
  UINT16           MinorVer;

  ZeroMem (&TpmArgs, sizeof (DIRECT_MSG_ARGS));
  TpmArgs.Arg0 = TPM2_FFA_GET_INTERFACE_VERSION;

  Status = ArmFfaLibMsgSendDirectReq2 (TpmPartId, &gTpm2ServiceFfaGuid, &TpmArgs);
  while (Status == EFI_INTERRUPT_PENDING) {
    // We are assuming vCPU0 of the TPM SP since it is UP.
    Status = ArmFfaLibRun (TpmPartId, 0x00);
  }

  if (EFI_ERROR (Status) || (TpmArgs.Arg0 != TPM2_FFA_SUCCESS_OK_RESULTS_RETURNED)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get interface version... Status: %r, TpmArgs.Arg0: 0x%x\n",
      __func__,
      Status,
      TpmArgs.Arg0
      ));

    return EFI_DEVICE_ERROR;
  }

  MajorVer = (TpmArgs.Arg1 & TPM2_FFA_SERVICE_MAJOR_VER_MASK) >> TPM2_FFA_SERVICE_MAJOR_VER_SHIFT;
  MinorVer = (TpmArgs.Arg1 & TPM2_FFA_SERVICE_MINOR_VER_MASK) >> TPM2_FFA_SERVICE_MINOR_VER_SHIFT;
  if ((MajorVer != TPM2_FFA_SERVICE_MAJOR_VERSION) || (MinorVer < TPM2_FFA_SERVICE_MINOR_VERSION)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Wrong Tpm Ffa Interface Version...: v%d.%d\n",
      __func__,
      MajorVer,
      MinorVer
      ));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  The entry point for Tcg2 configuration driver.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.
**/
EFI_STATUS
EFIAPI
Tcg2ConfigFfaPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                    Status;
  UINTN                         Size;
  UINT16                        PartId;
  UINT16                        TpmPartId;
  EFI_FFA_PART_INFO_DESC        *TpmPartInfo;
  VOID                          *TxBuffer;
  UINT64                        TxBufferSize;
  VOID                          *RxBuffer;
  UINT64                        RxBufferSize;
  UINT32                        Count;
  CONST EFI_PEI_PPI_DESCRIPTOR  *PpiList;
  GUID                          *TpmInstanceGuid;

  DEBUG ((DEBUG_INFO, "%a\n", __func__));

  TpmInstanceGuid = &gEfiTpmDeviceInstanceNoneGuid;
  PpiList         = &mTpmInitializationDonePpiList;

  Status = ArmFfaLibPartitionIdGet (&PartId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get partition-id. Status: %r\n", Status));
    goto Cleanup;
  }

  Status = ArmFfaLibGetRxTxBuffers (
             &TxBuffer,
             &TxBufferSize,
             &RxBuffer,
             &RxBufferSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get Rx/Tx Buffer. Status: %r\n", Status));
    goto Cleanup;
  }

  Status = ArmFfaLibPartitionInfoGet (
             &gTpm2ServiceFfaGuid,
             FFA_PART_INFO_FLAG_TYPE_DESC,
             &Count,
             (UINT32 *)&Size
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get Tpm2 partition info. Status: %r\n", Status));
    goto Cleanup;
  }

  if ((Count != 1) || (Size < sizeof (EFI_FFA_PART_INFO_DESC))) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((DEBUG_ERROR, "Invalid partition Info(%g). Count: %d, Size: %d\n", &gTpm2ServiceFfaGuid, Count, Size));
    ArmFfaLibRxRelease (PartId);
    goto Cleanup;
  }

  TpmPartInfo = (EFI_FFA_PART_INFO_DESC *)RxBuffer;
  TpmPartId   = TpmPartInfo->PartitionId;
  ArmFfaLibRxRelease (PartId);

  Status = Tpm2FfaCheckInterfaceVersion (TpmPartId);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  /*
   * Set the PcdTpmInstanceGuid to TPM using CRB over FF-A
   * So that Tpm2DeviceLibRouterPei/Dxe can communicate with
   * TPM secure partition.
   */
  TpmInstanceGuid = &gTpm2ServiceFfaGuid;
  PpiList         = &mTpmSelectedPpi;

Cleanup:
  Size   = sizeof (GUID);
  Status = PcdSetPtrS (
             PcdTpmInstanceGuid,
             &Size,
             TpmInstanceGuid
             );
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesInstallPpi (PpiList);

  return Status;
}
