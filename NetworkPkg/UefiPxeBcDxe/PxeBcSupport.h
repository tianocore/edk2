/** @file
  Support functions declaration for UefiPxeBc Driver.

  Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_PXEBC_SUPPORT_H__
#define __EFI_PXEBC_SUPPORT_H__


#define ICMP_DEST_UNREACHABLE      3
#define ICMP_SOURCE_QUENCH         4
#define ICMP_REDIRECT              5
#define ICMP_ECHO_REQUEST          8
#define ICMP_TIME_EXCEEDED         11
#define ICMP_PARAMETER_PROBLEM     12



/**
  Flush the previous configration using the new station Ip address.

  @param[in]   Private        Pointer to PxeBc private data.
  @param[in]   StationIp      Pointer to the station Ip address.
  @param[in]   SubnetMask     Pointer to the subnet mask address for v4.

  @retval EFI_SUCCESS         Successfully flushed the previous config.
  @retval Others              Failed to flush using the new station Ip.

**/
EFI_STATUS
PxeBcFlushStationIp (
  PXEBC_PRIVATE_DATA       *Private,
  EFI_IP_ADDRESS           *StationIp,
  EFI_IP_ADDRESS           *SubnetMask     OPTIONAL
  );


/**
  Notify callback function when an event is triggered.

  @param[in]  Event           The triggered event.
  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
PxeBcCommonNotify (
  IN EFI_EVENT           Event,
  IN VOID                *Context
  );


/**
  Perform arp resolution from the arp cache in PxeBcMode.

  @param  Mode           Pointer to EFI_PXE_BASE_CODE_MODE.
  @param  Ip4Addr        The Ip4 address for resolution.
  @param  MacAddress     The resoluted MAC address if the resolution is successful.
                         The value is undefined if resolution fails.

  @retval TRUE           Found a matched entry.
  @retval FALSE          Did not find a matched entry.

**/
BOOLEAN
PxeBcCheckArpCache (
  IN  EFI_PXE_BASE_CODE_MODE    *Mode,
  IN  EFI_IPv4_ADDRESS          *Ip4Addr,
  OUT EFI_MAC_ADDRESS           *MacAddress
  );


/**
  Update arp cache periodically.

  @param  Event              Pointer to EFI_PXE_BC_PROTOCOL.
  @param  Context            Context of the timer event.

**/
VOID
EFIAPI
PxeBcArpCacheUpdate (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  );


/**
  xxx

  @param  Event                 The event signaled.
  @param  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
PxeBcIcmpErrorUpdate (
  IN EFI_EVENT             Event,
  IN VOID                  *Context
  );


/**
  xxx

  @param  Event                 The event signaled.
  @param  Context               The context passed in by the event notifier.

**/
VOID
EFIAPI
PxeBcIcmp6ErrorUpdate (
  IN EFI_EVENT             Event,
  IN VOID                  *Context
  );


/**
  This function is to configure a UDPv4 instance for UdpWrite.

  @param[in]       Udp4                 Pointer to EFI_UDP4_PROTOCOL.
  @param[in]       StationIp            Pointer to the station address.
  @param[in]       SubnetMask           Pointer to the subnet mask.
  @param[in]       Gateway              Pointer to the gateway address.
  @param[in, out]  SrcPort              Pointer to the source port.
  @param[in]       DoNotFragment        The flag of DoNotFragment bit in the IPv4
                                        packet.

  @retval          EFI_SUCCESS          Successfully configured this instance.
  @retval          Others               Failed to configure this instance.

**/
EFI_STATUS
PxeBcConfigUdp4Write (
  IN     EFI_UDP4_PROTOCOL  *Udp4,
  IN     EFI_IPv4_ADDRESS   *StationIp,
  IN     EFI_IPv4_ADDRESS   *SubnetMask,
  IN     EFI_IPv4_ADDRESS   *Gateway,
  IN OUT UINT16             *SrcPort,
  IN     BOOLEAN            DoNotFragment
  );


/**
  This function is to configure a UDPv6 instance for UdpWrite.

  @param[in]       Udp6                 Pointer to EFI_UDP6_PROTOCOL.
  @param[in]       StationIp            Pointer to the station address.
  @param[in, out]  SrcPort              Pointer to the source port.

  @retval          EFI_SUCCESS          Successfuly configured this instance.
  @retval          Others               Failed to configure this instance.

**/
EFI_STATUS
PxeBcConfigUdp6Write (
  IN     EFI_UDP6_PROTOCOL  *Udp6,
  IN     EFI_IPv6_ADDRESS   *StationIp,
  IN OUT UINT16             *SrcPort
  );

