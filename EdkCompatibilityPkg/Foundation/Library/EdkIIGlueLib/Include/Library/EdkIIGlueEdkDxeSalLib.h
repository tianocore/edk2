/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueEdkDxeSalLib.h
  
Abstract: 

--*/


#ifndef __EDKII_GLUE_ESAL_SERVICE_LIB_H__
#define __EDKII_GLUE_ESAL_SERVICE_LIB_H__

#include EFI_PROTOCOL_DEFINITION (ExtendedSalGuid)
#include EFI_PROTOCOL_DEFINITION (ExtendedSalBootService)

EFI_STATUS
EFIAPI
RegisterEsalFunction (
  IN  UINT64                                    FunctionId,
  IN  EFI_GUID                                  *ClassGuid,
  IN  SAL_INTERNAL_EXTENDED_SAL_PROC            Function,
  IN  VOID                                      *ModuleGlobal
  )
/*++

Routine Description:

  Register ESAL Class Function and it's asociated global.
  This function is boot service only!

Arguments:
  FunctionId    - ID of function to register
  ClassGuid     - GUID of function class 
  Function      - Function to register under ClassGuid/FunctionId pair
  ModuleGlobal  - Module global for Function.

Returns: 
  EFI_SUCCESS - If ClassGuid/FunctionId Function was registered.

--*/
;

EFI_STATUS
EFIAPI
RegisterEsalClass (
  IN  EFI_GUID                                  *ClassGuid,
  IN  VOID                                      *ModuleGlobal,
  ...
  )
/*++

Routine Description:

  Register ESAL Class and it's asociated global.
  This function is boot service only!

Arguments:
  ClassGuid     - GUID of function class 
  ModuleGlobal  - Module global for Function.
  ..            - SAL_INTERNAL_EXTENDED_SAL_PROC and FunctionId pairs. NULL 
                  indicates the end of the list.

Returns: 
  EFI_SUCCESS - All members of ClassGuid registered

--*/
;

SAL_RETURN_REGS
EFIAPI
EfiCallEsalService (
  IN  EFI_GUID                                      *ClassGuid,
  IN  UINT64                                        FunctionId,
  IN  UINT64                                        Arg2,
  IN  UINT64                                        Arg3,
  IN  UINT64                                        Arg4,
  IN  UINT64                                        Arg5,
  IN  UINT64                                        Arg6,
  IN  UINT64                                        Arg7,
  IN  UINT64                                        Arg8
  )
/*++

Routine Description:

  Call module that is not linked direclty to this module. This code is IP 
  relative and hides the binding issues of virtual or physical calling. The
  function that gets dispatched has extra arguments that include the registered
  module global and a boolean flag to indicate if the system is in virutal mode.

Arguments:
  ClassGuid   - GUID of function
  FunctionId  - Function in ClassGuid to call
  Arg2        - Argument 2 ClassGuid/FunctionId defined
  Arg3        - Argument 3 ClassGuid/FunctionId defined
  Arg4        - Argument 4 ClassGuid/FunctionId defined
  Arg5        - Argument 5 ClassGuid/FunctionId defined
  Arg6        - Argument 6 ClassGuid/FunctionId defined
  Arg7        - Argument 7 ClassGuid/FunctionId defined
  Arg8        - Argument 8 ClassGuid/FunctionId defined

Returns: 
  Status of ClassGuid/FuncitonId

--*/
;

SAL_RETURN_REGS
EFIAPI
SetEsalVirtualEntryPoint (
  IN  UINT64  EntryPoint,
  IN  UINT64  Gp
  );

SAL_RETURN_REGS
EFIAPI
SetEsalPhysicalEntryPoint (
  IN  UINT64  EntryPoint,
  IN  UINT64  Gp
  );

SAL_RETURN_REGS
EFIAPI
GetEsalEntryPoint (
  VOID
  );


#endif
