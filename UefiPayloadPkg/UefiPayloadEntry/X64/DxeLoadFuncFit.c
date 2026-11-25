/** @file
  x64-specifc functionality for DxeLoad.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"
#include "X64/VirtualMemory.h"

//
// Global Descriptor Table (GDT) - 8-byte aligned
// GDT with 8 entries.
// (0x0)NULL,         (0x8)NULL
// (0x10)NULL,        (0x18)NULL,
// (0x20)NULL,        (0x28)NULL,
// (0x30)64-bit Data, (0x38)64-bit Code
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_GDT  gGdtEntries[8] = {
  // NULL descriptor (entry 0-5)
  { .Uint64 = 0                     },
  { .Uint64 = 0                     },
  { .Uint64 = 0                     },
  { .Uint64 = 0                     },
  { .Uint64 = 0                     },
  { .Uint64 = 0                     },
  // .short 0xffff           /* limit 15:00 */
  // .short 0x0000           /* base 15:00 */
  // .byte  0x00             /* base 23:16 */
  // .byte  0b10010010       /* P(1) DPL(00) S(1) 1 E(0) W(1) A(0) */
  // .byte  0b11001111       /* G(1) B(1) 0 AVL(0) limit 19:16 */
  // .byte  0x0              /* base 31:24 */
  { .Uint64 = 0x00CF92000000FFFFULL },
  // .short 0xffff           /* limit 15:00 */
  // .short 0x0000           /* base 15:00 */
  // .byte  0x00             /* base 23:16 */
  // .byte  0b10011010       /* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
  // .byte  0b10101111       /* G(1) D(0) L(1) AVL(0) limit 19:16 */
  // .byte  0x0              /* base 31:24 */
  { .Uint64 = 0x00AF9A000000FFFFULL }
};

/**
  Entry point to the C language phase of UEFI payload.
  @param[in]   BootloaderParameter    The starting address of bootloader parameter block.
  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN  BootloaderParameter
  )
{
  IA32_DESCRIPTOR  Gdtr;

  // Set up temporary GDT
  Gdtr.Base  = (UINTN)gGdtEntries;
  Gdtr.Limit = (UINT16)(sizeof (gGdtEntries) - 1);
  AsmWriteGdtr (&Gdtr);

  return FitUplEntryPoint (BootloaderParameter);
}
