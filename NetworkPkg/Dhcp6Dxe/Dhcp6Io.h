/** @file
  Dhcp6 internal functions declaration.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_DHCP6_IO_H__
#define __EFI_DHCP6_IO_H__


/**
  Clean up the specific nodes in the retry list.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  Scope           The scope of cleanup nodes.

**/
VOID
Dhcp6CleanupRetry (
  IN DHCP6_INSTANCE         *Instance,
  IN UINT32                 Scope
  );

/**
  Clean up the session of the instance stateful exchange.

  @param[in, out]  Instance        The pointer to the Dhcp6 instance.
  @param[in]       Status          The return status from udp.

**/
VOID
Dhcp6CleanupSession (
  IN OUT DHCP6_INSTANCE          *Instance,
  IN     EFI_STATUS              Status
  );

/**
  Create the solicit message and send it.

  @param[in]  Instance        The pointer to Dhcp6 instance.

  @retval EFI_SUCCESS           Create and send the solicit message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval Others                Failed to send the solicit message.

**/
EFI_STATUS
Dhcp6SendSolicitMsg (
  IN DHCP6_INSTANCE         *Instance
  );

/**
  Create the request message and send it.

  @param[in]  Instance        The pointer to the Dhcp6 instance.

  @retval EFI_SUCCESS           Create and send the request message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected error.
  @retval Others                Failed to send the request message.

**/
EFI_STATUS
Dhcp6SendRequestMsg (
  IN DHCP6_INSTANCE         *Instance
  );

/**
  Create the renew/rebind message and send it.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  RebindRequest   If TRUE, it is a Rebind type message.
                              Otherwise, it is a Renew type message.

  @retval EFI_SUCCESS           Create and send the renew/rebind message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected error.
  @retval Others                Failed to send the renew/rebind message.

**/
EFI_STATUS
Dhcp6SendRenewRebindMsg (
  IN DHCP6_INSTANCE         *Instance,
  IN BOOLEAN                RebindRequest
  );

/**
  Create the decline message and send it.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  DecIa           The pointer to the decline Ia.

  @retval EFI_SUCCESS           Create and send the decline message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected error.
  @retval Others                Failed to send the decline message.

**/
EFI_STATUS
Dhcp6SendDeclineMsg (
  IN DHCP6_INSTANCE            *Instance,
  IN EFI_DHCP6_IA              *DecIa
  );

/**
  Create the release message and send it.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  RelIa           The pointer to the release Ia.

  @retval EFI_SUCCESS           Create and send the release message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected error.
  @retval Others                Failed to send the release message.

**/
EFI_STATUS
Dhcp6SendReleaseMsg (
  IN DHCP6_INSTANCE            *Instance,
  IN EFI_DHCP6_IA              *RelIa
  );

/**
  Start the information request process.

  @param[in]  Instance          The pointer to the Dhcp6 instance.
  @param[in]  SendClientId      If TRUE, the client identifier option will be included in
                                information request message. Otherwise, the client identifier
                                option will not be included.
  @param[in]  OptionRequest     The pointer to the option request option.
  @param[in]  OptionCount       The number options in the OptionList.
  @param[in]  OptionList        The array pointers to the appended options.
  @param[in]  Retransmission    The pointer to the retransmission control.
  @param[in]  TimeoutEvent      The event of timeout.
  @param[in]  ReplyCallback     The callback function when the reply was received.
  @param[in]  CallbackContext   The pointer to the parameter passed to the callback.

  @retval EFI_SUCCESS           Start the info-request process successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_NO_MAPPING        No source address is available for use.
  @retval Others                Failed to start the info-request process.

**/
EFI_STATUS
Dhcp6StartInfoRequest (
  IN DHCP6_INSTANCE            *Instance,
  IN BOOLEAN                   SendClientId,
  IN EFI_DHCP6_PACKET_OPTION   *OptionRequest,
  IN UINT32                    OptionCount,
  IN EFI_DHCP6_PACKET_OPTION   *OptionList[]    OPTIONAL,
  IN EFI_DHCP6_RETRANSMISSION  *Retransmission,
  IN EFI_EVENT                 TimeoutEvent     OPTIONAL,
  IN EFI_DHCP6_INFO_CALLBACK   ReplyCallback,
  IN VOID                      *CallbackContext OPTIONAL
  );

/**
  Create the information request message and send it.

  @param[in]  Instance        The pointer to the Dhcp6 instance.
  @param[in]  InfCb           The pointer to the information request control block.
  @param[in]  SendClientId    If TRUE, the client identifier option will be included in
                              information request message. Otherwise, the client identifier
                              option will not be included.
  @param[in]  OptionRequest   The pointer to the option request option.
  @param[in]  OptionCount     The number options in the OptionList.
  @param[in]  OptionList      The array pointers to the appended options.
  @param[in]  Retransmission  The pointer to the retransmission control.

  @retval EFI_SUCCESS           Create and send the info-request message successfully.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval Others                Failed to send the info-request message.

**/
EFI_STATUS
Dhcp6SendInfoRequestMsg (
  IN DHCP6_INSTANCE            *Instance,
  IN DHCP6_INF_CB              *InfCb,
  IN BOOLEAN                   SendClientId,
  IN EFI_DHCP6_PACKET_OPTION   *OptionRequest,
  IN UINT32                    OptionCount,
  IN EFI_DHCP6_PACKET_OPTION   *OptionList[],
  IN EFI_DHCP6_RETRANSMISSION  *Retransmission
  );

/**
  The receive callback function for the Dhcp6 exchange process.

  @param[in]  Udp6Wrap        The pointer to the received net buffer.
  @param[in]  EndPoint        The pointer to the udp end point.
  @param[in]  IoStatus        The return status from udp io.
  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
Dhcp6ReceivePacket (
  IN NET_BUF                *Udp6Wrap,
  IN UDP_END_POINT          *EndPoint,
  IN EFI_STATUS             IoStatus,
  IN VOID                   *Context
  );

/**
  The timer routine of the Dhcp6 instance for each second.

  @param[in]  Event           The timer event.
  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
Dhcp6OnTimerTick (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );

#endif
