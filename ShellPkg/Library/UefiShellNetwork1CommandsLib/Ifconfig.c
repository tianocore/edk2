/** @file
  The implementation for Shell command ifconfig based on IP4Config2 protocol.

  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellNetwork1CommandsLib.h"

typedef enum {
  IfConfigOpList     = 1,
  IfConfigOpSet      = 2,
  IfConfigOpClear    = 3
} IFCONFIG_OPCODE;

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

typedef struct _IFCONFIG_INTERFACE_CB {
  EFI_HANDLE                                  NicHandle;
  LIST_ENTRY                                  Link;
  EFI_IP4_CONFIG2_PROTOCOL                    *IfCfg;
  EFI_IP4_CONFIG2_INTERFACE_INFO              *IfInfo;
  EFI_IP4_CONFIG2_POLICY                      Policy;
  UINT32                                      DnsCnt;
  EFI_IPv4_ADDRESS                            DnsAddr[1];
} IFCONFIG_INTERFACE_CB;

typedef struct _ARG_LIST ARG_LIST;

struct _ARG_LIST {
  ARG_LIST    *Next;
  CHAR16      *Arg;
};

typedef struct _IFCONFIG4_PRIVATE_DATA {
  LIST_ENTRY  IfList;

  UINT32      OpCode;
  CHAR16      *IfName;
  ARG_LIST    *VarArg;
} IFCONFIG_PRIVATE_DATA;

typedef struct _VAR_CHECK_ITEM{
  CHAR16                 *FlagStr;
  UINT32                 FlagID;
  UINT32                 ConflictMask;
  VAR_CHECK_FLAG_TYPE    FlagType;
} VAR_CHECK_ITEM;

SHELL_PARAM_ITEM    mIfConfigCheckList[] = {
  {
    L"-b",
    TypeFlag
  },
  {
    L"-l",
    TypeValue
  },
  {
    L"-r",
    TypeValue
  },
  {
    L"-c",
    TypeValue
  },
  {
    L"-s",
    TypeMaxValue
  },
  {
    NULL,
    TypeMax
  },
};

VAR_CHECK_ITEM  mSetCheckList[] = {
  {
   L"static",
    0x00000001,
    0x00000001,
    FlagTypeSingle
  },
  {
    L"dhcp",
    0x00000002,
    0x00000001,
    FlagTypeSingle
  },
  {
    L"dns",
    0x00000008,
    0x00000004,
    FlagTypeSingle
  },
  {
    NULL,
    0x0,
    0x0,
    FlagTypeSkipUnknown
  },
};

STATIC CONST CHAR16 PermanentString[10] = L"PERMANENT";

/**
  Free the ARG_LIST.

  @param List Pointer to ARG_LIST to free.
**/
VOID
FreeArgList (
  ARG_LIST       *List
)
{
  ARG_LIST       *Next;
  while (List->Next != NULL) {
    Next = List->Next;
    FreePool (List);
    List = Next;
  }

  FreePool (List);
}

/**
  Split a string with specified separator and save the substring to a list.

  @param[in]    String       The pointer of the input string.
  @param[in]    Separator    The specified separator.

  @return The pointer of headnode of ARG_LIST.

**/
ARG_LIST *
SplitStrToList (
  IN CONST CHAR16    *String,
  IN CHAR16          Separator
  )
{
  CHAR16      *Str;
  CHAR16      *ArgStr;
  ARG_LIST    *ArgList;
  ARG_LIST    *ArgNode;

  if (*String == L'\0') {
    return NULL;
  }

  //
  // Copy the CONST string to a local copy.
  //
  Str = AllocateCopyPool (StrSize (String), String);
  if (Str == NULL) {
    return NULL;
  }
  ArgStr  = Str;

  //
  // init a node for the list head.
  //
  ArgNode = (ARG_LIST *) AllocateZeroPool (sizeof (ARG_LIST));
  if (ArgNode == NULL) {
    return NULL;
  }
  ArgList = ArgNode;

  //
  // Split the local copy and save in the list node.
  //
  while (*Str != L'\0') {
    if (*Str == Separator) {
      *Str          = L'\0';
      ArgNode->Arg  = ArgStr;
      ArgStr        = Str + 1;
      ArgNode->Next = (ARG_LIST *) AllocateZeroPool (sizeof (ARG_LIST));
      if (ArgNode->Next == NULL) {
        //
        // Free the local copy of string stored in the first node
        //
        FreePool (ArgList->Arg);
        FreeArgList (ArgList);
        return NULL;
      }
      ArgNode = ArgNode->Next;
    }

    Str++;
  }

  ArgNode->Arg  = ArgStr;
  ArgNode->Next = NULL;

  return ArgList;
}

