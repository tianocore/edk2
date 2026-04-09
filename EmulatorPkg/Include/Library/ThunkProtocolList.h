/** @file
  Emulator Thunk to abstract OS services from pure EFI code

  Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Protocol/EmuIoThunk.h>

EFI_STATUS
EFIAPI
AddThunkProtocol (
  IN  EMU_IO_THUNK_PROTOCOL  *ThunkIo,
  IN  CHAR16                 *ConfigString,
  IN  BOOLEAN                EmuBusDriver
  );

EFI_STATUS
EFIAPI
GetNextThunkProtocol (
  IN  BOOLEAN                EmuBusDriver,
  OUT EMU_IO_THUNK_PROTOCOL  **Instance
  );
