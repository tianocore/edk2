/** @file
DnsDxe support functions implementation.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_DNS_IMPL_H_
#define __EFI_DNS_IMPL_H_

#include <Uefi.h>

//
// Libraries classes
//
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>
#include <Library/DebugLib.h>
#include <Library/DpcLib.h>
#include <Library/PrintLib.h>
#include <Library/UdpIoLib.h>

//
// UEFI Driver Model Protocols
//
#include <Protocol/DriverBinding.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/ComponentName.h>

#include <Protocol/Udp4.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/Dns4.h>

#include <Protocol/Udp6.h>
#include <Protocol/Dhcp6.h>
#include <Protocol/Dns6.h>

#include <Protocol/Ip4Config2.h>

#include "DnsDriver.h"
#include "DnsDhcp.h"

//
// Driver Version
//
#define DNS_VERSION  0x00000000

//
// Protocol instances
//
extern EFI_COMPONENT_NAME_PROTOCOL   gDnsComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gDnsComponentName2;
extern EFI_UNICODE_STRING_TABLE      *gDnsControllerNameTable;

extern EFI_DRIVER_BINDING_PROTOCOL   gDns4DriverBinding;
extern EFI_SERVICE_BINDING_PROTOCOL  mDns4ServiceBinding;
extern EFI_DNS4_PROTOCOL             mDns4Protocol;

extern EFI_DRIVER_BINDING_PROTOCOL   gDns6DriverBinding;
extern EFI_SERVICE_BINDING_PROTOCOL  mDns6ServiceBinding;
extern EFI_DNS6_PROTOCOL             mDns6Protocol;

//
// DNS related
//
#define DNS_SERVER_PORT   53

#define DNS_PROTOCOL_UDP   EFI_IP_PROTO_UDP
#define DNS_PROTOCOL_TCP   EFI_IP_PROTO_TCP

#define DNS_STATE_UNCONFIGED     0
#define DNS_STATE_CONFIGED       1
#define DNS_STATE_DESTROY        2

#define DNS_DEFAULT_TIMEOUT      2

#define DNS_TIME_TO_GETMAP       5

#pragma pack(1)

typedef union _DNS_FLAGS  DNS_FLAGS;

typedef struct {
  LIST_ENTRY             AllCacheLink;
  EFI_DNS4_CACHE_ENTRY   DnsCache;
} DNS4_CACHE;

typedef struct {
  LIST_ENTRY             AllCacheLink;
  EFI_DNS6_CACHE_ENTRY   DnsCache;
} DNS6_CACHE;

typedef struct {
  LIST_ENTRY             AllServerLink;
  EFI_IPv4_ADDRESS       Dns4ServerIp;
} DNS4_SERVER_IP;

typedef struct {
  LIST_ENTRY             AllServerLink;
  EFI_IPv6_ADDRESS       Dns6ServerIp;
} DNS6_SERVER_IP;

typedef struct {
  UINT32                     RetryCounting;
  UINT32                     PacketToLive;
  CHAR16                     *QueryHostName;
  EFI_IPv4_ADDRESS           QueryIpAddress;
  BOOLEAN                    GeneralLookUp;
  EFI_DNS4_COMPLETION_TOKEN  *Token;
} DNS4_TOKEN_ENTRY;

typedef struct {
  UINT32                     RetryCounting;
  UINT32                     PacketToLive;
  CHAR16                     *QueryHostName;
  EFI_IPv6_ADDRESS           QueryIpAddress;
  BOOLEAN                    GeneralLookUp;
  EFI_DNS6_COMPLETION_TOKEN  *Token;
} DNS6_TOKEN_ENTRY;

union _DNS_FLAGS {
  struct {
    UINT16     RCode:4;
    UINT16     Zero:3;
    UINT16     RA:1;
    UINT16     RD:1;
    UINT16     TC:1;
    UINT16     AA:1;
    UINT16     OpCode:4;
    UINT16     QR:1;
  } Bits;
  UINT16  Uint16;
};

#define DNS_FLAGS_QR_QUERY     0
#define DNS_FLAGS_QR_RESPONSE  1

#define DNS_FLAGS_OPCODE_STANDARD     0
#define DNS_FLAGS_OPCODE_INVERSE      1
#define DNS_FLAGS_OPCODE_SERVER_STATE 2

#define DNS_FLAGS_RCODE_NO_ERROR    0
#define DNS_FLAGS_RCODE_NAME_ERROR  3

typedef struct {
  UINT16      Identification;
  DNS_FLAGS   Flags;
  UINT16      QuestionsNum;
  UINT16      AnswersNum;
  UINT16      AuthorityNum;
  UINT16      AditionalNum;
} DNS_HEADER;

typedef struct {
  UINT16      Type;
  UINT16      Class;
} DNS_QUERY_SECTION;

typedef struct {
  UINT16      Type;
  UINT16      Class;
  UINT32      Ttl;
  UINT16      DataLength;
} DNS_ANSWER_SECTION;

#define DNS4_DOMAIN  L"in-addr.arpa"
#define DNS6_DOMAIN  L"IP6.ARPA"


#pragma pack()

/**
  Remove TokenEntry from TokenMap.

  @param[in] TokenMap          All DNSv4 Token entrys.
  @param[in] TokenEntry        TokenEntry need to be removed.

  @retval EFI_SUCCESS          Remove TokenEntry from TokenMap successfully.
  @retval EFI_NOT_FOUND        TokenEntry is not found in TokenMap.

**/
EFI_STATUS
Dns4RemoveTokenEntry (
  IN NET_MAP                    *TokenMap,
  IN DNS4_TOKEN_ENTRY           *TokenEntry
  );

