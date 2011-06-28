/*++ @file

Copyright (c) 2011, Apple Inc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  IN EFI_KEY_DATA   *KeyData
  );

/**
  KeyMapBreak gets called on key releases.

  @param  KeyData       Key that was pressed.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
KeyMapBreak (
  IN EFI_KEY_DATA   *KeyData
  );
