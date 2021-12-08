/** @file
  Declaration of structures and functions of MnpDxe driver.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MNP_IMPL_H_
#define _MNP_IMPL_H_

#include "MnpDriver.h"

#define NET_ETHER_FCS_SIZE  4

#define MNP_SYS_POLL_INTERVAL        (10 * TICKS_PER_MS)    // 10 milliseconds
#define MNP_TIMEOUT_CHECK_INTERVAL   (50 * TICKS_PER_MS)    // 50 milliseconds
#define MNP_MEDIA_DETECT_INTERVAL    (500 * TICKS_PER_MS)   // 500 milliseconds
#define MNP_TX_TIMEOUT_TIME          (500 * TICKS_PER_MS)   // 500 milliseconds
#define MNP_INIT_NET_BUFFER_NUM      512
#define MNP_NET_BUFFER_INCREASEMENT  64
#define MNP_MAX_NET_BUFFER_NUM       65536
#define MNP_TX_BUFFER_INCREASEMENT   32     // Same as the recycling Q length for xmit_done in UNDI command.
#define MNP_MAX_TX_BUFFER_NUM        65536

#define MNP_MAX_RCVD_PACKET_QUE_SIZE  256

#define MNP_RECEIVE_UNICAST    0x01
#define MNP_RECEIVE_BROADCAST  0x02

#define UNICAST_PACKET    MNP_RECEIVE_UNICAST
#define BROADCAST_PACKET  MNP_RECEIVE_BROADCAST

#define MNP_INSTANCE_DATA_SIGNATURE  SIGNATURE_32 ('M', 'n', 'p', 'I')

#define MNP_INSTANCE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  MNP_INSTANCE_DATA, \
  ManagedNetwork, \
  MNP_INSTANCE_DATA_SIGNATURE \
  )

typedef struct {
  UINT32                             Signature;

  MNP_SERVICE_DATA                   *MnpServiceData;

  EFI_HANDLE                         Handle;

  LIST_ENTRY                         InstEntry;

  EFI_MANAGED_NETWORK_PROTOCOL       ManagedNetwork;

  BOOLEAN                            Configured;
  BOOLEAN                            Destroyed;

  LIST_ENTRY                         GroupCtrlBlkList;

  NET_MAP                            RxTokenMap;

  LIST_ENTRY                         RxDeliveredPacketQueue;
  LIST_ENTRY                         RcvdPacketQueue;
  UINTN                              RcvdPacketQueueSize;

  EFI_MANAGED_NETWORK_CONFIG_DATA    ConfigData;

  UINT8                              ReceiveFilter;
} MNP_INSTANCE_DATA;

typedef struct {
  LIST_ENTRY         AddrEntry;
  EFI_MAC_ADDRESS    Address;
  INTN               RefCnt;
} MNP_GROUP_ADDRESS;

typedef struct {
  LIST_ENTRY           CtrlBlkEntry;
  MNP_GROUP_ADDRESS    *GroupAddress;
} MNP_GROUP_CONTROL_BLOCK;

typedef struct {
  LIST_ENTRY                          WrapEntry;
  MNP_INSTANCE_DATA                   *Instance;
  EFI_MANAGED_NETWORK_RECEIVE_DATA    RxData;
  NET_BUF                             *Nbuf;
  UINT64                              TimeoutTick;
} MNP_RXDATA_WRAP;

#define MNP_TX_BUF_WRAP_SIGNATURE  SIGNATURE_32 ('M', 'T', 'B', 'W')

typedef struct {
  UINT32        Signature;
  LIST_ENTRY    WrapEntry;            // Link to FreeTxBufList
  LIST_ENTRY    AllEntry;             // Link to AllTxBufList
  BOOLEAN       InUse;
  UINT8         TxBuf[1];
} MNP_TX_BUF_WRAP;

/**
  Initialize the mnp device context data.

  @param[in, out]  MnpDeviceData      Pointer to the mnp device context data.
  @param[in]       ImageHandle        The driver image handle.
  @param[in]       ControllerHandle   Handle of device to bind driver to.

  @retval EFI_SUCCESS           The mnp service context is initialized.
  @retval EFI_UNSUPPORTED       ControllerHandle does not support Simple Network Protocol.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
MnpInitializeDeviceData (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData,
  IN     EFI_HANDLE       ImageHandle,
  IN     EFI_HANDLE       ControllerHandle
  );

/**
  Destroy the MNP device context data.

  @param[in, out]  MnpDeviceData      Pointer to the mnp device context data.
  @param[in]       ImageHandle        The driver image handle.

**/
VOID
MnpDestroyDeviceData (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData,
  IN     EFI_HANDLE       ImageHandle
  );

