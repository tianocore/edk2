/** @file
  The implementation of dump policy entry function in IpSecConfig application.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecConfig.h"
#include "Dump.h"
#include "ForEach.h"
#include "Helper.h"

/**
  Private function called to get the version infomation from an EFI_IP_ADDRESS_INFO structure.

  @param[in] AddressInfo    The pointer to the EFI_IP_ADDRESS_INFO structure.

  @return the value of version.
**/
UINTN
GetVerFromAddrInfo (
  IN EFI_IP_ADDRESS_INFO    *AddressInfo
)
{
  if((AddressInfo->PrefixLength <= 32) && (AddressInfo->Address.Addr[1] == 0) &&
     (AddressInfo->Address.Addr[2] == 0) && (AddressInfo->Address.Addr[3] == 0)) {
    return IP_VERSION_4;
  } else {
    return IP_VERSION_6;
  }
}

/**
  Private function called to get the version information from a EFI_IP_ADDRESS structure.

  @param[in] Address    The pointer to the EFI_IP_ADDRESS structure.

  @return The value of the version.
**/
UINTN
GetVerFromIpAddr (
  IN EFI_IP_ADDRESS    *Address
)
{
  if ((Address->Addr[1] == 0) && (Address->Addr[2] == 0) && (Address->Addr[3] == 0)) {
    return IP_VERSION_4;
  } else {
    return IP_VERSION_6;
  }
}

/**
  Private function called to print an ASCII string in unicode char format.

  @param[in] Str       The pointer to the ASCII string.
  @param[in] Length    The value of the ASCII string length.
**/
VOID
DumpAsciiString (
  IN CHAR8    *Str,
  IN UINTN    Length
  )
{
  UINTN    Index;
  Print (L"\"");
  for (Index = 0; Index < Length; Index++) {
    Print (L"%c", (CHAR16) Str[Index]);
  }
  Print (L"\"");
}

/**
  Private function called to print a buffer in Hex format.

  @param[in] Data      The pointer to the buffer.
  @param[in] Length    The size of the buffer.

**/
VOID
DumpBuf (
  IN UINT8    *Data,
  IN UINTN    Length
  )
{
  UINTN    Index;
  for (Index = 0; Index < Length; Index++) {
    Print (L"%02x ", Data[Index]);
  }
}

/**
  Private function called to print EFI_IP_ADDRESS_INFO content.

  @param[in] AddressInfo    The pointer to the EFI_IP_ADDRESS_INFO structure.
**/
VOID
DumpAddressInfo (
  IN EFI_IP_ADDRESS_INFO    *AddressInfo
  )
{
  if (IP_VERSION_4 == GetVerFromAddrInfo (AddressInfo)) {
    Print (
      L"%d.%d.%d.%d",
      (UINTN) AddressInfo->Address.v4.Addr[0],
      (UINTN) AddressInfo->Address.v4.Addr[1],
      (UINTN) AddressInfo->Address.v4.Addr[2],
      (UINTN) AddressInfo->Address.v4.Addr[3]
      );
    if (AddressInfo->PrefixLength != 32) {
      Print (L"/%d", (UINTN) AddressInfo->PrefixLength);
    }
  }

  if (IP_VERSION_6 == GetVerFromAddrInfo (AddressInfo)) {
    Print (
      L"%x:%x:%x:%x:%x:%x:%x:%x",
      (((UINT16) AddressInfo->Address.v6.Addr[0]) << 8) | ((UINT16) AddressInfo->Address.v6.Addr[1]),
      (((UINT16) AddressInfo->Address.v6.Addr[2]) << 8) | ((UINT16) AddressInfo->Address.v6.Addr[3]),
      (((UINT16) AddressInfo->Address.v6.Addr[4]) << 8) | ((UINT16) AddressInfo->Address.v6.Addr[5]),
      (((UINT16) AddressInfo->Address.v6.Addr[6]) << 8) | ((UINT16) AddressInfo->Address.v6.Addr[7]),
      (((UINT16) AddressInfo->Address.v6.Addr[8]) << 8) | ((UINT16) AddressInfo->Address.v6.Addr[9]),
      (((UINT16) AddressInfo->Address.v6.Addr[10]) << 8) | ((UINT16) AddressInfo->Address.v6.Addr[11]),
      (((UINT16) AddressInfo->Address.v6.Addr[12]) << 8) | ((UINT16) AddressInfo->Address.v6.Addr[13]),
      (((UINT16) AddressInfo->Address.v6.Addr[14]) << 8) | ((UINT16) AddressInfo->Address.v6.Addr[15])
      );
    if (AddressInfo->PrefixLength != 128) {
      Print (L"/%d", AddressInfo->PrefixLength);
    }
  }
}

