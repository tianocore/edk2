/** @file
  Dhcp and Discover routines for PxeBc.

Copyright (c) 2007 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_PXEBC_DHCP_H__
#define __EFI_PXEBC_DHCP_H__

#define PXEBC_DHCP4_MAX_OPTION_NUM         16
#define PXEBC_DHCP4_MAX_OPTION_SIZE        312
#define PXEBC_DHCP4_MAX_PACKET_SIZE        1472

#define PXEBC_DHCP4_S_PORT                 67
#define PXEBC_DHCP4_C_PORT                 68
#define PXEBC_BS_DOWNLOAD_PORT             69
#define PXEBC_BS_DISCOVER_PORT             4011

#define PXEBC_DHCP4_OPCODE_REQUEST         1
#define PXEBC_DHCP4_OPCODE_REPLY           2
#define PXEBC_DHCP4_MSG_TYPE_REQUEST       3
#define PXEBC_DHCP4_MAGIC                  0x63538263 // network byte order
//
// Dhcp Options
//
#define PXEBC_DHCP4_TAG_PAD                0    // Pad Option
#define PXEBC_DHCP4_TAG_EOP                255  // End Option
#define PXEBC_DHCP4_TAG_NETMASK            1    // Subnet Mask
#define PXEBC_DHCP4_TAG_TIME_OFFSET        2    // Time Offset from UTC
#define PXEBC_DHCP4_TAG_ROUTER             3    // Router option,
#define PXEBC_DHCP4_TAG_TIME_SERVER        4    // Time Server
#define PXEBC_DHCP4_TAG_NAME_SERVER        5    // Name Server
#define PXEBC_DHCP4_TAG_DNS_SERVER         6    // Domain Name Server
#define PXEBC_DHCP4_TAG_HOSTNAME           12   // Host Name
#define PXEBC_DHCP4_TAG_BOOTFILE_LEN       13   // Boot File Size
#define PXEBC_DHCP4_TAG_DUMP               14   // Merit Dump File
#define PXEBC_DHCP4_TAG_DOMAINNAME         15   // Domain Name
#define PXEBC_DHCP4_TAG_ROOTPATH           17   // Root path
#define PXEBC_DHCP4_TAG_EXTEND_PATH        18   // Extensions Path
#define PXEBC_DHCP4_TAG_EMTU               22   // Maximum Datagram Reassembly Size
#define PXEBC_DHCP4_TAG_TTL                23   // Default IP Time-to-live
#define PXEBC_DHCP4_TAG_BROADCAST          28   // Broadcast Address
#define PXEBC_DHCP4_TAG_NIS_DOMAIN         40   // Network Information Service Domain
#define PXEBC_DHCP4_TAG_NIS_SERVER         41   // Network Information Servers
#define PXEBC_DHCP4_TAG_NTP_SERVER         42   // Network Time Protocol Servers
#define PXEBC_DHCP4_TAG_VENDOR             43   // Vendor Specific Information
#define PXEBC_DHCP4_TAG_REQUEST_IP         50   // Requested IP Address
#define PXEBC_DHCP4_TAG_LEASE              51   // IP Address Lease Time
#define PXEBC_DHCP4_TAG_OVERLOAD           52   // Option Overload
#define PXEBC_DHCP4_TAG_MSG_TYPE           53   // DHCP Message Type
#define PXEBC_DHCP4_TAG_SERVER_ID          54   // Server Identifier
#define PXEBC_DHCP4_TAG_PARA_LIST          55   // Parameter Request List
#define PXEBC_DHCP4_TAG_MAXMSG             57   // Maximum DHCP Message Size
#define PXEBC_DHCP4_TAG_T1                 58   // Renewal (T1) Time Value
#define PXEBC_DHCP4_TAG_T2                 59   // Rebinding (T2) Time Value
#define PXEBC_DHCP4_TAG_CLASS_ID           60   // Vendor class identifier
#define PXEBC_DHCP4_TAG_CLIENT_ID          61   // Client-identifier
#define PXEBC_DHCP4_TAG_TFTP               66   // TFTP server name
#define PXEBC_DHCP4_TAG_BOOTFILE           67   // Bootfile name
#define PXEBC_PXE_DHCP4_TAG_ARCH           93
#define PXEBC_PXE_DHCP4_TAG_UNDI           94
#define PXEBC_PXE_DHCP4_TAG_UUID           97
//
// Sub-Options in Dhcp Vendor Option
//
#define PXEBC_VENDOR_TAG_MTFTP_IP          1
#define PXEBC_VENDOR_TAG_MTFTP_CPORT       2
#define PXEBC_VENDOR_TAG_MTFTP_SPORT       3
#define PXEBC_VENDOR_TAG_MTFTP_TIMEOUT     4
#define PXEBC_VENDOR_TAG_MTFTP_DELAY       5
#define PXEBC_VENDOR_TAG_DISCOVER_CTRL     6
#define PXEBC_VENDOR_TAG_DISCOVER_MCAST    7
#define PXEBC_VENDOR_TAG_BOOT_SERVERS      8
#define PXEBC_VENDOR_TAG_BOOT_MENU         9
#define PXEBC_VENDOR_TAG_MENU_PROMPT       10
#define PXEBC_VENDOR_TAG_MCAST_ALLOC       11
#define PXEBC_VENDOR_TAG_CREDENTIAL_TYPES  12
#define PXEBC_VENDOR_TAG_BOOT_ITEM         71

#define PXEBC_DHCP4_DISCOVER_INIT_TIMEOUT  4
#define PXEBC_DHCP4_DISCOVER_RETRIES       4

#define PXEBC_MAX_MENU_NUM                 24
#define PXEBC_MAX_OFFER_NUM                16

#define PXEBC_BOOT_REQUEST_TIMEOUT         1
#define PXEBC_BOOT_REQUEST_RETRIES         4

#define PXEBC_DHCP4_OVERLOAD_FILE          1
#define PXEBC_DHCP4_OVERLOAD_SERVER_NAME   2

//
// The array index of the DHCP4 option tag interested
//
#define PXEBC_DHCP4_TAG_INDEX_BOOTFILE_LEN 0
#define PXEBC_DHCP4_TAG_INDEX_VENDOR       1
#define PXEBC_DHCP4_TAG_INDEX_OVERLOAD     2
#define PXEBC_DHCP4_TAG_INDEX_MSG_TYPE     3
#define PXEBC_DHCP4_TAG_INDEX_SERVER_ID    4
#define PXEBC_DHCP4_TAG_INDEX_CLASS_ID     5
#define PXEBC_DHCP4_TAG_INDEX_BOOTFILE     6
#define PXEBC_DHCP4_TAG_INDEX_MAX          7

//
// The type of DHCP OFFER, arranged by priority, PXE10 has the highest priority.
//
#define DHCP4_PACKET_TYPE_PXE10            0
#define DHCP4_PACKET_TYPE_WFM11A           1
#define DHCP4_PACKET_TYPE_BINL             2
#define DHCP4_PACKET_TYPE_DHCP_ONLY        3
#define DHCP4_PACKET_TYPE_BOOTP            4
#define DHCP4_PACKET_TYPE_MAX              5

#define BIT(x)  (1 << x)
#define CTRL(x) (0x1F & (x))

//
// WfM11a options
//
#define MTFTP_VENDOR_OPTION_BIT_MAP (BIT (PXEBC_VENDOR_TAG_MTFTP_IP) | \
                                     BIT (PXEBC_VENDOR_TAG_MTFTP_CPORT) | \
                                     BIT (PXEBC_VENDOR_TAG_MTFTP_SPORT) | \
                                     BIT (PXEBC_VENDOR_TAG_MTFTP_TIMEOUT) | \
                                     BIT (PXEBC_VENDOR_TAG_MTFTP_DELAY))
//
// Discoverty options
//
#define DISCOVER_VENDOR_OPTION_BIT_MAP  (BIT (PXEBC_VENDOR_TAG_DISCOVER_CTRL) | \
                                         BIT (PXEBC_VENDOR_TAG_DISCOVER_MCAST) | \
                                         BIT (PXEBC_VENDOR_TAG_BOOT_SERVERS) | \
                                         BIT (PXEBC_VENDOR_TAG_BOOT_MENU) | \
                                         BIT (PXEBC_VENDOR_TAG_MENU_PROMPT))

#define IS_VALID_BOOT_PROMPT(x) \
  ((((x)[0]) & BIT (PXEBC_VENDOR_TAG_MENU_PROMPT)) == BIT (PXEBC_VENDOR_TAG_MENU_PROMPT))

#define IS_VALID_BOOT_MENU(x) \
  ((((x)[0]) & BIT (PXEBC_VENDOR_TAG_BOOT_MENU)) == BIT (PXEBC_VENDOR_TAG_BOOT_MENU))

#define IS_VALID_MTFTP_VENDOR_OPTION(x) \
    (((UINT32) ((x)[0]) & MTFTP_VENDOR_OPTION_BIT_MAP) == MTFTP_VENDOR_OPTION_BIT_MAP)

#define IS_VALID_DISCOVER_VENDOR_OPTION(x)  (((UINT32) ((x)[0]) & DISCOVER_VENDOR_OPTION_BIT_MAP) != 0)

#define IS_VALID_CREDENTIAL_VENDOR_OPTION(x) \
    (((UINT32) ((x)[0]) & BIT (PXEBC_VENDOR_TAG_CREDENTIAL_TYPES)) == BIT (PXEBC_VENDOR_TAG_CREDENTIAL_TYPES))

#define IS_VALID_BOOTITEM_VENDOR_OPTION(x) \
    (((UINT32) ((x)[PXEBC_VENDOR_TAG_BOOT_ITEM / 32]) & BIT (PXEBC_VENDOR_TAG_BOOT_ITEM % 32)) \
      == BIT (PXEBC_VENDOR_TAG_BOOT_ITEM % 32))

#define IS_DISABLE_BCAST_DISCOVER(x)    (((x) & BIT (0)) == BIT (0))
#define IS_DISABLE_MCAST_DISCOVER(x)    (((x) & BIT (1)) == BIT (1))
#define IS_ENABLE_USE_SERVER_LIST(x)    (((x) & BIT (2)) == BIT (2))
#define IS_ENABLE_BOOT_FILE_NAME(x)     (((x) & BIT (3)) == BIT (3))

#define SET_VENDOR_OPTION_BIT_MAP(x, y) (((x)[(y) / 32]) = (UINT32) ((x)[(y) / 32]) | BIT ((y) % 32))

#pragma pack(1)
typedef struct {
  UINT8 ParaList[135];
} PXEBC_DHCP4_OPTION_PARA;

typedef struct {
  UINT16  Size;
} PXEBC_DHCP4_OPTION_MAX_MESG_SIZE;

typedef struct {
  UINT8 Type;
  UINT8 MajorVer;
  UINT8 MinorVer;
} PXEBC_DHCP4_OPTION_UNDI;

typedef struct {
  UINT8 Type;
} PXEBC_DHCP4_OPTION_MESG;

typedef struct {
  UINT16  Type;
} PXEBC_DHCP4_OPTION_ARCH;

#define DEFAULT_CLASS_ID_DATA "PXEClient:Arch:xxxxx:UNDI:003000"
#define DEFAULT_UNDI_TYPE     1
#define DEFAULT_UNDI_MAJOR    3
#define DEFAULT_UNDI_MINOR    0

typedef struct {
  UINT8 ClassIdentifier[10];
  UINT8 ArchitecturePrefix[5];
  UINT8 ArchitectureType[5];
  UINT8 Lit3[1];
  UINT8 InterfaceName[4];
  UINT8 Lit4[1];
  UINT8 UndiMajor[3];
  UINT8 UndiMinor[3];
} PXEBC_DHCP4_OPTION_CLID;

typedef struct {
  UINT8 Type;
  UINT8 Guid[16];
} PXEBC_DHCP4_OPTION_UUID;

typedef struct {
  UINT16  Type;
  UINT16  Layer;
} PXEBC_OPTION_BOOT_ITEM;

#pragma pack()

typedef union {
  PXEBC_DHCP4_OPTION_PARA           *Para;
  PXEBC_DHCP4_OPTION_UNDI           *Undi;
  PXEBC_DHCP4_OPTION_ARCH           *Arch;
  PXEBC_DHCP4_OPTION_CLID           *Clid;
  PXEBC_DHCP4_OPTION_UUID           *Uuid;
  PXEBC_DHCP4_OPTION_MESG           *Mesg;
  PXEBC_DHCP4_OPTION_MAX_MESG_SIZE  *MaxMesgSize;
} PXEBC_DHCP4_OPTION_ENTRY;

typedef struct {
  UINT16            Type;
  UINT8             IpCnt;
  EFI_IPv4_ADDRESS  IpAddr[1];
} PXEBC_BOOT_SVR_ENTRY;

typedef struct {
  UINT16  Type;
  UINT8   DescLen;
  UINT8   DescStr[1];
} PXEBC_BOOT_MENU_ENTRY;

typedef struct {
  UINT8 Timeout;
  UINT8 Prompt[1];
} PXEBC_MENU_PROMPT;

typedef struct {
  UINT32                BitMap[8];
  EFI_IPv4_ADDRESS      MtftpIp;
  UINT16                MtftpCPort;
  UINT16                MtftpSPort;
  UINT8                 MtftpTimeout;
  UINT8                 MtftpDelay;
  UINT8                 DiscoverCtrl;
  EFI_IPv4_ADDRESS      DiscoverMcastIp;
  EFI_IPv4_ADDRESS      McastIpBase;
  UINT16                McastIpBlock;
  UINT16                McastIpRange;
  UINT16                BootSrvType;
  UINT16                BootSrvLayer;
  PXEBC_BOOT_SVR_ENTRY  *BootSvr;
  UINT8                 BootSvrLen;
  PXEBC_BOOT_MENU_ENTRY *BootMenu;
  UINT8                 BootMenuLen;
  PXEBC_MENU_PROMPT     *MenuPrompt;
  UINT8                 MenuPromptLen;
  UINT32                *CredType;
  UINT8                 CredTypeLen;
} PXEBC_VENDOR_OPTION;

#define PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE  (OFFSET_OF (EFI_DHCP4_PACKET, Dhcp4) + PXEBC_DHCP4_MAX_PACKET_SIZE)

typedef union {
  EFI_DHCP4_PACKET  Offer;
  EFI_DHCP4_PACKET  Ack;
  UINT8             Buffer[PXEBC_CACHED_DHCP4_PACKET_MAX_SIZE];
} PXEBC_DHCP4_PACKET;

typedef struct {
  PXEBC_DHCP4_PACKET      Packet;
  BOOLEAN                 IsPxeOffer;
  UINT8                   OfferType;
  EFI_DHCP4_PACKET_OPTION *Dhcp4Option[PXEBC_DHCP4_TAG_INDEX_MAX];
  PXEBC_VENDOR_OPTION    PxeVendorOption;
} PXEBC_CACHED_DHCP4_PACKET;

#define GET_NEXT_DHCP_OPTION(Opt) \
  (EFI_DHCP4_PACKET_OPTION *) ((UINT8 *) (Opt) + sizeof (EFI_DHCP4_PACKET_OPTION) + (Opt)->Length - 1)

#define GET_OPTION_BUFFER_LEN(Pkt)  ((Pkt)->Length - sizeof (EFI_DHCP4_HEADER) - 4)
#define IS_PROXY_DHCP_OFFER(Offer)  EFI_IP4_EQUAL (&((Offer)->Dhcp4.Header.YourAddr), &mZeroIp4Addr)

#define GET_NEXT_BOOT_SVR_ENTRY(Ent) \
  (PXEBC_BOOT_SVR_ENTRY *) ((UINT8 *) Ent + sizeof (*(Ent)) + ((Ent)->IpCnt - 1) * sizeof (EFI_IPv4_ADDRESS))


/**
  This function initialize the DHCP4 message instance.

  This function will pad each item of dhcp4 message packet.
  
  @param  Seed    Pointer to the message instance of the DHCP4 packet.
  @param  Udp4    Pointer to the EFI_UDP4_PROTOCOL instance.

**/
VOID
PxeBcInitSeedPacket (
  IN EFI_DHCP4_PACKET  *Seed,
  IN EFI_UDP4_PROTOCOL *Udp4
  );


