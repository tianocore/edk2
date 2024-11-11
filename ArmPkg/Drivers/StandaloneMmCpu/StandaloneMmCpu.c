/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2016 HP Development Company, L.P.
  Copyright (c) 2016 - 2024, Arm Limited. All rights reserved.
  Copyright (c) 2023, Ventana Micro System Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Pi/PiMmCis.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>

#include <Protocol/DebugSupport.h> // for EFI_SYSTEM_CONTEXT

#include <Guid/ZeroGuid.h>
#include <Guid/MmramMemoryReserve.h>

#include <StandaloneMmCpu.h>

//
// mPiMmCpuDriverEpProtocol for Cpu driver entry point to handle
// mm communication event.
//
extern EDKII_PI_MM_CPU_DRIVER_EP_PROTOCOL  mPiMmCpuDriverEpProtocol;

//
// Private copy of the MM system table for future use
//
EFI_MM_SYSTEM_TABLE  *mMmst = NULL;

//
// Globals used to initialize the protocol
//
STATIC EFI_HANDLE  mMmCpuHandle = NULL;

/** Entry point for the Standalone MM CPU driver.

  @param  [in]  ImageHandle   Unused. Not actual image handle.
  @param  [in]  SystemTable   Pointer to MM System table.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
  @retval  EFI_NOT_FOUND          Failed to find the HOB for the CPU
                                  driver endpoint descriptor.
**/
EFI_STATUS
StandaloneMmCpuInitialize (
  IN EFI_HANDLE           ImageHandle,   // not actual imagehandle
  IN EFI_MM_SYSTEM_TABLE  *SystemTable   // not actual systemtable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DispatchHandle;

  ASSERT (SystemTable != NULL);
  mMmst = SystemTable;

  // publish the MM config protocol so the MM core can register its entry point
  Status = mMmst->MmInstallProtocolInterface (
                    &mMmCpuHandle,
                    &gEfiMmConfigurationProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mMmConfig
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Install  entry point of this CPU driver to allow
  // the entry point driver to be invoked upon receipt of an event in
  // DelegatedEventLoop.
  Status = mMmst->MmInstallProtocolInterface (
                    &mMmCpuHandle,
                    &gEdkiiPiMmCpuDriverEpProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mPiMmCpuDriverEpProtocol
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // register the root MMI handler
  Status = mMmst->MmiHandlerRegister (
                    PiMmCpuTpFwRootMmiHandler,
                    NULL,
                    &DispatchHandle
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}
