/** @file
  BMC Event Log functions.

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/IpmiCommandLib.h>

#include <Library/ManageabilityTransportHelperLib.h>

EFI_STATUS
EFIAPI
CheckIfSelIsFull (
  VOID
  );

/**
  This function erases event logs and waits until complete.

  @param [in]  ResvId              - Reserved ID

  @retval  EFI_STATUS               EFI_SUCCESS
                                    EFI_NO_RESPONSE

**/
EFI_STATUS
WaitTillErased (
  IN  UINT8  *ResvId
  )
{
  INTN                     Counter;
  IPMI_CLEAR_SEL_REQUEST   ClearSel;
  IPMI_CLEAR_SEL_RESPONSE  ClearSelResponse;

  Counter = 0x200;
  ZeroMem (&ClearSelResponse, sizeof (ClearSelResponse));

  while (TRUE) {
    ZeroMem (&ClearSel, sizeof (ClearSel));
    ClearSel.Reserve[0] = ResvId[0];
    ClearSel.Reserve[1] = ResvId[1];
    ClearSel.AscC       = 0x43;
    ClearSel.AscL       = 0x4C;
    ClearSel.AscR       = 0x52;
    ClearSel.Erase      = 0x00;

    IpmiClearSel (
      &ClearSel,
      &ClearSelResponse
      );

    if ((ClearSelResponse.ErasureProgress & 0xf) == 1) {
      return EFI_SUCCESS;
    }

    //
    //  If there is not a response from the BMC controller we need to return and not hang.
    //
    --Counter;
    if (Counter == 0x0) {
      return EFI_NO_RESPONSE;
    }
  }
}

/**
  This function activates BMC event log.

  @param [in] EnableElog  Enable/Disable event log
  @param [out] ElogStatus  return log status

  @retval  EFI_STATUS

**/
EFI_STATUS
EfiActivateBmcElog (
  IN BOOLEAN   *EnableElog,
  OUT BOOLEAN  *ElogStatus
  )
{
  EFI_STATUS                            Status;
  UINT8                                 ElogStat;
  IPMI_SET_BMC_GLOBAL_ENABLES_REQUEST   SetBmcGlobalEnables;
  IPMI_GET_BMC_GLOBAL_ENABLES_RESPONSE  GetBmcGlobalEnables;
  UINT8                                 CompletionCode;

  Status   = EFI_SUCCESS;
  ElogStat = 0;

  Status = IpmiGetBmcGlobalEnables (&GetBmcGlobalEnables);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (EnableElog == NULL) {
    *ElogStatus = GetBmcGlobalEnables.GetEnables.Bits.SystemEventLogging;
  } else {
    if (Status == EFI_SUCCESS) {
      if (*EnableElog) {
        ElogStat = 1;
      }

      CopyMem (&SetBmcGlobalEnables, (UINT8 *)&GetBmcGlobalEnables + 1, sizeof (UINT8));
      SetBmcGlobalEnables.SetEnables.Bits.SystemEventLogging = ElogStat;

      Status = IpmiSetBmcGlobalEnables (&SetBmcGlobalEnables, &CompletionCode);
    }
  }

  return Status;
}

/**

  @retval  EFI_STATUS

**/
EFI_STATUS
SetElogRedirInstall (
  VOID
  )
{
  BOOLEAN  EnableElog;
  BOOLEAN  ElogStatus;

  //
  // Activate the Event Log (This should depend upon Setup).
  //
  EfiActivateBmcElog (&EnableElog, &ElogStatus);
  return EFI_SUCCESS;
}

/**
  Entry point of BmcElog DXE driver

  @param [in]  ImageHandle  ImageHandle of the loaded driver
  @param [in]  SystemTable  Pointer to the System Table

  @retval  EFI_STATUS

**/
EFI_STATUS
EFIAPI
InitializeBmcElogLayer (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SetElogRedirInstall ();

  CheckIfSelIsFull ();

  return EFI_SUCCESS;
}

/**
  This function verifies the BMC SEL is full and When it is
  reports the error to the Error Manager.

  @retval  EFI_STATUS

**/
EFI_STATUS
EFIAPI
CheckIfSelIsFull (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINT8                       SelIsFull;
  IPMI_GET_SEL_INFO_RESPONSE  SelInfo;

  Status = IpmiGetSelInfo (&SelInfo);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check the Bit7 of the OperationByte if SEL is OverFlow.
  //
  SelIsFull = (SelInfo.OperationSupport & 0x80);
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "SelIsFull - 0x%x\n", SelIsFull));

  return EFI_SUCCESS;
}
