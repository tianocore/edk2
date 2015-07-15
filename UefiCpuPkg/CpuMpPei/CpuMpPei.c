/** @file
  CPU PEI Module installs CPU Multiple Processor PPI.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuMpPei.h"

//
// Global Descriptor Table (GDT)
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_GDT mGdtEntries[] = {
/* selector { Global Segment Descriptor                              } */
/* 0x00 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}}, //null descriptor
/* 0x08 */  {{0xffff, 0,  0,  0x2,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //linear data segment descriptor
/* 0x10 */  {{0xffff, 0,  0,  0xf,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //linear code segment descriptor
/* 0x18 */  {{0xffff, 0,  0,  0x3,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //system data segment descriptor
/* 0x20 */  {{0xffff, 0,  0,  0xa,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //system code segment descriptor
/* 0x28 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}}, //spare segment descriptor
/* 0x30 */  {{0xffff, 0,  0,  0x2,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //system data segment descriptor
/* 0x38 */  {{0xffff, 0,  0,  0xa,  1,  0,  1,  0xf,  0,  1, 0,  1,  0}}, //system code segment descriptor
/* 0x40 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}}, //spare segment descriptor
};

//
// IA32 Gdt register
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_DESCRIPTOR mGdt = {
  sizeof (mGdtEntries) - 1,
  (UINTN) mGdtEntries
  };

/**
  Get available system memory below 1MB by specified size.

  @param  WakeupBufferSize   Wakeup buffer size required

  @retval other   Return wakeup buffer address below 1MB.
  @retval -1      Cannot find free memory below 1MB.
**/
UINTN
GetWakeupBuffer (
  IN UINTN                WakeupBufferSize
  )
{
  EFI_PEI_HOB_POINTERS    Hob;
  UINTN                   WakeupBufferStart;
  UINTN                   WakeupBufferEnd;

  //
  // Get the HOB list for processing
  //
  Hob.Raw = GetHobList ();

  //
  // Collect memory ranges
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if ((Hob.ResourceDescriptor->PhysicalStart < BASE_1MB) &&
          (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) &&
          ((Hob.ResourceDescriptor->ResourceAttribute &
            (EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED |
             EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED |
             EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED
             )) == 0)
           ) {
        //
        // Need memory under 1MB to be collected here
        //
        WakeupBufferEnd = (UINTN) (Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength);
        if (WakeupBufferEnd > BASE_1MB) {
          //
          // Wakeup buffer should be under 1MB
          //
          WakeupBufferEnd = BASE_1MB;
        }
        //
        // Wakeup buffer should be aligned on 4KB
        //
        WakeupBufferStart = (WakeupBufferEnd - WakeupBufferSize) & ~(SIZE_4KB - 1);
        if (WakeupBufferStart < Hob.ResourceDescriptor->PhysicalStart) {
          continue;
        }
        //
        // Create a memory allocation HOB.
        //
        BuildMemoryAllocationHob (
          WakeupBufferStart,
          WakeupBufferSize,
          EfiBootServicesData
          );
        return WakeupBufferStart;
      }
    }
    //
    // Find the next HOB
    //
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return (UINTN) -1;
}

/**
  Get available system memory below 1MB by specified size.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data
**/
VOID
BackupAndPrepareWakeupBuffer(
  IN PEI_CPU_MP_DATA         *PeiCpuMpData
  )
{
  CopyMem (
    (VOID *) PeiCpuMpData->BackupBuffer,
    (VOID *) PeiCpuMpData->WakeupBuffer,
    PeiCpuMpData->BackupBufferSize
    );
  CopyMem (
    (VOID *) PeiCpuMpData->WakeupBuffer,
    (VOID *) PeiCpuMpData->AddressMap.RendezvousFunnelAddress,
    PeiCpuMpData->AddressMap.RendezvousFunnelSize
    );
}
/**
  Prepare for AP wakeup buffer and copy AP reset code into it.

  Get wakeup buffer below 1MB. Allocate memory for CPU MP Data and APs Stack.

  @return   Pointer to PEI CPU MP Data
**/
PEI_CPU_MP_DATA *
PrepareAPStartupVector (
  VOID
  )
{
  EFI_STATUS                    Status;
  UINT32                        MaxCpuCount;
  PEI_CPU_MP_DATA               *PeiCpuMpData;
  EFI_PHYSICAL_ADDRESS          Buffer;
  UINTN                         BufferSize;
  UINTN                         WakeupBuffer;
  UINTN                         WakeupBufferSize;
  MP_ASSEMBLY_ADDRESS_MAP       AddressMap;

  AsmGetAddressMap (&AddressMap);
  WakeupBufferSize = AddressMap.RendezvousFunnelSize + sizeof (MP_CPU_EXCHANGE_INFO);
  WakeupBuffer     = GetWakeupBuffer ((WakeupBufferSize + SIZE_4KB - 1) & ~(SIZE_4KB - 1));
  DEBUG ((EFI_D_INFO, "CpuMpPei: WakeupBuffer = 0x%x\n", WakeupBuffer));

  //
  // Allocate Pages for APs stack, CPU MP Data and backup buffer for wakeup buffer
  //
  MaxCpuCount = PcdGet32(PcdCpuMaxLogicalProcessorNumber);
  BufferSize  = PcdGet32 (PcdCpuApStackSize) * MaxCpuCount + sizeof (PEI_CPU_MP_DATA)
                  + WakeupBufferSize + sizeof (PEI_CPU_DATA) * MaxCpuCount;
  Status = PeiServicesAllocatePages (
             EfiBootServicesData,
             EFI_SIZE_TO_PAGES (BufferSize),
             &Buffer
             );
  ASSERT_EFI_ERROR (Status);

  PeiCpuMpData = (PEI_CPU_MP_DATA *) (UINTN) (Buffer + PcdGet32 (PcdCpuApStackSize) * MaxCpuCount);
  PeiCpuMpData->Buffer            = (UINTN) Buffer;
  PeiCpuMpData->CpuApStackSize    = PcdGet32 (PcdCpuApStackSize);
  PeiCpuMpData->WakeupBuffer      = WakeupBuffer;
  PeiCpuMpData->BackupBuffer      = (UINTN)PeiCpuMpData + sizeof (PEI_CPU_MP_DATA);
  PeiCpuMpData->BackupBufferSize  = WakeupBufferSize;
  PeiCpuMpData->MpCpuExchangeInfo = (MP_CPU_EXCHANGE_INFO *) (UINTN) (WakeupBuffer + AddressMap.RendezvousFunnelSize);

  PeiCpuMpData->CpuCount                 = 1;
  PeiCpuMpData->BspNumber                = 0;
  PeiCpuMpData->CpuData                  = (PEI_CPU_DATA *) (PeiCpuMpData->MpCpuExchangeInfo + 1);
  PeiCpuMpData->CpuData[0].ApicId        = GetInitialApicId ();
  PeiCpuMpData->CpuData[0].Health.Uint32 = 0;
  CopyMem (&PeiCpuMpData->AddressMap, &AddressMap, sizeof (MP_ASSEMBLY_ADDRESS_MAP));

  //
  // Backup original data and copy AP reset code in it
  //
  BackupAndPrepareWakeupBuffer(PeiCpuMpData);

  return PeiCpuMpData;
}
/**
  The Entry point of the MP CPU PEIM.

  This function will wakeup APs and collect CPU AP count and install the
  Mp Service Ppi.

  @param  FileHandle    Handle of the file being invoked.
  @param  PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   MpServicePpi is installed successfully.

**/
EFI_STATUS
EFIAPI
CpuMpPeimInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{

  PEI_CPU_MP_DATA      *PeiCpuMpData;

  //
  // Load new GDT table on BSP
  //
  AsmInitializeGdt (&mGdt);
  //
  // Get wakeup buffer and copy AP reset code in it
  //
  PeiCpuMpData = PrepareAPStartupVector ();

  return EFI_SUCCESS;
}
