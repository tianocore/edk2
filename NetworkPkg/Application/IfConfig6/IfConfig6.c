/** @file
  The implementation for Shell application IfConfig6.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/ShellLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/NetLib.h>

#include <Protocol/Ip6.h>
#include <Protocol/Ip6Config.h>

#include "IfConfig6.h"

EFI_HII_HANDLE      mHiiHandle;

SHELL_PARAM_ITEM    mIfConfig6CheckList[] = {
  {
    L"-b",
    TypeFlag
  },
  {
    L"-s",
    TypeMaxValue
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
    L"-?",
    TypeFlag
  },
  {
    NULL,
    TypeMax
  },
};

VAR_CHECK_ITEM  mSetCheckList[] = {
  {
   L"auto",
    0x00000001,
    0x00000001,
    FlagTypeSingle
  },
  {
    L"man",
    0x00000002,
    0x00000001,
    FlagTypeSingle
  },
  {
    L"host",
    0x00000004,
    0x00000002,
    FlagTypeSingle
  },
  {
    L"dad",
    0x00000008,
    0x00000004,
    FlagTypeSingle
  },
  {
    L"gw",
    0x00000010,
    0x00000008,
    FlagTypeSingle
  },
  {
    L"dns",
    0x00000020,
    0x00000010,
    FlagTypeSingle
  },
  {
    L"id",
    0x00000040,
    0x00000020,
    FlagTypeSingle
  },
  {
    NULL,
    0x0,
    0x0,
    FlagTypeSkipUnknown
  },
};

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
  Str     = (CHAR16 *) AllocateZeroPool (StrSize (String));
  ASSERT (Str != NULL);
  Str     = StrnCpy (Str, String, StrLen (String));
  ArgStr  = Str;

  //
  // init a node for the list head.
  //
  ArgNode = (ARG_LIST *) AllocateZeroPool (sizeof (ARG_LIST));
  ASSERT (ArgNode != NULL);
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
      ASSERT (ArgNode->Next != NULL);
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

  @return The value of VAR_CHECK_CODE.

**/
VAR_CHECK_CODE
IfConfig6RetriveCheckListByName(
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
IfConfig6ManualAddressNotify (
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
IfConfig6PrintMacAddr (
  IN UINT8     *Node,
  IN UINT32    Size
  )
{
  UINTN    Index;

  ASSERT (Size <= MACADDRMAXSIZE);

  for (Index = 0; Index < Size; Index++) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_MAC_ADDR_BODY), mHiiHandle, Node[Index]);
    if (Index + 1 < Size) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_COLON), mHiiHandle);
    }
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_NEWLINE), mHiiHandle);
}

/**
  Print IPv6 address.

  @param[in]    Ip           The pointer of Ip bufffer in EFI_IPv6_ADDRESS format.
  @param[in]    PrefixLen    The pointer of PrefixLen that describes the size Prefix.

**/
VOID
IfConfig6PrintIpAddr (
  IN EFI_IPv6_ADDRESS    *Ip,
  IN UINT8               *PrefixLen
  )
{
  UINTN      Index;
  BOOLEAN    Short;

  Short = FALSE;

  for (Index = 0; Index < PREFIXMAXLEN; Index = Index + 2) {

    if (!Short && (Index + 1 < PREFIXMAXLEN) && (Index % 2 == 0) && (Ip->Addr[Index] == 0) && (Ip->Addr[Index + 1] == 0)) {
      //
      // Deal with the case of ::.
      //
      if (Index == 0) {
        //
        // :: is at the beginning of the address.
        //
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_COLON), mHiiHandle);
      }
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_COLON), mHiiHandle);

      while ((Ip->Addr[Index] == 0) && (Ip->Addr[Index + 1] == 0) && (Index < PREFIXMAXLEN)) {
        Index = Index + 2;
        if (Index > PREFIXMAXLEN - 2) {
          break;
        }
      }

      Short = TRUE;

      if (Index == PREFIXMAXLEN) {
        //
        // :: is at the end of the address.
        //
        break;
      }
    }

    if (Index < PREFIXMAXLEN - 1) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_IP_ADDR_BODY), mHiiHandle, Ip->Addr[Index]);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_IP_ADDR_BODY), mHiiHandle, Ip->Addr[Index + 1]);
    }

    if (Index + 2 < PREFIXMAXLEN) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_COLON), mHiiHandle);
    }
  }

  if (PrefixLen != NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_PREFIX_LEN), mHiiHandle, *PrefixLen);
  }
}

