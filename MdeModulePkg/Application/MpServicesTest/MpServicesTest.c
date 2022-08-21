/** @file
  UEFI Application to exercise EFI_MP_SERVICES_PROTOCOL.

  Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>

#include "Options.h"

#define APFUNC_BUFFER_LEN  256

typedef struct {
  EFI_MP_SERVICES_PROTOCOL    *Mp;
  CHAR16                      **Buffer;
} APFUNC_ARG;

/** The procedure to run with the MP Services interface.

  @param Arg The procedure argument.

**/
STATIC
VOID
EFIAPI
ApFunction (
  IN OUT VOID  *Arg
  )
{
  APFUNC_ARG  *Param;
  UINTN       ProcessorId;

  if (Arg != NULL) {
    Param = Arg;

    Param->Mp->WhoAmI (Param->Mp, &ProcessorId);
    UnicodeSPrint (Param->Buffer[ProcessorId], APFUNC_BUFFER_LEN, L"Hello from CPU %ld\n", ProcessorId);
  }
}

/**
  Fetches the number of processors and which processor is the BSP.

  @param Mp  MP Services Protocol.
  @param NumProcessors Number of processors.
  @param BspIndex      The index of the BSP.
**/
STATIC
EFI_STATUS
GetProcessorInformation (
  IN  EFI_MP_SERVICES_PROTOCOL  *Mp,
  OUT UINTN                     *NumProcessors,
  OUT UINTN                     *BspIndex
  )
{
  EFI_STATUS  Status;
  UINTN       NumEnabledProcessors;

  Status = Mp->GetNumberOfProcessors (Mp, NumProcessors, &NumEnabledProcessors);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Mp->WhoAmI (Mp, BspIndex);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/** Displays information returned from MP Services Protocol.

  @param Mp       The MP Services Protocol
  @param BspIndex On return, contains the index of the BSP.

  @return The number of CPUs in the system.

**/
STATIC
UINTN
PrintProcessorInformation (
  IN   EFI_MP_SERVICES_PROTOCOL  *Mp,
  OUT  UINTN                     *BspIndex
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

      if ((CpuInfo.StatusFlag & PROCESSOR_AS_BSP_BIT) && (BspIndex != NULL)) {
        *BspIndex = Index;
      }

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

/** Allocates memory in ApArg for the single AP specified.

  @param ApArg          Pointer to the AP argument structure.
  @param Mp             The MP Services Protocol.
  @param ProcessorIndex The index of the AP.

  @retval EFI_SUCCESS          Memory was successfully allocated.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.

**/
STATIC
EFI_STATUS
AllocateApFuncBufferSingleAP (
  IN APFUNC_ARG                *ApArg,
  IN EFI_MP_SERVICES_PROTOCOL  *Mp,
  IN UINTN                     ProcessorIndex
  )
{
  ApArg->Mp = Mp;

  ApArg->Buffer = AllocateZeroPool ((ProcessorIndex + 1) * sizeof (VOID *));
  if (ApArg->Buffer == NULL) {
    Print (L"Failed to allocate buffer for AP buffer\n");
    return EFI_OUT_OF_RESOURCES;
  }

  ApArg->Buffer[ProcessorIndex] = AllocateZeroPool (APFUNC_BUFFER_LEN);
  if (ApArg->Buffer[ProcessorIndex] == NULL) {
    Print (L"Failed to allocate buffer for AP buffer\n");
    FreePool (ApArg->Buffer);
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/** Allocates memory in ApArg for all APs.

  @param ApArg   Pointer to the AP argument structure.
  @param Mp      The MP Services Protocol.
  @param NumCpus The number of CPUs.

  @retval EFI_SUCCESS          Memory was successfully allocated.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.

**/
STATIC
EFI_STATUS
AllocateApFuncBufferAllAPs (
  IN APFUNC_ARG                *ApArg,
  IN EFI_MP_SERVICES_PROTOCOL  *Mp,
  IN UINTN                     NumCpus
  )
{
  INT32  Index;

  ApArg->Mp = Mp;

  ApArg->Buffer = AllocateZeroPool (NumCpus * sizeof (VOID *));
  if (ApArg->Buffer == NULL) {
    Print (L"Failed to allocate buffer for AP message\n");
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < NumCpus; Index++) {
    ApArg->Buffer[Index] = AllocateZeroPool (APFUNC_BUFFER_LEN);
    if (ApArg->Buffer[Index] == NULL) {
      Print (L"Failed to allocate buffer for AP message\n");
      for (--Index; Index >= 0; Index++) {
        FreePool (ApArg->Buffer[Index]);
      }

      FreePool (ApArg->Buffer);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}

/** Frees memory in ApArg for all APs.

  @param ApArg   Pointer to the AP argument structure.
  @param NumCpus The number of CPUs.

**/
STATIC
VOID
FreeApFuncBuffer (
  APFUNC_ARG  *ApArg,
  UINTN       NumCpus
  )
{
  UINTN  Index;

  for (Index = 0; Index < NumCpus; Index++) {
    if (ApArg->Buffer[Index] != NULL) {
      FreePool (ApArg->Buffer[Index]);
    }
  }

  FreePool (ApArg->Buffer);
}

/** Runs a specified AP.

  @param Mp             The MP Services Protocol.
  @param ProcessorIndex The processor index.
  @param Timeout        Timeout in milliseconds.

  @return EFI_SUCCESS on success, or an error code.

**/
STATIC
EFI_STATUS
StartupThisAP (
  IN EFI_MP_SERVICES_PROTOCOL  *Mp,
  IN UINTN                     ProcessorIndex,
  IN UINTN                     Timeout
  )
{
  EFI_STATUS  Status;
  APFUNC_ARG  ApArg;

  Status = AllocateApFuncBufferSingleAP (&ApArg, Mp, ProcessorIndex);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AllocateApFuncBufferSingleAP (&ApArg, Mp, ProcessorIndex);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Print (
    L"StartupThisAP on Processor %d with %d%s timeout...",
    ProcessorIndex,
    Timeout,
    (Timeout == INFINITE_TIMEOUT) ? " (infinite)" : "ms"
    );
  Status = Mp->StartupThisAP (
                 Mp,
                 ApFunction,
                 ProcessorIndex,
                 NULL,
                 Timeout * 1000,
                 &ApArg,
                 NULL
                 );
  if (EFI_ERROR (Status)) {
    Print (L"failed: %r\n", Status);
    return Status;
  } else {
    Print (L"done.\n");
    Print (ApArg.Buffer[ProcessorIndex]);
  }

  FreeApFuncBuffer (&ApArg, ProcessorIndex + 1);

  return EFI_SUCCESS;
}

/** Runs all APs.

  @param Mp                 The MP Services Protocol.
  @param NumCpus            The number of CPUs in the system.
  @param Timeout            Timeout in milliseconds.
  @param RunAPsSequentially Run APs sequentially (FALSE: run simultaneously)

  @return EFI_SUCCESS on success, or an error code.

**/
STATIC
EFI_STATUS
StartupAllAPs (
  IN EFI_MP_SERVICES_PROTOCOL  *Mp,
  IN UINTN                     NumCpus,
  IN UINTN                     Timeout,
  IN BOOLEAN                   RunAPsSequentially
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  APFUNC_ARG  ApArg;

  Status = AllocateApFuncBufferAllAPs (&ApArg, Mp, NumCpus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Print (
    L"Running with SingleThread TRUE, %dms timeout...",
    Timeout,
    (Timeout == INFINITE_TIMEOUT) ? " (infinite)" : "ms"
    );
  Status = Mp->StartupAllAPs (
                 Mp,
                 ApFunction,
                 RunAPsSequentially,
                 NULL,
                 Timeout * 1000,
                 &ApArg,
                 NULL
                 );
  if (EFI_ERROR (Status)) {
    Print (L"failed: %r\n", Status);
    return Status;
  } else {
    Print (L"done.\n");
    for (Index = 0; Index < NumCpus; Index++) {
      Print (ApArg.Buffer[Index]);
    }
  }

  FreeApFuncBuffer (&ApArg, NumCpus);
  return EFI_SUCCESS;
}

/**
  Enables the specified AP.

  @param Mp               The MP Services Protocol.
  @param ProcessorIndex   The processor to enable.
  @param ProcessorHealthy The health status of the processor.

  @return EFI_SUCCESS on success, or an error code.
**/
STATIC
EFI_STATUS
EnableAP (
  IN EFI_MP_SERVICES_PROTOCOL  *Mp,
  UINTN                        ProcessorIndex,
  BOOLEAN                      ProcessorHealthy
  )
{
  EFI_STATUS  Status;
  UINT32      HealthFlag;

  if (ProcessorHealthy) {
    Print (L"Enabling Processor %d with HealthFlag healthy...", ProcessorIndex);
    HealthFlag = PROCESSOR_HEALTH_STATUS_BIT;
  } else {
    Print (L"Enabling Processor %d with HealthFlag faulted...", ProcessorIndex);
    HealthFlag = 0;
  }

  Status = Mp->EnableDisableAP (Mp, ProcessorIndex, TRUE, &HealthFlag);
  if (EFI_ERROR (Status)) {
    Print (L"failed: %r\n", Status);
    return Status;
  } else {
    Print (L"done.\n");
  }

  return Status;
}

/**
  Disables the specified AP.

  @param Mp               The MP Services Protocol.
  @param ProcessorIndex   The processor to disable.
  @param ProcessorHealthy The health status of the processor.

  @return EFI_SUCCESS on success, or an error code.
**/
STATIC
EFI_STATUS
DisableAP (
  IN EFI_MP_SERVICES_PROTOCOL  *Mp,
  UINTN                        ProcessorIndex,
  BOOLEAN                      ProcessorHealthy
  )
{
  EFI_STATUS  Status;
  UINT32      HealthFlag;

  if (ProcessorHealthy) {
    Print (L"Disabling Processor %d with HealthFlag healthy...", ProcessorIndex);
    HealthFlag = PROCESSOR_HEALTH_STATUS_BIT;
  } else {
    Print (L"Disabling Processor %d with HealthFlag faulted...", ProcessorIndex);
    HealthFlag = 0;
  }

  Status = Mp->EnableDisableAP (Mp, ProcessorIndex, FALSE, &HealthFlag);
  if (EFI_ERROR (Status)) {
    Print (L"failed: %r\n", Status);
    return Status;
  } else {
    Print (L"done.\n");
  }

  return Status;
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
  UINTN                     BspIndex;
  UINTN                     CpuIndex;
  UINTN                     NumCpus;
  BOOLEAN                   ProcessorHealthy;
  MP_SERVICES_TEST_OPTIONS  Options;

  WriteBackDataCacheRange ((VOID *)&ApFunction, 32);

  BspIndex = 0;

  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **)&Mp
                  );
  if (EFI_ERROR (Status)) {
    Print (L"Failed to locate EFI_MP_SERVICES_PROTOCOL (%r). Not installed on platform?\n", Status);
    return EFI_NOT_FOUND;
  }

  Status = ParseArguments (&Options);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetProcessorInformation (Mp, &NumCpus, &BspIndex);
  if (EFI_ERROR (Status)) {
    Print (L"Error: Failed to fetch processor information.\n");
    return Status;
  }

  if (Options.PrintBspProcessorIndex) {
    Status = Mp->WhoAmI (Mp, &CpuIndex);
    if (EFI_ERROR (Status)) {
      Print (L"WhoAmI failed: %r\n", Status);
      return Status;
    } else {
      Print (L"BSP: %016lx\n", CpuIndex);
    }
  }

  if (Options.PrintProcessorInformation) {
    NumCpus = PrintProcessorInformation (Mp, &BspIndex);
    if (NumCpus < 2) {
      Print (L"Error: Uniprocessor system found.\n");
      return EFI_INVALID_PARAMETER;
    }
  }

  if (Options.RunSingleAP) {
    Status = StartupThisAP (
               Mp,
               Options.ProcessorIndex,
               Options.Timeout
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Options.RunAllAPs) {
    Status = StartupAllAPs (Mp, NumCpus, Options.Timeout, Options.RunAPsSequentially);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Options.EnableProcessor) {
    ProcessorHealthy = TRUE;
    if (Options.SetProcessorUnhealthy) {
      ProcessorHealthy = FALSE;
    }

    Status = EnableAP (Mp, Options.ProcessorIndex, ProcessorHealthy);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Options.DisableProcessor) {
    ProcessorHealthy = TRUE;
    if (Options.SetProcessorUnhealthy) {
      ProcessorHealthy = FALSE;
    }

    Status = DisableAP (Mp, Options.ProcessorIndex, ProcessorHealthy);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}
