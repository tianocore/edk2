/** 
  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:


  PpmPolicy.c

Abstract:

  This file is a wrapper for Intel PPM Platform Policy driver.
  Get Setup Value to initilize Intel PPM DXE Platform Policy.

--*/
#include "PpmPolicy.h"
#include <Protocol/MpService.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/CpuIA32.h>

#include <PchRegs.h>
#include <Library/PchPlatformLib.h>

#define EFI_CPUID_FAMILY                      0x0F00
#define EFI_CPUID_MODEL                       0x00F0
#define EFI_CPUID_STEPPING                    0x000F



EFI_STATUS 
EFIAPI
PpmPolicyEntry(
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
)
{
  EFI_MP_SERVICES_PROTOCOL *MpService;
  EFI_CPUID_REGISTER        Cpuid01 = { 0, 0, 0, 0};
  EFI_HANDLE                Handle;
  EFI_STATUS                Status;
  UINTN                     CpuCount;
  UINT64                    MaxRatio;
  UINT8                     CPUMobileFeature;

  PCH_STEPPING              Stepping;


  gBS = SystemTable->BootServices;
  pBS = SystemTable->BootServices;
  pRS = SystemTable->RuntimeServices;

  //
  // Set PPM policy structure to known value
  //
  gBS->SetMem (&mDxePlatformPpmPolicy, sizeof(PPM_PLATFORM_POLICY_PROTOCOL), 0);

  //
  // Find the MpService Protocol
  //
  Status = pBS->LocateProtocol (&gEfiMpServiceProtocolGuid,
                                NULL,
                                (void **)&MpService
                               );
  ASSERT_EFI_ERROR (Status);

  //
  // Get processor count from MP service.
  //
  Status = MpService->GetNumberOfProcessors (MpService, &CpuCount, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // Store the CPUID for use by SETUP items.
  //
  AsmCpuid (EFI_CPUID_VERSION_INFO, &Cpuid01.RegEax, &Cpuid01.RegEbx, &Cpuid01.RegEcx, &Cpuid01.RegEdx);
  MaxRatio = ((RShiftU64 (AsmReadMsr64(EFI_MSR_IA32_PLATFORM_ID), 8)) & 0x1F);


  mDxePlatformPpmPolicy.Revision                       = PPM_PLATFORM_POLICY_PROTOCOL_REVISION_4;

  //Read CPU Mobile feature from PLATFORM_ID_MSR MSR(0x17) NOTFB_I_AM_NOT_MOBILE_FUSE_CLIAMC00H Bit 28
  //Bit Description: { Disables Mobile features 0 = I am NOT a mobile part 1 = I am a mobile part (default)"}
  CPUMobileFeature = ((RShiftU64 (AsmReadMsr64(EFI_MSR_IA32_PLATFORM_ID), 28)) & 0x1);

  if (!EFI_ERROR(Status)) {
    if (CPUMobileFeature == 1){//CPU mobile feature
      mDxePlatformPpmPolicy.FunctionEnables.EnableGv       = ICH_DEVICE_ENABLE;
      mDxePlatformPpmPolicy.FunctionEnables.EnableCx       = ICH_DEVICE_ENABLE;
      mDxePlatformPpmPolicy.FunctionEnables.EnableCxe      = ICH_DEVICE_DISABLE;
      mDxePlatformPpmPolicy.FunctionEnables.EnableTm       = ICH_DEVICE_ENABLE;
      //MaxC7
      mDxePlatformPpmPolicy.FunctionEnables.EnableC7       = ICH_DEVICE_ENABLE;
      mDxePlatformPpmPolicy.FunctionEnables.EnableC6       = ICH_DEVICE_ENABLE;
      mDxePlatformPpmPolicy.FunctionEnables.EnableC4       = ICH_DEVICE_ENABLE;
       
      
    }else{//CPU desktop feature
       mDxePlatformPpmPolicy.FunctionEnables.EnableGv       = ICH_DEVICE_DISABLE;
       mDxePlatformPpmPolicy.FunctionEnables.EnableCx       = ICH_DEVICE_DISABLE;
       mDxePlatformPpmPolicy.FunctionEnables.EnableCxe      = ICH_DEVICE_DISABLE;
       mDxePlatformPpmPolicy.FunctionEnables.EnableTm       = ICH_DEVICE_DISABLE;
       mDxePlatformPpmPolicy.FunctionEnables.EnableC4       = ICH_DEVICE_DISABLE;
       mDxePlatformPpmPolicy.FunctionEnables.EnableC6       = ICH_DEVICE_DISABLE;
       mDxePlatformPpmPolicy.FunctionEnables.EnableC7       = ICH_DEVICE_DISABLE;
    }


    mDxePlatformPpmPolicy.FunctionEnables.EnableProcHot  = ICH_DEVICE_ENABLE;
    mDxePlatformPpmPolicy.FunctionEnables.TStatesEnable  = ICH_DEVICE_ENABLE;

    
    Stepping = PchStepping();
    if (Stepping < PchB3) {
      // If SoC is B0~B2 Stepping, disable the Turbo
      mDxePlatformPpmPolicy.FunctionEnables.EnableTurboMode= ICH_DEVICE_DISABLE;
    } else {
      mDxePlatformPpmPolicy.FunctionEnables.EnableTurboMode= ICH_DEVICE_ENABLE;
    }
    
    mDxePlatformPpmPolicy.FunctionEnables.EnableTm      = ICH_DEVICE_ENABLE;

    mDxePlatformPpmPolicy.FunctionEnables.EnableCMP      = ICH_DEVICE_ENABLE;

  } else {
    mDxePlatformPpmPolicy.FunctionEnables.EnableGv       = ICH_DEVICE_ENABLE;
    mDxePlatformPpmPolicy.FunctionEnables.EnableCx       = ICH_DEVICE_ENABLE;
    mDxePlatformPpmPolicy.FunctionEnables.EnableCxe      = ICH_DEVICE_ENABLE;
    mDxePlatformPpmPolicy.FunctionEnables.EnableTm      = ICH_DEVICE_ENABLE;
    mDxePlatformPpmPolicy.FunctionEnables.EnableProcHot  = ICH_DEVICE_ENABLE;
    mDxePlatformPpmPolicy.FunctionEnables.EnableCMP       = ICH_DEVICE_DISABLE;
    mDxePlatformPpmPolicy.FunctionEnables.TStatesEnable  = ICH_DEVICE_ENABLE;
    mDxePlatformPpmPolicy.FunctionEnables.EnableTurboMode= ICH_DEVICE_ENABLE;
    mDxePlatformPpmPolicy.FunctionEnables.EnableC4       = ICH_DEVICE_ENABLE;
    mDxePlatformPpmPolicy.FunctionEnables.EnableC6       = ICH_DEVICE_ENABLE;
  }



  mDxePlatformPpmPolicy.S3RestoreMsrSwSmiNumber                       = S3_RESTORE_MSR_SW_SMI;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                                                  &Handle,
                                                  &gPpmPlatformPolicyProtocolGuid,
                                                  &mDxePlatformPpmPolicy,
                                                  NULL
                                                  );

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
