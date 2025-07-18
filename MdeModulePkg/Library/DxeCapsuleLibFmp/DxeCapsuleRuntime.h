/** @file
  Capsule library runtime support.

  Copyright (c) 2026, Arm ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Library/DevicePathLib.h>

#include <Protocol/EsrtManagement.h>
#include <Protocol/FirmwareManagement.h>
#include <Protocol/FirmwareManagementProgress.h>
#include <Protocol/DevicePath.h>

typedef struct {
  EFI_HANDLE                                     Handle;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL               *Fmp;
  EDKII_FIRMWARE_MANAGEMENT_PROGRESS_PROTOCOL    *FmpProgress;
  EFI_DEVICE_PATH_PROTOCOL                       *FmpDevicePath;
  CHAR16                                         *DevicePathStr;
  UINTN                                          DevicePathSize;
} RUNTIME_FIRMWARE_MANAGEMENT_INFO;

extern EFI_SYSTEM_RESOURCE_TABLE         *mEsrtTable;
extern BOOLEAN                           mDxeCapsuleLibIsExitBootService;
extern RUNTIME_FIRMWARE_MANAGEMENT_INFO  *mRuntimeFmpList;
extern UINTN                             mRuntimeFmpCount;
extern EFI_FIRMWARE_IMAGE_DESCRIPTOR     *mRuntimeImageInfoBuffer;
extern VOID                              *mRuntimeCapsuleResultVariable;
extern EFI_HANDLE                        *mRuntimeMatchedHandleBuffer;
extern BOOLEAN                           *mRuntimeMatchedResetRequiredBuffer;
