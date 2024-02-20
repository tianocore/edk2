/** @file
  Dhcp6 internal data structure and definition declaration.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_DHCP6_IMPL_H__
#define __EFI_DHCP6_IMPL_H__

#include <Uefi.h>

#include <IndustryStandard/Dhcp.h>

#include <Protocol/Dhcp6.h>
#include <Protocol/Udp6.h>
#include <Protocol/Ip6Config.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/DriverBinding.h>

#include <Library/UdpIoLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/NetLib.h>
#include <Library/PrintLib.h>
#include <Guid/ZeroGuid.h>

typedef struct _DHCP6_IA_CB     DHCP6_IA_CB;
typedef struct _DHCP6_INF_CB    DHCP6_INF_CB;
typedef struct _DHCP6_TX_CB     DHCP6_TX_CB;
typedef struct _DHCP6_SERVICE   DHCP6_SERVICE;
typedef struct _DHCP6_INSTANCE  DHCP6_INSTANCE;

#include "Dhcp6Utility.h"
#include "Dhcp6Io.h"
#include "Dhcp6Driver.h"

#define DHCP6_SERVICE_SIGNATURE   SIGNATURE_32 ('D', 'H', '6', 'S')
#define DHCP6_INSTANCE_SIGNATURE  SIGNATURE_32 ('D', 'H', '6', 'I')

#define DHCP6_PACKET_ALL        0
#define DHCP6_PACKET_STATEFUL   1
#define DHCP6_PACKET_STATELESS  2

#define DHCP6_BASE_PACKET_SIZE  1024

#define DHCP6_PORT_CLIENT  546
#define DHCP6_PORT_SERVER  547

#define DHCP_CHECK_MEDIA_WAITING_TIME  EFI_TIMER_PERIOD_SECONDS(20)

#define DHCP6_INSTANCE_FROM_THIS(Instance)  CR ((Instance), DHCP6_INSTANCE, Dhcp6, DHCP6_INSTANCE_SIGNATURE)
#define DHCP6_SERVICE_FROM_THIS(Service)    CR ((Service), DHCP6_SERVICE, ServiceBinding, DHCP6_SERVICE_SIGNATURE)

//
// For more information on DHCP options see RFC 8415, Section 21.1
//
// The format of DHCP options is:
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |          option-code          |           option-len          |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                          option-data                          |
//    |                      (option-len octets)                      |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
#define DHCP6_SIZE_OF_OPT_CODE  (sizeof (((EFI_DHCP6_PACKET_OPTION *)0)->OpCode))
#define DHCP6_SIZE_OF_OPT_LEN   (sizeof (((EFI_DHCP6_PACKET_OPTION *)0)->OpLen))

// Combined size of Code and Length
#define DHCP6_SIZE_OF_COMBINED_CODE_AND_LEN  (DHCP6_SIZE_OF_OPT_CODE + \
                                              DHCP6_SIZE_OF_OPT_LEN)

STATIC_ASSERT (
  DHCP6_SIZE_OF_COMBINED_CODE_AND_LEN == 4,
  "Combined size of Code and Length must be 4 per RFC 8415"
  );

// Offset to the length is just past the code
#define DHCP6_OFFSET_OF_OPT_LEN(a)  (a + DHCP6_SIZE_OF_OPT_CODE)
STATIC_ASSERT (
  DHCP6_OFFSET_OF_OPT_LEN (0) == 2,
  "Offset of length is + 2 past start of option"
  );

#define DHCP6_OFFSET_OF_OPT_DATA(a)  (a + DHCP6_SIZE_OF_COMBINED_CODE_AND_LEN)
STATIC_ASSERT (
  DHCP6_OFFSET_OF_OPT_DATA (0) == 4,
  "Offset to option data should be +4 from start of option"
  );
//
// Identity Association options (both NA (Non-Temporary) and TA (Temporary Association))
// are defined in RFC 8415 and are a deriviation of a TLV stucture
// For more information on IA_NA see Section 21.4
// For more information on IA_TA see Section 21.5
//
//
//  The format of IA_NA and IA_TA option:
//
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |          OPTION_IA_NA         |          option-len           |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                        IAID (4 octets)                        |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                        T1 (only for IA_NA)                    |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                        T2 (only for IA_NA)                    |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |                                                               |
//    .                  IA_NA-options/IA_TA-options                  .
//    .                                                               .
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
#define DHCP6_SIZE_OF_IAID           (sizeof(UINT32))
#define DHCP6_SIZE_OF_TIME_INTERVAL  (sizeof(UINT32))

// Combined size of IAID, T1, and T2
#define DHCP6_SIZE_OF_COMBINED_IAID_T1_T2  (DHCP6_SIZE_OF_IAID +  \
                                            DHCP6_SIZE_OF_TIME_INTERVAL + \
                                            DHCP6_SIZE_OF_TIME_INTERVAL)
STATIC_ASSERT (
  DHCP6_SIZE_OF_COMBINED_IAID_T1_T2 == 12,
  "Combined size of IAID, T1, T2 must be 12 per RFC 8415"
  );

// This is the size of IA_TA without options
#define DHCP6_MIN_SIZE_OF_IA_TA  (DHCP6_SIZE_OF_COMBINED_CODE_AND_LEN + \
                                  DHCP6_SIZE_OF_IAID)
STATIC_ASSERT (
  DHCP6_MIN_SIZE_OF_IA_TA == 8,
  "Minimum combined size of IA_TA per RFC 8415"
  );

// Offset to a IA_TA inner option
#define DHCP6_OFFSET_OF_IA_TA_INNER_OPT(a)  (a + DHCP6_MIN_SIZE_OF_IA_TA)
STATIC_ASSERT (
  DHCP6_OFFSET_OF_IA_TA_INNER_OPT (0) == 8,
  "Offset of IA_TA Inner option is + 8 past start of option"
  );

// This is the size of IA_NA without options (16)
#define DHCP6_MIN_SIZE_OF_IA_NA  DHCP6_SIZE_OF_COMBINED_CODE_AND_LEN + \
                                 DHCP6_SIZE_OF_COMBINED_IAID_T1_T2
STATIC_ASSERT (
  DHCP6_MIN_SIZE_OF_IA_NA == 16,
  "Minimum combined size of IA_TA per RFC 8415"
  );

#define DHCP6_OFFSET_OF_IA_NA_INNER_OPT(a)  (a + DHCP6_MIN_SIZE_OF_IA_NA)
STATIC_ASSERT (
  DHCP6_OFFSET_OF_IA_NA_INNER_OPT (0) == 16,
  "Offset of IA_NA Inner option is + 16 past start of option"
  );

#define DHCP6_OFFSET_OF_IA_NA_T1(a)  (a + \
                                   DHCP6_SIZE_OF_COMBINED_CODE_AND_LEN + \
                                   DHCP6_SIZE_OF_IAID)
STATIC_ASSERT (
  DHCP6_OFFSET_OF_IA_NA_T1 (0) == 8,
  "Offset of IA_NA Inner option is + 8 past start of option"
  );

#define DHCP6_OFFSET_OF_IA_NA_T2(a)  (a + \
                                   DHCP6_SIZE_OF_COMBINED_CODE_AND_LEN +\
                                   DHCP6_SIZE_OF_IAID + \
                                   DHCP6_SIZE_OF_TIME_INTERVAL)
STATIC_ASSERT (
  DHCP6_OFFSET_OF_IA_NA_T2 (0) == 12,
  "Offset of IA_NA Inner option is + 12 past start of option"
  );

//
// For more information see RFC 8415 Section 21.13
//
// The format of the Status Code Option:
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |       OPTION_STATUS_CODE      |         option-len            |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |          status-code          |                               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
//    .                                                               .
//    .                        status-message                         .
//    .                                                               .
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
#define DHCP6_OFFSET_OF_STATUS_CODE(a)  (a + DHCP6_SIZE_OF_COMBINED_CODE_AND_LEN)
STATIC_ASSERT (
  DHCP6_OFFSET_OF_STATUS_CODE (0) == 4,
  "Offset of status is + 4 past start of option"
  );

extern EFI_IPv6_ADDRESS    mAllDhcpRelayAndServersAddress;
extern EFI_DHCP6_PROTOCOL  gDhcp6ProtocolTemplate;

//
// Control block for each IA.
//
struct _DHCP6_IA_CB {
  EFI_DHCP6_IA    *Ia;
  UINT32          T1;
  UINT32          T2;
  UINT32          AllExpireTime;
  UINT32          LeaseTime;
};

//
// Control block for each transmitted message.
//
struct _DHCP6_TX_CB {
  LIST_ENTRY                  Link;
  UINT32                      Xid;
  EFI_DHCP6_PACKET            *TxPacket;
  EFI_DHCP6_RETRANSMISSION    RetryCtl;
  UINT32                      RetryCnt;
  UINT32                      RetryExp;
  UINT32                      RetryLos;
  UINT32                      TickTime;
  UINT16                      *Elapsed;
  BOOLEAN                     SolicitRetry;
};

//
// Control block for each info-request message.
//
struct _DHCP6_INF_CB {
  LIST_ENTRY                 Link;
  UINT32                     Xid;
  EFI_DHCP6_INFO_CALLBACK    ReplyCallback;
  VOID                       *CallbackContext;
  EFI_EVENT                  TimeoutEvent;
};

//
// Control block for Dhcp6 instance, it's per configuration data.
//
struct _DHCP6_INSTANCE {
  UINT32                   Signature;
  EFI_HANDLE               Handle;
  DHCP6_SERVICE            *Service;
  LIST_ENTRY               Link;
  EFI_DHCP6_PROTOCOL       Dhcp6;
  EFI_EVENT                Timer;
  EFI_DHCP6_CONFIG_DATA    *Config;
  EFI_DHCP6_IA             *CacheIa;
  DHCP6_IA_CB              IaCb;
  LIST_ENTRY               TxList;
  LIST_ENTRY               InfList;
  EFI_DHCP6_PACKET         *AdSelect;
  UINT8                    AdPref;
  EFI_IPv6_ADDRESS         *Unicast;
  volatile EFI_STATUS      UdpSts;
  BOOLEAN                  InDestroy;
  BOOLEAN                  MediaPresent;
  //
  // StartTime is used to calculate the 'elapsed-time' option. Refer to RFC3315,
  // the elapsed-time is amount of time since the client began its current DHCP transaction.
  //
  UINT64                   StartTime;
};

//
// Control block for Dhcp6 service, it's per Nic handle.
//
struct _DHCP6_SERVICE {
  UINT32                          Signature;
  EFI_HANDLE                      Controller;
  EFI_HANDLE                      Image;
  EFI_SERVICE_BINDING_PROTOCOL    ServiceBinding;
  EFI_SIMPLE_NETWORK_PROTOCOL     *Snp;
  EFI_IP6_CONFIG_PROTOCOL         *Ip6Cfg;
  EFI_DHCP6_DUID                  *ClientId;
  UDP_IO                          *UdpIo;
  UINT32                          Xid;
  LIST_ENTRY                      Child;
  UINTN                           NumOfChild;
};

/**
  Starts the DHCPv6 standard S.A.R.R. process.

  The Start() function starts the DHCPv6 standard process. This function can
  be called only when the state of Dhcp6 instance is in the Dhcp6Init state.
  If the DHCP process completes successfully, the state of the Dhcp6 instance
  will be transferred through Dhcp6Selecting and Dhcp6Requesting to the
  Dhcp6Bound state.
  Refer to rfc-3315 for precise state transitions during this process. At the
  time when each event occurs in this process, the callback function that was set
  by EFI_DHCP6_PROTOCOL.Configure() will be called and the user can take this
  opportunity to control the process.

  @param[in]  This              The pointer to Dhcp6 protocol.

  @retval EFI_SUCCESS           The DHCPv6 standard process has started, or it
                                completed when CompletionEvent was NULL.
  @retval EFI_ACCESS_DENIED     The EFI DHCPv6 Child instance hasn't been configured.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_TIMEOUT           The DHCPv6 configuration process failed because no
                                response was received from the server within the
                                specified timeout value.
  @retval EFI_ABORTED           The user aborted the DHCPv6 process.
  @retval EFI_ALREADY_STARTED   Some other Dhcp6 instance already started the DHCPv6
                                standard process.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Start (
  IN EFI_DHCP6_PROTOCOL  *This
  );

/**
  Stops the DHCPv6 standard S.A.R.R. process.

  The Stop() function is used to stop the DHCPv6 standard process. After this
  function is called successfully, the state of Dhcp6 instance is transferred
  into the Dhcp6Init. EFI_DHCP6_PROTOCOL.Configure() needs to be called
  before DHCPv6 standard process can be started again. This function can be
  called when the Dhcp6 instance is in any state.

  @param[in]  This              The pointer to the Dhcp6 protocol.

  @retval EFI_SUCCESS           The Dhcp6 instance is now in the Dhcp6Init state.
  @retval EFI_INVALID_PARAMETER This is NULL.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Stop (
  IN EFI_DHCP6_PROTOCOL  *This
  );

/**
  Returns the current operating mode data for the Dhcp6 instance.

  The GetModeData() function returns the current operating mode and
  cached data packet for the Dhcp6 instance.

  @param[in]  This              The pointer to the Dhcp6 protocol.
  @param[out] Dhcp6ModeData     The pointer to the Dhcp6 mode data.
  @param[out] Dhcp6ConfigData   The pointer to the Dhcp6 configure data.

  @retval EFI_SUCCESS           The mode data was returned.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_ACCESS_DENIED     The EFI DHCPv6 Protocol instance has not
                                been configured when Dhcp6ConfigData is
                                not NULL.
**/
EFI_STATUS
EFIAPI
EfiDhcp6GetModeData (
  IN  EFI_DHCP6_PROTOCOL     *This,
  OUT EFI_DHCP6_MODE_DATA    *Dhcp6ModeData      OPTIONAL,
  OUT EFI_DHCP6_CONFIG_DATA  *Dhcp6ConfigData    OPTIONAL
  );

