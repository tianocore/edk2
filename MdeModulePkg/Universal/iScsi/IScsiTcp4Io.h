/*++

Copyright (c)  2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  IScsiTcp4Io.h

Abstract:

  iSCSI Tcp4 IO related definitions.

--*/

#ifndef _ISCSI_TCP4_IO_H_
#define _ISCSI_TCP4_IO_H_

#include <Library/NetLib.h>
#include <protocol/Tcp4.h>

typedef struct _TCP4_IO_CONFIG_DATA {
  EFI_IPv4_ADDRESS  LocalIp;
  EFI_IPv4_ADDRESS  SubnetMask;
  EFI_IPv4_ADDRESS  Gateway;

  EFI_IPv4_ADDRESS  RemoteIp;
  UINT16            RemotePort;
} TCP4_IO_CONFIG_DATA;

typedef struct _TCP4_IO {
  EFI_HANDLE                Image;
  EFI_HANDLE                Controller;

  EFI_HANDLE                Handle;
  EFI_TCP4_PROTOCOL         *Tcp4;

  EFI_TCP4_CONNECTION_TOKEN ConnToken;
  EFI_TCP4_IO_TOKEN         TxToken;
  EFI_TCP4_IO_TOKEN         RxToken;
  EFI_TCP4_CLOSE_TOKEN      CloseToken;

  BOOLEAN                   IsConnDone;
  BOOLEAN                   IsTxDone;
  BOOLEAN                   IsRxDone;
  BOOLEAN                   IsCloseDone;
} TCP4_IO;

EFI_STATUS
Tcp4IoCreateSocket (
  IN EFI_HANDLE           Image,
  IN EFI_HANDLE           Controller,
  IN TCP4_IO_CONFIG_DATA  *ConfigData,
  IN TCP4_IO              *Tcp4Io
  );

VOID
Tcp4IoDestroySocket (
  IN TCP4_IO  *Tcp4Io
  );

EFI_STATUS
Tcp4IoConnect (
  IN TCP4_IO    *Tcp4Io,
  IN EFI_EVENT  Timeout
  );

VOID
Tcp4IoReset (
  IN TCP4_IO  *Tcp4Io
  );

EFI_STATUS
Tcp4IoTransmit (
  IN TCP4_IO  *Tcp4Io,
  IN NET_BUF  *Packet
  );

EFI_STATUS
Tcp4IoReceive (
  IN TCP4_IO    *Tcp4Io,
  IN NET_BUF    *Packet,
  IN BOOLEAN    AsyncMode,
  IN EFI_EVENT  Timeout
  );

#endif
