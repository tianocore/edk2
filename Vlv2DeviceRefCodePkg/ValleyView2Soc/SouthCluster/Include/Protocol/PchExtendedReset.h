/*++

Copyright (c) 2008  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  PchExtendedReset.h

Abstract:

  PCH Extended Reset Protocol

--*/
#ifndef _EFI_PCH_EXTENDED_RESET_H_
#define _EFI_PCH_EXTENDED_RESET_H_



//
#define EFI_PCH_EXTENDED_RESET_PROTOCOL_GUID \
  { \
    0xf0bbfca0, 0x684e, 0x48b3, 0xba, 0xe2, 0x6c, 0x84, 0xb8, 0x9e, 0x53, 0x39 \
  }
extern EFI_GUID                                 gEfiPchExtendedResetProtocolGuid;

//
// Forward reference for ANSI C compatibility
//
typedef struct _EFI_PCH_EXTENDED_RESET_PROTOCOL EFI_PCH_EXTENDED_RESET_PROTOCOL;

//
// Related Definitions
//
//
// PCH Extended Reset Types
//
typedef struct {
  UINT8 PowerCycle  : 1;  // 0: Disabled*; 1: Enabled
  UINT8 GlobalReset : 1;  // 0: Disabled*; 1: Enabled
  UINT8 SusPwrDnAck : 1;  // 0: Do Nothing;
  // 1: GPIO[30](SUS_PWR_DN_ACK) level is set low prior to Global Reset(for systems with an embedded controller)
  UINT8 RsvdBits : 5;     // Reserved fields for future expansion w/o protocol change
} PCH_EXTENDED_RESET_TYPES;

//
// Member functions
//
typedef
EFI_STATUS
(EFIAPI *EFI_PCH_EXTENDED_RESET) (
  IN     EFI_PCH_EXTENDED_RESET_PROTOCOL   * This,
  IN     PCH_EXTENDED_RESET_TYPES          PchExtendedResetTypes
  );

/*++

Routine Description:

  Execute Pch Extended Reset from the host controller.

Arguments:

  This                    - Pointer to the EFI_PCH_EXTENDED_RESET_PROTOCOL instance.
  PchExtendedResetTypes   - Pch Extended Reset Types which includes PowerCycle, Globalreset.

Returns:

  Does not return if the reset takes place.
  EFI_INVALID_PARAMETER   - If ResetType is invalid.

--*/

//
// Interface structure for the Pch Extended Reset Protocol
//
struct _EFI_PCH_EXTENDED_RESET_PROTOCOL {
  EFI_PCH_EXTENDED_RESET  Reset;
};

#endif
