/** @file
PiSmmCommunication SMM Driver.

Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiSmm.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmMemLib.h>
#include <Library/PcdLib.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/AcpiTable.h>
#include <Ppi/SmmCommunication.h>
#include <Guid/Acpi.h>

#include "PiSmmCommunicationPrivate.h"

EFI_SMM_COMMUNICATION_CONTEXT  mSmmCommunicationContext = {
  SMM_COMMUNICATION_SIGNATURE
};

EFI_SMM_COMMUNICATION_ACPI_TABLE  mSmmCommunicationAcpiTable = {
  {
    {
      EFI_ACPI_4_0_UEFI_ACPI_DATA_TABLE_SIGNATURE,
      sizeof (EFI_SMM_COMMUNICATION_ACPI_TABLE),
      0x1,   // Revision
      0x0,   // Checksum
      {0x0}, // OemId[6]
      0x0,   // OemTableId
      0x0,   // OemRevision
      0x0,   // CreatorId
      0x0    // CreatorRevision
    },
    {0x0},                                                      // Identifier
    OFFSET_OF (EFI_SMM_COMMUNICATION_ACPI_TABLE, SwSmiNumber)   // DataOffset
  },
  0x0,                                                   // SwSmiNumber
  0x0                                                    // BufferPtrAddress
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
  EFI_ACPI_TABLE_PROTOCOL       *AcpiTableProtocol;
  UINTN                         TableKey;
  UINT64                        OemTableId;
  EFI_PHYSICAL_ADDRESS          *BufferPtrAddress;

  CopyMem (
    mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.OemId)
    );
  OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (&mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.OemTableId, &OemTableId, sizeof (UINT64));
  mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.OemRevision      = PcdGet32 (PcdAcpiDefaultOemRevision);
  mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.CreatorId        = PcdGet32 (PcdAcpiDefaultCreatorId);
  mSmmCommunicationAcpiTable.UefiAcpiDataTable.Header.CreatorRevision  = PcdGet32 (PcdAcpiDefaultCreatorRevision);

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

  //
  // Set ACPI table
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **) &AcpiTableProtocol);
  ASSERT_EFI_ERROR (Status);

  mSmmCommunicationAcpiTable.SwSmiNumber = (UINT32)SmmSwDispatchContext.SwSmiInputValue;
  BufferPtrAddress = AllocateAcpiNvsMemoryBelow4G (sizeof(EFI_PHYSICAL_ADDRESS));
  ASSERT (BufferPtrAddress != NULL);
  DEBUG ((EFI_D_INFO, "SmmCommunication BufferPtrAddress: 0x%016lx, BufferPtr: 0x%016lx\n", (EFI_PHYSICAL_ADDRESS)(UINTN)BufferPtrAddress, *BufferPtrAddress));
  mSmmCommunicationAcpiTable.BufferPtrAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)BufferPtrAddress;
  CopyMem (&mSmmCommunicationAcpiTable.UefiAcpiDataTable.Identifier, &gEfiSmmCommunicationProtocolGuid, sizeof(gEfiSmmCommunicationProtocolGuid));

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                &mSmmCommunicationAcpiTable,
                                sizeof(mSmmCommunicationAcpiTable),
                                &TableKey
                                );
  ASSERT_EFI_ERROR (Status);

  //
  // Save context
  //
  mSmmCommunicationContext.SwSmiNumber = (UINT32)SmmSwDispatchContext.SwSmiInputValue;
  mSmmCommunicationContext.BufferPtrAddress = mSmmCommunicationAcpiTable.BufferPtrAddress;
  SetCommunicationContext ();

  return Status;
}
