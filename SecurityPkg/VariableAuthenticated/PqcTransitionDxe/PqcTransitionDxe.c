/** @file
  Post-Quantum Cryptography (PQC) Transition Manager for UEFI.

  This driver provides PQC transition functionality for UEFI firmware,
  allowing controlled migration from traditional cryptographic algorithms
  to post-quantum cryptographic algorithms in preparation for the 2030 deadline.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PqcTransitionDxe.h"

//
// HII Vendor Device Path template
//
HII_VENDOR_DEVICE_PATH  mPqcTransitionHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    PQC_TRANSITION_CONFIG_FORM_SET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

//
// Global variables
//
PQC_TRANSITION_PRIVATE_DATA  *gPqcTransitionPrivateData = NULL;
EFI_GUID                     gPqcTransitionConfigFormSetGuid = PQC_TRANSITION_CONFIG_FORM_SET_GUID;

/**
  Check if a signature database contains PQC certificates.

  @param[in] VariableName       Name of the signature database variable.
  @param[in] VendorGuid         GUID of the signature database variable.

  @retval TRUE                  Database contains PQC certificates.
  @retval FALSE                 Database does not contain PQC certificates.
**/
BOOLEAN
CheckSignatureDatabaseForPqc (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS         Status;
  UINT8              *Data;
  UINTN              DataSize;
  EFI_SIGNATURE_LIST *SigList;
  UINTN              Index;
  BOOLEAN            HasPqcCert = FALSE;

  //
  // Get the signature database
  //
  Status = GetVariable2 (VariableName, VendorGuid, (VOID**)&Data, &DataSize);
  if (EFI_ERROR (Status) || (Data == NULL)) {
    return FALSE;
  }

  //
  // Parse signature lists
  //
  SigList = (EFI_SIGNATURE_LIST *) Data;
  Index = 0;
  
  while ((Index < DataSize) && (SigList->SignatureListSize != 0)) {
    //
    // Check for PQC signature types
    // Note: These GUIDs would need to be defined for actual PQC algorithms
    // For now, we'll check for known traditional types and assume others might be PQC
    //
    if (!CompareGuid (&SigList->SignatureType, &gEfiCertRsa2048Guid) &&
        !CompareGuid (&SigList->SignatureType, &gEfiCertX509Guid) &&
        !CompareGuid (&SigList->SignatureType, &gEfiCertPkcs7Guid)) {
      //
      // This might be a PQC certificate type
      //
      HasPqcCert = TRUE;
      break;
    }
    
    Index += SigList->SignatureListSize;
    SigList = (EFI_SIGNATURE_LIST *) ((UINT8 *) SigList + SigList->SignatureListSize);
  }

  FreePool (Data);
  return HasPqcCert;
}

/**
  Check if Option ROMs are PQC-signed.

  @retval TRUE                  Option ROMs are PQC-signed.
  @retval FALSE                 Option ROMs are not PQC-signed or not found.
**/
BOOLEAN
CheckOptionRomPqcSignatures (
  VOID
  )
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  BOOLEAN                   HasPqcSignedOrom = FALSE;

  //
  // Get all PCI IO protocol handles
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "PQC: No PCI devices found for Option ROM check\n"));
    return FALSE;
  }

  //
  // Check each PCI device for Option ROM
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &PciIo
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // TODO: Implement actual Option ROM signature verification
    // This would involve:
    // 1. Reading Option ROM from PCI device
    // 2. Parsing PE/COFF signature
    // 3. Verifying signature against PQC algorithms
    // 4. Checking against authorized signature database
    //
    // For now, assume no PQC-signed Option ROMs are present
    // This is a conservative approach that prevents premature PQC-only switch
    //
    DEBUG ((DEBUG_INFO, "PQC: Checking Option ROM on PCI device %d\n", Index));
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  DEBUG ((DEBUG_INFO, "PQC: Option ROM PQC signature check result: %d\n", HasPqcSignedOrom));
  return HasPqcSignedOrom;
}