/**
  Remove TokenEntry from TokenMap.

  @param[in] TokenMap           All DNSv6 Token entrys.
  @param[in] TokenEntry         TokenEntry need to be removed.

  @retval EFI_SUCCESS           Remove TokenEntry from TokenMap successfully.
  @retval EFI_NOT_FOUND         TokenEntry is not found in TokenMap.

**/
EFI_STATUS
Dns6RemoveTokenEntry (
  IN NET_MAP                    *TokenMap,
  IN DNS6_TOKEN_ENTRY           *TokenEntry
  );

/**
  This function cancel the token specified by Arg in the Map.

  @param[in]  Map             Pointer to the NET_MAP.
  @param[in]  Item            Pointer to the NET_MAP_ITEM.
  @param[in]  Arg             Pointer to the token to be cancelled. If NULL, all
                              the tokens in this Map will be cancelled.
                              This parameter is optional and may be NULL.

  @retval EFI_SUCCESS         The token is cancelled if Arg is NULL, or the token
                              is not the same as that in the Item, if Arg is not
                              NULL.
  @retval EFI_ABORTED         Arg is not NULL, and the token specified by Arg is
                              cancelled.

**/
EFI_STATUS
EFIAPI
Dns4CancelTokens (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg OPTIONAL
  );

/**
  This function cancel the token specified by Arg in the Map.

  @param[in]  Map             Pointer to the NET_MAP.
  @param[in]  Item            Pointer to the NET_MAP_ITEM.
  @param[in]  Arg             Pointer to the token to be cancelled. If NULL, all
                              the tokens in this Map will be cancelled.
                              This parameter is optional and may be NULL.

  @retval EFI_SUCCESS         The token is cancelled if Arg is NULL, or the token
                              is not the same as that in the Item, if Arg is not
                              NULL.
  @retval EFI_ABORTED         Arg is not NULL, and the token specified by Arg is
                              cancelled.

**/
EFI_STATUS
EFIAPI
Dns6CancelTokens (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg OPTIONAL
  );

/**
  Get the TokenEntry from the TokensMap.

  @param[in]  TokensMap           All DNSv4 Token entrys
  @param[in]  Token               Pointer to the token to be get.
  @param[out] TokenEntry          Pointer to TokenEntry corresponding Token.

  @retval EFI_SUCCESS             Get the TokenEntry from the TokensMap successfully.
  @retval EFI_NOT_FOUND           TokenEntry is not found in TokenMap.

**/
EFI_STATUS
EFIAPI
GetDns4TokenEntry (
  IN     NET_MAP                   *TokensMap,
  IN     EFI_DNS4_COMPLETION_TOKEN *Token,
     OUT DNS4_TOKEN_ENTRY          **TokenEntry
  );

/**
  Get the TokenEntry from the TokensMap.

  @param[in]  TokensMap           All DNSv6 Token entrys
  @param[in]  Token               Pointer to the token to be get.
  @param[out] TokenEntry          Pointer to TokenEntry corresponding Token.

  @retval EFI_SUCCESS             Get the TokenEntry from the TokensMap successfully.
  @retval EFI_NOT_FOUND           TokenEntry is not found in TokenMap.

**/
EFI_STATUS
EFIAPI
GetDns6TokenEntry (
  IN     NET_MAP                   *TokensMap,
  IN     EFI_DNS6_COMPLETION_TOKEN *Token,
     OUT DNS6_TOKEN_ENTRY          **TokenEntry
  );

