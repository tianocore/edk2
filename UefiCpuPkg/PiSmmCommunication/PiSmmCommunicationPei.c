/** @file
PiSmmCommunication PEI Driver.

Copyright (c) 2010 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <PiDxe.h>
#include <PiSmm.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Protocol/SmmCommunication.h>
#include <Ppi/SmmCommunication.h>
#include <Ppi/SmmAccess.h>
#include <Ppi/SmmControl.h>
#include <Guid/AcpiS3Context.h>

#include "PiSmmCommunicationPrivate.h"

/**
  the whole picture is below:

  +----------------------------------+
  | ACPI_VARIABLE_HOB                |
  |   SmramDescriptor                | <- DRAM
  |     CpuStart                     |---
  +----------------------------------+   |
                                         |
  +----------------------------------+<--
  | SMM_S3_RESUME_STATE              |
  |   Signature                      | <- SMRAM
  |   Smst                           |---
  +----------------------------------+   |
                                         |
  +----------------------------------+<--
  | EFI_SMM_SYSTEM_TABLE2            |
  |   NumberOfTableEntries           | <- SMRAM
  |   SmmConfigurationTable          |---
  +----------------------------------+   |
                                         |
  +----------------------------------+<--
  | EFI_SMM_COMMUNICATION_CONTEXT    |
  |   SwSmiNumber                    | <- SMRAM
  |   BufferPtrAddress               |---
  +----------------------------------+   |
                                         |
  +----------------------------------+<--
  | Communication Buffer Pointer     | <- AcpiNvs
  +----------------------------------+---
                                         |
  +----------------------------------+<--
  | EFI_SMM_COMMUNICATE_HEADER       |
  |   HeaderGuid                     | <- DRAM
  |   MessageLength                  |
  +----------------------------------+

**/

#if defined (MDE_CPU_IA32)
typedef struct {
  EFI_TABLE_HEADER    Hdr;
  UINT64              SmmFirmwareVendor;
  UINT64              SmmFirmwareRevision;
  UINT64              SmmInstallConfigurationTable;
  UINT64              SmmIoMemRead;
  UINT64              SmmIoMemWrite;
  UINT64              SmmIoIoRead;
  UINT64              SmmIoIoWrite;
  UINT64              SmmAllocatePool;
  UINT64              SmmFreePool;
  UINT64              SmmAllocatePages;
  UINT64              SmmFreePages;
  UINT64              SmmStartupThisAp;
  UINT64              CurrentlyExecutingCpu;
  UINT64              NumberOfCpus;
  UINT64              CpuSaveStateSize;
  UINT64              CpuSaveState;
  UINT64              NumberOfTableEntries;
  UINT64              SmmConfigurationTable;
} EFI_SMM_SYSTEM_TABLE2_64;

typedef struct {
  EFI_GUID    VendorGuid;
  UINT64      VendorTable;
} EFI_CONFIGURATION_TABLE64;
#endif

#if defined (MDE_CPU_X64)
typedef EFI_SMM_SYSTEM_TABLE2    EFI_SMM_SYSTEM_TABLE2_64;
typedef EFI_CONFIGURATION_TABLE  EFI_CONFIGURATION_TABLE64;
#endif

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_SMM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize       The size of the data buffer being passed in.On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_NOT_STARTED        The service is NOT started.
**/
EFI_STATUS
EFIAPI
Communicate (
  IN CONST EFI_PEI_SMM_COMMUNICATION_PPI  *This,
  IN OUT VOID                             *CommBuffer,
  IN OUT UINTN                            *CommSize
  );

EFI_PEI_SMM_COMMUNICATION_PPI  mSmmCommunicationPpi = { Communicate };

EFI_PEI_PPI_DESCRIPTOR  mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiSmmCommunicationPpiGuid,
  &mSmmCommunicationPpi
};

/**
  Get SMM communication context.

  @return SMM communication context.
**/
EFI_SMM_COMMUNICATION_CONTEXT *
GetCommunicationContext (
  VOID
  )
{
  EFI_HOB_GUID_TYPE              *GuidHob;
  EFI_SMM_COMMUNICATION_CONTEXT  *SmmCommunicationContext;

  GuidHob = GetFirstGuidHob (&gEfiPeiSmmCommunicationPpiGuid);
  ASSERT (GuidHob != NULL);

  SmmCommunicationContext = (EFI_SMM_COMMUNICATION_CONTEXT *)GET_GUID_HOB_DATA (GuidHob);

  return SmmCommunicationContext;
}