/**
  Private function called to print EFI_IP_ADDRESS content.

  @param[in] IpAddress    The pointer to the EFI_IP_ADDRESS structure.
**/
VOID
DumpIpAddress (
  IN EFI_IP_ADDRESS    *IpAddress
  )
{
  if (IP_VERSION_4 == GetVerFromIpAddr (IpAddress)) {
    Print (
      L"%d.%d.%d.%d",
      (UINTN) IpAddress->v4.Addr[0],
      (UINTN) IpAddress->v4.Addr[1],
      (UINTN) IpAddress->v4.Addr[2],
      (UINTN) IpAddress->v4.Addr[3]
      );
  }

  if (IP_VERSION_6 == GetVerFromIpAddr (IpAddress)) {
    Print (
      L"%x:%x:%x:%x:%x:%x:%x:%x",
      (((UINT16) IpAddress->v6.Addr[0]) << 8) | ((UINT16) IpAddress->v6.Addr[1]),
      (((UINT16) IpAddress->v6.Addr[2]) << 8) | ((UINT16) IpAddress->v6.Addr[3]),
      (((UINT16) IpAddress->v6.Addr[4]) << 8) | ((UINT16) IpAddress->v6.Addr[5]),
      (((UINT16) IpAddress->v6.Addr[6]) << 8) | ((UINT16) IpAddress->v6.Addr[7]),
      (((UINT16) IpAddress->v6.Addr[8]) << 8) | ((UINT16) IpAddress->v6.Addr[9]),
      (((UINT16) IpAddress->v6.Addr[10]) << 8) | ((UINT16) IpAddress->v6.Addr[11]),
      (((UINT16) IpAddress->v6.Addr[12]) << 8) | ((UINT16) IpAddress->v6.Addr[13]),
      (((UINT16) IpAddress->v6.Addr[14]) << 8) | ((UINT16) IpAddress->v6.Addr[15])
      );
  }

}

/**
  Private function called to print EFI_IPSEC_SPD_SELECTOR content.

  @param[in] Selector    The pointer to the EFI_IPSEC_SPD_SELECTOR structure.
**/
VOID
DumpSpdSelector (
  IN EFI_IPSEC_SPD_SELECTOR    *Selector
  )
{
  UINT32    Index;
  CHAR16    *Str;

  for (Index = 0; Index < Selector->LocalAddressCount; Index++) {
    if (Index > 0) {
      Print (L",");
    }

    DumpAddressInfo (&Selector->LocalAddress[Index]);
  }

  if (Index == 0) {
    Print (L"localhost");
  }

  Print (L" -> ");

  for (Index = 0; Index < Selector->RemoteAddressCount; Index++) {
    if (Index > 0) {
      Print (L",");
    }

    DumpAddressInfo (&Selector->RemoteAddress[Index]);
  }

  Str = MapIntegerToString (Selector->NextLayerProtocol, mMapIpProtocol);
  if (Str != NULL) {
    Print (L" %s", Str);
  } else {
    Print (L" proto:%d", (UINTN) Selector->NextLayerProtocol);
  }

  if ((Selector->NextLayerProtocol == EFI_IP4_PROTO_TCP) || (Selector->NextLayerProtocol == EFI_IP4_PROTO_UDP)) {
    Print (L" port:");
    if (Selector->LocalPort != EFI_IPSEC_ANY_PORT) {
      Print (L"%d", Selector->LocalPort);
      if (Selector->LocalPortRange != 0) {
        Print (L"~%d", (UINTN) Selector->LocalPort + Selector->LocalPortRange);
      }
    } else {
      Print (L"any");
    }

    Print (L" -> ");
    if (Selector->RemotePort != EFI_IPSEC_ANY_PORT) {
      Print (L"%d", Selector->RemotePort);
      if (Selector->RemotePortRange != 0) {
        Print (L"~%d", (UINTN) Selector->RemotePort + Selector->RemotePortRange);
      }
    } else {
      Print (L"any");
    }
  } else if (Selector->NextLayerProtocol == EFI_IP4_PROTO_ICMP) {
    Print (L" class/code:");
    if (Selector->LocalPort != 0) {
      Print (L"%d", (UINTN) (UINT8) Selector->LocalPort);
    } else {
      Print (L"any");
    }

    Print (L"/");
    if (Selector->RemotePort != 0) {
      Print (L"%d", (UINTN) (UINT8) Selector->RemotePort);
    } else {
      Print (L"any");
    }
  }
}

