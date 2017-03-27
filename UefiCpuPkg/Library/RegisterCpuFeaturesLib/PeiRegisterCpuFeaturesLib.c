/** @file
  CPU Register Table Library functions.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Ppi/MpServices.h>
#include "RegisterCpuFeatures.h"

#define REGISTER_CPU_FEATURES_GUID \
  { \
    0xa694c467, 0x697a, 0x446b, { 0xb9, 0x29, 0x5b, 0x14, 0xa0, 0xcf, 0x39, 0xf } \
  }

EFI_GUID mRegisterCpuFeaturesHobGuid = REGISTER_CPU_FEATURES_GUID;

/**
  Worker function to get CPU_FEATURES_DATA pointer.

  @return Pointer to CPU_FEATURES_DATA.
**/
CPU_FEATURES_DATA *
GetCpuFeaturesData (
  VOID
  )
{
  CPU_FEATURES_DATA       *CpuInitData;
  EFI_HOB_GUID_TYPE       *GuidHob;
  VOID                    *DataInHob;
  UINT64                  Data64;

  CpuInitData = NULL;
  GuidHob = GetFirstGuidHob (&mRegisterCpuFeaturesHobGuid);
  if (GuidHob != NULL) {
    DataInHob = GET_GUID_HOB_DATA (GuidHob);
    CpuInitData = (CPU_FEATURES_DATA *) (*(UINTN *) DataInHob);
    ASSERT (CpuInitData != NULL);
  } else {
    CpuInitData = AllocateZeroPool (sizeof (CPU_FEATURES_DATA));
    ASSERT (CpuInitData != NULL);
    //
    // Build location of CPU MP DATA buffer in HOB
    //
    Data64 = (UINT64) (UINTN) CpuInitData;
    BuildGuidDataHob (
      &mRegisterCpuFeaturesHobGuid,
      (VOID *) &Data64,
      sizeof (UINT64)
      );
  }

  return CpuInitData;
}

/**
  Worker function to get MP PPI service pointer.

  @return PEI PPI service pointer.
**/
EFI_PEI_MP_SERVICES_PPI *
GetMpPpi (
  VOID
  )
{
  EFI_STATUS                 Status;
  EFI_PEI_MP_SERVICES_PPI    *CpuMpPpi;

  //
  // Get MP Services Protocol
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiMpServicesPpiGuid,
             0,
             NULL,
             (VOID **)&CpuMpPpi
             );
  ASSERT_EFI_ERROR (Status);
  return CpuMpPpi;
}

/**
  Worker function to return processor index.

  @return  The processor index.
**/
UINTN
GetProcessorIndex (
  VOID
  )
{
  EFI_STATUS                 Status;
  EFI_PEI_MP_SERVICES_PPI    *CpuMpPpi;
  UINTN                      ProcessorIndex;

  CpuMpPpi = GetMpPpi ();

  Status = CpuMpPpi->WhoAmI(GetPeiServicesTablePointer (), CpuMpPpi, &ProcessorIndex);
  ASSERT_EFI_ERROR (Status);
  return ProcessorIndex;
}

/**
  Worker function to MP-related information on the requested processor at the
  instant this call is made.

  @param[in]  ProcessorNumber       The handle number of processor.
  @param[out] ProcessorInfoBuffer   A pointer to the buffer where information for
                                    the requested processor is deposited.

  @return Status of MpServices->GetProcessorInfo().
**/
EFI_STATUS
GetProcessorInformation (
  IN  UINTN                            ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION        *ProcessorInfoBuffer
  )
{
  EFI_PEI_MP_SERVICES_PPI    *CpuMpPpi;
  EFI_STATUS                 Status;

  CpuMpPpi = GetMpPpi ();
  Status = CpuMpPpi->GetProcessorInfo (
               GetPeiServicesTablePointer(),
               CpuMpPpi,
               ProcessorNumber,
               ProcessorInfoBuffer
               );
  return Status;
}

/**
  Worker function to execute a caller provided function on all enabled APs.

  @param[in]  Procedure               A pointer to the function to be run on
                                      enabled APs of the system.
**/
VOID
StartupAPsWorker (
  IN  EFI_AP_PROCEDURE                 Procedure
  )
{
  EFI_STATUS                           Status;
  EFI_PEI_MP_SERVICES_PPI              *CpuMpPpi;

  //
  // Get MP Services Protocol
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiMpServicesPpiGuid,
             0,
             NULL,
             (VOID **)&CpuMpPpi
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Wakeup all APs for data collection.
  //
  Status = CpuMpPpi->StartupAllAPs (
                 GetPeiServicesTablePointer (),
                 CpuMpPpi,
                 Procedure,
                 FALSE,
                 0,
                 NULL
                 );
  ASSERT_EFI_ERROR (Status);
}

/**
  Worker function to switch the requested AP to be the BSP from that point onward.

  @param[in] ProcessorNumber   The handle number of AP that is to become the new BSP.
**/
VOID
SwitchNewBsp (
  IN  UINTN                            ProcessorNumber
  )
{
  EFI_STATUS                           Status;
  EFI_PEI_MP_SERVICES_PPI              *CpuMpPpi;

  //
  // Get MP Services Protocol
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiMpServicesPpiGuid,
             0,
             NULL,
             (VOID **)&CpuMpPpi
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Wakeup all APs for data collection.
  //
  Status = CpuMpPpi->SwitchBSP (
                 GetPeiServicesTablePointer (),
                 CpuMpPpi,
                 ProcessorNumber,
                 TRUE
                 );
  ASSERT_EFI_ERROR (Status);
}

