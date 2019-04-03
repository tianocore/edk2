/** @file
  CPU Register Table Library functions.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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

  @return MP_SERVICES variable.
**/
MP_SERVICES
GetMpService (
  VOID
  )
{
  EFI_STATUS                 Status;
  MP_SERVICES                MpService;

  //
  // Get MP Services Protocol
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiMpServicesPpiGuid,
             0,
             NULL,
             (VOID **)&MpService.Ppi
             );
  ASSERT_EFI_ERROR (Status);
  return MpService;
}

/**
  Worker function to return processor index.

  @param  CpuFeaturesData    Cpu Feature Data structure.

  @return  The processor index.
**/
UINTN
GetProcessorIndex (
  IN CPU_FEATURES_DATA        *CpuFeaturesData
  )
{
  EFI_STATUS                 Status;
  EFI_PEI_MP_SERVICES_PPI    *CpuMpPpi;
  UINTN                      ProcessorIndex;

  CpuMpPpi = CpuFeaturesData->MpService.Ppi;

  //
  // For two reasons which use NULL for WhoAmI:
  // 1. This function will be called by APs and AP should not use PeiServices Table
  // 2. Check WhoAmI implementation, this parameter will not be used.
  //
  Status = CpuMpPpi->WhoAmI(NULL, CpuMpPpi, &ProcessorIndex);
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
  CPU_FEATURES_DATA          *CpuFeaturesData;

  CpuFeaturesData = GetCpuFeaturesData ();
  CpuMpPpi = CpuFeaturesData->MpService.Ppi;

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
  @param[in]  MpEvent                 The Event used to sync the result.

**/
VOID
StartupAPsWorker (
  IN  EFI_AP_PROCEDURE                 Procedure,
  IN  EFI_EVENT                        MpEvent
  )
{
  EFI_STATUS                           Status;
  EFI_PEI_MP_SERVICES_PPI              *CpuMpPpi;
  CPU_FEATURES_DATA                    *CpuFeaturesData;

  CpuFeaturesData = GetCpuFeaturesData ();
  CpuMpPpi = CpuFeaturesData->MpService.Ppi;

  //
  // Wakeup all APs for data collection.
  //
  Status = CpuMpPpi->StartupAllAPs (
                 GetPeiServicesTablePointer (),
                 CpuMpPpi,
                 Procedure,
                 FALSE,
                 0,
                 CpuFeaturesData
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
  CPU_FEATURES_DATA                    *CpuFeaturesData;

  CpuFeaturesData = GetCpuFeaturesData ();
  CpuMpPpi = CpuFeaturesData->MpService.Ppi;

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
  CPU_FEATURES_DATA          *CpuFeaturesData;

  CpuFeaturesData = GetCpuFeaturesData ();
  CpuMpPpi = CpuFeaturesData->MpService.Ppi;

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
  Performs CPU features Initialization.

  This service will invoke MP service to perform CPU features
  initialization on BSP/APs per user configuration.

  @note This service could be called by BSP only.
**/
VOID
EFIAPI
CpuFeaturesInitialize (
  VOID
  )
{
  CPU_FEATURES_DATA          *CpuFeaturesData;
  UINTN                      OldBspNumber;

  CpuFeaturesData = GetCpuFeaturesData ();

  OldBspNumber = GetProcessorIndex (CpuFeaturesData);
  CpuFeaturesData->BspNumber = OldBspNumber;

  //
  // Known limitation: In PEI phase, CpuFeatures driver not
  // support async mode execute tasks. So semaphore type
  // register can't been used for this instance, must use
  // DXE type instance.
  //

  //
  // Wakeup all APs for programming.
  //
  StartupAPsWorker (SetProcessorRegister, NULL);
  //
  // Programming BSP
  //
  SetProcessorRegister (CpuFeaturesData);

  //
  // Switch to new BSP if required
  //
  if (CpuFeaturesData->BspNumber != OldBspNumber) {
    SwitchNewBsp (CpuFeaturesData->BspNumber);
  }
}

