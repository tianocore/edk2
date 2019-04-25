/** @file
  Enroll default PK, KEK, db, dbx.

  Copyright (C) 2014-2019, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Guid/AuthenticatedVariableFormat.h>    // gEfiCustomModeEnableGuid
#include <Guid/GlobalVariable.h>                 // EFI_SETUP_MODE_NAME
#include <Guid/ImageAuthentication.h>            // EFI_IMAGE_SECURITY_DATABASE
#include <Guid/MicrosoftVendor.h>                // gMicrosoftVendorGuid
#include <Library/BaseMemoryLib.h>               // CopyGuid()
#include <Library/DebugLib.h>                    // ASSERT()
#include <Library/MemoryAllocationLib.h>         // FreePool()
#include <Library/ShellCEntryLib.h>              // ShellAppMain()
#include <Library/UefiLib.h>                     // AsciiPrint()
#include <Library/UefiRuntimeServicesTableLib.h> // gRT

#include "EnrollDefaultKeys.h"

/**
  Enroll a set of certificates in a global variable, overwriting it.

  The variable will be rewritten with NV+BS+RT+AT attributes.

  @param[in] VariableName  The name of the variable to overwrite.

  @param[in] VendorGuid    The namespace (ie. vendor GUID) of the variable to
                           overwrite.

  @param[in] CertType      The GUID determining the type of all the
                           certificates in the set that is passed in. For
                           example, gEfiCertX509Guid stands for DER-encoded
                           X.509 certificates, while gEfiCertSha256Guid stands
                           for SHA256 image hashes.

  @param[in] ...           A list of

                             IN CONST UINT8    *Cert,
                             IN UINTN          CertSize,
                             IN CONST EFI_GUID *OwnerGuid

                           triplets. If the first component of a triplet is
                           NULL, then the other two components are not
                           accessed, and processing is terminated. The list of
                           certificates is enrolled in the variable specified,
                           overwriting it. The OwnerGuid component identifies
                           the agent installing the certificate.

  @retval EFI_INVALID_PARAMETER  The triplet list is empty (ie. the first Cert
                                 value is NULL), or one of the CertSize values
                                 is 0, or one of the CertSize values would
                                 overflow the accumulated UINT32 data size.

  @retval EFI_OUT_OF_RESOURCES   Out of memory while formatting variable
                                 payload.

  @retval EFI_SUCCESS            Enrollment successful; the variable has been
                                 overwritten (or created).

  @return                        Error codes from gRT->GetTime() and
                                 gRT->SetVariable().
**/
STATIC
EFI_STATUS
EFIAPI
EnrollListOfCerts (
  IN CHAR16   *VariableName,
  IN EFI_GUID *VendorGuid,
  IN EFI_GUID *CertType,
  ...
  )
{
  UINTN            DataSize;
  SINGLE_HEADER    *SingleHeader;
  REPEATING_HEADER *RepeatingHeader;
  VA_LIST          Marker;
  CONST UINT8      *Cert;
  EFI_STATUS       Status;
  UINT8            *Data;
  UINT8            *Position;

  Status = EFI_SUCCESS;

  //
  // compute total size first, for UINT32 range check, and allocation
  //
  DataSize = sizeof *SingleHeader;
  VA_START (Marker, CertType);
  for (Cert = VA_ARG (Marker, CONST UINT8 *);
       Cert != NULL;
       Cert = VA_ARG (Marker, CONST UINT8 *)) {
    UINTN          CertSize;

    CertSize = VA_ARG (Marker, UINTN);
    (VOID)VA_ARG (Marker, CONST EFI_GUID *);

    if (CertSize == 0 ||
        CertSize > MAX_UINT32 - sizeof *RepeatingHeader ||
        DataSize > MAX_UINT32 - sizeof *RepeatingHeader - CertSize) {
      Status = EFI_INVALID_PARAMETER;
      break;
    }
    DataSize += sizeof *RepeatingHeader + CertSize;
  }
  VA_END (Marker);

  if (DataSize == sizeof *SingleHeader) {
    Status = EFI_INVALID_PARAMETER;
  }
  if (EFI_ERROR (Status)) {
    goto Out;
  }

  Data = AllocatePool (DataSize);
  if (Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Out;
  }

  Position = Data;

  SingleHeader = (SINGLE_HEADER *)Position;
  Status = gRT->GetTime (&SingleHeader->TimeStamp, NULL);
  if (EFI_ERROR (Status)) {
    goto FreeData;
  }
  SingleHeader->TimeStamp.Pad1       = 0;
  SingleHeader->TimeStamp.Nanosecond = 0;
  SingleHeader->TimeStamp.TimeZone   = 0;
  SingleHeader->TimeStamp.Daylight   = 0;
  SingleHeader->TimeStamp.Pad2       = 0;
#if 0
  SingleHeader->dwLength         = DataSize - sizeof SingleHeader->TimeStamp;
#else
  //
  // This looks like a bug in edk2. According to the UEFI specification,
  // dwLength is "The length of the entire certificate, including the length of
  // the header, in bytes". That shouldn't stop right after CertType -- it
  // should include everything below it.
  //
  SingleHeader->dwLength         = sizeof *SingleHeader
                                     - sizeof SingleHeader->TimeStamp;
#endif
  SingleHeader->wRevision        = 0x0200;
  SingleHeader->wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyGuid (&SingleHeader->CertType, &gEfiCertPkcs7Guid);
  Position += sizeof *SingleHeader;

  VA_START (Marker, CertType);
  for (Cert = VA_ARG (Marker, CONST UINT8 *);
       Cert != NULL;
       Cert = VA_ARG (Marker, CONST UINT8 *)) {
    UINTN            CertSize;
    CONST EFI_GUID   *OwnerGuid;

    CertSize  = VA_ARG (Marker, UINTN);
    OwnerGuid = VA_ARG (Marker, CONST EFI_GUID *);

    RepeatingHeader = (REPEATING_HEADER *)Position;
    CopyGuid (&RepeatingHeader->SignatureType, CertType);
    RepeatingHeader->SignatureListSize   =
      (UINT32)(sizeof *RepeatingHeader + CertSize);
    RepeatingHeader->SignatureHeaderSize = 0;
    RepeatingHeader->SignatureSize       =
      (UINT32)(sizeof RepeatingHeader->SignatureOwner + CertSize);
    CopyGuid (&RepeatingHeader->SignatureOwner, OwnerGuid);
    Position += sizeof *RepeatingHeader;

    CopyMem (Position, Cert, CertSize);
    Position += CertSize;
  }
  VA_END (Marker);

  ASSERT (Data + DataSize == Position);

  Status = gRT->SetVariable (VariableName, VendorGuid,
                  (EFI_VARIABLE_NON_VOLATILE |
                   EFI_VARIABLE_BOOTSERVICE_ACCESS |
                   EFI_VARIABLE_RUNTIME_ACCESS |
                   EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
                  DataSize, Data);

FreeData:
  FreePool (Data);

Out:
  if (EFI_ERROR (Status)) {
    AsciiPrint ("error: %a(\"%s\", %g): %r\n", __FUNCTION__, VariableName,
      VendorGuid, Status);
  }
  return Status;
}


STATIC
EFI_STATUS
GetExact (
  IN CHAR16   *VariableName,
  IN EFI_GUID *VendorGuid,
  OUT VOID    *Data,
  IN UINTN    DataSize,
  IN BOOLEAN  AllowMissing
  )
{
  UINTN      Size;
  EFI_STATUS Status;

  Size = DataSize;
  Status = gRT->GetVariable (VariableName, VendorGuid, NULL, &Size, Data);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND && AllowMissing) {
      ZeroMem (Data, DataSize);
      return EFI_SUCCESS;
    }

    AsciiPrint ("error: GetVariable(\"%s\", %g): %r\n", VariableName,
      VendorGuid, Status);
    return Status;
  }

  if (Size != DataSize) {
    AsciiPrint ("error: GetVariable(\"%s\", %g): expected size 0x%Lx, "
      "got 0x%Lx\n", VariableName, VendorGuid, (UINT64)DataSize, (UINT64)Size);
    return EFI_PROTOCOL_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetSettings (
  OUT SETTINGS *Settings
  )
{
  EFI_STATUS Status;

  Status = GetExact (EFI_SETUP_MODE_NAME, &gEfiGlobalVariableGuid,
             &Settings->SetupMode, sizeof Settings->SetupMode, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_SECURE_BOOT_MODE_NAME, &gEfiGlobalVariableGuid,
             &Settings->SecureBoot, sizeof Settings->SecureBoot, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_SECURE_BOOT_ENABLE_NAME,
             &gEfiSecureBootEnableDisableGuid, &Settings->SecureBootEnable,
             sizeof Settings->SecureBootEnable, TRUE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid,
             &Settings->CustomMode, sizeof Settings->CustomMode, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_VENDOR_KEYS_VARIABLE_NAME, &gEfiGlobalVariableGuid,
             &Settings->VendorKeys, sizeof Settings->VendorKeys, FALSE);
  return Status;
}

STATIC
VOID
PrintSettings (
  IN CONST SETTINGS *Settings
  )
{
  AsciiPrint ("info: SetupMode=%d SecureBoot=%d SecureBootEnable=%d "
    "CustomMode=%d VendorKeys=%d\n", Settings->SetupMode, Settings->SecureBoot,
    Settings->SecureBootEnable, Settings->CustomMode, Settings->VendorKeys);
}


INTN
EFIAPI
ShellAppMain (
  IN UINTN  Argc,
  IN CHAR16 **Argv
  )
{
  EFI_STATUS Status;
  SETTINGS   Settings;

  Status = GetSettings (&Settings);
  if (EFI_ERROR (Status)) {
    return 1;
  }
  PrintSettings (&Settings);

  if (Settings.SetupMode != 1) {
    AsciiPrint ("error: already in User Mode\n");
    return 1;
  }

  if (Settings.CustomMode != CUSTOM_SECURE_BOOT_MODE) {
    Settings.CustomMode = CUSTOM_SECURE_BOOT_MODE;
    Status = gRT->SetVariable (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid,
                    (EFI_VARIABLE_NON_VOLATILE |
                     EFI_VARIABLE_BOOTSERVICE_ACCESS),
                    sizeof Settings.CustomMode, &Settings.CustomMode);
    if (EFI_ERROR (Status)) {
      AsciiPrint ("error: SetVariable(\"%s\", %g): %r\n", EFI_CUSTOM_MODE_NAME,
        &gEfiCustomModeEnableGuid, Status);
      return 1;
    }
  }

  Status = EnrollListOfCerts (
             EFI_IMAGE_SECURITY_DATABASE,
             &gEfiImageSecurityDatabaseGuid,
             &gEfiCertX509Guid,
             mMicrosoftPca,    mSizeOfMicrosoftPca,    &gMicrosoftVendorGuid,
             mMicrosoftUefiCa, mSizeOfMicrosoftUefiCa, &gMicrosoftVendorGuid,
             NULL);
  if (EFI_ERROR (Status)) {
    return 1;
  }

  Status = EnrollListOfCerts (
             EFI_IMAGE_SECURITY_DATABASE1,
             &gEfiImageSecurityDatabaseGuid,
             &gEfiCertSha256Guid,
             mSha256OfDevNull, mSizeOfSha256OfDevNull, &gEfiCallerIdGuid,
             NULL);
  if (EFI_ERROR (Status)) {
    return 1;
  }

  Status = EnrollListOfCerts (
             EFI_KEY_EXCHANGE_KEY_NAME,
             &gEfiGlobalVariableGuid,
             &gEfiCertX509Guid,
             mRedHatPkKek1, mSizeOfRedHatPkKek1, &gEfiCallerIdGuid,
             mMicrosoftKek, mSizeOfMicrosoftKek, &gMicrosoftVendorGuid,
             NULL);
  if (EFI_ERROR (Status)) {
    return 1;
  }

  Status = EnrollListOfCerts (
             EFI_PLATFORM_KEY_NAME,
             &gEfiGlobalVariableGuid,
             &gEfiCertX509Guid,
             mRedHatPkKek1, mSizeOfRedHatPkKek1, &gEfiGlobalVariableGuid,
             NULL);
  if (EFI_ERROR (Status)) {
    return 1;
  }

  Settings.CustomMode = STANDARD_SECURE_BOOT_MODE;
  Status = gRT->SetVariable (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof Settings.CustomMode, &Settings.CustomMode);
  if (EFI_ERROR (Status)) {
    AsciiPrint ("error: SetVariable(\"%s\", %g): %r\n", EFI_CUSTOM_MODE_NAME,
      &gEfiCustomModeEnableGuid, Status);
    return 1;
  }

  Status = GetSettings (&Settings);
  if (EFI_ERROR (Status)) {
    return 1;
  }
  PrintSettings (&Settings);

  if (Settings.SetupMode != 0 || Settings.SecureBoot != 1 ||
      Settings.SecureBootEnable != 1 || Settings.CustomMode != 0 ||
      Settings.VendorKeys != 0) {
    AsciiPrint ("error: unexpected\n");
    return 1;
  }

  AsciiPrint ("info: success\n");
  return 0;
}
