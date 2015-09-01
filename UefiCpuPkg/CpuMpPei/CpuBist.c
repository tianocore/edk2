/** @file
  Update and publish processors' BIST information.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuMpPei.h"

EFI_SEC_PLATFORM_INFORMATION2_PPI mSecPlatformInformation2Ppi = {
  SecPlatformInformation2
};

EFI_PEI_PPI_DESCRIPTOR mPeiSecPlatformInformation2Ppi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiSecPlatformInformation2PpiGuid,
  &mSecPlatformInformation2Ppi
};

/**
  Implementation of the PlatformInformation2 service in EFI_SEC_PLATFORM_INFORMATION2_PPI.

  @param  PeiServices                The pointer to the PEI Services Table.
  @param  StructureSize              The pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord2 The pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD2.

  @retval EFI_SUCCESS                The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL       The buffer was too small. The current buffer size needed to
                                     hold the record is returned in StructureSize.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation2 (
  IN CONST EFI_PEI_SERVICES                   **PeiServices,
  IN OUT UINT64                               *StructureSize,
     OUT EFI_SEC_PLATFORM_INFORMATION_RECORD2 *PlatformInformationRecord2
  )
{
  PEI_CPU_MP_DATA                      *PeiCpuMpData;
  UINTN                                BistInformationSize;
  UINTN                                CpuIndex;
  EFI_SEC_PLATFORM_INFORMATION_CPU     *CpuInstance;

  PeiCpuMpData = GetMpHobData ();

  BistInformationSize = sizeof (EFI_SEC_PLATFORM_INFORMATION_RECORD2) +
                        sizeof (EFI_SEC_PLATFORM_INFORMATION_CPU) * PeiCpuMpData->CpuCount;
  //
  // return the information size if input buffer size is too small
  //
  if ((*StructureSize) < (UINT64) BistInformationSize) {
    *StructureSize = (UINT64) BistInformationSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  PlatformInformationRecord2->NumberOfCpus = PeiCpuMpData->CpuCount;
  CpuInstance = PlatformInformationRecord2->CpuInstance;
  for (CpuIndex = 0; CpuIndex < PeiCpuMpData->CpuCount; CpuIndex ++) {
    CpuInstance[CpuIndex].CpuLocation                = PeiCpuMpData->CpuData[CpuIndex].ApicId;
    CpuInstance[CpuIndex].InfoRecord.IA32HealthFlags = PeiCpuMpData->CpuData[CpuIndex].Health;
  }

  return EFI_SUCCESS;
}

/**
  Worker function to get CPUs' BIST by calling SecPlatformInformationPpi
  or SecPlatformInformation2Ppi.

  @param  PeiServices         Pointer to PEI Services Table
  @param  Guid                PPI Guid
  @param  PpiDescriptor       Return a pointer to instance of the
                              EFI_PEI_PPI_DESCRIPTOR
  @param  BistInformationData Pointer to BIST information data

  @retval EFI_SUCCESS         Retrieve of the BIST data successfully
  @retval EFI_NOT_FOUND       No sec platform information(2) ppi export
  @retval EFI_DEVICE_ERROR    Failed to get CPU Information

**/
EFI_STATUS
GetBistInfoFromPpi (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN CONST EFI_GUID             *Guid,
     OUT EFI_PEI_PPI_DESCRIPTOR **PpiDescriptor,
     OUT VOID                   **BistInformationData
  )
{
  EFI_STATUS                            Status;
  EFI_SEC_PLATFORM_INFORMATION2_PPI     *SecPlatformInformation2Ppi;
  EFI_SEC_PLATFORM_INFORMATION_RECORD2  *SecPlatformInformation2;
  UINT64                                InformationSize;

  Status = PeiServicesLocatePpi (
             Guid,                                // GUID
             0,                                   // INSTANCE
             PpiDescriptor,                       // EFI_PEI_PPI_DESCRIPTOR
             (VOID **)&SecPlatformInformation2Ppi // PPI
             );
  if (Status == EFI_NOT_FOUND) {
    return EFI_NOT_FOUND;
  }

  if (Status == EFI_SUCCESS) {
    //
    // Get the size of the sec platform information2(BSP/APs' BIST data)
    //
    InformationSize         = 0;
    SecPlatformInformation2 = NULL;
    Status = SecPlatformInformation2Ppi->PlatformInformation2 (
                                           PeiServices,
                                           &InformationSize,
                                           SecPlatformInformation2
                                           );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Status = PeiServicesAllocatePool (
                 (UINTN) InformationSize,
                 (VOID **) &SecPlatformInformation2
                 );
      if (Status == EFI_SUCCESS) {
        //
        // Retrieve BIST data
        //
        Status = SecPlatformInformation2Ppi->PlatformInformation2 (
                                               PeiServices,
                                               &InformationSize,
                                               SecPlatformInformation2
                                               );
        if (Status == EFI_SUCCESS) {
          *BistInformationData = SecPlatformInformation2;
          return EFI_SUCCESS;
        }
      }
    }
  }

  return EFI_DEVICE_ERROR;
}