/**
  Check the correctness of input Args with '-s' option.

  @param[in]    CheckList    The pointer of VAR_CHECK_ITEM array.
  @param[in]    Name         The pointer of input arg.
  @param[in]    Init         The switch to execute the check.

  @return   VarCheckOk          Valid parameter or Initialize check successfully.
  @return   VarCheckDuplicate   Duplicated parameter happened.
  @return   VarCheckConflict    Conflicted parameter happened
  @return   VarCheckUnknown     Unknown parameter.

**/
VAR_CHECK_CODE
IfConfigRetriveCheckListByName(
  IN VAR_CHECK_ITEM    *CheckList,
  IN CHAR16            *Name,
  IN BOOLEAN           Init
)
{
  STATIC UINT32     CheckDuplicate;
  STATIC UINT32     CheckConflict;
  VAR_CHECK_CODE    RtCode;
  UINT32            Index;
  VAR_CHECK_ITEM    Arg;

  if (Init) {
    CheckDuplicate = 0;
    CheckConflict  = 0;
    return VarCheckOk;
  }

  RtCode  = VarCheckOk;
  Index   = 0;
  Arg     = CheckList[Index];

  //
  // Check the Duplicated/Conflicted/Unknown input Args.
  //
  while (Arg.FlagStr != NULL) {
    if (StrCmp (Arg.FlagStr, Name) == 0) {

      if (CheckDuplicate & Arg.FlagID) {
        RtCode = VarCheckDuplicate;
        break;
      }

      if (CheckConflict & Arg.ConflictMask) {
        RtCode = VarCheckConflict;
        break;
      }

      CheckDuplicate |= Arg.FlagID;
      CheckConflict  |= Arg.ConflictMask;
      break;
    }

    Arg = CheckList[++Index];
  }

  if (Arg.FlagStr == NULL) {
    RtCode = VarCheckUnknown;
  }

  return RtCode;
}

/**
  The notify function of create event when performing a manual config.

  @param[in]    Event        The event this notify function registered to.
  @param[in]    Context      Pointer to the context data registered to the event.

**/
VOID
EFIAPI
IfConfigManualAddressNotify (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  *((BOOLEAN *) Context) = TRUE;
}

/**
  Print MAC address.

  @param[in]    Node    The pointer of MAC address buffer.
  @param[in]    Size    The size of MAC address buffer.

**/
VOID
IfConfigPrintMacAddr (
  IN UINT8     *Node,
  IN UINT32    Size
  )
{
  UINTN    Index;

  ASSERT (Size <= MACADDRMAXSIZE);

  for (Index = 0; Index < Size; Index++) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_MAC_ADDR_BODY), gShellNetwork1HiiHandle, Node[Index]);
    if (Index + 1 < Size) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_COLON), gShellNetwork1HiiHandle);
    }
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_NEWLINE), gShellNetwork1HiiHandle);
}