/**
  Create mnp service context data.

  @param[in]       MnpDeviceData      Pointer to the mnp device context data.
  @param[in]       VlanId             The VLAN ID.
  @param[in]       Priority           The VLAN priority. If VlanId is 0,
                                      Priority is ignored.

  @return A pointer to MNP_SERVICE_DATA or NULL if failed to create MNP service context.

**/
MNP_SERVICE_DATA *
MnpCreateServiceData (
  IN MNP_DEVICE_DATA  *MnpDeviceData,
  IN UINT16           VlanId,
  IN UINT8            Priority OPTIONAL
  );

/**
  Initialize the mnp service context data.

  @param[in, out]  MnpServiceData     Pointer to the mnp service context data.
  @param[in]       ImageHandle        The driver image handle.
  @param[in]       ControllerHandle   Handle of device to bind driver to.

  @retval EFI_SUCCESS           The mnp service context is initialized.
  @retval EFI_UNSUPPORTED       ControllerHandle does not support Simple Network Protocol.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
MnpInitializeServiceData (
  IN OUT MNP_SERVICE_DATA  *MnpServiceData,
  IN     EFI_HANDLE        ImageHandle,
  IN     EFI_HANDLE        ControllerHandle
  );

/**
  Destroy the MNP service context data.

  @param[in, out]  MnpServiceData    Pointer to the mnp service context data.

  @retval EFI_SUCCESS           The mnp service context is destroyed.
  @retval Others                Errors as indicated.

**/
EFI_STATUS
MnpDestroyServiceData (
  IN OUT MNP_SERVICE_DATA  *MnpServiceData
  );

/**
  Destroy all child of the MNP service data.

  @param[in, out]  MnpServiceData    Pointer to the mnp service context data.

  @retval EFI_SUCCESS           All child are destroyed.
  @retval Others                Failed to destroy all child.

**/
EFI_STATUS
MnpDestroyServiceChild (
  IN OUT MNP_SERVICE_DATA  *MnpServiceData
  );

/**
  Find the MNP Service Data for given VLAN ID.

  @param[in]  MnpDeviceData      Pointer to the mnp device context data.
  @param[in]  VlanId             The VLAN ID.

  @return A pointer to MNP_SERVICE_DATA or NULL if not found.

**/
MNP_SERVICE_DATA *
MnpFindServiceData (
  IN MNP_DEVICE_DATA  *MnpDeviceData,
  IN UINT16           VlanId
  );

/**
  Initialize the mnp instance context data.

  @param[in]       MnpServiceData   Pointer to the mnp service context data.
  @param[in, out]  Instance         Pointer to the mnp instance context data
                                    to initialize.

**/
VOID
MnpInitializeInstanceData (
  IN     MNP_SERVICE_DATA   *MnpServiceData,
  IN OUT MNP_INSTANCE_DATA  *Instance
  );

/**
  Check whether the token specified by Arg matches the token in Item.

  @param[in]  Map               Pointer to the NET_MAP.
  @param[in]  Item              Pointer to the NET_MAP_ITEM.
  @param[in]  Arg               Pointer to the Arg, it's a pointer to the token to
                                check.

  @retval EFI_SUCCESS           The token specified by Arg is different from the
                                token in Item.
  @retval EFI_ACCESS_DENIED     The token specified by Arg is the same as that in
                                Item.

**/
EFI_STATUS
EFIAPI
MnpTokenExist (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg
  );

/**
  Cancel the token specified by Arg if it matches the token in Item.

  @param[in, out]  Map               Pointer to the NET_MAP.
  @param[in, out]  Item              Pointer to the NET_MAP_ITEM.
  @param[in]       Arg               Pointer to the Arg, it's a pointer to the
                                     token to cancel.

  @retval EFI_SUCCESS       The Arg is NULL, and the token in Item is cancelled,
                            or the Arg isn't NULL, and the token in Item is
                            different from the Arg.
  @retval EFI_ABORTED       The Arg isn't NULL, the token in Item mathces the
                            Arg, and the token is cancelled.

**/
EFI_STATUS
EFIAPI
MnpCancelTokens (
  IN OUT NET_MAP       *Map,
  IN OUT NET_MAP_ITEM  *Item,
  IN     VOID          *Arg
  );

