/**
  This protocol is used to report and control what BIOS is mapped to the
  BIOS address space anchored at 4GB boundary.

  This protocol is EFI compatible.

  E.G. For current generation ICH, the 4GB-16MB to 4GB range can be mapped
  to PCI, SPI, or FWH.

Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent


**/


#ifndef _EFI_ACTIVE_BIOS_PROTOCOL_H_
#define _EFI_ACTIVE_BIOS_PROTOCOL_H_

//
// Define the  protocol GUID
//
#define EFI_ACTIVE_BIOS_PROTOCOL_GUID  \
  { 0xebbe2d1b, 0x1647, 0x4bda, {0xab, 0x9a, 0x78, 0x63, 0xe3, 0x96, 0xd4, 0x1a} }

typedef struct _EFI_ACTIVE_BIOS_PROTOCOL EFI_ACTIVE_BIOS_PROTOCOL;

//
// Protocol definitions
//
typedef enum {
  ActiveBiosStateSpi,
  ActiveBiosStatePci,
  ActiveBiosStateLpc,
  ActiveBiosStateMax
} EFI_ACTIVE_BIOS_STATE;

typedef
EFI_STATUS
(EFIAPI *EFI_ACTIVE_BIOS_SET_ACTIVE_BIOS_STATE) (
  IN EFI_ACTIVE_BIOS_PROTOCOL     *This,
  IN EFI_ACTIVE_BIOS_STATE        DesiredState,
  IN UINTN                        Key
  );
/*++

Routine Description:

  Change the current active BIOS settings to the requested state.
  The caller is responsible for requesting a supported state from
  the EFI_ACTIVE_BIOS_STATE selections.

  This will fail if someone has locked the interface and the correct key is
  not provided.

Arguments:

  This                    Pointer to the EFI_ACTIVE_BIOS_PROTOCOL instance.
  DesiredState            The requested state to configure the system for.
  Key                     If the interface is locked, Key must be the Key
                          returned from the LockState function call.

Returns:

  EFI_SUCCESS             Command succeed.
  EFI_ACCESS_DENIED       The interface is currently locked.
  EFI_DEVICE_ERROR        Device error, command aborts abnormally.

--*/

typedef
EFI_STATUS
(EFIAPI *EFI_ACTIVE_BIOS_LOCK_ACTIVE_BIOS_STATE) (
  IN     EFI_ACTIVE_BIOS_PROTOCOL   *This,
  IN     BOOLEAN                    Lock,
  IN OUT UINTN                      *Key
  );
/*++

Routine Description:

  Lock the current active BIOS state from further changes.  This allows a
  caller to implement a critical section.  This is optionally supported
  functionality.  Size conscious implementations may choose to require
  callers cooperate without support from this protocol.

Arguments:

  This                    Pointer to the EFI_ACTIVE_BIOS_PROTOCOL instance.
  Lock                    TRUE to lock the current state, FALSE to unlock.
  Key                     If Lock is TRUE, then a key will be returned.  If
                          Lock is FALSE, the key returned from the prior call
                          to lock the protocol must be provided to unlock the
                          protocol.  The value of Key is undefined except that it
                          will never be 0.

Returns:

  EFI_SUCCESS             Command succeed.
  EFI_UNSUPPORTED         The function is not supported.
  EFI_ACCESS_DENIED       The interface is currently locked.
  EFI_DEVICE_ERROR        Device error, command aborts abnormally.

--*/

//
// Protocol definition
//
// Note that some functions are optional.  This means that they may be NULL.
// Caller is required to verify that an optional function is defined by checking
// that the value is not NULL.
//
struct _EFI_ACTIVE_BIOS_PROTOCOL {
  EFI_ACTIVE_BIOS_STATE                       State;
  EFI_ACTIVE_BIOS_SET_ACTIVE_BIOS_STATE       SetState;
  EFI_ACTIVE_BIOS_LOCK_ACTIVE_BIOS_STATE      LockState;
};

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gEfiActiveBiosProtocolGuid;

#endif