/**
  Parse the cached dhcp packet.

  @param  CachedPacket  Pointer to cached dhcp packet.

  @retval TRUE          Succeed to parse and validation.
  @retval FALSE         Fail to parse or validation.

**/
BOOLEAN
PxeBcParseCachedDhcpPacket (
  IN PXEBC_CACHED_DHCP4_PACKET  *CachedPacket
  );

/**
  This function is to check the selected proxy offer (include BINL dhcp offer and
  DHCP_ONLY offer ) and set the flag and copy the DHCP packets to the Pxe base code
  mode structure.

  @param  Private          Pointer to PxeBc private data.

  @retval EFI_SUCCESS      Operational successful.
  @retval EFI_NO_RESPONSE  Offer dhcp service failed.

**/
EFI_STATUS
PxeBcCheckSelectedOffer (
  IN PXEBC_PRIVATE_DATA  *Private
  );


/**
  Callback routine.
  
  EFI_DHCP4_CALLBACK is provided by the consumer of the EFI DHCPv4 Protocol driver
  to intercept events that occurred in the configuration process. This structure
  provides advanced control of each state transition of the DHCP process. The
  returned status code determines the behavior of the EFI DHCPv4 Protocol driver.
  There are three possible returned values, which are described in the following
  table.

  @param  This                  Pointer to the EFI DHCPv4 Protocol instance that is used to
                                configure this callback function.
  @param  Context               Pointer to the context that is initialized by
                                EFI_DHCP4_PROTOCOL.Configure().
  @param  CurrentState          The current operational state of the EFI DHCPv4 Protocol
                                driver.
  @param  Dhcp4Event            The event that occurs in the current state, which usually means a
                                state transition.
  @param  Packet                The DHCP packet that is going to be sent or already received.
  @param  NewPacket             The packet that is used to replace the above Packet.

  @retval EFI_SUCCESS           Tells the EFI DHCPv4 Protocol driver to continue the DHCP process.
  @retval EFI_NOT_READY         Only used in the Dhcp4Selecting state. The EFI DHCPv4 Protocol
                                driver will continue to wait for more DHCPOFFER packets until the retry
                                timeout expires.
  @retval EFI_ABORTED           Tells the EFI DHCPv4 Protocol driver to abort the current process and
                                return to the Dhcp4Init or Dhcp4InitReboot state.

**/
EFI_STATUS
EFIAPI
PxeBcDhcpCallBack (
  IN EFI_DHCP4_PROTOCOL                * This,
  IN VOID                              *Context,
  IN EFI_DHCP4_STATE                   CurrentState,
  IN EFI_DHCP4_EVENT                   Dhcp4Event,
  IN EFI_DHCP4_PACKET                  * Packet OPTIONAL,
  OUT EFI_DHCP4_PACKET                 **NewPacket OPTIONAL
  );


