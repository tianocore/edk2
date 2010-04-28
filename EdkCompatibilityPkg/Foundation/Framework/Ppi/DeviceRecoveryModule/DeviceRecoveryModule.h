/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  DeviceRecoveryModule.h
    
Abstract:

  Device Recovery Module PPI as defined in EFI 2.0

--*/

#ifndef _PEI_DEVICE_RECOVERY_MODULE_PPI_H
#define _PEI_DEVICE_RECOVERY_MODULE_PPI_H

#define PEI_DEVICE_RECOVERY_MODULE_INTERFACE_PPI \
  { \
    0x0DE2CE25, 0x446A, 0x45a7, {0xBF, 0xC9, 0x37, 0xDA, 0x26, 0x34, 0x4B, 0x37} \
  }

EFI_FORWARD_DECLARATION (PEI_DEVICE_RECOVERY_MODULE_INTERFACE);

typedef
EFI_STATUS
(EFIAPI *PEI_DEVICE_GET_NUMBER_RECOVERY_CAPSULE) (
  IN EFI_PEI_SERVICES                               **PeiServices,
  IN PEI_DEVICE_RECOVERY_MODULE_INTERFACE           * This,
  OUT UINTN                                         *NumberRecoveryCapsules
  );

typedef
EFI_STATUS
(EFIAPI *PEI_DEVICE_GET_RECOVERY_CAPSULE_INFO) (
  IN  EFI_PEI_SERVICES                              **PeiServices,
  IN PEI_DEVICE_RECOVERY_MODULE_INTERFACE           * This,
  IN  UINTN                                         CapsuleInstance,
  OUT UINTN                                         *Size,
  OUT EFI_GUID                                      * CapsuleType
  );

typedef
EFI_STATUS
(EFIAPI *PEI_DEVICE_LOAD_RECOVERY_CAPSULE) (
  IN OUT EFI_PEI_SERVICES                         **PeiServices,
  IN PEI_DEVICE_RECOVERY_MODULE_INTERFACE         * This,
  IN UINTN                                        CapsuleInstance,
  OUT VOID                                        *Buffer
  );

struct _PEI_DEVICE_RECOVERY_MODULE_INTERFACE {
  PEI_DEVICE_GET_NUMBER_RECOVERY_CAPSULE  GetNumberRecoveryCapsules;
  PEI_DEVICE_GET_RECOVERY_CAPSULE_INFO    GetRecoveryCapsuleInfo;
  PEI_DEVICE_LOAD_RECOVERY_CAPSULE        LoadRecoveryCapsule;
};

extern EFI_GUID gPeiDeviceRecoveryModulePpiGuid;

#endif