/**
  This function is to configure a UDPv4 instance for UdpWrite.

  @param[in]       Udp4                 Pointer to EFI_UDP4_PROTOCOL.
  @param[in]       Session              Pointer to the UDP4 session data.
  @param[in]       TimeoutEvent         The event for timeout.
  @param[in]       Gateway              Pointer to the gateway address.
  @param[in]       HeaderSize           An optional field which may be set to the length of a header
                                        at HeaderPtr to be prefixed to the data at BufferPtr.
  @param[in]       HeaderPtr            If HeaderSize is not NULL, a pointer to a header to be
                                        prefixed to the data at BufferPtr.
  @param[in]       BufferSize           A pointer to the size of the data at BufferPtr.
  @param[in]       BufferPtr            A pointer to the data to be written.

  @retval          EFI_SUCCESS          Successfully sent out data with Udp4Write.
  @retval          Others               Failed to send out data.

**/
EFI_STATUS
PxeBcUdp4Write (
  IN EFI_UDP4_PROTOCOL       *Udp4,
  IN EFI_UDP4_SESSION_DATA   *Session,
  IN EFI_EVENT               TimeoutEvent,
  IN EFI_IPv4_ADDRESS        *Gateway      OPTIONAL,
  IN UINTN                   *HeaderSize   OPTIONAL,
  IN VOID                    *HeaderPtr    OPTIONAL,
  IN UINTN                   *BufferSize,
  IN VOID                    *BufferPtr
  );


/**
  This function is to configure a UDPv6 instance for UdpWrite.

  @param[in]       Udp6                 Pointer to EFI_UDP6_PROTOCOL.
  @param[in]       Session              Pointer to the UDP6 session data.
  @param[in]       TimeoutEvent         The event for timeout.
  @param[in]       HeaderSize           An optional field which may be set to the length of a header
                                        at HeaderPtr to be prefixed to the data at BufferPtr.
  @param[in]       HeaderPtr            If HeaderSize is not NULL, a pointer to a header to be
                                        prefixed to the data at BufferPtr.
  @param[in]       BufferSize           A pointer to the size of the data at BufferPtr.
  @param[in]       BufferPtr            A pointer to the data to be written.

  @retval          EFI_SUCCESS          Successfully to send out data with Udp6Write.
  @retval          Others               Failed to send out data.

**/
EFI_STATUS
PxeBcUdp6Write (
  IN EFI_UDP6_PROTOCOL       *Udp6,
  IN EFI_UDP6_SESSION_DATA   *Session,
  IN EFI_EVENT               TimeoutEvent,
  IN UINTN                   *HeaderSize   OPTIONAL,
  IN VOID                    *HeaderPtr    OPTIONAL,
  IN UINTN                   *BufferSize,
  IN VOID                    *BufferPtr
  );


/**
  Check the received packet with the Ip filter.

  @param[in]  Mode                Pointer to mode data of PxeBc.
  @param[in]  Session             Pointer to the current UDPv4 session.
  @param[in]  OpFlags             Operation flag for UdpRead/UdpWrite.

  @retval     TRUE                Succesfully passed the Ip filter.
  @retval     FALSE               Failed to pass the Ip filter.

**/
BOOLEAN
PxeBcCheckByIpFilter (
  IN EFI_PXE_BASE_CODE_MODE    *Mode,
  IN VOID                      *Session,
  IN UINT16                    OpFlags
  );


/**
  Filter the received packet with the destination Ip.

  @param[in]       Mode           Pointer to mode data of PxeBc.
  @param[in]       Session        Pointer to the current UDPv4 session.
  @param[in, out]  DestIp         Pointer to the dest Ip address.
  @param[in]       OpFlags        Operation flag for UdpRead/UdpWrite.

  @retval     TRUE                Succesfully passed the IPv4 filter.
  @retval     FALSE               Failed to pass the IPv4 filter.

**/
BOOLEAN
PxeBcCheckByDestIp (
  IN     EFI_PXE_BASE_CODE_MODE    *Mode,
  IN     VOID                      *Session,
  IN OUT EFI_IP_ADDRESS            *DestIp,
  IN     UINT16                    OpFlags
  );


