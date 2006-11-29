/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EsalServiceLib.c

Abstract:

--*/

#include <Ipf/IpfDefines.h>

BOOLEAN mLibraryInitialized = FALSE;
STATIC EXTENDED_SAL_BOOT_SERVICE_PROTOCOL *mEsalBootService;
STATIC EFI_PLABEL                         mPlabel;

EFI_STATUS
EFIAPI
DxeSalLibConstruct (
//  IN EFI_HANDLE        ImageHandle,
//  IN EFI_SYSTEM_TABLE  *SystemTable
  VOID
  )
{
  EFI_PLABEL  *Plabel;
  EFI_STATUS  Status;

  if (mLibraryInitialized == TRUE) {
    return EFI_SUCCESS;
  }
  mLibraryInitialized = TRUE;

  //
  // The protocol contains a function pointer, which is an indirect procedure call.
  // An indirect procedure call goes through a plabel, and pointer to a function is
  // a pointer to a plabel. To implement indirect procedure calls that can work in
  // both physical and virtual mode, two plabels are required (one physical and one
  // virtual). So lets grap the physical PLABEL for the EsalEntryPoint and store it
  // away. We cache it in a module global, so we can register the vitrual version.
  //
  Status              = gBS->LocateProtocol (&gEfiExtendedSalBootServiceProtocolGuid, NULL, &mEsalBootService);
  ASSERT_EFI_ERROR (Status);

  Plabel              = (EFI_PLABEL *) (UINTN) mEsalBootService->ExtendedSalProc;

  mPlabel.EntryPoint  = Plabel->EntryPoint;
  mPlabel.GP          = Plabel->GP;
  SetEsalPhysicalEntryPoint (mPlabel.EntryPoint, mPlabel.GP);

  return Status;
}

VOID
EFIAPI
DxeSalVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

  Fixup virtual address pointer of label.

Arguments:

  Event   - The Event that is being processed
  
  Context - Event Context

Returns: 

  None

--*/
{
  EfiConvertPointer (0x0, (VOID **) &mPlabel.EntryPoint);
  EfiConvertPointer (EFI_IPF_GP_POINTER, (VOID **) &mPlabel.GP);

  SetEsalVirtualEntryPoint (mPlabel.EntryPoint, mPlabel.GP);
}

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
{
  DxeSalLibConstruct ();
  return mEsalBootService->AddExtendedSalProc (
                            mEsalBootService,
                            ClassGuid,
                            FunctionId,
                            Function,
                            ModuleGlobal
                            );
}

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
  ...           - SAL_INTERNAL_EXTENDED_SAL_PROC and FunctionId pairs. NULL 
                  indicates the end of the list.

Returns: 
  EFI_SUCCESS - All members of ClassGuid registered

--*/
{
  VA_LIST                         Args;
  EFI_STATUS                      Status;
  SAL_INTERNAL_EXTENDED_SAL_PROC  Function;
  UINT64                          FunctionId;
  EFI_HANDLE                      NewHandle;

  VA_START (Args, ModuleGlobal);

  Status = EFI_SUCCESS;
  while (!EFI_ERROR (Status)) {
    Function = (SAL_INTERNAL_EXTENDED_SAL_PROC) VA_ARG (Args, SAL_INTERNAL_EXTENDED_SAL_PROC);
    if (Function == NULL) {
      break;
    }

    FunctionId  = VA_ARG (Args, UINT64);

    Status      = RegisterEsalFunction (FunctionId, ClassGuid, Function, ModuleGlobal);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  NewHandle = NULL;
  return gBS->InstallProtocolInterface (
                &NewHandle,
                ClassGuid,
                EFI_NATIVE_INTERFACE,
                NULL
                );
}

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
{
  SAL_RETURN_REGS       ReturnReg;
  SAL_EXTENDED_SAL_PROC EsalProc;

  DxeSalLibConstruct ();
  ReturnReg = GetEsalEntryPoint ();
  if (ReturnReg.Status != EFI_SAL_SUCCESS) {
    return ReturnReg;
  }

  if (ReturnReg.r11 & PSR_IT_MASK) {
    //
    // Virtual mode plabel to entry point
    //
    EsalProc = (SAL_EXTENDED_SAL_PROC) ReturnReg.r10;
  } else {
    //
    // Physical mode plabel to entry point
    //
    EsalProc = (SAL_EXTENDED_SAL_PROC) ReturnReg.r9;
  }

  return EsalProc (
          ClassGuid,
          FunctionId,
          Arg2,
          Arg3,
          Arg4,
          Arg5,
          Arg6,
          Arg7,
          Arg8
          );
}
