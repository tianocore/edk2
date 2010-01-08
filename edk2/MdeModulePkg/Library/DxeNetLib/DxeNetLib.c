/** @file
  Network library.

Copyright (c) 2005 - 2010, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Protocol/DriverBinding.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/ManagedNetwork.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>

#include <Guid/NicIp4ConfigNvData.h>

#include <Library/NetLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>

#define NIC_ITEM_CONFIG_SIZE   sizeof (NIC_IP4_CONFIG_INFO) + sizeof (EFI_IP4_ROUTE_TABLE) * MAX_IP4_CONFIG_IN_VARIABLE

//
// All the supported IP4 maskes in host byte order.
//
GLOBAL_REMOVE_IF_UNREFERENCED IP4_ADDR  gIp4AllMasks[IP4_MASK_NUM] = {
  0x00000000,
  0x80000000,
  0xC0000000,
  0xE0000000,
  0xF0000000,
  0xF8000000,
  0xFC000000,
  0xFE000000,

  0xFF000000,
  0xFF800000,
  0xFFC00000,
  0xFFE00000,
  0xFFF00000,
  0xFFF80000,
  0xFFFC0000,
  0xFFFE0000,

  0xFFFF0000,
  0xFFFF8000,
  0xFFFFC000,
  0xFFFFE000,
  0xFFFFF000,
  0xFFFFF800,
  0xFFFFFC00,
  0xFFFFFE00,

  0xFFFFFF00,
  0xFFFFFF80,
  0xFFFFFFC0,
  0xFFFFFFE0,
  0xFFFFFFF0,
  0xFFFFFFF8,
  0xFFFFFFFC,
  0xFFFFFFFE,
  0xFFFFFFFF,
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_IPv4_ADDRESS  mZeroIp4Addr = {{0, 0, 0, 0}};

//
// Any error level digitally larger than mNetDebugLevelMax
// will be silently discarded.
//
GLOBAL_REMOVE_IF_UNREFERENCED UINTN  mNetDebugLevelMax = NETDEBUG_LEVEL_ERROR;
GLOBAL_REMOVE_IF_UNREFERENCED UINT32 mSyslogPacketSeq  = 0xDEADBEEF;

//
// You can change mSyslogDstMac mSyslogDstIp and mSyslogSrcIp
// here to direct the syslog packets to the syslog deamon. The
// default is broadcast to both the ethernet and IP.
//
GLOBAL_REMOVE_IF_UNREFERENCED UINT8  mSyslogDstMac[NET_ETHER_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
GLOBAL_REMOVE_IF_UNREFERENCED UINT32 mSyslogDstIp                      = 0xffffffff;
GLOBAL_REMOVE_IF_UNREFERENCED UINT32 mSyslogSrcIp                      = 0;

GLOBAL_REMOVE_IF_UNREFERENCED CHAR8 *mMonthName[] = {
  "Jan",
  "Feb",
  "Mar",
  "Apr",
  "May",
  "Jun",
  "Jul",
  "Aug",
  "Sep",
  "Oct",
  "Nov",
  "Dec"
};

//
// VLAN device path node template
//
GLOBAL_REMOVE_IF_UNREFERENCED VLAN_DEVICE_PATH mNetVlanDevicePathTemplate = {
  {
    MESSAGING_DEVICE_PATH,
    MSG_VLAN_DP,
    {
      (UINT8) (sizeof (VLAN_DEVICE_PATH)),
      (UINT8) ((sizeof (VLAN_DEVICE_PATH)) >> 8)
    }
  },
  0
};

/**
  Locate the handles that support SNP, then open one of them
  to send the syslog packets. The caller isn't required to close
  the SNP after use because the SNP is opened by HandleProtocol.

  @return The point to SNP if one is properly openned. Otherwise NULL

**/
EFI_SIMPLE_NETWORK_PROTOCOL *
SyslogLocateSnp (
  VOID
  )
{
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  EFI_STATUS                  Status;
  EFI_HANDLE                  *Handles;
  UINTN                       HandleCount;
  UINTN                       Index;

  //
  // Locate the handles which has SNP installed.
  //
  Handles = NULL;
  Status  = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiSimpleNetworkProtocolGuid,
                   NULL,
                   &HandleCount,
                   &Handles
                   );

  if (EFI_ERROR (Status) || (HandleCount == 0)) {
    return NULL;
  }

  //
  // Try to open one of the ethernet SNP protocol to send packet
  //
  Snp = NULL;

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiSimpleNetworkProtocolGuid,
                    (VOID **) &Snp
                    );

    if ((Status == EFI_SUCCESS) && (Snp != NULL) &&
        (Snp->Mode->IfType == NET_IFTYPE_ETHERNET) &&
        (Snp->Mode->MaxPacketSize >= NET_SYSLOG_PACKET_LEN)) {

      break;
    }

    Snp = NULL;
  }

  FreePool (Handles);
  return Snp;
}

