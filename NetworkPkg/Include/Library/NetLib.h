/** @file
  This library is only intended to be used by UEFI network stack modules.
  It provides basic functions for the UEFI network stack.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _NET_LIB_H_
#define _NET_LIB_H_

#include <Protocol/Ip6.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

typedef UINT32          IP4_ADDR;
typedef UINT32          TCP_SEQNO;
typedef UINT16          TCP_PORTNO;


#define  NET_ETHER_ADDR_LEN    6
#define  NET_IFTYPE_ETHERNET   0x01

#define  NET_VLAN_TAG_LEN      4
#define  ETHER_TYPE_VLAN       0x8100

#define  EFI_IP_PROTO_UDP      0x11
#define  EFI_IP_PROTO_TCP      0x06
#define  EFI_IP_PROTO_ICMP     0x01
#define  IP4_PROTO_IGMP        0x02
#define  IP6_ICMP              58
#define  DNS_MAX_NAME_SIZE     255
#define  DNS_MAX_MESSAGE_SIZE  512

//
// The address classification
//
#define  IP4_ADDR_CLASSA       1     // Deprecated
#define  IP4_ADDR_CLASSB       2     // Deprecated
#define  IP4_ADDR_CLASSC       3     // Deprecated
#define  IP4_ADDR_CLASSD       4
#define  IP4_ADDR_CLASSE       5

#define  IP4_MASK_NUM          33
#define  IP6_PREFIX_NUM        129

#define  IP4_MASK_MAX          32
#define  IP6_PREFIX_MAX        128

#define  IP6_HOP_BY_HOP        0
#define  IP6_DESTINATION       60
#define  IP6_ROUTING           43
#define  IP6_FRAGMENT          44
#define  IP6_AH                51
#define  IP6_ESP               50
#define  IP6_NO_NEXT_HEADER    59

#define  IP_VERSION_4          4
#define  IP_VERSION_6          6

#define  IP6_PREFIX_LENGTH     64

//
// DNS QTYPE values
//
#define  DNS_TYPE_A            1
#define  DNS_TYPE_NS           2
#define  DNS_TYPE_CNAME        5
#define  DNS_TYPE_SOA          6
#define  DNS_TYPE_WKS          11
#define  DNS_TYPE_PTR          12
#define  DNS_TYPE_HINFO        13
#define  DNS_TYPE_MINFO        14
#define  DNS_TYPE_MX           15
#define  DNS_TYPE_TXT          16
#define  DNS_TYPE_AAAA         28
#define  DNS_TYPE_SRV_RR       33
#define  DNS_TYPE_AXFR         252
#define  DNS_TYPE_MAILB        253
#define  DNS_TYPE_ANY          255

//
// DNS QCLASS values
//
#define  DNS_CLASS_INET        1
#define  DNS_CLASS_CH          3
#define  DNS_CLASS_HS          4
#define  DNS_CLASS_ANY         255

//
// Number of 100ns units time Interval for network media state detect
//
#define MEDIA_STATE_DETECT_TIME_INTERVAL  1000000U


#pragma pack(1)

//
// Ethernet head definition
//
typedef struct {
  UINT8                 DstMac [NET_ETHER_ADDR_LEN];
  UINT8                 SrcMac [NET_ETHER_ADDR_LEN];
  UINT16                EtherType;
} ETHER_HEAD;

//
// 802.1Q VLAN Tag Control Information
//
typedef union {
  struct {
    UINT16              Vid      : 12;  // Unique VLAN identifier (0 to 4094)
    UINT16              Cfi      : 1;   // Canonical Format Indicator
    UINT16              Priority : 3;   // 802.1Q priority level (0 to 7)
  } Bits;
  UINT16                Uint16;
} VLAN_TCI;

#define VLAN_TCI_CFI_CANONICAL_MAC      0
#define VLAN_TCI_CFI_NON_CANONICAL_MAC  1

//
// The EFI_IP4_HEADER is hard to use because the source and
// destination address are defined as EFI_IPv4_ADDRESS, which
// is a structure. Two structures can't be compared or masked
// directly. This is why there is an internal representation.
//
typedef struct {
  UINT8                 HeadLen : 4;
  UINT8                 Ver     : 4;
  UINT8                 Tos;
  UINT16                TotalLen;
  UINT16                Id;
  UINT16                Fragment;
  UINT8                 Ttl;
  UINT8                 Protocol;
  UINT16                Checksum;
  IP4_ADDR              Src;
  IP4_ADDR              Dst;
} IP4_HEAD;


//
// ICMP head definition. Each ICMP message is categorized as either an error
// message or query message. Two message types have their own head format.
//
typedef struct {
  UINT8                 Type;
  UINT8                 Code;
  UINT16                Checksum;
} IP4_ICMP_HEAD;

typedef struct {
  IP4_ICMP_HEAD         Head;
  UINT32                Fourth; // 4th filed of the head, it depends on Type.
  IP4_HEAD              IpHead;
} IP4_ICMP_ERROR_HEAD;

typedef struct {
  IP4_ICMP_HEAD         Head;
  UINT16                Id;
  UINT16                Seq;
} IP4_ICMP_QUERY_HEAD;

typedef struct {
  UINT8                 Type;
  UINT8                 Code;
  UINT16                Checksum;
} IP6_ICMP_HEAD;

typedef struct {
  IP6_ICMP_HEAD         Head;
  UINT32                Fourth;
  EFI_IP6_HEADER        IpHead;
} IP6_ICMP_ERROR_HEAD;

typedef struct {
  IP6_ICMP_HEAD         Head;
  UINT32                Fourth;
} IP6_ICMP_INFORMATION_HEAD;

//
// UDP header definition
//
typedef struct {
  UINT16                SrcPort;
  UINT16                DstPort;
  UINT16                Length;
  UINT16                Checksum;
} EFI_UDP_HEADER;

//
// TCP header definition
//
typedef struct {
  TCP_PORTNO            SrcPort;
  TCP_PORTNO            DstPort;
  TCP_SEQNO             Seq;
  TCP_SEQNO             Ack;
  UINT8                 Res     : 4;
  UINT8                 HeadLen : 4;
  UINT8                 Flag;
  UINT16                Wnd;
  UINT16                Checksum;
  UINT16                Urg;
} TCP_HEAD;

#pragma pack()

#define NET_MAC_EQUAL(pMac1, pMac2, Len)     \
    (CompareMem ((pMac1), (pMac2), Len) == 0)

#define NET_MAC_IS_MULTICAST(Mac, BMac, Len) \
    (((*((UINT8 *) Mac) & 0x01) == 0x01) && (!NET_MAC_EQUAL (Mac, BMac, Len)))

#define NTOHL(x)  SwapBytes32 (x)

#define HTONL(x)  NTOHL(x)

#define NTOHS(x)  SwapBytes16 (x)

#define HTONS(x)   NTOHS(x)
#define NTOHLL(x)  SwapBytes64 (x)
#define HTONLL(x)  NTOHLL(x)
#define NTOHLLL(x) Ip6Swap128 (x)
#define HTONLLL(x) NTOHLLL(x)

//
// Test the IP's attribute, All the IPs are in host byte order.
//
#define IP4_IS_MULTICAST(Ip)              (((Ip) & 0xF0000000) == 0xE0000000)
#define IP4_IS_UNSPECIFIED(Ip)            ((Ip) == 0)
#define IP4_IS_LOCAL_BROADCAST(Ip)        ((Ip) == 0xFFFFFFFF)
#define IP4_NET_EQUAL(Ip1, Ip2, NetMask)  (((Ip1) & (NetMask)) == ((Ip2) & (NetMask)))
#define IP4_IS_VALID_NETMASK(Ip)          (NetGetMaskLength (Ip) != (IP4_MASK_MAX + 1))

#define IP6_IS_MULTICAST(Ip6)             (((Ip6)->Addr[0]) == 0xFF)

//
// Convert the EFI_IP4_ADDRESS to plain UINT32 IP4 address.
//
#define EFI_IP4(EfiIpAddr)       (*(IP4_ADDR *) ((EfiIpAddr).Addr))
#define EFI_NTOHL(EfiIp)         (NTOHL (EFI_IP4 ((EfiIp))))
#define EFI_IP4_EQUAL(Ip1, Ip2)  (CompareMem ((Ip1), (Ip2), sizeof (EFI_IPv4_ADDRESS)) == 0)

#define EFI_IP6_EQUAL(Ip1, Ip2)  (CompareMem ((Ip1), (Ip2), sizeof (EFI_IPv6_ADDRESS)) == 0)

#define IP4_COPY_ADDRESS(Dest, Src) (CopyMem ((Dest), (Src), sizeof (EFI_IPv4_ADDRESS)))
#define IP6_COPY_ADDRESS(Dest, Src) (CopyMem ((Dest), (Src), sizeof (EFI_IPv6_ADDRESS)))
#define IP6_COPY_LINK_ADDRESS(Mac1, Mac2) (CopyMem ((Mac1), (Mac2), sizeof (EFI_MAC_ADDRESS)))

//
// The debug level definition. This value is also used as the
// syslog's severity level. Don't change it.
//
#define NETDEBUG_LEVEL_TRACE   5
#define NETDEBUG_LEVEL_WARNING 4
#define NETDEBUG_LEVEL_ERROR   3

//
// Network debug message is sent out as syslog packet.
//
#define NET_SYSLOG_FACILITY    16                 // Syslog local facility local use
#define NET_SYSLOG_PACKET_LEN  512
#define NET_SYSLOG_TX_TIMEOUT  (500 * 1000 * 10)  // 500ms
#define NET_DEBUG_MSG_LEN      470                // 512 - (ether+ip4+udp4 head length)

//
// The debug output expects the ASCII format string, Use %a to print ASCII
// string, and %s to print UNICODE string. PrintArg must be enclosed in ().
// For example: NET_DEBUG_TRACE ("Tcp", ("State transit to %a\n", Name));
//
#define NET_DEBUG_TRACE(Module, PrintArg) \
  NetDebugOutput ( \
    NETDEBUG_LEVEL_TRACE, \
    Module, \
    __FILE__, \
    __LINE__, \
    NetDebugASPrint PrintArg \
    )

#define NET_DEBUG_WARNING(Module, PrintArg) \
  NetDebugOutput ( \
    NETDEBUG_LEVEL_WARNING, \
    Module, \
    __FILE__, \
    __LINE__, \
    NetDebugASPrint PrintArg \
    )

#define NET_DEBUG_ERROR(Module, PrintArg) \
  NetDebugOutput ( \
    NETDEBUG_LEVEL_ERROR, \
    Module, \
    __FILE__, \
    __LINE__, \
    NetDebugASPrint PrintArg \
    )

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
                 or NULL if memory allocation failed.

**/
CHAR8 *
EFIAPI
NetDebugASPrint (
  IN CHAR8                  *Format,
  ...
  );

