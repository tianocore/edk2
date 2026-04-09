/** @file
  Provides cache info for each package, core type, cache level and cache type.

  Copyright (c) 2020 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/CpuCacheInfoLib.h>
#include <InternalCpuCacheInfoLib.h>

/**
  Get EFI_MP_SERVICES_PROTOCOL pointer.

  @param[out] MpServices    A pointer to the buffer where EFI_MP_SERVICES_PROTOCOL is stored

  @retval EFI_SUCCESS       EFI_MP_SERVICES_PROTOCOL interface is returned
  @retval EFI_NOT_FOUND     EFI_MP_SERVICES_PROTOCOL interface is not found
**/
EFI_STATUS
CpuCacheInfoGetMpServices (
  OUT MP_SERVICES  *MpServices
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpServices->Protocol);
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

  Status = MpServices.Protocol->StartupAllAPs (MpServices.Protocol, Procedure, FALSE, NULL, 0, ProcedureArgument, NULL);
  if (Status == EFI_NOT_STARTED) {
    //
    // EFI_NOT_STARTED is returned when there is no enabled AP.
    // Treat this case as EFI_SUCCESS.
    //
    Status = EFI_SUCCESS;
  }

  ASSERT_EFI_ERROR (Status);

  Procedure (ProcedureArgument);
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

  Status = MpServices.Protocol->GetProcessorInfo (MpServices.Protocol, ProcessorNum, ProcessorInfo);
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

  Status = MpServices.Protocol->WhoAmI (MpServices.Protocol, &ProcessorNum);
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

  Status = MpServices.Protocol->GetNumberOfProcessors (MpServices.Protocol, &NumberOfProcessor, &NumberOfEnabledProcessor);
  ASSERT_EFI_ERROR (Status);

  return (UINT32)NumberOfProcessor;
}
