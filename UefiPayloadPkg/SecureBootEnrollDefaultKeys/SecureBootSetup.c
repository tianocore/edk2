/** @file
   Enroll default PK, KEK, DB and DBX

   Copyright (C) 2014, Red Hat, Inc.

   This program and the accompanying materials are licensed and made available
   under the terms and conditions of the BSD License which accompanies this
   distribution. The full text of the license may be found at
   http://opensource.org/licenses/bsd-license.

   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
   WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 **/

#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/GlobalVariable.h>
#include <Guid/ImageAuthentication.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#define EFI_MICROSOFT_KEK_CERT_GUID \
  { 0xA23665E3, 0xACA6, 0x4F6D, {0x80, 0xCC, 0x34, 0x1E, 0x7D, 0x7B, 0x8C, 0xC6} }

#define EFI_SECUREBOOT_PK_CERT_GUID \
  { 0xF8104268, 0xA364, 0x45F5, {0x8E, 0x00, 0xAB, 0xA3, 0xFD, 0xEA, 0x12, 0xBE} }

#define EFI_MICROSOFT_DB1_CERT_GUID \
  { 0x26A517B0, 0xE3FD, 0x46C2, {0x89, 0x32, 0xE9, 0x26, 0xBF, 0x98, 0x94, 0x1F} }

#define EFI_MICROSOFT_DB2_CERT_GUID \
  { 0x91D2E32B, 0x0134, 0x4306, {0xBA, 0x90, 0x54, 0xED, 0xCB, 0xF3, 0x49, 0xCA} }

#define EFI_MICROSOFT_DBX_GUID \
  { 0x74BB6E72, 0x2A56, 0x4D0E, {0xA5, 0xB3, 0x5D, 0x39, 0xFC, 0x2E, 0xE3, 0x46} }

#define EFI_MICROSOFT_OWNER_GUID \
  { 0x77FA9ABD, 0x0359, 0x4D32, {0xBD, 0x60, 0x28, 0xF4, 0xE7, 0x8F, 0x78, 0x4B} }

EFI_GUID gEfiSecureBootDb1CertGuid = EFI_MICROSOFT_DB1_CERT_GUID;
EFI_GUID gEfiSecureBootDb2CertGuid = EFI_MICROSOFT_DB2_CERT_GUID;
EFI_GUID gEfiSecureBootDbxCrlGuid = EFI_MICROSOFT_DBX_GUID;
EFI_GUID gEfiSecureBootKekCertGuid = EFI_MICROSOFT_KEK_CERT_GUID;
EFI_GUID gEfiSecureBootPkCertGuid = EFI_SECUREBOOT_PK_CERT_GUID;
EFI_GUID gEfiMicrosoftOwnerGuid = EFI_MICROSOFT_OWNER_GUID;