/**
  Builds an UDP4 syslog packet and send it using SNP.

  This function will locate a instance of SNP then send the message through it.
  Because it isn't open the SNP BY_DRIVER, apply caution when using it.

  @param Level    The severity level of the message.
  @param Module   The Module that generates the log.
  @param File     The file that contains the log.
  @param Line     The exact line that contains the log.
  @param Message  The user message to log.

  @retval EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the packet
  @retval EFI_SUCCESS           The log is discard because that it is more verbose
                                than the mNetDebugLevelMax. Or, it has been sent out.
**/
EFI_STATUS
EFIAPI
NetDebugOutput (
  IN UINT32                    Level,
  IN UINT8                     *Module,
  IN UINT8                     *File,
  IN UINT32                    Line,
  IN UINT8                     *Message
  );


/**
  Return the length of the mask.

  Return the length of the mask. Valid values are 0 to 32.
  If the mask is invalid, return the invalid length 33, which is IP4_MASK_NUM.
  NetMask is in the host byte order.

  @param[in]  NetMask              The netmask to get the length from.

  @return The length of the netmask, or IP4_MASK_NUM (33) if the mask is invalid.

**/
INTN
EFIAPI
NetGetMaskLength (
  IN IP4_ADDR               NetMask
  );

/**
  Return the class of the IP address, such as class A, B, C.
  Addr is in host byte order.

  [ATTENTION]
  Classful addressing (IP class A/B/C) has been deprecated according to RFC4632.
  Caller of this function could only check the returned value against
  IP4_ADDR_CLASSD (multicast) or IP4_ADDR_CLASSE (reserved) now.

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
  );

/**
  Check whether the IP is a valid unicast address according to
  the netmask.

  ASSERT if NetMask is zero.

  If all bits of the host address of IP are 0 or 1, IP is also not a valid unicast address,
  except when the originator is one of the endpoints of a point-to-point link with a 31-bit
  mask (RFC3021), or a 32bit NetMask (all 0xFF) is used for special network environment (e.g.
  PPP link).

  @param[in]  Ip                    The IP to check against.
  @param[in]  NetMask               The mask of the IP.

  @return TRUE if IP is a valid unicast address on the network, otherwise FALSE.

**/
BOOLEAN
EFIAPI
NetIp4IsUnicast (
  IN IP4_ADDR               Ip,
  IN IP4_ADDR               NetMask
  );

/**
  Check whether the incoming IPv6 address is a valid unicast address.

  ASSERT if Ip6 is NULL.

  If the address is a multicast address has binary 0xFF at the start, it is not
  a valid unicast address. If the address is unspecified ::, it is not a valid
  unicast address to be assigned to any node. If the address is loopback address
  ::1, it is also not a valid unicast address to be assigned to any physical
  interface.

  @param[in]  Ip6                   The IPv6 address to check against.

  @return TRUE if Ip6 is a valid unicast address on the network, otherwise FALSE.

**/
BOOLEAN
EFIAPI
NetIp6IsValidUnicast (
  IN EFI_IPv6_ADDRESS       *Ip6
  );


/**
  Check whether the incoming Ipv6 address is the unspecified address or not.

  ASSERT if Ip6 is NULL.

  @param[in] Ip6   - Ip6 address, in network order.

  @retval TRUE     - Yes, incoming Ipv6 address is the unspecified address.
  @retval FALSE    - The incoming Ipv6 address is not the unspecified address

**/
BOOLEAN
EFIAPI
NetIp6IsUnspecifiedAddr (
  IN EFI_IPv6_ADDRESS       *Ip6
  );

/**
  Check whether the incoming Ipv6 address is a link-local address.

  ASSERT if Ip6 is NULL.

  @param[in] Ip6   - Ip6 address, in network order.

  @retval TRUE  - The incoming Ipv6 address is a link-local address.
  @retval FALSE - The incoming Ipv6 address is not a link-local address.

**/
BOOLEAN
EFIAPI
NetIp6IsLinkLocalAddr (
  IN EFI_IPv6_ADDRESS *Ip6
  );

/**
  Check whether the Ipv6 address1 and address2 are on the connected network.

  ASSERT if Ip1 or Ip2 is NULL.
  ASSERT if PrefixLength exceeds or equals to IP6_PREFIX_MAX.

  @param[in] Ip1          - Ip6 address1, in network order.
  @param[in] Ip2          - Ip6 address2, in network order.
  @param[in] PrefixLength - The prefix length of the checking net.

  @retval TRUE            - Yes, the Ipv6 address1 and address2 are connected.
  @retval FALSE           - No the Ipv6 address1 and address2 are not connected.

**/
BOOLEAN
EFIAPI
NetIp6IsNetEqual (
  EFI_IPv6_ADDRESS *Ip1,
  EFI_IPv6_ADDRESS *Ip2,
  UINT8            PrefixLength
  );