/**
  Check the received packet with the destination port.

  @param[in]       Mode           Pointer to mode data of PxeBc.
  @param[in]       Session        Pointer to the current UDPv4 session.
  @param[in, out]  DestPort       Pointer to the destination port.
  @param[in]       OpFlags        Operation flag for UdpRead/UdpWrite.

  @retval     TRUE                Succesfully passed the IPv4 filter.
  @retval     FALSE               Failed to pass the IPv4 filter.

**/
BOOLEAN
PxeBcCheckByDestPort (
  IN     EFI_PXE_BASE_CODE_MODE    *Mode,
  IN     VOID                      *Session,
  IN OUT UINT16                    *DestPort,
  IN     UINT16                    OpFlags
  );


/**
  Filter the received packet with the source Ip.

  @param[in]       Mode           Pointer to mode data of PxeBc.
  @param[in]       Session        Pointer to the current UDPv4 session.
  @param[in, out]  SrcIp          Pointer to the source Ip address.
  @param[in]       OpFlags        Operation flag for UdpRead/UdpWrite.

  @retval     TRUE                Succesfully passed the IPv4 filter.
  @retval     FALSE               Failed to pass the IPv4 filter.

**/
BOOLEAN
PxeBcFilterBySrcIp (
  IN     EFI_PXE_BASE_CODE_MODE    *Mode,
  IN     VOID                      *Session,
  IN OUT EFI_IP_ADDRESS            *SrcIp,
  IN     UINT16                    OpFlags
  );


/**
  Filter the received packet with the source port.

  @param[in]       Mode           Pointer to mode data of PxeBc.
  @param[in]       Session        Pointer to the current UDPv4 session.
  @param[in, out]  SrcPort        Pointer to the source port.
  @param[in]       OpFlags        Operation flag for UdpRead/UdpWrite.

  @retval     TRUE                Succesfully passed the IPv4 filter.
  @retval     FALSE               Failed to pass the IPv4 filter.

**/
BOOLEAN
PxeBcFilterBySrcPort (
  IN     EFI_PXE_BASE_CODE_MODE    *Mode,
  IN     VOID                      *Session,
  IN OUT UINT16                    *SrcPort,
  IN     UINT16                    OpFlags
  );


/**
  This function is to receive packet with Udp4Read.

  @param[in]       Udp4                 Pointer to EFI_UDP4_PROTOCOL.
  @param[in]       Token                Pointer to EFI_UDP4_COMPLETION_TOKEN.
  @param[in]       Mode                 Pointer to EFI_PXE_BASE_CODE_MODE.
  @param[in]       TimeoutEvent         The event for timeout.
  @param[in]       OpFlags              The UDP operation flags.
  @param[in]       IsDone               Pointer to IsDone flag.
  @param[out]      IsMatched            Pointer to IsMatched flag.
  @param[in, out]  DestIp               Pointer to destination address.
  @param[in, out]  DestPort             Pointer to destination port.
  @param[in, out]  SrcIp                Pointer to source address.
  @param[in, out]  SrcPort              Pointer to source port.

  @retval          EFI_SUCCESS          Successfully read data with Udp4.
  @retval          Others               Failed to send out data.

**/
EFI_STATUS
PxeBcUdp4Read (
  IN     EFI_UDP4_PROTOCOL            *Udp4,
  IN     EFI_UDP4_COMPLETION_TOKEN    *Token,
  IN     EFI_PXE_BASE_CODE_MODE       *Mode,
  IN     EFI_EVENT                    TimeoutEvent,
  IN     UINT16                       OpFlags,
  IN     BOOLEAN                      *IsDone,
     OUT BOOLEAN                      *IsMatched,
  IN OUT EFI_IP_ADDRESS               *DestIp      OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT   *DestPort    OPTIONAL,
  IN OUT EFI_IP_ADDRESS               *SrcIp       OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT   *SrcPort     OPTIONAL
  );


