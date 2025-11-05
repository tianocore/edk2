/** @file
  Unit tests of the CpuExceptionHandlerLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuExceptionHandlerTest.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Timer.h>

/**
  Initialize Bsp Idt with a new Idt table and return the IA32_DESCRIPTOR buffer.
  In PEIM, store original PeiServicePointer before new Idt table.

  @return Pointer to the allocated IA32_DESCRIPTOR buffer.
**/
VOID *
InitializeBspIdt (
  VOID
  )
{
  UINTN            *NewIdtTable;
  IA32_DESCRIPTOR  *Idtr;

  Idtr = AllocateZeroPool (sizeof (IA32_DESCRIPTOR));
  ASSERT (Idtr != NULL);
  NewIdtTable = AllocateZeroPool (sizeof (IA32_IDT_GATE_DESCRIPTOR) * CPU_INTERRUPT_NUM);
  ASSERT (NewIdtTable != NULL);
  Idtr->Base  = (UINTN)NewIdtTable;
  Idtr->Limit = (UINT16)(sizeof (IA32_IDT_GATE_DESCRIPTOR) * CPU_INTERRUPT_NUM - 1);

  AsmWriteIdtr (Idtr);
  return Idtr;
}

/**
  Retrieve the number of logical processor in the platform and the number of those logical processors that
  are enabled on this boot.

  @param[in]  MpServices          MP_SERVICES structure.
  @param[out] NumberOfProcessors  Pointer to the total number of logical processors in the system, including
                                  the BSP and disabled APs.
  @param[out] NumberOfEnabledProcessors Pointer to the number of processors in the system that are enabled.

  @retval EFI_SUCCESS       Retrieve the number of logical processor successfully
  @retval Others            Retrieve the number of logical processor unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestGetNumberOfProcessors (
  IN MP_SERVICES  MpServices,
  OUT UINTN       *NumberOfProcessors,
  OUT UINTN       *NumberOfEnabledProcessors
  )
{
  return MpServices.Protocol->GetNumberOfProcessors (MpServices.Protocol, NumberOfProcessors, NumberOfEnabledProcessors);
}

/**
  Get the handle number for the calling processor.

  @param[in]  MpServices      MP_SERVICES structure.
  @param[out] ProcessorNumber The handle number for the calling processor.

  @retval EFI_SUCCESS       Get the handle number for the calling processor successfully.
  @retval Others            Get the handle number for the calling processor unsuccessfully.
**/
EFI_STATUS
MpServicesUnitTestWhoAmI (
  IN MP_SERVICES  MpServices,
  OUT UINTN       *ProcessorNumber
  )
{
  return MpServices.Protocol->WhoAmI (MpServices.Protocol, ProcessorNumber);
}

/**
  Caller gets one enabled AP to execute a caller-provided function.

  @param[in]  MpServices    MP_SERVICES structure.
  @param[in]  Procedure     Pointer to the function to be run on enabled APs of the system.
  @param[in]  ProcessorNumber       The handle number of the AP.
  @param[in]  TimeoutInMicroSeconds Indicates the time limit in microseconds for APs to return from Procedure,
                                    for blocking mode only. Zero means infinity.
  @param[in]  ProcedureArgument     The parameter passed into Procedure for all APs.


  @retval EFI_SUCCESS       Caller gets one enabled AP to execute a caller-provided function successfully
  @retval Others            Caller gets one enabled AP to execute a caller-provided function unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestStartupThisAP (
  IN MP_SERVICES       MpServices,
  IN EFI_AP_PROCEDURE  Procedure,
  IN UINTN             ProcessorNumber,
  IN UINTN             TimeoutInMicroSeconds,
  IN VOID              *ProcedureArgument
  )
{
  return MpServices.Protocol->StartupThisAP (MpServices.Protocol, Procedure, ProcessorNumber, NULL, TimeoutInMicroSeconds, ProcedureArgument, NULL);
}

/**
  Execute a caller provided function on all enabled APs.

  @param[in]  MpServices    MP_SERVICES structure.
  @param[in]  Procedure     Pointer to the function to be run on enabled APs of the system.
  @param[in]  SingleThread  If TRUE, then all the enabled APs execute the function specified by Procedure
                            one by one, in ascending order of processor handle number.
                            If FALSE, then all the enabled APs execute the function specified by Procedure
                            simultaneously.
  @param[in]  TimeoutInMicroSeconds Indicates the time limit in microseconds for APs to return from Procedure,
                                    for blocking mode only. Zero means infinity.
  @param[in]  ProcedureArgument     The parameter passed into Procedure for all APs.

  @retval EFI_SUCCESS       Execute a caller provided function on all enabled APs successfully
  @retval Others            Execute a caller provided function on all enabled APs unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestStartupAllAPs (
  IN MP_SERVICES       MpServices,
  IN EFI_AP_PROCEDURE  Procedure,
  IN BOOLEAN           SingleThread,
  IN UINTN             TimeoutInMicroSeconds,
  IN VOID              *ProcedureArgument
  )
{
  return MpServices.Protocol->StartupAllAPs (MpServices.Protocol, Procedure, SingleThread, NULL, TimeoutInMicroSeconds, ProcedureArgument, NULL);
}

/**
  Get EFI_MP_SERVICES_PROTOCOL pointer.

  @param[out] MpServices    Pointer to the buffer where EFI_MP_SERVICES_PROTOCOL is stored

  @retval EFI_SUCCESS       EFI_MP_SERVICES_PROTOCOL interface is returned
  @retval EFI_NOT_FOUND     EFI_MP_SERVICES_PROTOCOL interface is not found
**/
EFI_STATUS
GetMpServices (
  OUT MP_SERVICES  *MpServices
  )
{
  return gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpServices->Protocol);
}

/**
  Entry for CpuExceptionHandlerDxeTest driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to the System Table.

  @retval EFI_SUCCESS    The driver executed normally.

**/
EFI_STATUS
EFIAPI
CpuExceptionHandlerTestEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  EFI_TIMER_ARCH_PROTOCOL     *TimerArchProtocol;
  UINT64                      TimerPeriod;

  Framework         = NULL;
  TimerArchProtocol = NULL;
  TimerPeriod       = 0;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  Status = AddCommonTestCase (Framework);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in AddCommonTestCase. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // If HpetTimer driver has been dispatched, disable HpetTimer before Unit Test.
  //
  gBS->LocateProtocol (&gEfiTimerArchProtocolGuid, NULL, (VOID **)&TimerArchProtocol);
  if (TimerArchProtocol != NULL) {
    Status = TimerArchProtocol->GetTimerPeriod (TimerArchProtocol, &TimerPeriod);
    ASSERT_EFI_ERROR (Status);
    if (TimerPeriod > 0) {
      DEBUG ((DEBUG_INFO, "HpetTimer has been dispatched. Disable HpetTimer.\n"));
      Status = TimerArchProtocol->SetTimerPeriod (TimerArchProtocol, 0);
      ASSERT_EFI_ERROR (Status);
    }
  }

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

  //
  // Restore HpetTimer after Unit Test.
  //
  if ((TimerArchProtocol != NULL) && (TimerPeriod > 0)) {
    DEBUG ((DEBUG_INFO, "Restore HpetTimer after DxeCpuExceptionHandlerLib UnitTest.\n"));
    Status = TimerArchProtocol->SetTimerPeriod (TimerArchProtocol, TimerPeriod);
    ASSERT_EFI_ERROR (Status);
  }

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}
