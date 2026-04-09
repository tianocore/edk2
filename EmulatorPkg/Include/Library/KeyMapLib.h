/*++ @file

Copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Protocol/SimpleTextInEx.h>

/**
  KeyMapMake gets called on key presses.

  @param  KeyData       Key that was pressed.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
KeyMapMake (
  IN EFI_KEY_DATA  *KeyData
  );

/**
  KeyMapBreak gets called on key releases.

  @param  KeyData       Key that was pressed.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
KeyMapBreak (
  IN EFI_KEY_DATA  *KeyData
  );
