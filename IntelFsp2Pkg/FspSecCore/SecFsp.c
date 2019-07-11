/** @file

  Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecFsp.h"

/**

  Calculate the FSP IDT gate descriptor.

  @param[in] IdtEntryTemplate     IDT gate descriptor template.

  @return                     FSP specific IDT gate descriptor.

**/
UINT64
FspGetExceptionHandler(
  IN  UINT64  IdtEntryTemplate
  )
{
  UINT32                    Entry;
  UINT64                    ExceptionHandler;
  IA32_IDT_GATE_DESCRIPTOR *IdtGateDescriptor;
  FSP_INFO_HEADER          *FspInfoHeader;

  FspInfoHeader     = (FSP_INFO_HEADER *)AsmGetFspInfoHeader();
  ExceptionHandler  = IdtEntryTemplate;
  IdtGateDescriptor = (IA32_IDT_GATE_DESCRIPTOR *)&ExceptionHandler;
  Entry = (IdtGateDescriptor->Bits.OffsetHigh << 16) | IdtGateDescriptor->Bits.OffsetLow;
  Entry = FspInfoHeader->ImageBase + FspInfoHeader->ImageSize - (~Entry + 1);
  IdtGateDescriptor->Bits.OffsetHigh = (UINT16)(Entry >> 16);
  IdtGateDescriptor->Bits.OffsetLow  = (UINT16)Entry;

  return ExceptionHandler;
}

/**
  This interface fills platform specific data.

  @param[in,out]  FspData           Pointer to the FSP global data.

**/
VOID
EFIAPI
SecGetPlatformData (
  IN OUT  FSP_GLOBAL_DATA    *FspData
  )
{
  FSP_PLAT_DATA    *FspPlatformData;
  UINT32            TopOfCar;
  UINT32           *StackPtr;
  UINT32            DwordSize;

  FspPlatformData = &FspData->PlatformData;

  //
  // The entries of platform information, together with the number of them,
  // reside in the bottom of stack, left untouched by normal stack operation.
  //

  FspPlatformData->DataPtr   = NULL;
  FspPlatformData->MicrocodeRegionBase = 0;
  FspPlatformData->MicrocodeRegionSize = 0;
  FspPlatformData->CodeRegionBase      = 0;
  FspPlatformData->CodeRegionSize      = 0;

  //
  // Pointer to the size field
  //
  TopOfCar = PcdGet32(PcdTemporaryRamBase) + PcdGet32(PcdTemporaryRamSize);
  StackPtr = (UINT32 *)(TopOfCar - sizeof (UINT32));

  if (*(StackPtr - 1) == FSP_MCUD_SIGNATURE) {
    while (*StackPtr != 0) {
      if (*(StackPtr - 1) == FSP_MCUD_SIGNATURE) {
        //
        // This following data was pushed onto stack after TempRamInit API
        //
        DwordSize = 4;
        StackPtr  = StackPtr - 1 - DwordSize;
        CopyMem (&(FspPlatformData->MicrocodeRegionBase), StackPtr, (DwordSize << 2));
        StackPtr--;
      } else if (*(StackPtr - 1) == FSP_PER0_SIGNATURE) {
        //
        // This is the performance data for InitTempMemory API entry/exit
        //
        DwordSize = 4;
        StackPtr  = StackPtr - 1 - DwordSize;
        CopyMem (FspData->PerfData, StackPtr, (DwordSize << 2));

        ((UINT8 *)(&FspData->PerfData[0]))[7] = FSP_PERF_ID_API_TEMP_RAM_INIT_ENTRY;
        ((UINT8 *)(&FspData->PerfData[1]))[7] = FSP_PERF_ID_API_TEMP_RAM_INIT_EXIT;

        StackPtr--;
      } else {
        StackPtr -= (*StackPtr);
      }
    }
  }
}

