/** @file
  Arm CCA Boot Sync Dxe.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/VariableFormat.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/ArmCcaBootSyncCryptoLib.h>
#include <Library/ArmCcaLib.h>
#include "Include/BootSyncBsbMsg.h"
#include "Include/BootSyncBsbParser.h"
#include "Include/BootSyncDebug.h"
#include "Include/BootSyncSecureChannel.h"
#include "Include/BootSyncProtocol.h"

#include "Guest/BootSyncProtocolGuest.h"

/**
  Validate the UEFI variable storage with data received from Boot Sync.

  @param[in]  VarStoreData        Pointer to the UEFI Variable store data.
  @param[in]  VarStoreDataLen     Length of the UEFI Variable store data.

  @retval EFI_SUCCESS             Success.
  @retval EFI_ABORTED             The validation failed.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
ValidateVariableStoreData (
  IN UINT8   *VarStoreData,
  IN UINT32  VarStoreDataLen
  )
{
  VARIABLE_STORE_HEADER  *VariableStore;

  if ((VarStoreData == NULL) ||
      (VarStoreDataLen < sizeof (VARIABLE_STORE_HEADER)))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  if (PcdGet32 (PcdVariableStoreSize) < VarStoreDataLen) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  VariableStore = (VARIABLE_STORE_HEADER *)VarStoreData;

  if ((VariableStore->Size < sizeof (VARIABLE_STORE_HEADER)) ||
      (VariableStore->Size < VarStoreDataLen) ||
      (VariableStore->Size > PcdGet32 (PcdVariableStoreSize)))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  if ((CompareGuid (&VariableStore->Signature, &gEfiAuthenticatedVariableGuid) ||
       CompareGuid (&VariableStore->Signature, &gEfiVariableGuid)) &&
      (VariableStore->Format == VARIABLE_STORE_FORMATTED) &&
      (VariableStore->State == VARIABLE_STORE_HEALTHY))
  {
    DEBUG ((
      DEBUG_INFO,
      "Variable Store reserved at %p appears to be valid\n",
      VariableStore
      ));
    return EFI_SUCCESS;
  }

  return EFI_ABORTED;
}

/**
  Initialise the UEFI variable storage with data received from Boot Sync.

  @param[in]  VarStoreData        Pointer to the UEFI Variable store data.
  @param[in]  VarStoreDataLen     Length of the UEFI Variable store data.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_PROTOCOL_ERROR      A protocol error occured.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
InitVariableStorage (
  IN UINT8   *VarStoreData,
  IN UINT32  VarStoreDataLen
  )
{
  EFI_STATUS  Status;
  UINT8       *VariableStore;

  Status = ValidateVariableStoreData (VarStoreData, VarStoreDataLen);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Variable Storage data validation failed!, Status = %r\n",
      Status
      ));
    ASSERT (0);
    return Status;
  }

  Status = PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to set PcdEmuVariableNvModeEnable, Status = %r\n",
      Status
      ));
    ASSERT (0);
    return Status;
  }

  VariableStore = (UINT8 *)PcdGet64 (PcdEmuVariableNvStoreReserved);
  if (VariableStore == NULL) {
    VariableStore = AllocateAlignedRuntimePages (
                      EFI_SIZE_TO_PAGES (PcdGet32 (PcdVariableStoreSize)),
                      EFI_PAGE_SIZE
                      );
    if (VariableStore == NULL) {
      ASSERT (0);
      return EFI_OUT_OF_RESOURCES;
    }

    Status = PcdSet64S (PcdEmuVariableNvStoreReserved, (UINT64)VariableStore);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to set PcdEmuVariableNvStoreReserved, Status = %r\n",
        Status
        ));
      ASSERT (0);
      return Status;
    }
  }

  // Copy the Variable Date to the Variable Store.
  CopyMem (VariableStore, VarStoreData, VarStoreDataLen);

  // The driver implementing the variable service can now be dispatched.
  Status = gBS->InstallProtocolInterface (
                  &gImageHandle,
                  &gEdkiiNvVarStoreFormattedGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Perform Arm CCA Boot Sync.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_PROTOCOL_ERROR      A protocol error occured.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
PerformSync (
  VOID
  )
{
  EFI_STATUS             Status;
  EFI_STATUS             Status1;
  RETURN_STATUS          RetStatus;
  SECURE_CHANNEL         SecChannel;
  BOOT_SYNC_BSB_HEADER   *BibHeader;
  BOOT_SYNC_BSB_ELEMENT  *BsbElement;

  ZeroMem (&SecChannel, sizeof (SECURE_CHANNEL));
  Status = EstablishSecureChannel (&SecChannel);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to establish Secure Session, Status = %r\n",
      Status
      ));
    return Status;
  }

  DBG_DUMP_KEYS ("Sec", &SecChannel);

  Status = BootSyncPerformAttestation (&SecChannel);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Attestation Failed!, Status = %r\n", Status));
    goto exit_handler;
  }

  DEBUG ((DEBUG_INFO, "Attestation Status = %r\n", Status));

  Status = BootSyncGetBib (&SecChannel, &BibHeader);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Boot Sync Failed!, Status = %r\n", Status));
    goto exit_handler;
  }

  DEBUG ((DEBUG_INFO, "Boot Sync Status = %r\n", Status));

  // Extend the rolling hash to REM3 to indicate that the Boot Sync Data
  // was received by the firmware.
  RetStatus = ArmCcaRsiExtendMeasurement (
                4,
                SecChannel.RmHash,
                sizeof (SecChannel.RmHash)
                );
  if (RETURN_ERROR (RetStatus)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to extend hash to REM3!, RetStatus = %r\n",
      RetStatus
      ));
    Status = EFI_ABORTED;
    goto exit_handler1;
  }

  // Retrive the BIB elements.
  Status = BsbGetElement (
             BibHeader,
             &gArmBootSyncVarData,
             &BsbElement
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to retrieve Variable Store Data!\n",
      Status
      ));
    goto exit_handler1;
  }

  DBG_DUMP_RAW (
    "Variable Store Data",
    (UINT8 *)(BsbElement + 1),
    (BsbElement->Header.Length - sizeof (BOOT_SYNC_BSB_ELEMENT))
    );

  Status = InitVariableStorage (
             (UINT8 *)(BsbElement + 1),
             (BsbElement->Header.Length - sizeof (BOOT_SYNC_BSB_ELEMENT))
             );
  ASSERT_EFI_ERROR (Status);

  // Send the FIN message to indicate End of Transmission.
  Status = SendFin (&SecChannel, BOOT_SYNC_COMM_END_REASON_EOT);
  ASSERT_EFI_ERROR (Status);

exit_handler1:
  // Scrub the BIB
  ZeroMem (BibHeader, BibHeader->Header.Length);
  // Free the BIB
  FreePool (BibHeader);
exit_handler:
  Status1 = TerminateSecureChannel (&SecChannel);
  if (EFI_ERROR (Status1)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to Close Secure Session Status = %r\n",
      Status1
      ));
  }

  // Return the first error otherwise return Status1.
  if (!EFI_ERROR (Status)) {
    Status = Status1;
  }

  return Status;
}

/**
  Entrypoint of Arm CCA Boot Sync Dxe.

  @param[in]  ImageHandle   Handle to the Image.
  @param[in]  SystemTable   Pointer to the system table.

  @retval EFI_SUCCESS           Success.
  @retval EFI_UNSUPPORTED       Unsupported.
  @retval EFI_ABORTED           An operation was aborted.
**/
EFI_STATUS
EFIAPI
ArmCcaBootSyncDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (!ArmCcaIsRealm ()) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "Boot Sync Dxe.\n"));

  Status = ArmCcaBootSyncCryptoInit ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to init Crypto interfaces, Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = PerformSync ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to perform Boot Sync, Status = %r\n",
      Status
      ));
    CpuDeadLoop ();
  }

  return Status;
}
