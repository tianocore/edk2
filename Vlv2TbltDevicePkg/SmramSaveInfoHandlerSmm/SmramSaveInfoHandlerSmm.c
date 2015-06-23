/** @file
  A helper driver to save information to SMRAM after SMRR is enabled.

  This driver is for ECP platforms.

  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#include <PiSmm.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Protocol/SmmSwDispatch.h>
#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmControl.h>
#include <Guid/Vlv2DeviceRefCodePkgTokenSpace.h>

#define SMM_FROM_SMBASE_DRIVER        0x55
#define SMM_FROM_CPU_DRIVER_SAVE_INFO 0x81

#define EFI_SMRAM_CPU_NVS_HEADER_GUID \
  { \
    0x429501d9, 0xe447, 0x40f4, 0x86, 0x7b, 0x75, 0xc9, 0x3a, 0x1d, 0xb5, 0x4e \
  }

UINT8    mSmiDataRegister;
BOOLEAN  mLocked = FALSE;
EFI_GUID mSmramCpuNvsHeaderGuid = EFI_SMRAM_CPU_NVS_HEADER_GUID;

/**
  Dispatch function for a Software SMI handler.

  @param  DispatchHandle        The handle of this dispatch function.
  @param  DispatchContext       The pointer to the dispatch function's context.
                                The SwSmiInputValue field is filled in
                                by the software dispatch driver prior to
                                invoking this dispatch function.
                                The dispatch function will only be called
                                for input values for which it is registered.

  @return None

**/
VOID
EFIAPI
SmramSaveInfoHandler (
  IN  EFI_HANDLE                    DispatchHandle,
  IN  EFI_SMM_SW_DISPATCH_CONTEXT   *DispatchContext
  )
{
  ASSERT (DispatchContext != NULL);
  ASSERT (DispatchContext->SwSmiInputValue == SMM_FROM_SMBASE_DRIVER);

  if (!mLocked && IoRead8 (mSmiDataRegister) == SMM_FROM_CPU_DRIVER_SAVE_INFO) {
      CopyMem (
        (VOID *)(UINTN)(PcdGetEx64 (&gEfiVLVTokenSpaceGuid, PcdCpuLockBoxDataAddress)),
        (VOID *)(UINTN)(PcdGetEx64 (&gEfiVLVTokenSpaceGuid, PcdCpuSmramCpuDataAddress)),
        (UINTN)(PcdGetEx64 (&gEfiVLVTokenSpaceGuid, PcdCpuLockBoxSize))
        );
  }
}

/**
  Smm Ready To Lock event notification handler.

  It sets a flag indicating that SMRAM has been locked.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification handler runs successfully.
 **/
EFI_STATUS
EFIAPI
SmmReadyToLockEventNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  mLocked = TRUE;
  return EFI_SUCCESS;
}

/**
  Entry point function of this driver.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
SmramSaveInfoHandlerSmmMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_SMM_SW_DISPATCH_PROTOCOL  *SmmSwDispatch;
  EFI_SMM_SW_DISPATCH_CONTEXT   SmmSwDispatchContext;
  EFI_HANDLE                    DispatchHandle;
  EFI_SMM_CONTROL_PROTOCOL      *SmmControl;
  EFI_SMM_CONTROL_REGISTER      SmmControlRegister;
  VOID                          *Registration;

  //
  // Get SMI data register
  //
  Status = SystemTable->BootServices->LocateProtocol (
                                        &gEfiSmmControlProtocolGuid,
                                        NULL,
                                        (VOID **)&SmmControl
                                        );
  ASSERT_EFI_ERROR (Status);
  Status = SmmControl->GetRegisterInfo (SmmControl, &SmmControlRegister);
  ASSERT_EFI_ERROR (Status);
  mSmiDataRegister = SmmControlRegister.SmiDataRegister;

  //
  // Register software SMI handler
  //

  Status = SystemTable->BootServices->LocateProtocol (
                                        &gEfiSmmSwDispatchProtocolGuid,
                                        NULL,
                                        (VOID **)&SmmSwDispatch
                                        );
  ASSERT_EFI_ERROR (Status);

  SmmSwDispatchContext.SwSmiInputValue = SMM_FROM_SMBASE_DRIVER;
  Status = SmmSwDispatch->Register (
                            SmmSwDispatch,
                            &SmramSaveInfoHandler,
                            &SmmSwDispatchContext,
                            &DispatchHandle
                            );
  ASSERT_EFI_ERROR (Status);

  //
  // Register SMM Ready To Lock Protocol notification
  //
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmReadyToLockProtocolGuid,
                    SmmReadyToLockEventNotify,
                    &Registration
                    );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

