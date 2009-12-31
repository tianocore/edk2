/** @file
  Include file for SMM Base Helper SMM driver.
  
  Copyright (c) 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef  _SMM_BASE_HELPER_H_
#define  _SMM_BASE_HELPER_H_

#include <PiSmm.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/DevicePathLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Guid/SmmBaseThunkCommunication.h>
#include <Protocol/SmmBaseHelperReady.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/LoadedImage.h>
#include "CpuSaveState.h"

///
/// Structure for tracking paired information of registered Framework SMI handler
/// and correpsonding dispatch handle for SMI handler thunk.
///
typedef struct {
  LIST_ENTRY                    Link;
  EFI_HANDLE                    DispatchHandle;
  EFI_HANDLE                    SmmImageHandle;
  EFI_SMM_CALLBACK_ENTRY_POINT  CallbackAddress;
} CALLBACK_INFO;

typedef struct {
  ///
  /// PI SMM CPU Save State register index
  ///
  EFI_SMM_SAVE_STATE_REGISTER   Register;
  ///
  /// Offset in Framework SMST
  ///
  UINTN                         Offset;
} CPU_SAVE_STATE_CONVERSION;

#define CPU_SAVE_STATE_GET_OFFSET(Field)  (UINTN)(&(((EFI_SMM_CPU_SAVE_STATE *) 0)->Ia32SaveState.Field))

#endif  
