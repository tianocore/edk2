/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPDM_SECURITY_LIB_INTERNAL_H_
#define SPDM_SECURITY_LIB_INTERNAL_H_

#include <Uefi.h>
#include <hal/base.h>
#include <Stub/SpdmLibStub.h>
#include <industry_standard/spdm.h>
#include <industry_standard/spdm_secured_message.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Tpm20.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/TpmMeasurementLib.h>
#include <Library/RngLib.h>
#include <Library/BaseCryptLib.h>
#include <library/spdm_requester_lib.h>

#include <Guid/DeviceAuthentication.h>
#include <Guid/ImageAuthentication.h>

#include <Protocol/PciIo.h>
#include <Library/SpdmSecurityLib.h>
#include "library/spdm_crypt_lib.h"

#define SPDM_DEVICE_CONTEXT_SIGNATURE  SIGNATURE_32 ('S', 'P', 'D', 'C')

typedef struct {
  UINT32                      Signature;
  // UEFI Context
  EDKII_DEVICE_IDENTIFIER     DeviceId;
  BOOLEAN                     IsEmbeddedDevice;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  VOID                        *DeviceIo;
  UINT64                      DeviceUID;
  // SPDM Context
  UINTN                       SpdmContextSize;
  VOID                        *SpdmContext;
  UINTN                       ScratchBufferSize;
  VOID                        *ScratchBuffer;
  UINT8                       SpdmVersion;
  VOID                        *SpdmIoProtocol;
  EFI_SIGNATURE_LIST          *SignatureList;
  UINTN                       SignatureListSize;
} SPDM_DEVICE_CONTEXT;

typedef struct {
  UINTN                  Signature;
  LIST_ENTRY             Link;
  SPDM_DEVICE_CONTEXT    *SpdmDeviceContext;
} SPDM_DEVICE_CONTEXT_INSTANCE;

#define SPDM_DEVICE_CONTEXT_INSTANCE_SIGNATURE  SIGNATURE_32 ('S', 'D', 'C', 'S')
#define SPDM_DEVICE_CONTEXT_INSTANCE_FROM_LINK(a)  CR (a, SPDM_DEVICE_CONTEXT_INSTANCE, Link, SPDM_DEVICE_CONTEXT_INSTANCE_SIGNATURE)

VOID *
EFIAPI
GetSpdmIoProtocolViaSpdmContext (
  IN VOID  *SpdmContext
  );

/**
  This function creates the spdm device context and init connection to the
  responder with the device info.

  @param[in]  SpdmDeviceInfo        A pointer to device info.
  @param[out] SecurityState         A pointer to the security state of the requester.

  @return the spdm device conext after the init connection succeeds.

**/
SPDM_DEVICE_CONTEXT *
EFIAPI
CreateSpdmDeviceContext (
  IN  EDKII_SPDM_DEVICE_INFO       *SpdmDeviceInfo,
  OUT EDKII_DEVICE_SECURITY_STATE  *SecurityState
  );

VOID
EFIAPI
DestroySpdmDeviceContext (
  IN SPDM_DEVICE_CONTEXT  *SpdmDeviceContext
  );

/**
  This function returns the SPDM device type for TCG SPDM event.

  @param[in]  SpdmDeviceContext             The SPDM context for the device.

  @return TCG SPDM device type
**/
UINT32
EFIAPI
GetSpdmDeviceType (
  IN  SPDM_DEVICE_CONTEXT  *SpdmDeviceContext
  );

/**
  This function returns the SPDM device measurement context size for TCG SPDM event.

  @param[in]  SpdmDeviceContext             The SPDM context for the device.

  @return TCG SPDM device measurement context size
**/
UINTN
EFIAPI
GetDeviceMeasurementContextSize (
  IN  SPDM_DEVICE_CONTEXT  *SpdmDeviceContext
  );

