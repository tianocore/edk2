/** @file
  Library to define platform customization functions for a
  Firmare Volume Block driver.

  Copyright (c) 2009, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PLATFORM_FVB_LIB__
#define __PLATFORM_FVB_LIB__

#include <Protocol/FirmwareVolumeBlock.h>

/**
  This function will be called following a call to the
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL Write function.

  @param[in] This     EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL instance.
  @param[in] Lba      The starting logical block index to written to.

**/
VOID
EFIAPI
PlatformFvbDataWritten (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN        EFI_LBA                             Lba
  );

#endif