/**
  Switches the endianess of an IPv6 address.

  ASSERT if Ip6 is NULL.

  This function swaps the bytes in a 128-bit IPv6 address to switch the value
  from little endian to big endian or vice versa. The byte swapped value is
  returned.

  @param  Ip6 Points to an IPv6 address.

  @return The byte swapped IPv6 address.

**/
EFI_IPv6_ADDRESS *
EFIAPI
Ip6Swap128 (
  EFI_IPv6_ADDRESS *Ip6
  );

extern IP4_ADDR gIp4AllMasks[IP4_MASK_NUM];


extern EFI_IPv4_ADDRESS  mZeroIp4Addr;

#define NET_IS_DIGIT(Ch)            (('0' <= (Ch)) && ((Ch) <= '9'))
#define NET_IS_HEX(Ch)              ((('0' <= (Ch)) && ((Ch) <= '9')) || (('A' <= (Ch)) && ((Ch) <= 'F')) || (('a' <= (Ch)) && ((Ch) <= 'f')))
#define NET_ROUNDUP(size, unit)     (((size) + (unit) - 1) & (~((unit) - 1)))
#define NET_IS_LOWER_CASE_CHAR(Ch)  (('a' <= (Ch)) && ((Ch) <= 'z'))
#define NET_IS_UPPER_CASE_CHAR(Ch)  (('A' <= (Ch)) && ((Ch) <= 'Z'))

#define TICKS_PER_MS            10000U
#define TICKS_PER_SECOND        10000000U

#define NET_RANDOM(Seed)        ((UINT32) ((UINT32) (Seed) * 1103515245UL + 12345) % 4294967295UL)

/**
  Extract a UINT32 from a byte stream.

  ASSERT if Buf is NULL.

  This function copies a UINT32 from a byte stream, and then converts it from Network
  byte order to host byte order. Use this function to avoid alignment error.

  @param[in]  Buf                 The buffer to extract the UINT32.

  @return The UINT32 extracted.

**/
UINT32
EFIAPI
NetGetUint32 (
  IN UINT8                  *Buf
  );

/**
  Puts a UINT32 into the byte stream in network byte order.

  ASSERT if Buf is NULL.

  Converts a UINT32 from host byte order to network byte order, then copies it to the
  byte stream.

  @param[in, out]  Buf          The buffer in which to put the UINT32.
  @param[in]       Data         The data to be converted and put into the byte stream.

**/
VOID
EFIAPI
NetPutUint32 (
  IN OUT UINT8                 *Buf,
  IN     UINT32                Data
  );

/**
  Initialize a random seed using current time and monotonic count.

  Get current time and monotonic count first. Then initialize a random seed
  based on some basic mathematics operation on the hour, day, minute, second,
  nanosecond and year of the current time and the monotonic count value.

  @return The random seed initialized with current time.

**/
UINT32
EFIAPI
NetRandomInitSeed (
  VOID
  );


#define NET_LIST_USER_STRUCT(Entry, Type, Field)        \
          BASE_CR(Entry, Type, Field)

#define NET_LIST_USER_STRUCT_S(Entry, Type, Field, Sig)  \
          CR(Entry, Type, Field, Sig)

//
// Iterate through the double linked list. It is NOT delete safe
//
#define NET_LIST_FOR_EACH(Entry, ListHead) \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

//
// Iterate through the double linked list. This is delete-safe.
// Don't touch NextEntry. Also, don't use this macro if list
// entries other than the Entry may be deleted when processing
// the current Entry.
//
#define NET_LIST_FOR_EACH_SAFE(Entry, NextEntry, ListHead) \
  for(Entry = (ListHead)->ForwardLink, NextEntry = Entry->ForwardLink; \
      Entry != (ListHead); \
      Entry = NextEntry, NextEntry = Entry->ForwardLink \
     )

//
// Make sure the list isn't empty before getting the first/last record.
//
#define NET_LIST_HEAD(ListHead, Type, Field)  \
          NET_LIST_USER_STRUCT((ListHead)->ForwardLink, Type, Field)

#define NET_LIST_TAIL(ListHead, Type, Field)  \
          NET_LIST_USER_STRUCT((ListHead)->BackLink, Type, Field)


/**
  Remove the first node entry on the list, and return the removed node entry.

  Removes the first node entry from a doubly linked list. It is up to the caller of
  this function to release the memory used by the first node, if that is required. On
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
  );

/**
  Remove the last node entry on the list and return the removed node entry.

  Removes the last node entry from a doubly linked list. It is up to the caller of
  this function to release the memory used by the first node, if that is required. On
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
  );

/**
  Insert a new node entry after a designated node entry of a doubly linked list.

  ASSERT if PrevEntry or NewEntry is NULL.

  Inserts a new node entry designated by NewEntry after the node entry designated by PrevEntry
  of the doubly linked list.

  @param[in, out]  PrevEntry             The entry after which to insert.
  @param[in, out]  NewEntry              The new entry to insert.

**/
VOID
EFIAPI
NetListInsertAfter (
  IN OUT LIST_ENTRY         *PrevEntry,
  IN OUT LIST_ENTRY         *NewEntry
  );

/**
  Insert a new node entry before a designated node entry of a doubly linked list.

  ASSERT if PostEntry or NewEntry is NULL.

  Inserts a new node entry designated by NewEntry before the node entry designated by PostEntry
  of the doubly linked list.

  @param[in, out]  PostEntry             The entry to insert before.
  @param[in, out]  NewEntry              The new entry to insert.

**/
VOID
EFIAPI
NetListInsertBefore (
  IN OUT LIST_ENTRY     *PostEntry,
  IN OUT LIST_ENTRY     *NewEntry
  );

/**
  Callback function which provided by user to remove one node in NetDestroyLinkList process.

  @param[in]    Entry           The entry to be removed.
  @param[in]    Context         Pointer to the callback context corresponds to the Context in NetDestroyLinkList.

  @retval EFI_SUCCESS           The entry has been removed successfully.
  @retval Others                Fail to remove the entry.

**/
typedef
EFI_STATUS
(EFIAPI *NET_DESTROY_LINK_LIST_CALLBACK) (
  IN LIST_ENTRY         *Entry,
  IN VOID               *Context   OPTIONAL
  );

/**
  Safe destroy nodes in a linked list, and return the length of the list after all possible operations finished.

  Destroy network children list by list traversals is not safe due to graph dependencies between nodes.
  This function performs a safe traversal to destroy these nodes by checking to see if the node being destroyed
  has been removed from the list or not.
  If it has been removed, then restart the traversal from the head.
  If it hasn't been removed, then continue with the next node directly.
  This function will end the iterate and return the CallBack's last return value if error happens,
  or retrun EFI_SUCCESS if 2 complete passes are made with no changes in the number of children in the list.

  @param[in]    List             The head of the list.
  @param[in]    CallBack         Pointer to the callback function to destroy one node in the list.
  @param[in]    Context          Pointer to the callback function's context: corresponds to the
                                 parameter Context in NET_DESTROY_LINK_LIST_CALLBACK.
  @param[out]   ListLength       The length of the link list if the function returns successfully.

  @retval EFI_SUCCESS            Two complete passes are made with no changes in the number of children.
  @retval EFI_INVALID_PARAMETER  The input parameter is invalid.
  @retval Others                 Return the CallBack's last return value.

**/
EFI_STATUS
EFIAPI
NetDestroyLinkList (
  IN   LIST_ENTRY                       *List,
  IN   NET_DESTROY_LINK_LIST_CALLBACK   CallBack,
  IN   VOID                             *Context,    OPTIONAL
  OUT  UINTN                            *ListLength  OPTIONAL
  );