/**
  Cancel DNS4 tokens from the DNS4 instance.

  @param[in]  Instance           Pointer to the DNS instance context data.
  @param[in]  Token              Pointer to the token to be canceled. If NULL, all
                                 tokens in this instance will be cancelled.
                                 This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The Token is cancelled.
  @retval EFI_NOT_FOUND          The Token is not found.

**/
EFI_STATUS
Dns4InstanceCancelToken (
  IN DNS_INSTANCE               *Instance,
  IN EFI_DNS4_COMPLETION_TOKEN  *Token
  );

/**
  Cancel DNS6 tokens from the DNS6 instance.

  @param[in]  Instance           Pointer to the DNS instance context data.
  @param[in]  Token              Pointer to the token to be canceled. If NULL, all
                                 tokens in this instance will be cancelled.
                                 This parameter is optional and may be NULL.

  @retval EFI_SUCCESS            The Token is cancelled.
  @retval EFI_NOT_FOUND          The Token is not found.

**/
EFI_STATUS
Dns6InstanceCancelToken (
  IN DNS_INSTANCE               *Instance,
  IN EFI_DNS6_COMPLETION_TOKEN  *Token
  );

/**
  Free the resource related to the configure parameters.

  @param  Config                 The DNS configure data

**/
VOID
Dns4CleanConfigure (
  IN OUT EFI_DNS4_CONFIG_DATA  *Config
  );

/**
  Free the resource related to the configure parameters.

  @param  Config                 The DNS configure data

**/
VOID
Dns6CleanConfigure (
  IN OUT EFI_DNS6_CONFIG_DATA  *Config
  );

/**
  Allocate memory for configure parameter such as timeout value for Dst,
  then copy the configure parameter from Src to Dst.

  @param[out]  Dst               The destination DHCP configure data.
  @param[in]   Src               The source DHCP configure data.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_SUCCESS            The configure is copied.

**/
EFI_STATUS
Dns4CopyConfigure (
  OUT EFI_DNS4_CONFIG_DATA  *Dst,
  IN  EFI_DNS4_CONFIG_DATA  *Src
  );

/**
  Allocate memory for configure parameter such as timeout value for Dst,
  then copy the configure parameter from Src to Dst.

  @param[out]  Dst               The destination DHCP configure data.
  @param[in]   Src               The source DHCP configure data.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_SUCCESS            The configure is copied.

**/
EFI_STATUS
Dns6CopyConfigure (
  OUT EFI_DNS6_CONFIG_DATA  *Dst,
  IN  EFI_DNS6_CONFIG_DATA  *Src
  );

/**
  Callback of Dns packet. Does nothing.

  @param Arg           The context.

**/
VOID
EFIAPI
DnsDummyExtFree (
  IN VOID                   *Arg
  );

/**
  Poll the UDP to get the IP4 default address, which may be retrieved
  by DHCP.

  The default time out value is 5 seconds. If IP has retrieved the default address,
  the UDP is reconfigured.

  @param  Instance               The DNS instance
  @param  UdpIo                  The UDP_IO to poll
  @param  UdpCfgData             The UDP configure data to reconfigure the UDP_IO

  @retval TRUE                   The default address is retrieved and UDP is reconfigured.
  @retval FALSE                  Some error occurred.

**/
BOOLEAN
Dns4GetMapping (
  IN DNS_INSTANCE           *Instance,
  IN UDP_IO                 *UdpIo,
  IN EFI_UDP4_CONFIG_DATA   *UdpCfgData
  );

/**
  Configure the opened Udp6 instance until the corresponding Ip6 instance
  has been configured.

  @param  Instance               The DNS instance
  @param  UdpIo                  The UDP_IO to poll
  @param  UdpCfgData             The UDP configure data to reconfigure the UDP_IO

  @retval TRUE                   Configure the Udp6 instance successfully.
  @retval FALSE                  Some error occurred.

**/
BOOLEAN
Dns6GetMapping (
  IN DNS_INSTANCE           *Instance,
  IN UDP_IO                 *UdpIo,
  IN EFI_UDP6_CONFIG_DATA   *UdpCfgData
  );

/**
  Configure the UDP.

  @param  Instance               The DNS session
  @param  UdpIo                  The UDP_IO instance

  @retval EFI_SUCCESS            The UDP is successfully configured for the
                                 session.

**/
EFI_STATUS
Dns4ConfigUdp (
  IN DNS_INSTANCE           *Instance,
  IN UDP_IO                 *UdpIo
  );

/**
  Configure the UDP.

  @param  Instance               The DNS session
  @param  UdpIo                  The UDP_IO instance

  @retval EFI_SUCCESS            The UDP is successfully configured for the
                                 session.

**/
EFI_STATUS
Dns6ConfigUdp (
  IN DNS_INSTANCE           *Instance,
  IN UDP_IO                 *UdpIo
  );