/**
  Transmit a syslog packet synchronously through SNP. The Packet
  already has the ethernet header prepended. This function should
  fill in the source MAC because it will try to locate a SNP each
  time it is called to avoid the problem if SNP is unloaded.
  This code snip is copied from MNP.

  @param[in] Packet          The Syslog packet
  @param[in] Length          The length of the packet

  @retval EFI_DEVICE_ERROR   Failed to locate a usable SNP protocol
  @retval EFI_TIMEOUT        Timeout happened to send the packet.
  @retval EFI_SUCCESS        Packet is sent.

**/
EFI_STATUS
SyslogSendPacket (
  IN CHAR8                    *Packet,
  IN UINT32                   Length
  )
{
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  ETHER_HEAD                  *Ether;
  EFI_STATUS                  Status;
  EFI_EVENT                   TimeoutEvent;
  UINT8                       *TxBuf;

  Snp = SyslogLocateSnp ();

  if (Snp == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Ether = (ETHER_HEAD *) Packet;
  CopyMem (Ether->SrcMac, Snp->Mode->CurrentAddress.Addr, NET_ETHER_ADDR_LEN);

  //
  // Start the timeout event.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_NOTIFY,
                  NULL,
                  NULL,
                  &TimeoutEvent
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->SetTimer (TimeoutEvent, TimerRelative, NET_SYSLOG_TX_TIMEOUT);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  for (;;) {
    //
    // Transmit the packet through SNP.
    //
    Status = Snp->Transmit (Snp, 0, Length, Packet, NULL, NULL, NULL);

    if ((Status != EFI_SUCCESS) && (Status != EFI_NOT_READY)) {
      Status = EFI_DEVICE_ERROR;
      break;
    }

    //
    // If Status is EFI_SUCCESS, the packet is put in the transmit queue.
    // if Status is EFI_NOT_READY, the transmit engine of the network
    // interface is busy. Both need to sync SNP.
    //
    TxBuf = NULL;

    do {
      //
      // Get the recycled transmit buffer status.
      //
      Snp->GetStatus (Snp, NULL, (VOID **) &TxBuf);

      if (!EFI_ERROR (gBS->CheckEvent (TimeoutEvent))) {
        Status = EFI_TIMEOUT;
        break;
      }

    } while (TxBuf == NULL);

    if ((Status == EFI_SUCCESS) || (Status == EFI_TIMEOUT)) {
      break;
    }

    //
    // Status is EFI_NOT_READY. Restart the timer event and
    // call Snp->Transmit again.
    //
    gBS->SetTimer (TimeoutEvent, TimerRelative, NET_SYSLOG_TX_TIMEOUT);
  }

  gBS->SetTimer (TimeoutEvent, TimerCancel, 0);

ON_EXIT:
  gBS->CloseEvent (TimeoutEvent);
  return Status;
}

/**
  Build a syslog packet, including the Ethernet/Ip/Udp headers
  and user's message.

  @param[in]  Level     Syslog servity level
  @param[in]  Module    The module that generates the log
  @param[in]  File      The file that contains the current log
  @param[in]  Line      The line of code in the File that contains the current log
  @param[in]  Message   The log message
  @param[in]  BufLen    The lenght of the Buf
  @param[out] Buf       The buffer to put the packet data

  @return The length of the syslog packet built.

**/
UINT32
SyslogBuildPacket (
  IN  UINT32                Level,
  IN  UINT8                 *Module,
  IN  UINT8                 *File,
  IN  UINT32                Line,
  IN  UINT8                 *Message,
  IN  UINT32                BufLen,
  OUT CHAR8                 *Buf
  )
{
  ETHER_HEAD                *Ether;
  IP4_HEAD                  *Ip4;
  EFI_UDP_HEADER            *Udp4;
  EFI_TIME                  Time;
  UINT32                    Pri;
  UINT32                    Len;

  //
  // Fill in the Ethernet header. Leave alone the source MAC.
  // SyslogSendPacket will fill in the address for us.
  //
  Ether = (ETHER_HEAD *) Buf;
  CopyMem (Ether->DstMac, mSyslogDstMac, NET_ETHER_ADDR_LEN);
  ZeroMem (Ether->SrcMac, NET_ETHER_ADDR_LEN);

  Ether->EtherType = HTONS (0x0800);    // IPv4 protocol

  Buf             += sizeof (ETHER_HEAD);
  BufLen          -= sizeof (ETHER_HEAD);

  //
  // Fill in the IP header
  //
  Ip4              = (IP4_HEAD *) Buf;
  Ip4->HeadLen     = 5;
  Ip4->Ver         = 4;
  Ip4->Tos         = 0;
  Ip4->TotalLen    = 0;
  Ip4->Id          = (UINT16) mSyslogPacketSeq;
  Ip4->Fragment    = 0;
  Ip4->Ttl         = 16;
  Ip4->Protocol    = 0x11;
  Ip4->Checksum    = 0;
  Ip4->Src         = mSyslogSrcIp;
  Ip4->Dst         = mSyslogDstIp;

  Buf             += sizeof (IP4_HEAD);
  BufLen          -= sizeof (IP4_HEAD);

  //
  // Fill in the UDP header, Udp checksum is optional. Leave it zero.
  //
  Udp4             = (EFI_UDP_HEADER *) Buf;
  Udp4->SrcPort    = HTONS (514);
  Udp4->DstPort    = HTONS (514);
  Udp4->Length     = 0;
  Udp4->Checksum   = 0;

  Buf             += sizeof (EFI_UDP_HEADER);
  BufLen          -= sizeof (EFI_UDP_HEADER);

  //
  // Build the syslog message body with <PRI> Timestamp  machine module Message
  //
  Pri = ((NET_SYSLOG_FACILITY & 31) << 3) | (Level & 7);
  gRT->GetTime (&Time, NULL);

  //
  // Use %a to format the ASCII strings, %s to format UNICODE strings
  //
  Len  = 0;
  Len += (UINT32) AsciiSPrint (
                    Buf,
                    BufLen,
                    "<%d> %a %d %d:%d:%d ",
                    Pri,
                    mMonthName [Time.Month-1],
                    Time.Day,
                    Time.Hour,
                    Time.Minute,
                    Time.Second
                    );
  Len--;

  Len += (UINT32) AsciiSPrint (
                    Buf + Len,
                    BufLen - Len,
                    "Tiano %a: %a (Line: %d File: %a)",
                    Module,
                    Message,
                    Line,
                    File
                    );
  Len--;

  //
  // OK, patch the IP length/checksum and UDP length fields.
  //
  Len           += sizeof (EFI_UDP_HEADER);
  Udp4->Length   = HTONS ((UINT16) Len);

  Len           += sizeof (IP4_HEAD);
  Ip4->TotalLen  = HTONS ((UINT16) Len);
  Ip4->Checksum  = (UINT16) (~NetblockChecksum ((UINT8 *) Ip4, sizeof (IP4_HEAD)));

  return Len + sizeof (ETHER_HEAD);
}

/**
  Allocate a buffer, then format the message to it. This is a
  help function for the NET_DEBUG_XXX macros. The PrintArg of
  these macros treats the variable length print parameters as a
  single parameter, and pass it to the NetDebugASPrint. For
  example, NET_DEBUG_TRACE ("Tcp", ("State transit to %a\n", Name))
  if extracted to:

         NetDebugOutput (
           NETDEBUG_LEVEL_TRACE,
           "Tcp",
           __FILE__,
           __LINE__,
           NetDebugASPrint ("State transit to %a\n", Name)
         )

  @param Format  The ASCII format string.
  @param ...     The variable length parameter whose format is determined
                 by the Format string.

  @return        The buffer containing the formatted message,
                 or NULL if failed to allocate memory.

**/
CHAR8 *
NetDebugASPrint (
  IN CHAR8                  *Format,
  ...
  )
{
  VA_LIST                   Marker;
  CHAR8                     *Buf;

  Buf = (CHAR8 *) AllocatePool (NET_DEBUG_MSG_LEN);

  if (Buf == NULL) {
    return NULL;
  }

  VA_START (Marker, Format);
  AsciiVSPrint (Buf, NET_DEBUG_MSG_LEN, Format, Marker);
  VA_END (Marker);

  return Buf;
}

/**
  Builds an UDP4 syslog packet and send it using SNP.

  This function will locate a instance of SNP then send the message through it.
  Because it isn't open the SNP BY_DRIVER, apply caution when using it.

  @param Level    The servity level of the message.
  @param Module   The Moudle that generates the log.
  @param File     The file that contains the log.
  @param Line     The exact line that contains the log.
  @param Message  The user message to log.

  @retval EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the packet
  @retval EFI_SUCCESS           The log is discard because that it is more verbose
                                than the mNetDebugLevelMax. Or, it has been sent out.
**/
EFI_STATUS
NetDebugOutput (
  IN UINT32                    Level,
  IN UINT8                     *Module,
  IN UINT8                     *File,
  IN UINT32                    Line,
  IN UINT8                     *Message
  )
{
  CHAR8                        *Packet;
  UINT32                       Len;
  EFI_STATUS                   Status;

  //
  // Check whether the message should be sent out
  //
  if (Message == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Level > mNetDebugLevelMax) {
    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  //
  // Allocate a maxium of 1024 bytes, the caller should ensure
  // that the message plus the ethernet/ip/udp header is shorter
  // than this
  //
  Packet = (CHAR8 *) AllocatePool (NET_SYSLOG_PACKET_LEN);

  if (Packet == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Build the message: Ethernet header + IP header + Udp Header + user data
  //
  Len = SyslogBuildPacket (
          Level,
          Module,
          File,
          Line,
          Message,
          NET_SYSLOG_PACKET_LEN,
          Packet
          );

  mSyslogPacketSeq++;
  Status = SyslogSendPacket (Packet, Len);
  FreePool (Packet);

ON_EXIT:
  FreePool (Message);
  return Status;
}
/**
  Return the length of the mask.

  Return the length of the mask, the correct value is from 0 to 32.
  If the mask is invalid, return the invalid length 33, which is IP4_MASK_NUM.
  NetMask is in the host byte order.

  @param[in]  NetMask              The netmask to get the length from.

  @return The length of the netmask, IP4_MASK_NUM if the mask is invalid.

**/
INTN
EFIAPI
NetGetMaskLength (
  IN IP4_ADDR               NetMask
  )
{
  INTN                      Index;

  for (Index = 0; Index < IP4_MASK_NUM; Index++) {
    if (NetMask == gIp4AllMasks[Index]) {
      break;
    }
  }

  return Index;
}



/**
  Return the class of the IP address, such as class A, B, C.
  Addr is in host byte order.

  The address of class A  starts with 0.
  If the address belong to class A, return IP4_ADDR_CLASSA.
  The address of class B  starts with 10.
  If the address belong to class B, return IP4_ADDR_CLASSB.
  The address of class C  starts with 110.
  If the address belong to class C, return IP4_ADDR_CLASSC.
  The address of class D  starts with 1110.
  If the address belong to class D, return IP4_ADDR_CLASSD.
  The address of class E  starts with 1111.
  If the address belong to class E, return IP4_ADDR_CLASSE.


  @param[in]   Addr                  The address to get the class from.

  @return IP address class, such as IP4_ADDR_CLASSA.

**/
INTN
EFIAPI
NetGetIpClass (
  IN IP4_ADDR               Addr
  )
{
  UINT8                     ByteOne;

  ByteOne = (UINT8) (Addr >> 24);

  if ((ByteOne & 0x80) == 0) {
    return IP4_ADDR_CLASSA;

  } else if ((ByteOne & 0xC0) == 0x80) {
    return IP4_ADDR_CLASSB;

  } else if ((ByteOne & 0xE0) == 0xC0) {
    return IP4_ADDR_CLASSC;

  } else if ((ByteOne & 0xF0) == 0xE0) {
    return IP4_ADDR_CLASSD;

  } else {
    return IP4_ADDR_CLASSE;

  }
}


/**
  Check whether the IP is a valid unicast address according to
  the netmask. If NetMask is zero, use the IP address's class to get the default mask.

  If Ip is 0, IP is not a valid unicast address.
  Class D address is used for multicasting and class E address is reserved for future. If Ip
  belongs to class D or class E, IP is not a valid unicast address.
  If all bits of the host address of IP are 0 or 1, IP is also not a valid unicast address.

  @param[in]  Ip                    The IP to check against.
  @param[in]  NetMask               The mask of the IP.

  @return TRUE if IP is a valid unicast address on the network, otherwise FALSE.

**/
BOOLEAN
EFIAPI
NetIp4IsUnicast (
  IN IP4_ADDR               Ip,
  IN IP4_ADDR               NetMask
  )
{
  INTN                      Class;

  Class = NetGetIpClass (Ip);

  if ((Ip == 0) || (Class >= IP4_ADDR_CLASSD)) {
    return FALSE;
  }

  if (NetMask == 0) {
    NetMask = gIp4AllMasks[Class << 3];
  }

  if (((Ip &~NetMask) == ~NetMask) || ((Ip &~NetMask) == 0)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Check whether the incoming IPv6 address is a valid unicast address.

  If the address is a multicast address has binary 0xFF at the start, it is not
  a valid unicast address. If the address is unspecified ::, it is not a valid
  unicast address to be assigned to any node. If the address is loopback address
  ::1, it is also not a valid unicast address to be assigned to any physical
  interface.

  @param[in]  Ip6                   The IPv6 address to check against.

  @return TRUE if Ip6 is a valid unicast address on the network, otherwise FALSE.

**/
BOOLEAN
NetIp6IsValidUnicast (
  IN EFI_IPv6_ADDRESS       *Ip6
  )
{
  UINT8 Byte;
  UINT8 Index;

  if (Ip6->Addr[0] == 0xFF) {
    return FALSE;
  }

  for (Index = 0; Index < 15; Index++) {
    if (Ip6->Addr[Index] != 0) {
      return TRUE;
    }
  }

  Byte = Ip6->Addr[Index];

  if (Byte == 0x0 || Byte == 0x1) {
    return FALSE;
  }

  return TRUE;
}

/**
  Check whether the incoming Ipv6 address is the unspecified address or not.

  @param[in] Ip6   - Ip6 address, in network order.

  @retval TRUE     - Yes, unspecified
  @retval FALSE    - No

**/
BOOLEAN
NetIp6IsUnspecifiedAddr (
  IN EFI_IPv6_ADDRESS       *Ip6
  )
{
  UINT8 Index;

  for (Index = 0; Index < 16; Index++) {
    if (Ip6->Addr[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Check whether the incoming Ipv6 address is a link-local address.

  @param[in] Ip6   - Ip6 address, in network order.

  @retval TRUE  - Yes, link-local address
  @retval FALSE - No

**/
BOOLEAN
NetIp6IsLinkLocalAddr (
  IN EFI_IPv6_ADDRESS *Ip6
  )
{
  UINT8 Index;

  ASSERT (Ip6 != NULL);

  if (Ip6->Addr[0] != 0xFE) {
    return FALSE;
  }

  if (Ip6->Addr[1] != 0x80) {
    return FALSE;
  }

  for (Index = 2; Index < 8; Index++) {
    if (Ip6->Addr[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Check whether the Ipv6 address1 and address2 are on the connected network.

  @param[in] Ip1          - Ip6 address1, in network order.
  @param[in] Ip2          - Ip6 address2, in network order.
  @param[in] PrefixLength - The prefix length of the checking net.

  @retval TRUE            - Yes, connected.
  @retval FALSE           - No.

**/
BOOLEAN
NetIp6IsNetEqual (
  EFI_IPv6_ADDRESS *Ip1,
  EFI_IPv6_ADDRESS *Ip2,
  UINT8            PrefixLength
  )
{
  UINT8 Byte;
  UINT8 Bit;
  UINT8 Mask;

  ASSERT (Ip1 != NULL && Ip2 != NULL);

  if (PrefixLength == 0) {
    return TRUE;
  }

  Byte = (UINT8) (PrefixLength / 8);
  Bit  = (UINT8) (PrefixLength % 8);

  if (CompareMem (Ip1, Ip2, Byte) != 0) {
    return FALSE;
  }

  if (Bit > 0) {
    Mask = (UINT8) (0xFF << (8 - Bit));

    if ((Ip1->Addr[Byte] & Mask) != (Ip2->Addr[Byte] & Mask)) {
      return FALSE;
    }
  }

  return TRUE;
}


/**
  Switches the endianess of an IPv6 address

  This function swaps the bytes in a 128-bit IPv6 address to switch the value
  from little endian to big endian or vice versa. The byte swapped value is
  returned.

  @param  Ip6 Points to an IPv6 address

  @return The byte swapped IPv6 address.

**/
EFI_IPv6_ADDRESS *
Ip6Swap128 (
  EFI_IPv6_ADDRESS *Ip6
  )
{
  UINT64 High;
  UINT64 Low;

  CopyMem (&High, Ip6, sizeof (UINT64));
  CopyMem (&Low, &Ip6->Addr[8], sizeof (UINT64));

  High = SwapBytes64 (High);
  Low  = SwapBytes64 (Low);

  CopyMem (Ip6, &Low, sizeof (UINT64));
  CopyMem (&Ip6->Addr[8], &High, sizeof (UINT64));

  return Ip6;
}

/**
  Initialize a random seed using current time.

  Get current time first. Then initialize a random seed based on some basic
  mathematics operation on the hour, day, minute, second, nanosecond and year
  of the current time.

  @return The random seed initialized with current time.

**/
UINT32
EFIAPI
NetRandomInitSeed (
  VOID
  )
{
  EFI_TIME                  Time;
  UINT32                    Seed;

  gRT->GetTime (&Time, NULL);
  Seed = (~Time.Hour << 24 | Time.Day << 16 | Time.Minute << 8 | Time.Second);
  Seed ^= Time.Nanosecond;
  Seed ^= Time.Year << 7;

  return Seed;
}


/**
  Extract a UINT32 from a byte stream.

  Copy a UINT32 from a byte stream, then converts it from Network
  byte order to host byte order. Use this function to avoid alignment error.

  @param[in]  Buf                 The buffer to extract the UINT32.

  @return The UINT32 extracted.

**/
UINT32
EFIAPI
NetGetUint32 (
  IN UINT8                  *Buf
  )
{
  UINT32                    Value;

  CopyMem (&Value, Buf, sizeof (UINT32));
  return NTOHL (Value);
}


/**
  Put a UINT32 to the byte stream in network byte order.

  Converts a UINT32 from host byte order to network byte order. Then copy it to the
  byte stream.

  @param[in, out]  Buf          The buffer to put the UINT32.
  @param[in]      Data          The data to put.

**/
VOID
EFIAPI
NetPutUint32 (
  IN OUT UINT8                 *Buf,
  IN     UINT32                Data
  )
{
  Data = HTONL (Data);
  CopyMem (Buf, &Data, sizeof (UINT32));
}


/**
  Remove the first node entry on the list, and return the removed node entry.

  Removes the first node Entry from a doubly linked list. It is up to the caller of
  this function to release the memory used by the first node if that is required. On
  exit, the removed node is returned.

  If Head is NULL, then ASSERT().
  If Head was not initialized, then ASSERT().
  If PcdMaximumLinkedListLength is not zero, and the number of nodes in the
  linked list including the head node is greater than or equal to PcdMaximumLinkedListLength,
  then ASSERT().

  @param[in, out]  Head                  The list header.

  @return The first node entry that is removed from the list, NULL if the list is empty.

**/
LIST_ENTRY *
EFIAPI
NetListRemoveHead (
  IN OUT LIST_ENTRY            *Head
  )
{
  LIST_ENTRY            *First;

  ASSERT (Head != NULL);

  if (IsListEmpty (Head)) {
    return NULL;
  }

  First                         = Head->ForwardLink;
  Head->ForwardLink             = First->ForwardLink;
  First->ForwardLink->BackLink  = Head;

  DEBUG_CODE (
    First->ForwardLink  = (LIST_ENTRY *) NULL;
    First->BackLink     = (LIST_ENTRY *) NULL;
  );

  return First;
}


/**
  Remove the last node entry on the list and and return the removed node entry.

  Removes the last node entry from a doubly linked list. It is up to the caller of
  this function to release the memory used by the first node if that is required. On
  exit, the removed node is returned.

  If Head is NULL, then ASSERT().
  If Head was not initialized, then ASSERT().
  If PcdMaximumLinkedListLength is not zero, and the number of nodes in the
  linked list including the head node is greater than or equal to PcdMaximumLinkedListLength,
  then ASSERT().

  @param[in, out]  Head                  The list head.

  @return The last node entry that is removed from the list, NULL if the list is empty.

**/
LIST_ENTRY *
EFIAPI
NetListRemoveTail (
  IN OUT LIST_ENTRY            *Head
  )
{
  LIST_ENTRY            *Last;

  ASSERT (Head != NULL);

  if (IsListEmpty (Head)) {
    return NULL;
  }

  Last                        = Head->BackLink;
  Head->BackLink              = Last->BackLink;
  Last->BackLink->ForwardLink = Head;

  DEBUG_CODE (
    Last->ForwardLink = (LIST_ENTRY *) NULL;
    Last->BackLink    = (LIST_ENTRY *) NULL;
  );

  return Last;
}


/**
  Insert a new node entry after a designated node entry of a doubly linked list.

  Inserts a new node entry donated by NewEntry after the node entry donated by PrevEntry
  of the doubly linked list.

  @param[in, out]  PrevEntry             The previous entry to insert after.
  @param[in, out]  NewEntry              The new entry to insert.

**/
VOID
EFIAPI
NetListInsertAfter (
  IN OUT LIST_ENTRY         *PrevEntry,
  IN OUT LIST_ENTRY         *NewEntry
  )
{
  NewEntry->BackLink                = PrevEntry;
  NewEntry->ForwardLink             = PrevEntry->ForwardLink;
  PrevEntry->ForwardLink->BackLink  = NewEntry;
  PrevEntry->ForwardLink            = NewEntry;
}


/**
  Insert a new node entry before a designated node entry of a doubly linked list.

  Inserts a new node entry donated by NewEntry after the node entry donated by PostEntry
  of the doubly linked list.

  @param[in, out]  PostEntry             The entry to insert before.
  @param[in, out]  NewEntry              The new entry to insert.

**/
VOID
EFIAPI
NetListInsertBefore (
  IN OUT LIST_ENTRY     *PostEntry,
  IN OUT LIST_ENTRY     *NewEntry
  )
{
  NewEntry->ForwardLink             = PostEntry;
  NewEntry->BackLink                = PostEntry->BackLink;
  PostEntry->BackLink->ForwardLink  = NewEntry;
  PostEntry->BackLink               = NewEntry;
}


/**
  Initialize the netmap. Netmap is a reposity to keep the <Key, Value> pairs.

  Initialize the forward and backward links of two head nodes donated by Map->Used
  and Map->Recycled of two doubly linked lists.
  Initializes the count of the <Key, Value> pairs in the netmap to zero.

  If Map is NULL, then ASSERT().
  If the address of Map->Used is NULL, then ASSERT().
  If the address of Map->Recycled is NULl, then ASSERT().

  @param[in, out]  Map                   The netmap to initialize.

**/
VOID
EFIAPI
NetMapInit (
  IN OUT NET_MAP                *Map
  )
{
  ASSERT (Map != NULL);

  InitializeListHead (&Map->Used);
  InitializeListHead (&Map->Recycled);
  Map->Count = 0;
}


/**
  To clean up the netmap, that is, release allocated memories.

  Removes all nodes of the Used doubly linked list and free memory of all related netmap items.
  Removes all nodes of the Recycled doubly linked list and free memory of all related netmap items.
  The number of the <Key, Value> pairs in the netmap is set to be zero.

  If Map is NULL, then ASSERT().

  @param[in, out]  Map                   The netmap to clean up.

**/
VOID
EFIAPI
NetMapClean (
  IN OUT NET_MAP            *Map
  )
{
  NET_MAP_ITEM              *Item;
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;

  ASSERT (Map != NULL);

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Map->Used) {
    Item = NET_LIST_USER_STRUCT (Entry, NET_MAP_ITEM, Link);

    RemoveEntryList (&Item->Link);
    Map->Count--;

    gBS->FreePool (Item);
  }

  ASSERT ((Map->Count == 0) && IsListEmpty (&Map->Used));

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Map->Recycled) {
    Item = NET_LIST_USER_STRUCT (Entry, NET_MAP_ITEM, Link);

    RemoveEntryList (&Item->Link);
    gBS->FreePool (Item);
  }

  ASSERT (IsListEmpty (&Map->Recycled));
}


/**
  Test whether the netmap is empty and return true if it is.

  If the number of the <Key, Value> pairs in the netmap is zero, return TRUE.

  If Map is NULL, then ASSERT().


  @param[in]  Map                   The net map to test.

  @return TRUE if the netmap is empty, otherwise FALSE.

**/
BOOLEAN
EFIAPI
NetMapIsEmpty (
  IN NET_MAP                *Map
  )
{
  ASSERT (Map != NULL);
  return (BOOLEAN) (Map->Count == 0);
}


/**
  Return the number of the <Key, Value> pairs in the netmap.

  @param[in]  Map                   The netmap to get the entry number.

  @return The entry number in the netmap.

**/
UINTN
EFIAPI
NetMapGetCount (
  IN NET_MAP                *Map
  )
{
  return Map->Count;
}


/**
  Return one allocated item.

  If the Recycled doubly linked list of the netmap is empty, it will try to allocate
  a batch of items if there are enough resources and add corresponding nodes to the begining
  of the Recycled doubly linked list of the netmap. Otherwise, it will directly remove
  the fist node entry of the Recycled doubly linked list and return the corresponding item.

  If Map is NULL, then ASSERT().

  @param[in, out]  Map          The netmap to allocate item for.

  @return                       The allocated item. If NULL, the
                                allocation failed due to resource limit.

**/
NET_MAP_ITEM *
NetMapAllocItem (
  IN OUT NET_MAP            *Map
  )
{
  NET_MAP_ITEM              *Item;
  LIST_ENTRY                *Head;
  UINTN                     Index;

  ASSERT (Map != NULL);

  Head = &Map->Recycled;

  if (IsListEmpty (Head)) {
    for (Index = 0; Index < NET_MAP_INCREAMENT; Index++) {
      Item = AllocatePool (sizeof (NET_MAP_ITEM));

      if (Item == NULL) {
        if (Index == 0) {
          return NULL;
        }

        break;
      }

      InsertHeadList (Head, &Item->Link);
    }
  }

  Item = NET_LIST_HEAD (Head, NET_MAP_ITEM, Link);
  NetListRemoveHead (Head);

  return Item;
}


/**
  Allocate an item to save the <Key, Value> pair to the head of the netmap.

  Allocate an item to save the <Key, Value> pair and add corresponding node entry
  to the beginning of the Used doubly linked list. The number of the <Key, Value>
  pairs in the netmap increase by 1.

  If Map is NULL, then ASSERT().

  @param[in, out]  Map                   The netmap to insert into.
  @param[in]       Key                   The user's key.
  @param[in]       Value                 The user's value for the key.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the item.
  @retval EFI_SUCCESS           The item is inserted to the head.

**/
EFI_STATUS
EFIAPI
NetMapInsertHead (
  IN OUT NET_MAP            *Map,
  IN VOID                   *Key,
  IN VOID                   *Value    OPTIONAL
  )
{
  NET_MAP_ITEM              *Item;

  ASSERT (Map != NULL);

  Item = NetMapAllocItem (Map);

  if (Item == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Item->Key   = Key;
  Item->Value = Value;
  InsertHeadList (&Map->Used, &Item->Link);

  Map->Count++;
  return EFI_SUCCESS;
}


/**
  Allocate an item to save the <Key, Value> pair to the tail of the netmap.

  Allocate an item to save the <Key, Value> pair and add corresponding node entry
  to the tail of the Used doubly linked list. The number of the <Key, Value>
  pairs in the netmap increase by 1.

  If Map is NULL, then ASSERT().

  @param[in, out]  Map                   The netmap to insert into.
  @param[in]       Key                   The user's key.
  @param[in]       Value                 The user's value for the key.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the item.
  @retval EFI_SUCCESS           The item is inserted to the tail.

**/
EFI_STATUS
EFIAPI
NetMapInsertTail (
  IN OUT NET_MAP            *Map,
  IN VOID                   *Key,
  IN VOID                   *Value    OPTIONAL
  )
{
  NET_MAP_ITEM              *Item;

  ASSERT (Map != NULL);

  Item = NetMapAllocItem (Map);

  if (Item == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Item->Key   = Key;
  Item->Value = Value;
  InsertTailList (&Map->Used, &Item->Link);

  Map->Count++;

  return EFI_SUCCESS;
}


/**
  Check whether the item is in the Map and return TRUE if it is.

  @param[in]  Map                   The netmap to search within.
  @param[in]  Item                  The item to search.

  @return TRUE if the item is in the netmap, otherwise FALSE.

**/
BOOLEAN
NetItemInMap (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item
  )
{
  LIST_ENTRY            *ListEntry;

  NET_LIST_FOR_EACH (ListEntry, &Map->Used) {
    if (ListEntry == &Item->Link) {
      return TRUE;
    }
  }

  return FALSE;
}


/**
  Find the key in the netmap and returns the point to the item contains the Key.

  Iterate the Used doubly linked list of the netmap to get every item. Compare the key of every
  item with the key to search. It returns the point to the item contains the Key if found.

  If Map is NULL, then ASSERT().

  @param[in]  Map                   The netmap to search within.
  @param[in]  Key                   The key to search.

  @return The point to the item contains the Key, or NULL if Key isn't in the map.

**/
NET_MAP_ITEM *
EFIAPI
NetMapFindKey (
  IN  NET_MAP               *Map,
  IN  VOID                  *Key
  )
{
  LIST_ENTRY              *Entry;
  NET_MAP_ITEM            *Item;

  ASSERT (Map != NULL);

  NET_LIST_FOR_EACH (Entry, &Map->Used) {
    Item = NET_LIST_USER_STRUCT (Entry, NET_MAP_ITEM, Link);

    if (Item->Key == Key) {
      return Item;
    }
  }

  return NULL;
}


/**
  Remove the node entry of the item from the netmap and return the key of the removed item.

  Remove the node entry of the item from the Used doubly linked list of the netmap.
  The number of the <Key, Value> pairs in the netmap decrease by 1. Then add the node
  entry of the item to the Recycled doubly linked list of the netmap. If Value is not NULL,
  Value will point to the value of the item. It returns the key of the removed item.

  If Map is NULL, then ASSERT().
  If Item is NULL, then ASSERT().
  if item in not in the netmap, then ASSERT().

  @param[in, out]  Map                   The netmap to remove the item from.
  @param[in, out]  Item                  The item to remove.
  @param[out]      Value                 The variable to receive the value if not NULL.

  @return                                The key of the removed item.

**/
VOID *
EFIAPI
NetMapRemoveItem (
  IN  OUT NET_MAP             *Map,
  IN  OUT NET_MAP_ITEM        *Item,
  OUT VOID                    **Value           OPTIONAL
  )
{
  ASSERT ((Map != NULL) && (Item != NULL));
  ASSERT (NetItemInMap (Map, Item));

  RemoveEntryList (&Item->Link);
  Map->Count--;
  InsertHeadList (&Map->Recycled, &Item->Link);

  if (Value != NULL) {
    *Value = Item->Value;
  }

  return Item->Key;
}


/**
  Remove the first node entry on the netmap and return the key of the removed item.

  Remove the first node entry from the Used doubly linked list of the netmap.
  The number of the <Key, Value> pairs in the netmap decrease by 1. Then add the node
  entry to the Recycled doubly linked list of the netmap. If parameter Value is not NULL,
  parameter Value will point to the value of the item. It returns the key of the removed item.

  If Map is NULL, then ASSERT().
  If the Used doubly linked list is empty, then ASSERT().

  @param[in, out]  Map                   The netmap to remove the head from.
  @param[out]      Value                 The variable to receive the value if not NULL.

  @return                                The key of the item removed.

**/
VOID *
EFIAPI
NetMapRemoveHead (
  IN OUT NET_MAP            *Map,
  OUT VOID                  **Value         OPTIONAL
  )
{
  NET_MAP_ITEM  *Item;

  //
  // Often, it indicates a programming error to remove
  // the first entry in an empty list
  //
  ASSERT (Map && !IsListEmpty (&Map->Used));

  Item = NET_LIST_HEAD (&Map->Used, NET_MAP_ITEM, Link);
  RemoveEntryList (&Item->Link);
  Map->Count--;
  InsertHeadList (&Map->Recycled, &Item->Link);

  if (Value != NULL) {
    *Value = Item->Value;
  }

  return Item->Key;
}


/**
  Remove the last node entry on the netmap and return the key of the removed item.

  Remove the last node entry from the Used doubly linked list of the netmap.
  The number of the <Key, Value> pairs in the netmap decrease by 1. Then add the node
  entry to the Recycled doubly linked list of the netmap. If parameter Value is not NULL,
  parameter Value will point to the value of the item. It returns the key of the removed item.

  If Map is NULL, then ASSERT().
  If the Used doubly linked list is empty, then ASSERT().

  @param[in, out]  Map                   The netmap to remove the tail from.
  @param[out]      Value                 The variable to receive the value if not NULL.

  @return                                The key of the item removed.

**/
VOID *
EFIAPI
NetMapRemoveTail (
  IN OUT NET_MAP            *Map,
  OUT VOID                  **Value       OPTIONAL
  )
{
  NET_MAP_ITEM              *Item;

  //
  // Often, it indicates a programming error to remove
  // the last entry in an empty list
  //
  ASSERT (Map && !IsListEmpty (&Map->Used));

  Item = NET_LIST_TAIL (&Map->Used, NET_MAP_ITEM, Link);
  RemoveEntryList (&Item->Link);
  Map->Count--;
  InsertHeadList (&Map->Recycled, &Item->Link);

  if (Value != NULL) {
    *Value = Item->Value;
  }

  return Item->Key;
}


/**
  Iterate through the netmap and call CallBack for each item.

  It will contiue the traverse if CallBack returns EFI_SUCCESS, otherwise, break
  from the loop. It returns the CallBack's last return value. This function is
  delete safe for the current item.

  If Map is NULL, then ASSERT().
  If CallBack is NULL, then ASSERT().

  @param[in]  Map                   The Map to iterate through.
  @param[in]  CallBack              The callback function to call for each item.
  @param[in]  Arg                   The opaque parameter to the callback.

  @retval EFI_SUCCESS            There is no item in the netmap or CallBack for each item
                                 return EFI_SUCCESS.
  @retval Others                 It returns the CallBack's last return value.

**/
EFI_STATUS
EFIAPI
NetMapIterate (
  IN NET_MAP                *Map,
  IN NET_MAP_CALLBACK       CallBack,
  IN VOID                   *Arg      OPTIONAL
  )
{

  LIST_ENTRY            *Entry;
  LIST_ENTRY            *Next;
  LIST_ENTRY            *Head;
  NET_MAP_ITEM          *Item;
  EFI_STATUS            Result;

  ASSERT ((Map != NULL) && (CallBack != NULL));

  Head = &Map->Used;

  if (IsListEmpty (Head)) {
    return EFI_SUCCESS;
  }

  NET_LIST_FOR_EACH_SAFE (Entry, Next, Head) {
    Item   = NET_LIST_USER_STRUCT (Entry, NET_MAP_ITEM, Link);
    Result = CallBack (Map, Item, Arg);

    if (EFI_ERROR (Result)) {
      return Result;
    }
  }

  return EFI_SUCCESS;
}


/**
  This is the default unload handle for all the network drivers.

  Disconnect the driver specified by ImageHandle from all the devices in the handle database.
  Uninstall all the protocols installed in the driver entry point.

  @param[in]  ImageHandle       The drivers' driver image.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
NetLibDefaultUnload (
  IN EFI_HANDLE             ImageHandle
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        *DeviceHandleBuffer;
  UINTN                             DeviceHandleCount;
  UINTN                             Index;
  EFI_DRIVER_BINDING_PROTOCOL       *DriverBinding;
  EFI_COMPONENT_NAME_PROTOCOL       *ComponentName;
  EFI_COMPONENT_NAME2_PROTOCOL      *ComponentName2;

  //
  // Get the list of all the handles in the handle database.
  // If there is an error getting the list, then the unload
  // operation fails.
  //
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &DeviceHandleCount,
                  &DeviceHandleBuffer
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Disconnect the driver specified by ImageHandle from all
  // the devices in the handle database.
  //
  for (Index = 0; Index < DeviceHandleCount; Index++) {
    Status = gBS->DisconnectController (
                    DeviceHandleBuffer[Index],
                    ImageHandle,
                    NULL
                    );
  }

  //
  // Uninstall all the protocols installed in the driver entry point
  //
  for (Index = 0; Index < DeviceHandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    DeviceHandleBuffer[Index],
                    &gEfiDriverBindingProtocolGuid,
                    (VOID **) &DriverBinding
                    );

    if (EFI_ERROR (Status)) {
      continue;
    }

    if (DriverBinding->ImageHandle != ImageHandle) {
      continue;
    }

    gBS->UninstallProtocolInterface (
          ImageHandle,
          &gEfiDriverBindingProtocolGuid,
          DriverBinding
          );
    Status = gBS->HandleProtocol (
                    DeviceHandleBuffer[Index],
                    &gEfiComponentNameProtocolGuid,
                    (VOID **) &ComponentName
                    );
    if (!EFI_ERROR (Status)) {
      gBS->UninstallProtocolInterface (
             ImageHandle,
             &gEfiComponentNameProtocolGuid,
             ComponentName
             );
    }

    Status = gBS->HandleProtocol (
                    DeviceHandleBuffer[Index],
                    &gEfiComponentName2ProtocolGuid,
                    (VOID **) &ComponentName2
                    );
    if (!EFI_ERROR (Status)) {
      gBS->UninstallProtocolInterface (
             ImageHandle,
             &gEfiComponentName2ProtocolGuid,
             ComponentName2
             );
    }
  }

  //
  // Free the buffer containing the list of handles from the handle database
  //
  if (DeviceHandleBuffer != NULL) {
    gBS->FreePool (DeviceHandleBuffer);
  }

  return EFI_SUCCESS;
}



/**
  Create a child of the service that is identified by ServiceBindingGuid.

  Get the ServiceBinding Protocol first, then use it to create a child.

  If ServiceBindingGuid is NULL, then ASSERT().
  If ChildHandle is NULL, then ASSERT().

  @param[in]       Controller            The controller which has the service installed.
  @param[in]       Image                 The image handle used to open service.
  @param[in]       ServiceBindingGuid    The service's Guid.
  @param[in, out]  ChildHandle           The handle to receive the create child.

  @retval EFI_SUCCESS           The child is successfully created.
  @retval Others                Failed to create the child.

**/
EFI_STATUS
EFIAPI
NetLibCreateServiceChild (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            Image,
  IN  EFI_GUID              *ServiceBindingGuid,
  IN  OUT EFI_HANDLE        *ChildHandle
  )
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL  *Service;


  ASSERT ((ServiceBindingGuid != NULL) && (ChildHandle != NULL));

  //
  // Get the ServiceBinding Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  ServiceBindingGuid,
                  (VOID **) &Service,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create a child
  //
  Status = Service->CreateChild (Service, ChildHandle);
  return Status;
}


/**
  Destory a child of the service that is identified by ServiceBindingGuid.

  Get the ServiceBinding Protocol first, then use it to destroy a child.

  If ServiceBindingGuid is NULL, then ASSERT().

  @param[in]   Controller            The controller which has the service installed.
  @param[in]   Image                 The image handle used to open service.
  @param[in]   ServiceBindingGuid    The service's Guid.
  @param[in]   ChildHandle           The child to destory.

  @retval EFI_SUCCESS           The child is successfully destoried.
  @retval Others                Failed to destory the child.

**/
EFI_STATUS
EFIAPI
NetLibDestroyServiceChild (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            Image,
  IN  EFI_GUID              *ServiceBindingGuid,
  IN  EFI_HANDLE            ChildHandle
  )
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL  *Service;

  ASSERT (ServiceBindingGuid != NULL);

  //
  // Get the ServiceBinding Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  ServiceBindingGuid,
                  (VOID **) &Service,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // destory the child
  //
  Status = Service->DestroyChild (Service, ChildHandle);
  return Status;
}

/**
  Get handle with Simple Network Protocol installed on it.

  There should be MNP Service Binding Protocol installed on the input ServiceHandle.
  If Simple Network Protocol is already installed on the ServiceHandle, the
  ServiceHandle will be returned. If SNP is not installed on the ServiceHandle,
  try to find its parent handle with SNP installed.

  @param[in]   ServiceHandle    The handle where network service binding protocols are
                                installed on.
  @param[out]  Snp              The pointer to store the address of the SNP instance.
                                This is an optional parameter that may be NULL.

  @return The SNP handle, or NULL if not found.

**/
EFI_HANDLE
EFIAPI
NetLibGetSnpHandle (
  IN   EFI_HANDLE                  ServiceHandle,
  OUT  EFI_SIMPLE_NETWORK_PROTOCOL **Snp  OPTIONAL
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_NETWORK_PROTOCOL  *SnpInstance;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;
  EFI_HANDLE                   SnpHandle;

  //
  // Try to open SNP from ServiceHandle
  //
  SnpInstance = NULL;
  Status = gBS->HandleProtocol (ServiceHandle, &gEfiSimpleNetworkProtocolGuid, (VOID **) &SnpInstance);
  if (!EFI_ERROR (Status)) {
    if (Snp != NULL) {
      *Snp = SnpInstance;
    }
    return ServiceHandle;
  }

  //
  // Failed to open SNP, try to get SNP handle by LocateDevicePath()
  //
  DevicePath = DevicePathFromHandle (ServiceHandle);
  if (DevicePath == NULL) {
    return NULL;
  }

  SnpHandle = NULL;
  Status = gBS->LocateDevicePath (&gEfiSimpleNetworkProtocolGuid, &DevicePath, &SnpHandle);
  if (EFI_ERROR (Status)) {
    //
    // Failed to find SNP handle
    //
    return NULL;
  }

  Status = gBS->HandleProtocol (SnpHandle, &gEfiSimpleNetworkProtocolGuid, (VOID **) &SnpInstance);
  if (!EFI_ERROR (Status)) {
    if (Snp != NULL) {
      *Snp = SnpInstance;
    }
    return SnpHandle;
  }

  return NULL;
}

/**
  Retrieve VLAN ID of a VLAN device handle.

  Search VLAN device path node in Device Path of specified ServiceHandle and
  return its VLAN ID. If no VLAN device path node found, then this ServiceHandle
  is not a VLAN device handle, and 0 will be returned.

  @param[in]   ServiceHandle    The handle where network service binding protocols are
                                installed on.

  @return VLAN ID of the device handle, or 0 if not a VLAN device.

**/
UINT16
EFIAPI
NetLibGetVlanId (
  IN EFI_HANDLE             ServiceHandle
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Node;

  DevicePath = DevicePathFromHandle (ServiceHandle);
  if (DevicePath == NULL) {
    return 0;
  }

  Node = DevicePath;
  while (!IsDevicePathEnd (Node)) {
    if (Node->Type == MESSAGING_DEVICE_PATH && Node->SubType == MSG_VLAN_DP) {
      return ((VLAN_DEVICE_PATH *) Node)->VlanId;
    }
    Node = NextDevicePathNode (Node);
  }

  return 0;
}

/**
  Find VLAN device handle with specified VLAN ID.

  The VLAN child device handle is created by VLAN Config Protocol on ControllerHandle.
  This function will append VLAN device path node to the parent device path,
  and then use LocateDevicePath() to find the correct VLAN device handle.

  @param[in]   ControllerHandle The handle where network service binding protocols are
                                installed on.
  @param[in]   VlanId           The configured VLAN ID for the VLAN device.

  @return The VLAN device handle, or NULL if not found.

**/
EFI_HANDLE
EFIAPI
NetLibGetVlanHandle (
  IN EFI_HANDLE             ControllerHandle,
  IN UINT16                 VlanId
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *VlanDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  VLAN_DEVICE_PATH          VlanNode;
  EFI_HANDLE                Handle;

  ParentDevicePath = DevicePathFromHandle (ControllerHandle);
  if (ParentDevicePath == NULL) {
    return NULL;
  }

  //
  // Construct VLAN device path
  //
  CopyMem (&VlanNode, &mNetVlanDevicePathTemplate, sizeof (VLAN_DEVICE_PATH));
  VlanNode.VlanId = VlanId;
  VlanDevicePath = AppendDevicePathNode (
                     ParentDevicePath,
                     (EFI_DEVICE_PATH_PROTOCOL *) &VlanNode
                     );
  if (VlanDevicePath == NULL) {
    return NULL;
  }

  //
  // Find VLAN device handle
  //
  Handle = NULL;
  DevicePath = VlanDevicePath;
  gBS->LocateDevicePath (
         &gEfiDevicePathProtocolGuid,
         &DevicePath,
         &Handle
         );
  if (!IsDevicePathEnd (DevicePath)) {
    //
    // Device path is not exactly match
    //
    Handle = NULL;
  }

  FreePool (VlanDevicePath);
  return Handle;
}

/**
  Get MAC address associated with the network service handle.

  There should be MNP Service Binding Protocol installed on the input ServiceHandle.
  If SNP is installed on the ServiceHandle or its parent handle, MAC address will
  be retrieved from SNP. If no SNP found, try to get SNP mode data use MNP.

  @param[in]   ServiceHandle    The handle where network service binding protocols are
                                installed on.
  @param[out]  MacAddress       The pointer to store the returned MAC address.
  @param[out]  AddressSize      The length of returned MAC address.

  @retval EFI_SUCCESS           MAC address is returned successfully.
  @retval Others                Failed to get SNP mode data.

**/
EFI_STATUS
EFIAPI
NetLibGetMacAddress (
  IN  EFI_HANDLE            ServiceHandle,
  OUT EFI_MAC_ADDRESS       *MacAddress,
  OUT UINTN                 *AddressSize
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;
  EFI_SIMPLE_NETWORK_MODE      *SnpMode;
  EFI_SIMPLE_NETWORK_MODE      SnpModeData;
  EFI_MANAGED_NETWORK_PROTOCOL *Mnp;
  EFI_SERVICE_BINDING_PROTOCOL *MnpSb;
  EFI_HANDLE                   *SnpHandle;
  EFI_HANDLE                   MnpChildHandle;

  ASSERT (MacAddress != NULL);
  ASSERT (AddressSize != NULL);

  //
  // Try to get SNP handle
  //
  Snp = NULL;
  SnpHandle = NetLibGetSnpHandle (ServiceHandle, &Snp);
  if (SnpHandle != NULL) {
    //
    // SNP found, use it directly
    //
    SnpMode = Snp->Mode;
  } else {
    //
    // Failed to get SNP handle, try to get MAC address from MNP
    //
    MnpChildHandle = NULL;
    Status = gBS->HandleProtocol (
                    ServiceHandle,
                    &gEfiManagedNetworkServiceBindingProtocolGuid,
                    (VOID **) &MnpSb
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Create a MNP child
    //
    Status = MnpSb->CreateChild (MnpSb, &MnpChildHandle);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Open MNP protocol
    //
    Status = gBS->HandleProtocol (
                    MnpChildHandle,
                    &gEfiManagedNetworkProtocolGuid,
                    (VOID **) &Mnp
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Try to get SNP mode from MNP
    //
    Status = Mnp->GetModeData (Mnp, NULL, &SnpModeData);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    SnpMode = &SnpModeData;

    //
    // Destroy the MNP child
    //
    MnpSb->DestroyChild (MnpSb, MnpChildHandle);
  }

  *AddressSize = SnpMode->HwAddressSize;
  CopyMem (MacAddress->Addr, SnpMode->CurrentAddress.Addr, SnpMode->HwAddressSize);

  return EFI_SUCCESS;
}

/**
  Convert MAC address of the NIC associated with specified Service Binding Handle
  to a unicode string. Callers are responsible for freeing the string storage.

  Locate simple network protocol associated with the Service Binding Handle and
  get the mac address from SNP. Then convert the mac address into a unicode
  string. It takes 2 unicode characters to represent a 1 byte binary buffer.
  Plus one unicode character for the null-terminator.

  @param[in]   ServiceHandle         The handle where network service binding protocol is
                                     installed on.
  @param[in]   ImageHandle           The image handle used to act as the agent handle to
                                     get the simple network protocol.
  @param[out]  MacString             The pointer to store the address of the string
                                     representation of  the mac address.

  @retval EFI_SUCCESS           Convert the mac address a unicode string successfully.
  @retval EFI_OUT_OF_RESOURCES  There are not enough memory resource.
  @retval Others                Failed to open the simple network protocol.

**/
EFI_STATUS
EFIAPI
NetLibGetMacString (
  IN  EFI_HANDLE            ServiceHandle,
  IN  EFI_HANDLE            ImageHandle,
  OUT CHAR16                **MacString
  )
{
  EFI_STATUS                   Status;
  EFI_MAC_ADDRESS              MacAddress;
  UINT8                        *HwAddress;
  UINTN                        HwAddressSize;
  UINT16                       VlanId;
  CHAR16                       *String;
  UINTN                        Index;

  ASSERT (MacString != NULL);

  //
  // Get MAC address of the network device
  //
  Status = NetLibGetMacAddress (ServiceHandle, &MacAddress, &HwAddressSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // It takes 2 unicode characters to represent a 1 byte binary buffer.
  // If VLAN is configured, it will need extra 5 characters like "\0005".
  // Plus one unicode character for the null-terminator.
  //
  String = AllocateZeroPool ((2 * HwAddressSize + 5 + 1) * sizeof (CHAR16));
  if (String == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  *MacString = String;

  //
  // Convert the MAC address into a unicode string.
  //
  HwAddress = &MacAddress.Addr[0];
  for (Index = 0; Index < HwAddressSize; Index++) {
    String += UnicodeValueToString (String, PREFIX_ZERO | RADIX_HEX, *(HwAddress++), 2);
  }

  //
  // Append VLAN ID if any
  //
  VlanId = NetLibGetVlanId (ServiceHandle);
  if (VlanId != 0) {
    *String++ = L'\\';
    String += UnicodeValueToString (String, PREFIX_ZERO | RADIX_HEX, VlanId, 4);
  }

  //
  // Null terminate the Unicode string
  //
  *String = L'\0';

  return EFI_SUCCESS;
}

/**
  Check the default address used by the IPv4 driver is static or dynamic (acquired
  from DHCP).

  If the controller handle does not have the NIC Ip4 Config Protocol installed, the
  default address is static. If the EFI variable to save the configuration is not found,
  the default address is static. Otherwise, get the result from the EFI variable which
  saving the configuration.

  @param[in]   Controller     The controller handle which has the NIC Ip4 Config Protocol
                              relative with the default address to judge.

  @retval TRUE           If the default address is static.
  @retval FALSE          If the default address is acquired from DHCP.

**/
BOOLEAN
NetLibDefaultAddressIsStatic (
  IN EFI_HANDLE  Controller
  )
{
  EFI_STATUS                       Status;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  UINTN                            Len;
  NIC_IP4_CONFIG_INFO              *ConfigInfo;
  BOOLEAN                          IsStatic;
  EFI_STRING                       ConfigHdr;
  EFI_STRING                       ConfigResp;
  EFI_STRING                       AccessProgress;
  EFI_STRING                       AccessResults;
  EFI_STRING                       String;

  ConfigInfo       = NULL;
  ConfigHdr        = NULL;
  ConfigResp       = NULL;
  AccessProgress   = NULL;
  AccessResults    = NULL;
  IsStatic         = TRUE;

  Status = gBS->LocateProtocol (
                &gEfiHiiConfigRoutingProtocolGuid,
                NULL,
                (VOID **) &HiiConfigRouting
                );
  if (EFI_ERROR (Status)) {
    return TRUE;
  }

  //
  // Construct config request string header
  //
  ConfigHdr = HiiConstructConfigHdr (&gEfiNicIp4ConfigVariableGuid, EFI_NIC_IP4_CONFIG_VARIABLE, Controller);
  if (ConfigHdr == NULL) {
    return TRUE;
  }

  Len = StrLen (ConfigHdr);
  ConfigResp = AllocateZeroPool ((Len + NIC_ITEM_CONFIG_SIZE * 2 + 100) * sizeof (CHAR16));
  if (ConfigResp == NULL) {
    goto ON_EXIT;
  }
  StrCpy (ConfigResp, ConfigHdr);

  String = ConfigResp + Len;
  UnicodeSPrint (
    String,
    (8 + 4 + 7 + 4 + 1) * sizeof (CHAR16),
    L"&OFFSET=%04X&WIDTH=%04X",
    OFFSET_OF (NIC_IP4_CONFIG_INFO, Source),
    sizeof (UINT32)
    );

  Status = HiiConfigRouting->ExtractConfig (
                               HiiConfigRouting,
                               ConfigResp,
                               &AccessProgress,
                               &AccessResults
                               );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  ConfigInfo = AllocateZeroPool (sizeof (NIC_ITEM_CONFIG_SIZE));
  if (ConfigInfo == NULL) {
    goto ON_EXIT;
  }

  ConfigInfo->Source = IP4_CONFIG_SOURCE_STATIC;
  Len = NIC_ITEM_CONFIG_SIZE;
  Status = HiiConfigRouting->ConfigToBlock (
                               HiiConfigRouting,
                               AccessResults,
                               (UINT8 *) ConfigInfo,
                               &Len,
                               &AccessProgress
                               );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  IsStatic = (BOOLEAN) (ConfigInfo->Source == IP4_CONFIG_SOURCE_STATIC);

ON_EXIT:

  if (AccessResults != NULL) {
    FreePool (AccessResults);
  }
  if (ConfigInfo != NULL) {
    FreePool (ConfigInfo);
  }
  if (ConfigResp != NULL) {
    FreePool (ConfigResp);
  }
  if (ConfigHdr != NULL) {
    FreePool (ConfigHdr);
  }

  return IsStatic;
}

/**
  Create an IPv4 device path node.

  The header type of IPv4 device path node is MESSAGING_DEVICE_PATH.
  The header subtype of IPv4 device path node is MSG_IPv4_DP.
  The length of the IPv4 device path node in bytes is 19.
  Get other info from parameters to make up the whole IPv4 device path node.

  @param[in, out]  Node                  Pointer to the IPv4 device path node.
  @param[in]       Controller            The controller handle.
  @param[in]       LocalIp               The local IPv4 address.
  @param[in]       LocalPort             The local port.
  @param[in]       RemoteIp              The remote IPv4 address.
  @param[in]       RemotePort            The remote port.
  @param[in]       Protocol              The protocol type in the IP header.
  @param[in]       UseDefaultAddress     Whether this instance is using default address or not.

**/
VOID
EFIAPI
NetLibCreateIPv4DPathNode (
  IN OUT IPv4_DEVICE_PATH  *Node,
  IN EFI_HANDLE            Controller,
  IN IP4_ADDR              LocalIp,
  IN UINT16                LocalPort,
  IN IP4_ADDR              RemoteIp,
  IN UINT16                RemotePort,
  IN UINT16                Protocol,
  IN BOOLEAN               UseDefaultAddress
  )
{
  Node->Header.Type    = MESSAGING_DEVICE_PATH;
  Node->Header.SubType = MSG_IPv4_DP;
  SetDevicePathNodeLength (&Node->Header, 19);

  CopyMem (&Node->LocalIpAddress, &LocalIp, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&Node->RemoteIpAddress, &RemoteIp, sizeof (EFI_IPv4_ADDRESS));

  Node->LocalPort  = LocalPort;
  Node->RemotePort = RemotePort;

  Node->Protocol = Protocol;

  if (!UseDefaultAddress) {
    Node->StaticIpAddress = TRUE;
  } else {
    Node->StaticIpAddress = NetLibDefaultAddressIsStatic (Controller);
  }
}

/**
  Create an IPv6 device path node.

  The header type of IPv6 device path node is MESSAGING_DEVICE_PATH.
  The header subtype of IPv6 device path node is MSG_IPv6_DP.
  Get other info from parameters to make up the whole IPv6 device path node.

  @param[in, out]  Node                  Pointer to the IPv6 device path node.
  @param[in]       Controller            The controller handle.
  @param[in]       LocalIp               The local IPv6 address.
  @param[in]       LocalPort             The local port.
  @param[in]       RemoteIp              The remote IPv6 address.
  @param[in]       RemotePort            The remote port.
  @param[in]       Protocol              The protocol type in the IP header.

**/
VOID
EFIAPI
NetLibCreateIPv6DPathNode (
  IN OUT IPv6_DEVICE_PATH  *Node,
  IN EFI_HANDLE            Controller,
  IN EFI_IPv6_ADDRESS      *LocalIp,
  IN UINT16                LocalPort,
  IN EFI_IPv6_ADDRESS      *RemoteIp,
  IN UINT16                RemotePort,
  IN UINT16                Protocol
  )
{
  Node->Header.Type    = MESSAGING_DEVICE_PATH;
  Node->Header.SubType = MSG_IPv6_DP;
  SetDevicePathNodeLength (&Node->Header, sizeof (IPv6_DEVICE_PATH));

  CopyMem (&Node->LocalIpAddress, LocalIp, sizeof (EFI_IPv6_ADDRESS));
  CopyMem (&Node->RemoteIpAddress, RemoteIp, sizeof (EFI_IPv6_ADDRESS));

  Node->LocalPort  = LocalPort;
  Node->RemotePort = RemotePort;

  Node->Protocol        = Protocol;
  Node->StaticIpAddress = FALSE;
}

/**
  Find the UNDI/SNP handle from controller and protocol GUID.

  For example, IP will open a MNP child to transmit/receive
  packets, when MNP is stopped, IP should also be stopped. IP
  needs to find its own private data which is related the IP's
  service binding instance that is install on UNDI/SNP handle.
  Now, the controller is either a MNP or ARP child handle. But
  IP opens these handle BY_DRIVER, use that info, we can get the
  UNDI/SNP handle.

  @param[in]  Controller            Then protocol handle to check.
  @param[in]  ProtocolGuid          The protocol that is related with the handle.

  @return The UNDI/SNP handle or NULL for errors.

**/
EFI_HANDLE
EFIAPI
NetLibGetNicHandle (
  IN EFI_HANDLE             Controller,
  IN EFI_GUID               *ProtocolGuid
  )
{
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenBuffer;
  EFI_HANDLE                          Handle;
  EFI_STATUS                          Status;
  UINTN                               OpenCount;
  UINTN                               Index;

  Status = gBS->OpenProtocolInformation (
                  Controller,
                  ProtocolGuid,
                  &OpenBuffer,
                  &OpenCount
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Handle = NULL;

  for (Index = 0; Index < OpenCount; Index++) {
    if ((OpenBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
      Handle = OpenBuffer[Index].ControllerHandle;
      break;
    }
  }

  gBS->FreePool (OpenBuffer);
  return Handle;
}
