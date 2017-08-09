/*++
  This file contains a 'Sample Driver' and is licensed as such
  under the terms of your license agreement with Intel or your
  vendor.  This file may be modified by the user, subject to
  the additional terms of the license agreement
--*/
/*++

Copyright (c) 2016 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  DynamicLoad.c - Implement dynamic symbol load

Abstract:
--*/

#include "Host.h"
#include <dlfcn.h>

/**
  Implementation of Unix dlopen function. See dlopen
  documentation for details.

  @param [in] FileName      Name of library to open.
  @param [in] Flag          Flags

  @retval  Handle to the library, NULL on failure.

**/

VOID *
Emu_Dlopen (
  IN CONST CHAR8 *FileName,
  IN INT16 Flag
  )
{

  return dlopen((const char *)FileName, (int)Flag);

} // Emu_Dlopen

/**
  Implementation of Unix dlerror function. See dlerror
  documentation for details.

  @param   None

  @retval  String describing the last error.

**/

CHAR8 *
Emu_Dlerror (
  VOID
  )
{

  return (CHAR8 *)dlerror();


} // Emu_Dlerror

/**
  Implementation of Unix dlsym function. See dlsym
  documentation for details.

  @param [in] Handle  Ptr to a handle to a library
  @param [in] Symbol  The symobl to locate.

  @retval  Ptr to the symbol. NULL on failure.

**/

VOID *
Emu_Dlsym (
  IN VOID* Handle,
  IN CONST CHAR8* Symbol
  )
{

  return (VOID *)dlsym((void *)Handle, (const char *)Symbol);

} // Emu_Dlsym

/**
  Implementation of Unix dlclose function. See dlclose
  documentation for details.

  @param [in] Handle  Ptr to a handle to a library.

  @retval  0 on success, non-zero on failure.

**/

INT16
Emu_Dlclose (
  IN VOID* Handle
  )
{

  return (INT16)dlclose((void *)Handle);

} // Emu_Dlclose

EMU_DYNAMIC_LOAD_PROTOCOL gDynamicLoad = {
  Emu_Dlopen,
  Emu_Dlerror,
  Emu_Dlsym,
  Emu_Dlclose
};