/**
  This function is to receive packet with Udp6Read.

  @param[in]       Udp6                 Pointer to EFI_UDP6_PROTOCOL.
  @param[in]       Token                Pointer to EFI_UDP6_COMPLETION_TOKEN.
  @param[in]       Mode                 Pointer to EFI_PXE_BASE_CODE_MODE.
  @param[in]       TimeoutEvent         The event for timeout.
  @param[in]       OpFlags              The UDP operation flags.
  @param[in]       IsDone               Pointer to IsDone flag.
  @param[out]      IsMatched            Pointer to IsMatched flag.
  @param[in, out]  DestIp               Pointer to destination address.
  @param[in, out]  DestPort             Pointer to destination port.
  @param[in, out]  SrcIp                Pointer to source address.
  @param[in, out]  SrcPort              Pointer to source port.

  @retval          EFI_SUCCESS          Successfully read data with Udp6.
  @retval          Others               Failed to send out data.

**/
EFI_STATUS
PxeBcUdp6Read (
  IN     EFI_UDP6_PROTOCOL            *Udp6,
  IN     EFI_UDP6_COMPLETION_TOKEN    *Token,
  IN     EFI_PXE_BASE_CODE_MODE       *Mode,
  IN     EFI_EVENT                    TimeoutEvent,
  IN     UINT16                       OpFlags,
  IN     BOOLEAN                      *IsDone,
     OUT BOOLEAN                      *IsMatched,
  IN OUT EFI_IP_ADDRESS               *DestIp      OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT   *DestPort    OPTIONAL,
  IN OUT EFI_IP_ADDRESS               *SrcIp       OPTIONAL,
  IN OUT EFI_PXE_BASE_CODE_UDP_PORT   *SrcPort     OPTIONAL
  );


/**
  This function is to display the IPv4 address.

  @param[in]  Ip        Pointer to the IPv4 address.

**/
VOID
PxeBcShowIp4Addr (
  IN EFI_IPv4_ADDRESS   *Ip
  );


/**
  This function is to display the IPv6 address.

  @param[in]  Ip        Pointer to the IPv6 address.

**/
VOID
PxeBcShowIp6Addr (
  IN EFI_IPv6_ADDRESS   *Ip
  );


/**
  This function is to convert UINTN to ASCII string with required format.

  @param[in]  Number         Numeric value to be converted.
  @param[in]  Buffer         Pointer to the buffer for ASCII string.
  @param[in]  Length         Length of the required format.

**/
VOID
PxeBcUintnToAscDecWithFormat (
  IN UINTN                       Number,
  IN UINT8                       *Buffer,
  IN INTN                        Length
  );


/**
  This function is to convert a UINTN to a ASCII string, and return the
  actual length of the buffer.

  @param[in]  Number         Numeric value to be converted.
  @param[in]  Buffer         Pointer to the buffer for ASCII string.
  @param[in]  BufferSize     The maxsize of the buffer.
  
  @return     Length         The actual length of the ASCII string.

**/
UINTN
PxeBcUintnToAscDec (
  IN UINTN               Number,
  IN UINT8               *Buffer,
  IN UINTN               BufferSize
  );

/**
  This function is to convert unicode hex number to a UINT8.

  @param[out]  Digit                   The converted UINT8 for output.
  @param[in]   Char                    The unicode hex number to be converted.

  @retval      EFI_SUCCESS             Successfully converted the unicode hex.
  @retval      EFI_INVALID_PARAMETER   Failed to convert the unicode hex.

**/
EFI_STATUS
PxeBcUniHexToUint8 (
  OUT UINT8                *Digit,
  IN  CHAR16               Char
  );

/**
  Calculate the elapsed time.

  @param[in]      Private      The pointer to PXE private data

**/
VOID
CalcElapsedTime (
  IN     PXEBC_PRIVATE_DATA     *Private
  );

/**
  Get the Nic handle using any child handle in the IPv4 stack.

  @param[in]  ControllerHandle    Pointer to child handle over IPv4.

  @return NicHandle               The pointer to the Nic handle.

**/
EFI_HANDLE
PxeBcGetNicByIp4Children (
  IN EFI_HANDLE                 ControllerHandle
  );

/**
  Get the Nic handle using any child handle in the IPv6 stack.

  @param[in]  ControllerHandle    Pointer to child handle over IPv6.

  @return NicHandle               The pointer to the Nic handle.

**/
EFI_HANDLE
PxeBcGetNicByIp6Children (
  IN EFI_HANDLE                  ControllerHandle
  );
#endif
