/** @file
  The internal structure and function declaration in IpSecConfig application.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

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

#define EFI_IPSEC_CONFIG_GUID \
  { \
    0x9db0c3ac, 0xd9d2, 0x4f96, {0x9e, 0xd7, 0x6d, 0xa6, 0x12, 0xa4, 0xf3, 0x27} \
  }

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

typedef struct _VAR_CHECK_ITEM {
  CHAR16      *VarName;
  UINT32      Attribute1;
  UINT32      Attribute2;
  UINT32      Attribute3;
  UINT32      Attribute4;
} VAR_CHECK_ITEM;

typedef struct _SHELL_PARAM_PACKAGE{
  LIST_ENTRY     Link;
  CHAR16         *Name;
  ParamType      Type;
  CHAR16         *Value;
  UINTN          OriginalPosition;
} SHELL_PARAM_PACKAGE;

typedef struct _STR2INT {
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
