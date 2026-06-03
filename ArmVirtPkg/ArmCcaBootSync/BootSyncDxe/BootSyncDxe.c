/** @file
  Arm CCA Boot Sync Dxe.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/ArmCcaSecretLocation.h>
#include <Guid/CocoSecret.h>
#include <Guid/ConfidentialComputingSecret.h>
#include <Guid/VariableFormat.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
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

/** Add the Secret Memory address range to the memory map.

  @param [in]  ImageHandle     The handle to the image.
  @param [in]  SecretLocation  The Secret memory location info.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
**/
STATIC
EFI_STATUS
MapSecretMemory (
  IN EFI_HANDLE               ImageHandle,
  IN ARM_CCA_SECRET_LOCATION  *SecretLocation
  )
{
  EFI_STATUS  Status;

  if (SecretLocation == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeReserved,
                  SecretLocation->Base,
                  SecretLocation->Size,
                  EFI_MEMORY_WB | EFI_MEMORY_XP
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to add memory space. Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = gDS->AllocateMemorySpace (
                  EfiGcdAllocateAddress,
                  EfiGcdMemoryTypeReserved,
                  0,
                  SecretLocation->Size,
                  &SecretLocation->Base,
                  ImageHandle,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to allocate memory space. Status = %r\n",
      Status
      ));
    gDS->RemoveMemorySpace (
           SecretLocation->Base,
           SecretLocation->Size
           );
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  SecretLocation->Base,
                  SecretLocation->Size,
                  EFI_MEMORY_WB | EFI_MEMORY_XP
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to set memory attributes. Status = %r\n",
      Status
      ));
    gDS->FreeMemorySpace (
           SecretLocation->Base,
           SecretLocation->Size
           );
    gDS->RemoveMemorySpace (
           SecretLocation->Base,
           SecretLocation->Size
           );
  }

  DEBUG ((
    DEBUG_INFO,
    "Secret memory space mapping Status = %r\n",
    Status
    ));
  return Status;
}