/**
  Update Dns4 cache to shared list of caches of all DNSv4 instances.

  @param  Dns4CacheList      All Dns4 cache list.
  @param  DeleteFlag         If FALSE, this function is to add one entry to the DNS Cache.
                             If TRUE, this function will delete matching DNS Cache entry.
  @param  Override           If TRUE, the matching DNS cache entry will be overwritten with the supplied parameter.
                             If FALSE, EFI_ACCESS_DENIED will be returned if the entry to be added is already exists.
  @param  DnsCacheEntry      Entry Pointer to DNS Cache entry.

  @retval EFI_SUCCESS        Update Dns4 cache successfully.
  @retval Others             Failed to update Dns4 cache.

**/
EFI_STATUS
EFIAPI
UpdateDns4Cache (
  IN LIST_ENTRY             *Dns4CacheList,
  IN BOOLEAN                DeleteFlag,
  IN BOOLEAN                Override,
  IN EFI_DNS4_CACHE_ENTRY   DnsCacheEntry
  );

/**
  Update Dns6 cache to shared list of caches of all DNSv6 instances.

  @param  Dns6CacheList      All Dns6 cache list.
  @param  DeleteFlag         If FALSE, this function is to add one entry to the DNS Cache.
                             If TRUE, this function will delete matching DNS Cache entry.
  @param  Override           If TRUE, the matching DNS cache entry will be overwritten with the supplied parameter.
                             If FALSE, EFI_ACCESS_DENIED will be returned if the entry to be added is already exists.
  @param  DnsCacheEntry      Entry Pointer to DNS Cache entry.

  @retval EFI_SUCCESS        Update Dns6 cache successfully.
  @retval Others             Failed to update Dns6 cache.
**/
EFI_STATUS
EFIAPI
UpdateDns6Cache (
  IN LIST_ENTRY             *Dns6CacheList,
  IN BOOLEAN                DeleteFlag,
  IN BOOLEAN                Override,
  IN EFI_DNS6_CACHE_ENTRY   DnsCacheEntry
  );

/**
  Add Dns4 ServerIp to common list of addresses of all configured DNSv4 server.

  @param  Dns4ServerList    Common list of addresses of all configured DNSv4 server.
  @param  ServerIp          DNS server Ip.

  @retval EFI_SUCCESS       Add Dns4 ServerIp to common list successfully.
  @retval Others            Failed to add Dns4 ServerIp to common list.

**/
EFI_STATUS
EFIAPI
AddDns4ServerIp (
  IN LIST_ENTRY                *Dns4ServerList,
  IN EFI_IPv4_ADDRESS           ServerIp
  );

/**
  Add Dns6 ServerIp to common list of addresses of all configured DNSv6 server.

  @param  Dns6ServerList    Common list of addresses of all configured DNSv6 server.
  @param  ServerIp          DNS server Ip.

  @retval EFI_SUCCESS       Add Dns6 ServerIp to common list successfully.
  @retval Others            Failed to add Dns6 ServerIp to common list.

**/
EFI_STATUS
EFIAPI
AddDns6ServerIp (
  IN LIST_ENTRY                *Dns6ServerList,
  IN EFI_IPv6_ADDRESS           ServerIp
  );

/**
  Find out whether the response is valid or invalid.

  @param  TokensMap       All DNS transmittal Tokens entry.
  @param  Identification  Identification for queried packet.
  @param  Type            Type for queried packet.
  @param  Class           Class for queried packet.
  @param  Item            Return corresponding Token entry.

  @retval TRUE            The response is valid.
  @retval FALSE           The response is invalid.

**/
BOOLEAN
IsValidDnsResponse (
  IN     NET_MAP      *TokensMap,
  IN     UINT16       Identification,
  IN     UINT16       Type,
  IN     UINT16       Class,
     OUT NET_MAP_ITEM **Item
  );

/**
  Parse Dns Response.

  @param  Instance              The DNS instance
  @param  RxString              Received buffer.
  @param  Length                Received buffer length.
  @param  Completed             Flag to indicate that Dns response is valid.

  @retval EFI_SUCCESS           Parse Dns Response successfully.
  @retval Others                Failed to parse Dns Response.

**/
EFI_STATUS
ParseDnsResponse (
  IN OUT DNS_INSTANCE              *Instance,
  IN     UINT8                     *RxString,
  IN     UINT32                    Length,
     OUT BOOLEAN                   *Completed
  );