/**
  This function checks the input Handle to see if it's one of these handles in ChildHandleBuffer.

  @param[in]  Handle             Handle to be checked.
  @param[in]  NumberOfChildren   Number of Handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer  An array of child handles to be freed. May be NULL
                                 if NumberOfChildren is 0.

  @retval TRUE                   Found the input Handle in ChildHandleBuffer.
  @retval FALSE                  Can't find the input Handle in ChildHandleBuffer.

**/
BOOLEAN
EFIAPI
NetIsInHandleBuffer (
  IN  EFI_HANDLE          Handle,
  IN  UINTN               NumberOfChildren,
  IN  EFI_HANDLE          *ChildHandleBuffer OPTIONAL
  );

//
// Object container: EFI network stack spec defines various kinds of
// tokens. The drivers can share code to manage those objects.
//
typedef struct {
  LIST_ENTRY                Link;
  VOID                      *Key;
  VOID                      *Value;
} NET_MAP_ITEM;

typedef struct {
  LIST_ENTRY                Used;
  LIST_ENTRY                Recycled;
  UINTN                     Count;
} NET_MAP;

#define NET_MAP_INCREAMENT  64

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
  );

/**
  To clean up the netmap, that is, release allocated memories.

  Removes all nodes of the Used doubly linked list and frees memory of all related netmap items.
  Removes all nodes of the Recycled doubly linked list and free memory of all related netmap items.
  The number of the <Key, Value> pairs in the netmap is set to zero.

  If Map is NULL, then ASSERT().

  @param[in, out]  Map                   The netmap to clean up.

**/
VOID
EFIAPI
NetMapClean (
  IN OUT NET_MAP            *Map
  );

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
  );

/**
  Return the number of the <Key, Value> pairs in the netmap.

  If Map is NULL, then ASSERT().

  @param[in]  Map                   The netmap to get the entry number.

  @return The entry number in the netmap.

**/
UINTN
EFIAPI
NetMapGetCount (
  IN NET_MAP                *Map
  );

/**
  Allocate an item to save the <Key, Value> pair to the head of the netmap.

  Allocate an item to save the <Key, Value> pair and add corresponding node entry
  to the beginning of the Used doubly linked list. The number of the <Key, Value>
  pairs in the netmap increase by 1.

  If Map is NULL, then ASSERT().
  If Key is NULL, then ASSERT().

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
  );

/**
  Allocate an item to save the <Key, Value> pair to the tail of the netmap.

  Allocate an item to save the <Key, Value> pair and add corresponding node entry
  to the tail of the Used doubly linked list. The number of the <Key, Value>
  pairs in the netmap increase by 1.

  If Map is NULL, then ASSERT().
  If Key is NULL, then ASSERT().

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
  );

/**
  Finds the key in the netmap and returns the point to the item containing the Key.

  Iterate the Used doubly linked list of the netmap to get every item. Compare the key of every
  item with the key to search. It returns the point to the item contains the Key if found.

  If Map is NULL, then ASSERT().
  If Key is NULL, then ASSERT().

  @param[in]  Map                   The netmap to search within.
  @param[in]  Key                   The key to search.

  @return The point to the item contains the Key, or NULL if Key isn't in the map.

**/
NET_MAP_ITEM *
EFIAPI
NetMapFindKey (
  IN  NET_MAP               *Map,
  IN  VOID                  *Key
  );

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
  );

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
  );

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
  );

typedef
EFI_STATUS
(EFIAPI *NET_MAP_CALLBACK) (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Arg
  );

/**
  Iterate through the netmap and call CallBack for each item.

  It will continue the traverse if CallBack returns EFI_SUCCESS, otherwise, break
  from the loop. It returns the CallBack's last return value. This function is
  delete safe for the current item.

  If Map is NULL, then ASSERT().
  If CallBack is NULL, then ASSERT().

  @param[in]  Map                   The Map to iterate through.
  @param[in]  CallBack              The callback function to call for each item.
  @param[in]  Arg                   The opaque parameter to the callback.

  @retval EFI_SUCCESS            There is no item in the netmap, or CallBack for each item
                                 returns EFI_SUCCESS.
  @retval Others                 It returns the CallBack's last return value.

**/
EFI_STATUS
EFIAPI
NetMapIterate (
  IN NET_MAP                *Map,
  IN NET_MAP_CALLBACK       CallBack,
  IN VOID                   *Arg      OPTIONAL
  );


//
// Helper functions to implement driver binding and service binding protocols.
//
/**
  Create a child of the service that is identified by ServiceBindingGuid.

  Get the ServiceBinding Protocol first, then use it to create a child.

  If ServiceBindingGuid is NULL, then ASSERT().
  If ChildHandle is NULL, then ASSERT().

  @param[in]       Controller            The controller which has the service installed.
  @param[in]       Image                 The image handle used to open service.
  @param[in]       ServiceBindingGuid    The service's Guid.
  @param[in, out]  ChildHandle           The handle to receive the created child.

  @retval EFI_SUCCESS           The child was successfully created.
  @retval Others                Failed to create the child.

**/
EFI_STATUS
EFIAPI
NetLibCreateServiceChild (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            Image,
  IN  EFI_GUID              *ServiceBindingGuid,
  IN  OUT EFI_HANDLE        *ChildHandle
  );

/**
  Destroy a child of the service that is identified by ServiceBindingGuid.

  Get the ServiceBinding Protocol first, then use it to destroy a child.

  If ServiceBindingGuid is NULL, then ASSERT().

  @param[in]   Controller            The controller which has the service installed.
  @param[in]   Image                 The image handle used to open service.
  @param[in]   ServiceBindingGuid    The service's Guid.
  @param[in]   ChildHandle           The child to destroy.

  @retval EFI_SUCCESS           The child was destroyed.
  @retval Others                Failed to destroy the child.

**/
EFI_STATUS
EFIAPI
NetLibDestroyServiceChild (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            Image,
  IN  EFI_GUID              *ServiceBindingGuid,
  IN  EFI_HANDLE            ChildHandle
  );

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
  );

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
  );

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
  );

/**
  Get MAC address associated with the network service handle.

  If MacAddress is NULL, then ASSERT().
  If AddressSize is NULL, then ASSERT().

  There should be MNP Service Binding Protocol installed on the input ServiceHandle.
  If SNP is installed on the ServiceHandle or its parent handle, MAC address will
  be retrieved from SNP. If no SNP found, try to get SNP mode data use MNP.

  @param[in]   ServiceHandle    The handle where network service binding protocols are
                                installed on.
  @param[out]  MacAddress       The pointer to store the returned MAC address.
  @param[out]  AddressSize      The length of returned MAC address.

  @retval EFI_SUCCESS           MAC address was returned successfully.
  @retval Others                Failed to get SNP mode data.

**/
EFI_STATUS
EFIAPI
NetLibGetMacAddress (
  IN  EFI_HANDLE            ServiceHandle,
  OUT EFI_MAC_ADDRESS       *MacAddress,
  OUT UINTN                 *AddressSize
  );

/**
  Convert MAC address of the NIC associated with specified Service Binding Handle
  to a unicode string. Callers are responsible for freeing the string storage.

  If MacString is NULL, then ASSERT().

  Locate simple network protocol associated with the Service Binding Handle and
  get the mac address from SNP. Then convert the mac address into a unicode
  string. It takes 2 unicode characters to represent a 1 byte binary buffer.
  Plus one unicode character for the null-terminator.

  @param[in]   ServiceHandle         The handle where network service binding protocol is
                                     installed.
  @param[in]   ImageHandle           The image handle used to act as the agent handle to
                                     get the simple network protocol. This parameter is
                                     optional and may be NULL.
  @param[out]  MacString             The pointer to store the address of the string
                                     representation of  the mac address.

  @retval EFI_SUCCESS           Converted the mac address a unicode string successfully.
  @retval EFI_OUT_OF_RESOURCES  There are not enough memory resources.
  @retval Others                Failed to open the simple network protocol.

**/
EFI_STATUS
EFIAPI
NetLibGetMacString (
  IN  EFI_HANDLE            ServiceHandle,
  IN  EFI_HANDLE            ImageHandle, OPTIONAL
  OUT CHAR16                **MacString
  );

