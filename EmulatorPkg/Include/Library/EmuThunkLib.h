/*++ @file

Copyright (c) 2011, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EMU_THUNK_LIB_H__
#define __EMU_THUNK_LIB_H__

#include <Protocol/EmuThunk.h>


extern EMU_THUNK_PROTOCOL   *gEmuThunk;


/**
  Serach the EMU IO Thunk database for a matching EMU IO Thunk
  Protocol instance.

  @param  Protocol   Protocol to search for.
  @param  Instance   Instance of protocol to search for.

  @retval NULL       Protocol and Instance not found.
  @retval other      EMU IO Thunk protocol that matched.

**/
EMU_IO_THUNK_PROTOCOL *
EFIAPI
GetIoThunkInstance (
  IN  EFI_GUID  *Protocol,
  IN  UINTN     Instance
  );


#endif
