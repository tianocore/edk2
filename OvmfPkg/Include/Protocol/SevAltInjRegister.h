/** @file
  The file provides the protocol that registers Alternate Injection for
  a SEV-SNP guest before ExitBootServices(). This protocol is only meant to be
  called by the OS loader, and not EDK2 itself.

  Copyright (c) 2026, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#define OVMF_SEV_ALT_INJ_REGISTER_PROTOCOL_GUID \
  { 0x3cf00587, \
    0x2329, \
    0x4c44, \
    {0xb1, 0x84, 0x8f, 0xd9, 0x87, 0x00, 0x42, 0xe0}}

typedef struct _OVMF_SEV_ALT_INJ_REGISTER_PROTOCOL
    OVMF_SEV_ALT_INJ_REGISTER_PROTOCOL;

/**
  Register the caller (guest kernel) as an Alternate Injection consumer.
  Wraps the SVSM registration; increments the SVSM registration counter.

  @retval EFI_SUCCESS           Registration succeeded.
  @retval EFI_UNSUPPORTED       Alt-Injection not available on this platform.
  @retval EFI_INVALID_PARAMETER The parameter "This" is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *OVMF_SEV_REGISTER_ALT_INJ)(
  IN OVMF_SEV_ALT_INJ_REGISTER_PROTOCOL  *This
  );

struct _OVMF_SEV_ALT_INJ_REGISTER_PROTOCOL {
  OVMF_SEV_REGISTER_ALT_INJ    SevRegisterAltInj;
};
