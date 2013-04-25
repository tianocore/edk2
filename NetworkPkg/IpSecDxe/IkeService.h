/** @file
  Prototypes definitions of IKE service.

  Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IKE_SERVICE_H_
#define _IKE_SERVICE_H_

#include "Ike.h"
#include "IpSecImpl.h"
#include "IkeCommon.h"

#define IPSEC_CRYPTO_LIB_MEMORY 128 * 1024

/**
  This is prototype definition of general interface to intialize a IKE negotiation.

  @param[in]  UdpService      Point to Udp Servcie used for the IKE packet sending.
  @param[in]  SpdEntry        Point to SPD entry related to this IKE negotiation.
  @param[in]  PadEntry        Point to PAD entry related to this IKE negotiation.
  @param[in]  RemoteIp        Point to IP Address which the remote peer to negnotiate.

  @retval EFI_SUCCESS     The operation is successful.
  @return Otherwise       The operation is failed.

**/
typedef
EFI_STATUS
(*IKE_NEGOTIATE_SA) (
  IN IKE_UDP_SERVICE                * UdpService,
  IN IPSEC_SPD_ENTRY                * SpdEntry,
  IN IPSEC_PAD_ENTRY                * PadEntry,
  IN EFI_IP_ADDRESS                 * RemoteIp
  );

/**
  This is prototype definition fo general interface to start a IKE negotiation at Quick Mode.

  This function will be called when the related IKE SA is existed and start to
  create a Child SA.

  @param[in]  IkeSaSession    Point to IKE SA Session related to this Negotiation.
  @param[in]  SpdEntry        Point to SPD entry related to this Negotiation.
  @param[in]  Context         Point to data passed from the caller.

  @retval EFI_SUCCESS     The operation is successful.
  @retval Otherwise       The operation is failed.

**/
typedef
EFI_STATUS
(*IKE_NEGOTIATE_CHILD_SA) (
  IN UINT8                          *IkeSaSession,
  IN IPSEC_SPD_ENTRY                *SpdEntry,
  IN UINT8                          *Context
  );

/**
  This is prototype definition of the general interface when initialize a Inforamtion
  Exchange.

  @param[in]  IkeSaSession      Point to IKE SA Session related to.
  @param[in]  Context           Point to data passed from caller.

**/
typedef
EFI_STATUS
(*IKE_NEGOTIATE_INFO) (
  IN UINT8                          *IkeSaSession,
  IN UINT8                          *Context
  );

/**
  This is prototype definition of the general interface when recived a IKE Pakcet
  for the IKE SA establishing.

  @param[in]  UdpService      Point to UDP service used to send IKE Packet.
  @param[in]  IkePacket       Point to received IKE packet.

**/
typedef
VOID
(*IKE_HANDLE_SA) (
  IN IKE_UDP_SERVICE                *UdpService,
  IN IKE_PACKET                     *IkePacket
  );

/**
  This is prototyp definition of the general interface when recived a IKE Packet
  xfor the Child SA establishing.

  @param[in]  UdpService      Point to UDP service used to send IKE packet.
  @param[in]  IkePacket       Point to received IKE packet.

**/
typedef
VOID
(*IKE_HANDLE_CHILD_SA) (
  IN IKE_UDP_SERVICE                *UdpService,
  IN IKE_PACKET                     *IkePacket
  );

/**
  This is prototype definition of the general interface when received a IKE
  information Packet.

  @param[in]  UdpService      Point to UDP service used to send IKE packet.
  @param[in]  IkePacket       Point to received IKE packet.

**/
typedef
VOID
(*IKE_HANDLE_INFO) (
  IN IKE_UDP_SERVICE                *UdpService,
  IN IKE_PACKET                     *IkePacket
  );

typedef struct _IKE_EXCHANGE_INTERFACE {
  UINT8                   IkeVer;
  IKE_NEGOTIATE_SA        NegotiateSa;
  IKE_NEGOTIATE_CHILD_SA  NegotiateChildSa;
  IKE_NEGOTIATE_INFO      NegotiateInfo;
  IKE_HANDLE_SA           HandleSa;
  IKE_HANDLE_CHILD_SA     HandleChildSa;
  IKE_HANDLE_INFO         HandleInfo;
} IKE_EXCHANGE_INTERFACE;

