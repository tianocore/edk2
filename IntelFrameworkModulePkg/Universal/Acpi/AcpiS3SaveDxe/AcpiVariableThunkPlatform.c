/** @file
  This is an implementation of the AcpiVariable platform field for ECP platform.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

==

typedef struct {
  EFI_PHYSICAL_ADDRESS  AcpiReservedMemoryBase;  <<===
  UINT32                AcpiReservedMemorySize;  <<===
  EFI_PHYSICAL_ADDRESS  S3ReservedLowMemoryBase;
  EFI_PHYSICAL_ADDRESS  AcpiBootScriptTable;
  EFI_PHYSICAL_ADDRESS  RuntimeScriptTableBase;
  EFI_PHYSICAL_ADDRESS  AcpiFacsTable;
  UINT64                SystemMemoryLength;      <<===
  ACPI_CPU_DATA_COMPATIBILITY         AcpiCpuData;
  EFI_PHYSICAL_ADDRESS  VideoOpromAddress;
  UINT32                VideoOpromSize;
  EFI_PHYSICAL_ADDRESS  S3DebugBufferAddress; 
  EFI_PHYSICAL_ADDRESS  S3ResumeNvsEntryPoint;    
} ACPI_VARIABLE_SET_COMPATIBILITY;

**/

#include <FrameworkDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Protocol/FrameworkMpService.h>
#include <Guid/AcpiVariableCompatibility.h>
#include <Guid/AcpiS3Context.h>

GLOBAL_REMOVE_IF_UNREFERENCED
ACPI_VARIABLE_SET_COMPATIBILITY               *mAcpiVariableSetCompatibility = NULL;

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param  Size         Size of memory to allocate.
  
  @return Allocated address for output.

**/
VOID*
AllocateAcpiNvsMemoryBelow4G (
  IN   UINTN   Size
  );

/**
  Hook point for AcpiVariableThunkPlatform for S3Ready.

  @param AcpiS3Context   ACPI s3 context
**/
VOID
S3ReadyThunkPlatform (
  IN ACPI_S3_CONTEXT      *AcpiS3Context
  )
{
  EFI_PHYSICAL_ADDRESS                          AcpiMemoryBase;
  UINT32                                        AcpiMemorySize;
  EFI_PEI_HOB_POINTERS                          Hob;
  UINT64                                        MemoryLength;

  DEBUG ((EFI_D_INFO, "S3ReadyThunkPlatform\n"));

  //
  // Allocate ACPI reserved memory under 4G
  //
  AcpiMemoryBase = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateAcpiNvsMemoryBelow4G (PcdGet32 (PcdS3AcpiReservedMemorySize));
  ASSERT (AcpiMemoryBase != 0);
  AcpiMemorySize = PcdGet32 (PcdS3AcpiReservedMemorySize);

  //
  // Calculate the system memory length by memory hobs
  //
  MemoryLength  = 0x100000;
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  ASSERT (Hob.Raw != NULL);
  while ((Hob.Raw != NULL) && (!END_OF_HOB_LIST (Hob))) {
    if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      //
      // Skip the memory region below 1MB
      //
      if (Hob.ResourceDescriptor->PhysicalStart >= 0x100000) {
        MemoryLength += Hob.ResourceDescriptor->ResourceLength;
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  mAcpiVariableSetCompatibility->AcpiReservedMemoryBase = AcpiMemoryBase;
  mAcpiVariableSetCompatibility->AcpiReservedMemorySize = AcpiMemorySize;
  mAcpiVariableSetCompatibility->SystemMemoryLength     = MemoryLength;

  DEBUG((EFI_D_INFO, "AcpiVariableThunkPlatform: AcpiMemoryBase is 0x%8x\n", mAcpiVariableSetCompatibility->AcpiReservedMemoryBase));
  DEBUG((EFI_D_INFO, "AcpiVariableThunkPlatform: AcpiMemorySize is 0x%8x\n", mAcpiVariableSetCompatibility->AcpiReservedMemorySize));
  DEBUG((EFI_D_INFO, "AcpiVariableThunkPlatform: SystemMemoryLength is 0x%8x\n", mAcpiVariableSetCompatibility->SystemMemoryLength));

  return ;
}

/**
  Hook point for AcpiVariableThunkPlatform for InstallAcpiS3Save.
**/
VOID
InstallAcpiS3SaveThunk (
  VOID
  )
{
  EFI_STATUS                           Status;
  FRAMEWORK_EFI_MP_SERVICES_PROTOCOL   *FrameworkMpService;
  UINTN                                VarSize;

  Status = gBS->LocateProtocol (
                  &gFrameworkEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID**) &FrameworkMpService
                  );
  if (!EFI_ERROR (Status)) {
    //
    // On ECP platform, if framework CPU drivers are in use, The compatible version of ACPI variable set 
    // should be produced by CPU driver. 
    //
    VarSize = sizeof (mAcpiVariableSetCompatibility);
    Status = gRT->GetVariable (
                    ACPI_GLOBAL_VARIABLE,
                    &gEfiAcpiVariableCompatiblityGuid,
                    NULL,
                    &VarSize,
                    &mAcpiVariableSetCompatibility
                    );
    ASSERT_EFI_ERROR (Status);
  } else {
    //
    // Allocate/initialize the compatible version of Acpi Variable Set since Framework chipset/platform 
    // driver need this variable
    //
    mAcpiVariableSetCompatibility = AllocateAcpiNvsMemoryBelow4G (sizeof(ACPI_VARIABLE_SET_COMPATIBILITY));
    Status = gRT->SetVariable (
                    ACPI_GLOBAL_VARIABLE,
                    &gEfiAcpiVariableCompatiblityGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof(mAcpiVariableSetCompatibility),
                    &mAcpiVariableSetCompatibility
                    );
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG((EFI_D_INFO, "AcpiVariableSetCompatibility is 0x%8x\n", mAcpiVariableSetCompatibility));
}
