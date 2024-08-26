/** @file

  Copyright (c) 2010 - 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  The source code contained or described herein and all documents related to the
  source code ("Material") are owned by Intel Corporation or its suppliers or
  licensors. Title to the Material remains with Intel Corporation or its suppliers
  and licensors. The Material may contain trade secrets and proprietary    and
  confidential information of Intel Corporation and its suppliers and licensors,
  and is protected by worldwide copyright and trade secret laws and treaty
  provisions. No part of the Material may be used, copied, reproduced, modified,
  published, uploaded, posted, transmitted, distributed, or disclosed in any way
  without Intel's prior express written permission.

  No license under any patent, copyright, trade secret or other intellectual
  property right is granted to or conferred upon you by disclosure or delivery
  of the Materials, either expressly, by implication, inducement, estoppel or
  otherwise. Any license under such intellectual property rights must be
  express and approved by Intel in writing.

  Unless otherwise agreed by Intel in writing, you may not remove or alter
  this notice or any other notice embedded in Materials by Intel or
  Intel's suppliers or licensors in any way.
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>
#include <Guid/SmiHandlerProfile.h>
#include <Protocol/MmSxDispatch.h>
#include <Guid/EndOfS3Resume.h>
#include <Protocol/SmmEndOfDxe.h>
#include <Guid/AcpiS3Context.h>
#include <Guid/AcpiS3Enable.h>

BOOLEAN  mDuringS3Resume = FALSE;

/**
  Main entry point for an SMM handler dispatch or communicate-based callback.
  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     Context         Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in,out] CommBuffer      A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in,out] CommBufferSize  The size of the CommBuffer.
  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.
**/
EFI_STATUS
EFIAPI
SmmS3EntryCallBack (
  IN           EFI_HANDLE  DispatchHandle,
  IN     CONST VOID        *Context         OPTIONAL,
  IN OUT       VOID        *CommBuffer      OPTIONAL,
  IN OUT       UINTN       *CommBufferSize  OPTIONAL
  )
{
  mDuringS3Resume = TRUE;
  return EFI_SUCCESS;
}

/**
  Software SMI handler that is called when the EndOfS3Resume signal is triggered.
  This function installs the SMM EndOfS3Resume Protocol so SMM Drivers are informed that
  S3 resume has finished.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmEndOfS3ResumeHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  SmmHandle;

  DEBUG ((DEBUG_INFO, "SmmEndOfS3ResumeHandler\n"));
  if (!mDuringS3Resume) {
    DEBUG ((DEBUG_ERROR, "It is not during S3 resume\n"));
    return EFI_SUCCESS;
  }

  //
  // Install SMM EndOfS3Resume protocol
  //
  SmmHandle = NULL;
  Status    =  gMmst->MmInstallProtocolInterface (
                        &SmmHandle,
                        &gEdkiiEndOfS3ResumeGuid,
                        EFI_NATIVE_INTERFACE,
                        NULL
                        );
  ASSERT_EFI_ERROR (Status);
  //
  // Uninstall the protocol here because the consumer just hook the
  // installation event.
  //
  Status = gMmst->MmUninstallProtocolInterface (
                    SmmHandle,
                    &gEdkiiEndOfS3ResumeGuid,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);
  mDuringS3Resume = FALSE;
  return Status;
}

/**
  The module Entry Point of driver.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the MM System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
S3ResumeStandaloneMmEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_SMM_SX_DISPATCH2_PROTOCOL  *SxDispatch;
  EFI_SMM_SX_REGISTER_CONTEXT    EntryRegisterContext;
  EFI_HANDLE                     S3EntryHandle;
  VOID                           *Registration;
  EFI_HOB_GUID_TYPE              *GuidHob;
  ACPI_S3_ENABLE                 *AcpiS3EnableHob;

  AcpiS3EnableHob = NULL;
  // Attempt to retrieve the ACPI_S3_ENABLE_HOB
  GuidHob         = GetFirstGuidHob (&gEdkiiAcpiS3EnableHobGuid);
  AcpiS3EnableHob = (GuidHob != NULL) ? GET_GUID_HOB_DATA (GuidHob) : NULL;

  // Validate the existence of AcpiS3EnableHob
  ASSERT (AcpiS3EnableHob != NULL);

  // Check if ACPI S3 is enabled
  if (AcpiS3EnableHob->AcpiS3Enable) {
    // Locate the SmmSxDispatch2 protocol
    Status = gMmst->MmLocateProtocol (&gEfiSmmSxDispatch2ProtocolGuid, NULL, (VOID **)&SxDispatch);
    if (!EFI_ERROR (Status) && (SxDispatch != NULL)) {
      EntryRegisterContext.Type  = SxS3;
      EntryRegisterContext.Phase = SxEntry;
      Status                     = SxDispatch->Register (
                                                 SxDispatch,
                                                 SmmS3EntryCallBack,
                                                 &EntryRegisterContext,
                                                 &S3EntryHandle
                                                 );
      ASSERT_EFI_ERROR (Status);
    }

    // Register S3 related SMI Handlers
    Status = gMmst->MmiHandlerRegister (SmmEndOfS3ResumeHandler, &gEdkiiEndOfS3ResumeGuid, NULL);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