/**
  Detect media status for specified network device.

  If MediaPresent is NULL, then ASSERT().

  The underlying UNDI driver may or may not support reporting media status from
  GET_STATUS command (PXE_STATFLAGS_GET_STATUS_NO_MEDIA_SUPPORTED). This routine
  will try to invoke Snp->GetStatus() to get the media status. If media is already
  present, it returns directly. If media is not present, it will stop SNP and then
  restart SNP to get the latest media status. This provides an opportunity to get
  the correct media status for old UNDI driver, which doesn't support reporting
  media status from GET_STATUS command.
  Note: there are two limitations for the current algorithm:
  1) For UNDI with this capability, when the cable is not attached, there will
     be an redundant Stop/Start() process.
  2) for UNDI without this capability, in case that network cable is attached when
     Snp->Initialize() is invoked while network cable is unattached later,
     NetLibDetectMedia() will report MediaPresent as TRUE, causing upper layer
     apps to wait for timeout time.

  @param[in]   ServiceHandle    The handle where network service binding protocols are
                                installed.
  @param[out]  MediaPresent     The pointer to store the media status.

  @retval EFI_SUCCESS           Media detection success.
  @retval EFI_INVALID_PARAMETER ServiceHandle is not a valid network device handle.
  @retval EFI_UNSUPPORTED       The network device does not support media detection.
  @retval EFI_DEVICE_ERROR      SNP is in an unknown state.

**/
EFI_STATUS
EFIAPI
NetLibDetectMedia (
  IN  EFI_HANDLE            ServiceHandle,
  OUT BOOLEAN               *MediaPresent
  );

/**
  Detect media state for a network device. This routine will wait for a period of time at
  a specified checking interval when a certain network is under connecting until connection
  process finishes or timeout. If Aip protocol is supported by low layer drivers, three kinds
  of media states can be detected: EFI_SUCCESS, EFI_NOT_READY and EFI_NO_MEDIA, represents
  connected state, connecting state and no media state respectively. When function detects
  the current state is EFI_NOT_READY, it will loop to wait for next time's check until state
  turns to be EFI_SUCCESS or EFI_NO_MEDIA. If Aip protocol is not supported, function will
  call NetLibDetectMedia() and return state directly.

  @param[in]   ServiceHandle    The handle where network service binding protocols are
                                installed on.
  @param[in]   Timeout          The maximum number of 100ns units to wait when network
                                is connecting. Zero value means detect once and return
                                immediately.
  @param[out]  MediaState       The pointer to the detected media state.

  @retval EFI_SUCCESS           Media detection success.
  @retval EFI_INVALID_PARAMETER ServiceHandle is not a valid network device handle or
                                MediaState pointer is NULL.
  @retval EFI_DEVICE_ERROR      A device error occurred.
  @retval EFI_TIMEOUT           Network is connecting but timeout.

**/
EFI_STATUS
EFIAPI
NetLibDetectMediaWaitTimeout (
  IN  EFI_HANDLE            ServiceHandle,
  IN  UINT64                Timeout,
  OUT EFI_STATUS            *MediaState
  );


/**
  Create an IPv4 device path node.

  If Node is NULL, then ASSERT().

  The header type of IPv4 device path node is MESSAGING_DEVICE_PATH.
  The header subtype of IPv4 device path node is MSG_IPv4_DP.
  The length of the IPv4 device path node in bytes is 19.
  Get other information from parameters to make up the whole IPv4 device path node.

  @param[in, out]  Node                  The pointer to the IPv4 device path node.
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
  );

/**
  Create an IPv6 device path node.

  If Node is NULL, then ASSERT().
  If LocalIp is NULL, then ASSERT().
  If RemoteIp is NULL, then ASSERT().

  The header type of IPv6 device path node is MESSAGING_DEVICE_PATH.
  The header subtype of IPv6 device path node is MSG_IPv6_DP.
  The length of the IPv6 device path node in bytes is 43.
  Get other information from parameters to make up the whole IPv6 device path node.

  @param[in, out]  Node                  The pointer to the IPv6 device path node.
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
  );


/**
  Find the UNDI/SNP handle from controller and protocol GUID.

  If ProtocolGuid is NULL, then ASSERT().

  For example, IP will open an MNP child to transmit/receive
  packets. When MNP is stopped, IP should also be stopped. IP
  needs to find its own private data that is related the IP's
  service binding instance that is installed on the UNDI/SNP handle.
  The controller is then either an MNP or an ARP child handle. Note that
  IP opens these handles using BY_DRIVER. Use that information to get the
  UNDI/SNP handle.

  @param[in]  Controller            The protocol handle to check.
  @param[in]  ProtocolGuid          The protocol that is related with the handle.

  @return The UNDI/SNP handle or NULL for errors.

**/
EFI_HANDLE
EFIAPI
NetLibGetNicHandle (
  IN EFI_HANDLE             Controller,
  IN EFI_GUID               *ProtocolGuid
  );

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
  );

/**
  Convert one Null-terminated ASCII string (decimal dotted) to EFI_IPv4_ADDRESS.

  @param[in]      String         The pointer to the Ascii string.
  @param[out]     Ip4Address     The pointer to the converted IPv4 address.

  @retval EFI_SUCCESS            Converted to an IPv4 address successfully.
  @retval EFI_INVALID_PARAMETER  The string is malformatted, or Ip4Address is NULL.

**/
EFI_STATUS
EFIAPI
NetLibAsciiStrToIp4 (
  IN CONST CHAR8                 *String,
  OUT      EFI_IPv4_ADDRESS      *Ip4Address
  );

/**
  Convert one Null-terminated ASCII string to EFI_IPv6_ADDRESS. The format of the
  string is defined in RFC 4291 - Text Representation of Addresses.

  @param[in]      String         The pointer to the Ascii string.
  @param[out]     Ip6Address     The pointer to the converted IPv6 address.

  @retval EFI_SUCCESS            Converted to an IPv6 address successfully.
  @retval EFI_INVALID_PARAMETER  The string is malformatted, or Ip6Address is NULL.

**/
EFI_STATUS
EFIAPI
NetLibAsciiStrToIp6 (
  IN CONST CHAR8                 *String,
  OUT      EFI_IPv6_ADDRESS      *Ip6Address
  );

/**
  Convert one Null-terminated Unicode string (decimal dotted) to EFI_IPv4_ADDRESS.

  @param[in]      String         The pointer to the Ascii string.
  @param[out]     Ip4Address     The pointer to the converted IPv4 address.

  @retval EFI_SUCCESS            Converted to an IPv4 address successfully.
  @retval EFI_INVALID_PARAMETER  The string is mal-formatted or Ip4Address is NULL.

**/
EFI_STATUS
EFIAPI
NetLibStrToIp4 (
  IN CONST CHAR16                *String,
  OUT      EFI_IPv4_ADDRESS      *Ip4Address
  );

/**
  Convert one Null-terminated Unicode string to EFI_IPv6_ADDRESS.  The format of
  the string is defined in RFC 4291 - Text Representation of Addresses.

  @param[in]      String         The pointer to the Ascii string.
  @param[out]     Ip6Address     The pointer to the converted IPv6 address.

  @retval EFI_SUCCESS            Converted to an IPv6 address successfully.
  @retval EFI_INVALID_PARAMETER  The string is malformatted or Ip6Address is NULL.

**/
EFI_STATUS
EFIAPI
NetLibStrToIp6 (
  IN CONST CHAR16                *String,
  OUT      EFI_IPv6_ADDRESS      *Ip6Address
  );

