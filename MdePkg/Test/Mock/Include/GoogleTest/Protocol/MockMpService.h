/** @file MockMpService.h
  This file declares a mock of MP service Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_MP_SERVICE_H_
#define MOCK_MP_SERVICE_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/MpService.h>
}

struct MockEfiMpServicesProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiMpServicesProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetNumberOfProcessors,
    (
     IN  EFI_MP_SERVICES_PROTOCOL  *This,
     OUT UINTN                     *NumberOfProcessors,
     OUT UINTN                     *NumberOfEnabledProcessors
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetProcessorInfo,
    (
     IN  EFI_MP_SERVICES_PROTOCOL   *This,
     IN  UINTN                      ProcessorNumber,
     OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    StartupAllAPs,
    (
     IN  EFI_MP_SERVICES_PROTOCOL  *This,
     IN  EFI_AP_PROCEDURE          Procedure,
     IN  BOOLEAN                   SingleThread,
     IN  EFI_EVENT                 WaitEvent               OPTIONAL,
     IN  UINTN                     TimeoutInMicroSeconds,
     IN  VOID                      *ProcedureArgument      OPTIONAL,
     OUT UINTN                     **FailedCpuList         OPTIONAL
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    StartupThisAP,
    (
     IN  EFI_MP_SERVICES_PROTOCOL  *This,
     IN  EFI_AP_PROCEDURE          Procedure,
     IN  UINTN                     ProcessorNumber,
     IN  EFI_EVENT                 WaitEvent               OPTIONAL,
     IN  UINTN                     TimeoutInMicroseconds,
     IN  VOID                      *ProcedureArgument      OPTIONAL,
     OUT BOOLEAN                   *Finished               OPTIONAL
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SwitchBSP,
    (
     IN EFI_MP_SERVICES_PROTOCOL  *This,
     IN  UINTN                    ProcessorNumber,
     IN  BOOLEAN                  EnableOldBSP
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EnableDisableAP,
    (
     IN  EFI_MP_SERVICES_PROTOCOL  *This,
     IN  UINTN                     ProcessorNumber,
     IN  BOOLEAN                   EnableAP,
     IN  UINT32                    *HealthFlag OPTIONAL
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    WhoAmI,
    (
     IN EFI_MP_SERVICES_PROTOCOL  *This,
     OUT UINTN                    *ProcessorNumber
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockEfiMpServicesProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiMpServicesProtocol, GetNumberOfProcessors, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiMpServicesProtocol, GetProcessorInfo, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiMpServicesProtocol, StartupAllAPs, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiMpServicesProtocol, StartupThisAP, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiMpServicesProtocol, SwitchBSP, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiMpServicesProtocol, EnableDisableAP, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiMpServicesProtocol, WhoAmI, 2, EFIAPI);

#define MOCK_EFI_MP_SERVICES_PROTOCOL_INSTANCE(NAME)  \
  EFI_MP_SERVICES_PROTOCOL NAME##_INSTANCE = {        \
    GetNumberOfProcessors,                            \
    GetProcessorInfo,                                 \
    StartupAllAPs,                                    \
    StartupThisAP,                                    \
    SwitchBSP,                                        \
    EnableDisableAP,                                  \
    WhoAmI };                                         \
  EFI_MP_SERVICES_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif // MOCK_MP_SERVICE_H_
