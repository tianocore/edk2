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

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_NOTIFY_DESCRIPTOR mNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  CpuMpEndOfPeiCallback
};

/**
  Sort the APIC ID of all processors.

  This function sorts the APIC ID of all processors so that processor number is
  assigned in the ascending order of APIC ID which eases MP debugging.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data
**/
VOID
SortApicId (
  IN PEI_CPU_MP_DATA   *PeiCpuMpData
  )
{
  UINTN             Index1;
  UINTN             Index2;
  UINTN             Index3;
  UINT32            ApicId;
  EFI_HEALTH_FLAGS  Health;
  UINT32            ApCount;

  ApCount = PeiCpuMpData->CpuCount - 1;

  if (ApCount != 0) {
    for (Index1 = 0; Index1 < ApCount; Index1++) {
      Index3 = Index1;
      //
      // Sort key is the hardware default APIC ID
      //
      ApicId = PeiCpuMpData->CpuData[Index1].ApicId;
      for (Index2 = Index1 + 1; Index2 <= ApCount; Index2++) {
        if (ApicId > PeiCpuMpData->CpuData[Index2].ApicId) {
          Index3 = Index2;
          ApicId = PeiCpuMpData->CpuData[Index2].ApicId;
        }
      }
      if (Index3 != Index1) {
        PeiCpuMpData->CpuData[Index3].ApicId = PeiCpuMpData->CpuData[Index1].ApicId;
        PeiCpuMpData->CpuData[Index1].ApicId = ApicId;
        Health = PeiCpuMpData->CpuData[Index3].Health;
        PeiCpuMpData->CpuData[Index3].Health = PeiCpuMpData->CpuData[Index1].Health;
        PeiCpuMpData->CpuData[Index1].Health = Health;
      }
    }

    //
    // Get the processor number for the BSP
    //
    ApicId = GetInitialApicId ();
    for (Index1 = 0; Index1 < PeiCpuMpData->CpuCount; Index1++) {
      if (PeiCpuMpData->CpuData[Index1].ApicId == ApicId) {
        PeiCpuMpData->BspNumber = (UINT32) Index1;
        break;
      }
    }
  }
}

/**
  Get CPU MP Data pointer from the Guided HOB.

  @return  Pointer to Pointer to PEI CPU MP Data
**/
PEI_CPU_MP_DATA *
GetMpHobData (
  VOID
  )
{
  EFI_HOB_GUID_TYPE       *GuidHob;
  VOID                    *DataInHob;
  PEI_CPU_MP_DATA         *CpuMpData;

  CpuMpData = NULL;
  GuidHob = GetFirstGuidHob (&gEfiCallerIdGuid);
  if (GuidHob != NULL) {
    DataInHob = GET_GUID_HOB_DATA (GuidHob);
    CpuMpData = (PEI_CPU_MP_DATA *)(*(UINTN *)DataInHob);
  }
  ASSERT (CpuMpData != NULL);
  return CpuMpData;
}

/**
  This function will be called from AP reset code if BSP uses WakeUpAP.

  @param ExchangeInfo     Pointer to the MP exchange info buffer
  @param NumApsExecuting  Number of curret executing AP
**/
VOID
EFIAPI
ApCFunction (
  IN MP_CPU_EXCHANGE_INFO      *ExchangeInfo,
  IN UINTN                     NumApsExecuting
  )
{
  PEI_CPU_MP_DATA            *PeiCpuMpData;
  UINTN                      ProcessorNumber;
  EFI_AP_PROCEDURE           Procedure;
  UINTN                      BistData;

  PeiCpuMpData = ExchangeInfo->PeiCpuMpData;
  if (PeiCpuMpData->InitFlag) {
    //
    // This is first time AP wakeup, get BIST inforamtion from AP stack
    //
    BistData = *(UINTN *) (PeiCpuMpData->Buffer + NumApsExecuting * PeiCpuMpData->CpuApStackSize - sizeof (UINTN));
    PeiCpuMpData->CpuData[NumApsExecuting].ApicId        = GetInitialApicId ();
    PeiCpuMpData->CpuData[NumApsExecuting].Health.Uint32 = (UINT32) BistData;
    //
    // Sync BSP's Mtrr table to all wakeup APs and load microcode on APs.
    //
    MtrrSetAllMtrrs (&PeiCpuMpData->MtrrTable);
    MicrocodeDetect ();
  } else {
    //
    // Execute AP function if AP is not disabled
    //
    GetProcessorNumber (PeiCpuMpData, &ProcessorNumber);
    if ((PeiCpuMpData->CpuData[ProcessorNumber].State != CpuStateDisabled) &&
        (PeiCpuMpData->ApFunction != 0)) {
      PeiCpuMpData->CpuData[ProcessorNumber].State = CpuStateBusy;
      Procedure = (EFI_AP_PROCEDURE)(UINTN)PeiCpuMpData->ApFunction;
      Procedure ((VOID *)(UINTN)PeiCpuMpData->ApFunctionArgument);
      PeiCpuMpData->CpuData[ProcessorNumber].State = CpuStateIdle;
    }
  }

  //
  // AP finished executing C code
  //
  InterlockedIncrement ((UINT32 *)&PeiCpuMpData->FinishedCount);

  AsmCliHltLoop ();
}