/**
  Print EFI_IPSEC_SPD_SELECTOR and EFI_IPSEC_SPD_DATA content.

  @param[in] Selector      The pointer to the EFI_IPSEC_SPD_SELECTOR structure.
  @param[in] Data          The pointer to the EFI_IPSEC_SPD_DATA structure.
  @param[in] EntryIndex    The pointer to the Index in SPD Database.

  @retval EFI_SUCCESS    Dump SPD information successfully.
**/
EFI_STATUS
DumpSpdEntry (
  IN EFI_IPSEC_SPD_SELECTOR    *Selector,
  IN EFI_IPSEC_SPD_DATA        *Data,
  IN UINTN                     *EntryIndex
  )
{
  BOOLEAN    HasPre;
  CHAR16     DataName[128];
  CHAR16     *String1;
  CHAR16     *String2;
  CHAR16     *String3;
  UINT8      Index;

  Print (L"%d.", (*EntryIndex)++);

  //
  // xxx.xxx.xxx.xxx/yy -> xxx.xxx.xxx.xx/yy proto:23 port:100~300 -> 300~400
  // Protect  PF:0x34323423 Name:First Entry
  // ext-sequence sequence-overflow fragcheck life:[B0,S1024,H3600]
  // ESP algo1 algo2 Tunnel [xxx.xxx.xxx.xxx xxx.xxx.xxx.xxx set]
  //

  DumpSpdSelector (Selector);
  Print (L"\n  ");

  Print (L"%s ", MapIntegerToString (Data->Action, mMapIpSecAction));
  Print (L"PF:%08x ", Data->PackageFlag);

  Index = 0;
  while (Data->Name[Index] != 0) {
    DataName[Index] = (CHAR16) Data->Name[Index];
    Index++;
    ASSERT (Index < 128);
  }
  DataName[Index] = L'\0';

  Print (L"Name:%s", DataName);

  if (Data->Action == EfiIPsecActionProtect) {
    Print (L"\n  ");
    if (Data->ProcessingPolicy->ExtSeqNum) {
      Print (L"ext-sequence ");
    }

    if (Data->ProcessingPolicy->SeqOverflow) {
      Print (L"sequence-overflow ");
    }

    if (Data->ProcessingPolicy->FragCheck) {
      Print (L"fragment-check ");
    }

    HasPre = FALSE;
    if (Data->ProcessingPolicy->SaLifetime.ByteCount != 0) {
      Print (HasPre ? L"," : L"life:[");
      Print (L"%lxB", Data->ProcessingPolicy->SaLifetime.ByteCount);
      HasPre = TRUE;
    }

    if (Data->ProcessingPolicy->SaLifetime.SoftLifetime != 0) {
      Print (HasPre ? L"," : L"life:[");
      Print (L"%lxs", Data->ProcessingPolicy->SaLifetime.SoftLifetime);
      HasPre = TRUE;
    }

    if (Data->ProcessingPolicy->SaLifetime.HardLifetime != 0) {
      Print (HasPre ? L"," : L"life:[");
      Print (L"%lxS", Data->ProcessingPolicy->SaLifetime.HardLifetime);
      HasPre = TRUE;
    }

    if (HasPre) {
      Print (L"]");
    }

    if (HasPre || Data->ProcessingPolicy->ExtSeqNum ||
        Data->ProcessingPolicy->SeqOverflow || Data->ProcessingPolicy->FragCheck) {
      Print (L"\n  ");
    }

    String1 = MapIntegerToString (Data->ProcessingPolicy->Proto, mMapIpSecProtocol);
    String2 = MapIntegerToString (Data->ProcessingPolicy->AuthAlgoId, mMapAuthAlgo);
    String3 = MapIntegerToString (Data->ProcessingPolicy->EncAlgoId, mMapEncAlgo);
    Print (
      L"%s Auth:%s Encrypt:%s ",
      String1,
      String2,
      String3
      );

    Print (L"%s ", MapIntegerToString (Data->ProcessingPolicy->Mode, mMapIpSecMode));
    if (Data->ProcessingPolicy->Mode == EfiIPsecTunnel) {
      Print (L"[");
      DumpIpAddress (&Data->ProcessingPolicy->TunnelOption->LocalTunnelAddress);
      Print (L" -> ");
      DumpIpAddress (&Data->ProcessingPolicy->TunnelOption->RemoteTunnelAddress);
      Print (L" %s]", MapIntegerToString (Data->ProcessingPolicy->TunnelOption->DF, mMapDfOption));
    }
  }

  Print (L"\n");

  return EFI_SUCCESS;
}

