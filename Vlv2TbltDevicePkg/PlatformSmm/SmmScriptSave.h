/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

    SmmScriptSave.h

Abstract:

  This is an implementation of the BootScript at run time.

--*/

#ifndef _RUNTIME_SCRIPT_SAVE_H
#define _RUNTIME_SCRIPT_SAVE_H

#include "Efi.h"
#include "EfiBootScript.h"


typedef EFI_PHYSICAL_ADDRESS     EFI_SMM_SCRIPT_TABLE;

EFI_STATUS
SmmBootScriptCreateTable (
  IN OUT EFI_SMM_SCRIPT_TABLE    *ScriptTable,
  IN UINTN                       Type
  );

EFI_STATUS
SmmBootScriptWrite (
  IN OUT EFI_SMM_SCRIPT_TABLE    *ScriptTable,
  IN UINTN                       Type,
  IN UINT16                      OpCode,
  ...
  );

EFI_STATUS
SmmBootScriptCloseTable (
  IN EFI_SMM_SCRIPT_TABLE        ScriptTableBase,
  IN EFI_SMM_SCRIPT_TABLE        ScriptTablePtr,
  IN UINTN                       Type
  );

#endif