/**
  Open and configure a UDPIO of Udp4 for IKE packet receiving.

  This function is called at the IPsecDriverBinding start. IPsec create a UDP4 and
  a UDP4 IO for each NIC handle.

  @param[in] Private        Point to IPSEC_PRIVATE_DATA
  @param[in] Controller     Handler for NIC card.
  @param[in] ImageHandle    The handle that contains the EFI_DRIVER_BINDING_PROTOCOL instance.

  @retval EFI_SUCCESS             The Operation is successful.
  @retval EFI_OUT_OF_RESOURCE     The required system resource can't be allocated.

**/
EFI_STATUS
IkeOpenInputUdp4 (
  IN IPSEC_PRIVATE_DATA             *Private,
  IN EFI_HANDLE                     Controller,
  IN EFI_HANDLE                     ImageHandle
  );

/**
  Open and configure a UDPIO of Udp6 for IKE packet receiving.

  This function is called at the IPsecDriverBinding start. IPsec create a UDP6 and UDP6
  IO for each NIC handle.

  @param[in] Private        Point to IPSEC_PRIVATE_DATA
  @param[in] Controller     Handler for NIC card.
  @param[in] ImageHandle    The handle that contains the EFI_DRIVER_BINDING_PROTOCOL instance.

  @retval EFI_SUCCESS             The Operation is successful.
  @retval EFI_OUT_OF_RESOURCE     The required system resource can't be allocated.

**/
EFI_STATUS
IkeOpenInputUdp6 (
  IN IPSEC_PRIVATE_DATA             *Private,
  IN EFI_HANDLE                     Controller,
  IN EFI_HANDLE                     ImageHandle
  );

/**
  The general interface of starting IPsec Key Exchange.

  This function is called when start a IKE negotiation to get a Key.

  @param[in] UdpService   Point to IKE_UDP_SERVICE which will be used for
                          IKE packet sending.
  @param[in] SpdEntry     Point to the SPD entry related to the IKE negotiation.
  @param[in] RemoteIp     Point to EFI_IP_ADDRESS related to the IKE negotiation.

  @retval EFI_SUCCESS          The Operation is successful.
  @retval EFI_ACCESS_DENIED    No related PAD entry was found.

**/
EFI_STATUS
IkeNegotiate (
  IN IKE_UDP_SERVICE                *UdpService,
  IN IPSEC_SPD_ENTRY                *SpdEntry,
  IN EFI_IP_ADDRESS                 *RemoteIp
  );

/**
  The general interface when receive a IKE packet.

  This function is called when UDP IO receives a IKE packet.

  @param[in] Packet       Point to received IKE packet.
  @param[in] EndPoint     Point to UDP_END_POINT which contains the information of
                          Remote IP and Port.
  @param[in] IoStatus     The Status of Recieve Token.
  @param[in] Context      Point to data passed from the caller.

**/
VOID
EFIAPI
IkeDispatch (
  IN NET_BUF                        *Packet,
  IN UDP_END_POINT                  *EndPoint,
  IN EFI_STATUS                     IoStatus,
  IN VOID                           *Context
  );

/**
  Check if the NIC handle is binded to a Udp service.

  @param[in]  Private    Pointer of IPSEC_PRIVATE_DATA
  @param[in]  Handle     The Handle of the NIC card
  @param[in]  IpVersion  The version of the IP stack.

  @return a pointer of IKE_UDP_SERVICE.

**/
IKE_UDP_SERVICE *
IkeLookupUdp (
  IN IPSEC_PRIVATE_DATA             *Private,
  IN EFI_HANDLE                     Handle,
  IN UINT8                          IpVersion
  );


/**
  Delete all established IKE SAs and related Child SAs.

  This function is the subfunction of the IpSecCleanupAllSa(). It first calls
  IkeDeleteChildSa() to delete all Child SAs then send out the related
  Information packet.

  @param[in]  Private           Pointer of the IPSEC_PRIVATE_DATA.
  @param[in]  IsDisableIpsec    Indicate whether needs to disable IPsec.

**/
VOID
IkeDeleteAllSas (
  IN IPSEC_PRIVATE_DATA             *Private,
  IN BOOLEAN                        IsDisableIpsec
  );


extern IKE_EXCHANGE_INTERFACE       mIkev1Exchange;
extern IKE_EXCHANGE_INTERFACE       mIkev2Exchange;

#endif
