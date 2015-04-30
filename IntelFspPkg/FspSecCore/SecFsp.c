/** @file

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  This function gets the FSP UPD region offset in flash.

  @return the offset of the UPD region.

**/
UINT32
EFIAPI
GetFspUpdRegionOffset (
  VOID
  )
{
  FSP_GLOBAL_DATA                   *FspData;
  UINT32                            *Offset;

  FspData       = GetFspGlobalDataPointer ();

  //
  // It is required to put PcdUpdRegionOffset at offset 0x000C
  // for all FSPs.
  // gPlatformFspPkgTokenSpaceGuid.PcdUpdRegionOffset       | 0x000C | 0x12345678
  //
  Offset        = (UINT32 *)(FspData->FspInfoHeader->ImageBase + \
                             FspData->FspInfoHeader->CfgRegionOffset + 0x0C);

  return  *Offset;
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
  TopOfCar = PcdGet32 (PcdTemporaryRamBase) + PcdGet32 (PcdTemporaryRamSize);

  FspPlatformData->DataPtr   = NULL;
  FspPlatformData->MicrocodeRegionBase = 0;
  FspPlatformData->MicrocodeRegionSize = 0;
  FspPlatformData->CodeRegionBase      = 0;
  FspPlatformData->CodeRegionSize      = 0;

  //
  // Pointer to the size field
  //
  StackPtr  = (UINT32 *)(TopOfCar - sizeof(UINT32));

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
      ((UINT8 *)(&FspData->PerfData[0]))[7] = FSP_PERF_ID_API_TMPRAMINIT_ENTRY;
      ((UINT8 *)(&FspData->PerfData[1]))[7] = FSP_PERF_ID_API_TMPRAMINIT_EXIT;
      StackPtr--;
    } else {
      StackPtr -= (*StackPtr);
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
  VOID              *UpdDataRgnPtr;
  FSP_INIT_PARAMS   *FspInitParams;
  CHAR8              ImageId[9];
  UINTN              Idx;

  //
  // Init PCIE_BAR with value and set global FSP data pointer.
  // PciExpress Base should have been programmed by platform already.
  //
  SetFspGlobalDataPointer    (PeiFspData);
  ZeroMem  ((VOID *)PeiFspData, sizeof(FSP_GLOBAL_DATA));

  PeiFspData->Signature          = FSP_GLOBAL_DATA_SIGNATURE;
  PeiFspData->CoreStack          = BootLoaderStack;
  PeiFspData->PerfIdx            = 2;

  SetFspMeasurePoint (FSP_PERF_ID_API_FSPINIT_ENTRY);

  //
  // Get FSP Header offset
  // It may have multiple FVs, so look into the last one for FSP header
  //
  PeiFspData->FspInfoHeader      = (FSP_INFO_HEADER *)AsmGetFspInfoHeader();
  SecGetPlatformData (PeiFspData);

  //
  // Set API calling mode
  //
  SetFspApiCallingMode (ApiIdx == 1 ? 0 : 1);

  //
  // Initialize UPD pointer.
  //
  FspInitParams = (FSP_INIT_PARAMS *)GetFspApiParameter ();
  UpdDataRgnPtr = ((FSP_INIT_RT_COMMON_BUFFER *)FspInitParams->RtBufferPtr)->UpdDataRgnPtr;
  if (UpdDataRgnPtr == NULL) {
    UpdDataRgnPtr = (VOID *)(PeiFspData->FspInfoHeader->ImageBase + GetFspUpdRegionOffset());
  }
  SetFspUpdDataPointer (UpdDataRgnPtr);

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

  DEBUG ((DEBUG_INFO | DEBUG_INIT, "\n============= PEIM FSP v1.%x (%a v%x.%x.%x.%x) =============\n", \
         PeiFspData->FspInfoHeader->HeaderRevision - 1, \
         ImageId, \
         (PeiFspData->FspInfoHeader->ImageRevision >> 24) & 0xff, \
         (PeiFspData->FspInfoHeader->ImageRevision >> 16) & 0xff, \
         (PeiFspData->FspInfoHeader->ImageRevision >> 8) & 0xff, \
         (PeiFspData->FspInfoHeader->ImageRevision >> 0) & 0xff));

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

/**
  This function check the FSP API calling condition.

  @param[in]  ApiIdx           Internal index of the FSP API.
  @param[in]  ApiParam         Parameter of the FSP API.

**/
EFI_STATUS
EFIAPI
FspApiCallingCheck (
  IN UINT32   ApiIdx,
  IN VOID     *ApiParam
  )
{
  EFI_STATUS                Status;
  FSP_GLOBAL_DATA           *FspData;
  FSP_INIT_PARAMS           *FspInitParams;
  FSP_INIT_RT_COMMON_BUFFER *FspRtBuffer;

  FspInitParams = (FSP_INIT_PARAMS *) ApiParam;
  FspRtBuffer = ((FSP_INIT_RT_COMMON_BUFFER *)FspInitParams->RtBufferPtr);

  Status = EFI_SUCCESS;
  FspData = GetFspGlobalDataPointer ();
  if (ApiIdx == 1) {
    //
    // FspInit check
    //
    if ((UINT32)FspData != 0xFFFFFFFF) {
      Status = EFI_UNSUPPORTED;
    } else if ((FspRtBuffer == NULL) || ((FspRtBuffer->BootLoaderTolumSize % EFI_PAGE_SIZE) != 0)) {
      Status = EFI_INVALID_PARAMETER;
    }
  } else if (ApiIdx == 2) {
    //
    // NotifyPhase check
    //
    if ((FspData == NULL) || ((UINT32)FspData == 0xFFFFFFFF)) {
      Status = EFI_UNSUPPORTED;
    } else {
      if (FspData->Signature != FSP_GLOBAL_DATA_SIGNATURE) {
        Status = EFI_UNSUPPORTED;
      }
    }
  } else if (ApiIdx == 3) {
    //
    // FspMemoryInit check
    //
    if ((UINT32)FspData != 0xFFFFFFFF) {
      Status = EFI_UNSUPPORTED;
    } else if ((FspRtBuffer == NULL) || ((FspRtBuffer->BootLoaderTolumSize % EFI_PAGE_SIZE) != 0)) {
      Status = EFI_INVALID_PARAMETER;
    }
  } else if (ApiIdx == 4) {
    //
    // TempRamExit check
    //
    if ((FspData == NULL) || ((UINT32)FspData == 0xFFFFFFFF)) {
      Status = EFI_UNSUPPORTED;
    } else {
      if (FspData->Signature != FSP_GLOBAL_DATA_SIGNATURE) {
        Status = EFI_UNSUPPORTED;
      }
    }
  } else if (ApiIdx == 5) {
    //
    // FspSiliconInit check
    //
    if ((FspData == NULL) || ((UINT32)FspData == 0xFFFFFFFF)) {
      Status = EFI_UNSUPPORTED;
    } else {
      if (FspData->Signature != FSP_GLOBAL_DATA_SIGNATURE) {
        Status = EFI_UNSUPPORTED;
      }
    }
  } else {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/**
  This function gets the boot FV offset in FSP.
  @return the boot firmware volumen offset inside FSP binary

**/
UINT32
EFIAPI
GetBootFirmwareVolumeOffset (
  VOID
  )
{ 
  return PcdGet32 (PcdFspBootFirmwareVolumeBase) - PcdGet32 (PcdFspAreaBaseAddress);
}