/**
  Pick up host IPv6 address in string format from Args with "-s" option and convert it to EFI_IP6_CONFIG_MANUAL_ADDRESS format.

  @param[in, out]    Arg        The pointer of the address of ARG_LIST which save Args with the "-s" option.
  @param[out]        Buf        The pointer of the address of EFI_IP6_CONFIG_MANUAL_ADDRESS.
  @param[out]        BufSize    The pointer of BufSize that describes the size of Buf in bytes.

  @retval EFI_SUCCESS    The convertion is successful.
  @retval Others         Does't find the host address, or it is an invalid IPv6 address in string format.

**/
EFI_STATUS
IfConfig6ParseManualAddressList (
  IN OUT ARG_LIST                         **Arg,
     OUT EFI_IP6_CONFIG_MANUAL_ADDRESS    **Buf,
     OUT UINTN                            *BufSize
  )
{
  EFI_STATUS                       Status;
  EFI_IP6_CONFIG_MANUAL_ADDRESS    *AddrBuf;
  ARG_LIST                         *VarArg;
  EFI_IPv6_ADDRESS                 Address;
  UINT8                            Prefix;
  UINT8                            AddrCnt;

  Prefix   = 0;
  AddrCnt  = 0;
  *BufSize = 0;
  *Buf     = NULL;
  VarArg   = *Arg;
  Status   = EFI_SUCCESS;

  //
  // Go through the list to check the correctness of input host ip6 address.
  //
  while ((!EFI_ERROR (Status)) && (VarArg != NULL)) {

    Status = NetLibStrToIp6andPrefix (VarArg->Arg, &Address, &Prefix);

    if (EFI_ERROR (Status)) {
      //
      // host ip ip ... gw
      //
      break;
    }

    VarArg = VarArg->Next;
    AddrCnt++;
  }

  if (AddrCnt == 0) {
    return EFI_INVALID_PARAMETER;
  }

  AddrBuf = AllocateZeroPool (AddrCnt * sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS));
  ASSERT (AddrBuf != NULL);

  AddrCnt = 0;
  VarArg  = *Arg;
  Status  = EFI_SUCCESS;

  //
  // Go through the list to fill in the EFI_IP6_CONFIG_MANUAL_ADDRESS structure.
  //
  while ((!EFI_ERROR (Status)) && (VarArg != NULL)) {

    Status = NetLibStrToIp6andPrefix (VarArg->Arg, &Address, &Prefix);

    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // If prefix length is not set, set it as Zero here. In the IfConfigSetInterfaceInfo()
    // Zero prefix, length will be transfered to default prefix length.
    //
    if (Prefix == 0xFF) {
      Prefix = 0;
    }
    AddrBuf[AddrCnt].IsAnycast    = FALSE;
    AddrBuf[AddrCnt].PrefixLength = Prefix;
    IP6_COPY_ADDRESS (&AddrBuf[AddrCnt].Address, &Address);
    VarArg = VarArg->Next;
    AddrCnt++;
  }

  *Arg = VarArg;

  if (EFI_ERROR (Status) && (Status != EFI_INVALID_PARAMETER)) {
    goto ON_ERROR;
  }

  *Buf     = AddrBuf;
  *BufSize = AddrCnt * sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS);

  return EFI_SUCCESS;

ON_ERROR:

  FreePool (AddrBuf);
  return Status;
}

/**
  Pick up gw/dns IPv6 address in string format from Args with "-s" option and convert it to EFI_IPv6_ADDRESS format.

  @param[in, out]    Arg        The pointer of the address of ARG_LIST that save Args with the "-s" option.
  @param[out]        Buf        The pointer of the address of EFI_IPv6_ADDRESS.
  @param[out]        BufSize    The pointer of BufSize that describes the size of Buf in bytes.

  @retval EFI_SUCCESS    The conversion is successful.
  @retval Others         Doesn't find the host address, or it is an invalid IPv6 address in string format.

**/
EFI_STATUS
IfConfig6ParseGwDnsAddressList (
  IN OUT ARG_LIST            **Arg,
     OUT EFI_IPv6_ADDRESS    **Buf,
     OUT UINTN               *BufSize
  )
{
  EFI_STATUS          Status;
  EFI_IPv6_ADDRESS    *AddrBuf;
  ARG_LIST            *VarArg;
  EFI_IPv6_ADDRESS    Address;
  UINT8               Prefix;
  UINT8               AddrCnt;

  AddrCnt  = 0;
  *BufSize = 0;
  *Buf     = NULL;
  VarArg   = *Arg;
  Status   = EFI_SUCCESS;

  //
  // Go through the list to check the correctness of input gw/dns address.
  //
  while ((!EFI_ERROR (Status)) && (VarArg != NULL)) {

    Status = NetLibStrToIp6andPrefix (VarArg->Arg, &Address, &Prefix);

    if (EFI_ERROR (Status)) {
      //
      // gw ip ip ... host
      //
      break;
    }

    VarArg = VarArg->Next;
    AddrCnt++;
  }

  if (AddrCnt == 0) {
    return EFI_INVALID_PARAMETER;
  }

  AddrBuf = AllocateZeroPool (AddrCnt * sizeof (EFI_IPv6_ADDRESS));
  ASSERT (AddrBuf != NULL);

  AddrCnt = 0;
  VarArg  = *Arg;
  Status  = EFI_SUCCESS;

  //
  // Go through the list to fill in the EFI_IPv6_ADDRESS structure.
  //
  while ((!EFI_ERROR (Status)) && (VarArg != NULL)) {

    Status = NetLibStrToIp6andPrefix (VarArg->Arg, &Address, &Prefix);

    if (EFI_ERROR (Status)) {
      break;
    }

    IP6_COPY_ADDRESS (&AddrBuf[AddrCnt], &Address);

    VarArg = VarArg->Next;
    AddrCnt++;
  }

  *Arg = VarArg;

  if (EFI_ERROR (Status) && (Status != EFI_INVALID_PARAMETER)) {
   goto ON_ERROR;
  }

  *Buf     = AddrBuf;
  *BufSize = AddrCnt * sizeof (EFI_IPv6_ADDRESS);

  return EFI_SUCCESS;

ON_ERROR:

  FreePool (AddrBuf);
  return Status;
}