/**
  Print EFI_IPSEC_SA_ID and EFI_IPSEC_SA_DATA2 content.

  @param[in] SaId          The pointer to the EFI_IPSEC_SA_ID structure.
  @param[in] Data          The pointer to the EFI_IPSEC_SA_DATA2 structure.
  @param[in] EntryIndex    The pointer to the Index in the SAD Database.

  @retval EFI_SUCCESS    Dump SAD information successfully.
**/
EFI_STATUS
DumpSadEntry (
  IN EFI_IPSEC_SA_ID      *SaId,
  IN EFI_IPSEC_SA_DATA2   *Data,
  IN UINTN                *EntryIndex
  )
{
  BOOLEAN    HasPre;
  CHAR16     *AuthAlgoStr;
  CHAR16     *EncAlgoStr;

  AuthAlgoStr      = NULL;
  EncAlgoStr       = NULL;

  //
  // SPI:1234 ESP Destination:xxx.xxx.xxx.xxx
  //  Mode:Transport SeqNum:134 AntiReplayWin:64 life:[0B,1023s,3400S] PathMTU:34
  //  Auth:xxxx/password Encrypt:yyyy/password
  //  xxx.xxx.xxx.xxx/yy -> xxx.xxx.xxx.xx/yy proto:23 port:100~300 -> 300~400
  //

  Print (L"%d.", (*EntryIndex)++);
  Print (L"0x%x %s ", (UINTN) SaId->Spi, MapIntegerToString (SaId->Proto, mMapIpSecProtocol));
  if (Data->Mode == EfiIPsecTunnel) {
    Print (L"TunnelSourceAddress:");
    DumpIpAddress (&Data->TunnelSourceAddress);
    Print (L"\n");
    Print (L"  TunnelDestination:");
    DumpIpAddress (&Data->TunnelDestinationAddress);
    Print (L"\n");
  }

  Print (
    L"  Mode:%s SeqNum:%lx AntiReplayWin:%d ",
    MapIntegerToString (Data->Mode, mMapIpSecMode),
    Data->SNCount,
    (UINTN) Data->AntiReplayWindows
    );

  HasPre = FALSE;
  if (Data->SaLifetime.ByteCount != 0) {
    Print (HasPre ? L"," : L"life:[");
    Print (L"%lxB", Data->SaLifetime.ByteCount);
    HasPre = TRUE;
  }

  if (Data->SaLifetime.SoftLifetime != 0) {
    Print (HasPre ? L"," : L"life:[");
    Print (L"%lxs", Data->SaLifetime.SoftLifetime);
    HasPre = TRUE;
  }

  if (Data->SaLifetime.HardLifetime != 0) {
    Print (HasPre ? L"," : L"life:[");
    Print (L"%lxS", Data->SaLifetime.HardLifetime);
    HasPre = TRUE;
  }

  if (HasPre) {
    Print (L"] ");
  }

  Print (L"PathMTU:%d\n", (UINTN) Data->PathMTU);

  if (SaId->Proto == EfiIPsecAH) {
    Print (
      L"  Auth:%s/%s\n",
      MapIntegerToString (Data->AlgoInfo.AhAlgoInfo.AuthAlgoId, mMapAuthAlgo),
      Data->AlgoInfo.AhAlgoInfo.AuthKey
      );
  } else {
    AuthAlgoStr = MapIntegerToString (Data->AlgoInfo.EspAlgoInfo.AuthAlgoId, mMapAuthAlgo);
    EncAlgoStr  = MapIntegerToString (Data->AlgoInfo.EspAlgoInfo.EncAlgoId, mMapEncAlgo);

    if (Data->ManualSet) {
      //
      // if the SAD is set manually the key is a Ascii string in most of time.
      // Print the Key in Ascii string format.
      //
      Print (L"  Auth:%s/",AuthAlgoStr);
      DumpAsciiString (
        Data->AlgoInfo.EspAlgoInfo.AuthKey,
        Data->AlgoInfo.EspAlgoInfo.AuthKeyLength
        );
      Print (L"\n  Encrypt:%s/",EncAlgoStr);
      DumpAsciiString (
        Data->AlgoInfo.EspAlgoInfo.EncKey,
        Data->AlgoInfo.EspAlgoInfo.EncKeyLength
        );
    } else {
      //
      // if the SAD is created by IKE, the key is a set of hex value in buffer.
      // Print the Key in Hex format.
      //
      Print (L"  Auth:%s/",AuthAlgoStr);
      DumpBuf ((UINT8 *)(Data->AlgoInfo.EspAlgoInfo.AuthKey), Data->AlgoInfo.EspAlgoInfo.AuthKeyLength);

      Print (L"\n  Encrypt:%s/",EncAlgoStr);
      DumpBuf ((UINT8 *)(Data->AlgoInfo.EspAlgoInfo.EncKey), Data->AlgoInfo.EspAlgoInfo.EncKeyLength);
    }
  }
  Print (L"\n");
  if (Data->SpdSelector != NULL) {
    Print (L"  ");
    DumpSpdSelector (Data->SpdSelector);
    Print (L"\n");
  }

  return EFI_SUCCESS;
}