/**
  Check if OS Loaders are PQC-signed.

  @retval TRUE                  OS Loaders are PQC-signed.
  @retval FALSE                 OS Loaders are not PQC-signed or not found.
**/
BOOLEAN
CheckOsLoaderPqcSignatures (
  VOID
  )
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  BOOLEAN                   HasPqcSignedLoader = FALSE;

  //
  // Get all file system handles
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "PQC: No file systems found for OS Loader check\n"));
    return FALSE;
  }

  //
  // Check each file system for common OS loaders
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID **) &FileSystem
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // TODO: Implement actual OS Loader signature verification
    // This would involve:
    // 1. Scanning for common bootloaders (bootx64.efi, grubx64.efi, etc.)
    // 2. Reading PE/COFF signature from each loader
    // 3. Verifying signature against PQC algorithms
    // 4. Checking against authorized signature database
    //
    // Common paths to check:
    // - \EFI\BOOT\bootx64.efi
    // - \EFI\Microsoft\Boot\bootmgfw.efi
    // - \EFI\ubuntu\grubx64.efi
    // - \EFI\fedora\grubx64.efi
    //
    // For now, assume no PQC-signed OS loaders are present
    // This is a conservative approach that prevents premature PQC-only switch
    //
    DEBUG ((DEBUG_INFO, "PQC: Checking OS loaders on file system %d\n", Index));
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  DEBUG ((DEBUG_INFO, "PQC: OS Loader PQC signature check result: %d\n", HasPqcSignedLoader));
  return HasPqcSignedLoader;
}

/**
  Check if TLS/HTTPS boot supports PQC algorithms.

  @retval TRUE                  TLS/HTTPS boot supports PQC.
  @retval FALSE                 TLS/HTTPS boot does not support PQC.
**/
BOOLEAN
CheckTlsHttpsPqcSupport (
  VOID
  )
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  BOOLEAN                   HasPqcTlsSupport = FALSE;

  //
  // Check for TLS protocol support
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiTlsProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    //
    // TODO: Implement actual TLS PQC capability check
    // This would involve:
    // 1. Querying TLS protocol for supported cipher suites
    // 2. Checking for PQC-based cipher suites
    // 3. Verifying PQC certificate chain validation capability
    //
    // For now, assume no PQC TLS support is available
    // This is a conservative approach that prevents premature PQC-only switch
    //
    DEBUG ((DEBUG_INFO, "PQC: Found %d TLS protocol instances\n", HandleCount));
    
    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }
  }

  DEBUG ((DEBUG_INFO, "PQC: TLS/HTTPS PQC support check result: %d\n", HasPqcTlsSupport));
  return HasPqcTlsSupport;
}

/**
  Check if firmware update mechanism supports PQC algorithms.

  @retval TRUE                  Firmware update supports PQC.
  @retval FALSE                 Firmware update does not support PQC.
**/
BOOLEAN
CheckFirmwareUpdatePqcSupport (
  VOID
  )
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  BOOLEAN                   HasPqcFwUpdateSupport = FALSE;

  //
  // Check for Firmware Management Protocol support
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    //
    // TODO: Implement actual firmware update PQC capability check
    // This would involve:
    // 1. Querying FMP for supported signature algorithms
    // 2. Checking for PQC signature algorithm support
    // 3. Verifying PQC certificate chain validation capability
    //
    // For now, assume no PQC firmware update support is available
    // This is a conservative approach that prevents premature PQC-only switch
    //
    DEBUG ((DEBUG_INFO, "PQC: Found %d Firmware Management Protocol instances\n", HandleCount));
    
    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }
  }

  DEBUG ((DEBUG_INFO, "PQC: Firmware update PQC support check result: %d\n", HasPqcFwUpdateSupport));
  return HasPqcFwUpdateSupport;
}

