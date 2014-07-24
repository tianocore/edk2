/** @file
  Provide FSP API related function.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Guid/FspHeaderFile.h>

#include <Library/FspApiLib.h>
#include <Library/BaseMemoryLib.h>

/**
  Wrapper for a thunk  to transition from long mode to compatibility mode to execute 32-bit code and then transit back to
  long mode.

  @param[in] Function     The 32bit code entry to be executed.
  @param[in] Param1       The first parameter to pass to 32bit code.

  @return FSP_STATUS.
**/
FSP_STATUS
Execute32BitCode (
  IN UINT64      Function,
  IN UINT64      Param1
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
  UINT8 *CheckPointer;

  CheckPointer = (UINT8 *) (UINTN) FlashFvFspBase;

  if (((EFI_FIRMWARE_VOLUME_HEADER *)CheckPointer)->Signature != EFI_FVH_SIGNATURE) {
    return NULL;
  }

  if (((EFI_FIRMWARE_VOLUME_HEADER *)CheckPointer)->ExtHeaderOffset != 0) {
    CheckPointer = CheckPointer + ((EFI_FIRMWARE_VOLUME_HEADER *)CheckPointer)->ExtHeaderOffset;
    CheckPointer = CheckPointer + ((EFI_FIRMWARE_VOLUME_EXT_HEADER *)CheckPointer)->ExtHeaderSize;
    CheckPointer = (UINT8 *) ALIGN_POINTER (CheckPointer, 8);
  } else {
    CheckPointer = CheckPointer + ((EFI_FIRMWARE_VOLUME_HEADER *)CheckPointer)->HeaderLength;
  }

  if (!CompareGuid (&((EFI_FFS_FILE_HEADER *)CheckPointer)->Name, &gFspHeaderFileGuid)) {
    return NULL;
  }

  CheckPointer = CheckPointer + sizeof (EFI_FFS_FILE_HEADER);

  if (((EFI_RAW_SECTION *)CheckPointer)->Type != EFI_SECTION_RAW) {
    return NULL;
  }

  CheckPointer = CheckPointer + sizeof (EFI_RAW_SECTION);

  return (FSP_INFO_HEADER *)CheckPointer;
}

/**
  Call FSP API - FspInit.

  @param[in] FspHeader     FSP header pointer.
  @param[in] FspInitParams Address pointer to the FSP_INIT_PARAMS structure.

  @return FSP status returned by FspInit API.
**/
FSP_STATUS
EFIAPI
CallFspInit (
  IN FSP_INFO_HEADER     *FspHeader,
  IN FSP_INIT_PARAMS     *FspInitParams
  )
{
  FSP_FSP_INIT        FspInitApi;
  FSP_STATUS          FspStatus;

  FspInitApi = (FSP_FSP_INIT)(UINTN)(FspHeader->ImageBase + FspHeader->FspInitEntryOffset);
  FspStatus = Execute32BitCode ((UINTN)FspInitApi, (UINTN)FspInitParams);

  return FspStatus;
}

/**
  Call FSP API - FspNotifyPhase.

  @param[in] FspHeader         FSP header pointer.
  @param[in] NotifyPhaseParams Address pointer to the NOTIFY_PHASE_PARAMS structure.

  @return FSP status returned by FspNotifyPhase API.
**/
FSP_STATUS
EFIAPI
CallFspNotifyPhase (
  IN FSP_INFO_HEADER     *FspHeader,
  IN NOTIFY_PHASE_PARAMS *NotifyPhaseParams
  )
{
  FSP_NOTFY_PHASE     NotifyPhaseApi;
  FSP_STATUS          FspStatus;

  NotifyPhaseApi = (FSP_NOTFY_PHASE)(UINTN)(FspHeader->ImageBase + FspHeader->NotifyPhaseEntryOffset);
  FspStatus = Execute32BitCode ((UINTN)NotifyPhaseApi, (UINTN)NotifyPhaseParams);

  return FspStatus;
}
