/** @file
  
  Mtftp4 Implementation.
  
  Mtftp4 Implementation, it supports the following RFCs:
  RFC1350 - THE TFTP PROTOCOL (REVISION 2)
  RFC2090 - TFTP Multicast Option
  RFC2347 - TFTP Option Extension
  RFC2348 - TFTP Blocksize Option
  RFC2349 - TFTP Timeout Interval and Transfer Size Options
  
Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __EFI_MTFTP4_IMPL_H__
#define __EFI_MTFTP4_IMPL_H__

#include <Uefi.h>

#include <Protocol/Udp4.h>
#include <Protocol/Mtftp4.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UdpIoLib.h>
#include <Library/PrintLib.h>

extern EFI_MTFTP4_PROTOCOL  gMtftp4ProtocolTemplate;

typedef struct _MTFTP4_SERVICE  MTFTP4_SERVICE;
typedef struct _MTFTP4_PROTOCOL MTFTP4_PROTOCOL;

#include "Mtftp4Driver.h"
#include "Mtftp4Option.h"
#include "Mtftp4Support.h"


///
/// Some constant value of Mtftp service.
///
#define MTFTP4_SERVICE_SIGNATURE    SIGNATURE_32 ('T', 'F', 'T', 'P')
#define MTFTP4_PROTOCOL_SIGNATURE   SIGNATURE_32 ('t', 'f', 't', 'p')

#define MTFTP4_DEFAULT_SERVER_PORT  69
#define MTFTP4_DEFAULT_TIMEOUT      3
#define MTFTP4_DEFAULT_RETRY        5
#define MTFTP4_DEFAULT_BLKSIZE      512
#define MTFTP4_TIME_TO_GETMAP       5

#define MTFTP4_STATE_UNCONFIGED     0
#define MTFTP4_STATE_CONFIGED       1
#define MTFTP4_STATE_DESTROY        2

///
/// Mtftp service block
///
struct _MTFTP4_SERVICE {
  UINT32                        Signature;
  EFI_SERVICE_BINDING_PROTOCOL  ServiceBinding;

  UINT16                        ChildrenNum;
  LIST_ENTRY                    Children;

  EFI_EVENT                     Timer;  ///< Ticking timer for all the MTFTP clients
  EFI_EVENT                     TimerToGetMap;

  EFI_HANDLE                    Controller;
  EFI_HANDLE                    Image;

  //
  // This UDP child is used to keep the connection between the UDP
  // and MTFTP, so MTFTP will be notified when UDP is uninstalled.
  //
  UDP_IO                        *ConnectUdp;
};


typedef struct {
  EFI_MTFTP4_PACKET             **Packet;
  UINT32                        *PacketLen;
  EFI_STATUS                    Status;
} MTFTP4_GETINFO_STATE;

struct _MTFTP4_PROTOCOL {
  UINT32                        Signature;
  LIST_ENTRY                    Link;
  EFI_MTFTP4_PROTOCOL           Mtftp4;

  INTN                          State;
  BOOLEAN                       InDestroy;

  MTFTP4_SERVICE                *Service;
  EFI_HANDLE                    Handle;

  EFI_MTFTP4_CONFIG_DATA        Config;

  //
  // Operation parameters: token and requested options.
  //
  EFI_MTFTP4_TOKEN              *Token;
  MTFTP4_OPTION                 RequestOption;
  UINT16                        Operation;

  //
  // Blocks is a list of MTFTP4_BLOCK_RANGE which contains
  // holes in the file
  //
  UINT16                        BlkSize;
  UINT16                        LastBlock;
  LIST_ENTRY                    Blocks;

  //
  // The server's communication end point: IP and two ports. one for
  // initial request, one for its selected port.
  //
  IP4_ADDR                      ServerIp;
  UINT16                        ListeningPort;
  UINT16                        ConnectedPort;
  IP4_ADDR                      Gateway;
  UDP_IO                        *UnicastPort;

  //
  // Timeout and retransmit status
  //
  NET_BUF                       *LastPacket;
  UINT32                        PacketToLive;
  UINT32                        CurRetry;
  UINT32                        MaxRetry;
  UINT32                        Timeout;

  //
  // Parameter used by RRQ's multicast download.
  //
  IP4_ADDR                      McastIp;
  UINT16                        McastPort;
  BOOLEAN                       Master;
  UDP_IO                        *McastUdpPort;
};

typedef struct {
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;
} MTFTP4_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT;

/**
  Clean up the MTFTP session to get ready for new operation.

  @param  Instance               The MTFTP session to clean up
  @param  Result                 The result to return to the caller who initiated
                                 the operation.
**/
VOID
Mtftp4CleanOperation (
  IN OUT MTFTP4_PROTOCOL        *Instance,
  IN     EFI_STATUS             Result
  );

/**
  Start the MTFTP session for upload.
  
  It will first init some states, then send the WRQ request packet, 
  and start receiving the packet.

  @param  Instance              The MTFTP session
  @param  Operation             Redundant parameter, which is always
                                EFI_MTFTP4_OPCODE_WRQ here.

  @retval EFI_SUCCESS           The upload process has been started.
  @retval Others                Failed to start the upload.

**/
EFI_STATUS
Mtftp4WrqStart (
  IN MTFTP4_PROTOCOL        *Instance,
  IN UINT16                 Operation
  );

/**
  Start the MTFTP session to download. 
  
  It will first initialize some of the internal states then build and send a RRQ 
  reqeuest packet, at last, it will start receive for the downloading.

  @param  Instance              The Mtftp session
  @param  Operation             The MTFTP opcode, it may be a EFI_MTFTP4_OPCODE_RRQ
                                or EFI_MTFTP4_OPCODE_DIR.

  @retval EFI_SUCCESS           The mtftp download session is started.
  @retval Others                Failed to start downloading.

**/
EFI_STATUS
Mtftp4RrqStart (
  IN MTFTP4_PROTOCOL        *Instance,
  IN UINT16                 Operation
  );

#define MTFTP4_SERVICE_FROM_THIS(a)   \
  CR (a, MTFTP4_SERVICE, ServiceBinding, MTFTP4_SERVICE_SIGNATURE)

#define MTFTP4_PROTOCOL_FROM_THIS(a)  \
  CR (a, MTFTP4_PROTOCOL, Mtftp4, MTFTP4_PROTOCOL_SIGNATURE)

#endif
