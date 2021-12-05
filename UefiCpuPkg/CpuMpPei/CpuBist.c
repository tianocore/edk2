/** @file
  Update and publish processors' BIST information.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuMpPei.h"

EFI_SEC_PLATFORM_INFORMATION2_PPI  mSecPlatformInformation2Ppi = {
  SecPlatformInformation2
};

EFI_PEI_PPI_DESCRIPTOR  mPeiSecPlatformInformation2Ppi = {
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
  IN CONST EFI_PEI_SERVICES                 **PeiServices,
  IN OUT UINT64                             *StructureSize,
  OUT EFI_SEC_PLATFORM_INFORMATION_RECORD2  *PlatformInformationRecord2
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  VOID               *DataInHob;
  UINTN              DataSize;

  GuidHob = GetFirstGuidHob (&gEfiSecPlatformInformation2PpiGuid);
  if (GuidHob == NULL) {
    *StructureSize = 0;
    return EFI_SUCCESS;
  }

  DataInHob = GET_GUID_HOB_DATA (GuidHob);
  DataSize  = GET_GUID_HOB_DATA_SIZE (GuidHob);

  //
  // return the information from BistHob
  //
  if ((*StructureSize) < (UINT64)DataSize) {
    *StructureSize = (UINT64)DataSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  *StructureSize = (UINT64)DataSize;
  CopyMem (PlatformInformationRecord2, DataInHob, DataSize);
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
  @param  BistInformationSize Return the size in bytes of BIST information

  @retval EFI_SUCCESS         Retrieve of the BIST data successfully
  @retval EFI_NOT_FOUND       No sec platform information(2) ppi export
  @retval EFI_DEVICE_ERROR    Failed to get CPU Information

**/
EFI_STATUS
GetBistInfoFromPpi (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN CONST EFI_GUID           *Guid,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiDescriptor,
  OUT VOID                    **BistInformationData,
  OUT UINT64                  *BistInformationSize OPTIONAL
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
    Status                  = SecPlatformInformation2Ppi->PlatformInformation2 (
                                                            PeiServices,
                                                            &InformationSize,
                                                            SecPlatformInformation2
                                                            );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Status = PeiServicesAllocatePool (
                 (UINTN)InformationSize,
                 (VOID **)&SecPlatformInformation2
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
          if (BistInformationSize != NULL) {
            *BistInformationSize = InformationSize;
          }

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

**/
VOID
CollectBistDataFromPpi (
  IN CONST EFI_PEI_SERVICES  **PeiServices
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
  EFI_PROCESSOR_INFORMATION             ProcessorInfo;
  EFI_HEALTH_FLAGS                      BistData;
  UINTN                                 NumberOfProcessors;
  UINTN                                 NumberOfEnabledProcessors;
  UINTN                                 BistInformationSize;
  EFI_SEC_PLATFORM_INFORMATION_RECORD2  *PlatformInformationRecord2;
  EFI_SEC_PLATFORM_INFORMATION_CPU      *CpuInstanceInHob;

  MpInitLibGetNumberOfProcessors (&NumberOfProcessors, &NumberOfEnabledProcessors);

  BistInformationSize = sizeof (EFI_SEC_PLATFORM_INFORMATION_RECORD2) +
                        sizeof (EFI_SEC_PLATFORM_INFORMATION_CPU) * NumberOfProcessors;
  Status = PeiServicesAllocatePool (
             (UINTN)BistInformationSize,
             (VOID **)&PlatformInformationRecord2
             );
  ASSERT_EFI_ERROR (Status);
  PlatformInformationRecord2->NumberOfCpus = (UINT32)NumberOfProcessors;

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
             (VOID *)&SecPlatformInformation2,
             NULL
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
               (VOID *)&SecPlatformInformation,
               NULL
               );
    if (Status == EFI_SUCCESS) {
      NumberOfData = 1;
      //
      // SEC Platform Information only includes BSP's BIST information
      // and does not have BSP's APIC ID
      //
      BspCpuInstance.CpuLocation                       = GetInitialApicId ();
      BspCpuInstance.InfoRecord.IA32HealthFlags.Uint32 = SecPlatformInformation->IA32HealthFlags.Uint32;
      CpuInstance                                      = &BspCpuInstance;
    } else {
      DEBUG ((DEBUG_INFO, "Does not find any stored CPU BIST information from PPI!\n"));
    }
  }

  for (ProcessorNumber = 0; ProcessorNumber < NumberOfProcessors; ProcessorNumber++) {
    MpInitLibGetProcessorInfo (ProcessorNumber, &ProcessorInfo, &BistData);
    for (CpuIndex = 0; CpuIndex < NumberOfData; CpuIndex++) {
      ASSERT (CpuInstance != NULL);
      if (ProcessorInfo.ProcessorId == CpuInstance[CpuIndex].CpuLocation) {
        //
        // Update processor's BIST data if it is already stored before
        //
        BistData = CpuInstance[CpuIndex].InfoRecord.IA32HealthFlags;
      }
    }

    if (BistData.Uint32 != 0) {
      //
      // Report Status Code that self test is failed
      //
      REPORT_STATUS_CODE (
        EFI_ERROR_CODE | EFI_ERROR_MAJOR,
        (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_SELF_TEST)
        );
    }

    DEBUG ((
      DEBUG_INFO,
      "  APICID - 0x%08x, BIST - 0x%08x\n",
      (UINT32)ProcessorInfo.ProcessorId,
      BistData
      ));
    CpuInstanceInHob                                             = PlatformInformationRecord2->CpuInstance;
    CpuInstanceInHob[ProcessorNumber].CpuLocation                = (UINT32)ProcessorInfo.ProcessorId;
    CpuInstanceInHob[ProcessorNumber].InfoRecord.IA32HealthFlags = BistData;
  }

  //
  // Build SecPlatformInformation2 PPI GUIDed HOB that also could be consumed
  // by CPU MP driver to get CPU BIST data
  //
  BuildGuidDataHob (
    &gEfiSecPlatformInformation2PpiGuid,
    PlatformInformationRecord2,
    (UINTN)BistInformationSize
    );

  if (SecPlatformInformation2 != NULL) {
    if (NumberOfData < NumberOfProcessors) {
      //
      // Reinstall SecPlatformInformation2 PPI to include new BIST information
      //
      Status = PeiServicesReInstallPpi (
                 SecInformationDescriptor,
                 &mPeiSecPlatformInformation2Ppi
                 );
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    //
    // Install SecPlatformInformation2 PPI
    //
    Status = PeiServicesInstallPpi (&mPeiSecPlatformInformation2Ppi);
    ASSERT_EFI_ERROR (Status);
  }
}
