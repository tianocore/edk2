/** @file
  Header file for NV data structure definition.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TLS_AUTH_CONFIG_NV_DATA_H__
#define __TLS_AUTH_CONFIG_NV_DATA_H__

#include <Guid/TlsAuthConfigHii.h>

#define TLS_AUTH_CONFIG_GUID_SIZE                 36
#define TLS_AUTH_CONFIG_GUID_STORAGE_SIZE         37

#define TLS_AUTH_CONFIG_FORMID1_FORM              1
#define TLS_AUTH_CONFIG_FORMID2_FORM              2
#define TLS_AUTH_CONFIG_FORMID3_FORM              3
#define TLS_AUTH_CONFIG_FORMID4_FORM              4
#define TLS_AUTH_CONFIG_FORMID5_FORM              5


#define KEY_TLS_AUTH_CONFIG_SERVER_CA                  0x1000
#define KEY_TLS_AUTH_CONFIG_CLIENT_CERT                0x1001
#define KEY_TLS_AUTH_CONFIG_ENROLL_CERT                0x1002
#define KEY_TLS_AUTH_CONFIG_DELETE_CERT                0x1003
#define KEY_TLS_AUTH_CONFIG_ENROLL_CERT_FROM_FILE      0x1004
#define KEY_TLS_AUTH_CONFIG_CERT_GUID                  0x1005
#define KEY_TLS_AUTH_CONFIG_VALUE_SAVE_AND_EXIT        0x1006
#define KEY_TLS_AUTH_CONFIG_VALUE_NO_SAVE_AND_EXIT     0x1007

#define OPTION_DEL_CA_ESTION_ID                        0x2000
#define OPTION_CONFIG_RANGE                            0x1000

#define LABEL_CA_DELETE                                0x1101
#define LABEL_END                                      0xffff

typedef struct {
  CHAR16    CertGuid[TLS_AUTH_CONFIG_GUID_STORAGE_SIZE];
} TLS_AUTH_CONFIG_IFR_NVDATA;

#endif

