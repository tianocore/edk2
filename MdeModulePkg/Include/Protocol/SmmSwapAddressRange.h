/** @file
  The EFI_SMM_SWAP_ADDRESS_RANGE_PROTOCOL is related to EDK II-specific implementation
  and used to abstract the swap operation of boot block and backup block of FV in EFI
  SMM environment. This swap is especially needed when updating the boot block of FV.
  If a power failure happens during the boot block update, the swapped backup block
  (now the boot block) can boot the machine with the old boot block backed up in it.
  The swap operation is platform dependent, so other protocols such as SMM FTW (Fault
  Tolerant Write) should use this protocol instead of handling hardware directly.

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMM_SWAP_ADDRESS_RANGE_H__
#define __SMM_SWAP_ADDRESS_RANGE_H__

#include <Protocol/SwapAddressRange.h>

#define EFI_SMM_SWAP_ADDRESS_RANGE_PROTOCOL_GUID \
  { \
    0x67c4f112, 0x3385, 0x4e55, { 0x9c, 0x5b, 0xc0, 0x5b, 0x71, 0x7c, 0x42, 0x28 } \
  }

//
// SMM Swap Address Range protocol structure is the same as Swap Address Range protocol.
// The SMM one is intend to run in SMM environment, which means it can be used by
// SMM drivers after ExitPmAuth.
//
typedef EFI_SWAP_ADDRESS_RANGE_PROTOCOL EFI_SMM_SWAP_ADDRESS_RANGE_PROTOCOL;

extern EFI_GUID  gEfiSmmSwapAddressRangeProtocolGuid;

#endif