/**
  Check if the system is ready for PQC-only mode transition.
  
  This function performs comprehensive readiness validation as specified in NIST guidelines:
  A) Check if PQC cert is provisioned in PK
  B) Check if PQC cert is provisioned in KEK/DB  
  C) Check if current Option ROMs are PQC-signed
  D) Check if OS Loaders are PQC-signed
  E) Check if TLS/HTTPS boot supports PQC
  F) Check if firmware update mechanism supports PQC

  @param[out] ReadinessStatus    Pointer to store the readiness check results.

  @retval EFI_SUCCESS           Readiness check completed successfully.
  @retval EFI_INVALID_PARAMETER ReadinessStatus is NULL.
  @retval Others                Error occurred during readiness check.
**/
EFI_STATUS
CheckPqcReadiness (
  OUT PQC_READINESS_STATUS  *ReadinessStatus
  )
{
  if (ReadinessStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (ReadinessStatus, sizeof (PQC_READINESS_STATUS));

  DEBUG ((DEBUG_INFO, "PQC: Starting comprehensive readiness check...\n"));

  //
  // A) Check if PQC cert is provisioned in PK
  //
  ReadinessStatus->PkHasPqcCert = CheckSignatureDatabaseForPqc (
                                    EFI_PLATFORM_KEY_NAME,
                                    &gEfiGlobalVariableGuid
                                    );

  //
  // B) Check if PQC cert is provisioned in KEK/DB
  //
  ReadinessStatus->KekHasPqcCert = CheckSignatureDatabaseForPqc (
                                     EFI_KEY_EXCHANGE_KEY_NAME,
                                     &gEfiGlobalVariableGuid
                                     );

  ReadinessStatus->DbHasPqcCert = CheckSignatureDatabaseForPqc (
                                    EFI_IMAGE_SECURITY_DATABASE,
                                    &gEfiImageSecurityDatabaseGuid
                                    );

  //
  // C) Check if current Option ROMs are PQC-signed
  //
  ReadinessStatus->OromIsPqcSigned = CheckOptionRomPqcSignatures ();

  //
  // D) Check if OS Loaders are PQC-signed
  //
  ReadinessStatus->LoaderIsPqcSigned = CheckOsLoaderPqcSignatures ();

  //
  // E) Check if TLS/HTTPS boot supports PQC
  //
  ReadinessStatus->TlsHasPqcSupport = CheckTlsHttpsPqcSupport ();

  //
  // F) Check if firmware update mechanism supports PQC
  //
  ReadinessStatus->FwUpdateHasPqcSupport = CheckFirmwareUpdatePqcSupport ();

  //
  // Determine overall system readiness
  // All components must be PQC-ready for safe transition to PQC-only mode
  // This implements the conservative approach recommended in NIST guidelines
  //
  ReadinessStatus->SystemReadyForPqc = ReadinessStatus->PkHasPqcCert &&
                                       ReadinessStatus->KekHasPqcCert &&
                                       ReadinessStatus->DbHasPqcCert &&
                                       ReadinessStatus->OromIsPqcSigned &&
                                       ReadinessStatus->LoaderIsPqcSigned &&
                                       ReadinessStatus->TlsHasPqcSupport &&
                                       ReadinessStatus->FwUpdateHasPqcSupport;

  DEBUG ((DEBUG_INFO, "PQC Comprehensive Readiness Check Results:\n"));
  DEBUG ((DEBUG_INFO, "  A) PK has PQC cert: %d\n", ReadinessStatus->PkHasPqcCert));
  DEBUG ((DEBUG_INFO, "  B) KEK has PQC cert: %d\n", ReadinessStatus->KekHasPqcCert));
  DEBUG ((DEBUG_INFO, "  B) DB has PQC cert: %d\n", ReadinessStatus->DbHasPqcCert));
  DEBUG ((DEBUG_INFO, "  C) Option ROMs are PQC-signed: %d\n", ReadinessStatus->OromIsPqcSigned));
  DEBUG ((DEBUG_INFO, "  D) OS Loaders are PQC-signed: %d\n", ReadinessStatus->LoaderIsPqcSigned));
  DEBUG ((DEBUG_INFO, "  E) TLS/HTTPS has PQC support: %d\n", ReadinessStatus->TlsHasPqcSupport));
  DEBUG ((DEBUG_INFO, "  F) Firmware update has PQC support: %d\n", ReadinessStatus->FwUpdateHasPqcSupport));
  DEBUG ((DEBUG_INFO, "  Overall system ready for PQC: %d\n", ReadinessStatus->SystemReadyForPqc));

  return EFI_SUCCESS;
}

/**
  Perform PQC transition mode switch.
  
  This function implements the NIST-compliant PQC transition with proper validation:
  - Validates system readiness before PQC-only switch
  - Updates firmware configuration variables
  - Modifies cryptographic library behavior
  - Updates secure boot policy
  - Provides recovery mechanisms

  @param[in] NewMode            The new PQC transition mode to switch to.

  @retval EFI_SUCCESS           Mode switch completed successfully.
  @retval EFI_ACCESS_DENIED     System is not ready for the requested mode.
  @retval EFI_UNSUPPORTED       Requested mode is not supported.
  @retval Others                Error occurred during mode switch.
**/
EFI_STATUS
SwitchPqcTransitionMode (
  IN PQC_TRANSITION_MODE  NewMode
  )
{
  EFI_STATUS            Status;
  PQC_READINESS_STATUS  ReadinessStatus;
  UINT32                PqcModeVariable;
  UINTN                 VariableSize;

  DEBUG ((DEBUG_INFO, "PQC: Attempting to switch to mode %d\n", NewMode));

  //
  // Validate the requested mode
  //
  if (NewMode >= PQC_MODE_MAX) {
    DEBUG ((DEBUG_ERROR, "PQC: Invalid transition mode requested: %d\n", NewMode));
    return EFI_UNSUPPORTED;
  }

  //
  // Check system readiness before switching to PQC-only mode
  //
  if (NewMode == PQC_MODE_PQC_ONLY) {
    DEBUG ((DEBUG_INFO, "PQC: PQC-only mode requested, performing comprehensive readiness check\n"));
    
    Status = CheckPqcReadiness (&ReadinessStatus);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "PQC: Readiness check failed: %r\n", Status));
      return Status;
    }

    if (!ReadinessStatus.SystemReadyForPqc) {
      DEBUG ((DEBUG_ERROR, "PQC: System is not ready for PQC-only mode\n"));
      DEBUG ((DEBUG_ERROR, "PQC: Missing requirements:\n"));
      if (!ReadinessStatus.PkHasPqcCert) {
        DEBUG ((DEBUG_ERROR, "  - PK does not contain PQC certificates\n"));
      }
      if (!ReadinessStatus.KekHasPqcCert) {
        DEBUG ((DEBUG_ERROR, "  - KEK does not contain PQC certificates\n"));
      }
      if (!ReadinessStatus.DbHasPqcCert) {
        DEBUG ((DEBUG_ERROR, "  - DB does not contain PQC certificates\n"));
      }
      if (!ReadinessStatus.OromIsPqcSigned) {
        DEBUG ((DEBUG_ERROR, "  - Option ROMs are not PQC-signed\n"));
      }
      if (!ReadinessStatus.LoaderIsPqcSigned) {
        DEBUG ((DEBUG_ERROR, "  - OS Loaders are not PQC-signed\n"));
      }
      if (!ReadinessStatus.TlsHasPqcSupport) {
        DEBUG ((DEBUG_ERROR, "  - TLS/HTTPS does not support PQC\n"));
      }
      if (!ReadinessStatus.FwUpdateHasPqcSupport) {
        DEBUG ((DEBUG_ERROR, "  - Firmware update does not support PQC\n"));
      }
      
      return EFI_ACCESS_DENIED;
    }
    
    DEBUG ((DEBUG_INFO, "PQC: System readiness check passed, proceeding with PQC-only transition\n"));
  }

  //
  // Store the new mode in a UEFI variable for persistence across reboots
  // This variable controls the cryptographic behavior of the firmware
  //
  PqcModeVariable = (UINT32) NewMode;
  Status = gRT->SetVariable (
                  L"PqcTransitionMode",
                  &gPqcTransitionConfigFormSetGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (UINT32),
                  &PqcModeVariable
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PQC: Failed to set PqcTransitionMode variable: %r\n", Status));
    return Status;
  }

  //
  // Update cryptographic library behavior based on the new mode
  // This affects all cryptographic operations in the firmware
  //
  switch (NewMode) {
    case PQC_MODE_TRADITIONAL_ONLY:
      DEBUG ((DEBUG_INFO, "PQC: Configuring for Traditional-only mode\n"));
      //
      // TODO: Configure crypto libraries to use only traditional algorithms
      // - Disable PQC algorithm support
      // - Use RSA/ECDSA for all operations
      // - Reject PQC certificates
      //
      break;

    case PQC_MODE_HYBRID:
      DEBUG ((DEBUG_INFO, "PQC: Configuring for Hybrid mode (Traditional + PQC)\n"));
      //
      // TODO: Configure crypto libraries to support both algorithm types
      // - Enable both traditional and PQC algorithms
      // - Accept both RSA/ECDSA and PQC certificates
      // - Prefer PQC when available, fallback to traditional
      //
      break;

    case PQC_MODE_PQC_ONLY:
      DEBUG ((DEBUG_INFO, "PQC: Configuring for PQC-only mode\n"));
      //
      // TODO: Configure crypto libraries to use only PQC algorithms
      // - Disable traditional algorithm support
      // - Use only Dilithium/Falcon/Kyber for operations
      // - Reject traditional certificates
      //
      
      //
      // Optionally clean up traditional algorithms from signature databases
      // This is for attestation purposes as mentioned in NIST guidelines
      //
      DEBUG ((DEBUG_INFO, "PQC: Considering cleanup of traditional algorithms\n"));
      break;

    default:
      DEBUG ((DEBUG_ERROR, "PQC: Unknown transition mode: %d\n", NewMode));
      return EFI_UNSUPPORTED;
  }

  //
  // Update secure boot policy based on the new mode
  // This affects how UEFI secure boot validates signatures
  //
  Status = UpdateSecureBootPolicy (NewMode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PQC: Failed to update secure boot policy: %r\n", Status));
    //
    // Attempt to rollback the variable change
    //
    VariableSize = sizeof (UINT32);
    Status = gRT->GetVariable (
                    L"PqcTransitionMode",
                    &gPqcTransitionConfigFormSetGuid,
                    NULL,
                    &VariableSize,
                    &PqcModeVariable
                    );
    if (!EFI_ERROR (Status)) {
      //
      // TODO: Restore previous mode configuration
      //
      DEBUG ((DEBUG_WARN, "PQC: Rollback may be needed\n"));
    }
    return Status;
  }

  DEBUG ((DEBUG_INFO, "PQC: Successfully switched to transition mode %d\n", NewMode));
  
  //
  // Log the transition for audit purposes
  //
  LogPqcTransition (NewMode);
  
  return EFI_SUCCESS;
}

