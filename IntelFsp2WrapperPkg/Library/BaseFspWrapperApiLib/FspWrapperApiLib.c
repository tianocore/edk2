/** @file
  Provide FSP API related function.

  Copyright (c) 2014 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/FspWrapperApiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

/**
  Wrapper for a thunk to transition from long mode to compatibility mode to execute 32-bit code and then transit back to
  long mode.

  @param[in] Function     The 32bit code entry to be executed.
  @param[in] Param1       The first parameter to pass to 32bit code.
  @param[in] Param2       The second parameter to pass to 32bit code.

  @return EFI_STATUS.
**/
EFI_STATUS
Execute32BitCode (
  IN UINT64  Function,
  IN UINT64  Param1,
  IN UINT64  Param2
  );

/**
  Wrapper to execute 64-bit code directly from long mode.

  @param[in] Function     The 64bit code entry to be executed.
  @param[in] Param1       The first parameter to pass to 64bit code.
  @param[in] Param2       The second parameter to pass to 64bit code.

  @return EFI_STATUS.
**/
EFI_STATUS
Execute64BitCode (
  IN UINT64  Function,
  IN UINT64  Param1,
  IN UINT64  Param2
  );

/**
  Find FSP header pointer.

  @param[in] FlashFvFspBase Flash address of FSP FV.

  @return FSP header pointer.
**/
FSP_INFO_HEADER *
EFIAPI
FspFindFspHeader (
  IN EFI_PHYSICAL_ADDRESS  FlashFvFspBase
  )
{
  UINT8  *CheckPointer;

  CheckPointer = (UINT8 *)(UINTN)FlashFvFspBase;

  if (((EFI_FIRMWARE_VOLUME_HEADER *)CheckPointer)->Signature != EFI_FVH_SIGNATURE) {
    return NULL;
  }

  if (((EFI_FIRMWARE_VOLUME_HEADER *)CheckPointer)->ExtHeaderOffset != 0) {
    CheckPointer = CheckPointer + ((EFI_FIRMWARE_VOLUME_HEADER *)CheckPointer)->ExtHeaderOffset;
    CheckPointer = CheckPointer + ((EFI_FIRMWARE_VOLUME_EXT_HEADER *)CheckPointer)->ExtHeaderSize;
    CheckPointer = (UINT8 *)ALIGN_POINTER (CheckPointer, 8);
  } else {
    CheckPointer = CheckPointer + ((EFI_FIRMWARE_VOLUME_HEADER *)CheckPointer)->HeaderLength;
  }

  CheckPointer = CheckPointer + sizeof (EFI_FFS_FILE_HEADER);

  if (((EFI_RAW_SECTION *)CheckPointer)->Type != EFI_SECTION_RAW) {
    return NULL;
  }

  CheckPointer = CheckPointer + sizeof (EFI_RAW_SECTION);

  return (FSP_INFO_HEADER *)CheckPointer;
}

/**
  Call FSP API - FspNotifyPhase.

  @param[in] NotifyPhaseParams Address pointer to the NOTIFY_PHASE_PARAMS structure.

  @return EFI status returned by FspNotifyPhase API.
**/
EFI_STATUS
EFIAPI
CallFspNotifyPhase (
  IN NOTIFY_PHASE_PARAMS  *NotifyPhaseParams
  )
{
  FSP_INFO_HEADER   *FspHeader;
  FSP_NOTIFY_PHASE  NotifyPhaseApi;
  EFI_STATUS        Status;
  BOOLEAN           InterruptState;

  FspHeader = (FSP_INFO_HEADER *)FspFindFspHeader (PcdGet32 (PcdFspsBaseAddress));
  if (FspHeader == NULL) {
    return EFI_DEVICE_ERROR;
  }

  NotifyPhaseApi = (FSP_NOTIFY_PHASE)((UINTN)FspHeader->ImageBase + FspHeader->NotifyPhaseEntryOffset);
  InterruptState = SaveAndDisableInterrupts ();
  if ((FspHeader->ImageAttribute & IMAGE_ATTRIBUTE_64BIT_MODE_SUPPORT) == FSP_IA32) {
    Status = Execute32BitCode ((UINTN)NotifyPhaseApi, (UINTN)NotifyPhaseParams, (UINTN)NULL);
  } else {
    Status = Execute64BitCode ((UINTN)NotifyPhaseApi, (UINTN)NotifyPhaseParams, (UINTN)NULL);
  }

  SetInterruptState (InterruptState);

  return Status;
}