/**
  Convert one Null-terminated Unicode string to EFI_IPv6_ADDRESS and prefix length.
  The format of the string is defined in RFC 4291 - Text Representation of Addresses
  Prefixes: ipv6-address/prefix-length.

  @param[in]      String         The pointer to the Ascii string.
  @param[out]     Ip6Address     The pointer to the converted IPv6 address.
  @param[out]     PrefixLength   The pointer to the converted prefix length.

  @retval EFI_SUCCESS            Converted to an  IPv6 address successfully.
  @retval EFI_INVALID_PARAMETER  The string is malformatted, or Ip6Address is NULL.

**/
EFI_STATUS
EFIAPI
NetLibStrToIp6andPrefix (
  IN CONST CHAR16                *String,
  OUT      EFI_IPv6_ADDRESS      *Ip6Address,
  OUT      UINT8                 *PrefixLength
  );

/**

  Convert one EFI_IPv6_ADDRESS to Null-terminated Unicode string.
  The text representation of address is defined in RFC 4291.

  @param[in]       Ip6Address     The pointer to the IPv6 address.
  @param[out]      String         The buffer to return the converted string.
  @param[in]       StringSize     The length in bytes of the input String.

  @retval EFI_SUCCESS             Convert to string successfully.
  @retval EFI_INVALID_PARAMETER   The input parameter is invalid.
  @retval EFI_BUFFER_TOO_SMALL    The BufferSize is too small for the result. BufferSize has been
                                  updated with the size needed to complete the request.
**/
EFI_STATUS
EFIAPI
NetLibIp6ToStr (
  IN         EFI_IPv6_ADDRESS      *Ip6Address,
  OUT        CHAR16                *String,
  IN         UINTN                 StringSize
  );

//
// Various signatures
//
#define  NET_BUF_SIGNATURE    SIGNATURE_32 ('n', 'b', 'u', 'f')
#define  NET_VECTOR_SIGNATURE SIGNATURE_32 ('n', 'v', 'e', 'c')
#define  NET_QUE_SIGNATURE    SIGNATURE_32 ('n', 'b', 'q', 'u')


#define  NET_PROTO_DATA       64   // Opaque buffer for protocols
#define  NET_BUF_HEAD         1    // Trim or allocate space from head
#define  NET_BUF_TAIL         0    // Trim or allocate space from tail
#define  NET_VECTOR_OWN_FIRST 0x01  // We allocated the 1st block in the vector

#define NET_CHECK_SIGNATURE(PData, SIGNATURE) \
  ASSERT (((PData) != NULL) && ((PData)->Signature == (SIGNATURE)))

//
// Single memory block in the vector.
//
typedef struct {
  UINT32              Len;        // The block's length
  UINT8               *Bulk;      // The block's Data
} NET_BLOCK;

typedef VOID (EFIAPI *NET_VECTOR_EXT_FREE) (VOID *Arg);

//
//NET_VECTOR contains several blocks to hold all packet's
//fragments and other house-keeping stuff for sharing. It
//doesn't specify the where actual packet fragment begins.
//
typedef struct {
  UINT32              Signature;
  INTN                RefCnt;  // Reference count to share NET_VECTOR.
  NET_VECTOR_EXT_FREE Free;    // external function to free NET_VECTOR
  VOID                *Arg;    // opaque argument to Free
  UINT32              Flag;    // Flags, NET_VECTOR_OWN_FIRST
  UINT32              Len;     // Total length of the associated BLOCKs

  UINT32              BlockNum;
  NET_BLOCK           Block[1];
} NET_VECTOR;

//
//NET_BLOCK_OP operates on the NET_BLOCK. It specifies
//where the actual fragment begins and ends
//
typedef struct {
  UINT8               *BlockHead;   // Block's head, or the smallest valid Head
  UINT8               *BlockTail;   // Block's tail. BlockTail-BlockHead=block length
  UINT8               *Head;        // 1st byte of the data in the block
  UINT8               *Tail;        // Tail of the data in the block, Tail-Head=Size
  UINT32              Size;         // The size of the data
} NET_BLOCK_OP;

typedef union {
  IP4_HEAD          *Ip4;
  EFI_IP6_HEADER    *Ip6;
} NET_IP_HEAD;

//
//NET_BUF is the buffer manage structure used by the
//network stack. Every network packet may be fragmented. The Vector points to
//memory blocks used by each fragment, and BlockOp
//specifies where each fragment begins and ends.
//
//It also contains an opaque area for the protocol to store
//per-packet information. Protocol must be careful not
//to overwrite the members after that.
//
typedef struct {
  UINT32         Signature;
  INTN           RefCnt;
  LIST_ENTRY     List;                       // The List this NET_BUF is on

  NET_IP_HEAD    Ip;                         // Network layer header, for fast access
  TCP_HEAD       *Tcp;                       // Transport layer header, for fast access
  EFI_UDP_HEADER *Udp;                       // User Datagram Protocol header
  UINT8          ProtoData [NET_PROTO_DATA]; //Protocol specific data

  NET_VECTOR     *Vector;                    // The vector containing the packet

  UINT32         BlockOpNum;                 // Total number of BlockOp in the buffer
  UINT32         TotalSize;                  // Total size of the actual packet
  NET_BLOCK_OP   BlockOp[1];                 // Specify the position of actual packet
} NET_BUF;

//
//A queue of NET_BUFs. It is a thin extension of
//NET_BUF functions.
//
typedef struct {
  UINT32              Signature;
  INTN                RefCnt;
  LIST_ENTRY          List;       // The List this buffer queue is on

  LIST_ENTRY          BufList;    // list of queued buffers
  UINT32              BufSize;    // total length of DATA in the buffers
  UINT32              BufNum;     // total number of buffers on the chain
} NET_BUF_QUEUE;

//
// Pseudo header for TCP and UDP checksum
//
#pragma pack(1)
typedef struct {
  IP4_ADDR            SrcIp;
  IP4_ADDR            DstIp;
  UINT8               Reserved;
  UINT8               Protocol;
  UINT16              Len;
} NET_PSEUDO_HDR;

typedef struct {
  EFI_IPv6_ADDRESS    SrcIp;
  EFI_IPv6_ADDRESS    DstIp;
  UINT32              Len;
  UINT32              Reserved:24;
  UINT32              NextHeader:8;
} NET_IP6_PSEUDO_HDR;
#pragma pack()

//
// The fragment entry table used in network interfaces. This is
// the same as NET_BLOCK now. Use two different to distinguish
// the two in case that NET_BLOCK be enhanced later.
//
typedef struct {
  UINT32              Len;
  UINT8               *Bulk;
} NET_FRAGMENT;

#define NET_GET_REF(PData)      ((PData)->RefCnt++)
#define NET_PUT_REF(PData)      ((PData)->RefCnt--)
#define NETBUF_FROM_PROTODATA(Info) BASE_CR((Info), NET_BUF, ProtoData)

#define NET_BUF_SHARED(Buf) \
  (((Buf)->RefCnt > 1) || ((Buf)->Vector->RefCnt > 1))

#define NET_VECTOR_SIZE(BlockNum) \
  (sizeof (NET_VECTOR) + ((BlockNum) - 1) * sizeof (NET_BLOCK))

#define NET_BUF_SIZE(BlockOpNum)  \
  (sizeof (NET_BUF) + ((BlockOpNum) - 1) * sizeof (NET_BLOCK_OP))

#define NET_HEADSPACE(BlockOp)  \
  ((UINTN)((BlockOp)->Head) - (UINTN)((BlockOp)->BlockHead))

#define NET_TAILSPACE(BlockOp)  \
  ((UINTN)((BlockOp)->BlockTail) - (UINTN)((BlockOp)->Tail))

/**
  Allocate a single block NET_BUF. Upon allocation, all the
  free space is in the tail room.

  @param[in]  Len              The length of the block.

  @return                      The pointer to the allocated NET_BUF, or NULL if the
                               allocation failed due to resource limitations.

**/
NET_BUF  *
EFIAPI
NetbufAlloc (
  IN UINT32                 Len
  );