/**
  Parse response packet.

  @param  Packet                The packets received.
  @param  EndPoint              The local/remote UDP access point
  @param  IoStatus              The status of the UDP receive
  @param  Context               The opaque parameter to the function.

**/
VOID
EFIAPI
DnsOnPacketReceived (
  NET_BUF                   *Packet,
  UDP_END_POINT             *EndPoint,
  EFI_STATUS                IoStatus,
  VOID                      *Context
  );

/**
  Release the net buffer when packet is sent.

  @param  Packet                The packets received.
  @param  EndPoint              The local/remote UDP access point
  @param  IoStatus              The status of the UDP receive
  @param  Context               The opaque parameter to the function.

**/
VOID
EFIAPI
DnsOnPacketSent (
  NET_BUF                   *Packet,
  UDP_END_POINT             *EndPoint,
  EFI_STATUS                IoStatus,
  VOID                      *Context
  );

/**
  Query request information.

  @param  Instance              The DNS instance
  @param  Packet                The packet for querying request information.

  @retval EFI_SUCCESS           Query request information successfully.
  @retval Others                Failed to query request information.

**/
EFI_STATUS
DoDnsQuery (
  IN  DNS_INSTANCE              *Instance,
  IN  NET_BUF                   *Packet
  );

/**
  Construct the Packet according query section.

  @param  Instance              The DNS instance
  @param  QueryName             Queried Name
  @param  Type                  Queried Type
  @param  Class                 Queried Class
  @param  Packet                The packet for query

  @retval EFI_SUCCESS           The packet is constructed.
  @retval Others                Failed to construct the Packet.

**/
EFI_STATUS
ConstructDNSQuery (
  IN  DNS_INSTANCE              *Instance,
  IN  CHAR8                     *QueryName,
  IN  UINT16                    Type,
  IN  UINT16                    Class,
  OUT NET_BUF                   **Packet
  );

/**
  Retransmit the packet.

  @param  Instance              The DNS instance
  @param  Packet                Retransmit the packet

  @retval EFI_SUCCESS           The packet is retransmitted.
  @retval Others                Failed to retransmit.

**/
EFI_STATUS
DnsRetransmit (
  IN DNS_INSTANCE        *Instance,
  IN NET_BUF             *Packet
  );

/**
  The timer ticking function for the DNS service.

  @param  Event                 The ticking event
  @param  Context               The DNS service instance

**/
VOID
EFIAPI
DnsOnTimerRetransmit (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );

/**
  The timer ticking function for the DNS driver.

  @param  Event                 The ticking event
  @param  Context               NULL

**/
VOID
EFIAPI
DnsOnTimerUpdate (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );


/**
  Retrieve mode data of this DNS instance.

  This function is used to retrieve DNS mode data for this DNS instance.

  @param[in]   This               Pointer to EFI_DNS4_PROTOCOL instance.
  @param[out]  DnsModeData        Point to the mode data.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_NOT_STARTED         When DnsConfigData is queried, no configuration data
                                  is available because this instance has not been
                                  configured.
  @retval EFI_INVALID_PARAMETER   This is NULL or DnsModeData is NULL.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns4GetModeData (
  IN  EFI_DNS4_PROTOCOL          *This,
  OUT EFI_DNS4_MODE_DATA         *DnsModeData
  );

/**
  Configure this DNS instance.

  This function is used to configure DNS mode data for this DNS instance.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  DnsConfigData       Point to the Configuration data.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_UNSUPPORTED         The designated protocol is not supported.
  @retval EFI_INVALID_PARAMETER   This is NULL.
                                  The StationIp address provided in DnsConfigData is not a
                                  valid unicast.
                                  DnsServerList is NULL while DnsServerListCount
                                  is not ZERO.
                                  DnsServerListCount is ZERO while DnsServerList
                                  is not NULL
  @retval EFI_OUT_OF_RESOURCES    The DNS instance data or required space could not be
                                  allocated.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. The
                                  EFI DNSv4 Protocol instance is not configured.
  @retval EFI_ALREADY_STARTED     Second call to Configure() with DnsConfigData. To
                                  reconfigure the instance the caller must call Configure()
                                  with NULL first to return driver to unconfigured state.
**/
EFI_STATUS
EFIAPI
Dns4Configure (
  IN EFI_DNS4_PROTOCOL           *This,
  IN EFI_DNS4_CONFIG_DATA        *DnsConfigData
  );

/**
  Host name to host address translation.

  The HostNameToIp () function is used to translate the host name to host IP address. A
  type A query is used to get the one or more IP addresses for this host.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  HostName            Host name.
  @param[in]  Token               Point to the completion token to translate host name
                                  to host address.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  HostName is NULL. HostName string is unsupported format.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_NOT_STARTED         This instance has not been started.
**/
EFI_STATUS
EFIAPI
Dns4HostNameToIp (
  IN  EFI_DNS4_PROTOCOL          *This,
  IN  CHAR16                    *HostName,
  IN  EFI_DNS4_COMPLETION_TOKEN  *Token
  );

