/** @file
  Header file for Post-Quantum Cryptography (PQC) Transition Manager.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PQC_TRANSITION_DXE_H__
#define __PQC_TRANSITION_DXE_H__

#include <Uefi.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <Protocol/Tls.h>
#include <Protocol/FirmwareManagement.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/SecureBootVariableLib.h>
#include <Library/PlatformSecureLib.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/GlobalVariable.h>

#include "PqcTransitionNvData.h"

//
// Tool generated IFR binary data and String package data
//
extern UINT8 PqcTransitionVfrBin[];
extern UINT8 PqcTransitionDxeStrings[];

//
// PQC Transition Form Set GUID
//
#define PQC_TRANSITION_CONFIG_FORM_SET_GUID \
  { \
    0x8F4B8F4B, 0x8F4B, 0x4F4B, { 0x8F, 0x4B, 0x8F, 0x4B, 0x8F, 0x4B, 0x8F, 0x4B } \
  }

extern EFI_GUID gPqcTransitionConfigFormSetGuid;

//
// PQC Algorithm Support Status
//
typedef enum {
  PqcAlgorithmNotSupported = 0,
  PqcAlgorithmSupported,
  PqcAlgorithmActive
} PQC_ALGORITHM_STATUS;

//
// PQC Readiness Check Results
//
typedef struct {
  BOOLEAN  PkHasPqcCert;           // PK contains PQC certificate
  BOOLEAN  KekHasPqcCert;          // KEK contains PQC certificate  
  BOOLEAN  DbHasPqcCert;           // DB contains PQC certificate
  BOOLEAN  OromIsPqcSigned;        // Option ROM is PQC signed
  BOOLEAN  LoaderIsPqcSigned;      // OS Loader is PQC signed
  BOOLEAN  TlsHasPqcSupport;       // TLS/HTTPS boot has PQC support
  BOOLEAN  FwUpdateHasPqcSupport;  // Firmware update has PQC support
  BOOLEAN  SystemReadyForPqc;      // Overall system readiness
} PQC_READINESS_STATUS;

//
// PQC Transition Mode
//
typedef enum {
  PqcTransitionModeDisabled = 0,   // Traditional algorithms only
  PqcTransitionModeHybrid,         // Both traditional and PQC allowed
  PqcTransitionModePqcOnly         // PQC algorithms only
} PQC_TRANSITION_MODE;

//
// PQC Transition Private Data Structure
//
#define PQC_TRANSITION_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('P', 'Q', 'C', 'T')

typedef struct {
  UINTN                             Signature;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  EFI_HII_HANDLE                    HiiHandle;
  EFI_HANDLE                        DriverHandle;
  
  PQC_TRANSITION_CONFIGURATION      Configuration;
  PQC_READINESS_STATUS              ReadinessStatus;
  
  CHAR16                            *VariableName;
  EFI_GUID                          *VendorGuid;
} PQC_TRANSITION_PRIVATE_DATA;

#define PQC_TRANSITION_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, PQC_TRANSITION_PRIVATE_DATA, ConfigAccess, PQC_TRANSITION_PRIVATE_DATA_SIGNATURE)

//
// HII Vendor Device Path
//
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

//
// Function Prototypes
//

/**
  Check if the system is ready for PQC-only mode transition.

  @param[out] ReadinessStatus    Pointer to store the readiness check results.

  @retval EFI_SUCCESS           Readiness check completed successfully.
  @retval EFI_INVALID_PARAMETER ReadinessStatus is NULL.
  @retval Others                Error occurred during readiness check.
**/
EFI_STATUS
CheckPqcReadiness (
  OUT PQC_READINESS_STATUS  *ReadinessStatus
  );

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
  );

/**
  Check if Option ROMs are PQC-signed.

  @retval TRUE                  Option ROMs are PQC-signed.
  @retval FALSE                 Option ROMs are not PQC-signed or not found.
**/
BOOLEAN
CheckOptionRomPqcSignatures (
  VOID
  );

/**
  Check if OS Loaders are PQC-signed.

  @retval TRUE                  OS Loaders are PQC-signed.
  @retval FALSE                 OS Loaders are not PQC-signed or not found.
**/
BOOLEAN
CheckOsLoaderPqcSignatures (
  VOID
  );

/**
  Check if TLS/HTTPS boot supports PQC algorithms.

  @retval TRUE                  TLS/HTTPS boot supports PQC.
  @retval FALSE                 TLS/HTTPS boot does not support PQC.
**/
BOOLEAN
CheckTlsHttpsPqcSupport (
  VOID
  );

/**
  Check if firmware update mechanism supports PQC algorithms.

  @retval TRUE                  Firmware update supports PQC.
  @retval FALSE                 Firmware update does not support PQC.
**/
BOOLEAN
CheckFirmwareUpdatePqcSupport (
  VOID
  );

/**
  Perform PQC transition mode switch.

  @param[in] NewMode            The new PQC transition mode to switch to.

  @retval EFI_SUCCESS           Mode switch completed successfully.
  @retval EFI_ACCESS_DENIED     System is not ready for the requested mode.
  @retval Others                Error occurred during mode switch.
**/
EFI_STATUS
SwitchPqcTransitionMode (
  IN PQC_TRANSITION_MODE  NewMode
  );

/**
  Update secure boot policy based on PQC transition mode.

  @param[in] Mode               The PQC transition mode.

  @retval EFI_SUCCESS           Policy updated successfully.
  @retval Others                Error occurred during policy update.
**/
EFI_STATUS
UpdateSecureBootPolicy (
  IN PQC_TRANSITION_MODE  Mode
  );

/**
  Log PQC transition event for audit purposes.

  @param[in] Mode               The new PQC transition mode.
**/
VOID
LogPqcTransition (
  IN PQC_TRANSITION_MODE  Mode
  );

/**
  Clean up traditional algorithms from signature databases.

  @retval EFI_SUCCESS           Cleanup completed successfully.
  @retval Others                Error occurred during cleanup.
**/
EFI_STATUS
CleanupTraditionalAlgorithms (
  VOID
  );

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
  );

/**
  HII Config Access Protocol ExtractConfig function.

  @param[in]  This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Request           A null-terminated Unicode string in <ConfigRequest> format.
  @param[out] Progress          On return, points to a character in the Request string.
  @param[out] Results           A null-terminated Unicode string in <ConfigAltResp> format.

  @retval EFI_SUCCESS           The Results is filled with the requested values.
  @retval Others                Error occurred.
**/
EFI_STATUS
EFIAPI
PqcTransitionExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );

/**
  HII Config Access Protocol RouteConfig function.

  @param[in]  This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration     A null-terminated Unicode string in <ConfigResp> format.
  @param[out] Progress          A pointer to a string filled in with the offset of
                                the most recent '&' before the first failing
                                name/value pair.

  @retval EFI_SUCCESS           The Results is processed successfully.
  @retval Others                Error occurred.
**/
EFI_STATUS
EFIAPI
PqcTransitionRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  );

/**
  HII Config Access Protocol Callback function.

  @param[in]  This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action            Specifies the type of action taken by the browser.
  @param[in]  QuestionId        A unique value which is sent to the original
                                exporting driver so that it can identify the type
                                of data to expect.
  @param[in]  Type              The type of value for the question.
  @param[in]  Value             A pointer to the data being sent to the original
                                exporting driver.
  @param[out] ActionRequest     On return, points to the action requested by the
                                callback function.

  @retval EFI_SUCCESS           The callback successfully handled the action.
  @retval Others                Error occurred.
**/
EFI_STATUS
EFIAPI
PqcTransitionCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );

#endif // __PQC_TRANSITION_DXE_H__