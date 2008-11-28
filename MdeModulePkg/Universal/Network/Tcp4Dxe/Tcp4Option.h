/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Tcp4Option.h

Abstract:


**/

#ifndef _TCP4_OPTION_H_
#define _TCP4_OPTION_H_

//
// The structure to store the parse option value.
// ParseOption only parse the options, don't process them.
//
typedef struct s_TCP_OPTION {
  UINT8   Flag;     // flag such as TCP_OPTION_RCVD_MSS
  UINT8   WndScale; // the WndScale received
  UINT16  Mss;      // the Mss received
  UINT32  TSVal;    // the TSVal field in a timestamp option
  UINT32  TSEcr;    // the TSEcr field in a timestamp option
} TCP_OPTION;

typedef enum {

  //
  // supported TCP option type and their length
  //
  TCP_OPTION_EOP            = 0,  // End Of oPtion
  TCP_OPTION_NOP            = 1,  // No-Option.
  TCP_OPTION_MSS            = 2,  // Maximum Segment Size
  TCP_OPTION_WS             = 3,  // Window scale
  TCP_OPTION_TS             = 8,  // Timestamp
  TCP_OPTION_MSS_LEN        = 4,  // length of MSS option
  TCP_OPTION_WS_LEN         = 3,  // length of window scale option
  TCP_OPTION_TS_LEN         = 10, // length of timestamp option
  TCP_OPTION_WS_ALIGNED_LEN = 4,  // length of window scale option, aligned
  TCP_OPTION_TS_ALIGNED_LEN = 12, // length of timestamp option, aligned

  //
  // recommend format of timestamp window scale
  // option for fast process.
  //
  TCP_OPTION_TS_FAST = ((TCP_OPTION_NOP << 24) |
                        (TCP_OPTION_NOP << 16) |
                        (TCP_OPTION_TS << 8) |
                        TCP_OPTION_TS_LEN),

  TCP_OPTION_WS_FAST = ((TCP_OPTION_NOP << 24) |
                        (TCP_OPTION_WS << 16) |
                        (TCP_OPTION_WS_LEN << 8)),

  TCP_OPTION_MSS_FAST = ((TCP_OPTION_MSS << 24) |
                         (TCP_OPTION_MSS_LEN << 16)),

  //
  // Other misc definations
  //
  TCP_OPTION_RCVD_MSS       = 0x01,
  TCP_OPTION_RCVD_WS        = 0x02,
  TCP_OPTION_RCVD_TS        = 0x04,
  TCP_OPTION_MAX_WS         = 14,     // Maxium window scale value
  TCP_OPTION_MAX_WIN        = 0xffff  // max window size in TCP header
} TCP_OPTION_TYPE;

UINT8
TcpComputeScale (
  IN TCP_CB *Tcb
  );

UINT16
TcpSynBuildOption (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Buf
  );

UINT16
TcpBuildOption (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Buf
  );

INTN
TcpParseOption (
  IN TCP_HEAD   *Tcp,
  IN TCP_OPTION *Option
  );

UINT32
TcpPawsOK (
  IN TCP_CB *Tcb,
  IN UINT32 TSVal
  );

#endif