/**
  IPv4 address to host name translation also known as Reverse DNS lookup.

  The IpToHostName() function is used to translate the host address to host name. A type PTR
  query is used to get the primary name of the host. Support of this function is optional.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  IpAddress           Ip Address.
  @param[in]  Token               Point to the completion token to translate host
                                  address to host name.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_UNSUPPORTED         This function is not supported.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  IpAddress is not valid IP address .
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_ALREADY_STARTED     This Token is being used in another DNS session.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns4IpToHostName (
  IN  EFI_DNS4_PROTOCOL             *This,
  IN  EFI_IPv4_ADDRESS              IpAddress,
  IN  EFI_DNS4_COMPLETION_TOKEN     *Token
  );

/**
  Retrieve arbitrary information from the DNS server.

  This GeneralLookup() function retrieves arbitrary information from the DNS. The caller
  supplies a QNAME, QTYPE, and QCLASS, and all of the matching RRs are returned. All
  RR content (e.g., TTL) was returned. The caller need parse the returned RR to get
  required information. The function is optional.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  QName               Pointer to Query Name.
  @param[in]  QType               Query Type.
  @param[in]  QClass              Query Name.
  @param[in]  Token               Point to the completion token to retrieve arbitrary
                                  information.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_UNSUPPORTED         This function is not supported. Or the requested
                                  QType is not supported
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  QName is NULL.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_ALREADY_STARTED     This Token is being used in another DNS session.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns4GeneralLookUp (
  IN  EFI_DNS4_PROTOCOL                *This,
  IN  CHAR8                            *QName,
  IN  UINT16                           QType,
  IN  UINT16                           QClass,
  IN  EFI_DNS4_COMPLETION_TOKEN        *Token
  );

/**
  This function is to update the DNS Cache.

  The UpdateDnsCache() function is used to add/delete/modify DNS cache entry. DNS cache
  can be normally dynamically updated after the DNS resolve succeeds. This function
  provided capability to manually add/delete/modify the DNS cache.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  DeleteFlag          If FALSE, this function is to add one entry to the
                                  DNS Cache. If TRUE, this function will delete
                                  matching DNS Cache entry.
  @param[in]  Override            If TRUE, the matching DNS cache entry will be
                                  overwritten with the supplied parameter. If FALSE,
                                  EFI_ACCESS_DENIED will be returned if the entry to
                                  be added is already existed.
  @param[in]  DnsCacheEntry       Pointer to DNS Cache entry.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  DnsCacheEntry.HostName is NULL.
                                  DnsCacheEntry.IpAddress is NULL.
                                  DnsCacheEntry.Timeout is zero.
  @retval EFI_ACCESS_DENIED       The DNS cache entry already exists and Override is
                                  not TRUE.
**/
EFI_STATUS
EFIAPI
Dns4UpdateDnsCache (
  IN EFI_DNS4_PROTOCOL      *This,
  IN BOOLEAN               DeleteFlag,
  IN BOOLEAN               Override,
  IN EFI_DNS4_CACHE_ENTRY   DnsCacheEntry
  );

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function can be used by network drivers and applications to increase the
  rate that data packets are moved between the communications device and the transmit
  and receive queues.
  In some systems, the periodic timer event in the managed network driver may not poll
  the underlying communications device fast enough to transmit and/or receive all data
  packets without missing incoming packets or dropping outgoing packets. Drivers and
  applications that are experiencing packet loss should try calling the Poll()
  function more often.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.

  @retval EFI_SUCCESS             Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED         This EFI DNS Protocol instance has not been started.
  @retval EFI_INVALID_PARAMETER   This is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred.
  @retval EFI_TIMEOUT             Data was dropped out of the transmit and/or receive
                                  queue. Consider increasing the polling rate.
**/
EFI_STATUS
EFIAPI
Dns4Poll (
  IN EFI_DNS4_PROTOCOL    *This
  );

