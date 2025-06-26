/** @file
  Get SEC platform information(2) PPI and reinstall it.

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecMain.h"

/**
  Implementation of the PlatformInformation service in EFI_SEC_PLATFORM_INFORMATION_PPI.

  @param  PeiServices                Pointer to the PEI Services Table.
  @param  StructureSize              Pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord  Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.

  @retval EFI_SUCCESS                The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL       The buffer was too small.

**/
EFI_STATUS
EFIAPI
SecPlatformInformationBist (
  IN CONST EFI_PEI_SERVICES                **PeiServices,
  IN OUT UINT64                            *StructureSize,
  OUT EFI_SEC_PLATFORM_INFORMATION_RECORD  *PlatformInformationRecord
  );

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
SecPlatformInformation2Bist (
  IN CONST EFI_PEI_SERVICES                 **PeiServices,
  IN OUT UINT64                             *StructureSize,
  OUT EFI_SEC_PLATFORM_INFORMATION_RECORD2  *PlatformInformationRecord2
  );

EFI_SEC_PLATFORM_INFORMATION_PPI  mSecPlatformInformation = {
  SecPlatformInformationBist
};

EFI_PEI_PPI_DESCRIPTOR  mPeiSecPlatformInformation = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiSecPlatformInformationPpiGuid,
  &mSecPlatformInformation
};

EFI_SEC_PLATFORM_INFORMATION2_PPI  mSecPlatformInformation2 = {
  SecPlatformInformation2Bist
};

EFI_PEI_PPI_DESCRIPTOR  mPeiSecPlatformInformation2 = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiSecPlatformInformation2PpiGuid,
  &mSecPlatformInformation2
};

/**
  Worker function to parse CPU BIST information from Guided HOB.

  @param[in, out] StructureSize     Pointer to the variable describing size of the input buffer.
  @param[in, out] StructureBuffer   Pointer to the buffer save CPU BIST information.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL  The buffer was too small.

**/
EFI_STATUS
GetBistFromHob (
  IN OUT UINT64  *StructureSize,
  IN OUT VOID    *StructureBuffer
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  VOID               *DataInHob;
  UINTN              DataSize;

  GuidHob = GetFirstGuidHob (&gEfiCallerIdGuid);
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
  CopyMem (StructureBuffer, DataInHob, DataSize);
  return EFI_SUCCESS;
}

/**
  Implementation of the PlatformInformation service in EFI_SEC_PLATFORM_INFORMATION_PPI.

  @param[in]      PeiServices                Pointer to the PEI Services Table.
  @param[in, out] StructureSize              Pointer to the variable describing size of the input buffer.
  @param[out]     PlatformInformationRecord  Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.

  @retval EFI_SUCCESS                    The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL           The buffer was too small.

**/
EFI_STATUS
EFIAPI
SecPlatformInformationBist (
  IN CONST EFI_PEI_SERVICES                **PeiServices,
  IN OUT UINT64                            *StructureSize,
  OUT EFI_SEC_PLATFORM_INFORMATION_RECORD  *PlatformInformationRecord
  )
{
  return GetBistFromHob (StructureSize, PlatformInformationRecord);
}

/**
  Implementation of the PlatformInformation2 service in EFI_SEC_PLATFORM_INFORMATION2_PPI.

  @param[in]      PeiServices                The pointer to the PEI Services Table.
  @param[in, out] StructureSize              The pointer to the variable describing size of the input buffer.
  @param[out]     PlatformInformationRecord2 The pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD2.

  @retval EFI_SUCCESS                    The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL           The buffer was too small. The current buffer size needed to
                                         hold the record is returned in StructureSize.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation2Bist (
  IN CONST EFI_PEI_SERVICES                 **PeiServices,
  IN OUT UINT64                             *StructureSize,
  OUT EFI_SEC_PLATFORM_INFORMATION_RECORD2  *PlatformInformationRecord2
  )
{
  return GetBistFromHob (StructureSize, PlatformInformationRecord2);
}

/**
  Worker function to get CPUs' BIST by calling SecPlatformInformationPpi
  or SecPlatformInformation2Ppi.

  @param[in]  PeiServices         Pointer to PEI Services Table
  @param[in]  Guid                PPI Guid
  @param[out] PpiDescriptor       Return a pointer to instance of the
                                  EFI_PEI_PPI_DESCRIPTOR
  @param[out] BistInformationData Pointer to BIST information data
  @param[out] BistInformationSize Return the size in bytes of BIST information

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
  Get CPUs' BIST by calling SecPlatformInformationPpi/SecPlatformInformation2Ppi.

**/
VOID
RepublishSecPlatformInformationPpi (
  VOID
  )
{
  EFI_STATUS              Status;
  CONST EFI_PEI_SERVICES  **PeiServices;
  UINT64                  BistInformationSize;
  VOID                    *BistInformationData;
  EFI_PEI_PPI_DESCRIPTOR  *SecInformationDescriptor;

  PeiServices = GetPeiServicesTablePointer ();
  Status      = GetBistInfoFromPpi (
                  PeiServices,
                  &gEfiSecPlatformInformation2PpiGuid,
                  &SecInformationDescriptor,
                  &BistInformationData,
                  &BistInformationSize
                  );
  if (Status == EFI_SUCCESS) {
    BuildGuidDataHob (
      &gEfiCallerIdGuid,
      BistInformationData,
      (UINTN)BistInformationSize
      );
    //
    // The old SecPlatformInformation2 data is on temporary memory.
    // After memory discovered, we should never get it from temporary memory,
    // or the data will be crashed. So, we reinstall SecPlatformInformation2 PPI here.
    //
    Status = PeiServicesReInstallPpi (
               SecInformationDescriptor,
               &mPeiSecPlatformInformation2
               );
  }

  if (Status == EFI_NOT_FOUND) {
    Status = GetBistInfoFromPpi (
               PeiServices,
               &gEfiSecPlatformInformationPpiGuid,
               &SecInformationDescriptor,
               &BistInformationData,
               &BistInformationSize
               );
    if (Status == EFI_SUCCESS) {
      BuildGuidDataHob (
        &gEfiCallerIdGuid,
        BistInformationData,
        (UINTN)BistInformationSize
        );
      //
      // The old SecPlatformInformation data is on temporary memory.
      // After memory discovered, we should never get it from temporary memory,
      // or the data will be crashed. So, we reinstall SecPlatformInformation PPI here.
      //
      Status = PeiServicesReInstallPpi (
                 SecInformationDescriptor,
                 &mPeiSecPlatformInformation
                 );
    } else if (Status == EFI_NOT_FOUND) {
      return;
    }
  }

  ASSERT_EFI_ERROR (Status);
}