/**
  Free the net buffer and its associated NET_VECTOR.

  Decrease the reference count of the net buffer by one. Free the associated net
  vector and itself if the reference count of the net buffer is decreased to 0.
  The net vector free operation decreases the reference count of the net
  vector by one, and performs the resource free operation when the reference count
  of the net vector is 0.

  @param[in]  Nbuf                  The pointer to the NET_BUF to be freed.

**/
VOID
EFIAPI
NetbufFree (
  IN NET_BUF                *Nbuf
  );

/**
  Get the index of NET_BLOCK_OP that contains the byte at Offset in the net
  buffer.

  For example, this function can be used to retrieve the IP header in the packet. It
  also can be used to get the fragment that contains the byte used
  mainly by the library implementation itself.

  @param[in]   Nbuf      The pointer to the net buffer.
  @param[in]   Offset    The offset of the byte.
  @param[out]  Index     Index of the NET_BLOCK_OP that contains the byte at
                         Offset.

  @return       The pointer to the Offset'th byte of data in the net buffer, or NULL
                if there is no such data in the net buffer.

**/
UINT8  *
EFIAPI
NetbufGetByte (
  IN  NET_BUF               *Nbuf,
  IN  UINT32                Offset,
  OUT UINT32                *Index  OPTIONAL
  );

/**
  Create a copy of the net buffer that shares the associated net vector.

  The reference count of the newly created net buffer is set to 1. The reference
  count of the associated net vector is increased by one.

  @param[in]  Nbuf              The pointer to the net buffer to be cloned.

  @return                       The pointer to the cloned net buffer, or NULL if the
                                allocation failed due to resource limitations.

**/
NET_BUF *
EFIAPI
NetbufClone (
  IN NET_BUF                *Nbuf
  );

/**
  Create a duplicated copy of the net buffer with data copied and HeadSpace
  bytes of head space reserved.

  The duplicated net buffer will allocate its own memory to hold the data of the
  source net buffer.

  @param[in]       Nbuf         The pointer to the net buffer to be duplicated from.
  @param[in, out]  Duplicate    The pointer to the net buffer to duplicate to. If
                                NULL, a new net buffer is allocated.
  @param[in]      HeadSpace     The length of the head space to reserve.

  @return                       The pointer to the duplicated net buffer, or NULL if
                                the allocation failed due to resource limitations.

**/
NET_BUF  *
EFIAPI
NetbufDuplicate (
  IN NET_BUF                *Nbuf,
  IN OUT NET_BUF            *Duplicate        OPTIONAL,
  IN UINT32                 HeadSpace
  );

/**
  Create a NET_BUF structure which contains Len byte data of Nbuf starting from
  Offset.

  A new NET_BUF structure will be created but the associated data in NET_VECTOR
  is shared. This function exists to perform IP packet fragmentation.

  @param[in]  Nbuf         The pointer to the net buffer to be extracted.
  @param[in]  Offset       Starting point of the data to be included in the new
                           net buffer.
  @param[in]  Len          The bytes of data to be included in the new net buffer.
  @param[in]  HeadSpace    The bytes of the head space to reserve for the protocol header.

  @return                  The pointer to the cloned net buffer, or NULL if the
                           allocation failed due to resource limitations.

**/
NET_BUF  *
EFIAPI
NetbufGetFragment (
  IN NET_BUF                *Nbuf,
  IN UINT32                 Offset,
  IN UINT32                 Len,
  IN UINT32                 HeadSpace
  );

/**
  Reserve some space in the header room of the net buffer.

  Upon allocation, all the space is in the tail room of the buffer. Call this
  function to move space to the header room. This function is quite limited
  in that it can only reserve space from the first block of an empty NET_BUF not
  built from the external. However, it should be enough for the network stack.

  @param[in, out]  Nbuf     The pointer to the net buffer.
  @param[in]       Len      The length of buffer to be reserved from the header.

**/
VOID
EFIAPI
NetbufReserve (
  IN OUT NET_BUF            *Nbuf,
  IN UINT32                 Len
  );

/**
  Allocate Len bytes of space from the header or tail of the buffer.

  @param[in, out]  Nbuf       The pointer to the net buffer.
  @param[in]       Len        The length of the buffer to be allocated.
  @param[in]       FromHead   The flag to indicate whether to reserve the data
                              from head (TRUE) or tail (FALSE).

  @return                     The pointer to the first byte of the allocated buffer,
                              or NULL, if there is no sufficient space.

**/
UINT8*
EFIAPI
NetbufAllocSpace (
  IN OUT NET_BUF            *Nbuf,
  IN UINT32                 Len,
  IN BOOLEAN                FromHead
  );

/**
  Trim Len bytes from the header or the tail of the net buffer.

  @param[in, out]  Nbuf         The pointer to the net buffer.
  @param[in]       Len          The length of the data to be trimmed.
  @param[in]      FromHead      The flag to indicate whether trim data is from the
                                head (TRUE) or the tail (FALSE).

  @return    The length of the actual trimmed data, which may be less
             than Len if the TotalSize of Nbuf is less than Len.

**/
UINT32
EFIAPI
NetbufTrim (
  IN OUT NET_BUF            *Nbuf,
  IN UINT32                 Len,
  IN BOOLEAN                FromHead
  );

/**
  Copy Len bytes of data from the specific offset of the net buffer to the
  destination memory.

  The Len bytes of data may cross several fragments of the net buffer.

  @param[in]   Nbuf         The pointer to the net buffer.
  @param[in]   Offset       The sequence number of the first byte to copy.
  @param[in]   Len          The length of the data to copy.
  @param[in]   Dest         The destination of the data to copy to.

  @return           The length of the actual copied data, or 0 if the offset
                    specified exceeds the total size of net buffer.

**/
UINT32
EFIAPI
NetbufCopy (
  IN NET_BUF                *Nbuf,
  IN UINT32                 Offset,
  IN UINT32                 Len,
  IN UINT8                  *Dest
  );

/**
  Build a NET_BUF from external blocks.

  A new NET_BUF structure will be created from external blocks. An additional block
  of memory will be allocated to hold reserved HeadSpace bytes of header room
  and existing HeadLen bytes of header, but the external blocks are shared by the
  net buffer to avoid data copying.

  @param[in]  ExtFragment           The pointer to the data block.
  @param[in]  ExtNum                The number of the data blocks.
  @param[in]  HeadSpace             The head space to be reserved.
  @param[in]  HeadLen               The length of the protocol header. The function
                                    pulls this amount of data into a linear block.
  @param[in]  ExtFree               The pointer to the caller-provided free function.
  @param[in]  Arg                   The argument passed to ExtFree when ExtFree is
                                    called.

  @return                  The pointer to the net buffer built from the data blocks,
                           or NULL if the allocation failed due to resource
                           limit.

**/
NET_BUF  *
EFIAPI
NetbufFromExt (
  IN NET_FRAGMENT           *ExtFragment,
  IN UINT32                 ExtNum,
  IN UINT32                 HeadSpace,
  IN UINT32                 HeadLen,
  IN NET_VECTOR_EXT_FREE    ExtFree,
  IN VOID                   *Arg          OPTIONAL
  );

/**
  Build a fragment table to contain the fragments in the net buffer. This is the
  opposite operation of the NetbufFromExt.

  @param[in]       Nbuf                  Points to the net buffer.
  @param[in, out]  ExtFragment           The pointer to the data block.
  @param[in, out]  ExtNum                The number of the data blocks.

  @retval EFI_BUFFER_TOO_SMALL  The number of non-empty blocks is bigger than
                                ExtNum.
  @retval EFI_SUCCESS           The fragment table was built successfully.

**/
EFI_STATUS
EFIAPI
NetbufBuildExt (
  IN NET_BUF                *Nbuf,
  IN OUT NET_FRAGMENT       *ExtFragment,
  IN OUT UINT32             *ExtNum
  );

