/** @file
  EFI_DHCP4_PROTOCOL as defined in UEFI 2.0.
  EFI_DHCP4_SERVICE_BINDING_PROTOCOL as defined in UEFI 2.0.
  These protocols are used to collect configuration information for the EFI IPv4 Protocol
  drivers and to provide DHCPv4 server and PXE boot server discovery services.

  Copyright (c) 2006 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_DHCP4_PROTOCOL_H__
#define __EFI_DHCP4_PROTOCOL_H__

#define EFI_DHCP4_PROTOCOL_GUID \
  { \
    0x8a219718, 0x4ef5, 0x4761, {0x91, 0xc8, 0xc0, 0xf0, 0x4b, 0xda, 0x9e, 0x56 } \
  }

#define EFI_DHCP4_SERVICE_BINDING_PROTOCOL_GUID \
  { \
    0x9d9a39d8, 0xbd42, 0x4a73, {0xa4, 0xd5, 0x8e, 0xe9, 0x4b, 0xe1, 0x13, 0x80 } \
  }

typedef struct _EFI_DHCP4_PROTOCOL EFI_DHCP4_PROTOCOL;


#pragma pack(1)
typedef struct {
  UINT8               OpCode;
  UINT8               Length;
  UINT8               Data[1];
} EFI_DHCP4_PACKET_OPTION;
#pragma pack()


#pragma pack(1)
typedef struct {
    UINT8             OpCode;
    UINT8             HwType;
    UINT8             HwAddrLen;
    UINT8             Hops;
    UINT32            Xid;
    UINT16            Seconds;
    UINT16            Reserved;
    EFI_IPv4_ADDRESS  ClientAddr;       ///< Client IP address from client
    EFI_IPv4_ADDRESS  YourAddr;         ///< Client IP address from server
    EFI_IPv4_ADDRESS  ServerAddr;       ///< IP address of next server in bootstrap
    EFI_IPv4_ADDRESS  GatewayAddr;      ///< Relay agent IP address
    UINT8             ClientHwAddr[16]; ///< Client hardware address
    CHAR8             ServerName[64];
    CHAR8             BootFileName[128];
}EFI_DHCP4_HEADER;
#pragma pack()


#pragma pack(1)
typedef struct {
  UINT32              Size;
  UINT32              Length;

  struct {
    EFI_DHCP4_HEADER  Header;
    UINT32            Magik;
    UINT8             Option[1];
  } Dhcp4;
} EFI_DHCP4_PACKET;
#pragma pack()


typedef enum {
  Dhcp4Stopped        = 0x0,
  Dhcp4Init           = 0x1,
  Dhcp4Selecting      = 0x2,
  Dhcp4Requesting     = 0x3,
  Dhcp4Bound          = 0x4,
  Dhcp4Renewing       = 0x5,
  Dhcp4Rebinding      = 0x6,
  Dhcp4InitReboot     = 0x7,
  Dhcp4Rebooting      = 0x8
} EFI_DHCP4_STATE;


typedef enum{
  Dhcp4SendDiscover   = 0x01,
  Dhcp4RcvdOffer      = 0x02,
  Dhcp4SelectOffer    = 0x03,
  Dhcp4SendRequest    = 0x04,
  Dhcp4RcvdAck        = 0x05,
  Dhcp4RcvdNak        = 0x06,
  Dhcp4SendDecline    = 0x07,
  Dhcp4BoundCompleted = 0x08,
  Dhcp4EnterRenewing  = 0x09,
  Dhcp4EnterRebinding = 0x0a,
  Dhcp4AddressLost    = 0x0b,
  Dhcp4Fail           = 0x0c
} EFI_DHCP4_EVENT;

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
typedef
EFI_STATUS
(EFIAPI *EFI_DHCP4_CALLBACK)(
  IN  EFI_DHCP4_PROTOCOL         *This,
  IN  VOID                       *Context,
  IN  EFI_DHCP4_STATE            CurrentState,
  IN  EFI_DHCP4_EVENT            Dhcp4Event,
  IN  EFI_DHCP4_PACKET           *Packet     OPTIONAL,
  OUT EFI_DHCP4_PACKET           **NewPacket OPTIONAL
  );


typedef struct {
  UINT32                      DiscoverTryCount;
  UINT32                      *DiscoverTimeout;
  UINT32                      RequestTryCount;
  UINT32                      *RequestTimeout;
  EFI_IPv4_ADDRESS            ClientAddress;
  EFI_DHCP4_CALLBACK          Dhcp4Callback;
  void                        *CallbackContext;
  UINT32                      OptionCount;
  EFI_DHCP4_PACKET_OPTION     **OptionList;
} EFI_DHCP4_CONFIG_DATA;


typedef struct {
  EFI_DHCP4_STATE             State;
  EFI_DHCP4_CONFIG_DATA       ConfigData;
  EFI_IPv4_ADDRESS            ClientAddress;
  EFI_MAC_ADDRESS             ClientMacAddress;
  EFI_IPv4_ADDRESS            ServerAddress;
  EFI_IPv4_ADDRESS            RouterAddress;
  EFI_IPv4_ADDRESS            SubnetMask;
  UINT32                      LeaseTime;
  EFI_DHCP4_PACKET            *ReplyPacket;
} EFI_DHCP4_MODE_DATA;


typedef struct {
  EFI_IPv4_ADDRESS            ListenAddress;
  EFI_IPv4_ADDRESS            SubnetMask;
  UINT16                      ListenPort;
} EFI_DHCP4_LISTEN_POINT;


typedef struct {
  EFI_STATUS              Status;
  EFI_EVENT               CompletionEvent;
  EFI_IPv4_ADDRESS        RemoteAddress;
  UINT16                  RemotePort;
  EFI_IPv4_ADDRESS        GatewayAddress;
  UINT32                  ListenPointCount;
  EFI_DHCP4_LISTEN_POINT  *ListenPoints;
  UINT32                  TimeoutValue;
  EFI_DHCP4_PACKET        *Packet;
  UINT32                  ResponseCount;
  EFI_DHCP4_PACKET        *ResponseList;
} EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN;


/**
  Returns the current operating mode and cached data packet for the EFI DHCPv4 Protocol driver.
  
  The GetModeData() function returns the current operating mode and cached data
  packet for the EFI DHCPv4 Protocol driver.

  @param  This          Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param  Dhcp4ModeData Pointer to storage for the EFI_DHCP4_MODE_DATA structure.

  @retval EFI_SUCCESS           The mode data was returned.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DHCP4_GET_MODE_DATA)(
  IN  EFI_DHCP4_PROTOCOL      *This,
  OUT EFI_DHCP4_MODE_DATA     *Dhcp4ModeData
  );

/**
  Initializes, changes, or resets the operational settings for the EFI DHCPv4 Protocol driver.

  The Configure() function is used to initialize, change, or reset the operational
  settings of the EFI DHCPv4 Protocol driver for the communication device on which
  the EFI DHCPv4 Service Binding Protocol is installed. This function can be
  successfully called only if both of the following are true:
  * This instance of the EFI DHCPv4 Protocol driver is in the Dhcp4Stopped, Dhcp4Init,
    Dhcp4InitReboot, or Dhcp4Bound states.
  * No other EFI DHCPv4 Protocol driver instance that is controlled by this EFI
    DHCPv4 Service Binding Protocol driver instance has configured this EFI DHCPv4
    Protocol driver.
  When this driver is in the Dhcp4Stopped state, it can transfer into one of the
  following two possible initial states:
  * Dhcp4Init
  * Dhcp4InitReboot
  The driver can transfer into these states by calling Configure() with a non-NULL
  Dhcp4CfgData. The driver will transfer into the appropriate state based on the
  supplied client network address in the ClientAddress parameter and DHCP options
  in the OptionList parameter as described in RFC 2131.
  When Configure() is called successfully while Dhcp4CfgData is set to NULL, the
  default configuring data will be reset in the EFI DHCPv4 Protocol driver and
  the state of the EFI DHCPv4 Protocol driver will not be changed. If one instance
  wants to make it possible for another instance to configure the EFI DHCPv4 Protocol
  driver, it must call this function with Dhcp4CfgData set to NULL.

  @param  This                   Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param  Dhcp4CfgData           Pointer to the EFI_DHCP4_CONFIG_DATA.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the Dhcp4Init or
                                Dhcp4InitReboot state, if the original state of this driver
                                was Dhcp4Stopped and the value of Dhcp4CfgData was
                                not NULL. Otherwise, the state was left unchanged.
  @retval EFI_ACCESS_DENIED     This instance of the EFI DHCPv4 Protocol driver was not in the
                                Dhcp4Stopped, Dhcp4Init, Dhcp4InitReboot, or Dhcp4Bound state;
                                Or onother instance of this EFI DHCPv4 Protocol driver is already
                                in a valid configured state.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DHCP4_CONFIGURE)(
  IN EFI_DHCP4_PROTOCOL       *This,
  IN EFI_DHCP4_CONFIG_DATA    *Dhcp4CfgData  OPTIONAL
  );


/**
  Starts the DHCP configuration process.

  The Start() function starts the DHCP configuration process. This function can
  be called only when the EFI DHCPv4 Protocol driver is in the Dhcp4Init or
  Dhcp4InitReboot state.
  If the DHCP process completes successfully, the state of the EFI DHCPv4 Protocol
  driver will be transferred through Dhcp4Selecting and Dhcp4Requesting to the
  Dhcp4Bound state. The CompletionEvent will then be signaled if it is not NULL.
  If the process aborts, either by the user or by some unexpected network error,
  the state is restored to the Dhcp4Init state. The Start() function can be called
  again to restart the process.
  Refer to RFC 2131 for precise state transitions during this process. At the
  time when each event occurs in this process, the callback function that was set
  by EFI_DHCP4_PROTOCOL.Configure() will be called and the user can take this
  opportunity to control the process.
  
  @param  This            Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param  CompletionEvent If not NULL, indicates the event that will be signaled when the
                          EFI DHCPv4 Protocol driver is transferred into the
                          Dhcp4Bound state or when the DHCP process is aborted.
                          EFI_DHCP4_PROTOCOL.GetModeData() can be called to
                          check the completion status. If NULL,
                          EFI_DHCP4_PROTOCOL.Start() will wait until the driver
                          is transferred into the Dhcp4Bound state or the process fails.

  @retval EFI_SUCCESS           The DHCP configuration process has started, or it has completed
                                when CompletionEvent is NULL.
  @retval EFI_NOT_STARTED       The EFI DHCPv4 Protocol driver is in the Dhcp4Stopped
                                state. EFI_DHCP4_PROTOCOL. Configure() needs to be called.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_TIMEOUT           The DHCP configuration process failed because no response was
                                received from the server within the specified timeout value.
  @retval EFI_ABORTED           The user aborted the DHCP process.
  @retval EFI_ALREADY_STARTED   Some other EFI DHCPv4 Protocol instance already started the
                                DHCP process.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DHCP4_START)(
  IN EFI_DHCP4_PROTOCOL       *This,
  IN EFI_EVENT                CompletionEvent   OPTIONAL
  );

/**
  Extends the lease time by sending a request packet.
  
  The RenewRebind() function is used to manually extend the lease time when the
  EFI DHCPv4 Protocol driver is in the Dhcp4Bound state and the lease time has
  not expired yet. This function will send a request packet to the previously
  found server (or to any server when RebindRequest is TRUE) and transfer the
  state into the Dhcp4Renewing state (or Dhcp4Rebinding when RebindingRequest is
  TRUE). When a response is received, the state is returned to Dhcp4Bound.
  If no response is received before the try count is exceeded (the RequestTryCount
  field that is specified in EFI_DHCP4_CONFIG_DATA) but before the lease time that
  was issued by the previous server expires, the driver will return to the Dhcp4Bound
  state and the previous configuration is restored. The outgoing and incoming packets
  can be captured by the EFI_DHCP4_CALLBACK function.

  @param  This            Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param  RebindRequest   If TRUE, this function broadcasts the request packets and enters
                          the Dhcp4Rebinding state. Otherwise, it sends a unicast
                          request packet and enters the Dhcp4Renewing state.
  @param  CompletionEvent If not NULL, this event is signaled when the renew/rebind phase
                          completes or some error occurs.
                          EFI_DHCP4_PROTOCOL.GetModeData() can be called to
                          check the completion status. If NULL,
                          EFI_DHCP4_PROTOCOL.RenewRebind() will busy-wait
                          until the DHCP process finishes.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the
                                Dhcp4Renewing state or is back to the Dhcp4Bound state.
  @retval EFI_NOT_STARTED       The EFI DHCPv4 Protocol driver is in the Dhcp4Stopped
                                state. EFI_DHCP4_PROTOCOL.Configure() needs to
                                be called.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_TIMEOUT           There was no response from the server when the try count was
                                exceeded.
  @retval EFI_ACCESS_DENIED     The driver is not in the Dhcp4Bound state.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DHCP4_RENEW_REBIND)(
  IN EFI_DHCP4_PROTOCOL       *This,
  IN BOOLEAN                  RebindRequest,
  IN EFI_EVENT                CompletionEvent  OPTIONAL
  );

/**
  Releases the current address configuration.

  The Release() function releases the current configured IP address by doing either
  of the following:
  * Sending a DHCPRELEASE packet when the EFI DHCPv4 Protocol driver is in the
    Dhcp4Bound state
  * Setting the previously assigned IP address that was provided with the
    EFI_DHCP4_PROTOCOL.Configure() function to 0.0.0.0 when the driver is in
    Dhcp4InitReboot state
  After a successful call to this function, the EFI DHCPv4 Protocol driver returns
  to the Dhcp4Init state and any subsequent incoming packets will be discarded silently.

  @param  This                  Pointer to the EFI_DHCP4_PROTOCOL instance.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the Dhcp4Init phase.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_ACCESS_DENIED     The EFI DHCPv4 Protocol driver is not Dhcp4InitReboot state.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DHCP4_RELEASE)(
  IN EFI_DHCP4_PROTOCOL       *This
  );

/**
  Stops the current address configuration.
  
  The Stop() function is used to stop the DHCP configuration process. After this
  function is called successfully, the EFI DHCPv4 Protocol driver is transferred
  into the Dhcp4Stopped state. EFI_DHCP4_PROTOCOL.Configure() needs to be called
  before DHCP configuration process can be started again. This function can be
  called when the EFI DHCPv4 Protocol driver is in any state.

  @param  This                  Pointer to the EFI_DHCP4_PROTOCOL instance.

  @retval EFI_SUCCESS           The EFI DHCPv4 Protocol driver is now in the Dhcp4Stopped phase.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DHCP4_STOP)(
  IN EFI_DHCP4_PROTOCOL       *This
  );

/**
  Builds a DHCP packet, given the options to be appended or deleted or replaced.

  The Build() function is used to assemble a new packet from the original packet
  by replacing or deleting existing options or appending new options. This function
  does not change any state of the EFI DHCPv4 Protocol driver and can be used at
  any time.

  @param  This        Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param  SeedPacket  Initial packet to be used as a base for building new packet.
  @param  DeleteCount Number of opcodes in the DeleteList.
  @param  DeleteList  List of opcodes to be deleted from the seed packet.
                      Ignored if DeleteCount is zero.
  @param  AppendCount Number of entries in the OptionList.
  @param  AppendList  Pointer to a DHCP option list to be appended to SeedPacket.
                      If SeedPacket also contains options in this list, they are
                      replaced by new options (except pad option). Ignored if
                      AppendCount is zero. Type EFI_DHCP4_PACKET_OPTION
  @param  NewPacket   Pointer to storage for the pointer to the new allocated packet.
                      Use the EFI Boot Service FreePool() on the resulting pointer
                      when done with the packet.

  @retval EFI_SUCCESS           The new packet was built.
  @retval EFI_OUT_OF_RESOURCES  Storage for the new packet could not be allocated.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DHCP4_BUILD)(
  IN  EFI_DHCP4_PROTOCOL      *This,
  IN  EFI_DHCP4_PACKET        *SeedPacket,
  IN  UINT32                  DeleteCount,
  IN  UINT8                   *DeleteList         OPTIONAL,
  IN  UINT32                  AppendCount,
  IN  EFI_DHCP4_PACKET_OPTION *AppendList[]       OPTIONAL,
  OUT EFI_DHCP4_PACKET        **NewPacket
  );


/**
  Transmits a DHCP formatted packet and optionally waits for responses.
  
  The TransmitReceive() function is used to transmit a DHCP packet and optionally
  wait for the response from servers. This function does not change the state of
  the EFI DHCPv4 Protocol driver and thus can be used at any time.

  @param  This    Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param  Token   Pointer to the EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN structure.

  @retval EFI_SUCCESS           The packet was successfully queued for transmission.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_NOT_READY         The previous call to this function has not finished yet. Try to call
                                this function after collection process completes.
  @retval EFI_NO_MAPPING        The default station address is not available yet.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval Others                Some other unexpected error occurred.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DHCP4_TRANSMIT_RECEIVE)(
  IN EFI_DHCP4_PROTOCOL                *This,
  IN EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN  *Token
  );


/**
  Parses the packed DHCP option data.
  
  The Parse() function is used to retrieve the option list from a DHCP packet.
  If *OptionCount isn’t zero, and there is enough space for all the DHCP options
  in the Packet, each element of PacketOptionList is set to point to somewhere in
  the Packet->Dhcp4.Option where a new DHCP option begins. If RFC3396 is supported,
  the caller should reassemble the parsed DHCP options to get the finial result.
  If *OptionCount is zero or there isn’t enough space for all of them, the number
  of DHCP options in the Packet is returned in OptionCount.

  @param  This             Pointer to the EFI_DHCP4_PROTOCOL instance.
  @param  Packet           Pointer to packet to be parsed.
  @param  OptionCount      On input, the number of entries in the PacketOptionList.
                           On output, the number of entries that were written into the
                           PacketOptionList.
  @param  PacketOptionList List of packet option entries to be filled in. End option or pad
                           options are not included.

  @retval EFI_SUCCESS           The packet was successfully parsed.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_BUFFER_TOO_SMALL  One or more of the following conditions is TRUE:
                                1) *OptionCount is smaller than the number of options that
                                were found in the Packet.
                                2) PacketOptionList is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DHCP4_PARSE)(
  IN EFI_DHCP4_PROTOCOL        *This,
  IN EFI_DHCP4_PACKET          *Packet,
  IN OUT UINT32                *OptionCount,
  OUT EFI_DHCP4_PACKET_OPTION  *PacketOptionList[]  OPTIONAL
  );

///
/// This protocol is used to collect configuration information for the EFI IPv4 Protocol drivers
/// and to provide DHCPv4 server and PXE boot server discovery services.
///
struct _EFI_DHCP4_PROTOCOL {
  EFI_DHCP4_GET_MODE_DATA      GetModeData;
  EFI_DHCP4_CONFIGURE          Configure;
  EFI_DHCP4_START              Start;
  EFI_DHCP4_RENEW_REBIND       RenewRebind;
  EFI_DHCP4_RELEASE            Release;
  EFI_DHCP4_STOP               Stop;
  EFI_DHCP4_BUILD              Build;
  EFI_DHCP4_TRANSMIT_RECEIVE   TransmitReceive;
  EFI_DHCP4_PARSE              Parse;
};

extern EFI_GUID gEfiDhcp4ProtocolGuid;
extern EFI_GUID gEfiDhcp4ServiceBindingProtocolGuid;

#endif