/**
  This function will be called by BSP to wakeup AP.

  @param PeiCpuMpData       Pointer to PEI CPU MP Data
  @param Broadcast          TRUE:  Send broadcast IPI to all APs
                            FALSE: Send IPI to AP by ApicId
  @param ApicId             Apic ID for the processor to be waked
  @param Procedure          The function to be invoked by AP
  @param ProcedureArgument  The argument to be passed into AP function
**/
VOID
WakeUpAP (
  IN PEI_CPU_MP_DATA           *PeiCpuMpData,
  IN BOOLEAN                   Broadcast,
  IN UINT32                    ApicId,
  IN EFI_AP_PROCEDURE          Procedure,              OPTIONAL
  IN VOID                      *ProcedureArgument      OPTIONAL
  )
{
  volatile MP_CPU_EXCHANGE_INFO    *ExchangeInfo;

  PeiCpuMpData->ApFunction         = (UINTN) Procedure;
  PeiCpuMpData->ApFunctionArgument = (UINTN) ProcedureArgument;
  PeiCpuMpData->FinishedCount      = 0;

  ExchangeInfo                     = PeiCpuMpData->MpCpuExchangeInfo;
  ExchangeInfo->Lock               = 0;
  ExchangeInfo->StackStart         = PeiCpuMpData->Buffer;
  ExchangeInfo->StackSize          = PeiCpuMpData->CpuApStackSize;
  ExchangeInfo->BufferStart        = PeiCpuMpData->WakeupBuffer;
  ExchangeInfo->PmodeOffset        = PeiCpuMpData->AddressMap.PModeEntryOffset;
  ExchangeInfo->LmodeOffset        = PeiCpuMpData->AddressMap.LModeEntryOffset;
  ExchangeInfo->Cr3                = AsmReadCr3 ();
  ExchangeInfo->CFunction          = (UINTN) ApCFunction;
  ExchangeInfo->NumApsExecuting    = 0;
  ExchangeInfo->PeiCpuMpData       = PeiCpuMpData;

  //
  // Get the BSP's data of GDT and IDT
  //
  CopyMem ((VOID *)&ExchangeInfo->GdtrProfile, &mGdt, sizeof(mGdt));
  AsmReadIdtr ((IA32_DESCRIPTOR *) &ExchangeInfo->IdtrProfile);

  if (Broadcast) {
    SendInitSipiSipiAllExcludingSelf ((UINT32) ExchangeInfo->BufferStart);
  } else {
    SendInitSipiSipi (ApicId, (UINT32) ExchangeInfo->BufferStart);
  }

  return ;
}

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
  Restore wakeup buffer data.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data
**/
VOID
RestoreWakeupBuffer(
  IN PEI_CPU_MP_DATA         *PeiCpuMpData
  )
{
  CopyMem ((VOID *) PeiCpuMpData->WakeupBuffer, (VOID *) PeiCpuMpData->BackupBuffer, PeiCpuMpData->BackupBufferSize);
}