/**
  Abort an asynchronous DNS operation, including translation between IP and Host, and
  general look up behavior.

  The Cancel() function is used to abort a pending resolution request. After calling
  this function, Token.Status will be set to EFI_ABORTED and then Token.Event will be
  signaled. If the token is not in one of the queues, which usually means that the
  asynchronous operation has completed, this function will not signal the token and
  EFI_NOT_FOUND is returned.

  @param[in]  This                Pointer to EFI_DNS4_PROTOCOL instance.
  @param[in]  Token               Pointer to a token that has been issued by
                                  EFI_DNS4_PROTOCOL.HostNameToIp (),
                                  EFI_DNS4_PROTOCOL.IpToHostName() or
                                  EFI_DNS4_PROTOCOL.GeneralLookup().
                                  If NULL, all pending tokens are aborted.

  @retval EFI_SUCCESS             Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED         This EFI DNS4 Protocol instance has not been started.
  @retval EFI_INVALID_PARAMETER   This is NULL.
  @retval EFI_NOT_FOUND           When Token is not NULL, and the asynchronous DNS
                                  operation was not found in the transmit queue. It
                                  was either completed or was not issued by
                                  HostNameToIp(), IpToHostName() or GeneralLookup().
**/
EFI_STATUS
EFIAPI
Dns4Cancel (
  IN  EFI_DNS4_PROTOCOL          *This,
  IN  EFI_DNS4_COMPLETION_TOKEN  *Token
  );


/**
  Retrieve mode data of this DNS instance.

  This function is used to retrieve DNS mode data for this DNS instance.

  @param[in]   This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[out]  DnsModeData         Pointer to the caller-allocated storage for the
                                   EFI_DNS6_MODE_DATA data.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_NOT_STARTED         When DnsConfigData is queried, no configuration data
                                  is available because this instance has not been
                                  configured.
  @retval EFI_INVALID_PARAMETER   This is NULL or DnsModeData is NULL.
  @retval EFI_OUT_OF_RESOURCE     Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns6GetModeData (
  IN  EFI_DNS6_PROTOCOL          *This,
  OUT EFI_DNS6_MODE_DATA         *DnsModeData
  );

/**
  Configure this DNS instance.

  The Configure() function is used to set and change the configuration data for this
  EFI DNSv6 Protocol driver instance. Reset the DNS instance if DnsConfigData is NULL.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  DnsConfigData       Pointer to the configuration data structure. All associated
                                  storage to be allocated and released by caller.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER    This is NULL.
                                  The StationIp address provided in DnsConfigData is not zero and not a valid unicast.
                                  DnsServerList is NULL while DnsServerList Count is not ZERO.
                                  DnsServerList Count is ZERO while DnsServerList is not NULL.
  @retval EFI_OUT_OF_RESOURCES    The DNS instance data or required space could not be allocated.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. The
                                  EFI DNSv6 Protocol instance is not configured.
  @retval EFI_UNSUPPORTED         The designated protocol is not supported.
  @retval EFI_ALREADY_STARTED     Second call to Configure() with DnsConfigData. To
                                  reconfigure the instance the caller must call Configure() with
                                  NULL first to return driver to unconfigured state.
**/
EFI_STATUS
EFIAPI
Dns6Configure (
  IN EFI_DNS6_PROTOCOL           *This,
  IN EFI_DNS6_CONFIG_DATA        *DnsConfigData
  );

