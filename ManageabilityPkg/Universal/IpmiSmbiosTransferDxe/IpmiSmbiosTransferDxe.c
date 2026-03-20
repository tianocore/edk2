/** @file

  A driver that sends SMBIOS tables to an OpenBMC receiver

  SPDX-FileCopyrightText: copyright (c) 2022-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Guid/SmBios.h>

#include <IndustryStandard/SmBios.h>

#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/IpmiBlobTransfer.h>

#define SMBIOS_HASH_VARIABLE                   L"SmbiosHash"
#define SMBIOS_TRANSFER_DEBUG                  DEBUG_MANAGEABILITY
#define SMBIOS_EC_DESC_NO_SMBIOS_TABLE         "No SMBIOS table installed"
#define SMBIOS_EC_DESC_SMBIOS_TRANSFER_FAILED  "Failed to send SMBIOS tables to BMC"
#define SMBIOS_IPMI_COMMIT_RETRY               10

/**
  This function will calculate smbios hash, compares it with stored
  hash, updates UEFI variable if smbios data changed and return status.

  @param[in]  SmbiosData      The smbios data to detect if changed.
  @param[in]  SmbiosDataSize  The smbios data size.

  @retval TRUE  If smbios data changed, otherwise FALSE.
**/
BOOLEAN
DetectSmbiosChange (
  UINT8   *SmbiosData,
  UINT32  SmbiosDataSize
  )
{
  BOOLEAN     DeleteVar;
  BOOLEAN     Response;
  UINT8       ComputedHashValue[SHA256_DIGEST_SIZE];
  UINT8       StoredHashValue[SHA256_DIGEST_SIZE];
  UINTN       StoredHashValueSize;
  EFI_STATUS  Status;

  DeleteVar = FALSE;
  Response  = Sha256HashAll ((VOID *)SmbiosData, SmbiosDataSize, ComputedHashValue);

  if (!Response) {
    // Delete the SmbiosHash variable and continue with smbios transfer to BMC
    Status = gRT->SetVariable (
                    SMBIOS_HASH_VARIABLE,
                    &gManageabilityVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    0,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to delete UEFI Variable SmbiosHash %r\n", __func__, Status));
    }

    DeleteVar = TRUE;
  } else {
    StoredHashValueSize = SHA256_DIGEST_SIZE;
    // Get the stored smbios data hash
    Status = gRT->GetVariable (
                    SMBIOS_HASH_VARIABLE,
                    &gManageabilityVariableGuid,
                    NULL,
                    &StoredHashValueSize,
                    (VOID *)StoredHashValue
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a: Failed to get UEFI Variable SmbiosHash %r\n", __func__, Status));
    } else {
      // Compare StoredHash with currently ComputedHash
      if (StoredHashValueSize != SHA256_DIGEST_SIZE) {
        DEBUG ((DEBUG_ERROR, "%a: Invalid Hash Size %u\n", __func__, StoredHashValueSize));
      }

      if (CompareMem (StoredHashValue, ComputedHashValue, SHA256_DIGEST_SIZE) == 0) {
        DEBUG ((DEBUG_INFO, "%a: Same Keys , Hash values match\n", __func__));
        return FALSE;
      }
    }
  }

  if (!DeleteVar) {
    //
    // ComputedHashValue is valid
    // store the computed hash and transfer smbios tables if smbios hash variable not found or hash mismatched.
    //
    Status = gRT->SetVariable (
                    SMBIOS_HASH_VARIABLE,
                    &gManageabilityVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    SHA256_DIGEST_SIZE,
                    (VOID *)ComputedHashValue
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to set UEFI Variable SmbiosHash %r\n", __func__, Status));
    }
  }

  return TRUE;
}