/**
  Set SMM communication context.

  @param SmmCommunicationContext SMM communication context.
**/
VOID
SetCommunicationContext (
  IN EFI_SMM_COMMUNICATION_CONTEXT  *SmmCommunicationContext
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 BufferSize;

  BufferSize = sizeof (*SmmCommunicationContext);
  Hob.Raw    = BuildGuidHob (
                 &gEfiPeiSmmCommunicationPpiGuid,
                 BufferSize
                 );
  ASSERT (Hob.Raw);

  CopyMem ((VOID *)Hob.Raw, SmmCommunicationContext, sizeof (*SmmCommunicationContext));
}

/**
  Get VendorTable by VendorGuid in Smst.

  @param Signature  Signature of SMM_S3_RESUME_STATE
  @param Smst       SMM system table
  @param VendorGuid vendor guid

  @return vendor table.
**/
VOID *
InternalSmstGetVendorTableByGuid (
  IN UINT64                 Signature,
  IN EFI_SMM_SYSTEM_TABLE2  *Smst,
  IN EFI_GUID               *VendorGuid
  )
{
  EFI_CONFIGURATION_TABLE    *SmmConfigurationTable;
  UINTN                      NumberOfTableEntries;
  UINTN                      Index;
  EFI_SMM_SYSTEM_TABLE2_64   *Smst64;
  EFI_CONFIGURATION_TABLE64  *SmmConfigurationTable64;

  if ((sizeof (UINTN) == sizeof (UINT32)) && (Signature == SMM_S3_RESUME_SMM_64)) {
    //
    // 32 PEI + 64 DXE
    //
    Smst64 = (EFI_SMM_SYSTEM_TABLE2_64 *)Smst;
    DEBUG ((DEBUG_INFO, "InitCommunicationContext - SmmConfigurationTable: %x\n", Smst64->SmmConfigurationTable));
    DEBUG ((DEBUG_INFO, "InitCommunicationContext - NumberOfTableEntries: %x\n", Smst64->NumberOfTableEntries));
    SmmConfigurationTable64 = (EFI_CONFIGURATION_TABLE64 *)(UINTN)Smst64->SmmConfigurationTable;
    NumberOfTableEntries    = (UINTN)Smst64->NumberOfTableEntries;
    for (Index = 0; Index < NumberOfTableEntries; Index++) {
      if (CompareGuid (&SmmConfigurationTable64[Index].VendorGuid, VendorGuid)) {
        return (VOID *)(UINTN)SmmConfigurationTable64[Index].VendorTable;
      }
    }

    return NULL;
  } else {
    DEBUG ((DEBUG_INFO, "InitCommunicationContext - SmmConfigurationTable: %x\n", Smst->SmmConfigurationTable));
    DEBUG ((DEBUG_INFO, "InitCommunicationContext - NumberOfTableEntries: %x\n", Smst->NumberOfTableEntries));
    SmmConfigurationTable = Smst->SmmConfigurationTable;
    NumberOfTableEntries  = Smst->NumberOfTableEntries;
    for (Index = 0; Index < NumberOfTableEntries; Index++) {
      if (CompareGuid (&SmmConfigurationTable[Index].VendorGuid, VendorGuid)) {
        return (VOID *)SmmConfigurationTable[Index].VendorTable;
      }
    }

    return NULL;
  }
}