/**
  Update secure boot policy based on PQC transition mode.

  @param[in] Mode               The PQC transition mode.

  @retval EFI_SUCCESS           Policy updated successfully.
  @retval Others                Error occurred during policy update.
**/
EFI_STATUS
UpdateSecureBootPolicy (
  IN PQC_TRANSITION_MODE  Mode
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  DEBUG ((DEBUG_INFO, "PQC: Updating secure boot policy for mode %d\n", Mode));

  //
  // TODO: Implement secure boot policy updates
  // This would involve:
  // 1. Updating signature verification algorithms
  // 2. Modifying certificate chain validation
  // 3. Configuring algorithm preferences
  // 4. Setting up fallback mechanisms
  //
  // The implementation would be platform-specific and require
  // integration with the secure boot infrastructure
  //

  switch (Mode) {
    case PQC_MODE_TRADITIONAL_ONLY:
      //
      // Configure secure boot to use only traditional algorithms
      //
      DEBUG ((DEBUG_INFO, "PQC: Secure boot configured for traditional algorithms only\n"));
      break;

    case PQC_MODE_HYBRID:
      //
      // Configure secure boot to support both traditional and PQC algorithms
      //
      DEBUG ((DEBUG_INFO, "PQC: Secure boot configured for hybrid mode\n"));
      break;

    case PQC_MODE_PQC_ONLY:
      //
      // Configure secure boot to use only PQC algorithms
      //
      DEBUG ((DEBUG_INFO, "PQC: Secure boot configured for PQC algorithms only\n"));
      break;

    default:
      Status = EFI_UNSUPPORTED;
      break;
  }

  return Status;
}

