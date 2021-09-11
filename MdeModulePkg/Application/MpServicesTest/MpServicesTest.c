/** @file

    Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/RngLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>

#define MAX_RANDOM_PROCESSOR_RETRIES 10

#define AP_STARTUP_TEST_TIMEOUT_US  50000
#define INFINITE_TIMEOUT            0


/** The procedure to run with the MP Services interface.

  @param Buffer The procedure argument.

**/
STATIC
VOID
EFIAPI
ApFunction (
  IN OUT VOID *Buffer
  )
{
}

/** Displays information returned from MP Services Protocol.

  @param Mp  The MP Services Protocol

  @return The number of CPUs in the system.

**/
STATIC
UINTN
PrintProcessorInformation (
  IN EFI_MP_SERVICES_PROTOCOL *Mp
  )
{
  EFI_STATUS                 Status;
  EFI_PROCESSOR_INFORMATION  CpuInfo;
  UINTN                      Index;
  UINTN                      NumCpu;
  UINTN                      NumEnabledCpu;

  Status = Mp->GetNumberOfProcessors (Mp, &NumCpu, &NumEnabledCpu);
  if (EFI_ERROR (Status)) {
    Print (L"GetNumberOfProcessors failed: %r\n", Status);
  } else {
    Print (L"Number of CPUs: %ld, Enabled: %d\n", NumCpu, NumEnabledCpu);
  }

  for (Index = 0; Index < NumCpu; Index++) {
    Status = Mp->GetProcessorInfo (Mp, CPU_V2_EXTENDED_TOPOLOGY | Index, &CpuInfo);
    if (EFI_ERROR (Status)) {
      Print (L"GetProcessorInfo for Processor %d failed: %r\n", Index, Status);
    } else {
      Print (
        L"Processor %d:\n"
        L"\tID: %016lx\n"
        L"\tStatus: %s | ",
        Index,
        CpuInfo.ProcessorId,
        (CpuInfo.StatusFlag & PROCESSOR_AS_BSP_BIT) ? L"BSP" : L"AP"
        );

      Print (L"%s | ", (CpuInfo.StatusFlag & PROCESSOR_ENABLED_BIT) ? L"Enabled" : L"Disabled");
      Print (L"%s\n", (CpuInfo.StatusFlag & PROCESSOR_HEALTH_STATUS_BIT) ? L"Healthy" : L"Faulted");

      Print (
        L"\tLocation: Package %d, Core %d, Thread %d\n"
        L"\tExtended Information: Package %d, Module %d, Tile %d, Die %d, Core %d, Thread %d\n\n",
        CpuInfo.Location.Package,
        CpuInfo.Location.Core,
        CpuInfo.Location.Thread,
        CpuInfo.ExtendedInformation.Location2.Package,
        CpuInfo.ExtendedInformation.Location2.Module,
        CpuInfo.ExtendedInformation.Location2.Tile,
        CpuInfo.ExtendedInformation.Location2.Die,
        CpuInfo.ExtendedInformation.Location2.Core,
        CpuInfo.ExtendedInformation.Location2.Thread
        );
    }
  }

  return NumCpu;
}