/**

  Initialize the FSP global data region.
  It needs to be done as soon as possible after the stack is setup.

  @param[in,out] PeiFspData             Pointer of the FSP global data.
  @param[in]     BootLoaderStack        BootLoader stack.
  @param[in]     ApiIdx                 The index of the FSP API.

**/
VOID
FspGlobalDataInit (
  IN OUT  FSP_GLOBAL_DATA    *PeiFspData,
  IN UINT32                   BootLoaderStack,
  IN UINT8                    ApiIdx
  )
{
  VOID              *FspmUpdDataPtr;
  CHAR8              ImageId[9];
  UINTN              Idx;

  //
  // Set FSP Global Data pointer
  //
  SetFspGlobalDataPointer    (PeiFspData);
  ZeroMem  ((VOID *)PeiFspData, sizeof(FSP_GLOBAL_DATA));

  PeiFspData->Signature            = FSP_GLOBAL_DATA_SIGNATURE;
  PeiFspData->Version              = 0;
  PeiFspData->CoreStack            = BootLoaderStack;
  PeiFspData->PerfIdx              = 2;
  PeiFspData->PerfSig              = FSP_PERFORMANCE_DATA_SIGNATURE;

  SetFspMeasurePoint (FSP_PERF_ID_API_FSP_MEMORY_INIT_ENTRY);

  //
  // Get FSP Header offset
  // It may have multiple FVs, so look into the last one for FSP header
  //
  PeiFspData->FspInfoHeader      = (FSP_INFO_HEADER *)AsmGetFspInfoHeader();
  SecGetPlatformData (PeiFspData);

  //
  // Set API calling mode
  //
  SetFspApiCallingIndex (ApiIdx);

  //
  // Set UPD pointer
  //
  FspmUpdDataPtr = (VOID *) GetFspApiParameter ();
  if (FspmUpdDataPtr == NULL) {
    FspmUpdDataPtr = (VOID *)(PeiFspData->FspInfoHeader->ImageBase + PeiFspData->FspInfoHeader->CfgRegionOffset);
  }
  SetFspUpdDataPointer (FspmUpdDataPtr);
  SetFspMemoryInitUpdDataPointer (FspmUpdDataPtr);
  SetFspSiliconInitUpdDataPointer (NULL);

  //
  // Initialize serial port
  // It might have been done in ProcessLibraryConstructorList(), however,
  // the FSP global data is not initialized at that time. So do it again
  // for safe.
  //
  SerialPortInitialize ();

  //
  // Ensure the golbal data pointer is valid
  //
  ASSERT (GetFspGlobalDataPointer () == PeiFspData);

  for (Idx = 0; Idx < 8; Idx++) {
    ImageId[Idx] = PeiFspData->FspInfoHeader->ImageId[Idx];
  }
  ImageId[Idx] = 0;

  DEBUG ((DEBUG_INFO | DEBUG_INIT, "\n============= FSP Spec v%d.%d Header Revision v%x (%a v%x.%x.%x.%x) =============\n", \
         (PeiFspData->FspInfoHeader->SpecVersion >> 4) & 0xF, \
         PeiFspData->FspInfoHeader->SpecVersion & 0xF, \
         PeiFspData->FspInfoHeader->HeaderRevision, \
         ImageId, \
         (PeiFspData->FspInfoHeader->ImageRevision >> 24) & 0xFF, \
         (PeiFspData->FspInfoHeader->ImageRevision >> 16) & 0xFF, \
         (PeiFspData->FspInfoHeader->ImageRevision >> 8) & 0xFF, \
         PeiFspData->FspInfoHeader->ImageRevision & 0xFF));
}

/**

  Adjust the FSP data pointers after the stack is migrated to memory.

  @param[in] OffsetGap             The offset gap between the old stack and the new stack.

**/
VOID
FspDataPointerFixUp (
  IN UINT32   OffsetGap
  )
{
  FSP_GLOBAL_DATA  *NewFspData;

  NewFspData = (FSP_GLOBAL_DATA *)((UINTN)GetFspGlobalDataPointer() + (UINTN)OffsetGap);
  SetFspGlobalDataPointer (NewFspData);
}