/**
  Parse InterfaceId in string format from Args with the "-s" option and convert it to EFI_IP6_CONFIG_INTERFACE_ID format.

  @param[in, out]   Arg     The pointer of the address of ARG_LIST that saves Args with the "-s" option.
  @param[out]       IfId    The pointer of EFI_IP6_CONFIG_INTERFACE_ID.

  @retval EFI_SUCCESS              The get status processed successfullly.
  @retval EFI_INVALID_PARAMETER    The get status process failed.

**/
EFI_STATUS
IfConfig6ParseInterfaceId (
  IN OUT ARG_LIST                       **Arg,
     OUT EFI_IP6_CONFIG_INTERFACE_ID    **IfId
  )
{
  UINT8     Index;
  UINT8     NodeVal;
  CHAR16    *IdStr;

  if (*Arg == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Index = 0;
  IdStr = (*Arg)->Arg;
  ASSERT (IfId != NULL);
  *IfId = AllocateZeroPool (sizeof (EFI_IP6_CONFIG_INTERFACE_ID));
  ASSERT (*IfId != NULL);

  while ((*IdStr != L'\0') && (Index < 8)) {

    NodeVal = 0;
    while ((*IdStr != L':') && (*IdStr != L'\0')) {

      if ((*IdStr <= L'F') && (*IdStr >= L'A')) {
        NodeVal = (UINT8)((NodeVal << 4) + *IdStr - L'A' + 10);
      } else if ((*IdStr <= L'f') && (*IdStr >= L'a')) {
        NodeVal = (UINT8)((NodeVal << 4) + *IdStr - L'a' + 10);
      } else if ((*IdStr <= L'9') && (*IdStr >= L'0')) {
        NodeVal = (UINT8)((NodeVal << 4) + *IdStr - L'0');
      } else {
        FreePool (*IfId);
        return EFI_INVALID_PARAMETER;
      }

      IdStr++;
    }

    (*IfId)->Id[Index++] = NodeVal;

    if (*IdStr == L':') {
      IdStr++;
    }
  }

  *Arg = (*Arg)->Next;
  return EFI_SUCCESS;
}

/**
  Parse dad in string format from Args with the "-s" option and convert it to UINT32 format.

  @param[in, out]   Arg      The pointer of the address of ARG_LIST that saves Args with the "-s" option.
  @param[out]       Xmits    The pointer of Xmits.

  @retval EFI_SUCCESS    The get status processed successfully.
  @retval others         The get status process failed.

**/
EFI_STATUS
IfConfig6ParseDadXmits (
  IN OUT ARG_LIST    **Arg,
     OUT UINT32      *Xmits
  )
{
  CHAR16    *ValStr;

  if (*Arg == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ValStr = (*Arg)->Arg;
  *Xmits = 0;

  while (*ValStr != L'\0') {

    if ((*ValStr <= L'9') && (*ValStr >= L'0')) {

      *Xmits = (*Xmits * 10) + (*ValStr - L'0');

    } else {

      return EFI_INVALID_PARAMETER;
    }

    ValStr++;
  }

  *Arg = (*Arg)->Next;
  return EFI_SUCCESS;
}

/**
  The get current status of all handles.

  @param[in]   ImageHandle    The handle of  ImageHandle.
  @param[in]   IfName         The pointer of  IfName(interface name).
  @param[in]   IfList         The pointer of  IfList(interface list).

  @retval EFI_SUCCESS    The get status processed successfully.
  @retval others         The get status process failed.

**/
EFI_STATUS
IfConfig6GetInterfaceInfo (
  IN EFI_HANDLE    ImageHandle,
  IN CHAR16        *IfName,
  IN LIST_ENTRY    *IfList
  )
{
  EFI_STATUS                       Status;
  UINTN                            HandleIndex;
  UINTN                            HandleNum;
  EFI_HANDLE                       *HandleBuffer;
  EFI_IP6_CONFIG_PROTOCOL          *Ip6Cfg;
  EFI_IP6_CONFIG_INTERFACE_INFO    *IfInfo;
  IFCONFIG6_INTERFACE_CB           *IfCb;
  UINTN                            DataSize;

  HandleBuffer = NULL;
  HandleNum    = 0;

  IfInfo       = NULL;
  IfCb         = NULL;

  //
  // Locate all the handles with ip6 service binding protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiIp6ServiceBindingProtocolGuid,
                  NULL,
                  &HandleNum,
                  &HandleBuffer
                 );
  if (EFI_ERROR (Status) || (HandleNum == 0)) {
    return EFI_ABORTED;
  }

  //
  // Enumerate all handles that installed with ip6 service binding protocol.
  //
  for (HandleIndex = 0; HandleIndex < HandleNum; HandleIndex++) {
    IfCb      = NULL;
    IfInfo    = NULL;
    DataSize  = 0;

    //
    // Ip6config protocol and ip6 service binding protocol are installed
    // on the same handle.
    //
    ASSERT (HandleBuffer != NULL);
    Status = gBS->HandleProtocol (
                    HandleBuffer[HandleIndex],
                    &gEfiIp6ConfigProtocolGuid,
                    (VOID **) &Ip6Cfg
                    );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
    //
    // Get the interface information size.
    //
    Status = Ip6Cfg->GetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeInterfaceInfo,
                       &DataSize,
                       NULL
                       );

    if (Status != EFI_BUFFER_TOO_SMALL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_IP6CFG_GETDATA), mHiiHandle, Status);
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
    Status = Ip6Cfg->GetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeInterfaceInfo,
                       &DataSize,
                       IfInfo
                       );

    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_IP6CFG_GETDATA), mHiiHandle, Status);
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
    Status = Ip6Cfg->GetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeDnsServer,
                       &DataSize,
                       NULL
                       );

    if ((Status != EFI_BUFFER_TOO_SMALL) && (Status != EFI_NOT_FOUND)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_IP6CFG_GETDATA), mHiiHandle, Status);
      goto ON_ERROR;
    }

    IfCb = AllocateZeroPool (sizeof (IFCONFIG6_INTERFACE_CB) + DataSize);

    if (IfCb == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    IfCb->NicHandle = HandleBuffer[HandleIndex];
    IfCb->IfInfo    = IfInfo;
    IfCb->IfCfg     = Ip6Cfg;
    IfCb->DnsCnt    = (UINT32) (DataSize / sizeof (EFI_IPv6_ADDRESS));

    //
    // Get the dns server list if has.
    //
    if (DataSize > 0) {

      Status = Ip6Cfg->GetData (
                         Ip6Cfg,
                         Ip6ConfigDataTypeDnsServer,
                         &DataSize,
                         IfCb->DnsAddr
                         );

      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_IP6CFG_GETDATA), mHiiHandle, Status);
        goto ON_ERROR;
      }
    }
    //
    // Get the interface id if has.
    //
    DataSize   = sizeof (EFI_IP6_CONFIG_INTERFACE_ID);
    IfCb->IfId = AllocateZeroPool (DataSize);

    if (IfCb->IfId == NULL) {
      goto ON_ERROR;
    }

    Status = Ip6Cfg->GetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeAltInterfaceId,
                       &DataSize,
                       IfCb->IfId
                       );

    if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_IP6CFG_GETDATA), mHiiHandle, Status);
      goto ON_ERROR;
    }

    if (Status == EFI_NOT_FOUND) {
      FreePool (IfCb->IfId);
      IfCb->IfId = NULL;
    }
    //
    // Get the config policy.
    //
    DataSize = sizeof (EFI_IP6_CONFIG_POLICY);
    Status   = Ip6Cfg->GetData (
                         Ip6Cfg,
                         Ip6ConfigDataTypePolicy,
                         &DataSize,
                         &IfCb->Policy
                         );

    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_IP6CFG_GETDATA), mHiiHandle, Status);
      goto ON_ERROR;
    }
    //
    // Get the dad transmits.
    //
    DataSize = sizeof (EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS);
    Status   = Ip6Cfg->GetData (
                         Ip6Cfg,
                         Ip6ConfigDataTypeDupAddrDetectTransmits,
                         &DataSize,
                         &IfCb->Xmits
                         );

    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_IP6CFG_GETDATA), mHiiHandle, Status);
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
    if (IfCb->IfId != NULL) {
      FreePool (IfCb->IfId);
    }

    FreePool (IfCb);
  }

  return Status;
}

