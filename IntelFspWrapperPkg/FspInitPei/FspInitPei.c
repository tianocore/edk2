/** @file
  This PEIM will be invoked twice by pei core. In 1st entry, it will call FspInit API.
  In 2nd entry, it will parse the hoblist from fsp and report them into pei core.
  This file contains the main entrypoint of the PEIM.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "FspInitPei.h"

/**
  FSP Init continuation function.
  Control will be returned to this callback function after FspInit API call.

  @param[in] Status      Status of the FSP INIT API
  @param[in] HobListPtr  Pointer to the HOB data structure defined in the PI specification.

**/
VOID
ContinuationFunc (
  IN FSP_STATUS Status,
  IN VOID       *HobListPtr
  )
{
  EFI_BOOT_MODE             BootMode;
  UINT64                    StackSize;
  EFI_PHYSICAL_ADDRESS      StackBase;

  DEBUG ((DEBUG_INFO, "ContinuationFunc - %r\n", Status));
  DEBUG ((DEBUG_INFO, "HobListPtr - 0x%x\n", HobListPtr));

  if (Status != FSP_SUCCESS) {
    CpuDeadLoop ();
  }

  //
  // Can not call any PeiServices
  //
  BootMode = GetBootMode ();

  GetStackInfo (BootMode, TRUE, &StackBase, &StackSize);
  DEBUG ((DEBUG_INFO, "StackBase - 0x%x\n", StackBase));
  DEBUG ((DEBUG_INFO, "StackSize - 0x%x\n", StackSize));
  CallPeiCoreEntryPoint (
    HobListPtr,
    (VOID *)(UINTN)StackBase,
    (VOID *)(UINTN)(StackBase + StackSize)
    );
}

/**
  Call FspInit API.

  @param[in] FspHeader FSP header pointer.
**/
VOID
SecFspInit (
  IN FSP_INFO_HEADER *FspHeader
  )
{
  FSP_INIT_PARAMS           FspInitParams;
  FSP_INIT_RT_COMMON_BUFFER FspRtBuffer;
  UINT8                     FspUpdRgn[FixedPcdGet32 (PcdMaxUpdRegionSize)];
  UINT32                    UpdRegionSize;
  EFI_BOOT_MODE             BootMode;
  UINT64                    StackSize;
  EFI_PHYSICAL_ADDRESS      StackBase;
  FSP_STATUS                FspStatus;

  DEBUG ((DEBUG_INFO, "SecFspInit enter\n"));

  PeiServicesGetBootMode (&BootMode);
  DEBUG ((DEBUG_INFO, "BootMode - 0x%x\n", BootMode));

  GetStackInfo (BootMode, FALSE, &StackBase, &StackSize);
  DEBUG ((DEBUG_INFO, "StackBase - 0x%x\n", StackBase));
  DEBUG ((DEBUG_INFO, "StackSize - 0x%x\n", StackSize));

  ZeroMem (&FspRtBuffer, sizeof(FspRtBuffer));
  FspRtBuffer.StackTop = (UINT32 *)(UINTN)(StackBase + StackSize);

  FspRtBuffer.BootMode = BootMode;

  /* Platform override any UPD configs */
  UpdRegionSize = GetUpdRegionSize();
  DEBUG ((DEBUG_INFO, "UpdRegionSize - 0x%x\n", UpdRegionSize));
  DEBUG ((DEBUG_INFO, "sizeof(FspUpdRgn) - 0x%x\n", sizeof(FspUpdRgn)));
  ASSERT(sizeof(FspUpdRgn) >= UpdRegionSize);
  ZeroMem (FspUpdRgn, UpdRegionSize);
  FspRtBuffer.UpdDataRgnPtr = UpdateFspUpdConfigs (FspUpdRgn);

  ZeroMem (&FspInitParams, sizeof(FspInitParams));
  FspInitParams.NvsBufferPtr = GetNvsBuffer ();
  DEBUG ((DEBUG_INFO, "NvsBufferPtr - 0x%x\n", FspInitParams.NvsBufferPtr));
  FspInitParams.RtBufferPtr  = (VOID *)&FspRtBuffer;
  FspInitParams.ContinuationFunc = (CONTINUATION_PROC)ContinuationFunc;

  SaveSecContext (GetPeiServicesTablePointer ());

  DEBUG ((DEBUG_INFO, "FspInitParams      - 0x%x\n", &FspInitParams));
  DEBUG ((DEBUG_INFO, "  NvsBufferPtr     - 0x%x\n", FspInitParams.NvsBufferPtr));
  DEBUG ((DEBUG_INFO, "  RtBufferPtr      - 0x%x\n", FspInitParams.RtBufferPtr));
  DEBUG ((DEBUG_INFO, "    StackTop       - 0x%x\n", FspRtBuffer.StackTop));
  DEBUG ((DEBUG_INFO, "    BootMode       - 0x%x\n", FspRtBuffer.BootMode));
  DEBUG ((DEBUG_INFO, "    UpdDataRgnPtr  - 0x%x\n", FspRtBuffer.UpdDataRgnPtr));
  DEBUG ((DEBUG_INFO, "  ContinuationFunc - 0x%x\n", FspInitParams.ContinuationFunc));

  FspStatus = CallFspInit (FspHeader, &FspInitParams);
  //
  // Should never return
  //
  DEBUG((DEBUG_ERROR, "FSP Init failed, status: 0x%x\n", FspStatus));
  CpuDeadLoop ();
}

/**
  This is the entrypoint of PEIM

  @param[in] FileHandle  Handle of the file being invoked.
  @param[in] PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
FspPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  FSP_INFO_HEADER      *FspHeader;
  EFI_STATUS           Status;
  FSP_INIT_DONE_PPI    *FspInitDone;
  VOID                 *FspHobList;
  EFI_BOOT_MODE        BootMode;

  DEBUG ((DEBUG_INFO, "FspPeiEntryPoint\n"));

  Status = PeiServicesLocatePpi (
             &gFspInitDonePpiGuid,
             0,
             NULL,
             (VOID **) &FspInitDone
             );
  if (EFI_ERROR (Status)) {
    //
    // 1st entry
    //
    DEBUG ((DEBUG_INFO, "1st entry\n"));
    FspHeader = FspFindFspHeader (PcdGet32 (PcdFlashFvFspBase));
    DEBUG ((DEBUG_INFO, "FspHeader - 0x%x\n", FspHeader));
    if (FspHeader == NULL) {
      return EFI_DEVICE_ERROR;
    }

    SecFspInit (FspHeader);

    //
    // Never return here
    //
    CpuDeadLoop ();
  } else {
    //
    // 2nd entry
    //
    DEBUG ((DEBUG_INFO, "2nd entry\n"));
    Status = FspInitDone->GetFspHobList (PeiServices, FspInitDone, &FspHobList);
    DEBUG ((DEBUG_INFO, "FspHobList - 0x%x\n", FspHobList));
    FspHobProcess (FspHobList);

    PeiServicesGetBootMode (&BootMode);
    if (BootMode == BOOT_ON_S3_RESUME) {
      Status = PeiServicesNotifyPpi (&mS3EndOfPeiNotifyDesc);
      ASSERT_EFI_ERROR (Status);
    }
  }

  return EFI_SUCCESS;
}
