/** @file
A driver allocates common SMM communication buffer in EfiReservedMemoryType.

This driver allocates common SMM communication buffer in EfiReservedMemoryType,
then it publishes the information to EFI configuration table with
gEdkiiPiSmmCommunicationRegionTableGuid.
Any other driver or application can get the table and know the common
communication buffer.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/UefiLib.h>
#include <Guid/MmCommBuffer.h>
#include <Guid/PiSmmCommunicationRegionTable.h>

#define DEFAULT_COMMON_PI_SMM_COMMUNIATION_REGION_PAGES  4

/**
  Entry Point for SMM communication buffer driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCEESS
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
SmmCommunicationBufferEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                               Status;
  UINT32                                   DescriptorSize;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE  *PiSmmCommunicationRegionTable;
  EFI_MEMORY_DESCRIPTOR                    *Entry;
  EFI_HOB_GUID_TYPE                        *GuidHob;
  MM_COMM_BUFFER                           *MmCommBuffer;

  DescriptorSize = sizeof (EFI_MEMORY_DESCRIPTOR);

  //
  // Make sure Size != sizeof(EFI_MEMORY_DESCRIPTOR). This will
  // prevent people from having pointer math bugs in their code.
  // now you have to use *DescriptorSize to make things work.
  //
  DescriptorSize += sizeof (UINT64) - (DescriptorSize % sizeof (UINT64));

  //
  // Allocate and fill PiSmmCommunicationRegionTable
  //
  PiSmmCommunicationRegionTable = AllocateReservedPool (sizeof (EDKII_PI_SMM_COMMUNICATION_REGION_TABLE) + DescriptorSize);
  ASSERT (PiSmmCommunicationRegionTable != NULL);
  ZeroMem (PiSmmCommunicationRegionTable, sizeof (EDKII_PI_SMM_COMMUNICATION_REGION_TABLE) + DescriptorSize);

  PiSmmCommunicationRegionTable->Version         = EDKII_PI_SMM_COMMUNICATION_REGION_TABLE_VERSION;
  PiSmmCommunicationRegionTable->NumberOfEntries = 1;
  PiSmmCommunicationRegionTable->DescriptorSize  = DescriptorSize;
  Entry                                          = (EFI_MEMORY_DESCRIPTOR *)(PiSmmCommunicationRegionTable + 1);
  Entry->Type                                    = EfiConventionalMemory;

  GuidHob = GetFirstGuidHob (&gMmCommBufferHobGuid);

  if (GuidHob == NULL) {
    Entry->PhysicalStart = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateReservedPages (DEFAULT_COMMON_PI_SMM_COMMUNIATION_REGION_PAGES);
    Entry->NumberOfPages = DEFAULT_COMMON_PI_SMM_COMMUNIATION_REGION_PAGES;
  } else {
    MmCommBuffer         = GET_GUID_HOB_DATA (GuidHob);
    Entry->PhysicalStart = MmCommBuffer->PhysicalStart;
    Entry->NumberOfPages = MmCommBuffer->NumberOfPages;
  }

  ASSERT (Entry->PhysicalStart != 0);
  Entry->VirtualStart = 0;
  Entry->Attribute    = 0;

  DEBUG ((DEBUG_INFO, "PiSmmCommunicationRegionTable:(0x%x)\n", PiSmmCommunicationRegionTable));
  DEBUG ((DEBUG_INFO, "  Version         - 0x%x\n", PiSmmCommunicationRegionTable->Version));
  DEBUG ((DEBUG_INFO, "  NumberOfEntries - 0x%x\n", PiSmmCommunicationRegionTable->NumberOfEntries));
  DEBUG ((DEBUG_INFO, "  DescriptorSize  - 0x%x\n", PiSmmCommunicationRegionTable->DescriptorSize));
  DEBUG ((DEBUG_INFO, "Entry:(0x%x)\n", Entry));
  DEBUG ((DEBUG_INFO, "  Type            - 0x%x\n", Entry->Type));
  DEBUG ((DEBUG_INFO, "  PhysicalStart   - 0x%lx\n", Entry->PhysicalStart));
  DEBUG ((DEBUG_INFO, "  VirtualStart    - 0x%lx\n", Entry->VirtualStart));
  DEBUG ((DEBUG_INFO, "  NumberOfPages   - 0x%lx\n", Entry->NumberOfPages));
  DEBUG ((DEBUG_INFO, "  Attribute       - 0x%lx\n", Entry->Attribute));

  //
  // Publish this table, so that other driver can use the buffer.
  //
  Status = gBS->InstallConfigurationTable (&gEdkiiPiSmmCommunicationRegionTableGuid, PiSmmCommunicationRegionTable);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