/**
  The list process of the IfConfig6 application.

  @param[in]   IfList    The pointer of IfList(interface list).

  @retval EFI_SUCCESS    The IfConfig6 list processed successfully.
  @retval others         The IfConfig6 list process failed.

**/
EFI_STATUS
IfConfig6ShowInterfaceInfo (
  IN LIST_ENTRY    *IfList
  )
{
  EFI_STATUS                Status;
  LIST_ENTRY                *Entry;
  IFCONFIG6_INTERFACE_CB    *IfCb;
  UINTN                     Index;

  Entry  = IfList->ForwardLink;
  Status = EFI_SUCCESS;

  if (IsListEmpty (IfList)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_INVALID_INTERFACE), mHiiHandle);
  }

  //
  // Go through the interface list.
  //
  while (Entry != IfList) {

    IfCb = BASE_CR (Entry, IFCONFIG6_INTERFACE_CB, Link);

    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_BREAK), mHiiHandle);

    //
    // Print interface name.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_IF_NAME), mHiiHandle, IfCb->IfInfo->Name);

    //
    // Print interface config policy.
    //
    if (IfCb->Policy == Ip6ConfigPolicyAutomatic) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_POLICY_AUTO), mHiiHandle);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_POLICY_MAN), mHiiHandle);
    }

    //
    // Print dad transmit.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_DAD_TRANSMITS), mHiiHandle, IfCb->Xmits);

    //
    // Print interface id if has.
    //
    if (IfCb->IfId != NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_INTERFACE_ID_HEAD), mHiiHandle);

      IfConfig6PrintMacAddr (
        IfCb->IfId->Id,
        8
        );
    }
    //
    // Print mac address of the interface.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_MAC_ADDR_HEAD), mHiiHandle);

    IfConfig6PrintMacAddr (
      IfCb->IfInfo->HwAddress.Addr,
      IfCb->IfInfo->HwAddressSize
      );

    //
    // Print ip addresses list of the interface.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_IP_ADDR_HEAD), mHiiHandle);

    for (Index = 0; Index < IfCb->IfInfo->AddressInfoCount; Index++) {
      IfConfig6PrintIpAddr (
        &IfCb->IfInfo->AddressInfo[Index].Address,
        &IfCb->IfInfo->AddressInfo[Index].PrefixLength
        );
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_NEWLINE), mHiiHandle);
    }

    //
    // Print dns server addresses list of the interface if has.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_DNS_ADDR_HEAD), mHiiHandle);

    for (Index = 0; Index < IfCb->DnsCnt; Index++) {
      IfConfig6PrintIpAddr (
        &IfCb->DnsAddr[Index],
        NULL
        );
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_NEWLINE), mHiiHandle);
    }

    //
    // Print route table of the interface if has.
    //
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_ROUTE_HEAD), mHiiHandle);

    for (Index = 0; Index < IfCb->IfInfo->RouteCount; Index++) {
      IfConfig6PrintIpAddr (
        &IfCb->IfInfo->RouteTable[Index].Destination,
        &IfCb->IfInfo->RouteTable[Index].PrefixLength
        );
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_JOINT), mHiiHandle);

      IfConfig6PrintIpAddr (
        &IfCb->IfInfo->RouteTable[Index].Gateway,
        NULL
        );
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_NEWLINE), mHiiHandle);
    }

    Entry = Entry->ForwardLink;
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_INFO_BREAK), mHiiHandle);

  return Status;
}