/**
  This function creates the SPDM device measurement context for TCG SPDM event.

  @param[in]       SpdmDeviceContext       The SPDM context for the device.
  @param[in, OUT]  DeviceContext           The TCG SPDM device measurement context.
  @param[in]       DeviceContextSize       The size of TCG SPDM device measurement context.

  @retval EFI_SUCCESS      The TCG SPDM device measurement context is returned.
  @retval EFI_UNSUPPORTED  The TCG SPDM device measurement context is unsupported.
**/
EFI_STATUS
EFIAPI
CreateDeviceMeasurementContext (
  IN  SPDM_DEVICE_CONTEXT  *SpdmDeviceContext,
  IN OUT VOID              *DeviceContext,
  IN UINTN                 DeviceContextSize
  );

/**
  Extend Certicate and auth state to NV Index and measure trust anchor to PCR.

  @param[in]  SpdmDeviceContext          The SPDM context for the device.
  @param[in]  AuthState                  The auth state of this deice.
  @param[in]  CertChainSize              The size of cert chain.
  @param[in]  CertChain                  A pointer to a destination buffer to store the certificate chain.
  @param[in]  TrustAnchor                A buffer to hold the trust_anchor which is used to validate the peer
                                         certificate, if not NULL.
  @param[in]  TrustAnchorSize            A buffer to hold the trust_anchor_size, if not NULL..
  @param[in]  SlotId                     The number of slot for the certificate chain.
  @param[out]  SecurityState             A pointer to the security state of the requester.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
ExtendCertificate (
  IN  SPDM_DEVICE_CONTEXT          *SpdmDeviceContext,
  IN UINT8                         AuthState,
  IN UINTN                         CertChainSize,
  IN UINT8                         *CertChain,
  IN VOID                          *TrustAnchor,
  IN UINTN                         TrustAnchorSize,
  IN UINT8                         SlotId,
  OUT EDKII_DEVICE_SECURITY_STATE  *SecurityState
  );

/**
  This function executes SPDM measurement and extend to TPM.

  @param[in]  SpdmDeviceContext            The SPDM context for the device.
**/
EFI_STATUS
EFIAPI
DoDeviceMeasurement (
  IN  SPDM_DEVICE_CONTEXT          *SpdmDeviceContext,
  IN  UINT8                        SlotId,
  OUT EDKII_DEVICE_SECURITY_STATE  *SecurityState
  );

/**
  This function gets SPDM digest and certificates.

  @param[in]  SpdmDeviceContext           The SPDM context for the device.
  @param[out]  AuthState                  The auth state of the devices.
  @param[out]  ValidSlotId                The number of slot for the certificate chain.
  @param[out]  SecurityState              The security state of the requester.
  @param[out]  IsValidCertChain           The validity of the certificate chain.
  @param[out]  RootCertMatch              The authority of the certificate chain.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
DoDeviceCertificate (
  IN  SPDM_DEVICE_CONTEXT          *SpdmDeviceContext,
  OUT UINT8                        *AuthState,
  OUT UINT8                        *ValidSlotId,
  OUT EDKII_DEVICE_SECURITY_STATE  *SecurityState,
  OUT BOOLEAN                      *IsValidCertChain,
  OUT BOOLEAN                      *RootCertMatch
  );

/**
  This function does authentication.

  @param[in]  SpdmDeviceContext           The SPDM context for the device.
  @param[out]  AuthState                  The auth state of the devices.
  @param[in]  ValidSlotId                 The number of slot for the certificate chain.
  @param[out]  SecurityState              The security state of the requester.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
DoDeviceAuthentication (
  IN  SPDM_DEVICE_CONTEXT          *SpdmDeviceContext,
  OUT UINT8                        *AuthState,
  IN  UINT8                        ValidSlotId,
  IN  BOOLEAN                      IsValidCertChain,
  IN  BOOLEAN                      RootCertMatch,
  OUT EDKII_DEVICE_SECURITY_STATE  *SecurityState
  );

/**
 * This function dump raw data.
 *
 * @param  data  raw data
 * @param  size  raw data size
 **/
VOID
EFIAPI
InternalDumpData (
  CONST UINT8  *Data,
  UINTN        Size
  );

#endif