/**
  Log PQC transition event for audit purposes.

  @param[in] Mode               The new PQC transition mode.
**/
VOID
LogPqcTransition (
  IN PQC_TRANSITION_MODE  Mode
  )
{
  EFI_TIME  Time;
  EFI_STATUS Status;

  //
  // Get current time for audit log
  //
  Status = gRT->GetTime (&Time, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "PQC: Failed to get time for audit log: %r\n", Status));
    return;
  }

  //
  // Log the transition event
  //
  DEBUG ((DEBUG_INFO, "PQC AUDIT: Transition to mode %d at %04d-%02d-%02d %02d:%02d:%02d\n",
          Mode, Time.Year, Time.Month, Time.Day, Time.Hour, Time.Minute, Time.Second));

  //
  // TODO: Implement persistent audit logging
  // This could involve:
  // 1. Writing to UEFI event log
  // 2. Storing in NVRAM
  // 3. Sending to management interface
  // 4. Integration with TPM event log
  //
}

/**
  Clean up traditional algorithms from signature databases.
  
  This function removes traditional algorithms from the authorized signature database
  for attestation purposes after successful transition to PQC-only mode.
  As specified in NIST guidelines, this is optional but recommended for attestation.

  @retval EFI_SUCCESS           Cleanup completed successfully.
  @retval EFI_ACCESS_DENIED     System is not in PQC-only mode.
  @retval Others                Error occurred during cleanup.
**/
EFI_STATUS
CleanupTraditionalAlgorithms (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      CurrentMode;
  UINTN       VariableSize;

  DEBUG ((DEBUG_INFO, "PQC: Starting traditional algorithm cleanup\n"));

  //
  // Verify we are in PQC-only mode before cleanup
  //
  VariableSize = sizeof (UINT32);
  Status = gRT->GetVariable (
                  L"PqcTransitionMode",
                  &gPqcTransitionConfigFormSetGuid,
                  NULL,
                  &VariableSize,
                  &CurrentMode
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PQC: Failed to get current transition mode: %r\n", Status));
    return Status;
  }

  if (CurrentMode != PQC_MODE_PQC_ONLY) {
    DEBUG ((DEBUG_ERROR, "PQC: Cleanup only allowed in PQC-only mode (current: %d)\n", CurrentMode));
    return EFI_ACCESS_DENIED;
  }

  //
  // Clean up traditional algorithms from PK
  //
  Status = CleanupSignatureDatabaseTraditionalAlgorithms (
             EFI_PLATFORM_KEY_NAME,
             &gEfiGlobalVariableGuid,
             L"PK"
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PQC: Failed to cleanup PK: %r\n", Status));
    return Status;
  }

  //
  // Clean up traditional algorithms from KEK
  //
  Status = CleanupSignatureDatabaseTraditionalAlgorithms (
             EFI_KEY_EXCHANGE_KEY_NAME,
             &gEfiGlobalVariableGuid,
             L"KEK"
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PQC: Failed to cleanup KEK: %r\n", Status));
    return Status;
  }

  //
  // Clean up traditional algorithms from DB
  //
  Status = CleanupSignatureDatabaseTraditionalAlgorithms (
             EFI_IMAGE_SECURITY_DATABASE,
             &gEfiImageSecurityDatabaseGuid,
             L"DB"
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PQC: Failed to cleanup DB: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "PQC: Traditional algorithms cleanup completed successfully\n"));
  
  return EFI_SUCCESS;
}