/**
  Call FSP API - FspMemoryInit.

  @param[in]  FspmUpdDataPtr Address pointer to the FSP_MEMORY_INIT_PARAMS structure.
  @param[out] HobListPtr     Address of the HobList pointer.

  @return EFI status returned by FspMemoryInit API.
**/
EFI_STATUS
EFIAPI
CallFspMemoryInit (
  IN VOID   *FspmUpdDataPtr,
  OUT VOID  **HobListPtr
  )
{
  FSP_INFO_HEADER  *FspHeader;
  FSP_MEMORY_INIT  FspMemoryInitApi;
  EFI_STATUS       Status;
  BOOLEAN          InterruptState;

  FspHeader = (FSP_INFO_HEADER *)FspFindFspHeader (PcdGet32 (PcdFspmBaseAddress));
  if (FspHeader == NULL) {
    return EFI_DEVICE_ERROR;
  }

  FspMemoryInitApi = (FSP_MEMORY_INIT)((UINTN)FspHeader->ImageBase + FspHeader->FspMemoryInitEntryOffset);
  InterruptState   = SaveAndDisableInterrupts ();
  if ((FspHeader->ImageAttribute & IMAGE_ATTRIBUTE_64BIT_MODE_SUPPORT) == FSP_IA32) {
    Status = Execute32BitCode ((UINTN)FspMemoryInitApi, (UINTN)FspmUpdDataPtr, (UINTN)HobListPtr);
  } else {
    Status = Execute64BitCode ((UINTN)FspMemoryInitApi, (UINTN)FspmUpdDataPtr, (UINTN)HobListPtr);
  }

  SetInterruptState (InterruptState);

  return Status;
}

/**
  Call FSP API - TempRamExit.

  @param[in] TempRamExitParam    Address pointer to the TempRamExit parameters structure.

  @return EFI status returned by TempRamExit API.
**/
EFI_STATUS
EFIAPI
CallTempRamExit (
  IN VOID  *TempRamExitParam
  )
{
  FSP_INFO_HEADER    *FspHeader;
  FSP_TEMP_RAM_EXIT  TempRamExitApi;
  EFI_STATUS         Status;
  BOOLEAN            InterruptState;

  FspHeader = (FSP_INFO_HEADER *)FspFindFspHeader (PcdGet32 (PcdFspmBaseAddress));
  if (FspHeader == NULL) {
    return EFI_DEVICE_ERROR;
  }

  TempRamExitApi = (FSP_TEMP_RAM_EXIT)((UINTN)FspHeader->ImageBase + FspHeader->TempRamExitEntryOffset);
  InterruptState = SaveAndDisableInterrupts ();
  if ((FspHeader->ImageAttribute & IMAGE_ATTRIBUTE_64BIT_MODE_SUPPORT) == FSP_IA32) {
    Status = Execute32BitCode ((UINTN)TempRamExitApi, (UINTN)TempRamExitParam, (UINTN)NULL);
  } else {
    Status = Execute64BitCode ((UINTN)TempRamExitApi, (UINTN)TempRamExitParam, (UINTN)NULL);
  }

  SetInterruptState (InterruptState);

  return Status;
}

/**
  Call FSP API - FspSiliconInit.

  @param[in] FspsUpdDataPtr Address pointer to the Silicon Init parameters structure.

  @return EFI status returned by FspSiliconInit API.
**/
EFI_STATUS
EFIAPI
CallFspSiliconInit (
  IN VOID  *FspsUpdDataPtr
  )
{
  FSP_INFO_HEADER   *FspHeader;
  FSP_SILICON_INIT  FspSiliconInitApi;
  EFI_STATUS        Status;
  BOOLEAN           InterruptState;

  FspHeader = (FSP_INFO_HEADER *)FspFindFspHeader (PcdGet32 (PcdFspsBaseAddress));
  if (FspHeader == NULL) {
    return EFI_DEVICE_ERROR;
  }

  FspSiliconInitApi = (FSP_SILICON_INIT)((UINTN)FspHeader->ImageBase + FspHeader->FspSiliconInitEntryOffset);
  InterruptState    = SaveAndDisableInterrupts ();
  if ((FspHeader->ImageAttribute & IMAGE_ATTRIBUTE_64BIT_MODE_SUPPORT) == FSP_IA32) {
    Status = Execute32BitCode ((UINTN)FspSiliconInitApi, (UINTN)FspsUpdDataPtr, (UINTN)NULL);
  } else {
    Status = Execute64BitCode ((UINTN)FspSiliconInitApi, (UINTN)FspsUpdDataPtr, (UINTN)NULL);
  }

  SetInterruptState (InterruptState);

  return Status;
}
