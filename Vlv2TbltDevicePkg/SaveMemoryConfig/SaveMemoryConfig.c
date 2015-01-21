/** 
  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  SaveMemoryConfig.c

Abstract:
  This is the driver that locates the MemoryConfigurationData HOB, if it
  exists, and saves the data to nvRAM.

 

--*/

#include "SaveMemoryConfig.h"

CHAR16    EfiMemoryConfigVariable[] = L"MemoryConfig";


EFI_STATUS
EFIAPI
SaveMemoryConfigEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

  Routine Description:
    This is the standard EFI driver point that detects whether there is a
    MemoryConfigurationData HOB and, if so, saves its data to nvRAM.

  Arguments:
    ImageHandle   - Handle for the image of this driver
    SystemTable   - Pointer to the EFI System Table

  Returns:
    EFI_SUCCESS   - if the data is successfully saved or there was no data
    EFI_NOT_FOUND - if the HOB list could not be located.
    EFI_UNLOAD_IMAGE - It is not success

--*/
{
  EFI_STATUS                      Status=EFI_SUCCESS;
  VOID                            *MemHobData;
  VOID                            *VariableData;
  UINTN                           BufferSize;
  BOOLEAN                         MfgMode;
  EFI_PLATFORM_SETUP_ID           *BootModeBuffer;
  EFI_PLATFORM_INFO_HOB           *PlatformInfoHobPtr;
  MEM_INFO_PROTOCOL               *MemInfoProtocol;
  EFI_HANDLE                      Handle;
  UINT8							              Channel, Slot;
  VOID                            *GuidHob;

  VariableData   = NULL;
  MfgMode        = FALSE;
  Handle         = NULL;
  BootModeBuffer = NULL;
  MemHobData     = NULL;
  PlatformInfoHobPtr = NULL;
  BufferSize     = 0;

  //
  // Get Platform Info HOB
  //
  GuidHob = GetFirstGuidHob (&gEfiPlatformInfoGuid);
  if (GuidHob == NULL) {
    Status = EFI_NOT_FOUND;
  }
  ASSERT_EFI_ERROR (Status);

  PlatformInfoHobPtr = GET_GUID_HOB_DATA (GuidHob);

  //
  // Get the BootMode guid hob
  //
  GuidHob = GetFirstGuidHob (&gEfiPlatformBootModeGuid);
  if (GuidHob == NULL) {
    Status = EFI_NOT_FOUND;
  }
  ASSERT_EFI_ERROR (Status);

  BootModeBuffer = GET_GUID_HOB_DATA (GuidHob);


  //
  // Check whether in Manufacturing Mode
  //
  if (BootModeBuffer) {
    if ( !CompareMem (   //EfiCompareMem
            &BootModeBuffer->SetupName,
            MANUFACTURE_SETUP_NAME,
            StrSize (MANUFACTURE_SETUP_NAME)  //EfiStrSize
            ) ) {
      MfgMode = TRUE;
    }
  }

  if (MfgMode) {
    //
    // Don't save Memory Configuration in Manufacturing Mode. Clear memory configuration.
    //
    Status = gRT->SetVariable (
              EfiMemoryConfigVariable,
              &gEfiVlv2VariableGuid,
              EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
              0,
              NULL
              );      
  } else {

    MemInfoProtocol = (MEM_INFO_PROTOCOL*)AllocateZeroPool(sizeof(MEM_INFO_PROTOCOL));
    if (PlatformInfoHobPtr != NULL) {
      MemInfoProtocol->MemInfoData.memSize  = 0;
      for (Channel = 0; Channel < CH_NUM; Channel ++){
        for (Slot = 0; Slot < DIMM_NUM; Slot ++){               
          MemInfoProtocol->MemInfoData.dimmSize[Slot + (Channel * DIMM_NUM)] = PlatformInfoHobPtr->MemData.DimmSize[Slot];
        }
      }
  	  MemInfoProtocol->MemInfoData.memSize       = PlatformInfoHobPtr->MemData.MemSize;        
  	  MemInfoProtocol->MemInfoData.EccSupport    = PlatformInfoHobPtr->MemData.EccSupport;
      MemInfoProtocol->MemInfoData.ddrFreq       = PlatformInfoHobPtr->MemData.DdrFreq;
      MemInfoProtocol->MemInfoData.ddrType       = PlatformInfoHobPtr->MemData.DdrType;
      if (MemInfoProtocol->MemInfoData.memSize == 0){
        //
        // We hardcode if MRC didn't fill these info in
        //
        MemInfoProtocol->MemInfoData.memSize     = 0x800; //per 1MB 
        MemInfoProtocol->MemInfoData.dimmSize[0] = 0x800;
        MemInfoProtocol->MemInfoData.dimmSize[1] = 0;    
        MemInfoProtocol->MemInfoData.EccSupport  = FALSE;
        MemInfoProtocol->MemInfoData.ddrType     = 5; //DDRType_LPDDR3
      }

      Status = gBS->InstallMultipleProtocolInterfaces (
             &Handle,
             &gMemInfoProtocolGuid,
             MemInfoProtocol,
             NULL
             );
    }

    Status = EFI_SUCCESS;
    if (BOOT_WITH_MINIMAL_CONFIGURATION != GetBootModeHob()){
      //
      // Get the Memory Config guid hob
      //
      GuidHob = GetFirstGuidHob (&gEfiMemoryConfigDataGuid);
      if (GuidHob == NULL) {
        Status = EFI_NOT_FOUND;
      }
      ASSERT_EFI_ERROR (Status);
      
      MemHobData = GET_GUID_HOB_DATA (GuidHob);
      BufferSize = GET_GUID_HOB_DATA_SIZE (GuidHob);

      Status = gRT->GetVariable (
                  EfiMemoryConfigVariable,
                  &gEfiVlv2VariableGuid,
                  NULL,
                  &BufferSize,
                  VariableData
                  );
      if (EFI_ERROR(Status) && (MemHobData != NULL)) {    
        Status = gRT->SetVariable (
                      EfiMemoryConfigVariable,
                      &gEfiVlv2VariableGuid,
                      (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                      BufferSize,
                      MemHobData
                      );
      } 
    }

  } // if-else MfgMode

  return EFI_SUCCESS;
}
