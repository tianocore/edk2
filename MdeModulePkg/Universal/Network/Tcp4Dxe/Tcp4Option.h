/** @file
  Tcp option's routine header file.
    
Copyright (c) 2005 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TCP4_OPTION_H_
#define _TCP4_OPTION_H_

///
/// The structure to store the parse option value.
/// ParseOption only parse the options, don't process them.
///
typedef struct _TCP_OPTION {
  UINT8   Flag;     ///< Flag such as TCP_OPTION_RCVD_MSS
  UINT8   WndScale; ///< The WndScale received
  UINT16  Mss;      ///< The Mss received
  UINT32  TSVal;    ///< The TSVal field in a timestamp option
  UINT32  TSEcr;    ///< The TSEcr field in a timestamp option
} TCP_OPTION;

//
// supported TCP option type and their length
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


/**
  Compute the window scale value according to the given buffer size.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.

  @return  The scale value.

**/
UINT8
TcpComputeScale (
  IN TCP_CB *Tcb
  );

/**
  Build the TCP option in three-way handshake.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param  Nbuf    Pointer to the buffer to store the options.

  @return  The total length of the TCP option field.

**/
UINT16
TcpSynBuildOption (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Nbuf
  );

/**
  Build the TCP option in synchronized states.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param  Nbuf    Pointer to the buffer to store the options.

  @return  The total length of the TCP option field.

**/
UINT16
TcpBuildOption (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Nbuf
  );

/**
  Parse the supported options.

  @param  Tcp     Pointer to the TCP_CB of this TCP instance.
  @param  Option  Pointer to the TCP_OPTION used to store the successfully pasrsed
                  options.

  @retval 0       The options are successfully pasrsed.
  @retval -1      Ilegal option was found.

**/
INTN
TcpParseOption (
  IN     TCP_HEAD   *Tcp,
  IN OUT TCP_OPTION *Option
  );

/**
  Check the segment against PAWS.

  @param  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param  TSVal   The timestamp value.

  @retval 1       The segment passed the PAWS check.
  @retval 0       The segment failed to pass the PAWS check.

**/
UINT32
TcpPawsOK (
  IN TCP_CB *Tcb,
  IN UINT32 TSVal
  );

#endif