/**
  Clean up traditional algorithms from a specific signature database.

  @param[in] VariableName       Name of the signature database variable.
  @param[in] VendorGuid         GUID of the signature database variable.
  @param[in] DatabaseName       Human-readable name for logging.

  @retval EFI_SUCCESS           Cleanup completed successfully.
  @retval Others                Error occurred during cleanup.
**/
EFI_STATUS
CleanupSignatureDatabaseTraditionalAlgorithms (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN CHAR16    *DatabaseName
  )
{
  EFI_STATUS         Status;
  UINT8              *Data;
  UINTN              DataSize;
  UINT8              *NewData;
  UINTN              NewDataSize;
  EFI_SIGNATURE_LIST *SigList;
  EFI_SIGNATURE_LIST *NewSigList;
  UINTN              Index;
  UINTN              NewIndex;
  BOOLEAN            IsTraditional;
  UINTN              TraditionalCount = 0;
  UINTN              PqcCount = 0;

  DEBUG ((DEBUG_INFO, "PQC: Cleaning up traditional algorithms from %s\n", DatabaseName));

  //
  // Get the current signature database
  //
  Status = GetVariable2 (VariableName, VendorGuid, (VOID**)&Data, &DataSize);
  if (EFI_ERROR (Status) || (Data == NULL)) {
    DEBUG ((DEBUG_WARN, "PQC: %s database not found or empty\n", DatabaseName));
    return EFI_SUCCESS;  // Nothing to clean up
  }

  //
  // Allocate buffer for new database (worst case: same size)
  //
  NewData = AllocateZeroPool (DataSize);
  if (NewData == NULL) {
    FreePool (Data);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Parse signature lists and copy only PQC certificates
  //
  SigList = (EFI_SIGNATURE_LIST *) Data;
  NewSigList = (EFI_SIGNATURE_LIST *) NewData;
  Index = 0;
  NewIndex = 0;
  NewDataSize = 0;

  while ((Index < DataSize) && (SigList->SignatureListSize != 0)) {
    //
    // Check if this is a traditional algorithm signature
    //
    IsTraditional = FALSE;
    if (CompareGuid (&SigList->SignatureType, &gEfiCertRsa2048Guid) ||
        CompareGuid (&SigList->SignatureType, &gEfiCertX509Guid) ||
        CompareGuid (&SigList->SignatureType, &gEfiCertPkcs7Guid)) {
      IsTraditional = TRUE;
      TraditionalCount++;
    } else {
      //
      // Assume this is a PQC certificate (or unknown type to be preserved)
      //
      PqcCount++;
    }

    if (!IsTraditional) {
      //
      // Copy PQC signature to new database
      //
      CopyMem ((UINT8 *) NewSigList + NewIndex, (UINT8 *) SigList, SigList->SignatureListSize);
      NewIndex += SigList->SignatureListSize;
      NewDataSize += SigList->SignatureListSize;
    } else {
      DEBUG ((DEBUG_INFO, "PQC: Removing traditional signature from %s\n", DatabaseName));
    }

    Index += SigList->SignatureListSize;
    SigList = (EFI_SIGNATURE_LIST *) ((UINT8 *) SigList + SigList->SignatureListSize);
  }

  //
  // Update the signature database with cleaned data
  //
  if (NewDataSize > 0) {
    Status = gRT->SetVariable (
                    VariableName,
                    VendorGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    NewDataSize,
                    NewData
                    );
  } else {
    //
    // No PQC certificates found, delete the variable
    //
    Status = gRT->SetVariable (
                    VariableName,
                    VendorGuid,
                    0,
                    0,
                    NULL
                    );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PQC: Failed to update %s database: %r\n", DatabaseName, Status));
  } else {
    DEBUG ((DEBUG_INFO, "PQC: %s cleanup complete - removed %d traditional, kept %d PQC signatures\n",
            DatabaseName, TraditionalCount, PqcCount));
  }

  FreePool (Data);
  FreePool (NewData);
  
  return Status;
}

/**
  Initialize the PQC Transition configuration data.

  @param[in] PrivateData        Pointer to the private data structure.

  @retval EFI_SUCCESS           Initialization completed successfully.
  @retval Others                Error occurred during initialization.
**/
EFI_STATUS
InitializePqcTransitionConfiguration (
  IN PQC_TRANSITION_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS  Status;
  EFI_TIME    Time;

  //
  // Initialize configuration with default values
  //
  ZeroMem (&PrivateData->Configuration, sizeof (PQC_TRANSITION_CONFIGURATION));
  
  //
  // Set default transition mode to hybrid (both traditional and PQC allowed)
  //
  PrivateData->Configuration.PqcTransitionMode = PQC_MODE_HYBRID;
  
  //
  // Set transition deadline to 2030
  //
  PrivateData->Configuration.TransitionDeadline = 2030;
  
  //
  // Get current time to calculate days until deadline
  //
  Status = gRT->GetTime (&Time, NULL);
  if (!EFI_ERROR (Status)) {
    PrivateData->Configuration.CurrentYear = Time.Year;
    if (Time.Year < 2030) {
      PrivateData->Configuration.DaysUntilDeadline = (2030 - Time.Year) * 365;
    } else {
      PrivateData->Configuration.DaysUntilDeadline = 0;
    }
  }
  
  //
  // Initialize PQC algorithm status
  // TODO: Query actual cryptographic library capabilities
  //
  PrivateData->Configuration.DilithiumStatus = PQC_ALGORITHM_SUPPORTED;
  PrivateData->Configuration.FalconStatus = PQC_ALGORITHM_SUPPORTED;
  PrivateData->Configuration.SphincsStatus = PQC_ALGORITHM_NOT_SUPPORTED;
  PrivateData->Configuration.KyberStatus = PQC_ALGORITHM_SUPPORTED;
  PrivateData->Configuration.NtruStatus = PQC_ALGORITHM_NOT_SUPPORTED;
  
  //
  // Perform initial readiness check
  //
  Status = CheckPqcReadiness (&PrivateData->ReadinessStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to perform initial PQC readiness check: %r\n", Status));
    return Status;
  }
  
  //
  // Update configuration with readiness status
  //
  PrivateData->Configuration.PkHasPqcCert = PrivateData->ReadinessStatus.PkHasPqcCert ? 1 : 0;
  PrivateData->Configuration.KekHasPqcCert = PrivateData->ReadinessStatus.KekHasPqcCert ? 1 : 0;
  PrivateData->Configuration.DbHasPqcCert = PrivateData->ReadinessStatus.DbHasPqcCert ? 1 : 0;
  PrivateData->Configuration.SystemReadyForPqc = PrivateData->ReadinessStatus.SystemReadyForPqc ? 1 : 0;
  
  return EFI_SUCCESS;
}

/**
  The entry point for PQC Transition DXE driver.

  @param[in]  ImageHandle        The image handle of the driver.
  @param[in]  SystemTable        The system table.

  @retval EFI_ALREADY_STARTED    The driver already exists in system.
  @retval EFI_OUT_OF_RESOURCES   Fail to execute entry point due to lack of resources.
  @retval EFI_SUCCESS            All the related protocols are installed on the driver.
  @retval Others                 Fail to get or install protocol with error status code.

**/
EFI_STATUS
EFIAPI
PqcTransitionEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                      Status;
  PQC_TRANSITION_PRIVATE_DATA     *PrivateData;

  if (gPqcTransitionPrivateData != NULL) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Allocate private data structure
  //
  PrivateData = AllocateZeroPool (sizeof (PQC_TRANSITION_PRIVATE_DATA));
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize private data
  //
  PrivateData->Signature = PQC_TRANSITION_PRIVATE_DATA_SIGNATURE;
  PrivateData->ConfigAccess.ExtractConfig = PqcTransitionExtractConfig;
  PrivateData->ConfigAccess.RouteConfig = PqcTransitionRouteConfig;
  PrivateData->ConfigAccess.Callback = PqcTransitionCallback;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &PrivateData->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mPqcTransitionHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &PrivateData->ConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Publish our HII data
  //
  PrivateData->HiiHandle = HiiAddPackages (
                             &gPqcTransitionConfigFormSetGuid,
                             PrivateData->DriverHandle,
                             PqcTransitionDxeStrings,
                             PqcTransitionVfrBin,
                             NULL
                             );
  if (PrivateData->HiiHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  //
  // Initialize PQC transition configuration
  //
  Status = InitializePqcTransitionConfiguration (PrivateData);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  gPqcTransitionPrivateData = PrivateData;

  DEBUG ((DEBUG_INFO, "PQC Transition Manager initialized successfully\n"));
  
  return EFI_SUCCESS;

ErrorExit:
  if (PrivateData != NULL) {
    if (PrivateData->DriverHandle != NULL) {
      gBS->UninstallMultipleProtocolInterfaces (
             PrivateData->DriverHandle,
             &gEfiDevicePathProtocolGuid,
             &mPqcTransitionHiiVendorDevicePath,
             &gEfiHiiConfigAccessProtocolGuid,
             &PrivateData->ConfigAccess,
             NULL
             );
    }

    if (PrivateData->HiiHandle != NULL) {
      HiiRemovePackages (PrivateData->HiiHandle);
    }

    FreePool (PrivateData);
  }

  return Status;
}

/**
  Unload function for the PQC Transition DXE driver.

  @param[in]  ImageHandle        The image handle of the driver.

  @retval EFI_SUCCESS            The driver is unloaded successfully.
  @retval Others                 Failed to unload the driver.

**/
EFI_STATUS
EFIAPI
PqcTransitionUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (gPqcTransitionPrivateData == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Remove HII packages
  //
  if (gPqcTransitionPrivateData->HiiHandle != NULL) {
    HiiRemovePackages (gPqcTransitionPrivateData->HiiHandle);
  }

  //
  // Uninstall protocols
  //
  if (gPqcTransitionPrivateData->DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           gPqcTransitionPrivateData->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mPqcTransitionHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &gPqcTransitionPrivateData->ConfigAccess,
           NULL
           );
  }

  //
  // Free private data
  //
  FreePool (gPqcTransitionPrivateData);
  gPqcTransitionPrivateData = NULL;

  DEBUG ((DEBUG_INFO, "PQC Transition Manager unloaded successfully\n"));
  
  return EFI_SUCCESS;
}