/** @file
  IA-32 Specific relocation fixups.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  PeCoffLoaderEx.c

**/





/**
  Performs an IA-32 specific relocation fixup.

  @param  Reloc Pointer to the relocation record.

  @param  Fixup Pointer to the address to fix up.

  @param  FixupData Pointer to a buffer to log the fixups.

  @param  Adjust The offset to adjust the fixup.

  @retval  EFI_UNSUPPORTED Unsupported now.

**/
RETURN_STATUS
PeCoffLoaderRelocateImageEx (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  )
{
  return RETURN_UNSUPPORTED;
}
