/** @file
  Sealed Capsules DXE driver implementing CAPSULE_TRANSFORMATION_PROTOCOL.

  Caution: This module requires additional review when modified.
  This module will have external input - capsule image.
  This external input must be validated carefully to avoid security issues like
  buffer overflow, integer overflow.

  Copyright (c) 2026, 3mdeb Sp. z o.o. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>

#include <Guid/FmpCapsule.h>
#include <Guid/ImageAuthentication.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FmpAuthenticationLib.h>
#include <Library/FmpPayloadLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/CapsuleTransformation.h>
#include <Protocol/FirmwareManagement.h>

//
// A sealed capsule is an FMP capsule that acts as a container for the real FMP
// capsule.  The two-level structure is meant to reuse as much as possible of
// the existing functionality but address the fact that FMP signs each payload
// individually instead of a capsule as a whole.  Signing a capsule in its
// entirety avoids the need to use a separate mechanism to authenticate embedded
// drivers as well as prevents repacking the capsule to modify its unsigned
// parts in any way.
//
// The outer capsule (container) can have no embedded drivers, no dependencies
// and only one payload.  Its target GUID is ignored.  The only function it has
// is authenticating its payload which is the real (inner) capsule.  For flags
// of outer and inner capsules must match as capsule dispatching depends on
// them.
//
// Following a successful authentication the outer capsule is essentially
// discarded and the inner one is processed in its place.  This process is
// transparent for the capsule processing mechanism which initially cares only
// if an outer capsule looks like an FMP capsule and then sees the inner
// capsule.
//
// Description of a sealed capsule in terms of structures and their properties:
//
//    Outer/top capsule
//    |
//    |-- EFI_CAPSULE_HEADER
//    |   |-- GUID: FMP capsule
//    |   `-- flags: same as for the inner/nested capsule
//    |
//    `-- EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER
//        |-- embedded drivers: none
//        |-- dependencies: none
//        `-- payloads (exactly one):
//            |-- EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER
//            |-- EFI_FIRMWARE_IMAGE_AUTHENTICATION
//            |   `-- signed with root key embedded into BIOS
//            |-- FMP_PAYLOAD_HEADER
//            |   `-- fields aren't meaningful beyond size of payload's data
//            `-- Inner/nested capsule
//
//    Inner/nested capsule
//    |
//    |-- EFI_CAPSULE_HEADER
//    |   |-- GUID: FMP capsule
//    |   `-- flags: same as for the outer/top capsule
//    |
//    `-- EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER
//        |-- embedded drivers: any
//        |-- dependencies: any
//        `-- payloads (any, showing just one):
//            |-- EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER
//            |-- EFI_FIRMWARE_IMAGE_AUTHENTICATION
//            |   `-- signed with a test key to satisfy FmpDxe
//            |-- FMP_PAYLOAD_HEADER
//            |   `-- fields describe system firmware to be updated
//            `-- Firmware update image
//

/**
  Checks if a GUID is an FMP capsule GUID or not.

  @param[in] CapsuleGuid  A pointer to EFI_GUID.

  @retval TRUE   It is an FMP capsule GUID.
  @retval FALSE  It is not an FMP capsule GUID.
**/
STATIC
BOOLEAN
IsFmpCapsuleGuid (
  IN CONST EFI_GUID  *CapsuleGuid
  )
{
  return CompareGuid (&gEfiFmpCapsuleGuid, CapsuleGuid);
}

/**
  Validate capsule header for basic sanity.

  @param[in]  CapsuleHeader  Points to a capsule header.
  @param[in]  CapsuleSize    Size of the whole capsule image.

  @retval TRUE   It is a valid capsule header.
  @retval FALSE  It is not a valid capsule header.
**/
STATIC
BOOLEAN
IsValidCapsuleHeader (
  IN CONST EFI_CAPSULE_HEADER  *CapsuleHeader,
  IN UINT64                    CapsuleSize
  )
{
  return (CapsuleHeader->CapsuleImageSize == CapsuleSize) &&
         (CapsuleHeader->HeaderSize < CapsuleHeader->CapsuleImageSize);
}

