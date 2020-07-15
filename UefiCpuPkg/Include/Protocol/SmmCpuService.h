/** @file
SMM CPU Service protocol definition.

Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_CPU_SERVICE_PROTOCOL_H_
#define _SMM_CPU_SERVICE_PROTOCOL_H_

//
// Share some definitions with MP Services and CPU Arch Protocol
//
#include <Protocol/MpService.h>
#include <Protocol/Cpu.h>

#define EFI_SMM_CPU_SERVICE_PROTOCOL_GUID \
  { \
    0x1d202cab, 0xc8ab, 0x4d5c, { 0x94, 0xf7, 0x3c, 0xfc, 0xc0, 0xd3, 0xd3, 0x35 } \
  }

typedef struct _EFI_SMM_CPU_SERVICE_PROTOCOL EFI_SMM_CPU_SERVICE_PROTOCOL;

//
// Protocol functions
//

/**
  Gets processor information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  @param[in]  This                  A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL
                                    instance.
  @param[in]  ProcessorNumber       The handle number of processor.
  @param[out] ProcessorInfoBuffer   A pointer to the buffer where information for
                                    the requested processor is deposited.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.
**/
typedef
EFI_STATUS
(EFIAPI * EFI_SMM_GET_PROCESSOR_INFO) (
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL *This,
  IN       UINTN                        ProcessorNumber,
  OUT      EFI_PROCESSOR_INFORMATION    *ProcessorInfoBuffer
  );

/**
  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes.   This call can only be performed
  by the current BSP.

  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes. The new BSP can take over the
  execution of the old BSP and continue seamlessly from where the old one left
  off.

  If the BSP cannot be switched prior to the return from this service, then
  EFI_UNSUPPORTED must be returned.

  @param[in] This              A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[in] ProcessorNumber   The handle number of AP that is to become the new
                               BSP. The range is from 0 to the total number of
                               logical processors minus 1.

  @retval EFI_SUCCESS             BSP successfully switched.
  @retval EFI_UNSUPPORTED         Switching the BSP cannot be completed prior to
                                  this service returning.
  @retval EFI_UNSUPPORTED         Switching the BSP is not supported.
  @retval EFI_SUCCESS             The calling processor is an AP.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the current BSP or
                                  a disabled AP.
  @retval EFI_NOT_READY           The specified AP is busy.

**/
typedef
EFI_STATUS
(EFIAPI * EFI_SMM_SWITCH_BSP) (
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL *This,
  IN       UINTN                        ProcessorNumber
  );

/**
  Notify that a new processor has been added to the system.

  The SMM CPU driver should add the processor to the SMM CPU list.

  If the processor is disabled it won't participate any SMI handler during subsequent SMIs.

  @param  This                      A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param  ProcessorId               The hardware ID of the processor.
  @param  ProcessorNumber           The handle number of processor.
  @param  ProcessorResource         A pointer to EFI_SMM_PROCESSOR_RESOURCE which holds the assigned resources.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_ALREADY_STARTED  Processor already present.
  @retval EFI_NOT_READY        Space for a new handle could not be allocated.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_ADD_PROCESSOR) (
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL  *This,
  IN       UINT64                        ProcessorId,
  OUT      UINTN                         *ProcessorNumber
  );

/**
  Notify that a processor is hot-removed.

  Remove a processor from the CPU list of the SMM CPU driver. After this API is called, the removed processor
  must not respond to SMIs in the coherence domain.

  @param  This                 A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param  ProcessorId          The hardware ID of the processor.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_NOT_FOUND        Processor with the hardware ID specified by ProcessorId does not exist.
  @retval EFI_NOT_READY        Specified AP is busy.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_REMOVE_PROCESSOR) (
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL  *This,
  IN       UINTN                         ProcessorNumber
  );

/**
  This return the handle number for the calling processor.  This service may be
  called from the BSP and APs.

  This service returns the processor handle number for the calling processor.
  The returned value is in the range from 0 to the total number of logical
  processors minus 1. This service may be called from the BSP and APs.
  If ProcessorNumber is NULL, then EFI_INVALID_PARAMETER
  is returned. Otherwise, the current processors handle number is returned in
  ProcessorNumber, and EFI_SUCCESS is returned.

  @param[in] This              A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[in] ProcessorNumber   The handle number of AP that is to become the new
                               BSP. The range is from 0 to the total number of
                               logical processors minus 1.

  @retval EFI_SUCCESS             The current processor handle number was returned
                                  in ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.

**/
typedef
EFI_STATUS
(EFIAPI * EFI_SMM_WHOAMI) (
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL *This,
  OUT      UINTN                        *ProcessorNumber
  );

/**
  Register exception handler.

  @param  This                  A pointer to the SMM_CPU_SERVICE_PROTOCOL instance.
  @param  ExceptionType         Defines which interrupt or exception to hook. Type EFI_EXCEPTION_TYPE and
                                the valid values for this parameter are defined in EFI_DEBUG_SUPPORT_PROTOCOL
                                of the UEFI 2.0 specification.
  @param  InterruptHandler      A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER
                                that is called when a processor interrupt occurs.
                                If this parameter is NULL, then the handler will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_REGISTER_EXCEPTION_HANDLER) (
  IN EFI_SMM_CPU_SERVICE_PROTOCOL  *This,
  IN EFI_EXCEPTION_TYPE            ExceptionType,
  IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
  );

//
//  This protocol provides CPU services from SMM.
//
struct _EFI_SMM_CPU_SERVICE_PROTOCOL {
  EFI_SMM_GET_PROCESSOR_INFO          GetProcessorInfo;
  EFI_SMM_SWITCH_BSP                  SwitchBsp;
  EFI_SMM_ADD_PROCESSOR               AddProcessor;
  EFI_SMM_REMOVE_PROCESSOR            RemoveProcessor;
  EFI_SMM_WHOAMI                      WhoAmI;
  EFI_SMM_REGISTER_EXCEPTION_HANDLER  RegisterExceptionHandler;
};

extern EFI_GUID gEfiSmmCpuServiceProtocolGuid;

#endif