/**
  Collects BIST data from PPI.

  This function collects BIST data from Sec Platform Information2 PPI
  or SEC Platform Information PPI.

  @param PeiServices         Pointer to PEI Services Table
  @param PeiCpuMpData        Pointer to PEI CPU MP Data

**/
VOID
CollectBistDataFromPpi (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN PEI_CPU_MP_DATA                    *PeiCpuMpData
  )
{
  EFI_STATUS                            Status;
  EFI_PEI_PPI_DESCRIPTOR                *SecInformationDescriptor;
  EFI_SEC_PLATFORM_INFORMATION_RECORD2  *SecPlatformInformation2;
  EFI_SEC_PLATFORM_INFORMATION_RECORD   *SecPlatformInformation;
  UINTN                                 NumberOfData;
  EFI_SEC_PLATFORM_INFORMATION_CPU      *CpuInstance;
  EFI_SEC_PLATFORM_INFORMATION_CPU      BspCpuInstance;
  UINTN                                 ProcessorNumber;
  UINTN                                 CpuIndex;
  PEI_CPU_DATA                          *CpuData;

  SecPlatformInformation2 = NULL;
  SecPlatformInformation  = NULL;
  NumberOfData            = 0;
  CpuInstance             = NULL;

  //
  // Get BIST information from Sec Platform Information2 Ppi firstly
  //
  Status = GetBistInfoFromPpi (
             PeiServices,
             &gEfiSecPlatformInformation2PpiGuid,
             &SecInformationDescriptor,
             (VOID *) &SecPlatformInformation2
             );
  if (Status == EFI_SUCCESS) {
    //
    // Sec Platform Information2 PPI includes BSP/APs' BIST information
    //
    NumberOfData = SecPlatformInformation2->NumberOfCpus;
    CpuInstance  = SecPlatformInformation2->CpuInstance;
  } else {
    //
    // Otherwise, get BIST information from Sec Platform Information Ppi
    //
    Status = GetBistInfoFromPpi (
               PeiServices,
               &gEfiSecPlatformInformationPpiGuid,
               &SecInformationDescriptor,
               (VOID *) &SecPlatformInformation
               );
    if (Status == EFI_SUCCESS) {
      NumberOfData = 1;
      //
      // SEC Platform Information only includes BSP's BIST information
      // and does not have BSP's APIC ID
      //
      BspCpuInstance.CpuLocation = GetInitialApicId ();
      BspCpuInstance.InfoRecord.IA32HealthFlags.Uint32  = SecPlatformInformation->IA32HealthFlags.Uint32;
      CpuInstance = &BspCpuInstance;
    } else {
      DEBUG ((EFI_D_INFO, "Does not find any stored CPU BIST information from PPI!\n"));
    }
  }
  for (ProcessorNumber = 0; ProcessorNumber < PeiCpuMpData->CpuCount; ProcessorNumber ++) {
    CpuData = &PeiCpuMpData->CpuData[ProcessorNumber];
    for (CpuIndex = 0; CpuIndex < NumberOfData; CpuIndex ++) {
      ASSERT (CpuInstance != NULL);
      if (CpuData->ApicId == CpuInstance[CpuIndex].CpuLocation) {
        //
        // Update processor's BIST data if it is already stored before
        //
        CpuData->Health = CpuInstance[CpuIndex].InfoRecord.IA32HealthFlags;
      }
    }
    if (CpuData->Health.Uint32 == 0) {
      CpuData->CpuHealthy = TRUE;
    } else {
      CpuData->CpuHealthy = FALSE;
      //
      // Report Status Code that self test is failed
      //
      REPORT_STATUS_CODE (
        EFI_ERROR_CODE | EFI_ERROR_MAJOR,
        (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_SELF_TEST)
        );
    }
    DEBUG ((EFI_D_INFO, "  APICID - 0x%08x, BIST - 0x%08x\n",
            PeiCpuMpData->CpuData[ProcessorNumber].ApicId,
            PeiCpuMpData->CpuData[ProcessorNumber].Health.Uint32
            ));
  }

  if (SecPlatformInformation2 != NULL && NumberOfData < PeiCpuMpData->CpuCount) {
    //
    // Reinstall SecPlatformInformation2 PPI to include new BIST inforamtion
    //
    Status = PeiServicesReInstallPpi (
               SecInformationDescriptor,
               &mPeiSecPlatformInformation2Ppi
               );
    ASSERT_EFI_ERROR (Status);
  } else {
    //
    // Install SecPlatformInformation2 PPI to include new BIST inforamtion
    //
    Status = PeiServicesInstallPpi (&mPeiSecPlatformInformation2Ppi);
    ASSERT_EFI_ERROR(Status);
  }
}
