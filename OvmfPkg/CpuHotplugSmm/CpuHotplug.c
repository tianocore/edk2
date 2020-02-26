/** @file
  Root SMI handler for VCPU hotplug SMIs.

  Copyright (c) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Q35MchIch9.h>     // ICH9_APM_CNT
#include <Library/BaseLib.h>                 // CpuDeadLoop()
#include <Library/DebugLib.h>                // ASSERT()
#include <Library/MmServicesTableLib.h>      // gMmst
#include <Library/PcdLib.h>                  // PcdGetBool()
#include <Protocol/MmCpuIo.h>                // EFI_MM_CPU_IO_PROTOCOL
#include <Uefi/UefiBaseType.h>               // EFI_STATUS

//
// We use this protocol for accessing IO Ports.
//
STATIC EFI_MM_CPU_IO_PROTOCOL *mMmCpuIo;
//
// Represents the registration of the CPU Hotplug MMI handler.
//
STATIC EFI_HANDLE mDispatchHandle;


/**
  CPU Hotplug MMI handler function.

  This is a root MMI handler.

  @param[in] DispatchHandle      The unique handle assigned to this handler by
                                 EFI_MM_SYSTEM_TABLE.MmiHandlerRegister().

  @param[in] Context             Context passed in by
                                 EFI_MM_SYSTEM_TABLE.MmiManage(). Due to
                                 CpuHotplugMmi() being a root MMI handler,
                                 Context is ASSERT()ed to be NULL.

  @param[in,out] CommBuffer      Ignored, due to CpuHotplugMmi() being a root
                                 MMI handler.

  @param[in,out] CommBufferSize  Ignored, due to CpuHotplugMmi() being a root
                                 MMI handler.

  @retval EFI_SUCCESS                       The MMI was handled and the MMI
                                            source was quiesced. When returned
                                            by a non-root MMI handler,
                                            EFI_SUCCESS terminates the
                                            processing of MMI handlers in
                                            EFI_MM_SYSTEM_TABLE.MmiManage().
                                            For a root MMI handler (i.e., for
                                            the present function too),
                                            EFI_SUCCESS behaves identically to
                                            EFI_WARN_INTERRUPT_SOURCE_QUIESCED,
                                            as further root MMI handlers are
                                            going to be called by
                                            EFI_MM_SYSTEM_TABLE.MmiManage()
                                            anyway.

  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The MMI source has been quiesced,
                                              but other handlers should still
                                              be called.

  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The MMI source is still pending,
                                              and other handlers should still
                                              be called.

  @retval EFI_INTERRUPT_PENDING               The MMI source could not be
                                              quiesced.
**/
STATIC
EFI_STATUS
EFIAPI
CpuHotplugMmi (
  IN EFI_HANDLE DispatchHandle,
  IN CONST VOID *Context        OPTIONAL,
  IN OUT VOID   *CommBuffer     OPTIONAL,
  IN OUT UINTN  *CommBufferSize OPTIONAL
  )
{
  EFI_STATUS Status;
  UINT8      ApmControl;

  //
  // Assert that we are entering this function due to our root MMI handler
  // registration.
  //
  ASSERT (DispatchHandle == mDispatchHandle);
  //
  // When MmiManage() is invoked to process root MMI handlers, the caller (the
  // MM Core) is expected to pass in a NULL Context. MmiManage() then passes
  // the same NULL Context to individual handlers.
  //
  ASSERT (Context == NULL);
  //
  // Read the MMI command value from the APM Control Port, to see if this is an
  // MMI we should care about.
  //
  Status = mMmCpuIo->Io.Read (mMmCpuIo, MM_IO_UINT8, ICH9_APM_CNT, 1,
                          &ApmControl);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to read ICH9_APM_CNT: %r\n", __FUNCTION__,
      Status));
    //
    // We couldn't even determine if the MMI was for us or not.
    //
    goto Fatal;
  }

  if (ApmControl != ICH9_APM_CNT_CPU_HOTPLUG) {
    //
    // The MMI is not for us.
    //
    return EFI_WARN_INTERRUPT_SOURCE_QUIESCED;
  }

  //
  // We've handled this MMI.
  //
  return EFI_SUCCESS;

Fatal:
  ASSERT (FALSE);
  CpuDeadLoop ();
  //
  // We couldn't handle this MMI.
  //
  return EFI_INTERRUPT_PENDING;
}


//
// Entry point function of this driver.
//
EFI_STATUS
EFIAPI
CpuHotplugEntry (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;

  //
  // This module should only be included when SMM support is required.
  //
  ASSERT (FeaturePcdGet (PcdSmmSmramRequire));
  //
  // This driver depends on the dynamically detected "SMRAM at default SMBASE"
  // feature.
  //
  if (!PcdGetBool (PcdQ35SmramAtDefaultSmbase)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Errors from here on are fatal; we cannot allow the boot to proceed if we
  // can't set up this driver to handle CPU hotplug.
  //
  // First, collect the protocols needed later. All of these protocols are
  // listed in our module DEPEX.
  //
  Status = gMmst->MmLocateProtocol (&gEfiMmCpuIoProtocolGuid,
                    NULL /* Registration */, (VOID **)&mMmCpuIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: locate MmCpuIo: %r\n", __FUNCTION__, Status));
    goto Fatal;
  }

  //
  // Register the handler for the CPU Hotplug MMI.
  //
  Status = gMmst->MmiHandlerRegister (
                    CpuHotplugMmi,
                    NULL,            // HandlerType: root MMI handler
                    &mDispatchHandle
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: MmiHandlerRegister(): %r\n", __FUNCTION__,
      Status));
    goto Fatal;
  }

  return EFI_SUCCESS;

Fatal:
  ASSERT (FALSE);
  CpuDeadLoop ();
  return Status;
}
