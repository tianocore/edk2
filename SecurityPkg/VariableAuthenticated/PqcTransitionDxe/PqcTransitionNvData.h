/** @file
  Header file for NV data structure definition for PQC Transition Configuration.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PQC_TRANSITION_NV_DATA_H__
#define __PQC_TRANSITION_NV_DATA_H__

#include <Guid/HiiPlatformSetupFormset.h>

//
// PQC Transition Configuration Form IDs
//
#define PQC_TRANSITION_FORM_ID                    0x01
#define PQC_READINESS_CHECK_FORM_ID               0x02
#define PQC_ALGORITHM_STATUS_FORM_ID              0x03

//
// PQC Transition Configuration Question IDs
//
#define KEY_PQC_TRANSITION_MODE                   0x1000
#define KEY_PQC_READINESS_CHECK                   0x1001
#define KEY_PQC_FORCE_SWITCH                      0x1002
#define KEY_PQC_CLEANUP_TRADITIONAL               0x1003
#define KEY_PQC_RECOVERY_MODE                     0x1004

//
// PQC Readiness Status Question IDs
//
#define KEY_PQC_PK_STATUS                         0x2000
#define KEY_PQC_KEK_STATUS                        0x2001
#define KEY_PQC_DB_STATUS                         0x2002
#define KEY_PQC_OROM_STATUS                       0x2003
#define KEY_PQC_LOADER_STATUS                     0x2004
#define KEY_PQC_TLS_STATUS                        0x2005
#define KEY_PQC_FWUPDATE_STATUS                   0x2006
#define KEY_PQC_OVERALL_STATUS                    0x2007

//
// PQC Algorithm Status Question IDs
//
#define KEY_PQC_DILITHIUM_STATUS                  0x3000
#define KEY_PQC_FALCON_STATUS                     0x3001
#define KEY_PQC_SPHINCS_STATUS                    0x3002
#define KEY_PQC_KYBER_STATUS                      0x3003
#define KEY_PQC_NTRU_STATUS                       0x3004

//
// PQC Transition Mode Values
//
#define PQC_MODE_TRADITIONAL_ONLY                 0
#define PQC_MODE_HYBRID                           1
#define PQC_MODE_PQC_ONLY                         2
#define PQC_MODE_MAX                              3

//
// PQC Algorithm Status Values
//
#define PQC_ALGORITHM_NOT_SUPPORTED               0
#define PQC_ALGORITHM_SUPPORTED                   1
#define PQC_ALGORITHM_ACTIVE                      2

//
// PQC Transition Configuration Data Structure
//
typedef struct {
  //
  // Current PQC transition mode
  //
  UINT8    PqcTransitionMode;
  
  //
  // PQC algorithm support status
  //
  UINT8    DilithiumStatus;
  UINT8    FalconStatus;
  UINT8    SphincsStatus;
  UINT8    KyberStatus;
  UINT8    NtruStatus;
  
  //
  // PQC readiness check results
  //
  UINT8    PkHasPqcCert;
  UINT8    KekHasPqcCert;
  UINT8    DbHasPqcCert;
  UINT8    OromIsPqcSigned;
  UINT8    LoaderIsPqcSigned;
  UINT8    TlsHasPqcSupport;
  UINT8    FwUpdateHasPqcSupport;
  UINT8    SystemReadyForPqc;
  
  //
  // Configuration options
  //
  UINT8    ForceSwitch;
  UINT8    CleanupTraditional;
  UINT8    RecoveryMode;
  
  //
  // Transition timeline information
  //
  UINT32   CurrentYear;
  UINT32   TransitionDeadline;  // 2030
  UINT32   DaysUntilDeadline;
  
} PQC_TRANSITION_CONFIGURATION;

//
// Labels definition
//
#define LABEL_PQC_READINESS_START                 0x2000
#define LABEL_PQC_READINESS_END                   0x2001
#define LABEL_PQC_ALGORITHM_START                 0x3000
#define LABEL_PQC_ALGORITHM_END                   0x3001

#endif // __PQC_TRANSITION_NV_DATA_H__