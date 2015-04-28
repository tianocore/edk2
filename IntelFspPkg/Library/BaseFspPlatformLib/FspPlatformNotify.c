/** @file

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/FspSwitchStackLib.h>
#include <Library/FspCommonLib.h>
#include <Guid/EventGroup.h>
#include <FspApi.h>
#include <Protocol/PciEnumerationComplete.h>

EFI_PEI_PPI_DESCRIPTOR      mPeiPostPciEnumerationPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPciEnumerationCompleteProtocolGuid,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR      mPeiReadyToBootPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEventReadyToBootGuid,
  NULL
};


UINT32  mFspNotifySequence[] = {
  EnumInitPhaseAfterPciEnumeration,
  EnumInitPhaseReadyToBoot
};

/**
  Install FSP notification.

  @param[in] NotificationCode  FSP notification code

  @retval EFI_SUCCESS            Notify FSP successfully
  @retval EFI_INVALID_PARAMETER  NotificationCode is invalid

**/
EFI_STATUS
EFIAPI
FspNotificationHandler (
  IN  UINT32     NotificationCode
  )
{
  EFI_STATUS                Status;

  Status   = EFI_SUCCESS;

  switch (NotificationCode) {
  case EnumInitPhaseAfterPciEnumeration:
    //
    // Do POST PCI initialization if needed
    //
    DEBUG ((DEBUG_INFO | DEBUG_INIT, "FSP Post PCI Enumeration ...\n"));
    PeiServicesInstallPpi (&mPeiPostPciEnumerationPpi);
    break;

  case EnumInitPhaseReadyToBoot:
    //
    // Ready To Boot
    //
    DEBUG ((DEBUG_INFO| DEBUG_INIT, "FSP Ready To Boot ...\n"));
    PeiServicesInstallPpi (&mPeiReadyToBootPpi);
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}

/**
  This function transfer control to the ContinuationFunc passed in by the
  BootLoader.

**/
VOID
EFIAPI
FspInitDone (
  VOID
  )
{
  FSP_INIT_PARAMS        *FspInitParams;

  if (GetFspApiCallingMode() == 0) {
    //
    // FspInit API is used, so jump into the ContinuationFunc
    //
    FspInitParams   = (FSP_INIT_PARAMS *)GetFspApiParameter ();
  
    //
    // Modify the parameters for ContinuationFunc
    //
    SetFspContinuationFuncParameter(EFI_SUCCESS, 0);
    SetFspContinuationFuncParameter((UINT32)GetHobList(), 1);
  
    //
    // Modify the return address to ContinuationFunc
    //
    SetFspApiReturnAddress((UINT32)FspInitParams->ContinuationFunc);
  
    //
    // Give control back to the boot loader framework caller after FspInit is done
    // It is done throught the continuation function
    //
    SetFspMeasurePoint (FSP_PERF_ID_API_FSPINIT_EXIT);
  } else {
    //
    // FspMemoryInit API is used, so return directly
    //

    //
    // This is the end of the FspSiliconInit API
    // Give control back to the boot loader
    //
    DEBUG ((DEBUG_INFO | DEBUG_INIT, "FspSiliconInitApi() - End\n"));
    SetFspApiReturnStatus (EFI_SUCCESS);
  }

  Pei2LoaderSwitchStack();
}

/**
  This function handle NotifyPhase API call from the BootLoader.
  It gives control back to the BootLoader after it is handled. If the
  Notification code is a ReadyToBoot event, this function will return
  and FSP continues the remaining execution until it reaches the DxeIpl.

**/
VOID
FspWaitForNotify (
  VOID
  )
{
  EFI_STATUS                 Status;
  UINT32                     NotificationValue;
  UINT32                     NotificationCount;
  UINT8                      Count;

  NotificationCount = 0;
  while (NotificationCount < sizeof(mFspNotifySequence) / sizeof(UINT32)) {

    Count = (UINT8)((NotificationCount << 1) & 0x07);
    SetFspMeasurePoint (FSP_PERF_ID_API_NOTIFY_POSTPCI_ENTRY + Count);

    NotificationValue = ((NOTIFY_PHASE_PARAMS *)(UINTN)GetFspApiParameter ())->Phase;
    DEBUG ((DEBUG_INFO, "FSP Got Notification. Notification Value : 0x%08X\n", NotificationValue));

    if (mFspNotifySequence[NotificationCount] != NotificationValue) {
      //
      // Notify code does not follow the predefined order
      //
      DEBUG ((DEBUG_INFO, "Unsupported FSP Notification Value\n"));
      SetFspApiReturnStatus(EFI_UNSUPPORTED);
    } else {
      //
      // Process Notification and Give control back to the boot loader framework caller
      //
      Status = FspNotificationHandler (NotificationValue);
      DEBUG ((DEBUG_INFO, "FSP Notification Handler Returns : 0x%08X\n", Status));
      SetFspApiReturnStatus(Status);
      if (!EFI_ERROR(Status)) {
        NotificationCount++;
        SetFspApiReturnStatus(EFI_SUCCESS);
        if (NotificationValue == EnumInitPhaseReadyToBoot) {
          break;
        }
      }
    }
    SetFspMeasurePoint (FSP_PERF_ID_API_NOTIFY_POSTPCI_EXIT + Count);
    Pei2LoaderSwitchStack();
  }

  //
  // Control goes back to the PEI Core and it dispatches further PEIMs.
  // DXEIPL is the final one to transfer control back to the boot loader.
  //
}