/**
  Discover the boot of service and initialize the vendor option if exists.

  @param  Private               Pointer to PxeBc private data.
  @param  Type                  PxeBc option boot item type
  @param  Layer                 PxeBc option boot item layer
  @param  UseBis                Use BIS or not
  @param  DestIp                Ip address for server      
  @param  IpCount               The total count of the server ip address    
  @param  SrvList               Server list
  @param  IsDiscv               Discover the vendor or not
  @param  Reply                 The dhcp4 packet of Pxe reply

  @retval EFI_SUCCESS           Operation succeeds.
  @retval EFI_OUT_OF_RESOURCES  Allocate memory pool failed.
  @retval EFI_NOT_FOUND         There is no vendor option exists.
  @retval EFI_TIMEOUT           Send Pxe Discover time out. 
  
**/
EFI_STATUS
PxeBcDiscvBootService (
  IN PXEBC_PRIVATE_DATA                * Private,
  IN UINT16                            Type,
  IN UINT16                            *Layer,
  IN BOOLEAN                           UseBis,
  IN EFI_IP_ADDRESS                    * DestIp,
  IN UINT16                            IpCount,
  IN EFI_PXE_BASE_CODE_SRVLIST         * SrvList,
  IN BOOLEAN                           IsDiscv,
  OUT EFI_DHCP4_PACKET                 * Reply OPTIONAL
  );


