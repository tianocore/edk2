/** @file
  Ihis library is only intended to be used by UEFI network stack modules.
  It provides basic function for UEFI network stack.

Copyright (c) 2005 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _NET_LIB_H_
#define _NET_LIB_H_

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverConfiguration.h>
#include <Protocol/DriverDiagnostics.h>
#include <Protocol/Dpc.h>

typedef UINT32          IP4_ADDR;
typedef UINT32          TCP_SEQNO;
typedef UINT16          TCP_PORTNO;

typedef enum {
  NET_ETHER_ADDR_LEN    = 6,
  NET_IFTYPE_ETHERNET   = 0x01,

  EFI_IP_PROTO_UDP      = 0x11,
  EFI_IP_PROTO_TCP      = 0x06,
  EFI_IP_PROTO_ICMP     = 0x01,

  //
  // The address classification
  //
  IP4_ADDR_CLASSA       = 1,
  IP4_ADDR_CLASSB,
  IP4_ADDR_CLASSC,
  IP4_ADDR_CLASSD,
  IP4_ADDR_CLASSE,

  IP4_MASK_NUM          = 33
} IP4_CLASS_TYPE;

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
// ICMP head definition. ICMP message is categoried as either an error
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


//
// UDP header definition
//
typedef struct {
  UINT16                SrcPort;
  UINT16                DstPort;
  UINT16                Length;
  UINT16                Checksum;
} EFI_UDP4_HEADER;


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

#define NTOHL(x) (UINT32)((((UINT32) (x) & 0xff)     << 24) | \
                          (((UINT32) (x) & 0xff00)   << 8)  | \
                          (((UINT32) (x) & 0xff0000) >> 8)  | \
                          (((UINT32) (x) & 0xff000000) >> 24))

#define HTONL(x)  NTOHL(x)

#define NTOHS(x)  (UINT16)((((UINT16) (x) & 0xff) << 8) | \
                           (((UINT16) (x) & 0xff00) >> 8))

#define HTONS(x)  NTOHS(x)

//
// Test the IP's attribute, All the IPs are in host byte order.
//
#define IP4_IS_MULTICAST(Ip)              (((Ip) & 0xF0000000) == 0xE0000000)
#define IP4_IS_LOCAL_BROADCAST(Ip)        ((Ip) == 0xFFFFFFFF)
#define IP4_NET_EQUAL(Ip1, Ip2, NetMask)  (((Ip1) & (NetMask)) == ((Ip2) & (NetMask)))
#define IP4_IS_VALID_NETMASK(Ip)          (NetGetMaskLength (Ip) != IP4_MASK_NUM)

//
// Convert the EFI_IP4_ADDRESS to plain UINT32 IP4 address.
//
#define EFI_IP4(EfiIpAddr)       (*(IP4_ADDR *) ((EfiIpAddr).Addr))
#define EFI_NTOHL(EfiIp)         (NTOHL (EFI_IP4 ((EfiIp))))
#define EFI_IP4_EQUAL(Ip1, Ip2)  (CompareMem ((Ip1), (Ip2), sizeof (EFI_IPv4_ADDRESS)) == 0)

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
  );

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
  );

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
Ip4IsUnicast (
  IN IP4_ADDR               Ip,
  IN IP4_ADDR               NetMask
  );

extern IP4_ADDR gIp4AllMasks [IP4_MASK_NUM];


extern EFI_IPv4_ADDRESS  mZeroIp4Addr;

#define NET_IS_DIGIT(Ch)            (('0' <= (Ch)) && ((Ch) <= '9'))
#define NET_ROUNDUP(size, unit)     (((size) + (unit) - 1) & (~((unit) - 1)))
#define NET_IS_LOWER_CASE_CHAR(Ch)  (('a' <= (Ch)) && ((Ch) <= 'z'))
#define NET_IS_UPPER_CASE_CHAR(Ch)  (('A' <= (Ch)) && ((Ch) <= 'Z'))

#define TICKS_PER_MS            10000U
#define TICKS_PER_SECOND        10000000U

#define NET_RANDOM(Seed)        ((UINT32) ((UINT32) (Seed) * 1103515245UL + 12345) % 4294967295UL)

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
  );

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
  );

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
  );


#define NET_LIST_USER_STRUCT(Entry, Type, Field)        \
          BASE_CR(Entry, Type, Field)

#define NET_LIST_USER_STRUCT_S(Entry, Type, Field, Sig)  \
          CR(Entry, Type, Field, Sig)

//
// Iterate through the doule linked list. It is NOT delete safe
//
#define NET_LIST_FOR_EACH(Entry, ListHead) \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

//
// Iterate through the doule linked list. This is delete-safe.
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
// Make sure the list isn't empty before get the frist/last record.
//
#define NET_LIST_HEAD(ListHead, Type, Field)  \
          NET_LIST_USER_STRUCT((ListHead)->ForwardLink, Type, Field)

#define NET_LIST_TAIL(ListHead, Type, Field)  \
          NET_LIST_USER_STRUCT((ListHead)->BackLink, Type, Field)


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
  );

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
  );

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
  );

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
(*NET_MAP_CALLBACK) (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Arg
  );

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
  IN VOID                   *Arg
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
  );

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
  );

/**
  Convert the mac address of the simple network protocol installed on
  SnpHandle to a unicode string. Callers are responsible for freeing the
  string storage.

  Get the mac address of the Simple Network protocol from the SnpHandle. Then convert
  the mac address into a unicode string. It takes 2 unicode characters to represent 
  a 1 byte binary buffer. Plus one unicode character for the null-terminator.


  @param[in]   SnpHandle             The handle where the simple network protocol is
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
  IN  EFI_HANDLE            SnpHandle,
  IN  EFI_HANDLE            ImageHandle,
  OUT CHAR16                **MacString
  );

/**
  Create an IPv4 device path node.
  
  The header type of IPv4 device path node is MESSAGING_DEVICE_PATH.
  The header subtype of IPv4 device path node is MSG_IPv4_DP.
  The length of the IPv4 device path node in bytes is 19.
  Get other info from parameters to make up the whole IPv4 device path node.

  @param[in, out]  Node                  Pointer to the IPv4 device path node.
  @param[in]       Controller            The handle where the NIC IP4 config protocol resides.
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
  );

/**
  Add a Deferred Procedure Call to the end of the DPC queue.

  @param[in]  DpcTpl           The EFI_TPL that the DPC should be invoked.
  @param[in]  DpcProcedure     Pointer to the DPC's function.
  @param[in]  DpcContext       Pointer to the DPC's context.  Passed to DpcProcedure
                               when DpcProcedure is invoked.

  @retval  EFI_SUCCESS              The DPC was queued.
  @retval  EFI_INVALID_PARAMETER    DpcTpl is not a valid EFI_TPL, or DpcProcedure
                                    is NULL.
  @retval  EFI_OUT_OF_RESOURCES     There are not enough resources available to
                                    add the DPC to the queue.

**/
EFI_STATUS
EFIAPI
NetLibQueueDpc (
  IN EFI_TPL            DpcTpl,
  IN EFI_DPC_PROCEDURE  DpcProcedure,
  IN VOID               *DpcContext    OPTIONAL
  );

