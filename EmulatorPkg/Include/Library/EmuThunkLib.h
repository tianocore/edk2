/*++ @file

Copyright (c) 2011, Apple Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

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
