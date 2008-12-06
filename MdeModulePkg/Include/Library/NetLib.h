/** @file
  This library provides basic function for UEFI network stack.

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
  // The address classfication
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
  Return the length of the mask. If the mask is invalid,
  return the invalid length 33, which is IP4_MASK_NUM.
  NetMask is in the host byte order.

  @param  NetMask               The netmask to get the length from

  @return The length of the netmask, IP4_MASK_NUM if the mask isn't
  @return supported.

**/
INTN
EFIAPI
NetGetMaskLength (
  IN IP4_ADDR               Mask
  );

/**
  Return the class of the address, such as class a, b, c.
  Addr is in host byte order.

  @param  Addr                  The address to get the class from

  @return IP address class, such as IP4_ADDR_CLASSA

**/
INTN
EFIAPI
NetGetIpClass (
  IN IP4_ADDR               Addr
  );

/**
  Check whether the IP is a valid unicast address according to
  the netmask. If NetMask is zero, use the IP address's class to
  get the default mask.

  @param  Ip                    The IP to check againist
  @param  NetMask               The mask of the IP

  @return TRUE if IP is a valid unicast address on the network, otherwise FALSE

**/
BOOLEAN
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
  Extract a UINT32 from a byte stream, then convert it to host
  byte order. Use this function to avoid alignment error.

  @param  Buf                   The buffer to extract the UINT32.

  @return The UINT32 extracted.

**/
UINT32
EFIAPI
NetGetUint32 (
  IN UINT8                  *Buf
  );

/**
  Put a UINT32 to the byte stream. Convert it from host byte order
  to network byte order before putting.

  @param  Buf                   The buffer to put the UINT32
  @param  Data                  The data to put

  @return None

**/
VOID
EFIAPI
NetPutUint32 (
  IN UINT8                  *Buf,
  IN UINT32                 Data
  );

/**
  Initialize a random seed using current time.

  None

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
  Remove the first entry on the list

  @param  Head                  The list header

  @return The entry that is removed from the list, NULL if the list is empty.

**/
LIST_ENTRY *
EFIAPI
NetListRemoveHead (
  LIST_ENTRY            *Head
  );

/**
  Remove the last entry on the list

  @param  Head                  The list head

  @return The entry that is removed from the list, NULL if the list is empty.

**/
LIST_ENTRY *
EFIAPI
NetListRemoveTail (
  LIST_ENTRY            *Head
  );

/**
  Insert the NewEntry after the PrevEntry.

  @param  PrevEntry             The previous entry to insert after
  @param  NewEntry              The new entry to insert

  @return None

**/
VOID
EFIAPI
NetListInsertAfter (
  IN LIST_ENTRY         *PrevEntry,
  IN LIST_ENTRY         *NewEntry
  );