/**
  The clean process of the IfConfig6 application.

  @param[in]   IfList    The pointer of IfList(interface list).

  @retval EFI_SUCCESS    The IfConfig6 clean processed successfully.
  @retval others         The IfConfig6 clean process failed.

**/
EFI_STATUS
IfConfig6ClearInterfaceInfo (
  IN LIST_ENTRY    *IfList
  )
{
  EFI_STATUS                Status;
  LIST_ENTRY                *Entry;
  IFCONFIG6_INTERFACE_CB    *IfCb;
  EFI_IP6_CONFIG_POLICY     Policy;

  Policy = Ip6ConfigPolicyAutomatic;
  Entry  = IfList->ForwardLink;
  Status = EFI_SUCCESS;

  if (IsListEmpty (IfList)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_INVALID_INTERFACE), mHiiHandle);
  }

  //
  // Go through the interface list.
  //
  while (Entry != IfList) {

    IfCb = BASE_CR (Entry, IFCONFIG6_INTERFACE_CB, Link);

    Status = IfCb->IfCfg->SetData (
                            IfCb->IfCfg,
                            Ip6ConfigDataTypePolicy,
                            sizeof (EFI_IP6_CONFIG_POLICY),
                            &Policy
                            );

    if (EFI_ERROR (Status)) {
      break;
    }

    Entry  = Entry->ForwardLink;
  }

  return Status;
}