/**
  Dispatch the queue of DPCs. ALL DPCs that have been queued with a DpcTpl
  value greater than or equal to the current TPL are invoked in the order that
  they were queued.  DPCs with higher DpcTpl values are invoked before DPCs with
  lower DpcTpl values.

  @retval  EFI_SUCCESS              One or more DPCs were invoked.
  @retval  EFI_NOT_FOUND            No DPCs were invoked.

**/
EFI_STATUS
EFIAPI
NetLibDispatchDpc (
  VOID
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

typedef enum {
  //
  //Various signatures
  //
  NET_BUF_SIGNATURE    = SIGNATURE_32 ('n', 'b', 'u', 'f'),
  NET_VECTOR_SIGNATURE = SIGNATURE_32 ('n', 'v', 'e', 'c'),
  NET_QUE_SIGNATURE    = SIGNATURE_32 ('n', 'b', 'q', 'u'),


  NET_PROTO_DATA       = 64,   // Opaque buffer for protocols
  NET_BUF_HEAD         = 1,    // Trim or allocate space from head
  NET_BUF_TAIL         = 0,    // Trim or allocate space from tail
  NET_VECTOR_OWN_FIRST = 0x01  // We allocated the 1st block in the vector
} NET_SIGNATURE_TYPE;

#define NET_CHECK_SIGNATURE(PData, SIGNATURE) \
  ASSERT (((PData) != NULL) && ((PData)->Signature == (SIGNATURE)))

#define NET_SWAP_SHORT(Value) \
  ((((Value) & 0xff) << 8) | (((Value) >> 8) & 0xff))

//
// Single memory block in the vector.
//
typedef struct {
  UINT32              Len;        // The block's length
  UINT8               *Bulk;      // The block's Data
} NET_BLOCK;

typedef VOID (*NET_VECTOR_EXT_FREE) (VOID *Arg);

//
//NET_VECTOR contains several blocks to hold all packet's
//fragments and other house-keeping stuff for sharing. It
//doesn't specify the where actual packet fragment begins.
//
typedef struct {
  UINT32              Signature;
  INTN                RefCnt;  // Reference count to share NET_VECTOR.
  NET_VECTOR_EXT_FREE Free;    // external function to free NET_VECTOR
  VOID                *Arg;    // opeque argument to Free
  UINT32              Flag;    // Flags, NET_VECTOR_OWN_FIRST
  UINT32              Len;     // Total length of the assocated BLOCKs

  UINT32              BlockNum;
  NET_BLOCK           Block[1];
} NET_VECTOR;

//
//NET_BLOCK_OP operate on the NET_BLOCK, It specifies
//where the actual fragment begins and where it ends
//
typedef struct {
  UINT8               *BlockHead;   // Block's head, or the smallest valid Head
  UINT8               *BlockTail;   // Block's tail. BlockTail-BlockHead=block length
  UINT8               *Head;        // 1st byte of the data in the block
  UINT8               *Tail;        // Tail of the data in the block, Tail-Head=Size
  UINT32              Size;         // The size of the data
} NET_BLOCK_OP;


//
//NET_BUF is the buffer manage structure used by the
//network stack. Every network packet may be fragmented,
//and contains multiple fragments. The Vector points to
//memory blocks used by the each fragment, and BlockOp
//specifies where each fragment begins and ends.
//
//It also contains a opaque area for protocol to store
//per-packet informations. Protocol must be caution not
//to overwrite the members after that.
//
typedef struct {
  UINT32              Signature;
  INTN                RefCnt;
  LIST_ENTRY          List;       // The List this NET_BUF is on

  IP4_HEAD            *Ip;        // Network layer header, for fast access
  TCP_HEAD            *Tcp;       // Transport layer header, for fast access
  UINT8               ProtoData [NET_PROTO_DATA]; //Protocol specific data

  NET_VECTOR          *Vector;    // The vector containing the packet

  UINT32              BlockOpNum; // Total number of BlockOp in the buffer
  UINT32              TotalSize;  // Total size of the actual packet
  NET_BLOCK_OP        BlockOp[1]; // Specify the position of actual packet
} NET_BUF;


//
//A queue of NET_BUFs, It is just a thin extension of
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
  (UINTN)((BlockOp)->Head - (BlockOp)->BlockHead)

#define NET_TAILSPACE(BlockOp)  \
  (UINTN)((BlockOp)->BlockTail - (BlockOp)->Tail)

/**
  Allocate a single block NET_BUF. Upon allocation, all the
  free space is in the tail room.

  @param[in]  Len              The length of the block.

  @return                      Pointer to the allocated NET_BUF, or NULL if the 
                               allocation failed due to resource limit.

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
  The net vector free operation just decrease the reference count of the net 
  vector by one and do the real resource free operation when the reference count
  of the net vector is 0. 
 
  @param[in]  Nbuf                  Pointer to the NET_BUF to be freed.

**/
VOID
EFIAPI
NetbufFree (
  IN NET_BUF                *Nbuf
  );

/**
  Get the index of NET_BLOCK_OP that contains the byte at Offset in the net 
  buffer. 
  
  This can be used to, for example, retrieve the IP header in the packet. It 
  also can be used to get the fragment that contains the byte which is used 
  mainly by the library implementation itself. 

  @param[in]   Nbuf      Pointer to the net buffer.
  @param[in]   Offset    The offset of the byte.
  @param[out]  Index     Index of the NET_BLOCK_OP that contains the byte at 
                         Offset.

  @return       Pointer to the Offset'th byte of data in the net buffer, or NULL
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

  @param[in]  Nbuf              Pointer to the net buffer to be cloned.

  @return                       Pointer to the cloned net buffer, or NULL if the
                                allocation failed due to resource limit.

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
   
  @param[in]       Nbuf         Pointer to the net buffer to be duplicated from.
  @param[in, out]  Duplicate    Pointer to the net buffer to duplicate to, if 
                                NULL a new net buffer is allocated.
  @param[in]      HeadSpace     Length of the head space to reserve.

  @return                       Pointer to the duplicated net buffer, or NULL if
                                the allocation failed due to resource limit.

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
  is shared. This function exists to do IP packet fragmentation. 

  @param[in]  Nbuf         Pointer to the net buffer to be extracted.
  @param[in]  Offset       Starting point of the data to be included in the new 
                           net buffer.
  @param[in]  Len          Bytes of data to be included in the new net buffer. 
  @param[in]  HeadSpace    Bytes of head space to reserve for protocol header. 

  @return                  Pointer to the cloned net buffer, or NULL if the 
                           allocation failed due to resource limit.

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

  Upon allocation, all the space are in the tail room of the buffer. Call this 
  function to move some space to the header room. This function is quite limited
  in that it can only reserve space from the first block of an empty NET_BUF not 
  built from the external. But it should be enough for the network stack. 

  @param[in, out]  Nbuf     Pointer to the net buffer.
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

  @param[in, out]  Nbuf       Pointer to the net buffer.
  @param[in]       Len        The length of the buffer to be allocated.
  @param[in]       FromHead   The flag to indicate whether reserve the data 
                              from head (TRUE) or tail (FALSE).

  @return                     Pointer to the first byte of the allocated buffer, 
                              or NULL if there is no sufficient space.

**/
UINT8*
EFIAPI
NetbufAllocSpace (
  IN OUT NET_BUF            *Nbuf,
  IN UINT32                 Len,
  IN BOOLEAN                FromHead
  );

/**
  Trim Len bytes from the header or tail of the net buffer. 

  @param[in, out]  Nbuf         Pointer to the net buffer.
  @param[in]       Len          The length of the data to be trimmed.
  @param[in]      FromHead      The flag to indicate whether trim data from head 
                                (TRUE) or tail (FALSE).

  @return    Length of the actually trimmed data, which is possible to be less 
             than Len because the TotalSize of Nbuf is less than Len.

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
 
  The Len bytes of data may cross the several fragments of the net buffer.
 
  @param[in]   Nbuf         Pointer to the net buffer.
  @param[in]   Offset       The sequence number of the first byte to copy.
  @param[in]   Len          Length of the data to copy.
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
   
  A new NET_BUF structure will be created from external blocks. Additional block
  of memory will be allocated to hold reserved HeadSpace bytes of header room
  and existing HeadLen bytes of header but the external blocks are shared by the
  net buffer to avoid data copying.

  @param[in]  ExtFragment           Pointer to the data block.
  @param[in]  ExtNum                The number of the data blocks.
  @param[in]  HeadSpace             The head space to be reserved.
  @param[in]  HeadLen               The length of the protocol header, This function
                                    will pull that number of data into a linear block.
  @param[in]  ExtFree               Pointer to the caller provided free function.
  @param[in]  Arg                   The argument passed to ExtFree when ExtFree is
                                    called.

  @return                  Pointer to the net buffer built from the data blocks, 
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
   
  @param[in]       Nbuf                  Point to the net buffer.
  @param[in, out]  ExtFragment           Pointer to the data block.
  @param[in, out]  ExtNum                The number of the data blocks.

  @retval EFI_BUFFER_TOO_SMALL  The number of non-empty block is bigger than 
                                ExtNum.
  @retval EFI_SUCCESS           Fragment table is built successfully.

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
   
  All the fragments will be collected from the list of NEW_BUF and then a new 
  net buffer will be created through NetbufFromExt. 
   
  @param[in]   BufList    A List of the net buffer.
  @param[in]   HeadSpace  The head space to be reserved.
  @param[in]   HeaderLen  The length of the protocol header, This function
                          will pull that number of data into a linear block.
  @param[in]   ExtFree    Pointer to the caller provided free function.
  @param[in]   Arg        The argument passed to ExtFree when ExtFree is called.

  @return                 Pointer to the net buffer built from the list of net 
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

  @param[in, out]  Head              Pointer to the head of linked net buffers.

**/
VOID
EFIAPI
NetbufFreeList (
  IN OUT LIST_ENTRY         *Head
  );

/**
  Initiate the net buffer queue.

  @param[in, out]  NbufQue   Pointer to the net buffer queue to be initialized.

**/
VOID
EFIAPI
NetbufQueInit (
  IN OUT NET_BUF_QUEUE          *NbufQue
  );

/**
  Allocate and initialize a net buffer queue.

  @return         Pointer to the allocated net buffer queue, or NULL if the
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

  @param[in]  NbufQue               Pointer to the net buffer queue to be freed.

**/
VOID
EFIAPI
NetbufQueFree (
  IN NET_BUF_QUEUE          *NbufQue
  );

/**
  Remove a net buffer from the head in the specific queue and return it.

  @param[in, out]  NbufQue               Pointer to the net buffer queue.

  @return           Pointer to the net buffer removed from the specific queue, 
                    or NULL if there is no net buffer in the specific queue.

**/
NET_BUF  *
EFIAPI
NetbufQueRemove (
  IN OUT NET_BUF_QUEUE          *NbufQue
  );

/**
  Append a net buffer to the net buffer queue.

  @param[in, out]  NbufQue            Pointer to the net buffer queue.
  @param[in, out]  Nbuf               Pointer to the net buffer to be appended.

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
 
  The copying operation is the same as NetbufCopy but applies to the net buffer
  queue instead of the net buffer.
 
  @param[in]   NbufQue         Pointer to the net buffer queue.
  @param[in]   Offset          The sequence number of the first byte to copy.
  @param[in]   Len             Length of the data to copy.
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
  Trim Len bytes of data from the queue header, release any of the net buffer 
  whom is trimmed wholely.
   
  The trimming operation is the same as NetbufTrim but applies to the net buffer
  queue instead of the net buffer.

  @param[in, out]  NbufQue               Pointer to the net buffer queue.
  @param[in]       Len                   Length of the data to trim.

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

  @param[in, out]  NbufQue               Pointer to the queue to be flushed.

**/
VOID
EFIAPI
NetbufQueFlush (
  IN OUT NET_BUF_QUEUE          *NbufQue
  );

/**
  Compute the checksum for a bulk of data.

  @param[in]   Bulk                  Pointer to the data.
  @param[in]   Len                   Length of the data, in bytes.

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

  @param[in]   Nbuf                  Pointer to the net buffer.

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

#endif