/**
  The get current status of all handles.

  @param[in]   IfName         The pointer of IfName(interface name).
  @param[in]   IfList         The pointer of IfList(interface list).

  @retval EFI_SUCCESS    The get status processed successfully.
  @retval others         The get status process failed.

**/
EFI_STATUS
IfConfigGetInterfaceInfo (
  IN CHAR16        *IfName,
  IN LIST_ENTRY    *IfList
  )
{
  EFI_STATUS                       Status;
  UINTN                            HandleIndex;
  UINTN                            HandleNum;
  EFI_HANDLE                       *HandleBuffer;
  EFI_IP4_CONFIG2_PROTOCOL         *Ip4Cfg2;
  EFI_IP4_CONFIG2_INTERFACE_INFO   *IfInfo;
  IFCONFIG_INTERFACE_CB            *IfCb;
  UINTN                            DataSize;

  HandleBuffer = NULL;
  HandleNum    = 0;

  IfInfo       = NULL;
  IfCb         = NULL;

  //
  // Locate all the handles with ip4 service binding protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiIp4ServiceBindingProtocolGuid,
                  NULL,
                  &HandleNum,
                  &HandleBuffer
                 );
  if (EFI_ERROR (Status) || (HandleNum == 0)) {
    return Status;
  }

  //
  // Enumerate all handles that installed with ip4 service binding protocol.
  //
  for (HandleIndex = 0; HandleIndex < HandleNum; HandleIndex++) {
    IfCb      = NULL;
    IfInfo    = NULL;
    DataSize  = 0;

    //
    // Ip4config protocol and ip4 service binding protocol are installed
    // on the same handle.
    //
    ASSERT (HandleBuffer != NULL);
    Status = gBS->HandleProtocol (
                    HandleBuffer[HandleIndex],
                    &gEfiIp4Config2ProtocolGuid,
                    (VOID **) &Ip4Cfg2
                    );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Get the interface information size.
    //
    Status = Ip4Cfg2->GetData (
                       Ip4Cfg2,
                       Ip4Config2DataTypeInterfaceInfo,
                       &DataSize,
                       NULL
                       );

    if (Status != EFI_BUFFER_TOO_SMALL) {
      goto ON_ERROR;
    }

    IfInfo = AllocateZeroPool (DataSize);

    if (IfInfo == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    //
    // Get the interface info.
    //
    Status = Ip4Cfg2->GetData (
                       Ip4Cfg2,
                       Ip4Config2DataTypeInterfaceInfo,
                       &DataSize,
                       IfInfo
                       );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Check the interface name if required.
    //
    if ((IfName != NULL) && (StrCmp (IfName, IfInfo->Name) != 0)) {
      FreePool (IfInfo);
      continue;
    }

    DataSize = 0;

    //
    // Get the size of dns server list.
    //
    Status = Ip4Cfg2->GetData (
                       Ip4Cfg2,
                       Ip4Config2DataTypeDnsServer,
                       &DataSize,
                       NULL
                       );

    if ((Status != EFI_BUFFER_TOO_SMALL) && (Status != EFI_NOT_FOUND)) {
      goto ON_ERROR;
    }

    IfCb = AllocateZeroPool (sizeof (IFCONFIG_INTERFACE_CB) + DataSize);

    if (IfCb == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    IfCb->NicHandle = HandleBuffer[HandleIndex];
    IfCb->IfInfo    = IfInfo;
    IfCb->IfCfg     = Ip4Cfg2;
    IfCb->DnsCnt    = (UINT32) (DataSize / sizeof (EFI_IPv4_ADDRESS));

    //
    // Get the dns server list if has.
    //
    if (DataSize > 0) {
      Status = Ip4Cfg2->GetData (
                         Ip4Cfg2,
                         Ip4Config2DataTypeDnsServer,
                         &DataSize,
                         IfCb->DnsAddr
                         );

      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }
    }

    //
    // Get the config policy.
    //
    DataSize = sizeof (EFI_IP4_CONFIG2_POLICY);
    Status   = Ip4Cfg2->GetData (
                         Ip4Cfg2,
                         Ip4Config2DataTypePolicy,
                         &DataSize,
                         &IfCb->Policy
                         );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    InsertTailList (IfList, &IfCb->Link);

    if ((IfName != NULL) && (StrCmp (IfName, IfInfo->Name) == 0)) {
      //
      // Only need the appointed interface, keep the allocated buffer.
      //
      IfCb   = NULL;
      IfInfo = NULL;
      break;
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (IfInfo != NULL) {
    FreePool (IfInfo);
  }

  if (IfCb != NULL) {
    FreePool (IfCb);
  }

  return Status;
}

/**
  The list process of the ifconfig command.

  @param[in]   IfList    The pointer of IfList(interface list).

  @retval SHELL_SUCCESS  The ifconfig command list processed successfully.
  @retval others         The ifconfig command list process failed.

**/
SHELL_STATUS
IfConfigShowInterfaceInfo (
  IN LIST_ENTRY    *IfList
  )
{
  LIST_ENTRY                   *Entry;
  LIST_ENTRY                   *Next;
  IFCONFIG_INTERFACE_CB        *IfCb;
  EFI_STATUS                    MediaStatus;
  EFI_IPv4_ADDRESS              Gateway;
  UINT32                        Index;

  MediaStatus = EFI_SUCCESS;

  if (IsListEmpty (IfList)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INVALID_INTERFACE), gShellNetwork1HiiHandle);
  }

  //
  // Go through the interface list.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, Next, IfList) {
    IfCb = NET_LIST_USER_STRUCT (Entry, IFCONFIG_INTERFACE_CB, Link);

    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_BREAK), gShellNetwork1HiiHandle);

    //
    // Print interface name.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_IF_NAME), gShellNetwork1HiiHandle, IfCb->IfInfo->Name);

    //
    // Get Media State.
    //
    if (EFI_SUCCESS == NetLibDetectMediaWaitTimeout (IfCb->NicHandle, 0, &MediaStatus)) {
      if (MediaStatus != EFI_SUCCESS) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_MEDIA_STATE), gShellNetwork1HiiHandle, L"Media disconnected");
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_MEDIA_STATE), gShellNetwork1HiiHandle, L"Media present");
      }
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_MEDIA_STATE), gShellNetwork1HiiHandle, L"Media state unknown");
    }

    //
    // Print interface config policy.
    //
    if (IfCb->Policy == Ip4Config2PolicyDhcp) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_POLICY_DHCP), gShellNetwork1HiiHandle);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_POLICY_MAN), gShellNetwork1HiiHandle);
    }

    //
    // Print mac address of the interface.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_MAC_ADDR_HEAD), gShellNetwork1HiiHandle);

    IfConfigPrintMacAddr (
      IfCb->IfInfo->HwAddress.Addr,
      IfCb->IfInfo->HwAddressSize
      );

    //
    // Print IPv4 address list of the interface.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_IP_ADDR_HEAD), gShellNetwork1HiiHandle);

    ShellPrintHiiEx(
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_IFCONFIG_INFO_IP_ADDR_BODY),
      gShellNetwork1HiiHandle,
      (UINTN)IfCb->IfInfo->StationAddress.Addr[0],
      (UINTN)IfCb->IfInfo->StationAddress.Addr[1],
      (UINTN)IfCb->IfInfo->StationAddress.Addr[2],
      (UINTN)IfCb->IfInfo->StationAddress.Addr[3]
      );

    //
    // Print subnet mask list of the interface.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_SUBNET_MASK_HEAD), gShellNetwork1HiiHandle);

    ShellPrintHiiEx(
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_IFCONFIG_INFO_IP_ADDR_BODY),
      gShellNetwork1HiiHandle,
      (UINTN)IfCb->IfInfo->SubnetMask.Addr[0],
      (UINTN)IfCb->IfInfo->SubnetMask.Addr[1],
      (UINTN)IfCb->IfInfo->SubnetMask.Addr[2],
      (UINTN)IfCb->IfInfo->SubnetMask.Addr[3]
      );

    //
    // Print default gateway of the interface.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_GATEWAY_HEAD), gShellNetwork1HiiHandle);

    ZeroMem (&Gateway, sizeof (EFI_IPv4_ADDRESS));

    for (Index = 0; Index < IfCb->IfInfo->RouteTableSize; Index++) {
      if ((CompareMem (&IfCb->IfInfo->RouteTable[Index].SubnetAddress, &mZeroIp4Addr, sizeof (EFI_IPv4_ADDRESS)) == 0) &&
          (CompareMem (&IfCb->IfInfo->RouteTable[Index].SubnetMask   , &mZeroIp4Addr, sizeof (EFI_IPv4_ADDRESS)) == 0) ){
        CopyMem (&Gateway, &IfCb->IfInfo->RouteTable[Index].GatewayAddress, sizeof (EFI_IPv4_ADDRESS));
      }
    }

    ShellPrintHiiEx(
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_IFCONFIG_INFO_IP_ADDR_BODY),
      gShellNetwork1HiiHandle,
      (UINTN)Gateway.Addr[0],
      (UINTN)Gateway.Addr[1],
      (UINTN)Gateway.Addr[2],
      (UINTN)Gateway.Addr[3]
      );

    //
    // Print route table entry.
    //
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_ROUTES_SIZE), gShellNetwork1HiiHandle, IfCb->IfInfo->RouteTableSize);

    for (Index = 0; Index < IfCb->IfInfo->RouteTableSize; Index++) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_ROUTES_ENTRY_INDEX), gShellNetwork1HiiHandle, Index);

      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR),
        gShellNetwork1HiiHandle,
        L"Subnet ",
        (UINTN)IfCb->IfInfo->RouteTable[Index].SubnetAddress.Addr[0],
        (UINTN)IfCb->IfInfo->RouteTable[Index].SubnetAddress.Addr[1],
        (UINTN)IfCb->IfInfo->RouteTable[Index].SubnetAddress.Addr[2],
        (UINTN)IfCb->IfInfo->RouteTable[Index].SubnetAddress.Addr[3]
        );

      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR),
        gShellNetwork1HiiHandle,
        L"Netmask",
        (UINTN)IfCb->IfInfo->RouteTable[Index].SubnetMask.Addr[0],
        (UINTN)IfCb->IfInfo->RouteTable[Index].SubnetMask.Addr[1],
        (UINTN)IfCb->IfInfo->RouteTable[Index].SubnetMask.Addr[2],
        (UINTN)IfCb->IfInfo->RouteTable[Index].SubnetMask.Addr[3]
        );

      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IFCONFIG_SHOW_IP_ADDR),
        gShellNetwork1HiiHandle,
        L"Gateway",
        (UINTN)IfCb->IfInfo->RouteTable[Index].GatewayAddress.Addr[0],
        (UINTN)IfCb->IfInfo->RouteTable[Index].GatewayAddress.Addr[1],
        (UINTN)IfCb->IfInfo->RouteTable[Index].GatewayAddress.Addr[2],
        (UINTN)IfCb->IfInfo->RouteTable[Index].GatewayAddress.Addr[3]
        );
    }

    //
    // Print dns server addresses list of the interface if has.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_DNS_ADDR_HEAD), gShellNetwork1HiiHandle);

    for (Index = 0; Index < IfCb->DnsCnt; Index++) {
      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IFCONFIG_INFO_DNS_ADDR_BODY),
        gShellNetwork1HiiHandle,
        (UINTN) IfCb->DnsAddr[Index].Addr[0],
        (UINTN) IfCb->DnsAddr[Index].Addr[1],
        (UINTN) IfCb->DnsAddr[Index].Addr[2],
        (UINTN) IfCb->DnsAddr[Index].Addr[3]
        );

      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_NEWLINE), gShellNetwork1HiiHandle);
    }
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INFO_BREAK), gShellNetwork1HiiHandle);

  return SHELL_SUCCESS;
}

