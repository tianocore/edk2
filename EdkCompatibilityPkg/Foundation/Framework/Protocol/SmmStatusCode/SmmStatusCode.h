/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  SmmStatusCode.h

Abstract:

  SMM Status code Protocol as defined in the DXE CIS (Status Code Architectural Protocol)

  This code abstracts SMM Status Code reporting.

--*/

#ifndef _PROTOCOL_SMM_STATUS_CODE_H__
#define _PROTOCOL_SMM_STATUS_CODE_H__

//
// Global ID for the Smm Status Code Protocol
//
#define EFI_SMM_STATUS_CODE_PROTOCOL_GUID \
  { \
    0x6afd2b77, 0x98c1, 0x4acd, {0xa6, 0xf9, 0x8a, 0x94, 0x39, 0xde, 0xf, 0xb1} \
  }

extern EFI_GUID gEfiSmmStatusCodeProtocolGuid;

//
// Forward reference for pure ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_SMM_STATUS_CODE_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_REPORT_STATUS_CODE) (
  IN EFI_SMM_STATUS_CODE_PROTOCOL * This,
  IN EFI_STATUS_CODE_TYPE         CodeType,
  IN EFI_STATUS_CODE_VALUE        Value,
  IN UINT32                       Instance,
  IN EFI_GUID                     * CallerId,
  IN EFI_STATUS_CODE_DATA         * Data OPTIONAL
  );

struct _EFI_SMM_STATUS_CODE_PROTOCOL {
  EFI_SMM_REPORT_STATUS_CODE  ReportStatusCode;
};

#endif