/**
  Flush the instance's received data.

  @param[in, out]  Instance              Pointer to the mnp instance context data.

**/
VOID
MnpFlushRcvdDataQueue (
  IN OUT MNP_INSTANCE_DATA  *Instance
  );

/**
  Configure the Instance using ConfigData.

  @param[in, out]  Instance     Pointer to the mnp instance context data.
  @param[in]       ConfigData   Pointer to the configuration data used to configure
                                the instance.

  @retval EFI_SUCCESS           The Instance is configured.
  @retval EFI_UNSUPPORTED       EnableReceiveTimestamps is on and the
                                implementation doesn't support it.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
MnpConfigureInstance (
  IN OUT MNP_INSTANCE_DATA                *Instance,
  IN     EFI_MANAGED_NETWORK_CONFIG_DATA  *ConfigData OPTIONAL
  );

/**
  Do the group operations for this instance.

  @param[in, out]  Instance        Pointer to the instance context data.
  @param[in]       JoinFlag        Set to TRUE to join a group. Set to TRUE to
                                   leave a group/groups.
  @param[in]       MacAddress      Pointer to the group address to join or leave.
  @param[in]       CtrlBlk         Pointer to the group control block if JoinFlag
                                   is FALSE.

  @retval EFI_SUCCESS              The group operation finished.
  @retval EFI_OUT_OF_RESOURCES     Failed due to lack of memory resources.
  @retval Others                   Other errors as indicated.

**/
EFI_STATUS
MnpGroupOp (
  IN OUT MNP_INSTANCE_DATA        *Instance,
  IN     BOOLEAN                  JoinFlag,
  IN     EFI_MAC_ADDRESS          *MacAddress OPTIONAL,
  IN     MNP_GROUP_CONTROL_BLOCK  *CtrlBlk OPTIONAL
  );

/**
  Validates the Mnp transmit token.

  @param[in]  Instance            Pointer to the Mnp instance context data.
  @param[in]  Token               Pointer to the transmit token to check.

  @return The Token is valid or not.

**/
BOOLEAN
MnpIsValidTxToken (
  IN MNP_INSTANCE_DATA                     *Instance,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  );

/**
  Build the packet to transmit from the TxData passed in.

  @param[in]   MnpServiceData      Pointer to the mnp service context data.
  @param[in]   TxData              Pointer to the transmit data containing the information
                                   to build the packet.
  @param[out]  PktBuf              Pointer to record the address of the packet.
  @param[out]  PktLen              Pointer to a UINT32 variable used to record the packet's
                                   length.

  @retval EFI_SUCCESS           TxPackage is built.
  @retval EFI_OUT_OF_RESOURCES  The deliver fails due to lack of memory resource.

**/
EFI_STATUS
MnpBuildTxPacket (
  IN     MNP_SERVICE_DATA                   *MnpServiceData,
  IN     EFI_MANAGED_NETWORK_TRANSMIT_DATA  *TxData,
  OUT UINT8                                 **PktBuf,
  OUT UINT32                                *PktLen
  );

/**
  Synchronously send out the packet.

  This function places the packet buffer to SNP driver's tansmit queue. The packet
  can be considered successfully sent out once SNP accept the packet, while the
  packet buffer recycle is deferred for better performance.

  @param[in]       MnpServiceData      Pointer to the mnp service context data.
  @param[in]       Packet              Pointer to the packet buffer.
  @param[in]       Length              The length of the packet.
  @param[in, out]  Token               Pointer to the token the packet generated from.

  @retval EFI_SUCCESS                  The packet is sent out.
  @retval EFI_TIMEOUT                  Time out occurs, the packet isn't sent.
  @retval EFI_DEVICE_ERROR             An unexpected network error occurs.

**/
EFI_STATUS
MnpSyncSendPacket (
  IN     MNP_SERVICE_DATA                      *MnpServiceData,
  IN     UINT8                                 *Packet,
  IN     UINT32                                Length,
  IN OUT EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  );