/** Returns the index of an enabled AP selected at random.

  @param Mp             The MP Services Protocol.
  @param ProcessorIndex The index of a random enabled AP.

  @retval EFI_SUCCESS   An enabled processor was found and returned.
  @retval EFI_NOT_FOUND A processor was unable to be selected.

**/
STATIC
EFI_STATUS
GetRandomEnabledProcessorIndex (
  IN EFI_MP_SERVICES_PROTOCOL *Mp,
  OUT UINTN *ProcessorIndex
  )
{
  UINTN                      Index;
  UINTN                      IndexOfEnabledCpu;
  UINTN                      NumCpus;
  UINTN                      NumEnabledCpus;
  UINTN                      IndexOfEnabledCpuToUse;
  UINT16                     RandomNumber;
  BOOLEAN                    Success;
  EFI_STATUS                 Status;
  EFI_PROCESSOR_INFORMATION  CpuInfo;

  IndexOfEnabledCpu = 0;

  Success = GetRandomNumber16 (&RandomNumber);
  ASSERT (Success == TRUE);

  Status = Mp->GetNumberOfProcessors (Mp, &NumCpus, &NumEnabledCpus);
  ASSERT_EFI_ERROR (Status);

  if (NumEnabledCpus == 1) {
    Print (L"All APs are disabled\n");
    return EFI_NOT_FOUND;
  }

  IndexOfEnabledCpuToUse = RandomNumber % NumEnabledCpus;

  for (Index = 0; Index < NumCpus; Index++) {
    Status = Mp->GetProcessorInfo (Mp, Index, &CpuInfo);
    ASSERT_EFI_ERROR (Status);
    if ((CpuInfo.StatusFlag & PROCESSOR_ENABLED_BIT) &&
        !(CpuInfo.StatusFlag & PROCESSOR_AS_BSP_BIT)) {
      if (IndexOfEnabledCpuToUse == IndexOfEnabledCpu) {
        *ProcessorIndex = Index;
        Status = EFI_SUCCESS;
        break;
      }

      IndexOfEnabledCpu++;
    }
  }

  if (Index == NumCpus) {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

/** Tests for the StartupThisAP function.

  @param Mp The MP Services Protocol.

**/
STATIC
VOID
StartupThisApTests (
  IN EFI_MP_SERVICES_PROTOCOL *Mp
  )
{
  EFI_STATUS  Status;
  UINTN       ProcessorIndex;
  UINT32      Retries;

  Retries = 0;

  do {
    Status = GetRandomEnabledProcessorIndex (Mp, &ProcessorIndex);
  } while (EFI_ERROR (Status) && Retries++ < MAX_RANDOM_PROCESSOR_RETRIES);

  if (EFI_ERROR (Status)) {
    return;
  }

  Print (
    L"StartupThisAP on Processor %d with 0 (infinite) timeout...",
    ProcessorIndex
    );

  Status = Mp->StartupThisAP (
                 Mp,
                 ApFunction,
                 ProcessorIndex,
                 NULL,
                 INFINITE_TIMEOUT,
                 NULL,
                 NULL
                 );

  if (EFI_ERROR (Status)) {
    Print (L"failed: %r\n", Status);
    return;
  }
  else {
    Print (L"done.\n");
  }

  Retries = 0;

  do {
    Status = GetRandomEnabledProcessorIndex (Mp, &ProcessorIndex);
  } while (EFI_ERROR (Status) && Retries++ < MAX_RANDOM_PROCESSOR_RETRIES);

  if (EFI_ERROR (Status)) {
    return;
  }

  Print (
    L"StartupThisAP on Processor %d with %dms timeout...",
    ProcessorIndex,
    AP_STARTUP_TEST_TIMEOUT_US / 1000
    );
  Status = Mp->StartupThisAP (
                 Mp,
                 ApFunction,
                 ProcessorIndex,
                 NULL,
                 AP_STARTUP_TEST_TIMEOUT_US,
                 NULL,
                 NULL
                 );
  if (EFI_ERROR (Status)) {
    Print (L"failed: %r\n", Status);
    return;
  }
  else {
    Print (L"done.\n");
  }
}

/** Tests for the StartupAllAPs function.

  @param Mp      The MP Services Protocol.
  @param NumCpus The number of CPUs in the system.

**/
STATIC
VOID
StartupAllAPsTests (
  IN EFI_MP_SERVICES_PROTOCOL *Mp,
  IN UINTN NumCpus
  )
{
  EFI_STATUS  Status;
  UINTN       Timeout;

  Print (L"Running with SingleThread FALSE, 0 (infinite) timeout...");
  Status = Mp->StartupAllAPs (Mp, ApFunction, FALSE, NULL, INFINITE_TIMEOUT, NULL, NULL);
  if (EFI_ERROR (Status)) {
    Print (L"failed: %r\n", Status);
    return;
  }
  else {
    Print (L"done.\n");
  }

  Timeout = NumCpus * AP_STARTUP_TEST_TIMEOUT_US;

  Print (L"Running with SingleThread TRUE, %dms timeout...", Timeout / 1000);
  Status = Mp->StartupAllAPs (
                 Mp,
                 ApFunction,
                 TRUE,
                 NULL,
                 Timeout,
                 NULL,
                 NULL
                 );
  if (EFI_ERROR (Status)) {
    Print (L"failed: %r\n", Status);
    return;
  }
  else {
    Print (L"done.\n");
  }
}

/** Tests for the EnableDisableAP function.

  @param Mp      The MP Services Protocol.
  @param NumCpus The number of CPUs in the system.

**/
STATIC
VOID
EnableDisableAPTests (
  IN EFI_MP_SERVICES_PROTOCOL *Mp,
  IN UINTN                    NumCpus
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINT32      HealthFlag;

  HealthFlag = 0;

  for (Index = 1; Index < NumCpus; Index++) {
    Print (L"Disabling Processor %d with HealthFlag faulted...", Index);
    Status = Mp->EnableDisableAP (Mp, Index, FALSE, &HealthFlag);
    if (EFI_ERROR (Status)) {
      Print (L"failed: %r\n", Status);
      return;
    }
    else {
      Print (L"done.\n");
    }
  }

  HealthFlag = PROCESSOR_HEALTH_STATUS_BIT;

  for (Index = 1; Index < NumCpus; Index++) {
    Print (L"Enabling Processor %d with HealthFlag healthy...", Index);
    Status = Mp->EnableDisableAP (Mp, Index, TRUE, &HealthFlag);
    if (EFI_ERROR (Status)) {
      Print (L"failed: %r\n", Status);
      return;
    }
    else {
      Print (L"done.\n");
    }
  }
}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_MP_SERVICES_PROTOCOL  *Mp;
  EFI_HANDLE                *pHandle;
  UINTN                     HandleCount;
  UINTN                     BspId;
  UINTN                     NumCpus;
  UINTN                     Index;

  pHandle     = NULL;
  HandleCount = 0;
  BspId = 0;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  &HandleCount,
                  &pHandle
                  );

  if (EFI_ERROR (Status)) {
    Print (L"Failed to locate EFI_MP_SERVICES_PROTOCOL (%r). Not installed on platform?\n", Status);
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->OpenProtocol (
                    *pHandle,
                    &gEfiMpServiceProtocolGuid,
                    (VOID **)&Mp,
                    NULL,
                    gImageHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    pHandle++;
  }

  Print (L"Exercising WhoAmI\n\n");
  Status = Mp->WhoAmI (Mp, &BspId);
  if (EFI_ERROR (Status)) {
    Print (L"WhoAmI failed: %r\n", Status);
    return Status;
  } else {
    Print (L"WhoAmI: %016lx\n", BspId);
  }

  Print (L"\n");
  Print (
    L"Exercising GetNumberOfProcessors and GetProcessorInformation with "
    L"CPU_V2_EXTENDED_TOPOLOGY\n\n"
    );
  NumCpus = PrintProcessorInformation (Mp);
  if (NumCpus < 2) {
    Print (L"UP system found. Not running further tests.\n");
    return EFI_INVALID_PARAMETER;
  }

  Print (L"\n");
  Print (L"Exercising StartupThisAP:\n\n");
  StartupThisApTests (Mp);

  Print (L"\n");
  Print (L"Exercising StartupAllAPs:\n\n");
  StartupAllAPsTests (Mp, NumCpus);

  Print (L"\n");
  Print (L"Exercising EnableDisableAP:\n\n");
  EnableDisableAPTests (Mp, NumCpus);

  gBS->CloseProtocol (pHandle, &gEfiMpServiceProtocolGuid, gImageHandle, NULL);
  return EFI_SUCCESS;
}