/**
  This function will send all installed SMBIOS tables to the BMC

  @param  Event    The event of notify protocol.
  @param  Context  Notify event context.
**/
VOID
EFIAPI
IpmiSmbiosTransferSendTables (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                         Status;
  SMBIOS_TABLE_3_0_ENTRY_POINT       *Smbios30Table;
  SMBIOS_TABLE_3_0_ENTRY_POINT       *Smbios30TableModified;
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL  *IpmiBlobTransfer;
  UINT16                             Index;
  UINT16                             SessionId;
  UINT8                              *SendData;
  UINT32                             SendDataSize;
  UINT32                             RemainingDataSize;
  BOOLEAN                            SmbiosTransferRequired;
  UINTN                              RetryIndex;
  UINT16                             BlobState;

  gBS->CloseEvent (Event);

  Status = gBS->LocateProtocol (&gEdkiiIpmiBlobTransferProtocolGuid, NULL, (VOID **)&IpmiBlobTransfer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: No IpmiBlobTransferProtocol available. Exiting\n", __func__));
    goto ErrorExit;
  }

  SmbiosTransferRequired = TRUE;
  Smbios30Table          = NULL;
  Status                 = EfiGetSystemConfigurationTable (&gEfiSmbios3TableGuid, (VOID **)&Smbios30Table);
  if (EFI_ERROR (Status) || (Smbios30Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: No SMBIOS Table found: %r\n", __func__, Status));
    REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
      EFI_ERROR_CODE | EFI_ERROR_MAJOR,
      EFI_SOFTWARE_EFI_APPLICATION,
      SMBIOS_EC_DESC_NO_SMBIOS_TABLE,
      sizeof (SMBIOS_EC_DESC_NO_SMBIOS_TABLE)
      );
    return;
  }

  //
  // BMC expects the Smbios Entry Point to point to the address within the binary data sent
  // The value is initially pointing to the location in memory where the table lives
  // So we will save off that value and then modify the entry point to make the BMC happy
  //
  Smbios30TableModified = AllocateZeroPool (sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT));
  CopyMem (Smbios30TableModified, Smbios30Table, sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT));
  Smbios30TableModified->TableAddress = sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT);
  //
  // Fixup checksums in the Entry Point Structure
  //
  Smbios30TableModified->EntryPointStructureChecksum = 0;
  Smbios30TableModified->EntryPointStructureChecksum =
    CalculateCheckSum8 ((UINT8 *)Smbios30TableModified, Smbios30TableModified->EntryPointLength);

  SendData = AllocateZeroPool (sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT) + Smbios30Table->TableMaximumSize);
  CopyMem (SendData, Smbios30TableModified, sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT));
  CopyMem (SendData + sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT), (UINT8 *)Smbios30Table->TableAddress, Smbios30Table->TableMaximumSize);
  SendDataSize      = sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT) + Smbios30Table->TableMaximumSize;
  RemainingDataSize = SendDataSize;

  if (PcdGetBool (PcdSendSmbiosOnChanged)) {
    SmbiosTransferRequired = DetectSmbiosChange (SendData, SendDataSize);

    if (!SmbiosTransferRequired) {
      DEBUG ((DEBUG_INFO, "%a: Smbios tables are not changed, skipping transfer to BMC\n", __func__));
      return;
    }
  }

  DEBUG_CODE_BEGIN ();
  DEBUG ((SMBIOS_TRANSFER_DEBUG, "%a: SMBIOS BINARY DATA OUTPUT\n", __func__));
  DEBUG ((SMBIOS_TRANSFER_DEBUG, "%a: Table Address: 0x%x\n", __func__, Smbios30Table->TableAddress));
  DEBUG ((SMBIOS_TRANSFER_DEBUG, "%a: Table Length: %d\n", __func__, Smbios30Table->TableMaximumSize));
  for (Index = 0; Index < SendDataSize; Index++) {
    if (((Index % BLOB_MAX_DATA_PER_PACKET) / 2) == 0) {
      DEBUG ((SMBIOS_TRANSFER_DEBUG, "\n%04x: ", Index));
    }

    DEBUG ((SMBIOS_TRANSFER_DEBUG, "%02x ", *(SendData + Index)));
  }

  DEBUG ((SMBIOS_TRANSFER_DEBUG, "\n"));
  DEBUG_CODE_END ();

  Status = IpmiBlobTransfer->BlobOpen ((CHAR8 *)PcdGetPtr (PcdBmcSmbiosBlobTransferId), BLOB_TRANSFER_STAT_OPEN_W, &SessionId);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_UNSUPPORTED) {
      return;
    }

    DEBUG ((DEBUG_ERROR, "%a: Unable to open Blob with Id %a: %r\n", __func__, PcdGetPtr (PcdBmcSmbiosBlobTransferId), Status));
    goto ErrorExit;
  }

  for (Index = 0; Index < (SendDataSize / BLOB_MAX_DATA_PER_PACKET); Index++) {
    Status = IpmiBlobTransfer->BlobWrite (SessionId, Index * BLOB_MAX_DATA_PER_PACKET, SendData, BLOB_MAX_DATA_PER_PACKET);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failure writing to blob: %r\n", __func__, Status));
      goto ErrorExit;
    }

    SendData           = (UINT8 *)SendData + BLOB_MAX_DATA_PER_PACKET;
    RemainingDataSize -= BLOB_MAX_DATA_PER_PACKET;
  }

  ASSERT (RemainingDataSize < BLOB_MAX_DATA_PER_PACKET);
  if (RemainingDataSize) {
    Status = IpmiBlobTransfer->BlobWrite (SessionId, Index * BLOB_MAX_DATA_PER_PACKET, SendData, RemainingDataSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failure writing final block to blob: %r\n", __func__, Status));
      goto ErrorExit;
    }
  }

  Status = IpmiBlobTransfer->BlobCommit (SessionId, 0, NULL);
  if (EFI_ERROR (Status)) {
    // Poll the stat for the blob committed
    for (RetryIndex = 0, BlobState = 0; RetryIndex < SMBIOS_IPMI_COMMIT_RETRY; RetryIndex++, BlobState = 0) {
      Status = IpmiBlobTransfer->BlobSessionStat (SessionId, &BlobState, NULL, NULL, NULL);
      if (!EFI_ERROR (Status)) {
        if ((BlobState & BLOB_TRANSFER_STAT_COMMITTED) != 0) {
          DEBUG ((DEBUG_INFO, "%a: Blob committed %r\n", __func__));
          break;
        }

        if ((BlobState & BLOB_TRANSFER_STAT_COMMIT_ERROR) != 0) {
          DEBUG ((DEBUG_ERROR, "%a: Failure sending commit to blob: %r\n", __func__, Status));
          break;
        }
      }

      MicroSecondDelay (200 * 1000); // 200ms
    }

    if ((BlobState & BLOB_TRANSFER_STAT_COMMITTED) == 0) {
      DEBUG ((DEBUG_ERROR, "%a: Failure sending commit to blob: %r\n", __func__, Status));
      goto ErrorExit;
    }
  }

  Status = IpmiBlobTransfer->BlobClose (SessionId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Sent SMBIOS Tables to BMC: %r\n", __func__, Status));
    goto ErrorExit;
  }

  return;

ErrorExit:
  REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
    EFI_ERROR_CODE | EFI_ERROR_MAJOR,
    EFI_SOFTWARE_EFI_APPLICATION,
    SMBIOS_EC_DESC_SMBIOS_TRANSFER_FAILED,
    sizeof (SMBIOS_EC_DESC_SMBIOS_TRANSFER_FAILED)
    );
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.

**/
EFI_STATUS
EFIAPI
IpmiSmbiosTransferEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   ReadyToBootEvent;

  //
  // Register ReadyToBoot event to send the SMBIOS tables once they have all been installed
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  IpmiSmbiosTransferSendTables,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &ReadyToBootEvent
                  );

  ASSERT_EFI_ERROR (Status);
  return Status;
}