//
// The most important thing about the variable payload is that it is a list of
// lists, where the element size of any given *inner* list is constant.
//
// Since X509 certificates vary in size, each of our *inner* lists will contain
// one element only (one X.509 certificate). This is explicitly mentioned in
// the UEFI specification, in "28.4.1 Signature Database", in a Note.
//
// The list structure looks as follows:
//
// struct EFI_VARIABLE_AUTHENTICATION_2 {                           |
//   struct EFI_TIME {                                              |
//     UINT16 Year;                                                 |
//     UINT8  Month;                                                |
//     UINT8  Day;                                                  |
//     UINT8  Hour;                                                 |
//     UINT8  Minute;                                               |
//     UINT8  Second;                                               |
//     UINT8  Pad1;                                                 |
//     UINT32 Nanosecond;                                           |
//     INT16  TimeZone;                                             |
//     UINT8  Daylight;                                             |
//     UINT8  Pad2;                                                 |
//   } TimeStamp;                                                   |
//                                                                  |
//   struct WIN_CERTIFICATE_UEFI_GUID {                           | |
//     struct WIN_CERTIFICATE {                                   | |
//       UINT32 dwLength; ----------------------------------------+ |
//       UINT16 wRevision;                                        | |
//       UINT16 wCertificateType;                                 | |
//     } Hdr;                                                     | +- DataSize
//                                                                | |
//     EFI_GUID CertType;                                         | |
//     UINT8    CertData[1] = { <--- "struct hack"                | |
//       struct EFI_SIGNATURE_LIST {                            | | |
//         EFI_GUID SignatureType;                              | | |
//         UINT32   SignatureListSize; -------------------------+ | |
//         UINT32   SignatureHeaderSize;                        | | |
//         UINT32   SignatureSize; ---------------------------+ | | |
//         UINT8    SignatureHeader[SignatureHeaderSize];     | | | |
//                                                            v | | |
//         struct EFI_SIGNATURE_DATA {                        | | | |
//           EFI_GUID SignatureOwner;                         | | | |
//           UINT8    SignatureData[1] = { <--- "struct hack" | | | |
//             X.509 payload                                  | | | |
//           }                                                | | | |
//         } Signatures[];                                      | | |
//       } SigLists[];                                            | |
//     };                                                         | |
//   } AuthInfo;                                                  | |
// };                                                               |
//
// Given that the "struct hack" invokes undefined behavior (which is why C99
// introduced the flexible array member), and because subtracting those pesky
// sizes of 1 is annoying, and because the format is fully specified in the
// UEFI specification, we'll introduce two matching convenience structures that
// are customized for our X.509 purposes.
//

#pragma pack(1)
typedef struct {
  EFI_TIME TimeStamp;

  //
  // dwLength covers data below
  //
  UINT32 dwLength;
  UINT16 wRevision;
  UINT16 wCertificateType;
  EFI_GUID CertType;
} SINGLE_HEADER;

typedef struct {
  //
  // SignatureListSize covers data below
  //
  EFI_GUID SignatureType;
  UINT32 SignatureListSize;
  UINT32 SignatureHeaderSize; // constant 0
  UINT32 SignatureSize;

  //
  // SignatureSize covers data below
  //
  EFI_GUID SignatureOwner;

  //
  // X.509 certificate follows
  //
} REPEATING_HEADER;
#pragma pack()

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
  UINTN DataSize;
  SINGLE_HEADER    *SingleHeader;
  REPEATING_HEADER *RepeatingHeader;
  VA_LIST Marker;
  CONST UINT8      *Cert;
  EFI_STATUS Status;
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
    UINTN CertSize;

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
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: Invalid certificate parameters\n"));
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
    DEBUG ((EFI_D_INFO, "SecureBootSetup: GetTime failed\n"));
    // Fill in dummy values
    SingleHeader->TimeStamp.Year       = 2018;
    SingleHeader->TimeStamp.Month      = 1;
    SingleHeader->TimeStamp.Day        = 1;
    SingleHeader->TimeStamp.Hour       = 0;
    SingleHeader->TimeStamp.Minute     = 0;
    SingleHeader->TimeStamp.Second     = 0;
    Status = EFI_SUCCESS;
  }
  SingleHeader->TimeStamp.Pad1       = 0;
  SingleHeader->TimeStamp.Nanosecond = 0;
  SingleHeader->TimeStamp.TimeZone   = 0;
  SingleHeader->TimeStamp.Daylight   = 0;
  SingleHeader->TimeStamp.Pad2       = 0;

  //
  // This looks like a bug in edk2. According to the UEFI specification,
  // dwLength is "The length of the entire certificate, including the length of
  // the header, in bytes". That shouldn't stop right after CertType -- it
  // should include everything below it.
  //
  SingleHeader->dwLength         = sizeof *SingleHeader - sizeof SingleHeader->TimeStamp;
  SingleHeader->wRevision        = 0x0200;
  SingleHeader->wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyGuid (&SingleHeader->CertType, &gEfiCertPkcs7Guid);
  Position += sizeof *SingleHeader;

  VA_START (Marker, CertType);
  for (Cert = VA_ARG (Marker, CONST UINT8 *);
       Cert != NULL;
       Cert = VA_ARG (Marker, CONST UINT8 *)) {
    UINTN CertSize;
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
            EFI_VARIABLE_RUNTIME_ACCESS |
            EFI_VARIABLE_BOOTSERVICE_ACCESS |
            EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
           DataSize, Data);

  FreePool (Data);

