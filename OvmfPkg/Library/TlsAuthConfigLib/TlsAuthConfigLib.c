/** @file

  A hook-in library for NetworkPkg/TlsAuthConfigDxe, in order to set volatile
  variables related to TLS configuration, before TlsAuthConfigDxe or HttpDxe
  (which is a UEFI_DRIVER) consume them.

  Copyright (C) 2013, 2015, 2018, Red Hat, Inc.
  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

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
  EFI_STATUS           Status;
  FIRMWARE_CONFIG_ITEM HttpsCaCertsItem;
  UINTN                HttpsCaCertsSize;
  VOID                 *HttpsCaCerts;

  Status = QemuFwCfgFindFile ("etc/edk2/https/cacerts", &HttpsCaCertsItem,
             &HttpsCaCertsSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "%a:%a: not touching CA cert list\n",
      gEfiCallerBaseName, __FUNCTION__));
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
    DEBUG ((DEBUG_ERROR, "%a:%a: failed to delete %g:\"%s\"\n",
      gEfiCallerBaseName, __FUNCTION__, &gEfiTlsCaCertificateGuid,
      EFI_TLS_CA_CERTIFICATE_VARIABLE));
    ASSERT_EFI_ERROR (Status);
    CpuDeadLoop ();
  }

  if (HttpsCaCertsSize == 0) {
    DEBUG ((DEBUG_VERBOSE, "%a:%a: applied empty CA cert list\n",
      gEfiCallerBaseName, __FUNCTION__));
    return;
  }

  HttpsCaCerts = AllocatePool (HttpsCaCertsSize);
  if (HttpsCaCerts == NULL) {
    DEBUG ((DEBUG_ERROR, "%a:%a: failed to allocate HttpsCaCerts\n",
      gEfiCallerBaseName, __FUNCTION__));
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
    DEBUG ((DEBUG_ERROR, "%a:%a: failed to set %g:\"%s\": %r\n",
      gEfiCallerBaseName, __FUNCTION__, &gEfiTlsCaCertificateGuid,
      EFI_TLS_CA_CERTIFICATE_VARIABLE, Status));
    goto FreeHttpsCaCerts;
  }

  DEBUG ((DEBUG_VERBOSE, "%a:%a: stored CA cert list (%Lu byte(s))\n",
    gEfiCallerBaseName, __FUNCTION__, (UINT64)HttpsCaCertsSize));

FreeHttpsCaCerts:
  FreePool (HttpsCaCerts);
}

RETURN_STATUS
EFIAPI
TlsAuthConfigInit (
  VOID
  )
{
  SetCaCerts ();

  return RETURN_SUCCESS;
}