/**
  Initialize the DHCP options and build the option list.

  @param  Private          Pointer to PxeBc private data.
  @param  OptList          Pointer to a DHCP option list.
                           
  @param  IsDhcpDiscover   Discover dhcp option or not.     

  @return The index item number of the option list.

**/
UINT32
PxeBcBuildDhcpOptions (
  IN PXEBC_PRIVATE_DATA            *Private,
  IN EFI_DHCP4_PACKET_OPTION       **OptList,
  IN BOOLEAN                       IsDhcpDiscover
  );


/**
  Create the boot options.

  @param  OptList    Pointer to the list of the options
  @param  Type       the type of option
  @param  Layer      the layer of the boot options 
  @param  OptLen     length of opotion

**/
VOID
PxeBcCreateBootOptions (
  IN  EFI_DHCP4_PACKET_OPTION          *OptList,
  IN  UINT16                           Type,
  IN  UINT16                           *Layer,
  OUT UINT32                           *OptLen
  );


/**
  Parse interested dhcp options.

  @param  Buffer     Pointer to the dhcp options packet.
  @param  Length     The length of the dhcp options.
  @param  OptTag     The option OpCode.

  @return NULL if the buffer length is 0 and OpCode is not 
          PXEBC_DHCP4_TAG_EOP, or the pointer to the buffer.

**/
EFI_DHCP4_PACKET_OPTION *
PxeBcParseExtendOptions (
  IN UINT8                         *Buffer,
  IN UINT32                        Length,
  IN UINT8                         OptTag
  );


