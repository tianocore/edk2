/** @file
  The internal structure and function declaration in IpSecConfig application.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IPSEC_CONFIG_H_
#define _IPSEC_CONFIG_H_

#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/ShellLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/NetLib.h>

#include <Protocol/IpSecConfig.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define IPSECCONFIG_STATUS_NAME    L"IpSecStatus"

#define BIT(x)   (UINT32) (1 << (x))

#define IPSEC_STATUS_DISABLED    0x0
#define IPSEC_STATUS_ENABLED     0x1

#define EFI_IP4_PROTO_ICMP       0x1
#define EFI_IP4_PROTO_TCP        0x6
#define EFI_IP4_PROTO_UDP        0x11

#define EFI_IPSEC_ANY_PROTOCOL    0xFFFF
#define EFI_IPSEC_ANY_PORT        0

///
/// IPsec Authentication Algorithm Definition
///   The number value definition is aligned to IANA assignment
///
#define IPSEC_AALG_NONE                0x00
#define IPSEC_AALG_MD5HMAC             0x01
#define IPSEC_AALG_SHA1HMAC            0x02
#define IPSEC_AALG_SHA2_256HMAC        0x05
#define IPSEC_AALG_SHA2_384HMAC        0x06
#define IPSEC_AALG_SHA2_512HMAC        0x07
#define IPSEC_AALG_AES_XCBC_MAC        0x09
#define IPSEC_AALG_NULL                0xFB

///
/// IPsec Encryption Algorithm Definition
///   The number value definition is aligned to IANA assignment
///
#define IPSEC_EALG_NONE                0x00
#define IPSEC_EALG_DESCBC              0x02
#define IPSEC_EALG_3DESCBC             0x03
#define IPSEC_EALG_CASTCBC             0x06
#define IPSEC_EALG_BLOWFISHCBC         0x07
#define IPSEC_EALG_NULL                0x0B
#define IPSEC_EALG_AESCBC              0x0C
#define IPSEC_EALG_AESCTR              0x0D
#define IPSEC_EALG_AES_CCM_ICV8        0x0E
#define IPSEC_EALG_AES_CCM_ICV12       0x0F
#define IPSEC_EALG_AES_CCM_ICV16       0x10
#define IPSEC_EALG_AES_GCM_ICV8        0x12
#define IPSEC_EALG_AES_GCM_ICV12       0x13
#define IPSEC_EALG_AES_GCM_ICV16       0x14

typedef struct {
  CHAR16      *VarName;
  UINT32      Attribute1;
  UINT32      Attribute2;
  UINT32      Attribute3;
  UINT32      Attribute4;
} VAR_CHECK_ITEM;

typedef struct {
  LIST_ENTRY        Link;
  CHAR16            *Name;
  SHELL_PARAM_TYPE  Type;
  CHAR16            *Value;
  UINTN             OriginalPosition;
} SHELL_PARAM_PACKAGE;

typedef struct {
  CHAR16        *String;
  UINT32        Integer;
} STR2INT;

extern EFI_IPSEC_CONFIG_PROTOCOL    *mIpSecConfig;
extern EFI_HII_HANDLE               mHiiHandle;
extern CHAR16                       mAppName[];

//
// -P
//
extern STR2INT mMapPolicy[];

//
// --proto
//
extern STR2INT mMapIpProtocol[];

//
// --action
//
extern STR2INT mMapIpSecAction[];

//
// --mode
//
extern STR2INT mMapIpSecMode[];

//
// --dont-fragment
//
extern STR2INT mMapDfOption[];

//
// --ipsec-proto
//
extern STR2INT mMapIpSecProtocol[];
//
// --auth-algo
//
extern STR2INT mMapAuthAlgo[];

//
// --encrypt-algo
//
extern STR2INT mMapEncAlgo[];
//
// --auth-proto
//
extern STR2INT mMapAuthProto[];

//
// --auth-method
//
extern STR2INT mMapAuthMethod[];

#endif
