/** @file
  Enroll secure boot variables passed in via IGVM

  Copyright (C) 2025, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/AuthenticatedVariableFormat.h>
#include <IndustryStandard/IgvmData.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC EFI_IGVM_DATA_HOB  *mPK;
STATIC EFI_IGVM_DATA_HOB  *mKEK;
STATIC EFI_IGVM_DATA_HOB  *mDb;
STATIC EFI_IGVM_DATA_HOB  *mDbx;

STATIC
EFI_STATUS
IgvmSecureBootFindBlobs (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *Hob;
  EFI_IGVM_DATA_HOB  *IgvmData;
  CHAR16             *Name;

  for (Hob = GetFirstGuidHob (&gEfiIgvmDataHobGuid);
       Hob != NULL;
       Hob = GetNextGuidHob (&gEfiIgvmDataHobGuid, GET_NEXT_HOB (Hob)))
  {
    IgvmData = (VOID *)(Hob + 1);

    switch (IgvmData->DataType) {
      case 0x100:
        Name = L"PK";
        mPK  = IgvmData;
        break;
      case 0x101:
        Name = L"KEK";
        mKEK = IgvmData;
        break;
      case 0x102:
        Name = L"db";
        mDb  = IgvmData;
        break;
      case 0x103:
        Name = L"dbx";
        mDbx = IgvmData;
        break;
      default:
        Name = NULL;
        break;
    }

    if (Name) {
      DEBUG ((
        DEBUG_INFO,
        "%a: address=0x%lx length=0x%lx type=0x%x name=%s\n",
        __func__,
        IgvmData->Address,
        IgvmData->Length,
        IgvmData->DataType,
        Name
        ));
    }
  }

  if (!mPK) {
    DEBUG ((DEBUG_INFO, "%a: missing: PK\n", __func__));
    return EFI_NOT_FOUND;
  }

  if (!mKEK) {
    DEBUG ((DEBUG_INFO, "%a: missing: KEK\n", __func__));
    return EFI_NOT_FOUND;
  }

  if (!mDb) {
    DEBUG ((DEBUG_INFO, "%a: missing: db\n", __func__));
    return EFI_NOT_FOUND;
  }

  if (!mDbx) {
    DEBUG ((DEBUG_INFO, "%a: missing: dbx\n", __func__));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
IgvmSecureBootSetVariable (
  IN CHAR16          *Name,
  IN EFI_GUID        *Guid,
  EFI_IGVM_DATA_HOB  *Hob
  )
{
  EFI_STATUS  Status;
  UINT64      Length     = Hob->Length;
  VOID        *Data      = (VOID *)(UINTN)Hob->Address;
  UINT32      Attributes = (EFI_VARIABLE_NON_VOLATILE |
                            EFI_VARIABLE_BOOTSERVICE_ACCESS |
                            EFI_VARIABLE_RUNTIME_ACCESS |
                            EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);

  Status = gRT->SetVariable (Name, Guid, Attributes, Length, Data);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: set %s: %r\n", __func__, Name, Status));
  }

  return Status;
}

EFI_STATUS
EFIAPI
IgvmSecureBootCustomMode (
  UINT8  Value
  )
{
  EFI_STATUS  Status;

  Status = gRT->SetVariable (
                  EFI_CUSTOM_MODE_NAME,
                  &gEfiCustomModeEnableGuid,
                  (EFI_VARIABLE_NON_VOLATILE |
                   EFI_VARIABLE_BOOTSERVICE_ACCESS),
                  sizeof (Value),
                  &Value
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a CustomMode: %r\n",
      __func__,
      Value ? "enable" : "disable",
      Status
      ));
  }

  return Status;
}

BOOLEAN
EFIAPI
IgvmSecureBootIsConfigured (
  VOID
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Result;
  UINTN       Size;

  Status = gRT->GetVariable (
                  L"PK",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &Size,
                  NULL
                  );

  if (Status == EFI_NOT_FOUND) {
    Result = FALSE;
  } else {
    Result = TRUE;
  }

  DEBUG ((DEBUG_INFO, "%a: Result=%a\n", __func__, Result ? "Yes" : "No"));
  return Result;
}

EFI_STATUS
EFIAPI
IgvmSecureBootDxeEntrypoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT64      CcAttr;

  CcAttr = PcdGet64 (PcdConfidentialComputingGuestAttr);

  if (!CcAttr) {
    /*
     * The design idea is that the pages containing the HOBs and the EFI
     * security databases are part of the launch measurement of a confidential
     * VM.  This way the launch measurement will not only prove the guest runs
     * the firmware code it is supposed to run, but also prove secure boot is
     * configured the way it is supposed to be.
     *
     * Any code loaded by the firmware will be subject to standard secure boot
     * verification:
     *  - Option one is to use code signing and add the x509 vertificates to
     *    'db' (and revocation list to 'dbx'), which is the typical setup in
     *    non-confidential VMs.
     *  - More strict option two is to only allow specific efi binaries by
     *    adding the sha256 authenticode hashes to 'db'.
     *
     * This workflow can only work in confidential VMs, so error out otherwise.
     */
    DEBUG ((DEBUG_ERROR, "%a: ERROR: not running in Confidential VM\n", __func__));
    return EFI_UNSUPPORTED;
  }

  if (IgvmSecureBootIsConfigured ()) {
    DEBUG ((DEBUG_ERROR, "%a: secure boot is already configured.\n", __func__));
    return EFI_ALREADY_STARTED;
  }

  Status = IgvmSecureBootFindBlobs ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IgvmSecureBootSetVariable (L"dbx", &gEfiImageSecurityDatabaseGuid, mDbx);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IgvmSecureBootSetVariable (L"db", &gEfiImageSecurityDatabaseGuid, mDb);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IgvmSecureBootSetVariable (L"KEK", &gEfiGlobalVariableGuid, mKEK);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // allow setting a platform key (PK) which is not self-signed
  Status = IgvmSecureBootCustomMode (TRUE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IgvmSecureBootSetVariable (L"PK", &gEfiGlobalVariableGuid, mPK);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IgvmSecureBootCustomMode (FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: secure boot setup complete\n", __func__));
  return EFI_REQUEST_UNLOAD_IMAGE;
}
