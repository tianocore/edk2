/** @file
  Creates HOB during Standalone MM Foundation entry point
  on RISCV platforms.

Copyright (c) 2023, Ventana Micro System Inc. All rights reserved.<BR>
Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2025, Rivos Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <PiPei.h>
#include <Guid/MmramMemoryReserve.h>
#include <Guid/MpInformation.h>

#include <StandaloneMmCoreEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SerialPortLib.h>
#include <Guid/MmCommBuffer.h>

extern EFI_HOB_HANDOFF_INFO_TABLE *
HobConstructor (
  IN VOID   *EfiMemoryBegin,
  IN UINTN  EfiMemoryLength,
  IN VOID   *EfiFreeMemoryBottom,
  IN VOID   *EfiFreeMemoryTop
  );

/**
  Build communication buffer HOB.

  @return  EFI_STATUS

      +------------------------+
      |     Status             |
      |                        <----+
      +------------------------+    |
      |                        +----+
Page 0|    CommBuffer Struct   |
      +------------------------+
      |                        |
      |                        |
      |    NS Comm Buffer      |
Page 1|                        |
      +------------------------+

  Steal one page from allocated MMCommBuffer and use it
  for MM_COMM_BUFFER
*/

/**
  Build a communication buffer HOB for use by the MM environment.

  This function creates a GUIDed HOB that specifies the location and size of the
  communication buffer shared between the MM environment and the Normal world.
  Additionally, it initializes a buffer status structure to indicate readiness
  for communication.

  @param[in]  PayloadBootInfo  Pointer to the EFI_RISCV_SMM_PAYLOAD_INFO structure
                               containing the base address and size of the non-secure
                               communication buffer.

  @retval EFI_SUCCESS           The HOB was created successfully.
  @retval EFI_INVALID_PARAMETER The base address or size of the communication buffer
                                 in PayloadBootInfo is zero.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the HOB or buffer status.
**/
STATIC
EFI_STATUS
MmBuildCommBufferHob (
  IN       EFI_RISCV_SMM_PAYLOAD_INFO  *PayloadBootInfo
  )
{
  MM_COMM_BUFFER  *MmCommBuffer;

  if ((PayloadBootInfo->MmNsCommBufBase == 0) || (PayloadBootInfo->MmNsCommBufSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  // Create MMBuffer status HOB. Valid bit set in this status buffer tells
  // the MM that the buffer content is ready to be consumed.
  MmCommBuffer = BuildGuidHob (&gMmCommBufferHobGuid, sizeof (MM_COMM_BUFFER));
  if (MmCommBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Allocate runtime memory for MM communication status parameters :
  // ReturnStatus, ReturnBufferSize, IsCommBufferValid
  //
  MmCommBuffer->PhysicalStart = PayloadBootInfo->MmNsCommBufBase + EFI_PAGE_SIZE;
  MmCommBuffer->NumberOfPages = EFI_SIZE_TO_PAGES (PayloadBootInfo->MmNsCommBufSize - EFI_PAGE_SIZE);
  MmCommBuffer->Status        = (EFI_PHYSICAL_ADDRESS)PayloadBootInfo->MmNsCommBufBase + sizeof (MM_COMM_BUFFER);
  return EFI_SUCCESS;
}

/**
  Populates the MP information HOB data structure with processor information.

  @param[out] MpInformationHobData  Pointer to the MP information HOB data structure
  @param[in]  PayloadBootInfo       Boot information containing processor details
**/
STATIC
VOID
PopulateMpInformation (
  IN OUT MP_INFORMATION_HOB_DATA     *MpInformationHobData,
  IN     EFI_RISCV_SMM_PAYLOAD_INFO  *PayloadBootInfo
  )
{
  EFI_PROCESSOR_INFORMATION  *ProcInfoBuffer;
  EFI_RISCV_SMM_CPU_INFO     *CpuInfo;
  UINT32                     Index;
  UINT32                     Flags;

  MpInformationHobData->NumberOfProcessors        = PayloadBootInfo->NumCpus;
  MpInformationHobData->NumberOfEnabledProcessors = PayloadBootInfo->NumCpus;
  ProcInfoBuffer                                  = MpInformationHobData->ProcessorInfoBuffer;
  CpuInfo                                         = &(PayloadBootInfo->CpuInfo);

  for (Index = 0; Index < PayloadBootInfo->NumCpus; Index++) {
    ProcInfoBuffer[Index].ProcessorId      = CpuInfo[Index].ProcessorId;
    ProcInfoBuffer[Index].Location.Package = CpuInfo[Index].Package;
    ProcInfoBuffer[Index].Location.Core    = CpuInfo[Index].Core;
    ProcInfoBuffer[Index].Location.Thread  = 0;

    Flags = PROCESSOR_ENABLED_BIT | PROCESSOR_HEALTH_STATUS_BIT;
    if (CpuInfo[Index].Flags & CPU_INFO_FLAG_PRIMARY_CPU) {
      Flags |= PROCESSOR_AS_BSP_BIT;
    }

    ProcInfoBuffer[Index].StatusFlag = Flags;
  }
}

/**
  Populates the MMRAM ranges descriptor array with memory region information.

  @param[out] MmramRanges      Array of MMRAM descriptors to populate
  @param[in]  PayloadBootInfo  Boot information containing memory region details
  @param[in]  HobStart         Pointer to the HOB list

  @retval EFI_SUCCESS           MMRAM ranges were populated successfully
  @retval EFI_INVALID_PARAMETER Number of memory regions is not equal to expected number
**/
STATIC
EFI_STATUS
PopulateMmramRanges (
  IN OUT EFI_MMRAM_DESCRIPTOR        *MmramRanges,
  IN     EFI_RISCV_SMM_PAYLOAD_INFO  *PayloadBootInfo,
  IN     EFI_HOB_HANDOFF_INFO_TABLE  *HobStart
  )
{
  if (PayloadBootInfo->NumMmMemRegions != 5) {
    return EFI_INVALID_PARAMETER;
  }

  // MM Image region
  MmramRanges[0].PhysicalStart = PayloadBootInfo->MmImageBase;
  MmramRanges[0].CpuStart      = PayloadBootInfo->MmImageBase;
  MmramRanges[0].PhysicalSize  = PayloadBootInfo->MmImageSize;
  MmramRanges[0].RegionState   = EFI_CACHEABLE | EFI_ALLOCATED;

  // NS communication buffer
  MmramRanges[1].PhysicalStart = PayloadBootInfo->MmNsCommBufBase;
  MmramRanges[1].CpuStart      = PayloadBootInfo->MmNsCommBufBase;
  MmramRanges[1].PhysicalSize  = PayloadBootInfo->MmNsCommBufSize;
  MmramRanges[1].RegionState   = EFI_CACHEABLE | EFI_ALLOCATED;

  // CPU stacks
  MmramRanges[2].PhysicalStart = PayloadBootInfo->MmStackBase;
  MmramRanges[2].CpuStart      = PayloadBootInfo->MmStackBase;
  MmramRanges[2].PhysicalSize  = PayloadBootInfo->MmPcpuStackSize * PayloadBootInfo->NumCpus;
  MmramRanges[2].RegionState   = EFI_CACHEABLE | EFI_ALLOCATED;

  // Allocated heap
  MmramRanges[3].PhysicalStart = (EFI_PHYSICAL_ADDRESS)(UINTN)HobStart;
  MmramRanges[3].CpuStart      = (EFI_PHYSICAL_ADDRESS)(UINTN)HobStart;
  MmramRanges[3].PhysicalSize  = HobStart->EfiFreeMemoryBottom - (EFI_PHYSICAL_ADDRESS)(UINTN)HobStart;
  MmramRanges[3].RegionState   = EFI_CACHEABLE | EFI_ALLOCATED;

  // Free heap
  MmramRanges[4].PhysicalStart = HobStart->EfiFreeMemoryBottom;
  MmramRanges[4].CpuStart      = HobStart->EfiFreeMemoryBottom;
  MmramRanges[4].PhysicalSize  = HobStart->EfiFreeMemoryTop - HobStart->EfiFreeMemoryBottom;
  MmramRanges[4].RegionState   = EFI_CACHEABLE;

  return EFI_SUCCESS;
}

/**
  Creates and initializes a new HOB list using boot information passed by privileged firmware.

  @param[in, out] CpuDriverEntryPoint   Pointer to store the MM CPU driver entry point
  @param[in]      PayloadBootInfo       Boot information passed by privileged firmware

  @return Pointer to the created HOB list.
**/
VOID *
CreateHobListFromBootInfo (
  IN       EFI_RISCV_SMM_PAYLOAD_INFO  *PayloadBootInfo
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE      *HobStart;
  EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *MmramRangesHob;
  EFI_MMRAM_DESCRIPTOR            *MmramRanges;
  MP_INFORMATION_HOB_DATA         *MpInformationHobData;
  UINT32                          MpInfoBufferSize;
  UINT32                          MmramBufferSize;

  // Create hoblist with PHIT and EOH
  HobStart = HobConstructor (
               (VOID *)(UINTN)PayloadBootInfo->MmMemBase,
               (UINTN)PayloadBootInfo->MmMemLimit - PayloadBootInfo->MmMemBase,
               (VOID *)(UINTN)PayloadBootInfo->MmHeapBase,
               (VOID *)(UINTN)(PayloadBootInfo->MmHeapBase + PayloadBootInfo->MmHeapSize)
               );

  ASSERT (HobStart == (VOID *)(UINTN)PayloadBootInfo->MmHeapBase);

  // Build Boot Firmware Volume HOB
  BuildFvHob (PayloadBootInfo->MmImageBase, PayloadBootInfo->MmImageSize);

  // Create MP Information HOB
  MpInfoBufferSize = sizeof (MP_INFORMATION_HOB_DATA) +
                     (sizeof (EFI_PROCESSOR_INFORMATION) * PayloadBootInfo->NumCpus);

  MpInformationHobData = BuildGuidHob (&gMpInformationHobGuid, MpInfoBufferSize);
  if (MpInformationHobData != NULL) {
    PopulateMpInformation (MpInformationHobData, PayloadBootInfo);
  }

  // Build communication buffer HOB
  if (MmBuildCommBufferHob (PayloadBootInfo) != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Error building MmCommBufferHobs"));
  }

  // Create MMRAM ranges HOB
  MmramBufferSize = sizeof (EFI_MMRAM_HOB_DESCRIPTOR_BLOCK) +
                    (PayloadBootInfo->NumMmMemRegions * sizeof (EFI_MMRAM_DESCRIPTOR));

  MmramRangesHob = BuildGuidHob (&gEfiMmPeiMmramMemoryReserveGuid, MmramBufferSize);
  if (MmramRangesHob != NULL) {
    MmramRangesHob->NumberOfMmReservedRegions = PayloadBootInfo->NumMmMemRegions;
    MmramRanges                               = &MmramRangesHob->Descriptor[0];
    if (PopulateMmramRanges (MmramRanges, PayloadBootInfo, HobStart) != EFI_SUCCESS) {
      DEBUG ((DEBUG_ERROR, "Error populating MMRAM ranges\n"));
    }
  }

  return HobStart;
}