/**
  The clean process of the ifconfig command to clear interface info.

  @param[in]   IfList    The pointer of IfList(interface list).
  @param[in]   IfName    The pointer of interface name.

  @retval SHELL_SUCCESS  The ifconfig command clean processed successfully.
  @retval others         The ifconfig command clean process failed.

**/
SHELL_STATUS
IfConfigClearInterfaceInfo (
  IN LIST_ENTRY    *IfList,
  IN CHAR16        *IfName
  )
{
  EFI_STATUS                Status;
  SHELL_STATUS              ShellStatus;
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  IFCONFIG_INTERFACE_CB     *IfCb;
  EFI_IP4_CONFIG2_POLICY    Policy;

  Status = EFI_SUCCESS;
  ShellStatus = SHELL_SUCCESS;

  if (IsListEmpty (IfList)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INVALID_INTERFACE), gShellNetwork1HiiHandle);
  }

  //
  // Go through the interface list.
  // If the interface name is specified, DHCP DORA process will be
  // triggered by the policy transition (static -> dhcp).
  //
  NET_LIST_FOR_EACH_SAFE (Entry, Next, IfList) {
    IfCb = NET_LIST_USER_STRUCT (Entry, IFCONFIG_INTERFACE_CB, Link);

    if ((IfName != NULL) && (StrCmp (IfName, IfCb->IfInfo->Name) == 0)) {
      Policy = Ip4Config2PolicyStatic;

      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip4Config2DataTypePolicy,
                              sizeof (EFI_IP4_CONFIG2_POLICY),
                              &Policy
                              );
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_AD), gShellNetwork1HiiHandle, L"ifconfig");
        ShellStatus = SHELL_ACCESS_DENIED;
        break;
      }
    }

    Policy = Ip4Config2PolicyDhcp;

    Status = IfCb->IfCfg->SetData (
                            IfCb->IfCfg,
                            Ip4Config2DataTypePolicy,
                            sizeof (EFI_IP4_CONFIG2_POLICY),
                            &Policy
                            );
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_AD), gShellNetwork1HiiHandle, L"ifconfig");
      ShellStatus = SHELL_ACCESS_DENIED;
      break;
    }
  }

  return ShellStatus;
}