Out:
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: %a(\"%s\", %g): %r\n", __FUNCTION__, VariableName,
      VendorGuid, Status));
  }
  return Status;
}


STATIC
EFI_STATUS
EFIAPI
GetExact (
  IN CHAR16   *VariableName,
  IN EFI_GUID *VendorGuid,
  OUT VOID    *Data,
  IN UINTN DataSize,
  IN BOOLEAN AllowMissing
  )
{
  UINTN Size;
  EFI_STATUS Status;

  Size = DataSize;
  Status = gRT->GetVariable (VariableName, VendorGuid, NULL, &Size, Data);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND && AllowMissing) {
      ZeroMem (Data, DataSize);
      return EFI_SUCCESS;
    }

    DEBUG ((EFI_D_ERROR, "SecureBootSetup: GetVariable(\"%s\", %g): %r\n", VariableName,
      VendorGuid, Status));
    return Status;
  }

  if (Size != DataSize) {
    DEBUG ((EFI_D_INFO, "SecureBootSetup: GetVariable(\"%s\", %g): expected size 0x%Lx, "
      "got 0x%Lx\n", VariableName, VendorGuid, (UINT64)DataSize, (UINT64)Size));
    return EFI_PROTOCOL_ERROR;
  }

  return EFI_SUCCESS;
}

typedef struct {
  UINT8 SetupMode;
  UINT8 SecureBoot;
  UINT8 SecureBootEnable;
  UINT8 CustomMode;
  UINT8 VendorKeys;
} SETTINGS;

STATIC
EFI_STATUS
EFIAPI
GetSettings (
  OUT SETTINGS *Settings,
  BOOLEAN AllowMissing
  )
{
  EFI_STATUS Status;

  ZeroMem (Settings, sizeof(SETTINGS));

  Status = GetExact (EFI_SETUP_MODE_NAME, &gEfiGlobalVariableGuid,
         &Settings->SetupMode, sizeof Settings->SetupMode, AllowMissing);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_SECURE_BOOT_MODE_NAME, &gEfiGlobalVariableGuid,
         &Settings->SecureBoot, sizeof Settings->SecureBoot, AllowMissing);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_SECURE_BOOT_ENABLE_NAME,
         &gEfiSecureBootEnableDisableGuid, &Settings->SecureBootEnable,
         sizeof Settings->SecureBootEnable, AllowMissing);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid,
         &Settings->CustomMode, sizeof Settings->CustomMode, AllowMissing);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_VENDOR_KEYS_VARIABLE_NAME, &gEfiGlobalVariableGuid,
         &Settings->VendorKeys, sizeof Settings->VendorKeys, AllowMissing);
  return Status;
}

STATIC
VOID
EFIAPI
PrintSettings (
  IN CONST SETTINGS *Settings
  )
{
  DEBUG ((EFI_D_INFO, "SecureBootSetup: SetupMode=%d SecureBoot=%d SecureBootEnable=%d "
    "CustomMode=%d VendorKeys=%d\n", Settings->SetupMode, Settings->SecureBoot,
    Settings->SecureBootEnable, Settings->CustomMode, Settings->VendorKeys));
}

