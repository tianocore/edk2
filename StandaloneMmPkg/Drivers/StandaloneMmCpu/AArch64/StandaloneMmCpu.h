/** @file
  Private header with declarations and definitions specific to the MM Standalone
  CPU driver

  Copyright (c) 2017 - 2018, ARM Limited. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ARM_TF_CPU_DRIVER_H_
#define _ARM_TF_CPU_DRIVER_H_

#include <Protocol/MmCommunication.h>
#include <Protocol/MmConfiguration.h>
#include <Protocol/MmCpu.h>
#include <Guid/MpInformation.h>

//
// CPU driver initialization specific declarations
//
extern EFI_MM_SYSTEM_TABLE *mMmst;

//
// CPU State Save protocol specific declarations
//
extern EFI_MM_CPU_PROTOCOL mMmCpuState;

//
// MM event handling specific declarations
//
extern EFI_MM_COMMUNICATE_HEADER    **PerCpuGuidedEventContext;
extern EFI_MMRAM_DESCRIPTOR          mNsCommBuffer;
extern MP_INFORMATION_HOB_DATA       *mMpInformationHobData;
extern EFI_MM_CONFIGURATION_PROTOCOL mMmConfig;

EFI_STATUS
PiMmStandloneArmTfCpuDriverEntry (
  IN UINTN EventId,
  IN UINTN CpuNumber,
  IN UINTN NsCommBufferAddr
  );

EFI_STATUS
EFIAPI
PiMmCpuTpFwRootMmiHandler (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  );

EFI_STATUS _PiMmStandloneArmTfCpuDriverEntry (
  IN UINTN EventId,
  IN UINTN CpuNumber,
  IN UINTN NsCommBufferAddr
  );

#endif