/**
  Verify the signature of an outer FMP capsule image.

  @param[in]  ImageHeader  Points to the image header of the outer FMP capsule.

  @retval EFI_SUCCESS             Signature is valid.
  @retval EFI_UNSUPPORTED         Signature is using an unexpected format.
  @retval EFI_ABORTED             Root key is not available.
  @retval EFI_SECURITY_VIOLATION  Signature doesn't correspond to any known root key.
**/
STATIC
EFI_STATUS
CheckFmpImageSignature (
  IN EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader
  )
{
  EFI_FIRMWARE_IMAGE_AUTHENTICATION  *ImageAuthenticationHeader;
  GUID                               *CertType;
  UINT8                              *PublicKeyDataXdr;
  UINT8                              *PublicKeyDataXdrEnd;
  UINTN                              PublicKeyDataLength;
  UINTN                              Index;
  EFI_STATUS                         Status;

  PublicKeyDataXdr    = PcdGetPtr (PcdFmpDevicePkcs7CertBufferXdr);
  PublicKeyDataXdrEnd = PublicKeyDataXdr + PcdGetSize (PcdFmpDevicePkcs7CertBufferXdr);

  if ((PublicKeyDataXdr == NULL) || (PublicKeyDataXdr == PublicKeyDataXdrEnd)) {
    DEBUG ((DEBUG_ERROR, "%a(): invalid PKCS7 XDR, aborting.\n", __func__));
    return EFI_ABORTED;
  }

  ImageAuthenticationHeader = (EFI_FIRMWARE_IMAGE_AUTHENTICATION *)&ImageHeader[1];

  CertType = &ImageAuthenticationHeader->AuthInfo.CertType;
  if (!CompareGuid (&gEfiCertPkcs7Guid, CertType)) {
    DEBUG ((DEBUG_INFO, "%a(): unexpected certificate type: %g\n", __func__, CertType));
    return EFI_UNSUPPORTED;
  }

  //
  // Structure of PcdFmpDevicePkcs7CertBufferXdr:
  //  - an entry:
  //    + length of a public key as 32-bit big-endian
  //    + a public key
  //    + padding to 4-byte boundary (unnecessary for the last element)
  //  - other entries follow
  //
  for (Index = 0; PublicKeyDataXdr < PublicKeyDataXdrEnd; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "%a(): certificate #%d [%p..%p].\n",
      __func__,
      Index,
      PublicKeyDataXdr,
      PublicKeyDataXdrEnd
      ));

    if (PublicKeyDataXdr + sizeof (UINT32) > PublicKeyDataXdrEnd) {
      DEBUG ((DEBUG_ERROR, "%a(): certificate size extends beyond end of PCD, skipping it.\n", __func__));
      return EFI_ABORTED;
    }

    PublicKeyDataLength = SwapBytes32 (*(UINT32 *)PublicKeyDataXdr);
    PublicKeyDataXdr   += sizeof (UINT32);
    if (PublicKeyDataXdr + PublicKeyDataLength > PublicKeyDataXdrEnd) {
      DEBUG ((DEBUG_ERROR, "%a(): certificate extends beyond end of PCD, skipping it.\n", __func__));
      return EFI_ABORTED;
    }

    Status = AuthenticateFmpImage (
               ImageAuthenticationHeader,
               ImageHeader->UpdateImageSize,
               PublicKeyDataXdr,
               PublicKeyDataLength
               );
    if (!EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }

    PublicKeyDataXdr += PublicKeyDataLength;
    PublicKeyDataXdr  = (UINT8 *)ALIGN_POINTER (PublicKeyDataXdr, sizeof (UINT32));
  }

  return EFI_SECURITY_VIOLATION;
}

