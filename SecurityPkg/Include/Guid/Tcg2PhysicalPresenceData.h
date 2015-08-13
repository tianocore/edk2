/** @file
  Define the variable data structures used for TCG2 physical presence.
  The TPM2 request from firmware or OS is saved to variable. And it is
  cleared after it is processed in the next boot cycle. The TPM2 response 
  is saved to variable.

Copyright (c) 2015, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TCG2_PHYSICAL_PRESENCE_DATA_GUID_H__
#define __TCG2_PHYSICAL_PRESENCE_DATA_GUID_H__

#define EFI_TCG2_PHYSICAL_PRESENCE_DATA_GUID \
  { \
    0xaeb9c5c1, 0x94f1, 0x4d02, { 0xbf, 0xd9, 0x46, 0x2, 0xdb, 0x2d, 0x3c, 0x54 } \
  }

#define TCG2_PHYSICAL_PRESENCE_VARIABLE  L"Tcg2PhysicalPresence"

typedef struct {
  UINT8   PPRequest;      ///< Physical Presence request command.
  UINT32  PPRequestParameter; ///< Physical Presence request Parameter.
  UINT8   LastPPRequest;
  UINT32  PPResponse;
} EFI_TCG2_PHYSICAL_PRESENCE;

//
// This variable is used to save TCG2 Management Flags and corresponding operations.
// It should be protected from malicious software (e.g. Set it as read-only variable). 
//
#define TCG2_PHYSICAL_PRESENCE_FLAGS_VARIABLE  L"Tcg2PhysicalPresenceFlags"
typedef struct {
  UINT32  PPFlags;
} EFI_TCG2_PHYSICAL_PRESENCE_FLAGS;

extern EFI_GUID  gEfiTcg2PhysicalPresenceGuid;

#endif