/**
  Insert the NewEntry before the PostEntry.

  @param  PostEntry             The entry to insert before
  @param  NewEntry              The new entry to insert

  @return None

**/
VOID
EFIAPI
NetListInsertBefore (
  IN LIST_ENTRY         *PostEntry,
  IN LIST_ENTRY         *NewEntry
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

  @param  Map                   The netmap to initialize

  @return None

**/
VOID
EFIAPI
NetMapInit (
  IN NET_MAP                *Map
  );

/**
  To clean up the netmap, that is, release allocated memories.

  @param  Map                   The netmap to clean up.

  @return None

**/
VOID
EFIAPI
NetMapClean (
  IN NET_MAP                *Map
  );

/**
  Test whether the netmap is empty

  @param  Map                   The net map to test

  @return TRUE if the netmap is empty, otherwise FALSE.

**/
BOOLEAN
EFIAPI
NetMapIsEmpty (
  IN NET_MAP                *Map
  );

/**
  Return the number of the <Key, Value> pairs in the netmap.

  @param  Map                   The netmap to get the entry number

  @return The entry number in the netmap.

**/
UINTN
EFIAPI
NetMapGetCount (
  IN NET_MAP                *Map
  );

/**
  Allocate an item to save the <Key, Value> pair to the head of the netmap.

  @param  Map                   The netmap to insert into
  @param  Key                   The user's key
  @param  Value                 The user's value for the key

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the item
  @retval EFI_SUCCESS           The item is inserted to the head

**/
EFI_STATUS
EFIAPI
NetMapInsertHead (
  IN NET_MAP                *Map,
  IN VOID                   *Key,
  IN VOID                   *Value    OPTIONAL
  );

/**
  Allocate an item to save the <Key, Value> pair to the tail of the netmap.

  @param  Map                   The netmap to insert into
  @param  Key                   The user's key
  @param  Value                 The user's value for the key

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the memory for the item
  @retval EFI_SUCCESS           The item is inserted to the tail

**/
EFI_STATUS
EFIAPI
NetMapInsertTail (
  IN NET_MAP                *Map,
  IN VOID                   *Key,
  IN VOID                   *Value    OPTIONAL
  );

/**
  Find the key in the netmap

  @param  Map                   The netmap to search within
  @param  Key                   The key to search

  @return The point to the item contains the Key, or NULL if Key isn't in the map.

**/
NET_MAP_ITEM  *
EFIAPI
NetMapFindKey (
  IN  NET_MAP               *Map,
  IN  VOID                  *Key
  );

/**
  Remove the item from the netmap

  @param  Map                   The netmap to remove the item from
  @param  Item                  The item to remove
  @param  Value                 The variable to receive the value if not NULL

  @return The key of the removed item.

**/
VOID *
EFIAPI
NetMapRemoveItem (
  IN  NET_MAP               *Map,
  IN  NET_MAP_ITEM          *Item,
  OUT VOID                  **Value   OPTIONAL
  );

/**
  Remove the first entry on the netmap.

  @param  Map                   The netmap to remove the head from
  @param  Value                 The variable to receive the value if not NULL

  @return The key of the item removed

**/
VOID *
EFIAPI
NetMapRemoveHead (
  IN  NET_MAP               *Map,
  OUT VOID                  **Value   OPTIONAL
  );

/**
  Remove the last entry on the netmap.

  @param  Map                   The netmap to remove the tail from
  @param  Value                 The variable to receive the value if not NULL

  @return The key of the item removed

**/
VOID *
EFIAPI
NetMapRemoveTail (
  IN  NET_MAP               *Map,
  OUT VOID                  **Value OPTIONAL
  );

typedef
EFI_STATUS
(*NET_MAP_CALLBACK) (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Arg
  );

/**
  Iterate through the netmap and call CallBack for each item. It will
  contiue the traverse if CallBack returns EFI_SUCCESS, otherwise, break
  from the loop. It returns the CallBack's last return value. This
  function is delete safe for the current item.

  @param  Map                   The Map to iterate through
  @param  CallBack              The callback function to call for each item.
  @param  Arg                   The opaque parameter to the callback

  @return It returns the CallBack's last return value.

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

  @param  ControllerHandle      The controller which has the service installed.
  @param  ImageHandle           The image handle used to open service.
  @param  ServiceBindingGuid    The service's Guid.
  @param  ChildHandle           The handle to receive the create child

  @retval EFI_SUCCESS           The child is successfully created.
  @retval Others                Failed to create the child.

**/
EFI_STATUS
EFIAPI
NetLibCreateServiceChild (
  IN  EFI_HANDLE            ControllerHandle,
  IN  EFI_HANDLE            ImageHandle,
  IN  EFI_GUID              *ServiceBindingGuid,
  OUT EFI_HANDLE            *ChildHandle
  );

/**
  Destory a child of the service that is identified by ServiceBindingGuid.

  @param  ControllerHandle      The controller which has the service installed.
  @param  ImageHandle           The image handle used to open service.
  @param  ServiceBindingGuid    The service's Guid.
  @param  ChildHandle           The child to destory

  @retval EFI_SUCCESS           The child is successfully destoried.
  @retval Others                Failed to destory the child.

**/
EFI_STATUS
EFIAPI
NetLibDestroyServiceChild (
  IN  EFI_HANDLE            ControllerHandle,
  IN  EFI_HANDLE            ImageHandle,
  IN  EFI_GUID              *ServiceBindingGuid,
  IN  EFI_HANDLE            ChildHandle
  );

/**
  Convert the mac address of the simple network protocol installed on
  SnpHandle to a unicode string. Callers are responsible for freeing the
  string storage.

  @param  SnpHandle             The handle where the simple network protocol is
                                installed on.
  @param  ImageHandle           The image handle used to act as the agent handle to
                                get the simple network protocol.
  @param  MacString             The pointer to store the address of the string
                                representation of  the mac address.

  @retval EFI_OUT_OF_RESOURCES  There are not enough memory resource.
  @retval other                 Failed to open the simple network protocol.

**/
EFI_STATUS
EFIAPI
NetLibGetMacString (
  IN           EFI_HANDLE  SnpHandle,
  IN           EFI_HANDLE  ImageHandle,
  IN OUT       CHAR16      **MacString
  );

/**
  Create an IPv4 device path node.

  @param  Node                  Pointer to the IPv4 device path node.
  @param  Controller            The handle where the NIC IP4 config protocol resides.
  @param  LocalIp               The local IPv4 address.
  @param  LocalPort             The local port.
  @param  RemoteIp              The remote IPv4 address.
  @param  RemotePort            The remote port.
  @param  Protocol              The protocol type in the IP header.
  @param  UseDefaultAddress     Whether this instance is using default address or not.

  @retval None
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

  @param  Controller            Then protocol handle to check
  @param  ProtocolGuid          The protocol that is related with the handle.

  @return The UNDI/SNP handle or NULL.

**/
EFI_HANDLE
EFIAPI
NetLibGetNicHandle (
  IN EFI_HANDLE             Controller,
  IN EFI_GUID               *ProtocolGuid
  );

/**
  Add a Deferred Procedure Call to the end of the DPC queue.

  @param DpcTpl           The EFI_TPL that the DPC should be invoked.
  @param DpcProcedure     Pointer to the DPC's function.
  @param DpcContext       Pointer to the DPC's context.  Passed to DpcProcedure
                          when DpcProcedure is invoked.

  @retval  EFI_SUCCESS              The DPC was queued.
  @retval  EFI_INVALID_PARAMETER    DpcTpl is not a valid EFI_TPL.
                                    DpcProcedure is NULL.
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
  Add a Deferred Procedure Call to the end of the DPC queue.

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

  @param  ImageHandle           The drivers' driver image.

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
  NET_BUF_SIGNATURE    = EFI_SIGNATURE_32 ('n', 'b', 'u', 'f'),
  NET_VECTOR_SIGNATURE = EFI_SIGNATURE_32 ('n', 'v', 'e', 'c'),
  NET_QUE_SIGNATURE    = EFI_SIGNATURE_32 ('n', 'b', 'q', 'u'),


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

  @param  Len                   The length of the block.

  @retval *                     Pointer to the allocated NET_BUF. If NULL  the
                                allocation failed due to resource limit.

**/
NET_BUF  *
EFIAPI
NetbufAlloc (
  IN UINT32                 Len
  );

/**
  Free the buffer and its associated NET_VECTOR.

  @param  Nbuf                  Pointer to the NET_BUF to be freed.

  @return None.

**/
VOID
EFIAPI
NetbufFree (
  IN NET_BUF                *Nbuf
  );

/**
  Get the position of some byte in the net buffer. This can be used
  to, for example, retrieve the IP header in the packet. It also
  returns the fragment that contains the byte which is used mainly by
  the buffer implementation itself.

  @param  Nbuf                  Pointer to the net buffer.
  @param  Offset                The index or offset of the byte
  @param  Index                 Index of the fragment that contains the block

  @retval *                     Pointer to the nth byte of data in the net buffer.
                                If NULL, there is no such data in the net buffer.

**/
UINT8  *
EFIAPI
NetbufGetByte (
  IN  NET_BUF               *Nbuf,
  IN  UINT32                Offset,
  OUT UINT32                *Index      OPTIONAL
  );

/**
  Create a copy of NET_BUF that share the associated NET_DATA.

  @param  Nbuf                  Pointer to the net buffer to be cloned.

  @retval *                     Pointer to the cloned net buffer.

**/
NET_BUF  *
EFIAPI
NetbufClone (
  IN NET_BUF                *Nbuf
  );

/**
  Create a duplicated copy of Nbuf, data is copied. Also leave some
  head space before the data.

  @param  Nbuf                  Pointer to the net buffer to be cloned.
  @param  Duplicate             Pointer to the net buffer to duplicate to, if NULL
                                a new net  buffer is allocated.
  @param  HeadSpace             Length of the head space to reserve

  @retval *                     Pointer to the duplicated net buffer.

**/
NET_BUF  *
EFIAPI
NetbufDuplicate (
  IN NET_BUF                *Nbuf,
  IN NET_BUF                *Duplicate    OPTIONAL,
  IN UINT32                 HeadSpace
  );

/**
  Create a NET_BUF structure which contains Len byte data of
  Nbuf starting from Offset. A new NET_BUF structure will be
  created but the associated data in NET_VECTOR is shared.
  This function exists to do IP packet fragmentation.

  @param  Nbuf                  Pointer to the net buffer to be cloned.
  @param  Offset                Starting point of the data to be included in new
                                buffer.
  @param  Len                   How many data to include in new data
  @param  HeadSpace             How many bytes of head space to reserve for
                                protocol header

  @retval *                     Pointer to the cloned net buffer.

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
  Reserve some space in the header room of the buffer.
  Upon allocation, all the space are in the tail room
  of the buffer. Call this function to move some space
  to the header room. This function is quite limited in
  that it can only reserver space from the first block
  of an empty NET_BUF not built from the external. But
  it should be enough for the network stack.

  @param  Nbuf                  Pointer to the net buffer.
  @param  Len                   The length of buffer to be reserverd.

  @return None.

**/
VOID
EFIAPI
NetbufReserve (
  IN NET_BUF                *Nbuf,
  IN UINT32                 Len
  );

/**
  Allocate some space from the header or tail of the buffer.

  @param  Nbuf                  Pointer to the net buffer.
  @param  Len                   The length of the buffer to be allocated.
  @param  FromHead              The flag to indicate whether reserve the data from
                                head or tail. TRUE for from head, and FALSE for
                                from tail.

  @retval *                     Pointer to the first byte of the allocated buffer.

**/
UINT8  *
EFIAPI
NetbufAllocSpace (
  IN NET_BUF                *Nbuf,
  IN UINT32                 Len,
  IN BOOLEAN                FromHead
  );

/**
  Trim some data from the header or tail of the buffer.

  @param  Nbuf                  Pointer to the net buffer.
  @param  Len                   The length of the data to be trimmed.
  @param  FromHead              The flag to indicate whether trim data from head or
                                tail. TRUE for from head, and FALSE for from tail.

  @retval UINTN                 Length of the actually trimmed data.

**/
UINT32
EFIAPI
NetbufTrim (
  IN NET_BUF                *Nbuf,
  IN UINT32                 Len,
  IN BOOLEAN                FromHead
  );

/**
  Copy the data from the specific offset to the destination.

  @param  Nbuf                  Pointer to the net buffer.
  @param  Offset                The sequence number of the first byte to copy.
  @param  Len                   Length of the data to copy.
  @param  Dest                  The destination of the data to copy to.

  @retval UINTN                 The length of the copied data.

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

  @param  ExtFragment           Pointer to the data block.
  @param  ExtNum                The number of the data block.
  @param  HeadSpace             The head space to be reserved.
  @param  HeadLen               The length of the protocol header, This function
                                will pull that number of data into a linear block.
  @param  ExtFree               Pointer to the caller provided free function.
  @param  Arg                   The argument passed to ExtFree when ExtFree is
                                called.

  @retval *                     Pointer to the net buffer built from the data
                                blocks.

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
  Build a fragment table to contain the fragments in the
  buffer. This is the opposite of the NetbufFromExt.

  @param  Nbuf                  Point to the net buffer
  @param  ExtFragment           Pointer to the data block.
  @param  ExtNum                The number of the data block.

  @retval EFI_BUFFER_TOO_SMALL  The number of non-empty block is bigger than ExtNum
  @retval EFI_SUCCESS           Fragment table built.

**/
EFI_STATUS
EFIAPI
NetbufBuildExt (
  IN NET_BUF                *Nbuf,
  IN NET_FRAGMENT           *ExtFragment,
  IN UINT32                 *ExtNum
  );

/**
  Build a NET_BUF from a list of NET_BUF.

  @param  BufList               A List of NET_BUF.
  @param  HeadSpace             The head space to be reserved.
  @param  HeaderLen             The length of the protocol header, This function
                                will pull that number of data into a linear block.
  @param  ExtFree               Pointer to the caller provided free function.
  @param  Arg                   The argument passed to ExtFree when ExtFree is
                                called.

  @retval *                     Pointer to the net buffer built from the data
                                blocks.

**/
NET_BUF  *
EFIAPI
NetbufFromBufList (
  IN LIST_ENTRY             *BufList,
  IN UINT32                 HeadSpace,
  IN UINT32                 HeaderLen,
  IN NET_VECTOR_EXT_FREE    ExtFree,
  IN VOID                   *Arg                OPTIONAL
  );

/**
  Free a list of net buffers.

  @param  Head                  Pointer to the head of linked net buffers.

  @return None.

**/
VOID
EFIAPI
NetbufFreeList (
  IN LIST_ENTRY             *Head
  );

/**
  Initiate the net buffer queue.

  @param  NbufQue               Pointer to the net buffer queue to be initiated.

  @return None.

**/
VOID
EFIAPI
NetbufQueInit (
  IN NET_BUF_QUEUE          *NbufQue
  );

/**
  Allocate an initialized net buffer queue.

  None.

  @retval *                     Pointer to the allocated net buffer queue.

**/
NET_BUF_QUEUE  *
EFIAPI
NetbufQueAlloc (
  VOID
  );

/**
  Free a net buffer queue.

  @param  NbufQue               Poitner to the net buffer queue to be freed.

  @return None.

**/
VOID
EFIAPI
NetbufQueFree (
  IN NET_BUF_QUEUE          *NbufQue
  );

/**
  Remove a net buffer from head in the specific queue.

  @param  NbufQue               Pointer to the net buffer queue.

  @retval *                     Pointer to the net buffer removed from the specific
                                queue.

**/
NET_BUF  *
EFIAPI
NetbufQueRemove (
  IN NET_BUF_QUEUE          *NbufQue
  );

/**
  Append a buffer to the end of the queue.

  @param  NbufQue               Pointer to the net buffer queue.
  @param  Nbuf                  Pointer to the net buffer to be appended.

  @return None.

**/
VOID
EFIAPI
NetbufQueAppend (
  IN NET_BUF_QUEUE          *NbufQue,
  IN NET_BUF                *Nbuf
  );

/**
  Copy some data from the buffer queue to the destination.

  @param  NbufQue               Pointer to the net buffer queue.
  @param  Offset                The sequence number of the first byte to copy.
  @param  Len                   Length of the data to copy.
  @param  Dest                  The destination of the data to copy to.

  @retval UINTN                 The length of the copied data.

**/
UINT32
EFIAPI
NetbufQueCopy (
  IN NET_BUF_QUEUE          *NbufQue,
  IN UINT32                 Offset,
  IN UINT32                 Len,
  IN UINT8                  *Dest
  );

/**
  Trim some data from the queue header, release the buffer if
  whole buffer is trimmed.

  @param  NbufQue               Pointer to the net buffer queue.
  @param  Len                   Length of the data to trim.

  @retval UINTN                 The length of the data trimmed.

**/
UINT32
EFIAPI
NetbufQueTrim (
  IN NET_BUF_QUEUE          *NbufQue,
  IN UINT32                 Len
  );


/**
  Flush the net buffer queue.

  @param  NbufQue               Pointer to the queue to be flushed.

  @return None.

**/
VOID
EFIAPI
NetbufQueFlush (
  IN NET_BUF_QUEUE          *NbufQue
  );

/**
  Compute checksum for a bulk of data.

  @param  Bulk                  Pointer to the data.
  @param  Len                   Length of the data, in bytes.

  @retval UINT16                The computed checksum.

**/
UINT16
EFIAPI
NetblockChecksum (
  IN UINT8                  *Bulk,
  IN UINT32                 Len
  );

/**
  Add two checksums.

  @param  Checksum1             The first checksum to be added.
  @param  Checksum2             The second checksum to be added.

  @retval UINT16                The new checksum.

**/
UINT16
EFIAPI
NetAddChecksum (
  IN UINT16                 Checksum1,
  IN UINT16                 Checksum2
  );

/**
  Compute the checksum for a NET_BUF.

  @param  Nbuf                  Pointer to the net buffer.

  @retval UINT16                The computed checksum.

**/
UINT16
EFIAPI
NetbufChecksum (
  IN NET_BUF                *Nbuf
  );

/**
  Compute the checksum for TCP/UDP pseudo header.
  Src, Dst are in network byte order. and Len is
  in host byte order.

  @param  Src                   The source address of the packet.
  @param  Dst                   The destination address of the packet.
  @param  Proto                 The protocol type of the packet.
  @param  Len                   The length of the packet.

  @retval UINT16                The computed checksum.

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