/**
  The set process of the ifconfig command.

  @param[in]   IfList    The pointer of IfList(interface list).
  @param[in]   VarArg    The pointer of ARG_LIST(Args with "-s" option).

  @retval SHELL_SUCCESS  The ifconfig command set processed successfully.
  @retval others         The ifconfig command set process failed.

**/
SHELL_STATUS
IfConfigSetInterfaceInfo (
  IN LIST_ENTRY    *IfList,
  IN ARG_LIST      *VarArg
  )
{
  EFI_STATUS                       Status;
  SHELL_STATUS                     ShellStatus;
  IFCONFIG_INTERFACE_CB            *IfCb;
  VAR_CHECK_CODE                   CheckCode;
  EFI_EVENT                        TimeOutEvt;
  EFI_EVENT                        MappedEvt;
  BOOLEAN                          IsAddressOk;

  EFI_IP4_CONFIG2_POLICY           Policy;
  EFI_IP4_CONFIG2_MANUAL_ADDRESS   ManualAddress;
  UINTN                            DataSize;
  EFI_IPv4_ADDRESS                 Gateway;
  IP4_ADDR                         SubnetMask;
  IP4_ADDR                         TempGateway;
  EFI_IPv4_ADDRESS                 *Dns;
  ARG_LIST                         *Tmp;
  UINTN                            Index;

  CONST CHAR16* TempString;

  Dns = NULL;

  if (IsListEmpty (IfList)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INVALID_INTERFACE), gShellNetwork1HiiHandle);
    return SHELL_INVALID_PARAMETER;
  }

  //
  // Make sure to set only one interface each time.
  //
  IfCb   = NET_LIST_USER_STRUCT (IfList->ForwardLink, IFCONFIG_INTERFACE_CB, Link);
  Status = EFI_SUCCESS;
  ShellStatus = SHELL_SUCCESS;

  //
  // Initialize check list mechanism.
  //
  CheckCode = IfConfigRetriveCheckListByName(
                NULL,
                NULL,
                TRUE
                );

  //
  // Create events & timers for asynchronous settings.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TimeOutEvt
                  );
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_AD), gShellNetwork1HiiHandle, L"ifconfig");
    ShellStatus = SHELL_ACCESS_DENIED;
    goto ON_EXIT;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IfConfigManualAddressNotify,
                  &IsAddressOk,
                  &MappedEvt
                  );
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_AD), gShellNetwork1HiiHandle, L"ifconfig");
    ShellStatus = SHELL_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Parse the setting variables.
  //
  while (VarArg != NULL) {
    //
    // Check invalid parameters (duplication & unknown & conflict).
    //
    CheckCode = IfConfigRetriveCheckListByName(
                  mSetCheckList,
                  VarArg->Arg,
                  FALSE
                  );

    if (VarCheckOk != CheckCode) {
      switch (CheckCode) {
        case VarCheckDuplicate:
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_DUPLICATE_COMMAND), gShellNetwork1HiiHandle, VarArg->Arg);
          break;

        case VarCheckConflict:
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_CONFLICT_COMMAND), gShellNetwork1HiiHandle, VarArg->Arg);
          break;

        case VarCheckUnknown:
          //
          // To handle unsupported option.
          //
          TempString = PermanentString;
          if (StringNoCaseCompare(&VarArg->Arg, &TempString) == 0) {
            ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_UNSUPPORTED_OPTION), gShellNetwork1HiiHandle, PermanentString);
            goto ON_EXIT;
          }

          //
          // To handle unknown option.
          //
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_UNKNOWN_COMMAND), gShellNetwork1HiiHandle, VarArg->Arg);
          break;

        default:
          break;
      }

      VarArg = VarArg->Next;
      continue;
    }

    //
    // Process valid variables.
    //
    if (StrCmp(VarArg->Arg, L"dhcp") == 0) {
      //
      // Set dhcp config policy
      //
      Policy = Ip4Config2PolicyDhcp;
      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip4Config2DataTypePolicy,
                              sizeof (EFI_IP4_CONFIG2_POLICY),
                              &Policy
                              );
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_AD), gShellNetwork1HiiHandle, L"ifconfig");
        ShellStatus = SHELL_ACCESS_DENIED;
        goto ON_EXIT;
      }

      VarArg= VarArg->Next;

    } else if (StrCmp (VarArg->Arg, L"static") == 0) {
      VarArg= VarArg->Next;
      if (VarArg == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_LACK_COMMAND), gShellNetwork1HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      ZeroMem (&ManualAddress, sizeof (ManualAddress));

      //
      // Get manual IP address.
      //
      Status = NetLibStrToIp4 (VarArg->Arg, &ManualAddress.Address);
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_INVALID_IPADDRESS), gShellNetwork1HiiHandle, VarArg->Arg);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      //
      // Get subnetmask.
      //
      VarArg = VarArg->Next;
      if (VarArg == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_LACK_COMMAND), gShellNetwork1HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      Status = NetLibStrToIp4 (VarArg->Arg, &ManualAddress.SubnetMask);
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_INVALID_IPADDRESS), gShellNetwork1HiiHandle, VarArg->Arg);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      //
      // Get gateway.
      //
      VarArg = VarArg->Next;
      if (VarArg == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_LACK_COMMAND), gShellNetwork1HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      Status = NetLibStrToIp4 (VarArg->Arg, &Gateway);
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_INVALID_IPADDRESS), gShellNetwork1HiiHandle, VarArg->Arg);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      //
      // Need to check the gateway validity before set Manual Address.
      // In case we can set manual address but fail to configure Gateway.
      //
      CopyMem (&SubnetMask, &ManualAddress.SubnetMask, sizeof (IP4_ADDR));
      CopyMem (&TempGateway, &Gateway, sizeof (IP4_ADDR));
      SubnetMask  = NTOHL (SubnetMask);
      TempGateway = NTOHL (TempGateway);
      if ((SubnetMask != 0) &&
          (SubnetMask != 0xFFFFFFFFu) &&
          !NetIp4IsUnicast (TempGateway, SubnetMask)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_INVALID_GATEWAY), gShellNetwork1HiiHandle, VarArg->Arg);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto ON_EXIT;
      }

      //
      // Set manual config policy.
      //
      Policy = Ip4Config2PolicyStatic;
      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip4Config2DataTypePolicy,
                              sizeof (EFI_IP4_CONFIG2_POLICY),
                              &Policy
                              );
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_AD), gShellNetwork1HiiHandle, L"ifconfig");
        ShellStatus = SHELL_ACCESS_DENIED;
        goto ON_EXIT;
      }

      //
      // Set Manual Address.
      //
      IsAddressOk = FALSE;

      Status = IfCb->IfCfg->RegisterDataNotify (
                              IfCb->IfCfg,
                              Ip4Config2DataTypeManualAddress,
                              MappedEvt
                              );
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_SET_ADDR_FAILED), gShellNetwork1HiiHandle, Status);
        ShellStatus = SHELL_ACCESS_DENIED;
        goto ON_EXIT;
      }

      DataSize = sizeof (EFI_IP4_CONFIG2_MANUAL_ADDRESS);

      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip4Config2DataTypeManualAddress,
                              DataSize,
                              &ManualAddress
                              );

      if (Status == EFI_NOT_READY) {
        gBS->SetTimer (TimeOutEvt, TimerRelative, 50000000);

        while (EFI_ERROR (gBS->CheckEvent (TimeOutEvt))) {
          if (IsAddressOk) {
            Status = EFI_SUCCESS;
            break;
          }
        }
      }

      IfCb->IfCfg->UnregisterDataNotify (
                     IfCb->IfCfg,
                     Ip4Config2DataTypeManualAddress,
                     MappedEvt
                     );

      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_SET_ADDR_FAILED), gShellNetwork1HiiHandle, Status);
        ShellStatus = SHELL_ACCESS_DENIED;
        goto ON_EXIT;
      }

      //
      // Set gateway.
      //
      DataSize = sizeof (EFI_IPv4_ADDRESS);

      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip4Config2DataTypeGateway,
                              DataSize,
                              &Gateway
                              );
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_SET_ADDR_FAILED), gShellNetwork1HiiHandle, Status);
        ShellStatus = SHELL_ACCESS_DENIED;
        goto ON_EXIT;
      }

      VarArg = VarArg->Next;

    } else if (StrCmp (VarArg->Arg, L"dns") == 0) {
      //
      // Get DNS addresses.
      //
      VarArg = VarArg->Next;
      Tmp    = VarArg;
      Index  = 0;
      while (Tmp != NULL) {
        Index ++;
        Tmp = Tmp->Next;
      }

      Dns   = AllocatePool (Index * sizeof (EFI_IPv4_ADDRESS));
      if (Dns == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellNetwork1HiiHandle, L"ifconfig");
        ShellStatus = SHELL_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }
      Tmp   = VarArg;
      Index = 0;
      while (Tmp != NULL) {
        Status = NetLibStrToIp4 (Tmp->Arg, Dns + Index);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_IFCONFIG_INVALID_IPADDRESS), gShellNetwork1HiiHandle, Tmp->Arg);
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto ON_EXIT;
        }
        Index ++;
        Tmp = Tmp->Next;
      }

      VarArg = Tmp;

      //
      // Set DNS addresses.
      //
      DataSize = Index * sizeof (EFI_IPv4_ADDRESS);

      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip4Config2DataTypeDnsServer,
                              DataSize,
                              Dns
                              );
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_AD), gShellNetwork1HiiHandle, L"ifconfig");
        ShellStatus = SHELL_ACCESS_DENIED;
        goto ON_EXIT;
      }
    }
  }