/**
  The set process of the IfConfig6 application.

  @param[in]   IfList    The pointer of IfList(interface list).
  @param[in]   VarArg    The pointer of ARG_LIST(Args with "-s" option).

  @retval EFI_SUCCESS    The IfConfig6 set processed successfully.
  @retval others         The IfConfig6 set process failed.

**/
EFI_STATUS
IfConfig6SetInterfaceInfo (
  IN LIST_ENTRY    *IfList,
  IN ARG_LIST      *VarArg
  )
{
  EFI_STATUS                       Status;
  IFCONFIG6_INTERFACE_CB           *IfCb;
  EFI_IP6_CONFIG_MANUAL_ADDRESS    *CfgManAddr;
  EFI_IPv6_ADDRESS                 *CfgAddr;
  UINTN                            AddrSize;
  EFI_IP6_CONFIG_INTERFACE_ID      *InterfaceId;
  UINT32                           DadXmits;
  UINT32                           CurDadXmits;
  UINTN                            CurDadXmitsLen;
  EFI_IP6_CONFIG_POLICY            Policy;

  VAR_CHECK_CODE                   CheckCode;
  EFI_EVENT                        TimeOutEvt;
  EFI_EVENT                        MappedEvt;
  BOOLEAN                          IsAddressOk;

  UINTN                            DataSize;
  UINT32                           Index;
  UINT32                           Index2;
  BOOLEAN                          IsAddressSet;
  EFI_IP6_CONFIG_INTERFACE_INFO    *IfInfo;

  CfgManAddr  = NULL;
  CfgAddr     = NULL;
  TimeOutEvt  = NULL;
  MappedEvt   = NULL;
  IfInfo      = NULL;
  InterfaceId = NULL;
  CurDadXmits = 0;

  if (IsListEmpty (IfList)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_INVALID_INTERFACE), mHiiHandle);
    return EFI_INVALID_PARAMETER;
  }
  //
  // Make sure to set only one interface each time.
  //
  IfCb   = BASE_CR (IfList->ForwardLink, IFCONFIG6_INTERFACE_CB, Link);
  Status = EFI_SUCCESS;

  //
  // Initialize check list mechanism.
  //
  CheckCode = IfConfig6RetriveCheckListByName(
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
    goto ON_EXIT;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IfConfig6ManualAddressNotify,
                  &IsAddressOk,
                  &MappedEvt
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }
  //
  // Parse the setting variables.
  //
  while (VarArg != NULL) {
     //
     // Check invalid parameters (duplication & unknown & conflict).
     //
    CheckCode = IfConfig6RetriveCheckListByName(
                  mSetCheckList,
                  VarArg->Arg,
                  FALSE
                  );

    if (VarCheckOk != CheckCode) {
      switch (CheckCode) {
        case VarCheckDuplicate:
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_DUPLICATE_COMMAND), mHiiHandle, VarArg->Arg);
          break;

        case VarCheckConflict:
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_CONFLICT_COMMAND), mHiiHandle, VarArg->Arg);
          break;

        case VarCheckUnknown:
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_UNKNOWN_COMMAND), mHiiHandle, VarArg->Arg);
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
    if (StrCmp(VarArg->Arg, L"auto") == 0) {
      //
      // Set automaic config policy
      //
      Policy = Ip6ConfigPolicyAutomatic;
      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip6ConfigDataTypePolicy,
                              sizeof (EFI_IP6_CONFIG_POLICY),
                              &Policy
                              );

      if (EFI_ERROR(Status)) {
        goto ON_EXIT;
      }

      VarArg= VarArg->Next;

    } else if (StrCmp (VarArg->Arg, L"man") == 0) {
      //
      // Set manual config policy.
      //
      Policy = Ip6ConfigPolicyManual;
      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip6ConfigDataTypePolicy,
                              sizeof (EFI_IP6_CONFIG_POLICY),
                              &Policy
                              );

      if (EFI_ERROR(Status)) {
        goto ON_EXIT;
      }

      VarArg= VarArg->Next;

    } else if (StrCmp (VarArg->Arg, L"host") == 0) {
      //
      // Parse till the next tag or the end of command line.
      //
      VarArg = VarArg->Next;
      Status = IfConfig6ParseManualAddressList (
                 &VarArg,
                 &CfgManAddr,
                 &AddrSize
                 );

      if (EFI_ERROR (Status)) {
        if (Status == EFI_INVALID_PARAMETER) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_LACK_ARGUMENTS), mHiiHandle, L"host");
          continue;
        } else {
          goto ON_EXIT;
        }
      }
      //
      // Set static host ip6 address list.
      //   This is a asynchronous process.
      //
      IsAddressOk = FALSE;

      Status = IfCb->IfCfg->RegisterDataNotify (
                              IfCb->IfCfg,
                              Ip6ConfigDataTypeManualAddress,
                              MappedEvt
                              );
      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip6ConfigDataTypeManualAddress,
                              AddrSize,
                              CfgManAddr
                              );

      if (Status == EFI_NOT_READY) {
        //
        // Get current dad transmits count.
        //
        CurDadXmitsLen = sizeof (EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS);
        IfCb->IfCfg->GetData (
                       IfCb->IfCfg,
                       Ip6ConfigDataTypeDupAddrDetectTransmits,
                       &CurDadXmitsLen,
                       &CurDadXmits
                       );

        gBS->SetTimer (TimeOutEvt, TimerRelative, 50000000 + 10000000 * CurDadXmits);

        while (EFI_ERROR (gBS->CheckEvent (TimeOutEvt))) {
          if (IsAddressOk) {
            Status = EFI_SUCCESS;
            break;
          }
        }
      }

      IfCb->IfCfg->UnregisterDataNotify (
                     IfCb->IfCfg,
                     Ip6ConfigDataTypeManualAddress,
                     MappedEvt
                     );

      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_MAN_HOST), mHiiHandle, Status);
        goto ON_EXIT;
      }

      //
      // Check whether the address is set successfully.
      //
      DataSize = 0;

      Status = IfCb->IfCfg->GetData (
                              IfCb->IfCfg,
                              Ip6ConfigDataTypeInterfaceInfo,
                              &DataSize,
                              NULL
                              );

      if (Status != EFI_BUFFER_TOO_SMALL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_IP6CFG_GETDATA), mHiiHandle, Status);
        goto ON_EXIT;
      }

      IfInfo = AllocateZeroPool (DataSize);

      if (IfInfo == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }

      Status = IfCb->IfCfg->GetData (
                              IfCb->IfCfg,
                              Ip6ConfigDataTypeInterfaceInfo,
                              &DataSize,
                              IfInfo
                              );

      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_IP6CFG_GETDATA), mHiiHandle, Status);
        goto ON_EXIT;
      }

      for ( Index = 0; Index < (UINTN) (AddrSize / sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS)); Index++) {
        IsAddressSet = FALSE;
        //
        // By default, the prefix length 0 is regarded as 64.
        //
        if (CfgManAddr[Index].PrefixLength == 0) {
          CfgManAddr[Index].PrefixLength = 64;
        }

        for (Index2 = 0; Index2 < IfInfo->AddressInfoCount; Index2++) {
          if (EFI_IP6_EQUAL (&IfInfo->AddressInfo[Index2].Address, &CfgManAddr[Index].Address) &&
              (IfInfo->AddressInfo[Index2].PrefixLength == CfgManAddr[Index].PrefixLength)) {
            IsAddressSet = TRUE;
            break;
          }
        }

        if (!IsAddressSet) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_ADDRESS_FAILED), mHiiHandle);
          IfConfig6PrintIpAddr (
            &CfgManAddr[Index].Address,
            &CfgManAddr[Index].PrefixLength
            );
        }
      }

    } else if (StrCmp (VarArg->Arg, L"gw") == 0) {
      //
      // Parse till the next tag or the end of command line.
      //
      VarArg = VarArg->Next;
      Status = IfConfig6ParseGwDnsAddressList (
                 &VarArg,
                 &CfgAddr,
                 &AddrSize
                 );

      if (EFI_ERROR (Status)) {
        if (Status == EFI_INVALID_PARAMETER) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_LACK_ARGUMENTS), mHiiHandle, L"gw");
          continue;
        } else {
          goto ON_EXIT;
        }
      }
      //
      // Set static gateway ip6 address list.
      //
      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip6ConfigDataTypeGateway,
                              AddrSize,
                              CfgAddr
                              );

      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

    } else if (StrCmp (VarArg->Arg, L"dns") == 0) {
      //
      // Parse till the next tag or the end of command line.
      //
      VarArg = VarArg->Next;
      Status = IfConfig6ParseGwDnsAddressList (
                 &VarArg,
                 &CfgAddr,
                 &AddrSize
                 );

      if (EFI_ERROR (Status)) {
        if (Status == EFI_INVALID_PARAMETER) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_LACK_ARGUMENTS), mHiiHandle, L"dns");
          continue;
        } else {
          goto ON_EXIT;
        }
      }
      //
      // Set static dhs server ip6 address list.
      //
      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip6ConfigDataTypeDnsServer,
                              AddrSize,
                              CfgAddr
                              );

      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

    } else if (StrCmp (VarArg->Arg, L"id") == 0) {
      //
      // Parse till the next tag or the end of command line.
      //
      VarArg = VarArg->Next;
      Status = IfConfig6ParseInterfaceId (&VarArg, &InterfaceId);

      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }
      //
      // Set alternative interface id.
      //
      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip6ConfigDataTypeAltInterfaceId,
                              sizeof (EFI_IP6_CONFIG_INTERFACE_ID),
                              InterfaceId
                              );

      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

    } else if (StrCmp (VarArg->Arg, L"dad") == 0) {
      //
      // Parse till the next tag or the end of command line.
      //
      VarArg = VarArg->Next;
      Status = IfConfig6ParseDadXmits (&VarArg, &DadXmits);

      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }
      //
      // Set dad transmits count.
      //
      Status = IfCb->IfCfg->SetData (
                              IfCb->IfCfg,
                              Ip6ConfigDataTypeDupAddrDetectTransmits,
                              sizeof (EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS),
                              &DadXmits
                              );

      if (EFI_ERROR(Status)) {
        goto ON_EXIT;
      }
    }
  }

