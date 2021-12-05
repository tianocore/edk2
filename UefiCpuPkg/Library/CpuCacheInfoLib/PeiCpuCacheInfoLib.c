/** @file
  Provides cache info for each package, core type, cache level and cache type.

  Copyright (c) 2020 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/CpuCacheInfoLib.h>
#include <InternalCpuCacheInfoLib.h>

/**
  Get EDKII_PEI_MP_SERVICES2_PPI pointer.

  @param[out] MpServices    A pointer to the buffer where EDKII_PEI_MP_SERVICES2_PPI is stored

  @retval EFI_SUCCESS       EDKII_PEI_MP_SERVICES2_PPI interface is returned
  @retval EFI_NOT_FOUND     EDKII_PEI_MP_SERVICES2_PPI interface is not found
**/
EFI_STATUS
CpuCacheInfoGetMpServices (
  OUT MP_SERVICES  *MpServices
  )
{
  EFI_STATUS  Status;

  Status = PeiServicesLocatePpi (&gEdkiiPeiMpServices2PpiGuid, 0, NULL, (VOID **)&MpServices->Ppi);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Activate all of the logical processors.

  @param[in]  MpServices          MP_SERVICES structure.
  @param[in]  Procedure           A pointer to the function to be run on enabled logical processors.
  @param[in]  ProcedureArgument   The parameter passed into Procedure for all enabled logical processors.
**/
VOID
CpuCacheInfoStartupAllCPUs (
  IN MP_SERVICES       MpServices,
  IN EFI_AP_PROCEDURE  Procedure,
  IN VOID              *ProcedureArgument
  )
{
  EFI_STATUS  Status;

  Status = MpServices.Ppi->StartupAllCPUs (MpServices.Ppi, Procedure, 0, ProcedureArgument);
  ASSERT_EFI_ERROR (Status);
}

/**
  Get detailed information of the requested logical processor.

  @param[in]  MpServices          MP_SERVICES structure.
  @param[in]  ProcessorNum        The requested logical processor number.
  @param[out] ProcessorInfo       A pointer to the buffer where the processor information is stored
**/
VOID
CpuCacheInfoGetProcessorInfo (
  IN MP_SERVICES                 MpServices,
  IN UINTN                       ProcessorNum,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfo
  )
{
  EFI_STATUS  Status;

  Status = MpServices.Ppi->GetProcessorInfo (MpServices.Ppi, ProcessorNum, ProcessorInfo);
  ASSERT_EFI_ERROR (Status);
}

/**
  Get the logical processor number.

  @param[in]  MpServices          MP_SERVICES structure.

  @retval  Return the logical processor number.
**/
UINT32
CpuCacheInfoWhoAmI (
  IN MP_SERVICES  MpServices
  )
{
  EFI_STATUS  Status;
  UINTN       ProcessorNum;

  Status = MpServices.Ppi->WhoAmI (MpServices.Ppi, &ProcessorNum);
  ASSERT_EFI_ERROR (Status);

  return (UINT32)ProcessorNum;
}

/**
  Get the total number of logical processors in the platform.

  @param[in]  MpServices          MP_SERVICES structure.

  @retval  Return the total number of logical processors.
**/
UINT32
CpuCacheInfoGetNumberOfProcessors (
  IN MP_SERVICES  MpServices
  )
{
  EFI_STATUS  Status;
  UINTN       NumberOfProcessor;
  UINTN       NumberOfEnabledProcessor;

  Status = MpServices.Ppi->GetNumberOfProcessors (MpServices.Ppi, &NumberOfProcessor, &NumberOfEnabledProcessor);
  ASSERT_EFI_ERROR (Status);

  return (UINT32)NumberOfProcessor;
}