/**
  Init SMM communication context.
**/
VOID
InitCommunicationContext (
  VOID
  )
{
  EFI_SMRAM_DESCRIPTOR           *SmramDescriptor;
  SMM_S3_RESUME_STATE            *SmmS3ResumeState;
  VOID                           *GuidHob;
  EFI_SMM_COMMUNICATION_CONTEXT  *SmmCommunicationContext;

  GuidHob = GetFirstGuidHob (&gEfiAcpiVariableGuid);
  ASSERT (GuidHob != NULL);
  SmramDescriptor  = (EFI_SMRAM_DESCRIPTOR *)GET_GUID_HOB_DATA (GuidHob);
  SmmS3ResumeState = (SMM_S3_RESUME_STATE *)(UINTN)SmramDescriptor->CpuStart;

  DEBUG ((DEBUG_INFO, "InitCommunicationContext - SmmS3ResumeState: %x\n", SmmS3ResumeState));
  DEBUG ((DEBUG_INFO, "InitCommunicationContext - Smst: %x\n", SmmS3ResumeState->Smst));

  SmmCommunicationContext = (EFI_SMM_COMMUNICATION_CONTEXT *)InternalSmstGetVendorTableByGuid (
                                                               SmmS3ResumeState->Signature,
                                                               (EFI_SMM_SYSTEM_TABLE2 *)(UINTN)SmmS3ResumeState->Smst,
                                                               &gEfiPeiSmmCommunicationPpiGuid
                                                               );
  ASSERT (SmmCommunicationContext != NULL);

  SetCommunicationContext (SmmCommunicationContext);

  return;
}

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_SMM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize       The size of the data buffer being passed in.On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_NOT_STARTED        The service is NOT started.
**/
EFI_STATUS
EFIAPI
Communicate (
  IN CONST EFI_PEI_SMM_COMMUNICATION_PPI  *This,
  IN OUT VOID                             *CommBuffer,
  IN OUT UINTN                            *CommSize
  )
{
  EFI_STATUS                     Status;
  PEI_SMM_CONTROL_PPI            *SmmControl;
  PEI_SMM_ACCESS_PPI             *SmmAccess;
  UINT8                          SmiCommand;
  UINTN                          Size;
  EFI_SMM_COMMUNICATION_CONTEXT  *SmmCommunicationContext;

  DEBUG ((DEBUG_INFO, "PiSmmCommunicationPei Communicate Enter\n"));

  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get needed resource
  //
  Status = PeiServicesLocatePpi (
             &gPeiSmmControlPpiGuid,
             0,
             NULL,
             (VOID **)&SmmControl
             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_STARTED;
  }

  Status = PeiServicesLocatePpi (
             &gPeiSmmAccessPpiGuid,
             0,
             NULL,
             (VOID **)&SmmAccess
             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_STARTED;
  }

  //
  // Check SMRAM locked, it should be done after SMRAM lock.
  //
  if (!SmmAccess->LockState) {
    DEBUG ((DEBUG_INFO, "PiSmmCommunicationPei LockState - %x\n", (UINTN)SmmAccess->LockState));
    return EFI_NOT_STARTED;
  }

  SmmCommunicationContext = GetCommunicationContext ();
  DEBUG ((DEBUG_INFO, "PiSmmCommunicationPei BufferPtrAddress - 0x%016lx, BufferPtr: 0x%016lx\n", SmmCommunicationContext->BufferPtrAddress, *(EFI_PHYSICAL_ADDRESS *)(UINTN)SmmCommunicationContext->BufferPtrAddress));

  //
  // No need to check if BufferPtr is 0, because it is in PEI phase.
  //
  *(EFI_PHYSICAL_ADDRESS *)(UINTN)SmmCommunicationContext->BufferPtrAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)CommBuffer;
  DEBUG ((DEBUG_INFO, "PiSmmCommunicationPei CommBuffer - %x\n", (UINTN)CommBuffer));

  //
  // Send command
  //
  SmiCommand = (UINT8)SmmCommunicationContext->SwSmiNumber;
  Size       = sizeof (SmiCommand);
  Status     = SmmControl->Trigger (
                             (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                             SmmControl,
                             (INT8 *)&SmiCommand,
                             &Size,
                             FALSE,
                             0
                             );
  ASSERT_EFI_ERROR (Status);

  //
  // Setting BufferPtr to 0 means this transaction is done.
  //
  *(EFI_PHYSICAL_ADDRESS *)(UINTN)SmmCommunicationContext->BufferPtrAddress = 0;

  DEBUG ((DEBUG_INFO, "PiSmmCommunicationPei Communicate Exit\n"));

  return EFI_SUCCESS;
}

/**
  Entry Point for PI SMM communication PEIM.

  @param  FileHandle              Handle of the file being invoked.
  @param  PeiServices             Pointer to PEI Services table.

  @retval EFI_SUCCESS
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
PiSmmCommunicationPeiEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS          Status;
  PEI_SMM_ACCESS_PPI  *SmmAccess;
  EFI_BOOT_MODE       BootMode;
  UINTN               Index;

  BootMode = GetBootModeHob ();
  if (BootMode != BOOT_ON_S3_RESUME) {
    return EFI_UNSUPPORTED;
  }

  Status = PeiServicesLocatePpi (
             &gPeiSmmAccessPpiGuid,
             0,
             NULL,
             (VOID **)&SmmAccess
             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_STARTED;
  }

  //
  // Check SMRAM locked, it should be done before SMRAM lock.
  //
  if (SmmAccess->LockState) {
    DEBUG ((DEBUG_INFO, "PiSmmCommunicationPei LockState - %x\n", (UINTN)SmmAccess->LockState));
    return EFI_ACCESS_DENIED;
  }

  //
  // Open all SMRAM
  //
  for (Index = 0; !EFI_ERROR (Status); Index++) {
    Status = SmmAccess->Open ((EFI_PEI_SERVICES **)GetPeiServicesTablePointer (), SmmAccess, Index);
  }

  InitCommunicationContext ();

  PeiServicesInstallPpi (&mPpiList);

  return RETURN_SUCCESS;
}