/**
  This function will get CPU count in the system.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data

  @return  AP processor count
**/
UINT32
CountProcessorNumber (
  IN PEI_CPU_MP_DATA            *PeiCpuMpData
  )
{
  //
  // Load Microcode on BSP
  //
  MicrocodeDetect ();
  //
  // Store BSP's MTRR setting
  //
  MtrrGetAllMtrrs (&PeiCpuMpData->MtrrTable);
  //
  // Send broadcast IPI to APs to wakeup APs
  //
  PeiCpuMpData->InitFlag = 1;
  WakeUpAP (PeiCpuMpData, TRUE, 0, NULL, NULL);
  //
  // Wait for AP task to complete and then exit.
  //
  MicroSecondDelay (PcdGet32 (PcdCpuApInitTimeOutInMicroSeconds));
  PeiCpuMpData->InitFlag  = 0;
  PeiCpuMpData->CpuCount += (UINT32) PeiCpuMpData->MpCpuExchangeInfo->NumApsExecuting;
  ASSERT (PeiCpuMpData->CpuCount <= PcdGet32(PcdCpuMaxLogicalProcessorNumber));
  //
  // Sort BSP/Aps by CPU APIC ID in ascending order
  //
  SortApicId (PeiCpuMpData);

  DEBUG ((EFI_D_INFO, "CpuMpPei: Find %d processors in system.\n", PeiCpuMpData->CpuCount));
  return PeiCpuMpData->CpuCount;
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
  ASSERT (WakeupBuffer != (UINTN) -1);
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
  PeiCpuMpData->CpuData                  = (PEI_CPU_DATA *) (PeiCpuMpData->BackupBuffer +
                                                             PeiCpuMpData->BackupBufferSize);
  PeiCpuMpData->CpuData[0].ApicId        = GetInitialApicId ();
  PeiCpuMpData->CpuData[0].Health.Uint32 = 0;
  PeiCpuMpData->EndOfPeiFlag             = FALSE;
  CopyMem (&PeiCpuMpData->AddressMap, &AddressMap, sizeof (MP_ASSEMBLY_ADDRESS_MAP));

  //
  // Backup original data and copy AP reset code in it
  //
  BackupAndPrepareWakeupBuffer(PeiCpuMpData);

  return PeiCpuMpData;
}

/**
  Notify function on End Of Pei PPI.

  On S3 boot, this function will restore wakeup buffer data.
  On normal boot, this function will flag wakeup buffer to be un-used type.

  @param  PeiServices        The pointer to the PEI Services Table.
  @param  NotifyDescriptor   Address of the notification descriptor data structure.
  @param  Ppi                Address of the PPI that was installed.

  @retval EFI_SUCCESS        When everything is OK.

**/
EFI_STATUS
EFIAPI
CpuMpEndOfPeiCallback (
  IN      EFI_PEI_SERVICES        **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR    *NotifyDescriptor,
  IN VOID                         *Ppi
  )
{
  EFI_STATUS                Status;
  EFI_BOOT_MODE             BootMode;
  PEI_CPU_MP_DATA           *PeiCpuMpData;
  EFI_PEI_HOB_POINTERS      Hob;
  EFI_HOB_MEMORY_ALLOCATION *MemoryHob;

  DEBUG ((EFI_D_INFO, "CpuMpPei: CpuMpEndOfPeiCallback () invokded\n"));

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  PeiCpuMpData = GetMpHobData ();
  ASSERT (PeiCpuMpData != NULL);

  if (BootMode != BOOT_ON_S3_RESUME) {
    //
    // Get the HOB list for processing
    //
    Hob.Raw = GetHobList ();
    //
    // Collect memory ranges
    //
    while (!END_OF_HOB_LIST (Hob)) {
      if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
        MemoryHob = Hob.MemoryAllocation;
        if(MemoryHob->AllocDescriptor.MemoryBaseAddress == PeiCpuMpData->WakeupBuffer) {
          //
          // Flag this HOB type to un-used
          //
          GET_HOB_TYPE (Hob) = EFI_HOB_TYPE_UNUSED;
          break;
        }
      }
      Hob.Raw = GET_NEXT_HOB (Hob);
    }
  } else {
    RestoreWakeupBuffer (PeiCpuMpData);
    PeiCpuMpData->EndOfPeiFlag = TRUE;
  }
  return EFI_SUCCESS;
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
  EFI_STATUS           Status;
  PEI_CPU_MP_DATA      *PeiCpuMpData;
  UINT32               ProcessorCount;

  //
  // Load new GDT table on BSP
  //
  AsmInitializeGdt (&mGdt);
  //
  // Get wakeup buffer and copy AP reset code in it
  //
  PeiCpuMpData = PrepareAPStartupVector ();
  //
  // Count processor number and collect processor information
  //
  ProcessorCount = CountProcessorNumber (PeiCpuMpData);
  //
  // Build location of PEI CPU MP DATA buffer in HOB
  //
  BuildGuidDataHob (
    &gEfiCallerIdGuid,
    (VOID *)&PeiCpuMpData,
    sizeof(UINT64)
    );
  //
  // Update and publish CPU BIST information
  //
  CollectBistDataFromPpi (PeiServices, PeiCpuMpData);
  //
  // register an event for EndOfPei
  //
  Status  = PeiServicesNotifyPpi (&mNotifyList);
  ASSERT_EFI_ERROR (Status);
  //
  // Install CPU MP PPI
  //
  Status = PeiServicesInstallPpi(&mPeiCpuMpPpiDesc);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
