/** @file
PiSmmCommunication SMM Driver.

Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmMemLib.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmCommunication.h>
#include <Ppi/SmmCommunication.h>

#include "PiSmmCommunicationPrivate.h"

EFI_SMM_COMMUNICATION_CONTEXT  mSmmCommunicationContext = {
  SMM_COMMUNICATION_SIGNATURE
};

/**
  Set SMM communication context.
**/
VOID
SetCommunicationContext (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gSmst->SmmInstallConfigurationTable (
                    gSmst,
                    &gEfiPeiSmmCommunicationPpiGuid,
                    &mSmmCommunicationContext,
                    sizeof(mSmmCommunicationContext)
                    );
  ASSERT_EFI_ERROR (Status);
}

/**
  Dispatch function for a Software SMI handler.

  @param DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param Context         Points to an optional handler context which was specified when the
                         handler was registered.
  @param CommBuffer      A pointer to a collection of data in memory that will
                         be conveyed from a non-SMM environment into an SMM environment.
  @param CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS Command is handled successfully.

**/
EFI_STATUS
EFIAPI
PiSmmCommunicationHandler (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context         OPTIONAL,
  IN OUT VOID    *CommBuffer      OPTIONAL,
  IN OUT UINTN   *CommBufferSize  OPTIONAL
  )
{
  UINTN                            CommSize;
  EFI_STATUS                       Status;
  EFI_SMM_COMMUNICATE_HEADER       *CommunicateHeader;
  EFI_PHYSICAL_ADDRESS             *BufferPtrAddress;

  DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler Enter\n"));

  BufferPtrAddress = (EFI_PHYSICAL_ADDRESS *)(UINTN)mSmmCommunicationContext.BufferPtrAddress;
  CommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *)(UINTN)*BufferPtrAddress;
  DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler CommunicateHeader - %x\n", CommunicateHeader));
  if (CommunicateHeader == NULL) {
    DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler is NULL, needn't to call dispatch function\n"));
    Status = EFI_SUCCESS;
  } else {
    if (!SmmIsBufferOutsideSmmValid ((UINTN)CommunicateHeader, OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data))) {
      DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler CommunicateHeader invalid - 0x%x\n", CommunicateHeader));
      Status = EFI_SUCCESS;
      goto Done;
    }

    CommSize = (UINTN)CommunicateHeader->MessageLength;
    if (!SmmIsBufferOutsideSmmValid ((UINTN)&CommunicateHeader->Data[0], CommSize)) {
      DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler CommunicateData invalid - 0x%x\n", &CommunicateHeader->Data[0]));
      Status = EFI_SUCCESS;
      goto Done;
    }

    //
    // Call dispatch function
    //
    DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler Data - %x\n", &CommunicateHeader->Data[0]));
    Status = gSmst->SmiManage (
                      &CommunicateHeader->HeaderGuid,
                      NULL,
                      &CommunicateHeader->Data[0],
                      &CommSize
                      );
  }

Done:
  DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler %r\n", Status));
  DEBUG ((EFI_D_INFO, "PiSmmCommunicationHandler Exit\n"));

  return (Status == EFI_SUCCESS) ? EFI_SUCCESS : EFI_INTERRUPT_PENDING;
}

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param  Size         Size of memory to allocate.

  @return Allocated address for output.

**/
VOID*
AllocateAcpiNvsMemoryBelow4G (
  IN   UINTN   Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;

  Pages = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIMemoryNVS,
                   Pages,
                   &Address
                   );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

/**
  Entry Point for PI SMM communication SMM driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCEESS
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
PiSmmCommunicationSmmEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL *SmmSwDispatch2;
  EFI_SMM_SW_REGISTER_CONTEXT   SmmSwDispatchContext;
  EFI_HANDLE                    DispatchHandle;
  EFI_PHYSICAL_ADDRESS          *BufferPtrAddress;

  //
  // Register software SMI handler
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    (VOID **)&SmmSwDispatch2
                    );
  ASSERT_EFI_ERROR (Status);

  SmmSwDispatchContext.SwSmiInputValue = (UINTN)-1;
  Status = SmmSwDispatch2->Register (
                             SmmSwDispatch2,
                             PiSmmCommunicationHandler,
                             &SmmSwDispatchContext,
                             &DispatchHandle
                             );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_INFO, "SmmCommunication SwSmi: %x\n", (UINTN)SmmSwDispatchContext.SwSmiInputValue));

  BufferPtrAddress = AllocateAcpiNvsMemoryBelow4G (sizeof(EFI_PHYSICAL_ADDRESS));
  ASSERT (BufferPtrAddress != NULL);
  DEBUG ((EFI_D_INFO, "SmmCommunication BufferPtrAddress: 0x%016lx, BufferPtr: 0x%016lx\n", (EFI_PHYSICAL_ADDRESS)(UINTN)BufferPtrAddress, *BufferPtrAddress));

  //
  // Save context
  //
  mSmmCommunicationContext.SwSmiNumber = (UINT32)SmmSwDispatchContext.SwSmiInputValue;
  mSmmCommunicationContext.BufferPtrAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)BufferPtrAddress;
  SetCommunicationContext ();

  return Status;
}