/**
  Print EFI_IPSEC_PAD_ID and EFI_IPSEC_PAD_DATA content.

  @param[in] PadId         The pointer to the EFI_IPSEC_PAD_ID structure.
  @param[in] Data          The pointer to the EFI_IPSEC_PAD_DATA structure.
  @param[in] EntryIndex    The pointer to the Index in the PAD Database.

  @retval EFI_SUCCESS    Dump PAD information successfully.
**/
EFI_STATUS
DumpPadEntry (
  IN EFI_IPSEC_PAD_ID      *PadId,
  IN EFI_IPSEC_PAD_DATA    *Data,
  IN UINTN                 *EntryIndex
  )
{
  CHAR16    *String1;
  CHAR16    *String2;

  //
  // ADDR:10.23.17.34/15
  // IDEv1 PreSharedSecret IKE-ID
  // password
  //

  Print (L"%d.", (*EntryIndex)++);

  if (PadId->PeerIdValid) {
    Print (L"ID:%s", PadId->Id.PeerId);
  } else {
    Print (L"ADDR:");
    DumpAddressInfo (&PadId->Id.IpAddress);
  }

  Print (L"\n");

  String1 = MapIntegerToString (Data->AuthProtocol, mMapAuthProto);
  String2 = MapIntegerToString (Data->AuthMethod, mMapAuthMethod);
  Print (
    L"  %s %s",
    String1,
    String2
    );

  if (Data->IkeIdFlag) {
    Print (L"IKE-ID");
  }

  Print (L"\n");

  if (Data->AuthData != NULL) {
    DumpAsciiString (Data->AuthData, Data->AuthDataSize);
    Print (L"\n");
  }

  if (Data->RevocationData != NULL) {
    Print (L"  %s\n", Data->RevocationData);
  }

  return EFI_SUCCESS;

}

VISIT_POLICY_ENTRY  mDumpPolicyEntry[] = {
  (VISIT_POLICY_ENTRY) DumpSpdEntry,
  (VISIT_POLICY_ENTRY) DumpSadEntry,
  (VISIT_POLICY_ENTRY) DumpPadEntry
};

/**
  Print all entry information in the database according to datatype.

  @param[in] DataType        The value of EFI_IPSEC_CONFIG_DATA_TYPE.
  @param[in] ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS    Dump all information successfully.
  @retval Others         Some mistaken case.
**/
EFI_STATUS
ListPolicyEntry (
  IN EFI_IPSEC_CONFIG_DATA_TYPE    DataType,
  IN LIST_ENTRY                    *ParamPackage
  )
{
  UINTN  EntryIndex;

  EntryIndex = 0;
  return ForeachPolicyEntry (DataType, mDumpPolicyEntry[DataType], &EntryIndex);
}

