/** @file

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_PLATFORM_LIB_H_
#define _FSP_PLATFORM_LIB_H_

/**
  Get system memory resource descriptor by owner.

  @param[in] OwnerGuid   resource owner guid
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
EFIAPI
FspGetResourceDescriptorByOwner (
  IN EFI_GUID  *OwnerGuid
  );

/**
  Get system memory from HOB.

  @param[in,out] LowMemoryLength   less than 4G memory length
  @param[in,out] HighMemoryLength  greater than 4G memory length
**/
VOID
EFIAPI
FspGetSystemMemorySize (
  IN OUT UINT64  *LowMemoryLength,
  IN OUT UINT64  *HighMemoryLength
  );

/**
  Set a new stack frame for the continuation function.

**/
VOID
EFIAPI
FspSetNewStackFrame (
  VOID
  );

/**
  This function transfer control back to BootLoader after FspSiliconInit.

**/
VOID
EFIAPI
FspSiliconInitDone (
  VOID
  );

/**
  This function returns control to BootLoader after MemoryInitApi.

  @param[in,out] HobListPtr The address of HobList pointer.
**/
VOID
EFIAPI
FspMemoryInitDone (
  IN OUT VOID  **HobListPtr
  );

/**
  This function returns control to BootLoader after TempRamExitApi.

**/
VOID
EFIAPI
FspTempRamExitDone (
  VOID
  );

/**
  This function handle NotifyPhase API call from the BootLoader.
  It gives control back to the BootLoader after it is handled. If the
  Notification code is a ReadyToBoot event, this function will return
  and FSP continues the remaining execution until it reaches the DxeIpl.

**/
VOID
EFIAPI
FspWaitForNotify (
  VOID
  );

/**
  This function transfer control back to BootLoader after FspSiliconInit.

  @param[in] Status return status for the FspSiliconInit.
**/
VOID
EFIAPI
FspSiliconInitDone2 (
  IN EFI_STATUS  Status
  );

/**
  This function returns control to BootLoader after MemoryInitApi.

  @param[in] Status return status for the MemoryInitApi.
  @param[in,out] HobListPtr The address of HobList pointer.
**/
VOID
EFIAPI
FspMemoryInitDone2 (
  IN EFI_STATUS  Status,
  IN OUT VOID    **HobListPtr
  );

/**
  This function returns control to BootLoader after TempRamExitApi.

  @param[in] Status return status for the TempRamExitApi.
**/
VOID
EFIAPI
FspTempRamExitDone2 (
  IN EFI_STATUS  Status
  );

/**
  Calculate TemporaryRam Size using Base address.

  @param[in]  TemporaryRamBase         the address of target memory
  @param[out] TemporaryRamSize         the size of target memory
**/
VOID
EFIAPI
ReadTemporaryRamSize (
  IN  UINT32  TemporaryRamBase,
  OUT UINT32  *TemporaryRamSize
  );

#endif