ON_EXIT:
  if (Dns != NULL) {
    FreePool (Dns);
  }

  return ShellStatus;

}

/**
  The ifconfig command main process.

  @param[in]   Private    The pointer of IFCONFIG_PRIVATE_DATA.

  @retval SHELL_SUCCESS  ifconfig command processed successfully.
  @retval others         The ifconfig command process failed.

**/
SHELL_STATUS
IfConfig (
  IN IFCONFIG_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS    Status;
  SHELL_STATUS  ShellStatus;

  ShellStatus = SHELL_SUCCESS;

  //
  // Get configure information of all interfaces.
  //
  Status = IfConfigGetInterfaceInfo (
             Private->IfName,
             &Private->IfList
             );
  if (EFI_ERROR (Status)) {
    ShellStatus = SHELL_NOT_FOUND;
    goto ON_EXIT;
  }

  switch (Private->OpCode) {
  case IfConfigOpList:
    ShellStatus = IfConfigShowInterfaceInfo (&Private->IfList);
    break;

  case IfConfigOpClear:
    ShellStatus = IfConfigClearInterfaceInfo (&Private->IfList, Private->IfName);
    break;

  case IfConfigOpSet:
    ShellStatus = IfConfigSetInterfaceInfo (&Private->IfList, Private->VarArg);
    break;

  default:
    ShellStatus = SHELL_UNSUPPORTED;
  }

ON_EXIT:
  return ShellStatus;
}