/**
  Optionally transform a capsule out of an outer one.

  Caution: This function may receive untrusted input.

  This function validates the fields in EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER
  and EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER.

  This function checks if the payload is an FMP capsule.

  @param[in]      This                 A pointer to a CAPSULE_TRANSFORMATION_PROTOCOL.
  @param[in, out] CapsuleHeader        A pointer to a capsule header.
                                       On input points to the original capsule header.
                                       On output points to the transformed capsule header.

  @retval EFI_SUCCESS             The capsule has been transformed successfully.
  @retval EFI_INVALID_PARAMETER   An input pointer is NULL or some capsule check has failed.
  @retval EFI_UNSUPPORTED         The outer capsule is not an FMP capsule, contains an embedded
                                  driver or multiple payloads, uses old payload format, is not
                                  signed, or uses an unexpected signature format.
  @retval EFI_SECURITY_VIOLATION  The inner capsule is not authentic.
  @retval Others                  Statuses returned by AuthenticateFmpImage().
**/
STATIC
EFI_STATUS
EFIAPI
CapsuleTransformationImpl (
  IN     CAPSULE_TRANSFORMATION_PROTOCOL  *This,
  IN OUT EFI_CAPSULE_HEADER               **CapsuleHeader
  )
{
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER        *OuterFmpHeader;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *OuterImageHeader;
  EFI_FIRMWARE_IMAGE_AUTHENTICATION             *OuterAuthenticationHeader;
  UINTN                                         OuterAuthDataSize;
  UINTN                                         StartOfPayload;
  UINT64                                        *OuterOffsetList;
  EFI_STATUS                                    Status;
  EFI_CAPSULE_HEADER                            *InnerCapsuleHeader;
  EFI_CAPSULE_HEADER                            *OuterCapsuleHeader;
  UINTN                                         InnerCapsuleSize;
  UINTN                                         OuterCapsulePayloadSize;

  if ((CapsuleHeader == NULL) || (*CapsuleHeader == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a(): input pointer is NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  OuterCapsuleHeader = *CapsuleHeader;
  if (OuterCapsuleHeader->HeaderSize >= OuterCapsuleHeader->CapsuleImageSize) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): outer capsule's header is too large (0x%lx >= 0x%lx).\n",
      __func__,
      OuterCapsuleHeader->HeaderSize,
      OuterCapsuleHeader->CapsuleImageSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (!IsFmpCapsuleGuid (&OuterCapsuleHeader->CapsuleGuid)) {
    DEBUG ((DEBUG_ERROR, "%a(): outer capsule is not an FMP capsule.\n", __func__));
    return EFI_UNSUPPORTED;
  }

  OuterFmpHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *)((UINT8 *)OuterCapsuleHeader + OuterCapsuleHeader->HeaderSize);
  if (OuterFmpHeader->PayloadItemCount != 1) {
    DEBUG ((DEBUG_ERROR, "%a(): multiple payloads in the outer capsule.\n", __func__));
    return EFI_UNSUPPORTED;
  }

  if (OuterFmpHeader->EmbeddedDriverCount != 0) {
    DEBUG ((DEBUG_ERROR, "%a(): embedded driver inside the outer capsule.\n", __func__));
    return EFI_UNSUPPORTED;
  }

  OuterOffsetList  = (UINT64 *)&OuterFmpHeader[1];
  OuterImageHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)OuterFmpHeader + OuterOffsetList[0]);

  if (OuterImageHeader->Version < EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): outer capsule version must be at least %d, got %d.\n",
      __func__,
      EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION,
      OuterImageHeader->Version
      ));
    return EFI_UNSUPPORTED;
  }

  if (OuterImageHeader->ImageCapsuleSupport != CAPSULE_SUPPORT_AUTHENTICATION) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): outer capsule must support auth but not dependencies, got 0x%x.\n",
      __func__,
      OuterImageHeader->ImageCapsuleSupport
      ));
    return EFI_UNSUPPORTED;
  }

  Status = CheckFmpImageSignature (OuterImageHeader);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): the capsule failed to authenticate: %r.\n", __func__, Status));
    return Status;
  }

  OuterAuthenticationHeader = (EFI_FIRMWARE_IMAGE_AUTHENTICATION *)&OuterImageHeader[1];

  OuterAuthDataSize = OuterAuthenticationHeader->AuthInfo.Hdr.dwLength + sizeof (OuterAuthenticationHeader->MonotonicCount);

  StartOfPayload = (UINTN)OuterAuthenticationHeader + OuterAuthDataSize;
  if ((StartOfPayload < (UINTN)OuterAuthenticationHeader) ||
      (StartOfPayload >= (UINTN)OuterAuthenticationHeader + OuterImageHeader->UpdateImageSize))
  {
    DEBUG ((DEBUG_ERROR, "%a(): pointer overflow on handling payload image.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (OuterImageHeader->UpdateImageSize < OuterAuthDataSize) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): outer image is shorter than authentication data: 0x%x < 0x%lx.\n",
      __func__,
      OuterImageHeader->UpdateImageSize,
      OuterAuthDataSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  OuterCapsulePayloadSize = OuterImageHeader->UpdateImageSize - OuterAuthDataSize;

  Status = FmpPayloadGetData (
             (VOID *)StartOfPayload,
             OuterCapsulePayloadSize,
             (VOID **)&InnerCapsuleHeader,
             &InnerCapsuleSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): failed to extract FMP data: %r.\n", __func__, Status));
    return Status;
  }

  if (InnerCapsuleSize < sizeof (EFI_CAPSULE_HEADER)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a(): outer payload is smaller than a capsule header (0x%lx < 0x%lx).\n",
      __func__,
      InnerCapsuleSize,
      sizeof (EFI_CAPSULE_HEADER)
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (!IsValidCapsuleHeader (InnerCapsuleHeader, InnerCapsuleSize)) {
    DEBUG ((DEBUG_ERROR, "%a(): outer payload is not a capsule.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (!IsFmpCapsuleGuid (&InnerCapsuleHeader->CapsuleGuid)) {
    DEBUG ((DEBUG_ERROR, "%a(): nested capsule is not an FMP capsule.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((OuterCapsuleHeader->HeaderSize != InnerCapsuleHeader->HeaderSize) ||
      (OuterCapsuleHeader->Flags != InnerCapsuleHeader->Flags))
  {
    DEBUG ((DEBUG_ERROR, "%a(): parameters of outer and nested capsules don't match!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *CapsuleHeader = InnerCapsuleHeader;

  return EFI_SUCCESS;
}

STATIC CAPSULE_TRANSFORMATION_PROTOCOL  mCapsuleTransformationProtocol = {
  CapsuleTransformationImpl,
};

/**
  Driver entry point.  Installs CAPSULE_TRANSFORMATION_PROTOCOL.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  Protocol was installed successfully.
  @retval Others       Protocol installation failed.
**/
EFI_STATUS
EFIAPI
EntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gCapsuleTransformationProtocolGuid,
                &mCapsuleTransformationProtocol,
                NULL
                );
}