/**
  Host name to host address translation.

  The HostNameToIp () function is used to translate the host name to host IP address. A
  type AAAA query is used to get the one or more IPv6 addresses for this host.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  HostName            Host name.
  @param[in]  Token               Point to the completion token to translate host name
                                  to host address.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  HostName is NULL or buffer contained unsupported characters.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_ALREADY_STARTED     This Token is being used in another DNS session.
  @retval EFI_NOT_STARTED         This instance has not been started.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns6HostNameToIp (
  IN  EFI_DNS6_PROTOCOL          *This,
  IN  CHAR16                     *HostName,
  IN  EFI_DNS6_COMPLETION_TOKEN  *Token
  );

/**
  Host address to host name translation.

  The IpToHostName () function is used to translate the host address to host name. A
  type PTR query is used to get the primary name of the host. Implementation can choose
  to support this function or not.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  IpAddress           Ip Address.
  @param[in]  Token               Point to the completion token to translate host
                                  address to host name.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_UNSUPPORTED         This function is not supported.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  IpAddress is not valid IP address.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_NOT_STARTED         This instance has not been started.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns6IpToHostName (
  IN  EFI_DNS6_PROTOCOL             *This,
  IN  EFI_IPv6_ADDRESS              IpAddress,
  IN  EFI_DNS6_COMPLETION_TOKEN     *Token
  );

/**
  This function provides capability to retrieve arbitrary information from the DNS
  server.

  This GeneralLookup() function retrieves arbitrary information from the DNS. The caller
  supplies a QNAME, QTYPE, and QCLASS, and all of the matching RRs are returned. All
  RR content (e.g., TTL) was returned. The caller need parse the returned RR to get
  required information. The function is optional. Implementation can choose to support
  it or not.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  QName               Pointer to Query Name.
  @param[in]  QType               Query Type.
  @param[in]  QClass              Query Name.
  @param[in]  Token               Point to the completion token to retrieve arbitrary
                                  information.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_UNSUPPORTED         This function is not supported. Or the requested
                                  QType is not supported
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token.Event is NULL.
                                  QName is NULL.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_NOT_STARTED         This instance has not been started.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns6GeneralLookUp (
  IN  EFI_DNS6_PROTOCOL                *This,
  IN  CHAR8                            *QName,
  IN  UINT16                           QType,
  IN  UINT16                           QClass,
  IN  EFI_DNS6_COMPLETION_TOKEN        *Token
  );

/**
  This function is to update the DNS Cache.

  The UpdateDnsCache() function is used to add/delete/modify DNS cache entry. DNS cache
  can be normally dynamically updated after the DNS resolve succeeds. This function
  provided capability to manually add/delete/modify the DNS cache.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  DeleteFlag          If FALSE, this function is to add one entry to the
                                  DNS Cache. If TRUE, this function will delete
                                  matching DNS Cache entry.
  @param[in]  Override            If TRUE, the matching DNS cache entry will be
                                  overwritten with the supplied parameter. If FALSE,
                                  EFI_ACCESS_DENIED will be returned if the entry to
                                  be added is already existed.
  @param[in]  DnsCacheEntry       Pointer to DNS Cache entry.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  DnsCacheEntry.HostName is NULL.
                                  DnsCacheEntry.IpAddress is NULL.
                                  DnsCacheEntry.Timeout is zero.
  @retval EFI_ACCESS_DENIED       The DNS cache entry already exists and Override is
                                  not TRUE.
  @retval EFI_OUT_OF_RESOURCE     Failed to allocate needed resources.
**/
EFI_STATUS
EFIAPI
Dns6UpdateDnsCache (
  IN EFI_DNS6_PROTOCOL      *This,
  IN BOOLEAN               DeleteFlag,
  IN BOOLEAN               Override,
  IN EFI_DNS6_CACHE_ENTRY   DnsCacheEntry
  );

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function can be used by network drivers and applications to increase the
  rate that data packets are moved between the communications device and the transmit
  and receive queues.

  In some systems, the periodic timer event in the managed network driver may not poll
  the underlying communications device fast enough to transmit and/or receive all data
  packets without missing incoming packets or dropping outgoing packets. Drivers and
  applications that are experiencing packet loss should try calling the Poll()
  function more often.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.

  @retval EFI_SUCCESS             Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED         This EFI DNS Protocol instance has not been started.
  @retval EFI_INVALID_PARAMETER   This is NULL.
  @retval EFI_NO_MAPPING          There is no source address is available for use.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred.
  @retval EFI_TIMEOUT             Data was dropped out of the transmit and/or receive
                                  queue. Consider increasing the polling rate.
**/
EFI_STATUS
EFIAPI
Dns6Poll (
  IN EFI_DNS6_PROTOCOL    *This
  );

/**
  Abort an asynchronous DNS operation, including translation between IP and Host, and
  general look up behavior.

  The Cancel() function is used to abort a pending resolution request. After calling
  this function, Token.Status will be set to EFI_ABORTED and then Token.Event will be
  signaled. If the token is not in one of the queues, which usually means that the
  asynchronous operation has completed, this function will not signal the token and
  EFI_NOT_FOUND is returned.

  @param[in]  This                Pointer to EFI_DNS6_PROTOCOL instance.
  @param[in]  Token               Pointer to a token that has been issued by
                                  EFI_DNS6_PROTOCOL.HostNameToIp (),
                                  EFI_DNS6_PROTOCOL.IpToHostName() or
                                  EFI_DNS6_PROTOCOL.GeneralLookup().
                                  If NULL, all pending tokens are aborted.

  @retval EFI_SUCCESS             Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED         This EFI DNS6 Protocol instance has not been started.
  @retval EFI_INVALID_PARAMETER   This is NULL.
  @retval EFI_NO_MAPPING          There's no source address is available for use.
  @retval EFI_NOT_FOUND           When Token is not NULL, and the asynchronous DNS
                                  operation was not found in the transmit queue. It
                                  was either completed or was not issued by
                                  HostNameToIp(), IpToHostName() or GeneralLookup().
**/
EFI_STATUS
EFIAPI
Dns6Cancel (
  IN  EFI_DNS6_PROTOCOL          *This,
  IN  EFI_DNS6_COMPLETION_TOKEN  *Token
  );

#endif
