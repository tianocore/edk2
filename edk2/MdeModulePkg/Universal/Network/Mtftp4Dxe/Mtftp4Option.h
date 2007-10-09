/** @file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Mtftp4Option.h

Abstract:

  Mtftp4 option process routines.


**/

#ifndef __EFI_MTFTP4_OPTION_H__
#define __EFI_MTFTP4_OPTION_H__

enum {
  MTFTP4_SUPPORTED_OPTIONS = 4,
  MTFTP4_OPCODE_LEN        = 2,
  MTFTP4_ERRCODE_LEN       = 2,
  MTFTP4_BLKNO_LEN         = 2,
  MTFTP4_DATA_HEAD_LEN     = 4,

  MTFTP4_BLKSIZE_EXIST     = 0x01,
  MTFTP4_TIMEOUT_EXIST     = 0x02,
  MTFTP4_TSIZE_EXIST       = 0x04,
  MTFTP4_MCAST_EXIST       = 0x08
};

typedef struct {
  UINT16                    BlkSize;
  UINT8                     Timeout;
  UINT32                    Tsize;
  IP4_ADDR                  McastIp;
  UINT16                    McastPort;
  BOOLEAN                   Master;
  UINT32                    Exist;
} MTFTP4_OPTION;

EFI_STATUS
Mtftp4ExtractOptions (
  IN  EFI_MTFTP4_PACKET     *Packet,
  IN  UINT32                PacketLen,
  IN  OUT UINT32            *OptionCount,
  OUT EFI_MTFTP4_OPTION     **OptionList OPTIONAL
  );

EFI_STATUS
Mtftp4ParseOption (
  IN  EFI_MTFTP4_OPTION     *OptionList,
  IN  UINT32                Count,
  IN  BOOLEAN               Request,
  OUT MTFTP4_OPTION         *Option
  );

EFI_STATUS
Mtftp4ParseOptionOack (
  IN  EFI_MTFTP4_PACKET     *Packet,
  IN  UINT32                PacketLen,
  OUT MTFTP4_OPTION         *Option
  );

extern CHAR8  *mMtftp4SupportedOptions[MTFTP4_SUPPORTED_OPTIONS];
#endif
