/** @file
  Entrypoint of Extended SAL variable service module.

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"
#include "AuthService.h"

//
// Don't use module globals after the SetVirtualAddress map is signaled
//
EFI_EVENT mEfiVirtualNotifyEvent;

/**
  Common entry for Extended SAL Variable Services Class.

  This is the common entry of all functions of Extended SAL Variable Services Class.

  @param[in]  FunctionId        The Function ID of member function in Extended SAL Variable Services Class.
  @param[in]  Arg2              The 2nd parameter for SAL procedure call.
  @param[in]  Arg3              The 3rd parameter for SAL procedure call.
  @param[in]  Arg4              The 4th parameter for SAL procedure call.
  @param[in]  Arg5              The 5th parameter for SAL procedure call.
  @param[in]  Arg6              The 6th parameter for SAL procedure call.
  @param[in]  Arg7              The 7th parameter for SAL procedure call.
  @param[in]  Arg8              The 8th parameter for SAL procedure call.
  @param[in]  VirtualMode       The current calling mode for this function.
  @param[in]  Global            The context of this Extended SAL Variable Services Class call.

  @return                       The register of SAL.

**/
SAL_RETURN_REGS
EFIAPI
EsalVariableCommonEntry (
  IN  UINT64                                      FunctionId,
  IN  UINT64                                      Arg2,
  IN  UINT64                                      Arg3,
  IN  UINT64                                      Arg4,
  IN  UINT64                                      Arg5,
  IN  UINT64                                      Arg6,
  IN  UINT64                                      Arg7,
  IN  UINT64                                      Arg8,
  IN  BOOLEAN                                     VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL                        *Global
  )
{
  SAL_RETURN_REGS ReturnVal;
  
  ReturnVal.r9  = 0;
  ReturnVal.r10 = 0;
  ReturnVal.r11 = 0;

  switch (FunctionId) {
  case EsalGetVariableFunctionId:
    ReturnVal.Status = EsalGetVariable (
                         (CHAR16 *) Arg2,
                         (EFI_GUID *) Arg3,
                         (UINT32 *) Arg4,
                         (UINTN *) Arg5,
                         (VOID *) Arg6,
                         VirtualMode,
                         Global
                         );
    return ReturnVal;

  case EsalGetNextVariableNameFunctionId:
    ReturnVal.Status = EsalGetNextVariableName (
                         (UINTN *) Arg2,
                         (CHAR16 *) Arg3,
                         (EFI_GUID *) Arg4,
                         VirtualMode,
                         Global
                         );
    return ReturnVal;

  case EsalSetVariableFunctionId:
    ReturnVal.Status = EsalSetVariable (
                         (CHAR16 *) Arg2,
                         (EFI_GUID *) Arg3,
                         (UINT32) Arg4,
                         (UINTN) Arg5,
                         (VOID *) Arg6,
                         VirtualMode,
                         Global
                         );
    return ReturnVal;

  case EsalQueryVariableInfoFunctionId:
    ReturnVal.Status = EsalQueryVariableInfo (
                         (UINT32) Arg2,
                         (UINT64 *) Arg3,
                         (UINT64 *) Arg4,
                         (UINT64 *) Arg5,
                         VirtualMode,
                         Global
                         );
    return ReturnVal;

  default:
    ReturnVal.Status = EFI_SAL_INVALID_ARGUMENT;
    return ReturnVal;
  }
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param[in]  Event        The event whose notification function is being invoked.
  @param[in]  Context      The pointer to the notification function's context.

**/
VOID
EFIAPI
VariableClassAddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  UINTN Index;

  CopyMem (
    &mVariableModuleGlobal->VariableGlobal[Virtual],
    &mVariableModuleGlobal->VariableGlobal[Physical],
    sizeof (VARIABLE_GLOBAL)
    );

  EfiConvertPointer (
    0x0,
    (VOID **) &mVariableModuleGlobal->VariableGlobal[Virtual].NonVolatileVariableBase
    );
  EfiConvertPointer (
    0x0,
    (VOID **) &mVariableModuleGlobal->VariableGlobal[Virtual].VolatileVariableBase
    );

  mVariableModuleGlobal->PlatformLangCodes[Virtual] = mVariableModuleGlobal->PlatformLangCodes[Physical];
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->PlatformLangCodes[Virtual]);

  mVariableModuleGlobal->LangCodes[Virtual] = mVariableModuleGlobal->LangCodes[Physical];
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->LangCodes[Virtual]);

  mVariableModuleGlobal->PlatformLang[Virtual] = mVariableModuleGlobal->PlatformLang[Physical];
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->PlatformLang[Virtual]);

  CopyMem (
    mVariableModuleGlobal->VariableName[Virtual],
    mVariableModuleGlobal->VariableName[Physical],
    sizeof (mVariableModuleGlobal->VariableName[Physical])
    );
  for (Index = 0; Index < NUM_VAR_NAME; Index++) {
    EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->VariableName[Virtual][Index]);
  }

  mVariableModuleGlobal->GlobalVariableGuid[Virtual] = &gEfiGlobalVariableGuid;
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->GlobalVariableGuid[Virtual]);

  mVariableModuleGlobal->AuthenticatedVariableGuid[Virtual] = &gEfiAuthenticatedVariableGuid;
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->AuthenticatedVariableGuid[Virtual]);

  mVariableModuleGlobal->CertRsa2048Sha256Guid[Virtual] = &gEfiCertRsa2048Sha256Guid;
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->CertRsa2048Sha256Guid[Virtual]);

  mVariableModuleGlobal->ImageSecurityDatabaseGuid[Virtual] = &gEfiImageSecurityDatabaseGuid;
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->ImageSecurityDatabaseGuid[Virtual]);

  mVariableModuleGlobal->HashContext[Virtual] = mVariableModuleGlobal->HashContext[Physical];
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->HashContext[Virtual]);
}

/**
  Entry point of Extended SAL Variable service module.

  This function is the entry point of Extended SAL Variable service module.
  It registers all functions of Extended SAL Variable class, initializes
  variable store for non-volatile and volatile variables, and registers
  notification function for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  @param[in]  ImageHandle   The Image handle of this driver.
  @param[in]  SystemTable   The pointer of EFI_SYSTEM_TABLE.

  @retval     EFI_SUCCESS   Extended SAL Variable Services Class successfully registered.

**/
EFI_STATUS
EFIAPI
VariableServiceInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VariableClassAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mEfiVirtualNotifyEvent
                  );

  ASSERT_EFI_ERROR (Status);

  Status = VariableCommonInitialize (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);

  //
  // Authenticated variable initialize
  //
  Status = AutenticatedVariableServiceInitialize ();
  ASSERT_EFI_ERROR (Status);

  FlushHob2Nv ();

  //
  // Register All the Functions with Extended SAL Variable Services Class
  //
  RegisterEsalClass (
    EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_LO,
    EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_HI,
    mVariableModuleGlobal,
    EsalVariableCommonEntry,
    EsalGetVariableFunctionId,
    EsalVariableCommonEntry,
    EsalGetNextVariableNameFunctionId,
    EsalVariableCommonEntry,
    EsalSetVariableFunctionId,
    EsalVariableCommonEntry,
    EsalQueryVariableInfoFunctionId,
    NULL
    );

  return EFI_SUCCESS;
}