ON_EXIT:

  if (CfgManAddr != NULL) {
    FreePool (CfgManAddr);
  }

  if (CfgAddr != NULL) {
    FreePool (CfgAddr);
  }

  if (MappedEvt != NULL) {
    gBS->CloseEvent (MappedEvt);
  }

  if (TimeOutEvt != NULL) {
    gBS->CloseEvent (TimeOutEvt);
  }

  if (IfInfo != NULL) {
    FreePool (IfInfo);
  }

  return Status;

}

/**
  The IfConfig6 main process.

  @param[in]   Private    The pointer of IFCONFIG6_PRIVATE_DATA.

  @retval EFI_SUCCESS    IfConfig6 processed successfully.
  @retval others         The IfConfig6 process failed.

**/
EFI_STATUS
IfConfig6 (
  IN IFCONFIG6_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS    Status;

  //
  // Get configure information of all interfaces.
  //
  Status = IfConfig6GetInterfaceInfo (
             Private->ImageHandle,
             Private->IfName,
             &Private->IfList
             );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  switch (Private->OpCode) {
  case IfConfig6OpList:
    Status = IfConfig6ShowInterfaceInfo (&Private->IfList);
    break;

  case IfConfig6OpClear:
    Status = IfConfig6ClearInterfaceInfo (&Private->IfList);
    break;

  case IfConfig6OpSet:
    Status = IfConfig6SetInterfaceInfo (&Private->IfList, Private->VarArg);
    break;

  default:
    Status = EFI_ABORTED;
  }

ON_EXIT:

  return Status;
}

