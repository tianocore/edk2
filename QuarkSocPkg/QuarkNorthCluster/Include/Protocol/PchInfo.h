/** @file
This file defines the QNC Info Protocol.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/
#ifndef _PCH_INFO_H_
#define _PCH_INFO_H_

//
// Extern the GUID for protocol users.
//
extern EFI_GUID                       gEfiQncInfoProtocolGuid;

//
// Forward reference for ANSI C compatibility
//
typedef struct _EFI_QNC_INFO_PROTOCOL EFI_QNC_INFO_PROTOCOL;

//
// Protocol revision number
// Any backwards compatible changes to this protocol will result in an update in the revision number
// Major changes will require publication of a new protocol
//
// Revision 1:  Original version
// Revision 2:   Add RCVersion item to EFI_QNC_INFO_PROTOCOL
//
#define QNC_INFO_PROTOCOL_REVISION_1  1
#define QNC_INFO_PROTOCOL_REVISION_2  2

//
// RCVersion[7:0] is the release number.
//
#define QNC_RC_VERSION                0x01020000

//
// Protocol definition
//
struct _EFI_QNC_INFO_PROTOCOL {
  UINT8   Revision;
  UINT8   BusNumber;
  UINT32  RCVersion;
};

#endif
