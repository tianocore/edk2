/** @file
  Execute 32-bit code in Long Mode
  Provide a thunk function to transition from long mode to compatibility mode to execute 32-bit code and then transit
  back to long mode.
  
  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "ScriptSave.h"
/**
  Wrapper for a thunk  to transition from long mode to compatibility mode to execute 32-bit code and then transit back to
  long mode.
  
  @param  Function     The 32bit code entry to be executed.
  @param  Param1       The first parameter to pass to 32bit code
  @param  Param2       The second parameter to pass to 32bit code
  @retval EFI_SUCCESS  Execute 32bit code successfully.
  @retval other        Something wrong when execute the 32bit code 
              
**/  
EFI_STATUS
Execute32BitCode (
  IN UINT64      Function,
  IN UINT64      Param1,
  IN UINT64      Param2
  )
{
  DISPATCH_ENTRYPOINT_FUNC  EntryFunc;
  EFI_STATUS                 Status;

  EntryFunc = (DISPATCH_ENTRYPOINT_FUNC) (UINTN) (Function);
  Status    = EntryFunc ((VOID *)(UINTN)Param1, (VOID *)(UINTN)Param2);

  return Status;
}

