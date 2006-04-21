/*++ 

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  tftp.h

Abstract:

--*/

#ifndef __TFTP_H__
#define __TFTP_H__

//
// Definitions for trivial file transfer protocol functionality with IP v4
// Per RFC 1350, July 1992 and RFC 2347, 8, and 9, May 1998
//
#pragma pack(1)
//
// max and min packet sizes
// (all data packets in transmission except last)
//
#define MAX_TFTP_PKT_SIZE (BUFFER_ALLOCATE_SIZE - 512)
#define MIN_TFTP_PKT_SIZE 512

//
// TFTPv4 OpCodes
//
#define TFTP_RRQ    1 // read request
#define TFTP_WRQ    2 // write request
#define TFTP_DATA   3 // data
#define TFTP_ACK    4 // acknowledgement
#define TFTP_ERROR  5 // error packet
#define TFTP_OACK   6 // option acknowledge
#define TFTP_DIR    7 // read directory request
#define TFTP_DATA8  8
#define TFTP_ACK8   9

//
// request packet (read or write)
// Fields shown (except file name) are not to be referenced directly,
// since their placement is variable within a request packet.
// All are null terminated case insensitive ascii strings.
//
struct Tftpv4Req {
  UINT16  OpCode;       // TFTP Op code
  UINT8   FileName[2];  // file name
  UINT8   Mode[2];      // "netascii" or "octet"
  struct {              // optionally, one or more option requests
    UINT8 Option[2];    // option name
    UINT8 Value[2];     // value requested
  } OpReq[1];
};

//
// modes
//
#define MODE_ASCII  "netascii"
#define MODE_BINARY "octet"

//
// option strings
//
#define OP_BLKSIZE    "blksize"   // block size option
#define OP_TIMEOUT    "timeout"   // time to wait before retransmitting
#define OP_TFRSIZE    "tsize"     // total transfer size option
#define OP_OVERWRITE  "overwrite" // overwrite file option
#define OP_BIGBLKNUM  "bigblk#"   // big block number
// See RFC 2347, 8, and 9 for more information on TFTP options
// option acknowledge packet (optional)
// options not acknowledged are rejected
//
struct Tftpv4Oack {
  UINT16  OpCode;     // TFTP Op code
  struct {            // optionally, one or more option acknowledgements
    UINT8 Option[2];  // option name (of those requested)
    UINT8 Value[2];   // value acknowledged
  } OpAck[1];
};

//
// acknowledge packet
//
struct Tftpv4Ack {
  UINT16  OpCode; // TFTP Op code
  UINT16  BlockNum;
};

//
// data packet
//
struct Tftpv4Data {
  struct Tftpv4Ack  Header;
  UINT8             Data[512];
};

//
// big block number ack packet
//
struct Tftpv4Ack8 {
  UINT16  OpCode;
  UINT64  BlockNum;
};

//
// big block number data packet
//
struct Tftpv4Data8 {
  struct Tftpv4Ack8 Header;
  UINT8             Data[506];
};

//
// error packet
//
struct Tftpv4Error {
  UINT16  OpCode;     // TFTP Op code
  UINT16  ErrCode;    // error code
  UINT8   ErrMsg[1];  // error message (nul terminated)
};

#pragma pack()
//
// error codes
//
#define TFTP_ERR_UNDEF      0 //     Not defined, see error message (if any).
#define TFTP_ERR_NOT_FOUND  1 //     File not found.
#define TFTP_ERR_ACCESS     2 //     Access violation.
#define TFTP_ERR_FULL       3 //     Disk full or allocation exceeded.
#define TFTP_ERR_ILLEGAL    4 //     Illegal TFTP operation.
#define TFTP_ERR_BAD_ID     5 //     Unknown transfer ID.
#define TFTP_ERR_EXISTS     6 //     File already exists.
#define TFTP_ERR_NO_USER    7 //     No such user.
#define TFTP_ERR_OPTION     8 //     Option negotiation termination
//
// some defines
//
#define REQ_RESP_TIMEOUT        5 // Wait five seconds for request response.
#define ACK_TIMEOUT             4 // Wait four seconds for ack response.
#define NUM_ACK_RETRIES         3
#define NUM_MTFTP_OPEN_RETRIES  3

#endif /* __TFTP_H__ */

/* EOF - tftp.h */