/**
  Worker function to retrieve the number of logical processor in the platform.

  @param[out] NumberOfCpus                Pointer to the total number of logical
                                          processors in the system, including the BSP
                                          and disabled APs.
  @param[out] NumberOfEnabledProcessors   Pointer to the number of enabled logical
                                          processors that exist in system, including
                                          the BSP.
**/
VOID
GetNumberOfProcessor (
  OUT UINTN                            *NumberOfCpus,
  OUT UINTN                            *NumberOfEnabledProcessors
  )
{
  EFI_STATUS                 Status;
  EFI_PEI_MP_SERVICES_PPI    *CpuMpPpi;

  //
  // Get MP Services Protocol
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiMpServicesPpiGuid,
             0,
             NULL,
             (VOID **)&CpuMpPpi
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Get the number of CPUs
  //
  Status = CpuMpPpi->GetNumberOfProcessors (
                         GetPeiServicesTablePointer (),
                         CpuMpPpi,
                         NumberOfCpus,
                         NumberOfEnabledProcessors
                         );
  ASSERT_EFI_ERROR (Status);
}

/**
  Allocates ACPI NVS memory to save ACPI_CPU_DATA.

  @return  Pointer to allocated ACPI_CPU_DATA.
**/
ACPI_CPU_DATA *
AllocateAcpiCpuData (
  VOID
  )
{
  EFI_STATUS                           Status;
  EFI_PEI_MP_SERVICES_PPI              *CpuMpPpi;
  UINTN                                NumberOfCpus;
  UINTN                                NumberOfEnabledProcessors;
  ACPI_CPU_DATA                        *AcpiCpuData;
  EFI_PHYSICAL_ADDRESS                 Address;
  UINTN                                TableSize;
  CPU_REGISTER_TABLE                   *RegisterTable;
  UINTN                                Index;
  EFI_PROCESSOR_INFORMATION            ProcessorInfoBuffer;

  Status = PeiServicesAllocatePages (
             EfiACPIMemoryNVS,
             EFI_SIZE_TO_PAGES (sizeof (ACPI_CPU_DATA)),
             &Address
             );
  ASSERT_EFI_ERROR (Status);
  AcpiCpuData = (ACPI_CPU_DATA *) (UINTN) Address;
  ASSERT (AcpiCpuData != NULL);

  //
  // Get MP Services Protocol
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiMpServicesPpiGuid,
             0,
             NULL,
             (VOID **)&CpuMpPpi
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Get the number of CPUs
  //
  Status = CpuMpPpi->GetNumberOfProcessors (
                         GetPeiServicesTablePointer (),
                         CpuMpPpi,
                         &NumberOfCpus,
                         &NumberOfEnabledProcessors
                         );
  ASSERT_EFI_ERROR (Status);
  AcpiCpuData->NumberOfCpus = (UINT32)NumberOfCpus;

  //
  // Allocate buffer for empty RegisterTable and PreSmmInitRegisterTable for all CPUs
  //
  TableSize = 2 * NumberOfCpus * sizeof (CPU_REGISTER_TABLE);
  Status = PeiServicesAllocatePages (
             EfiACPIMemoryNVS,
             EFI_SIZE_TO_PAGES (TableSize),
             &Address
             );
  ASSERT_EFI_ERROR (Status);
  RegisterTable = (CPU_REGISTER_TABLE *) (UINTN) Address;

  for (Index = 0; Index < NumberOfCpus; Index++) {
    Status = CpuMpPpi->GetProcessorInfo (
                         GetPeiServicesTablePointer (),
                         CpuMpPpi,
                         Index,
                         &ProcessorInfoBuffer
                         );
    ASSERT_EFI_ERROR (Status);

    RegisterTable[Index].InitialApicId      = (UINT32)ProcessorInfoBuffer.ProcessorId;
    RegisterTable[Index].TableLength        = 0;
    RegisterTable[Index].AllocatedSize      = 0;
    RegisterTable[Index].RegisterTableEntry = 0;

    RegisterTable[NumberOfCpus + Index].InitialApicId      = (UINT32)ProcessorInfoBuffer.ProcessorId;
    RegisterTable[NumberOfCpus + Index].TableLength        = 0;
    RegisterTable[NumberOfCpus + Index].AllocatedSize      = 0;
    RegisterTable[NumberOfCpus + Index].RegisterTableEntry = 0;
  }
  AcpiCpuData->RegisterTable           = (EFI_PHYSICAL_ADDRESS)(UINTN)RegisterTable;
  AcpiCpuData->PreSmmInitRegisterTable = (EFI_PHYSICAL_ADDRESS)(UINTN)(RegisterTable + NumberOfCpus);

  return AcpiCpuData;
}

/**
  Enlarges CPU register table for each processor.

  @param[in, out]  RegisterTable   Pointer processor's CPU register table
**/
VOID
EnlargeRegisterTable (
  IN OUT CPU_REGISTER_TABLE            *RegisterTable
  )
{
  EFI_STATUS                           Status;
  EFI_PHYSICAL_ADDRESS                 Address;
  UINTN                                AllocatePages;

  AllocatePages = RegisterTable->AllocatedSize / EFI_PAGE_SIZE;
  Status = PeiServicesAllocatePages (
             EfiACPIMemoryNVS,
             AllocatePages + 1,
             &Address
             );
  ASSERT_EFI_ERROR (Status);

  //
  // If there are records existing in the register table, then copy its contents
  // to new region and free the old one.
  //
  if (RegisterTable->AllocatedSize > 0) {
    CopyMem (
      (VOID *) (UINTN) Address,
      (VOID *) (UINTN) RegisterTable->RegisterTableEntry,
      RegisterTable->AllocatedSize
      );
  }

  //
  // Adjust the allocated size and register table base address.
  //
  RegisterTable->AllocatedSize += EFI_PAGE_SIZE;
  RegisterTable->RegisterTableEntry = Address;
}
