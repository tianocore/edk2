/** @file
  Emulator Thunk to abstract OS services from pure EFI code

  Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Protocol/EmuIoThunk.h>


EFI_STATUS
EFIAPI
AddThunkProtocol (
  IN  EMU_IO_THUNK_PROTOCOL   *ThunkIo,
  IN  CHAR16                  *ConfigString,
  IN  BOOLEAN                 EmuBusDriver
  );

EFI_STATUS
EFIAPI
GetNextThunkProtocol (
  IN  BOOLEAN                 EmuBusDriver,
  OUT EMU_IO_THUNK_PROTOCOL   **Instance
  );


