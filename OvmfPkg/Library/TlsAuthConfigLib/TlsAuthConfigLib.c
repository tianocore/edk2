/** @file

  A hook-in library for NetworkPkg/TlsAuthConfigDxe, in order to set volatile
  variables related to TLS configuration, before TlsAuthConfigDxe or HttpDxe
  (which is a UEFI_DRIVER) consume them.

  Copyright (C) 2013, 2015, 2018, Red Hat, Inc.
  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Guid/HttpTlsCipherList.h>
#include <Guid/TlsAuthentication.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

/**
  Read the list of trusted CA certificates from the fw_cfg file
  "etc/edk2/https/cacerts", and store it to
  gEfiTlsCaCertificateGuid:EFI_TLS_CA_CERTIFICATE_VARIABLE.

  The contents are validated (for well-formedness) by NetworkPkg/HttpDxe.
**/
STATIC
VOID
SetCaCerts (
  VOID
  )
{
  EFI_STATUS            Status;
  FIRMWARE_CONFIG_ITEM  HttpsCaCertsItem;
  UINTN                 HttpsCaCertsSize;
  VOID                  *HttpsCaCerts;

  Status = QemuFwCfgFindFile (
             "etc/edk2/https/cacerts",
             &HttpsCaCertsItem,
             &HttpsCaCertsSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a:%a: not touching CA cert list\n",
      gEfiCallerBaseName,
      __func__
      ));
    return;
  }

  //
  // Delete the current EFI_TLS_CA_CERTIFICATE_VARIABLE if it exists. This
  // serves two purposes:
  //
  // (a) If the variable exists with EFI_VARIABLE_NON_VOLATILE attribute, we
  //     cannot make it volatile without deleting it first.
  //
  // (b) If we fail to recreate the variable later, deleting the current one is
  //     still justified if the fw_cfg file exists. Emptying the set of trusted
  //     CA certificates will fail HTTPS boot, which is better than trusting
  //     any certificate that's possibly missing from the fw_cfg file.
  //
  Status = gRT->SetVariable (
                  EFI_TLS_CA_CERTIFICATE_VARIABLE, // VariableName
                  &gEfiTlsCaCertificateGuid,       // VendorGuid
                  0,                               // Attributes
                  0,                               // DataSize
                  NULL                             // Data
                  );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    //
    // This is fatal.
    //
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: failed to delete %g:\"%s\"\n",
      gEfiCallerBaseName,
      __func__,
      &gEfiTlsCaCertificateGuid,
      EFI_TLS_CA_CERTIFICATE_VARIABLE
      ));
    ASSERT_EFI_ERROR (Status);
    CpuDeadLoop ();
  }

  if (HttpsCaCertsSize == 0) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a:%a: applied empty CA cert list\n",
      gEfiCallerBaseName,
      __func__
      ));
    return;
  }

  HttpsCaCerts = AllocatePool (HttpsCaCertsSize);
  if (HttpsCaCerts == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: failed to allocate HttpsCaCerts\n",
      gEfiCallerBaseName,
      __func__
      ));
    return;
  }

  QemuFwCfgSelectItem (HttpsCaCertsItem);
  QemuFwCfgReadBytes (HttpsCaCertsSize, HttpsCaCerts);

  Status = gRT->SetVariable (
                  EFI_TLS_CA_CERTIFICATE_VARIABLE, // VariableName
                  &gEfiTlsCaCertificateGuid,       // VendorGuid
                  EFI_VARIABLE_BOOTSERVICE_ACCESS, // Attributes
                  HttpsCaCertsSize,                // DataSize
                  HttpsCaCerts                     // Data
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: failed to set %g:\"%s\": %r\n",
      gEfiCallerBaseName,
      __func__,
      &gEfiTlsCaCertificateGuid,
      EFI_TLS_CA_CERTIFICATE_VARIABLE,
      Status
      ));
    goto FreeHttpsCaCerts;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: stored CA cert list (%Lu byte(s))\n",
    gEfiCallerBaseName,
    __func__,
    (UINT64)HttpsCaCertsSize
    ));

