/** @file
  This is an thunk implementation of the BootScript at run time. 
  
  SmmScriptLib in Framework implementation is to save S3 Boot Script in SMM runtime.
  Here is the header file to define the API in this thunk library.  
  
  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMM_SCRIPT_SAVE_H_
#define _SMM_SCRIPT_SAVE_H_

#include "Tiano.h"
#include "EfiBootScript.h"
#include "PiSmmDefinition.h"
#include "PiSmmS3SaveState.h"


typedef EFI_PHYSICAL_ADDRESS     EFI_SMM_SCRIPT_TABLE;
/**
  Intialize Boot Script table. 
  
  This function should be called in SMM mode. The Thunk implementation is try to 
  locate SmmSaveState protocol.
  
  @param  SystemTable         Pointer to the EFI sytem table
  @param  SmmScriptTablePages The expected ScriptTable page number
  @param  SmmScriptTableBase  The returned ScriptTable base address

  @retval EFI_OUT_OF_RESOURCES    No resource to do the initialization.
  @retval EFI_SUCCESS             Function has completed successfully.
  
**/
EFI_STATUS
EFIAPI
InitializeSmmScriptLib (
  IN  EFI_SYSTEM_TABLE     *SystemTable,
  IN  UINTN                SmmScriptTablePages,
  OUT EFI_PHYSICAL_ADDRESS *SmmScriptTableBase
  );
/**
  Create Boot Script table.
  
  It will be ignore and just return EFI_SUCCESS since the boot script table is 
  maintained by DxeBootScriptLib. Create Table is not needed.
  
  @param  ScriptTable     Pointer to the boot script table to create.
  @param  Type            The type of table to creat.

 
  @retval EFI_SUCCESS     Function has completed successfully.
  
**/
EFI_STATUS 
EFIAPI
SmmBootScriptCreateTable (
  IN OUT   EFI_SMM_SCRIPT_TABLE    *ScriptTable,
  IN       UINTN                   Type
  );
/**
  Adds a record into a specified Framework boot script table.

  This function is used to store a boot script record into a given boot
  script table in SMM runtime. The parameter is the same with definitionin BootScriptSave Protocol.
  
  @param  ScriptTable           Pointer to the script table to write to. In the thunk implementation, this parameter is ignored 
                                since the boot script table is maintained by BootScriptLib.
  @param  Type                  Not used.
  @param  OpCode                The operation code (opcode) number.
  @param  ...                   Argument list that is specific to each opcode. 

  @retval EFI_SUCCESS           The operation succeeded. A record was added into the
                                specified script table.
  @retval EFI_INVALID_PARAMETER The parameter is illegal or the given boot script is not supported.
                                If the opcode is unknow or not supported because of the PCD 
                                Feature Flags.
  @retval EFI_OUT_OF_RESOURCES  There is insufficient memory to store the boot script.
**/
EFI_STATUS 
EFIAPI
SmmBootScriptWrite (
  IN OUT   EFI_SMM_SCRIPT_TABLE    *ScriptTable,
  IN       UINTN                   Type,
  IN       UINT16                  OpCode,
  ...
  );
/**
  Close Boot Script table. 
  
  It will be ignore and just return EFI_SUCCESS since the boot script table 
  is maintained by DxeBootScriptLib. 

  @param  ScriptTableBase    Pointer to the boot script table to create.
  @param  ScriptTablePtr     Pointer to the script table to write to.
  @param  Type               The type of table to creat.
    
  @retval EFI_SUCCESS            -  Function has completed successfully.
  
**/
EFI_STATUS
EFIAPI
SmmBootScriptCloseTable (
  IN EFI_SMM_SCRIPT_TABLE        ScriptTableBase,
  IN EFI_SMM_SCRIPT_TABLE        ScriptTablePtr,
  IN UINTN                       Type
  );


#endif