/**
  Try to deliver the received packet to the instance.

  @param[in, out]  Instance     Pointer to the mnp instance context data.

  @retval EFI_SUCCESS           The received packet is delivered, or there is no
                                packet to deliver, or there is no available receive
                                token.
  @retval EFI_OUT_OF_RESOURCES  The deliver fails due to lack of memory resource.

**/
EFI_STATUS
MnpInstanceDeliverPacket (
  IN OUT MNP_INSTANCE_DATA  *Instance
  );

/**
  Recycle the RxData and other resources used to hold and deliver the received
  packet.

  @param[in]  Event               The event this notify function registered to.
  @param[in]  Context             Pointer to the context data registered to the Event.

**/
VOID
EFIAPI
MnpRecycleRxData (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Try to receive a packet and deliver it.

  @param[in, out]  MnpDeviceData        Pointer to the mnp device context data.

  @retval EFI_SUCCESS           add return value to function comment
  @retval EFI_NOT_STARTED       The simple network protocol is not started.
  @retval EFI_NOT_READY         No packet received.
  @retval EFI_DEVICE_ERROR      An unexpected error occurs.

**/
EFI_STATUS
MnpReceivePacket (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData
  );

/**
  Allocate a free NET_BUF from MnpDeviceData->FreeNbufQue. If there is none
  in the queue, first try to allocate some and add them into the queue, then
  fetch the NET_BUF from the updated FreeNbufQue.

  @param[in, out]  MnpDeviceData        Pointer to the MNP_DEVICE_DATA.

  @return     Pointer to the allocated free NET_BUF structure, if NULL the
              operation is failed.

**/
NET_BUF *
MnpAllocNbuf (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData
  );

/**
  Try to reclaim the Nbuf into the buffer pool.

  @param[in, out]  MnpDeviceData         Pointer to the mnp device context data.
  @param[in, out]  Nbuf                  Pointer to the NET_BUF to free.

**/
VOID
MnpFreeNbuf (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData,
  IN OUT NET_BUF          *Nbuf
  );

/**
  Allocate a free TX buffer from MnpDeviceData->FreeTxBufList. If there is none
  in the queue, first try to recycle some from SNP, then try to allocate some and add
  them into the queue, then fetch the NET_BUF from the updated FreeTxBufList.

  @param[in, out]  MnpDeviceData        Pointer to the MNP_DEVICE_DATA.

  @return     Pointer to the allocated free NET_BUF structure, if NULL the
              operation is failed.

**/
UINT8 *
MnpAllocTxBuf (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData
  );

/**
  Try to recycle all the transmitted buffer address from SNP.

  @param[in, out]  MnpDeviceData     Pointer to the mnp device context data.

  @retval EFI_SUCCESS             Successed to recycle the transmitted buffer address.
  @retval Others                  Failed to recycle the transmitted buffer address.

**/
EFI_STATUS
MnpRecycleTxBuf (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData
  );

/**
  Remove the received packets if timeout occurs.

  @param[in]  Event        The event this notify function registered to.
  @param[in]  Context      Pointer to the context data registered to the event.

**/
VOID
EFIAPI
MnpCheckPacketTimeout (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Poll to update MediaPresent field in SNP ModeData by Snp.GetStatus().

  @param[in]  Event        The event this notify function registered to.
  @param[in]  Context      Pointer to the context data registered to the event.

**/
VOID
EFIAPI
MnpCheckMediaStatus (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Poll to receive the packets from Snp. This function is either called by upperlayer
  protocols/applications or the system poll timer notify mechanism.

  @param[in]  Event        The event this notify function registered to.
  @param[in]  Context      Pointer to the context data registered to the event.

**/
VOID
EFIAPI
MnpSystemPoll (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Returns the operational parameters for the current MNP child driver. May also
  support returning the underlying SNP driver mode data.

  The GetModeData() function is used to read the current mode data (operational
  parameters) from the MNP or the underlying SNP.

  @param[in]  This          Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[out] MnpConfigData Pointer to storage for MNP operational parameters. Type
                            EFI_MANAGED_NETWORK_CONFIG_DATA is defined in "Related
                            Definitions" below.
  @param[out] SnpModeData   Pointer to storage for SNP operational parameters. This
                            feature may be unsupported. Type EFI_SIMPLE_NETWORK_MODE
                            is defined in the EFI_SIMPLE_NETWORK_PROTOCOL.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_UNSUPPORTED       The requested feature is unsupported in this
                                MNP implementation.
  @retval EFI_NOT_STARTED       This MNP child driver instance has not been
                                configured. The default values are returned in
                                MnpConfigData if it is not NULL.
  @retval Others                The mode data could not be read.

**/
EFI_STATUS
EFIAPI
MnpGetModeData (
  IN     EFI_MANAGED_NETWORK_PROTOCOL  *This,
  OUT EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData OPTIONAL,
  OUT EFI_SIMPLE_NETWORK_MODE          *SnpModeData OPTIONAL
  );

/**
  Sets or clears the operational parameters for the MNP child driver.

  The Configure() function is used to set, change, or reset the operational
  parameters for the MNP child driver instance. Until the operational parameters
  have been set, no network traffic can be sent or received by this MNP child
  driver instance. Once the operational parameters have been reset, no more
  traffic can be sent or received until the operational parameters have been set
  again.
  Each MNP child driver instance can be started and stopped independently of
  each other by setting or resetting their receive filter settings with the
  Configure() function.
  After any successful call to Configure(), the MNP child driver instance is
  started. The internal periodic timer (if supported) is enabled. Data can be
  transmitted and may be received if the receive filters have also been enabled.
  Note: If multiple MNP child driver instances will receive the same packet
  because of overlapping receive filter settings, then the first MNP child
  driver instance will receive the original packet and additional instances will
  receive copies of the original packet.
  Note: Warning: Receive filter settings that overlap will consume extra
  processor and/or DMA resources and degrade system and network performance.

  @param[in]  This           Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  MnpConfigData  Pointer to configuration data that will be assigned
                             to the MNP child driver instance. If NULL, the MNP
                             child driver instance is reset to startup defaults
                             and all pending transmit and receive requests are
                             flushed. Type EFI_MANAGED_NETWORK_CONFIG_DATA is
                             defined in EFI_MANAGED_NETWORK_PROTOCOL.GetModeData().

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is
                                 TRUE:
                                 * This is NULL.
                                 * MnpConfigData.ProtocolTypeFilter is not
                                   valid.
                                 The operational data for the MNP child driver
                                 instance is unchanged.
  @retval EFI_OUT_OF_RESOURCES   Required system resources (usually memory)
                                 could not be allocated.
                                 The MNP child driver instance has been reset to
                                 startup defaults.
  @retval EFI_UNSUPPORTED        The requested feature is unsupported in
                                 this [MNP] implementation. The operational data
                                 for the MNP child driver instance is unchanged.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error
                                 occurred. The MNP child driver instance has
                                 been reset to startup defaults.
  @retval Others                 The MNP child driver instance has been reset to
                                 startup defaults.

**/
EFI_STATUS
EFIAPI
MnpConfigure (
  IN EFI_MANAGED_NETWORK_PROTOCOL     *This,
  IN EFI_MANAGED_NETWORK_CONFIG_DATA  *MnpConfigData OPTIONAL
  );

/**
  Translates an IP multicast address to a hardware (MAC) multicast address. This
  function may be unsupported in some MNP implementations.

  The McastIpToMac() function translates an IP multicast address to a hardware
  (MAC) multicast address. This function may be implemented by calling the
  underlying EFI_SIMPLE_NETWORK. MCastIpToMac() function, which may also be
  unsupported in some MNP implementations.

  @param[in]  This        Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  Ipv6Flag    Set to TRUE to if IpAddress is an IPv6 multicast address.
                          Set to FALSE if IpAddress is an IPv4 multicast address.
  @param[in]  IpAddress   Pointer to the multicast IP address (in network byte
                          order) to convert.
  @param[out] MacAddress  Pointer to the resulting multicast MAC address.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER One of the following conditions is TRUE:
                                 * This is NULL.
                                 * IpAddress is NULL.
                                 * IpAddress is not a valid multicast IP
                                   address.
                                 * MacAddress is NULL.
  @retval EFI_NOT_STARTED       This MNP child driver instance has not been
                                configured.
  @retval EFI_UNSUPPORTED       The requested feature is unsupported in this
                                MNP implementation.
  @retval EFI_DEVICE_ERROR      An unexpected network or system error occurred.
  @retval Others                The address could not be converted.
**/
EFI_STATUS
EFIAPI
MnpMcastIpToMac (
  IN     EFI_MANAGED_NETWORK_PROTOCOL  *This,
  IN     BOOLEAN                       Ipv6Flag,
  IN     EFI_IP_ADDRESS                *IpAddress,
  OUT EFI_MAC_ADDRESS                  *MacAddress
  );

/**
  Enables and disables receive filters for multicast address. This function may
  be unsupported in some MNP implementations.

  The Groups() function only adds and removes multicast MAC addresses from the
  filter list. The MNP driver does not transmit or process Internet Group
  Management Protocol (IGMP) packets. If JoinFlag is FALSE and MacAddress is
  NULL, then all joined groups are left.

  @param[in]  This        Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  JoinFlag    Set to TRUE to join this multicast group.
                          Set to FALSE to leave this multicast group.
  @param[in]  MacAddress  Pointer to the multicast MAC group (address) to join or
                          leave.

  @retval EFI_SUCCESS           The requested operation completed successfully.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                * This is NULL.
                                * JoinFlag is TRUE and MacAddress is NULL.
                                * MacAddress is not a valid multicast MAC
                                  address.
                                * The MNP multicast group settings are
                                  unchanged.
  @retval EFI_NOT_STARTED       This MNP child driver instance has not been
                                configured.
  @retval EFI_ALREADY_STARTED   The supplied multicast group is already joined.
  @retval EFI_NOT_FOUND         The supplied multicast group is not joined.
  @retval EFI_DEVICE_ERROR      An unexpected network or system error occurred.
                                The MNP child driver instance has been reset to
                                startup defaults.
  @retval EFI_UNSUPPORTED       The requested feature is unsupported in this MNP
                                implementation.
  @retval Others                The requested operation could not be completed.
                                The MNP multicast group settings are unchanged.

**/
EFI_STATUS
EFIAPI
MnpGroups (
  IN EFI_MANAGED_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                       JoinFlag,
  IN EFI_MAC_ADDRESS               *MacAddress OPTIONAL
  );

/**
  Places asynchronous outgoing data packets into the transmit queue.

  The Transmit() function places a completion token into the transmit packet
  queue. This function is always asynchronous.
  The caller must fill in the Token.Event and Token.TxData fields in the
  completion token, and these fields cannot be NULL. When the transmit operation
  completes, the MNP updates the Token.Status field and the Token.Event is
  signaled.
  Note: There may be a performance penalty if the packet needs to be
  defragmented before it can be transmitted by the network device. Systems in
  which performance is critical should review the requirements and features of
  the underlying communications device and drivers.


  @param[in]  This    Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  Token   Pointer to a token associated with the transmit data
                      descriptor. Type EFI_MANAGED_NETWORK_COMPLETION_TOKEN
                      is defined in "Related Definitions" below.

  @retval EFI_SUCCESS            The transmit completion token was cached.
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is
                                 TRUE:
                                 * This is NULL.
                                 * Token is NULL.
                                 * Token.Event is NULL.
                                 * Token.TxData is NULL.
                                 * Token.TxData.DestinationAddress is not
                                   NULL and Token.TxData.HeaderLength is zero.
                                 * Token.TxData.FragmentCount is zero.
                                 * (Token.TxData.HeaderLength +
                                   Token.TxData.DataLength) is not equal to the
                                   sum of the
                                   Token.TxData.FragmentTable[].FragmentLength
                                   fields.
                                 * One or more of the
                                   Token.TxData.FragmentTable[].FragmentLength
                                   fields is zero.
                                 * One or more of the
                                   Token.TxData.FragmentTable[].FragmentBufferfields
                                   is NULL.
                                 * Token.TxData.DataLength is greater than MTU.
  @retval EFI_ACCESS_DENIED      The transmit completion token is already in the
                                 transmit queue.
  @retval EFI_OUT_OF_RESOURCES   The transmit data could not be queued due to a
                                 lack of system resources (usually memory).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The MNP child driver instance has been reset to
                                 startup defaults.
  @retval EFI_NOT_READY          The transmit request could not be queued because
                                 the transmit queue is full.

**/
EFI_STATUS
EFIAPI
MnpTransmit (
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  );

/**
  Aborts an asynchronous transmit or receive request.

  The Cancel() function is used to abort a pending transmit or receive request.
  If the token is in the transmit or receive request queues, after calling this
  function, Token.Status will be set to EFI_ABORTED and then Token.Event will be
  signaled. If the token is not in one of the queues, which usually means that
  the asynchronous operation has completed, this function will not signal the
  token and EFI_NOT_FOUND is returned.

  @param[in]  This     Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  Token    Pointer to a token that has been issued by
                       EFI_MANAGED_NETWORK_PROTOCOL.Transmit() or
                       EFI_MANAGED_NETWORK_PROTOCOL.Receive(). If NULL, all
                       pending tokens are aborted.

  @retval EFI_SUCCESS            The asynchronous I/O request was aborted and
                                 Token.Event was signaled. When Token is NULL,
                                 all pending requests were aborted and their
                                 events were signaled.
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_FOUND          When Token is not NULL, the asynchronous I/O
                                 request was not found in the transmit or
                                 receive queue. It has either completed or was
                                 not issued by Transmit() and Receive().

**/
EFI_STATUS
EFIAPI
MnpCancel (
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token OPTIONAL
  );

/**
  Places an asynchronous receiving request into the receiving queue.

  The Receive() function places a completion token into the receive packet
  queue. This function is always asynchronous.
  The caller must fill in the Token.Event field in the completion token, and
  this field cannot be NULL. When the receive operation completes, the MNP
  updates the Token.Status and Token.RxData fields and the Token.Event is
  signaled.

  @param[in]  This      Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.
  @param[in]  Token     Pointer to a token associated with the receive
                        data descriptor. Type
                        EFI_MANAGED_NETWORK_COMPLETION_TOKEN is defined in
                        EFI_MANAGED_NETWORK_PROTOCOL.Transmit().

  @retval EFI_SUCCESS            The receive completion token was cached.
  @retval EFI_NOT_STARTED        This MNP child driver instance has not been
                                 configured.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is
                                 TRUE:
                                 * This is NULL.
                                 * Token is NULL.
                                 * Token.Event is NULL
  @retval EFI_OUT_OF_RESOURCES   The transmit data could not be queued due to a
                                 lack of system resources (usually memory).
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
                                 The MNP child driver instance has been reset to
                                 startup defaults.
  @retval EFI_ACCESS_DENIED      The receive completion token was already in the
                                 receive queue.
  @retval EFI_NOT_READY          The receive request could not be queued because
                                 the receive queue is full.

**/
EFI_STATUS
EFIAPI
MnpReceive (
  IN EFI_MANAGED_NETWORK_PROTOCOL          *This,
  IN EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token
  );

/**
  Polls for incoming data packets and processes outgoing data packets.

  The Poll() function can be used by network drivers and applications to
  increase the rate that data packets are moved between the communications
  device and the transmit and receive queues.
  Normally, a periodic timer event internally calls the Poll() function. But, in
  some systems, the periodic timer event may not call Poll() fast enough to
  transmit and/or receive all data packets without missing packets. Drivers and
  applications that are experiencing packet loss should try calling the Poll()
  function more often.

  @param[in]  This         Pointer to the EFI_MANAGED_NETWORK_PROTOCOL instance.

  @retval EFI_SUCCESS      Incoming or outgoing data was processed.
  @retval EFI_NOT_STARTED  This MNP child driver instance has not been
                           configured.
  @retval EFI_DEVICE_ERROR An unexpected system or network error occurred. The
                           MNP child driver instance has been reset to startup
                           defaults.
  @retval EFI_NOT_READY    No incoming or outgoing data was processed. Consider
                           increasing the polling rate.
  @retval EFI_TIMEOUT      Data was dropped out of the transmit and/or receive
                           queue. Consider increasing the polling rate.

**/
EFI_STATUS
EFIAPI
MnpPoll (
  IN EFI_MANAGED_NETWORK_PROTOCOL  *This
  );

/**
  Configure the Snp receive filters according to the instances' receive filter
  settings.

  @param[in]  MnpDeviceData         Pointer to the mnp device context data.

  @retval     EFI_SUCCESS           The receive filters is configured.
  @retval     EFI_OUT_OF_RESOURCES  The receive filters can't be configured due
                                    to lack of memory resource.

**/
EFI_STATUS
MnpConfigReceiveFilters (
  IN MNP_DEVICE_DATA  *MnpDeviceData
  );

#endif