/**
  Install SecureBoot certificates once the VariableDriver is running.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
InstallSecureBootHook (
  IN EFI_EVENT                      Event,
  IN VOID                           *Context
  )
{
  EFI_STATUS  Status;
  VOID        *Protocol;
  SETTINGS Settings;

  UINT8 *MicrosoftPCA = 0;
  UINTN MicrosoftPCASize;
  UINT8 *MicrosoftUefiCA = 0;
  UINTN MicrosoftUefiCASize;
  UINT8 *MicrosoftKEK = 0;
  UINTN MicrosoftKEKSize;
  UINT8 *SecureBootPk = 0;
  UINTN SecureBootPkSize;
  UINT8 *MicrosoftDbx = 0;
  UINTN MicrosoftDbxSize;

  Status = gBS->LocateProtocol (&gEfiVariableWriteArchProtocolGuid, NULL, (VOID **)&Protocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = GetSettings (&Settings, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: Failed to get current settings\n"));
    return;
  }

  if (Settings.SetupMode != SETUP_MODE) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: already in User Mode\n"));
    return;
  }

  if (Settings.SecureBootEnable != SECURE_BOOT_MODE_ENABLE) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: SecureBootEnable is disabled.\n"));
    return;
  }

  PrintSettings (&Settings);

  if (Settings.CustomMode != CUSTOM_SECURE_BOOT_MODE) {
    Settings.CustomMode = CUSTOM_SECURE_BOOT_MODE;
    Status = gRT->SetVariable (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid,
             (EFI_VARIABLE_NON_VOLATILE |
              EFI_VARIABLE_BOOTSERVICE_ACCESS),
             sizeof Settings.CustomMode, &Settings.CustomMode);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "SecureBootSetup: SetVariable(\"%s\", %g): %r\n", EFI_CUSTOM_MODE_NAME,
        &gEfiCustomModeEnableGuid, Status));
      ASSERT_EFI_ERROR (Status);
    }
  }

  Status = GetSectionFromAnyFv(&gEfiSecureBootDb1CertGuid, EFI_SECTION_RAW, 0, (void **)&MicrosoftPCA, &MicrosoftPCASize);
  ASSERT_EFI_ERROR (Status);

  Status = GetSectionFromAnyFv(&gEfiSecureBootDb2CertGuid, EFI_SECTION_RAW, 0, (void **)&MicrosoftUefiCA, &MicrosoftUefiCASize);
  ASSERT_EFI_ERROR (Status);

  Status = GetSectionFromAnyFv(&gEfiSecureBootKekCertGuid, EFI_SECTION_RAW, 0, (void **)&MicrosoftKEK, &MicrosoftKEKSize);
  ASSERT_EFI_ERROR (Status);

  Status = GetSectionFromAnyFv(&gEfiSecureBootPkCertGuid, EFI_SECTION_RAW, 0, (void **)&SecureBootPk, &SecureBootPkSize);
  ASSERT_EFI_ERROR (Status);

  Status = GetSectionFromAnyFv(&gEfiSecureBootDbxCrlGuid, EFI_SECTION_RAW, 0, (void **)&MicrosoftDbx, &MicrosoftDbxSize);
  ASSERT_EFI_ERROR (Status);

  Status = gRT->SetVariable (EFI_IMAGE_SECURITY_DATABASE1, &gEfiImageSecurityDatabaseGuid,
           (EFI_VARIABLE_NON_VOLATILE |
            EFI_VARIABLE_BOOTSERVICE_ACCESS |
            EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
           MicrosoftDbxSize, MicrosoftDbx);
  ASSERT_EFI_ERROR (Status);

  Status = EnrollListOfCerts (
    EFI_IMAGE_SECURITY_DATABASE,
    &gEfiImageSecurityDatabaseGuid,
    &gEfiCertX509Guid,
    MicrosoftPCA,    MicrosoftPCASize,    &gEfiMicrosoftOwnerGuid,
    MicrosoftUefiCA, MicrosoftUefiCASize, &gEfiMicrosoftOwnerGuid,
    NULL);
  ASSERT_EFI_ERROR (Status);

  Status = EnrollListOfCerts (
    EFI_KEY_EXCHANGE_KEY_NAME,
    &gEfiGlobalVariableGuid,
    &gEfiCertX509Guid,
    SecureBootPk, SecureBootPkSize, &gEfiCallerIdGuid,
    MicrosoftKEK, MicrosoftKEKSize, &gEfiMicrosoftOwnerGuid,
    NULL);
  ASSERT_EFI_ERROR (Status);

  Status = EnrollListOfCerts (
    EFI_PLATFORM_KEY_NAME,
    &gEfiGlobalVariableGuid,
    &gEfiCertX509Guid,
    SecureBootPk, SecureBootPkSize, &gEfiGlobalVariableGuid,
    NULL);
  ASSERT_EFI_ERROR (Status);

  FreePool (MicrosoftPCA);
  FreePool (MicrosoftUefiCA);
  FreePool (MicrosoftKEK);
  FreePool (SecureBootPk);
  FreePool (MicrosoftDbx);

  Settings.CustomMode = STANDARD_SECURE_BOOT_MODE;
  Status = gRT->SetVariable (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid,
           EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
           sizeof Settings.CustomMode, &Settings.CustomMode);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: SetVariable(\"%s\", %g): %r\n", EFI_CUSTOM_MODE_NAME,
      &gEfiCustomModeEnableGuid, Status));
    ASSERT_EFI_ERROR (Status);
  }

  // FIXME: Force SecureBoot to ON. The AuthService will do this if authenticated variables
  // are supported, which aren't as the SMM handler isn't able to verify them.

  Settings.SecureBootEnable = SECURE_BOOT_ENABLE;
  Status = gRT->SetVariable (EFI_SECURE_BOOT_ENABLE_NAME, &gEfiSecureBootEnableDisableGuid,
           EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
           sizeof Settings.SecureBootEnable, &Settings.SecureBootEnable);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: SetVariable(\"%s\", %g): %r\n", EFI_SECURE_BOOT_ENABLE_NAME,
      &gEfiSecureBootEnableDisableGuid, Status));
    ASSERT_EFI_ERROR (Status);
  }

  Settings.SecureBoot = SECURE_BOOT_ENABLE;
  Status = gRT->SetVariable (EFI_SECURE_BOOT_MODE_NAME, &gEfiGlobalVariableGuid,
           EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
           sizeof Settings.SecureBoot, &Settings.SecureBoot);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: SetVariable(\"%s\", %g): %r\n", EFI_SECURE_BOOT_MODE_NAME,
      &gEfiGlobalVariableGuid, Status));
    ASSERT_EFI_ERROR (Status);
  }

  Status = GetSettings (&Settings, FALSE);
  ASSERT_EFI_ERROR (Status);

  //
  // Final sanity check:
  //
  //                                 [SetupMode]
  //                        (read-only, standardized by UEFI)
  //                                /                \_
  //                               0               1, default
  //                              /                    \_
  //                      PK enrolled                   no PK enrolled yet,
  //              (this is called "User Mode")          PK enrollment possible
  //                             |
  //                             |
  //                     [SecureBootEnable]
  //         (read-write, edk2-specific, boot service only)
  //                /                           \_
  //               0                         1, default
  //              /                               \_
  //       [SecureBoot]=0                     [SecureBoot]=1
  // (read-only, standardized by UEFI)  (read-only, standardized by UEFI)
  //     images are not verified         images are verified, platform is
  //                                      operating in Secure Boot mode
  //                                                 |
  //                                                 |
  //                                           [CustomMode]
  //                          (read-write, edk2-specific, boot service only)
  //                                /                           \_
  //                          0, default                         1
  //                              /                               \_
  //                      PK, KEK, db, dbx                PK, KEK, db, dbx
  //                    updates are verified          updates are not verified
  //

  PrintSettings (&Settings);

  if (Settings.SetupMode != 0 || Settings.SecureBoot != 1 ||
      Settings.SecureBootEnable != 1 || Settings.CustomMode != 0 ||
      Settings.VendorKeys != 0) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: disabled\n"));
    return;
  }

  DEBUG ((EFI_D_INFO, "SecureBootSetup: SecureBoot enabled\n"));
}

EFI_STATUS
EFIAPI
DriverEntry (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE            *SystemTable
  )
{
  EFI_STATUS Status;

  VOID *TcgProtocol;
  VOID *Registration;

  Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID **) &TcgProtocol);
  if (!EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SecureBootSetup: Started too late."
      "TPM is already running!\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // Create event callback, because we need access variable on SecureBootPolicyVariable
  // We should use VariableWriteArch instead of VariableArch, because Variable driver
  // may update SecureBoot value based on last setting.
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiVariableWriteArchProtocolGuid,
    TPL_CALLBACK,
    InstallSecureBootHook,
    NULL,
    &Registration);

  return EFI_SUCCESS;
}