/**
  Initializes, changes, or resets the operational settings for the Dhcp6 instance.

  The Configure() function is used to initialize or clean up the configuration
  data of the Dhcp6 instance:
  - When Dhcp6CfgData is not NULL and Configure() is called successfully, the
    configuration data will be initialized in the Dhcp6 instance and the state
    of the configured IA will be transferred into Dhcp6Init.
  - When Dhcp6CfgData is NULL and Configure() is called successfully, the
    configuration data will be cleaned up and no IA will be associated with
    the Dhcp6 instance.
  To update the configuration data for an Dhcp6 instance, the original data
  must be cleaned up before setting the new configuration data.

  @param[in]  This                   The pointer to the Dhcp6 protocol
  @param[in]  Dhcp6CfgData           The pointer to the EFI_DHCP6_CONFIG_DATA.

  @retval EFI_SUCCESS           The Dhcp6 is configured successfully with the
                                Dhcp6Init state, or cleaned up the original
                                configuration setting.
  @retval EFI_ACCESS_DENIED     The Dhcp6 instance has been already configured
                                when Dhcp6CfgData is not NULL.
                                The Dhcp6 instance has already started the
                                DHCPv6 S.A.R.R when Dhcp6CfgData is NULL.
  @retval EFI_INVALID_PARAMETER Some of the parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Configure (
  IN EFI_DHCP6_PROTOCOL     *This,
  IN EFI_DHCP6_CONFIG_DATA  *Dhcp6CfgData    OPTIONAL
  );

/**
  Request configuration information without the assignment of any
  Ia addresses of the client.

  The InfoRequest() function is used to request configuration information
  without the assignment of any IPv6 address of the client. Client sends
  out Information Request packet to obtain the required configuration
  information, and DHCPv6 server responds with Reply packet containing
  the information for the client. The received Reply packet will be passed
  to the user by ReplyCallback function. If user returns EFI_NOT_READY from
  ReplyCallback, the Dhcp6 instance will continue to receive other Reply
  packets unless timeout according to the Retransmission parameter.
  Otherwise, the Information Request exchange process will be finished
  successfully if user returns EFI_SUCCESS from ReplyCallback.

  @param[in]  This              The pointer to the Dhcp6 protocol.
  @param[in]  SendClientId      If TRUE, the DHCPv6 protocol instance will build Client
                                Identifier option and include it into Information Request
                                packet. Otherwise, Client Identifier option will not be included.
  @param[in]  OptionRequest     The pointer to the buffer of option request options.
  @param[in]  OptionCount       The option number in the OptionList.
  @param[in]  OptionList        The list of appended options.
  @param[in]  Retransmission    The pointer to the retransmission of the message.
  @param[in]  TimeoutEvent      The event of timeout.
  @param[in]  ReplyCallback     The callback function when a reply was received.
  @param[in]  CallbackContext   The pointer to the parameter passed to the callback.

  @retval EFI_SUCCESS           The DHCPv6 information request exchange process
                                completed when TimeoutEvent is NULL. Information
                                Request packet has been sent to DHCPv6 server when
                                TimeoutEvent is not NULL.
  @retval EFI_NO_RESPONSE       The DHCPv6 information request exchange process failed
                                because of no response, or not all requested-options
                                are responded to by DHCPv6 servers when Timeout happened.
  @retval EFI_ABORTED           The DHCPv6 information request exchange process was aborted
                                by the user.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp6InfoRequest (
  IN EFI_DHCP6_PROTOCOL        *This,
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
  Manually extend the valid and preferred lifetimes for the IPv6 addresses
  of the configured IA and update other configuration parameters by sending
  Renew or Rebind packet.

  The RenewRebind() function is used to manually extend the valid and preferred
  lifetimes for the IPv6 addresses of the configured IA and update other
  configuration parameters by sending a Renew or Rebind packet.
  - When RebindRequest is FALSE and the state of the configured IA is Dhcp6Bound,
    it will send Renew packet to the previously DHCPv6 server and transfer the
    state of the configured IA to Dhcp6Renewing. If valid Reply packet received,
    the state transfers to Dhcp6Bound and the valid and preferred timer restarts.
    If fails, the state transfers to Dhcp6Bound but the timer continues.
  - When RebindRequest is TRUE and the state of the configured IA is Dhcp6Bound,
    it will send a Rebind packet. If a valid Reply packet is received, the state transfers
    to Dhcp6Bound, and the valid and preferred timer restarts. If it fails, the state
    transfers to Dhcp6Init, and the IA can't be used.

  @param[in]  This              The pointer to the Dhcp6 protocol.
  @param[in]  RebindRequest     If TRUE, Rebind packet will be sent and enter Dhcp6Rebinding state.
                                Otherwise, Renew packet will be sent and enter Dhcp6Renewing state.

  @retval EFI_SUCCESS           The DHCPv6 renew/rebind exchange process
                                completed and at least one IPv6 address of the
                                configured IA was bound again when
                                EFI_DHCP6_CONFIG_DATA.IaInfoEvent was NULL.
                                The EFI DHCPv6 Protocol instance has sent Renew
                                or Rebind packet when
                                EFI_DHCP6_CONFIG_DATA.IaInfoEvent is not NULL.
  @retval EFI_ACCESS_DENIED     The Dhcp6 instance hasn't been configured, or the
                                state of the configured IA is not in Dhcp6Bound.
  @retval EFI_ALREADY_STARTED   The state of the configured IA has already entered
                                Dhcp6Renewing when RebindRequest is FALSE.
                                The state of the configured IA has already entered
                                Dhcp6Rebinding when RebindRequest is TRUE.
  @retval EFI_ABORTED           The DHCPv6 renew/rebind exchange process aborted
                                by user.
  @retval EFI_NO_RESPONSE       The DHCPv6 renew/rebind exchange process failed
                                because of no response.
  @retval EFI_NO_MAPPING        No IPv6 address has been bound to the configured
                                IA after the DHCPv6 renew/rebind exchange process.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp6RenewRebind (
  IN EFI_DHCP6_PROTOCOL  *This,
  IN BOOLEAN             RebindRequest
  );

/**
  Inform that one or more addresses assigned by a server are already
  in use by another node.

  The Decline() function is used to manually decline the assignment of
  IPv6 addresses, which have been already used by another node. If all
  IPv6 addresses of the configured IA are declined through this function,
  the state of the IA will switch through Dhcp6Declining to Dhcp6Init.
  Otherwise, the state of the IA will restore to Dhcp6Bound after the
  declining process. The Decline() can only be called when the IA is in
  Dhcp6Bound state. If the EFI_DHCP6_CONFIG_DATA.IaInfoEvent is NULL,
  this function is a blocking operation. It will return after the
  declining process finishes, or aborted by user.

  @param[in]  This              The pointer to the Dhcp6 protocol.
  @param[in]  AddressCount      The number of declining addresses.
  @param[in]  Addresses         The pointer to the buffer stored the declining
                                addresses.

  @retval EFI_SUCCESS           The DHCPv6 decline exchange process completed
                                when EFI_DHCP6_CONFIG_DATA.IaInfoEvent was NULL.
                                The Dhcp6 instance has sent Decline packet when
                                EFI_DHCP6_CONFIG_DATA.IaInfoEvent is not NULL.
  @retval EFI_ACCESS_DENIED     The Dhcp6 instance hasn't been configured, or the
                                state of the configured IA is not in Dhcp6Bound.
  @retval EFI_ABORTED           The DHCPv6 decline exchange process was aborted by the user.
  @retval EFI_NOT_FOUND         Any specified IPv6 address is not correlated with
                                the configured IA for this instance.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Decline (
  IN EFI_DHCP6_PROTOCOL  *This,
  IN UINT32              AddressCount,
  IN EFI_IPv6_ADDRESS    *Addresses
  );

/**
  Release one or more addresses associated with the configured Ia
  for the current instance.

  The Release() function is used to manually release the one or more
  IPv6 address. If AddressCount is zero, it will release all IPv6
  addresses of the configured IA. If all IPv6 addresses of the IA are
  released through this function, the state of the IA will switch
  through Dhcp6Releasing to Dhcp6Init, otherwise, the state of the
  IA will restore to Dhcp6Bound after the releasing process.
  The Release() can only be called when the IA is in a Dhcp6Bound state.
  If the EFI_DHCP6_CONFIG_DATA.IaInfoEvent is NULL, the function is
  a blocking operation. It will return after the releasing process
  finishes, or aborted by user.

  @param[in]  This              The pointer to the Dhcp6 protocol.
  @param[in]  AddressCount      The number of releasing addresses.
  @param[in]  Addresses         The pointer to the buffer stored the releasing
                                addresses.
  @retval EFI_SUCCESS           The DHCPv6 release exchange process has
                                completed when EFI_DHCP6_CONFIG_DATA.IaInfoEvent
                                is NULL. The Dhcp6 instance has sent Release
                                packet when EFI_DHCP6_CONFIG_DATA.IaInfoEvent
                                is not NULL.
  @retval EFI_ACCESS_DENIED     The Dhcp6 instance hasn't been configured, or the
                                state of the configured IA is not in Dhcp6Bound.
  @retval EFI_ABORTED           The DHCPv6 release exchange process was aborted by the user.
  @retval EFI_NOT_FOUND         Any specified IPv6 address is not correlated with
                                the configured IA for this instance.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Release (
  IN EFI_DHCP6_PROTOCOL  *This,
  IN UINT32              AddressCount,
  IN EFI_IPv6_ADDRESS    *Addresses
  );

/**
  Parse the option data in the Dhcp6 packet.

  The Parse() function is used to retrieve the option list in the DHCPv6 packet.

  @param[in]      This              The pointer to the Dhcp6 protocol.
  @param[in]      Packet            The pointer to the Dhcp6 packet.
  @param[in, out] OptionCount       The number of option in the packet.
  @param[out]     PacketOptionList  The array of pointers to the each option in the packet.

  @retval EFI_SUCCESS           The packet was successfully parsed.
  @retval EFI_INVALID_PARAMETER Some parameter is NULL.
  @retval EFI_BUFFER_TOO_SMALL  *OptionCount is smaller than the number of options
                                that were found in the Packet.

**/
EFI_STATUS
EFIAPI
EfiDhcp6Parse (
  IN EFI_DHCP6_PROTOCOL        *This,
  IN EFI_DHCP6_PACKET          *Packet,
  IN OUT UINT32                *OptionCount,
  OUT EFI_DHCP6_PACKET_OPTION  *PacketOptionList[]  OPTIONAL
  );

#endif
