/** @file
  EFI PXE Base Code Protocol definitions, which is used to access PXE-compatible 
  devices for network access and network booting.

  Copyright (c) 2006 - 2008, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/
#ifndef __PXE_BASE_CODE_PROTOCOL_H__
#define __PXE_BASE_CODE_PROTOCOL_H__

//
// PXE Base Code protocol
//
#define EFI_PXE_BASE_CODE_PROTOCOL_GUID \
  { \
    0x03c4e603, 0xac28, 0x11d3, {0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

typedef struct _EFI_PXE_BASE_CODE_PROTOCOL  EFI_PXE_BASE_CODE_PROTOCOL;

//
// Protocol defined in EFI1.1.
// 
typedef EFI_PXE_BASE_CODE_PROTOCOL  EFI_PXE_BASE_CODE;

//
// Default IP TTL and ToS.
//
#define DEFAULT_TTL 16
#define DEFAULT_ToS 0

//
// ICMP error format
//
typedef struct {
  UINT8   Type;
  UINT8   Code;
  UINT16  Checksum;
  union {
    UINT32  reserved;
    UINT32  Mtu;
    UINT32  Pointer;
    struct {
      UINT16  Identifier;
      UINT16  Sequence;
    } Echo;
  } u;
  UINT8 Data[494];
} EFI_PXE_BASE_CODE_ICMP_ERROR;

//
// TFTP error format
//
typedef struct {
  UINT8 ErrorCode;
  CHAR8 ErrorString[127];
} EFI_PXE_BASE_CODE_TFTP_ERROR;

//
// IP Receive Filter definitions
//
#define EFI_PXE_BASE_CODE_MAX_IPCNT 8

typedef struct {
  UINT8           Filters;
  UINT8           IpCnt;
  UINT16          reserved;
  EFI_IP_ADDRESS  IpList[EFI_PXE_BASE_CODE_MAX_IPCNT];
} EFI_PXE_BASE_CODE_IP_FILTER;

#define EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP            0x0001
#define EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST             0x0002
#define EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS           0x0004
#define EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS_MULTICAST 0x0008

//
// ARP Cache definitions
//
typedef struct {
  EFI_IP_ADDRESS  IpAddr;
  EFI_MAC_ADDRESS MacAddr;
} EFI_PXE_BASE_CODE_ARP_ENTRY;

typedef struct {
  EFI_IP_ADDRESS  IpAddr;
  EFI_IP_ADDRESS  SubnetMask;
  EFI_IP_ADDRESS  GwAddr;
} EFI_PXE_BASE_CODE_ROUTE_ENTRY;

//
// UDP definitions
//
typedef UINT16  EFI_PXE_BASE_CODE_UDP_PORT;

#define EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP    0x0001
#define EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT  0x0002
#define EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_IP   0x0004
#define EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_PORT 0x0008
#define EFI_PXE_BASE_CODE_UDP_OPFLAGS_USE_FILTER    0x0010
#define EFI_PXE_BASE_CODE_UDP_OPFLAGS_MAY_FRAGMENT  0x0020

//
// Discover() definitions
//
#define EFI_PXE_BASE_CODE_BOOT_TYPE_BOOTSTRAP         0
#define EFI_PXE_BASE_CODE_BOOT_TYPE_MS_WINNT_RIS      1
#define EFI_PXE_BASE_CODE_BOOT_TYPE_INTEL_LCM         2
#define EFI_PXE_BASE_CODE_BOOT_TYPE_DOSUNDI           3
#define EFI_PXE_BASE_CODE_BOOT_TYPE_NEC_ESMPRO        4
#define EFI_PXE_BASE_CODE_BOOT_TYPE_IBM_WSoD          5
#define EFI_PXE_BASE_CODE_BOOT_TYPE_IBM_LCCM          6
#define EFI_PXE_BASE_CODE_BOOT_TYPE_CA_UNICENTER_TNG  7
#define EFI_PXE_BASE_CODE_BOOT_TYPE_HP_OPENVIEW       8
#define EFI_PXE_BASE_CODE_BOOT_TYPE_ALTIRIS_9         9
#define EFI_PXE_BASE_CODE_BOOT_TYPE_ALTIRIS_10        10
#define EFI_PXE_BASE_CODE_BOOT_TYPE_ALTIRIS_11        11
#define EFI_PXE_BASE_CODE_BOOT_TYPE_NOT_USED_12       12
#define EFI_PXE_BASE_CODE_BOOT_TYPE_REDHAT_INSTALL    13
#define EFI_PXE_BASE_CODE_BOOT_TYPE_REDHAT_BOOT       14
#define EFI_PXE_BASE_CODE_BOOT_TYPE_REMBO             15
#define EFI_PXE_BASE_CODE_BOOT_TYPE_BEOBOOT           16
//
// 17 through 32767 are reserved
// 32768 through 65279 are for vendor use
// 65280 through 65534 are reserved
//
#define EFI_PXE_BASE_CODE_BOOT_TYPE_PXETEST   65535

#define EFI_PXE_BASE_CODE_BOOT_LAYER_MASK     0x7FFF
#define EFI_PXE_BASE_CODE_BOOT_LAYER_INITIAL  0x0000

//
// Discover() server list structure.
//
typedef struct {
  UINT16          Type;
  BOOLEAN         AcceptAnyResponse;
  UINT8           Reserved;
  EFI_IP_ADDRESS  IpAddr;
} EFI_PXE_BASE_CODE_SRVLIST;

//
// Discover() information override structure.
//
typedef struct {
  BOOLEAN                   UseMCast;
  BOOLEAN                   UseBCast;
  BOOLEAN                   UseUCast;
  BOOLEAN                   MustUseList;
  EFI_IP_ADDRESS            ServerMCastIp;
  UINT16                    IpCnt;
  EFI_PXE_BASE_CODE_SRVLIST SrvList[1];
} EFI_PXE_BASE_CODE_DISCOVER_INFO;

//
// Mtftp() definitions
//
typedef enum {
  EFI_PXE_BASE_CODE_TFTP_FIRST,
  EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,
  EFI_PXE_BASE_CODE_TFTP_READ_FILE,
  EFI_PXE_BASE_CODE_TFTP_WRITE_FILE,
  EFI_PXE_BASE_CODE_TFTP_READ_DIRECTORY,
  EFI_PXE_BASE_CODE_MTFTP_GET_FILE_SIZE,
  EFI_PXE_BASE_CODE_MTFTP_READ_FILE,
  EFI_PXE_BASE_CODE_MTFTP_READ_DIRECTORY,
  EFI_PXE_BASE_CODE_MTFTP_LAST
} EFI_PXE_BASE_CODE_TFTP_OPCODE;

typedef struct {
  EFI_IP_ADDRESS              MCastIp;
  EFI_PXE_BASE_CODE_UDP_PORT  CPort;
  EFI_PXE_BASE_CODE_UDP_PORT  SPort;
  UINT16                      ListenTimeout;
  UINT16                      TransmitTimeout;
} EFI_PXE_BASE_CODE_MTFTP_INFO;

//
// PXE Base Code Mode structure
//
#define EFI_PXE_BASE_CODE_MAX_ARP_ENTRIES   8
#define EFI_PXE_BASE_CODE_MAX_ROUTE_ENTRIES 8

typedef struct {
  BOOLEAN                       Started;
  BOOLEAN                       Ipv6Available;
  BOOLEAN                       Ipv6Supported;
  BOOLEAN                       UsingIpv6;
  BOOLEAN                       BisSupported;
  BOOLEAN                       BisDetected;
  BOOLEAN                       AutoArp;
  BOOLEAN                       SendGUID;
  BOOLEAN                       DhcpDiscoverValid;
  BOOLEAN                       DhcpAckReceived;
  BOOLEAN                       ProxyOfferReceived;
  BOOLEAN                       PxeDiscoverValid;
  BOOLEAN                       PxeReplyReceived;
  BOOLEAN                       PxeBisReplyReceived;
  BOOLEAN                       IcmpErrorReceived;
  BOOLEAN                       TftpErrorReceived;
  BOOLEAN                       MakeCallbacks;
  UINT8                         TTL;
  UINT8                         ToS;
  EFI_IP_ADDRESS                StationIp;
  EFI_IP_ADDRESS                SubnetMask;
  EFI_PXE_BASE_CODE_PACKET      DhcpDiscover;
  EFI_PXE_BASE_CODE_PACKET      DhcpAck;
  EFI_PXE_BASE_CODE_PACKET      ProxyOffer;
  EFI_PXE_BASE_CODE_PACKET      PxeDiscover;
  EFI_PXE_BASE_CODE_PACKET      PxeReply;
  EFI_PXE_BASE_CODE_PACKET      PxeBisReply;
  EFI_PXE_BASE_CODE_IP_FILTER   IpFilter;
  UINT32                        ArpCacheEntries;
  EFI_PXE_BASE_CODE_ARP_ENTRY   ArpCache[EFI_PXE_BASE_CODE_MAX_ARP_ENTRIES];
  UINT32                        RouteTableEntries;
  EFI_PXE_BASE_CODE_ROUTE_ENTRY RouteTable[EFI_PXE_BASE_CODE_MAX_ROUTE_ENTRIES];
  EFI_PXE_BASE_CODE_ICMP_ERROR  IcmpError;
  EFI_PXE_BASE_CODE_TFTP_ERROR  TftpError;
} EFI_PXE_BASE_CODE_MODE;

//
// PXE Base Code Interface Function definitions
//

/**                                                                 
  Enables the use of the PXE Base Code Protocol functions.
    
  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  UseIpv6               Specifies the type of IP addresses that are to be used during the session
                                that is being started. Set to TRUE for IPv6 addresses, and FALSE for     
                                IPv4 addresses.                                                                                                   
                                
  @retval EFI_SUCCESS           The PXE Base Code Protocol was started.
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this oper  
  @retval EFI_UNSUPPORTED       UseIpv6 is TRUE, but the Ipv6Supported field of the
                                EFI_PXE_BASE_CODE_MODE structure is FALSE.  
  @retval EFI_ALREADY_STARTED   The PXE Base Code Protocol is already in the started state.                                   
  @retval EFI_INVALID_PARAMETER The This parameter is NULL or does not point to a valid
                                EFI_PXE_BASE_CODE_PROTOCOL structure.      
  @retval EFI_OUT_OF_RESOURCES  Could not allocate enough memory or other resources to start the                                          
                                PXE Base Code Protocol.                                         
                                     
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_START)(
  IN EFI_PXE_BASE_CODE_PROTOCOL            *This,
  IN BOOLEAN                               UseIpv6
  );

/**                                                                 
  Disables the use of the PXE Base Code Protocol functions.
    
  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
 
  @retval EFI_SUCCESS           The PXE Base Code Protocol was stopped.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is already in the stopped state.  
  @retval EFI_INVALID_PARAMETER The This parameter is NULL or does not point to a valid
                                EFI_PXE_BASE_CODE_PROTOCOL structure.                  
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.                                
                                     
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_STOP)(
  IN EFI_PXE_BASE_CODE_PROTOCOL    *This
  );

/**                                                                 
  Attempts to complete a DHCPv4 D.O.R.A. (discover / offer / request / acknowledge) or DHCPv6
  S.A.R.R (solicit / advertise / request / reply) sequence.
    
  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  SortOffers            TRUE if the offers received should be sorted. Set to FALSE to try the
                                offers in the order that they are received.                          
 
  @retval EFI_SUCCESS           Valid DHCP has completed.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER The This parameter is NULL or does not point to a valid
                                EFI_PXE_BASE_CODE_PROTOCOL structure.                  
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.                                
  @retval EFI_OUT_OF_RESOURCES  Could not allocate enough memory to complete the DHCP Protocol.
  @retval EFI_ABORTED           The callback function aborted the DHCP Protocol.
  @retval EFI_TIMEOUT           The DHCP Protocol timed out.
  @retval EFI_ICMP_ERROR        An ICMP error packet was received during the DHCP session.
  @retval EFI_NO_RESPONSE       Valid PXE offer was not received.
                                     
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_DHCP)(
  IN EFI_PXE_BASE_CODE_PROTOCOL            *This,
  IN BOOLEAN                               SortOffers
  );

/**                                                                 
  Attempts to complete the PXE Boot Server and/or boot image discovery sequence.
    
  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  Type                  The type of bootstrap to perform.
  @param  Layer                 Pointer to the boot server layer number to discover, which must be
                                PXE_BOOT_LAYER_INITIAL when a new server type is being            
                                discovered.                                                       
  @param  UseBis                TRUE if Boot Integrity Services are to be used. FALSE otherwise.                                
  @param  Info                  Pointer to a data structure that contains additional information on the
                                type of discovery operation that is to be performed.                   
                                  
  @retval EFI_SUCCESS           The Discovery sequence has been completed.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                                
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.                                
  @retval EFI_OUT_OF_RESOURCES  Could not allocate enough memory to complete Discovery.
  @retval EFI_ABORTED           The callback function aborted the Discovery sequence.
  @retval EFI_TIMEOUT           The Discovery sequence timed out.
  @retval EFI_ICMP_ERROR        An ICMP error packet was received during the PXE discovery
                                session.                                                  
                                       
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_DISCOVER)(
  IN EFI_PXE_BASE_CODE_PROTOCOL           *This,
  IN UINT16                               Type,
  IN UINT16                               *Layer,
  IN BOOLEAN                              UseBis,
  IN EFI_PXE_BASE_CODE_DISCOVER_INFO      *Info   OPTIONAL
  );

/**                                                                 
  Used to perform TFTP and MTFTP services.
    
  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  Operation             The type of operation to perform.
  @param  BufferPtr             A pointer to the data buffer.                                                                     
  @param  Overwrite             Only used on write file operations. TRUE if a file on a remote server can
                                be overwritten.                                                          
  @param  BufferSize            For get-file-size operations, *BufferSize returns the size of the
                                requested file.                                                  
  @param  BlockSize             The requested block size to be used during a TFTP transfer.
  @param  ServerIp              The TFTP / MTFTP server IP address.
  @param  Filename              A Null-terminated ASCII string that specifies a directory name or a file
                                name.                                                                   
  @param  Info                  Pointer to the MTFTP information.
  @param  DontUseBuffer         Set to FALSE for normal TFTP and MTFTP read file operation.                       
                                  
  @retval EFI_SUCCESS           The TFTP/MTFTP operation was completed.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                                
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.                                
  @retval EFI_BUFFER_TOO_SMALL  The buffer is not large enough to complete the read operation.   
  @retval EFI_ABORTED           The callback function aborted the TFTP/MTFTP operation.
  @retval EFI_TIMEOUT           The TFTP/MTFTP operation timed out.
  @retval EFI_ICMP_ERROR        An ICMP error packet was received during the MTFTP session.
  @retval EFI_TFTP_ERROR        A TFTP error packet was received during the MTFTP session.
                                                                      
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_MTFTP)(
  IN EFI_PXE_BASE_CODE_PROTOCOL                *This,
  IN EFI_PXE_BASE_CODE_TFTP_OPCODE             Operation,
  IN OUT VOID                                  *BufferPtr OPTIONAL,
  IN BOOLEAN                                   Overwrite,
  IN OUT UINT64                                *BufferSize,
  IN UINTN                                     *BlockSize OPTIONAL,
  IN EFI_IP_ADDRESS                            *ServerIp,
  IN UINT8                                     *Filename  OPTIONAL,
  IN EFI_PXE_BASE_CODE_MTFTP_INFO              *Info      OPTIONAL,
  IN BOOLEAN                                   DontUseBuffer
  );

/**                                                                 
  Writes a UDP packet to the network interface.
    
  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  OpFlags               The UDP operation flags. 
  @param  DestIp                The destination IP address.
  @param  DestPort              The destination UDP port number.                                                                         
  @param  GatewayIp             The gateway IP address.                                                 
  @param  SrcIp                 The source IP address.
  @param  SrcPort               The source UDP port number.
  @param  HeaderSize            An optional field which may be set to the length of a header at
                                HeaderPtr to be prefixed to the data at BufferPtr.             
  @param  HeaderPtr             If HeaderSize is not NULL, a pointer to a header to be prefixed to the
                                data at BufferPtr.                                                    
  @param  BufferSize            A pointer to the size of the data at BufferPtr.
  @param  BufferPtr             A pointer to the data to be written.
                                  
  @retval EFI_SUCCESS           The UDP Write operation was completed.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                                
  @retval EFI_BAD_BUFFER_SIZE   The buffer is too long to be transmitted.  
  @retval EFI_ABORTED           The callback function aborted the UDP Write operation.
  @retval EFI_TIMEOUT           The UDP Write operation timed out.
  @retval EFI_ICMP_ERROR        An ICMP error packet was received during the UDP write session.  
                                                                      
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_UDP_WRITE)(
  IN EFI_PXE_BASE_CODE_PROTOCOL                *This,
  IN UINT16                                    OpFlags,
  IN EFI_IP_ADDRESS                            *DestIp,
  IN EFI_PXE_BASE_CODE_UDP_PORT                *DestPort,
  IN EFI_IP_ADDRESS                            *GatewayIp,  OPTIONAL
  IN EFI_IP_ADDRESS                            *SrcIp,      OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT            *SrcPort,    OPTIONAL
  IN UINTN                                     *HeaderSize, OPTIONAL
  IN VOID                                      *HeaderPtr,  OPTIONAL
  IN UINTN                                     *BufferSize,
  IN VOID                                      *BufferPtr
  );

/**                                                                 
  Reads a UDP packet from the network interface.
    
  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  OpFlags               The UDP operation flags. 
  @param  DestIp                The destination IP address.
  @param  DestPort              The destination UDP port number.                                                                         
  @param  GatewayIp             The gateway IP address.                                                 
  @param  SrcIp                 The source IP address.
  @param  SrcPort               The source UDP port number.
  @param  HeaderSize            An optional field which may be set to the length of a header at
                                HeaderPtr to be prefixed to the data at BufferPtr.             
  @param  HeaderPtr             If HeaderSize is not NULL, a pointer to a header to be prefixed to the
                                data at BufferPtr.                                                    
  @param  BufferSize            A pointer to the size of the data at BufferPtr.
  @param  BufferPtr             A pointer to the data to be read.
                                  
  @retval EFI_SUCCESS           The UDP Write operation was completed.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                                
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.
  @retval EFI_BUFFER_TOO_SMALL  The packet is larger than Buffer can hold.
  @retval EFI_ABORTED           The callback function aborted the UDP Read operation.
  @retval EFI_TIMEOUT           The UDP Read operation timed out.  
                                                                      
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_UDP_READ)(
  IN EFI_PXE_BASE_CODE_PROTOCOL                *This,
  IN UINT16                                    OpFlags,
  IN OUT EFI_IP_ADDRESS                        *DestIp,     OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT            *DestPort,   OPTIONAL
  IN OUT EFI_IP_ADDRESS                        *SrcIp,      OPTIONAL
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT            *SrcPort,    OPTIONAL
  IN UINTN                                     *HeaderSize, OPTIONAL
  IN VOID                                      *HeaderPtr,  OPTIONAL
  IN OUT UINTN                                 *BufferSize,
  IN VOID                                      *BufferPtr
  );

/**                                                                 
  Updates the IP receive filters of a network device and enables software filtering.
    
  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  NewFilter             Pointer to the new set of IP receive filters.
  
  @retval EFI_SUCCESS           The IP receive filter settings were updated.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                                  
                                                                      
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_SET_IP_FILTER)(
  IN EFI_PXE_BASE_CODE_PROTOCOL            *This,
  IN EFI_PXE_BASE_CODE_IP_FILTER           *NewFilter
  );

/**                                                                 
  Uses the ARP protocol to resolve a MAC address.
    
  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  IpAddr                Pointer to the IP address that is used to resolve a MAC address.
  @param  MacAddr               If not NULL, a pointer to the MAC address that was resolved with the
                                ARP protocol.                                                       
                                  
  @retval EFI_SUCCESS           The IP or MAC address was resolved.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                                
  @retval EFI_DEVICE_ERROR      The network device encountered an error during this operation.  
  @retval EFI_ABORTED           The callback function aborted the ARP Protocol.
  @retval EFI_TIMEOUT           The ARP Protocol encountered a timeout condition.
                                                                      
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_ARP)(
  IN EFI_PXE_BASE_CODE_PROTOCOL            *This,
  IN EFI_IP_ADDRESS                        *IpAddr,
  IN EFI_MAC_ADDRESS                       *MacAddr OPTIONAL
  );

/**                                                                 
  Updates the parameters that affect the operation of the PXE Base Code Protocol.
    
  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  NewAutoArp            If not NULL, a pointer to a value that specifies whether to replace the
                                current value of AutoARP.                                              
  @param  NewSendGUID           If not NULL, a pointer to a value that specifies whether to replace the
                                current value of SendGUID.                                             
  @param  NewTTL                If not NULL, a pointer to be used in place of the current value of TTL,
                                the "time to live" field of the IP header.                           
  @param  NewToS                If not NULL, a pointer to be used in place of the current value of ToS,
                                the "type of service" field of the IP header.                        
  @param  NewMakeCallback       If not NULL, a pointer to a value that specifies whether to replace the
                                current value of the MakeCallback field of the Mode structure.                                                                
                                  
  @retval EFI_SUCCESS           The new parameters values were updated.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                                  
                                                                      
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_SET_PARAMETERS)(
  IN EFI_PXE_BASE_CODE_PROTOCOL            *This,
  IN BOOLEAN                               *NewAutoArp,     OPTIONAL
  IN BOOLEAN                               *NewSendGUID,    OPTIONAL
  IN UINT8                                 *NewTTL,         OPTIONAL
  IN UINT8                                 *NewToS,         OPTIONAL
  IN BOOLEAN                               *NewMakeCallback OPTIONAL
  );

/**                                                                 
  Updates the station IP address and/or subnet mask values of a network device.
    
  @param  This                  Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  NewStationIp          Pointer to the new IP address to be used by the network device.  
  @param  NewSubnetMask         Pointer to the new subnet mask to be used by the network device.                                 
                                  
  @retval EFI_SUCCESS           The new station IP address and/or subnet mask were updated.
  @retval EFI_NOT_STARTED       The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                                  
                                                                      
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_SET_STATION_IP)(
  IN EFI_PXE_BASE_CODE_PROTOCOL            *This,
  IN EFI_IP_ADDRESS                        *NewStationIp,   OPTIONAL
  IN EFI_IP_ADDRESS                        *NewSubnetMask   OPTIONAL
  );

/**                                                                 
  Updates the contents of the cached DHCP and Discover packets.
    
  @param  This                   Pointer to the EFI_PXE_BASE_CODE_PROTOCOL instance.
  @param  NewDhcpDiscoverValid   Pointer to a value that will replace the current
                                 DhcpDiscoverValid field.                        
  @param  NewDhcpAckReceived     Pointer to a value that will replace the current
                                 DhcpAckReceived field.                          
  @param  NewProxyOfferReceived  Pointer to a value that will replace the current
                                 ProxyOfferReceived field.                       
  @param  NewPxeDiscoverValid    Pointer to a value that will replace the current     
                                 ProxyOfferReceived field.                       
  @param  NewPxeReplyReceived    Pointer to a value that will replace the current
                                 PxeReplyReceived field.                         
  @param  NewPxeBisReplyReceived Pointer to a value that will replace the current
                                 PxeBisReplyReceived field.                      
  @param  NewDhcpDiscover        Pointer to the new cached DHCP Discover packet contents.   
  @param  NewDhcpAck             Pointer to the new cached DHCP Ack packet contents.
  @param  NewProxyOffer          Pointer to the new cached Proxy Offer packet contents.
  @param  NewPxeDiscover         Pointer to the new cached PXE Discover packet contents.
  @param  NewPxeReply            Pointer to the new cached PXE Reply packet contents.
  @param  NewPxeBisReply         Pointer to the new cached PXE BIS Reply packet contents.
                                   
  @retval EFI_SUCCESS            The cached packet contents were updated.
  @retval EFI_NOT_STARTED        The PXE Base Code Protocol is in the stopped state.
  @retval EFI_INVALID_PARAMETER  This is NULL or not point to a valid EFI_PXE_BASE_CODE_PROTOCOL structure.
                                                                      
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PXE_BASE_CODE_SET_PACKETS)(
  IN EFI_PXE_BASE_CODE_PROTOCOL            *This,
  BOOLEAN                                  *NewDhcpDiscoverValid,   OPTIONAL
  BOOLEAN                                  *NewDhcpAckReceived,     OPTIONAL
  BOOLEAN                                  *NewProxyOfferReceived,  OPTIONAL
  BOOLEAN                                  *NewPxeDiscoverValid,    OPTIONAL
  BOOLEAN                                  *NewPxeReplyReceived,    OPTIONAL
  BOOLEAN                                  *NewPxeBisReplyReceived, OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET              *NewDhcpDiscover,        OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET              *NewDhcpAck,             OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET              *NewProxyOffer,          OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET              *NewPxeDiscover,         OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET              *NewPxeReply,            OPTIONAL
  IN EFI_PXE_BASE_CODE_PACKET              *NewPxeBisReply          OPTIONAL
  );

//
// PXE Base Code Protocol structure
//
#define EFI_PXE_BASE_CODE_PROTOCOL_REVISION   0x00010000

//
// Revision defined in EFI1.1
// 
#define EFI_PXE_BASE_CODE_INTERFACE_REVISION  EFI_PXE_BASE_CODE_PROTOCOL_REVISION

/**  
  @par Protocol Description:
  The EFI_PXE_BASE_CODE_PROTOCOL is used to control PXE-compatible devices.
  An EFI_PXE_BASE_CODE_PROTOCOL will be layered on top of an
  EFI_MANAGED_NETWORK_PROTOCOL protocol in order to perform packet level transactions.
  The EFI_PXE_BASE_CODE_PROTOCOL handle also supports the
  EFI_LOAD_FILE_PROTOCOL protocol. This provides a clean way to obtain control from the
  boot manager if the boot path is from the remote device.

  @param Revision
  The revision of the EFI_PXE_BASE_CODE_PROTOCOL. All future revisions must 
  be backwards compatible. If a future version is not backwards compatible 
  it is not the same GUID.

  @param Start
  Starts the PXE Base Code Protocol. Mode structure information is not valid and 
  no other Base Code Protocol functions will operate until the Base Code is started. 

  @param Stop
  Stops the PXE Base Code Protocol. Mode structure information is unchanged by this function. 
  No Base Code Protocol functions will operate until the Base Code is restarted. 

  @param Dhcp
  Attempts to complete a DHCPv4 D.O.R.A. (discover / offer / request / acknowledge) 
  or DHCPv6 S.A.R.R (solicit / advertise / request / reply) sequence. 

  @param Discover
  Attempts to complete the PXE Boot Server and/or boot image discovery sequence. 

  @param Mtftp
  Performs TFTP and MTFTP services. 

  @param UdpWrite
  Writes a UDP packet to the network interface. 

  @param UdpRead
  Reads a UDP packet from the network interface. 

  @param SetIpFilter
  Updates the IP receive filters of the network device. 

  @param Arp
  Uses the ARP protocol to resolve a MAC address. 

  @param SetParameters
  Updates the parameters that affect the operation of the PXE Base Code Protocol. 

  @param SetStationIp
  Updates the station IP address and subnet mask values. 

  @param SetPackets
  Updates the contents of the cached DHCP and Discover packets. 

  @param Mode
  Pointer to the EFI_PXE_BASE_CODE_MODE data for this device. 

**/
struct _EFI_PXE_BASE_CODE_PROTOCOL {
  UINT64                            Revision;
  EFI_PXE_BASE_CODE_START           Start;
  EFI_PXE_BASE_CODE_STOP            Stop;
  EFI_PXE_BASE_CODE_DHCP            Dhcp;
  EFI_PXE_BASE_CODE_DISCOVER        Discover;
  EFI_PXE_BASE_CODE_MTFTP           Mtftp;
  EFI_PXE_BASE_CODE_UDP_WRITE       UdpWrite;
  EFI_PXE_BASE_CODE_UDP_READ        UdpRead;
  EFI_PXE_BASE_CODE_SET_IP_FILTER   SetIpFilter;
  EFI_PXE_BASE_CODE_ARP             Arp;
  EFI_PXE_BASE_CODE_SET_PARAMETERS  SetParameters;
  EFI_PXE_BASE_CODE_SET_STATION_IP  SetStationIp;
  EFI_PXE_BASE_CODE_SET_PACKETS     SetPackets;
  EFI_PXE_BASE_CODE_MODE            *Mode;
};

extern EFI_GUID gEfiPxeBaseCodeProtocolGuid;

#endif 