/**
  Build a net buffer from a list of net buffers.

  All the fragments will be collected from the list of NEW_BUF, and then a new
  net buffer will be created through NetbufFromExt.

  @param[in]   BufList    A List of the net buffer.
  @param[in]   HeadSpace  The head space to be reserved.
  @param[in]   HeaderLen  The length of the protocol header. The function
                          pulls this amount of data into a linear block.
  @param[in]   ExtFree    The pointer to the caller provided free function.
  @param[in]   Arg        The argument passed to ExtFree when ExtFree is called.

  @return                 The pointer to the net buffer built from the list of net
                          buffers.

**/
NET_BUF  *
EFIAPI
NetbufFromBufList (
  IN LIST_ENTRY             *BufList,
  IN UINT32                 HeadSpace,
  IN UINT32                 HeaderLen,
  IN NET_VECTOR_EXT_FREE    ExtFree,
  IN VOID                   *Arg              OPTIONAL
  );

/**
  Free a list of net buffers.

  @param[in, out]  Head              The pointer to the head of linked net buffers.

**/
VOID
EFIAPI
NetbufFreeList (
  IN OUT LIST_ENTRY         *Head
  );

/**
  Initiate the net buffer queue.

  @param[in, out]  NbufQue   The pointer to the net buffer queue to be initialized.

**/
VOID
EFIAPI
NetbufQueInit (
  IN OUT NET_BUF_QUEUE          *NbufQue
  );

/**
  Allocate and initialize a net buffer queue.

  @return         The pointer to the allocated net buffer queue, or NULL if the
                  allocation failed due to resource limit.

**/
NET_BUF_QUEUE  *
EFIAPI
NetbufQueAlloc (
  VOID
  );

/**
  Free a net buffer queue.

  Decrease the reference count of the net buffer queue by one. The real resource
  free operation isn't performed until the reference count of the net buffer
  queue is decreased to 0.

  @param[in]  NbufQue               The pointer to the net buffer queue to be freed.

**/
VOID
EFIAPI
NetbufQueFree (
  IN NET_BUF_QUEUE          *NbufQue
  );

/**
  Remove a net buffer from the head in the specific queue and return it.

  @param[in, out]  NbufQue               The pointer to the net buffer queue.

  @return           The pointer to the net buffer removed from the specific queue,
                    or NULL if there is no net buffer in the specific queue.

**/
NET_BUF  *
EFIAPI
NetbufQueRemove (
  IN OUT NET_BUF_QUEUE          *NbufQue
  );

/**
  Append a net buffer to the net buffer queue.

  @param[in, out]  NbufQue            The pointer to the net buffer queue.
  @param[in, out]  Nbuf               The pointer to the net buffer to be appended.

**/
VOID
EFIAPI
NetbufQueAppend (
  IN OUT NET_BUF_QUEUE          *NbufQue,
  IN OUT NET_BUF                *Nbuf
  );

/**
  Copy Len bytes of data from the net buffer queue at the specific offset to the
  destination memory.

  The copying operation is the same as NetbufCopy, but applies to the net buffer
  queue instead of the net buffer.

  @param[in]   NbufQue         The pointer to the net buffer queue.
  @param[in]   Offset          The sequence number of the first byte to copy.
  @param[in]   Len             The length of the data to copy.
  @param[out]  Dest            The destination of the data to copy to.

  @return       The length of the actual copied data, or 0 if the offset
                specified exceeds the total size of net buffer queue.

**/
UINT32
EFIAPI
NetbufQueCopy (
  IN NET_BUF_QUEUE          *NbufQue,
  IN UINT32                 Offset,
  IN UINT32                 Len,
  OUT UINT8                 *Dest
  );

/**
  Trim Len bytes of data from the buffer queue and free any net buffer
  that is completely trimmed.

  The trimming operation is the same as NetbufTrim but applies to the net buffer
  queue instead of the net buffer.

  @param[in, out]  NbufQue               The pointer to the net buffer queue.
  @param[in]       Len                   The length of the data to trim.

  @return   The actual length of the data trimmed.

**/
UINT32
EFIAPI
NetbufQueTrim (
  IN OUT NET_BUF_QUEUE      *NbufQue,
  IN UINT32                 Len
  );


/**
  Flush the net buffer queue.

  @param[in, out]  NbufQue               The pointer to the queue to be flushed.

**/
VOID
EFIAPI
NetbufQueFlush (
  IN OUT NET_BUF_QUEUE          *NbufQue
  );

/**
  Compute the checksum for a bulk of data.

  @param[in]   Bulk                  The pointer to the data.
  @param[in]   Len                   The length of the data, in bytes.

  @return    The computed checksum.

**/
UINT16
EFIAPI
NetblockChecksum (
  IN UINT8                  *Bulk,
  IN UINT32                 Len
  );

/**
  Add two checksums.

  @param[in]   Checksum1             The first checksum to be added.
  @param[in]   Checksum2             The second checksum to be added.

  @return         The new checksum.

**/
UINT16
EFIAPI
NetAddChecksum (
  IN UINT16                 Checksum1,
  IN UINT16                 Checksum2
  );

/**
  Compute the checksum for a NET_BUF.

  @param[in]   Nbuf                  The pointer to the net buffer.

  @return    The computed checksum.

**/
UINT16
EFIAPI
NetbufChecksum (
  IN NET_BUF                *Nbuf
  );

/**
  Compute the checksum for TCP/UDP pseudo header.

  Src and Dst are in network byte order, and Len is in host byte order.

  @param[in]   Src                   The source address of the packet.
  @param[in]   Dst                   The destination address of the packet.
  @param[in]   Proto                 The protocol type of the packet.
  @param[in]   Len                   The length of the packet.

  @return   The computed checksum.

**/
UINT16
EFIAPI
NetPseudoHeadChecksum (
  IN IP4_ADDR               Src,
  IN IP4_ADDR               Dst,
  IN UINT8                  Proto,
  IN UINT16                 Len
  );

/**
  Compute the checksum for the TCP6/UDP6 pseudo header.

  Src and Dst are in network byte order, and Len is in host byte order.

  @param[in]   Src                   The source address of the packet.
  @param[in]   Dst                   The destination address of the packet.
  @param[in]   NextHeader            The protocol type of the packet.
  @param[in]   Len                   The length of the packet.

  @return   The computed checksum.

**/
UINT16
EFIAPI
NetIp6PseudoHeadChecksum (
  IN EFI_IPv6_ADDRESS       *Src,
  IN EFI_IPv6_ADDRESS       *Dst,
  IN UINT8                  NextHeader,
  IN UINT32                 Len
  );

/**
  The function frees the net buffer which allocated by the IP protocol. It releases
  only the net buffer and doesn't call the external free function.

  This function should be called after finishing the process of mIpSec->ProcessExt()
  for outbound traffic. The (EFI_IPSEC2_PROTOCOL)->ProcessExt() allocates a new
  buffer for the ESP, so there needs a function to free the old net buffer.

  @param[in]  Nbuf       The network buffer to be freed.

**/
VOID
NetIpSecNetbufFree (
  NET_BUF   *Nbuf
  );

/**
  This function obtains the system guid from the smbios table.

  If SystemGuid is NULL, then ASSERT().

  @param[out]  SystemGuid     The pointer of the returned system guid.

  @retval EFI_SUCCESS         Successfully obtained the system guid.
  @retval EFI_NOT_FOUND       Did not find the SMBIOS table.

**/
EFI_STATUS
EFIAPI
NetLibGetSystemGuid (
  OUT EFI_GUID              *SystemGuid
  );

/**
  Create Dns QName according the queried domain name.

  If DomainName is NULL, then ASSERT().

  QName is a domain name represented as a sequence of labels,
  where each label consists of a length octet followed by that
  number of octets. The QName terminates with the zero
  length octet for the null label of the root. Caller should
  take responsibility to free the buffer in returned pointer.

  @param  DomainName    The pointer to the queried domain name string.

  @retval NULL          Failed to fill QName.
  @return               QName filled successfully.

**/
CHAR8 *
EFIAPI
NetLibCreateDnsQName (
  IN  CHAR16              *DomainName
  );

#endif
