/** @file
  Tcp option's routine header file.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TCP_OPTION_H_
#define _TCP_OPTION_H_

//
// Supported TCP option types and their length.
//
#define TCP_OPTION_EOP             0  ///< End Of oPtion
#define TCP_OPTION_NOP             1  ///< No-Option.
#define TCP_OPTION_MSS             2  ///< Maximum Segment Size
#define TCP_OPTION_WS              3  ///< Window scale
#define TCP_OPTION_TS              8  ///< Timestamp
#define TCP_OPTION_MSS_LEN         4  ///< Length of MSS option
#define TCP_OPTION_WS_LEN          3  ///< Length of window scale option
#define TCP_OPTION_TS_LEN          10 ///< Length of timestamp option
#define TCP_OPTION_WS_ALIGNED_LEN  4  ///< Length of window scale option, aligned
#define TCP_OPTION_TS_ALIGNED_LEN  12 ///< Length of timestamp option, aligned

//
// recommend format of timestamp window scale
// option for fast process.
//
#define TCP_OPTION_TS_FAST ((TCP_OPTION_NOP << 24) | \
                            (TCP_OPTION_NOP << 16) | \
                            (TCP_OPTION_TS << 8)   | \
                            (TCP_OPTION_TS_LEN))

#define TCP_OPTION_WS_FAST   ((TCP_OPTION_NOP << 24) | \
                              (TCP_OPTION_WS << 16)  | \
                              (TCP_OPTION_WS_LEN << 8))

#define TCP_OPTION_MSS_FAST  ((TCP_OPTION_MSS << 24) | (TCP_OPTION_MSS_LEN << 16))

//
// Other misc definations
//
#define TCP_OPTION_RCVD_MSS        0x01
#define TCP_OPTION_RCVD_WS         0x02
#define TCP_OPTION_RCVD_TS         0x04
#define TCP_OPTION_MAX_WS          14      ///< Maxium window scale value
#define TCP_OPTION_MAX_WIN         0xffff  ///< Max window size in TCP header

///
/// The structure to store the parse option value.
/// ParseOption only parses the options, doesn't process them.
///
typedef struct _TCP_OPTION {
  UINT8   Flag;     ///< Flag such as TCP_OPTION_RCVD_MSS
  UINT8   WndScale; ///< The WndScale received
  UINT16  Mss;      ///< The Mss received
  UINT32  TSVal;    ///< The TSVal field in a timestamp option
  UINT32  TSEcr;    ///< The TSEcr field in a timestamp option
} TCP_OPTION;

/**
  Compute the window scale value according to the given buffer size.

  @param[in]  Tcb Pointer to the TCP_CB of this TCP instance.

  @return         The scale value.

**/
UINT8
TcpComputeScale (
  IN TCP_CB *Tcb
  );

/**
  Build the TCP option in three-way handshake.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  Nbuf    Pointer to the buffer to store the options.

  @return             The total length of the TCP option field.

**/
UINT16
TcpSynBuildOption (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Nbuf
  );

/**
  Build the TCP option in synchronized states.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  Nbuf    Pointer to the buffer to store the options.

  @return             The total length of the TCP option field.

**/
UINT16
TcpBuildOption (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Nbuf
  );

/**
  Parse the supported options.

  @param[in]       Tcp     Pointer to the TCP_CB of this TCP instance.
  @param[in, out]  Option  Pointer to the TCP_OPTION used to store the
                           successfully pasrsed options.

  @retval          0       The options successfully pasrsed.
  @retval          -1      Ilegal option was found.

**/
INTN
TcpParseOption (
  IN     TCP_HEAD   *Tcp,
  IN OUT TCP_OPTION *Option
  );

#endif
