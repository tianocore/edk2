/** @file
  The interface function declaration of shell application IfConfig6.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IFCONFIG6_H_
#define _IFCONFIG6_H_

enum {
  IfConfig6OpList     = 1,
  IfConfig6OpSet      = 2,
  IfConfig6OpClear    = 3
};

typedef enum {
  VarCheckReserved      = -1,
  VarCheckOk            = 0,
  VarCheckDuplicate,
  VarCheckConflict,
  VarCheckUnknown,
  VarCheckLackValue,
  VarCheckOutOfMem
} VAR_CHECK_CODE;

typedef enum {
  FlagTypeSingle         = 0,
  FlagTypeNeedVar,
  FlagTypeNeedSet,
  FlagTypeSkipUnknown
} VAR_CHECK_FLAG_TYPE;

#define MACADDRMAXSIZE    32
#define PREFIXMAXLEN      16 

typedef struct _IFCONFIG6_INTERFACE_CB {
  EFI_HANDLE                                  NicHandle;
  LIST_ENTRY                                  Link;
  EFI_IP6_CONFIG_PROTOCOL                     *IfCfg;
  EFI_IP6_CONFIG_INTERFACE_INFO               *IfInfo; 
  EFI_IP6_CONFIG_INTERFACE_ID                 *IfId;
  EFI_IP6_CONFIG_POLICY                       Policy;
  EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS    Xmits;
  UINT32                                      DnsCnt;
  EFI_IPv6_ADDRESS                            DnsAddr[1];
} IFCONFIG6_INTERFACE_CB;

typedef struct _ARG_LIST ARG_LIST;

struct _ARG_LIST {
  ARG_LIST    *Next;
  CHAR16      *Arg;
};

typedef struct _IFCONFIG6_PRIVATE_DATA {
  EFI_HANDLE  ImageHandle;
  LIST_ENTRY  IfList;

  UINT32      OpCode;
  CHAR16      *IfName;
  ARG_LIST    *VarArg;
} IFCONFIG6_PRIVATE_DATA;

typedef struct _VAR_CHECK_ITEM{
  CHAR16                 *FlagStr;
  UINT32                 FlagID;
  UINT32                 ConflictMask;
  VAR_CHECK_FLAG_TYPE    FlagType;
} VAR_CHECK_ITEM;
#endif