/**
  This function is to parse and check vendor options.

  @param  Dhcp4Option           Pointer to dhcp options
  @param  VendorOption          Pointer to vendor options

  @return TRUE if valid for vendor options, or FALSE.

**/
BOOLEAN
PxeBcParseVendorOptions (
  IN EFI_DHCP4_PACKET_OPTION       *Dhcp4Option,
  IN PXEBC_VENDOR_OPTION           *VendorOption
  );


/**
  Choose the boot prompt.

  @param  Private              Pointer to PxeBc private data.

  @retval EFI_SUCCESS          Select boot prompt done.
  @retval EFI_TIMEOUT          Select boot prompt time out. 
  @retval EFI_NOT_FOUND        The proxy offer is not Pxe10.
  @retval EFI_ABORTED          User cancel the operation.
  @retval EFI_NOT_READY        Read the input key from the keybroad has not finish.
  
**/
EFI_STATUS
PxeBcSelectBootPrompt (
  IN PXEBC_PRIVATE_DATA              *Private
  );


/**
  Select the boot menu.

  @param  Private         Pointer to PxeBc private data.
  @param  Type            The type of the menu.
  @param  UseDefaultItem  Use default item or not.
  
  @retval EFI_ABORTED     User cancel operation.
  @retval EFI_SUCCESS     Select the boot menu success.
  @retval EFI_NOT_READY   Read the input key from the keybroad has not finish.    

**/
EFI_STATUS
PxeBcSelectBootMenu (
  IN  PXEBC_PRIVATE_DATA              *Private,
  OUT UINT16                          *Type,
  IN  BOOLEAN                         UseDefaultItem
  );

#endif