/**
  The ifconfig command cleanup process, free the allocated memory.

  @param[in]   Private    The pointer of  IFCONFIG_PRIVATE_DATA.

**/
VOID
IfConfigCleanup (
  IN IFCONFIG_PRIVATE_DATA  *Private
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *NextEntry;
  IFCONFIG_INTERFACE_CB     *IfCb;

  ASSERT (Private != NULL);

  //
  // Clean the list which save the set config Args.
  //
  if (Private->VarArg != NULL) {
    FreeArgList (Private->VarArg);
  }

  if (Private->IfName != NULL) {
    FreePool (Private->IfName);
  }

  //
  // Clean the IFCONFIG_INTERFACE_CB list.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->IfList) {
    IfCb = NET_LIST_USER_STRUCT (Entry, IFCONFIG_INTERFACE_CB, Link);

    RemoveEntryList (&IfCb->Link);

    if (IfCb->IfInfo != NULL) {

      FreePool (IfCb->IfInfo);
    }

    FreePool (IfCb);
  }

  FreePool (Private);
}

/**
  Function for 'ifconfig' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).

  @retval EFI_SUCCESS    ifconfig command processed successfully.
  @retval others         The ifconfig command process failed.

**/
SHELL_STATUS
EFIAPI
ShellCommandRunIfconfig (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  IFCONFIG_PRIVATE_DATA     *Private;
  LIST_ENTRY                *ParamPackage;
  SHELL_STATUS              ShellStatus;
  CONST CHAR16              *ValueStr;
  ARG_LIST                  *ArgList;
  CHAR16                    *ProblemParam;
  CHAR16                    *Str;

  Status = EFI_INVALID_PARAMETER;
  Private = NULL;
  ShellStatus = SHELL_SUCCESS;

  Status = ShellCommandLineParseEx (mIfConfigCheckList, &ParamPackage, &ProblemParam, TRUE, FALSE);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellNetwork1HiiHandle, L"ifconfig", ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }

    goto ON_EXIT;
  }

  //
  // To handle unsupported option.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-c")) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_UNSUPPORTED_OPTION), gShellNetwork1HiiHandle,L"-c");
    ShellStatus = SHELL_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  //
  // To handle no option.
  //
  if (!ShellCommandLineGetFlag (ParamPackage, L"-r") && !ShellCommandLineGetFlag (ParamPackage, L"-s") &&
      !ShellCommandLineGetFlag (ParamPackage, L"-l")) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_LACK_OPTION), gShellNetwork1HiiHandle);
    ShellStatus = SHELL_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  //
  // To handle conflict options.
  //
  if (((ShellCommandLineGetFlag (ParamPackage, L"-r")) && (ShellCommandLineGetFlag (ParamPackage, L"-s"))) ||
      ((ShellCommandLineGetFlag (ParamPackage, L"-r")) && (ShellCommandLineGetFlag (ParamPackage, L"-l"))) ||
      ((ShellCommandLineGetFlag (ParamPackage, L"-s")) && (ShellCommandLineGetFlag (ParamPackage, L"-l")))) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CON), gShellNetwork1HiiHandle, L"ifconfig");
    ShellStatus = SHELL_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Private = AllocateZeroPool (sizeof (IFCONFIG_PRIVATE_DATA));
  if (Private == NULL) {
    ShellStatus = SHELL_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  InitializeListHead (&Private->IfList);

  //
  // To get interface name for the list option.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-l")) {
    Private->OpCode = IfConfigOpList;
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-l");
    if (ValueStr != NULL) {
      Str = AllocateCopyPool (StrSize (ValueStr), ValueStr);
      if (Str == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellNetwork1HiiHandle, L"ifconfig");
        ShellStatus = SHELL_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }
      Private->IfName = Str;
    }
  }

  //
  // To get interface name for the clear option.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-r")) {
    Private->OpCode = IfConfigOpClear;
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-r");
    if (ValueStr != NULL) {
      Str = AllocateCopyPool (StrSize (ValueStr), ValueStr);
      if (Str == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellNetwork1HiiHandle, L"ifconfig");
        ShellStatus = SHELL_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }
      Private->IfName = Str;
    }
  }

  //
  // To get interface name and corresponding Args for the set option.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-s")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-s");
    if (ValueStr == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_LACK_INTERFACE), gShellNetwork1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    //
    // To split the configuration into multi-section.
    //
    ArgList = SplitStrToList (ValueStr, L' ');
    if (ArgList == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellNetwork1HiiHandle, L"ifconfig");
      ShellStatus = SHELL_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    Private->OpCode = IfConfigOpSet;
    Private->IfName = ArgList->Arg;

    Private->VarArg = ArgList->Next;

    if (Private->IfName == NULL || Private->VarArg == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG_LACK_COMMAND), gShellNetwork1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto ON_EXIT;
    }
  }

  //
  // Main process of ifconfig.
  //
  ShellStatus = IfConfig (Private);

ON_EXIT:

  ShellCommandLineFreeVarList (ParamPackage);

  if (Private != NULL) {
    IfConfigCleanup (Private);
  }

  return ShellStatus;
}
