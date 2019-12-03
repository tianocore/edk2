/** @file
  Header file for NV data structure definition.

Copyright (c) 2011 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SECUREBOOT_CONFIG_NV_DATA_H__
#define __SECUREBOOT_CONFIG_NV_DATA_H__

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/SecureBootConfigHii.h>

//
// Used by VFR for form or button identification
//
#define SECUREBOOT_CONFIGURATION_VARSTORE_ID  0x0001
#define SECUREBOOT_CONFIGURATION_FORM_ID      0x01
#define FORMID_SECURE_BOOT_OPTION_FORM        0x02
#define FORMID_SECURE_BOOT_PK_OPTION_FORM     0x03
#define FORMID_SECURE_BOOT_KEK_OPTION_FORM    0x04
#define FORMID_SECURE_BOOT_DB_OPTION_FORM     0x05
#define FORMID_SECURE_BOOT_DBX_OPTION_FORM    0x06
#define FORMID_ENROLL_PK_FORM                 0x07
#define SECUREBOOT_ADD_PK_FILE_FORM_ID        0x08
#define FORMID_ENROLL_KEK_FORM                0x09
#define FORMID_DELETE_KEK_FORM                0x0a
#define SECUREBOOT_ENROLL_SIGNATURE_TO_DB     0x0b
#define SECUREBOOT_DELETE_SIGNATURE_FROM_DB   0x0c
#define SECUREBOOT_ENROLL_SIGNATURE_TO_DBX    0x0d
#define FORMID_SECURE_BOOT_DBT_OPTION_FORM    0x14
#define SECUREBOOT_ENROLL_SIGNATURE_TO_DBT    0x15
#define SECUREBOOT_DELETE_SIGNATURE_FROM_DBT  0x16
#define SECUREBOOT_DELETE_SIGNATURE_LIST_FORM 0x17
#define SECUREBOOT_DELETE_SIGNATURE_DATA_FORM 0x18

#define SECURE_BOOT_MODE_CUSTOM               0x01
#define SECURE_BOOT_MODE_STANDARD             0x00

#define KEY_SECURE_BOOT_ENABLE                0x1000
#define KEY_SECURE_BOOT_MODE                  0x1001
#define KEY_VALUE_SAVE_AND_EXIT_DB            0x1002
#define KEY_VALUE_NO_SAVE_AND_EXIT_DB         0x1003
#define KEY_VALUE_SAVE_AND_EXIT_PK            0x1004
#define KEY_VALUE_NO_SAVE_AND_EXIT_PK         0x1005
#define KEY_VALUE_SAVE_AND_EXIT_KEK           0x1008
#define KEY_VALUE_NO_SAVE_AND_EXIT_KEK        0x1009
#define KEY_VALUE_SAVE_AND_EXIT_DBX           0x100a
#define KEY_VALUE_NO_SAVE_AND_EXIT_DBX        0x100b
#define KEY_HIDE_SECURE_BOOT                  0x100c
#define KEY_VALUE_SAVE_AND_EXIT_DBT           0x100d
#define KEY_VALUE_NO_SAVE_AND_EXIT_DBT        0x100e

#define KEY_VALUE_FROM_DBX_TO_LIST_FORM       0x100f

#define KEY_SECURE_BOOT_OPTION                0x1100
#define KEY_SECURE_BOOT_PK_OPTION             0x1101
#define KEY_SECURE_BOOT_KEK_OPTION            0x1102
#define KEY_SECURE_BOOT_DB_OPTION             0x1103
#define KEY_SECURE_BOOT_DBX_OPTION            0x1104
#define KEY_SECURE_BOOT_DELETE_PK             0x1105
#define KEY_ENROLL_PK                         0x1106
#define KEY_ENROLL_KEK                        0x1107
#define KEY_DELETE_KEK                        0x1108
#define KEY_SECURE_BOOT_KEK_GUID              0x110a
#define KEY_SECURE_BOOT_SIGNATURE_GUID_DB     0x110b
#define KEY_SECURE_BOOT_SIGNATURE_GUID_DBX    0x110c
#define KEY_SECURE_BOOT_DBT_OPTION            0x110d
#define KEY_SECURE_BOOT_SIGNATURE_GUID_DBT    0x110e
#define KEY_SECURE_BOOT_DELETE_ALL_LIST       0x110f
#define KEY_SECURE_BOOT_DELETE_ALL_DATA       0x1110
#define KEY_SECURE_BOOT_DELETE_CHECK_DATA     0x1111

#define LABEL_KEK_DELETE                      0x1200
#define LABEL_DB_DELETE                       0x1201
#define LABEL_SIGNATURE_LIST_START            0x1202
#define LABEL_DBT_DELETE                      0x1203
#define LABEL_SIGNATURE_DATA_START            0x1204
#define LABEL_DELETE_ALL_LIST_BUTTON          0x1300
#define LABEL_END                             0xffff

#define SECURE_BOOT_MAX_ATTEMPTS_NUM          255

#define CONFIG_OPTION_OFFSET                  0x2000

#define OPTION_CONFIG_QUESTION_ID             0x2000
#define OPTION_CONFIG_RANGE                   0x1000

//
// Question ID 0x2000 ~ 0x2FFF is for KEK
//
#define OPTION_DEL_KEK_QUESTION_ID            0x2000
//
// Question ID 0x3000 ~ 0x3FFF is for DB
//
#define OPTION_DEL_DB_QUESTION_ID             0x3000
//
// Question ID 0x4000 ~ 0x4FFF is for signature list.
//
#define OPTION_SIGNATURE_LIST_QUESTION_ID     0X4000
//
// Question ID 0x6000 ~ 0x6FFF is for signature data.
//
#define OPTION_SIGNATURE_DATA_QUESTION_ID     0x6000

//
// Question ID 0x5000 ~ 0x5FFF is for DBT
//
#define OPTION_DEL_DBT_QUESTION_ID            0x5000

#define SECURE_BOOT_GUID_SIZE                 36
#define SECURE_BOOT_GUID_STORAGE_SIZE         37

#define UNKNOWN_FILE_TYPE                     0
#define X509_CERT_FILE_TYPE                   1
#define PE_IMAGE_FILE_TYPE                    2
#define AUTHENTICATION_2_FILE_TYPE            3

//
// Nv Data structure referenced by IFR
//
typedef struct {
  BOOLEAN AttemptSecureBoot;   // Attempt to enable/disable Secure Boot
  BOOLEAN HideSecureBoot;      // Hidden Attempt Secure Boot
  CHAR16  SignatureGuid[SECURE_BOOT_GUID_STORAGE_SIZE];
  BOOLEAN PhysicalPresent;     // If a Physical Present User
  UINT8   SecureBootMode;      // Secure Boot Mode: Standard Or Custom
  BOOLEAN DeletePk;
  BOOLEAN HasPk;               // If Pk is existed it is true
  BOOLEAN AlwaysRevocation;    // If the certificate is always revoked. Revocation time is hidden
  UINT8   CertificateFormat;   // The type of the certificate
  EFI_HII_DATE RevocationDate; // The revocation date of the certificate
  EFI_HII_TIME RevocationTime; // The revocation time of the certificate
  UINT8   FileEnrollType;      // File type of signature enroll
  UINT32  ListCount;           // The count of signature list.
  UINT32  CheckedDataCount;    // The count of checked signature data.
} SECUREBOOT_CONFIGURATION;

#endif
