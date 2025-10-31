/** @file
  Private header with declarations and definitions specific to the MM Standalone
  CPU driver

  Copyright (c) 2017 - 2021, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_CPU_DRIVER_H_
#define MM_CPU_DRIVER_H_

#include <Protocol/MmCommunication2.h>
#include <Protocol/MmConfiguration.h>
#include <Protocol/MmCpu.h>
#include <Protocol/PiMmCpuDriverEp.h>

#include <Guid/MpInformation.h>

//
// CPU driver initialization specific declarations
//
extern EFI_MM_SYSTEM_TABLE  *mMmst;

//
// CPU State Save protocol specific declarations
//
extern EFI_MM_CPU_PROTOCOL  mMmCpuState;

//
// MM event handling specific declarations
//
extern EFI_MM_CONFIGURATION_PROTOCOL  mMmConfig;

/**
  The PI Standalone MM entry point for the CPU driver.

  @param  [in] MmHandlerContext     Arm specific Mm handler context.
  @param  [in] CommBufferAddr       Address of the communication buffer.

  @retval   EFI_SUCCESS             Success.
  @retval   EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval   EFI_ACCESS_DENIED       Access not permitted.
  @retval   EFI_OUT_OF_RESOURCES    Out of resources.
  @retval   EFI_UNSUPPORTED         Operation not supported.
**/
EFI_STATUS
PiMmStandaloneMmCpuDriverEntry (
  IN CONST ARM_MM_HANDLER_CONTEXT  *MmHandlerContext,
  IN UINTN                         CommBufferAddr
  );

/**
  This function is the main entry point for an MM handler dispatch
  or communicate-based callback.

  @param  DispatchHandle  The unique handle assigned to this handler by
                          MmiHandlerRegister().
  @param  Context         Points to an optional handler context which was
                          specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-MM environment into an
                          MM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
PiMmCpuTpFwRootMmiHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  );

#endif /* MM_CPU_DRIVER_H_ */