FreeHttpsCaCerts:
  FreePool (HttpsCaCerts);
}

/**
  Read the list of trusted cipher suites from the fw_cfg file
  "etc/edk2/https/ciphers", and store it to
  gEdkiiHttpTlsCipherListGuid:EDKII_HTTP_TLS_CIPHER_LIST_VARIABLE.

  The contents are propagated by NetworkPkg/HttpDxe to NetworkPkg/TlsDxe; the
  list is processed by the latter.
**/
STATIC
VOID
SetCipherSuites (
  VOID
  )
{
  EFI_STATUS            Status;
  FIRMWARE_CONFIG_ITEM  HttpsCiphersItem;
  UINTN                 HttpsCiphersSize;
  VOID                  *HttpsCiphers;

  Status = QemuFwCfgFindFile (
             "etc/edk2/https/ciphers",
             &HttpsCiphersItem,
             &HttpsCiphersSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a:%a: not touching cipher suites\n",
      gEfiCallerBaseName,
      __func__
      ));
    return;
  }

  //
  // From this point on, any failure is fatal. An ordered cipher preference
  // list is available from QEMU, thus we cannot let the firmware attempt HTTPS
  // boot with either pre-existent or non-existent preferences. An empty set of
  // cipher suites does not fail HTTPS boot automatically; the default cipher
  // suite preferences would take effect, and we must prevent that.
  //
  // Delete the current EDKII_HTTP_TLS_CIPHER_LIST_VARIABLE if it exists. If
  // the variable exists with EFI_VARIABLE_NON_VOLATILE attribute, we cannot
  // make it volatile without deleting it first.
  //
  Status = gRT->SetVariable (
                  EDKII_HTTP_TLS_CIPHER_LIST_VARIABLE, // VariableName
                  &gEdkiiHttpTlsCipherListGuid,        // VendorGuid
                  0,                                   // Attributes
                  0,                                   // DataSize
                  NULL                                 // Data
                  );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: failed to delete %g:\"%s\"\n",
      gEfiCallerBaseName,
      __func__,
      &gEdkiiHttpTlsCipherListGuid,
      EDKII_HTTP_TLS_CIPHER_LIST_VARIABLE
      ));
    goto Done;
  }

  if (HttpsCiphersSize == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: list of cipher suites must not be empty\n",
      gEfiCallerBaseName,
      __func__
      ));
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  HttpsCiphers = AllocatePool (HttpsCiphersSize);
  if (HttpsCiphers == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: failed to allocate HttpsCiphers\n",
      gEfiCallerBaseName,
      __func__
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  QemuFwCfgSelectItem (HttpsCiphersItem);
  QemuFwCfgReadBytes (HttpsCiphersSize, HttpsCiphers);

  Status = gRT->SetVariable (
                  EDKII_HTTP_TLS_CIPHER_LIST_VARIABLE, // VariableName
                  &gEdkiiHttpTlsCipherListGuid,        // VendorGuid
                  EFI_VARIABLE_BOOTSERVICE_ACCESS,     // Attributes
                  HttpsCiphersSize,                    // DataSize
                  HttpsCiphers                         // Data
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: failed to set %g:\"%s\"\n",
      gEfiCallerBaseName,
      __func__,
      &gEdkiiHttpTlsCipherListGuid,
      EDKII_HTTP_TLS_CIPHER_LIST_VARIABLE
      ));
    goto FreeHttpsCiphers;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a: stored list of cipher suites (%Lu byte(s))\n",
    gEfiCallerBaseName,
    __func__,
    (UINT64)HttpsCiphersSize
    ));

FreeHttpsCiphers:
  FreePool (HttpsCiphers);

Done:
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    CpuDeadLoop ();
  }
}

RETURN_STATUS
EFIAPI
TlsAuthConfigInit (
  VOID
  )
{
  SetCaCerts ();
  SetCipherSuites ();

  return RETURN_SUCCESS;
}
