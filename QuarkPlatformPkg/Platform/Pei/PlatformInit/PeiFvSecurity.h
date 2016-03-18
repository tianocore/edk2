/** @file
Definition of Pei Core Structures and Services

Copyright (c) 2013 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_FV_SECURITY_H_
#define _PEI_FV_SECURITY_H_

#include <Ppi/FirmwareVolume.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Callback function to perform FV security checking on a FV Info PPI.

  @param PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param NotifyDescriptor  Address of the notification descriptor data structure.
  @param Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
FirmwareVolmeInfoPpiNotifySecurityCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  );

/**
  Authenticates the Firmware Volume

  @param CurrentFvAddress   Pointer to the current Firmware Volume under consideration

  @retval EFI_SUCCESS       Firmware Volume is legal

**/
EFI_STATUS
PeiSecurityVerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER  *CurrentFvAddress
  );

/**

  Entry point for the PEI Security PEIM
  Sets up a notification to perform PEI security checking

  @param  FfsHeader    Not used.
  @param  PeiServices  General purpose services available to every PEIM.

  @return EFI_SUCCESS  PEI Security notification installed successfully.
          All others: PEI Security notification failed to install.

**/
EFI_STATUS
PeiInitializeFvSecurity (
  VOID
  );

#endif