/**
  The IfConfig6 cleanup process, free the allocated memory.

  @param[in]   Private    The pointer of  IFCONFIG6_PRIVATE_DATA.

**/
VOID
IfConfig6Cleanup (
  IN IFCONFIG6_PRIVATE_DATA    *Private
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *NextEntry;
  IFCONFIG6_INTERFACE_CB    *IfCb;
  ARG_LIST                  *ArgNode;
  ARG_LIST                  *ArgHead;

  ASSERT (Private != NULL);

  //
  // Clean the list which save the set config Args.
  //
  if (Private->VarArg != NULL) {
    ArgHead = Private->VarArg;

    while (ArgHead->Next != NULL) {
      ArgNode = ArgHead->Next;
      FreePool (ArgHead);
      ArgHead = ArgNode;
    }

    FreePool (ArgHead);
  }

  if (Private->IfName != NULL)
    FreePool (Private->IfName);


  //
  // Clean the IFCONFIG6_INTERFACE_CB list.
  //
  Entry     = Private->IfList.ForwardLink;
  NextEntry = Entry->ForwardLink;

  while (Entry != &Private->IfList) {

    IfCb = BASE_CR (Entry, IFCONFIG6_INTERFACE_CB, Link);

    RemoveEntryList (&IfCb->Link);

    if (IfCb->IfId != NULL) {

      FreePool (IfCb->IfId);
    }

    if (IfCb->IfInfo != NULL) {

      FreePool (IfCb->IfInfo);
    }

    FreePool (IfCb);

    Entry     = NextEntry;
    NextEntry = Entry->ForwardLink;
  }

  FreePool (Private);
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers, including
  both device drivers and bus drivers.

  The entry point for the IfConfig6 application which parses the command line input and calls the IfConfig6 process.

  @param[in] ImageHandle    The image handle of this application.
  @param[in] SystemTable    The pointer to the EFI System Table.

  @retval EFI_SUCCESS    The operation completed successfully.
  @retval Others         Some errors occur.

**/
EFI_STATUS
EFIAPI
IfConfig6Initialize (
  IN  EFI_HANDLE         ImageHandle,
  IN  EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                Status;
  IFCONFIG6_PRIVATE_DATA    *Private;
  LIST_ENTRY                *ParamPackage;
  CONST CHAR16              *ValueStr;
  ARG_LIST                  *ArgList;
  CHAR16                    *ProblemParam;
  CHAR16                    *Str;

  Private = NULL;

  //
  // Register our string package with HII and return the handle to it.
  //
  mHiiHandle = HiiAddPackages (&gEfiCallerIdGuid, ImageHandle, IfConfig6Strings, NULL);
  ASSERT (mHiiHandle != NULL);

  Status = ShellCommandLineParseEx (mIfConfig6CheckList, &ParamPackage, &ProblemParam, TRUE, FALSE);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_INVALID_COMMAND), mHiiHandle, ProblemParam);
    goto ON_EXIT;
  }

  //
  // To handle no option.
  //
  if (!ShellCommandLineGetFlag (ParamPackage, L"-r") && !ShellCommandLineGetFlag (ParamPackage, L"-s") &&
      !ShellCommandLineGetFlag (ParamPackage, L"-?") && !ShellCommandLineGetFlag (ParamPackage, L"-l")) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_LACK_OPTION), mHiiHandle);
    goto ON_EXIT;
  }
  //
  // To handle conflict options.
  //
  if (((ShellCommandLineGetFlag (ParamPackage, L"-r")) && (ShellCommandLineGetFlag (ParamPackage, L"-s"))) ||
      ((ShellCommandLineGetFlag (ParamPackage, L"-r")) && (ShellCommandLineGetFlag (ParamPackage, L"-l"))) ||
      ((ShellCommandLineGetFlag (ParamPackage, L"-r")) && (ShellCommandLineGetFlag (ParamPackage, L"-?"))) ||
      ((ShellCommandLineGetFlag (ParamPackage, L"-s")) && (ShellCommandLineGetFlag (ParamPackage, L"-l"))) ||
      ((ShellCommandLineGetFlag (ParamPackage, L"-s")) && (ShellCommandLineGetFlag (ParamPackage, L"-?"))) ||
      ((ShellCommandLineGetFlag (ParamPackage, L"-l")) && (ShellCommandLineGetFlag (ParamPackage, L"-?")))) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_CONFLICT_OPTIONS), mHiiHandle);
    goto ON_EXIT;
  }
  //
  // To show the help information of ifconfig6 command.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-?")) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_HELP), mHiiHandle);
    goto ON_EXIT;
  }

  Status = EFI_INVALID_PARAMETER;

  Private = AllocateZeroPool (sizeof (IFCONFIG6_PRIVATE_DATA));

  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  InitializeListHead (&Private->IfList);

  //
  // To get interface name for the list option.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-l")) {
    Private->OpCode = IfConfig6OpList;
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-l");
    if (ValueStr != NULL) {
      Str             = (CHAR16 *) AllocateZeroPool (StrSize (ValueStr));
      ASSERT (Str != NULL);

      Str             = StrnCpy (Str, ValueStr, StrLen (ValueStr));
      Private->IfName = Str;
    }
  }
  //
  // To get interface name for the clear option.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-r")) {
    Private->OpCode = IfConfig6OpClear;
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-r");
    if (ValueStr != NULL) {
      Str             = (CHAR16 *) AllocateZeroPool (StrSize (ValueStr));
      ASSERT (Str != NULL);

      Str             = StrnCpy (Str, ValueStr, StrLen (ValueStr));
      Private->IfName = Str;
    }
  }
  //
  // To get interface name and corresponding Args for the set option.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-s")) {

    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-s");
    if (ValueStr == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_LACK_INTERFACE), mHiiHandle);
      goto ON_EXIT;
    }
    //
    // To split the configuration into multi-section.
    //
    ArgList         = SplitStrToList (ValueStr, L' ');
    ASSERT (ArgList != NULL);

    Private->OpCode = IfConfig6OpSet;
    Private->IfName = ArgList->Arg;

    Private->VarArg = ArgList->Next;

    if (Private->IfName == NULL || Private->VarArg == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IFCONFIG6_ERR_LACK_COMMAND), mHiiHandle);
      goto ON_EXIT;
    }
  }
  //
  // Main process of ifconfig6.
  //
  Status = IfConfig6 (Private);

ON_EXIT:

  ShellCommandLineFreeVarList (ParamPackage);
  HiiRemovePackages (mHiiHandle);
  if (Private != NULL)
    IfConfig6Cleanup (Private);

  return Status;
}