/**
  Initialise the EFI COCO Secret System Table with the secret data,
  i.e. the disk decryption key received from Boot Sync.

  @param[in]  ImageHandle       Handle to the Image.
  @param[in]  SecretData        Pointer to the secret data.
  @param[in]  SecretDataLen     Length of the secret data.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
InitialiseSecretTable (
  IN EFI_HANDLE  ImageHandle,
  IN UINT8       *SecretData,
  IN UINT32      SecretDataLen
  )
{
  EFI_STATUS          Status;
  RETURN_STATUS       RetStatus;
  UINT8               *Secret;
  UINT8               *SecretTableData;
  UINTN               SecretTableDataLength;
  UINTN               SecretTablePages;
  COCO_SECRET_HEADER  *SecretHeader;
  COCO_SECRET_ENTRY   *SecretEntry;

  CONFIDENTIAL_COMPUTING_SECRET_LOCATION  *SecretTable;
  ARM_CCA_SECRET_LOCATION                 *SecretLocation;
  VOID                                    *Hob;

  if ((SecretData == NULL) || (SecretDataLen == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->AllocatePool (
                  EfiACPIReclaimMemory,
                  sizeof (CONFIDENTIAL_COMPUTING_SECRET_LOCATION),
                  (VOID **)&SecretTable
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // When creating the password file do "echo -n root > password.dat"
  // Add 1 for null terminating the password string
  SecretTableDataLength = sizeof (COCO_SECRET_HEADER) +
                          sizeof (COCO_SECRET_ENTRY) + SecretDataLen + 1;
  SecretTablePages = EFI_SIZE_TO_PAGES (SecretTableDataLength);

  // Get the Arm CCA Secret Location information.
  Hob = GetFirstGuidHob (&gArmBootSyncSecretMemoryLocationGuid);
  if (Hob == NULL) {
    ASSERT (0);
    Status = EFI_NOT_FOUND;
    goto ExitHandler;
  }

  SecretLocation = (ARM_CCA_SECRET_LOCATION *)GET_GUID_HOB_DATA (Hob);

  if ((SecretLocation == NULL) ||
      (SecretLocation->Base == 0) ||
      (SecretLocation->Size < EFI_PAGES_TO_SIZE (SecretTablePages)))
  {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto ExitHandler;
  }

  DEBUG ((
    DEBUG_INFO,
    "SecretLocation.Base = 0x%llx\n",
    SecretLocation->Base
    ));
  DEBUG ((
    DEBUG_INFO,
    "SecretLocation.Size = 0x%llx\n",
    SecretLocation->Size
    ));

  // Ensure that the secret data region is protected RAM.
  RetStatus = ArmCcaRsiSetIpaState (
                (UINT64 *)SecretLocation->Base,
                SecretLocation->Size,
                RipasRam,
                ARM_CCA_RIPAS_CHANGE_FLAGS_RSI_NO_CHANGE_DESTROYED
                );
  if (RETURN_ERROR (RetStatus)) {
    ASSERT (0);
    Status = EFI_ABORTED;
    goto ExitHandler;
  }

  Status = MapSecretMemory (ImageHandle, SecretLocation);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto ExitHandler;
  }

  SecretTableData = (UINT8 *)SecretLocation->Base;

  DEBUG ((
    DEBUG_INFO,
    "SecretTableData = 0x%llx\n",
    SecretTableData
    ));

  DEBUG ((
    DEBUG_INFO,
    "SecretTableDataLength = 0x%llx\n",
    SecretTableDataLength
    ));

  DEBUG ((
    DEBUG_INFO,
    "SecretTablePages = 0x%llx\n",
    SecretTablePages
    ));

  // Populate the COCO Secret table.
  ZeroMem (SecretTableData, EFI_PAGES_TO_SIZE (SecretTablePages));

  // Install the COCO Secret Header.
  SecretHeader = (COCO_SECRET_HEADER *)SecretTableData;
  CopyMem (
    &SecretHeader->Guid,
    &gConfidentialComputingSecretHeader,
    sizeof (EFI_GUID)
    );
  SecretHeader->Length = SecretTableDataLength;

  // Install the COCO Secret Entry.
  SecretEntry = (COCO_SECRET_ENTRY *)((UINT8 *)SecretHeader +
                                      sizeof (COCO_SECRET_HEADER));
  CopyMem (
    &SecretEntry->Guid,
    &gConfidentialComputingSecretEntryGrubEfiDiskPassword,
    sizeof (EFI_GUID)
    );

  // Add 1 for null terminating the password string
  SecretEntry->Length = sizeof (COCO_SECRET_ENTRY) + SecretDataLen + 1;

  Secret = (UINT8 *)SecretEntry + sizeof (COCO_SECRET_ENTRY);
  CopyMem (Secret, SecretData, SecretDataLen);

  DEBUG ((
    DEBUG_INFO,
    "Secret Header (%p) = {%g}, Length = %d\n",
    SecretHeader,
    &SecretHeader->Guid,
    SecretHeader->Length
    ));
  DEBUG ((
    DEBUG_INFO,
    "SecretEntry (%p) = {%g}, Length = %d\n",
    SecretEntry,
    &SecretEntry->Guid,
    SecretEntry->Length
    ));

  SecretTable->Base = (UINT64)SecretTableData;
  SecretTable->Size = SecretTableDataLength;

  return gBS->InstallConfigurationTable (
                &gConfidentialComputingSecretGuid,
                SecretTable
                );

ExitHandler:
  gBS->FreePool (SecretTable);
  return Status;
}

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

  @param[in]  ImageHandle       Handle to the Image.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_PROTOCOL_ERROR      A protocol error occured.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
PerformSync (
  IN EFI_HANDLE  ImageHandle
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

  Status = BsbGetElement (
             BibHeader,
             &gArmBootSyncSecretData,
             &BsbElement
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Boot Sync to retrieve Disk Secret Key Data!\n", Status));
    goto exit_handler1;
  }

  DBG_DUMP_RAW (
    "Disk Secret Key Data",
    (UINT8 *)(BsbElement + 1),
    (BsbElement->Header.Length - sizeof (BOOT_SYNC_BSB_ELEMENT))
    );

  Status = InitialiseSecretTable (
             ImageHandle,
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

  Status = PerformSync (ImageHandle);
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
