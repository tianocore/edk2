/** @file
  Intel Processor Trace feature.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuCommonFeatures.h"

///
/// This macro define the max entries in the Topa table.
/// Each entry in the table contains some attribute bits, a pointer to an output region, and the size of the region.
/// The last entry in the table may hold a pointer to the next table. This pointer can either point to the top of the
/// current table (for circular array) or to the base of another table.
/// At least 2 entries are needed because the list of entries must
/// be terminated by an entry with the END bit set to 1, so 2
/// entries are required to use a single valid entry.
///
#define MAX_TOPA_ENTRY_COUNT  2

///
/// Processor trace output scheme selection.
///
typedef enum {
  RtitOutputSchemeSingleRange = 0,
  RtitOutputSchemeToPA
} RTIT_OUTPUT_SCHEME;

typedef struct  {
  BOOLEAN                                    TopaSupported;
  BOOLEAN                                    SingleRangeSupported;
  MSR_IA32_RTIT_CTL_REGISTER                 RtitCtrl;
  MSR_IA32_RTIT_OUTPUT_BASE_REGISTER         RtitOutputBase;
  MSR_IA32_RTIT_OUTPUT_MASK_PTRS_REGISTER    RtitOutputMaskPtrs;
} PROC_TRACE_PROCESSOR_DATA;

typedef struct  {
  UINT32                       NumberOfProcessors;

  UINT8                        ProcTraceOutputScheme;
  UINT32                       ProcTraceMemSize;

  UINTN                        *ThreadMemRegionTable;
  UINTN                        AllocatedThreads;

  UINTN                        *TopaMemArray;

  PROC_TRACE_PROCESSOR_DATA    *ProcessorData;
} PROC_TRACE_DATA;

typedef struct {
  RTIT_TOPA_TABLE_ENTRY    TopaEntry[MAX_TOPA_ENTRY_COUNT];
} PROC_TRACE_TOPA_TABLE;

/**
  Prepares for the data used by CPU feature detection and initialization.

  @param[in]  NumberOfProcessors  The number of CPUs in the platform.

  @return  Pointer to a buffer of CPU related configuration data.

  @note This service could be called by BSP only.
**/
VOID *
EFIAPI
ProcTraceGetConfigData (
  IN UINTN  NumberOfProcessors
  )
{
  PROC_TRACE_DATA  *ConfigData;

  ConfigData = AllocateZeroPool (sizeof (PROC_TRACE_DATA) + sizeof (PROC_TRACE_PROCESSOR_DATA) * NumberOfProcessors);
  ASSERT (ConfigData != NULL);
  ConfigData->ProcessorData = (PROC_TRACE_PROCESSOR_DATA *)((UINT8 *)ConfigData + sizeof (PROC_TRACE_DATA));

  ConfigData->NumberOfProcessors    = (UINT32)NumberOfProcessors;
  ConfigData->ProcTraceMemSize      = PcdGet32 (PcdCpuProcTraceMemSize);
  ConfigData->ProcTraceOutputScheme = PcdGet8 (PcdCpuProcTraceOutputScheme);

  return ConfigData;
}

/**
  Detects if Intel Processor Trace feature supported on current
  processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Processor Trace feature is supported.
  @retval FALSE    Processor Trace feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
ProcTraceSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  PROC_TRACE_DATA                              *ProcTraceData;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EBX  Ebx;
  CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF_ECX    Ecx;

  //
  // Check if ProcTraceMemorySize option is enabled (0xFF means disable by user)
  //
  ProcTraceData = (PROC_TRACE_DATA *)ConfigData;
  ASSERT (ProcTraceData != NULL);
  if ((ProcTraceData->ProcTraceMemSize > RtitTopaMemorySize128M) ||
      (ProcTraceData->ProcTraceOutputScheme > RtitOutputSchemeToPA))
  {
    return FALSE;
  }

  //
  // Check if Processor Trace is supported
  //
  AsmCpuidEx (CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS, 0, NULL, &Ebx.Uint32, NULL, NULL);
  if (Ebx.Bits.IntelProcessorTrace == 0) {
    return FALSE;
  }

  AsmCpuidEx (CPUID_INTEL_PROCESSOR_TRACE, CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF, NULL, NULL, &Ecx.Uint32, NULL);
  ProcTraceData->ProcessorData[ProcessorNumber].TopaSupported        = (BOOLEAN)(Ecx.Bits.RTIT == 1);
  ProcTraceData->ProcessorData[ProcessorNumber].SingleRangeSupported = (BOOLEAN)(Ecx.Bits.SingleRangeOutput == 1);
  if ((ProcTraceData->ProcessorData[ProcessorNumber].TopaSupported && (ProcTraceData->ProcTraceOutputScheme == RtitOutputSchemeToPA)) ||
      (ProcTraceData->ProcessorData[ProcessorNumber].SingleRangeSupported && (ProcTraceData->ProcTraceOutputScheme == RtitOutputSchemeSingleRange)))
  {
    ProcTraceData->ProcessorData[ProcessorNumber].RtitCtrl.Uint64           = AsmReadMsr64 (MSR_IA32_RTIT_CTL);
    ProcTraceData->ProcessorData[ProcessorNumber].RtitOutputBase.Uint64     = AsmReadMsr64 (MSR_IA32_RTIT_OUTPUT_BASE);
    ProcTraceData->ProcessorData[ProcessorNumber].RtitOutputMaskPtrs.Uint64 = AsmReadMsr64 (MSR_IA32_RTIT_OUTPUT_MASK_PTRS);
    return TRUE;
  }

  return FALSE;
}

/**
  Initializes Intel Processor Trace feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Processor Trace feature must be
                               enabled.
                               If FALSE, then the Processor Trace feature must be
                               disabled.

  @retval RETURN_SUCCESS       Intel Processor Trace feature is initialized.

**/
RETURN_STATUS
EFIAPI
ProcTraceInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  )
{
  UINT32                                   MemRegionSize;
  UINTN                                    Pages;
  UINTN                                    Alignment;
  UINTN                                    MemRegionBaseAddr;
  UINTN                                    *ThreadMemRegionTable;
  UINTN                                    Index;
  UINTN                                    TopaTableBaseAddr;
  UINTN                                    AlignedAddress;
  UINTN                                    *TopaMemArray;
  PROC_TRACE_TOPA_TABLE                    *TopaTable;
  PROC_TRACE_DATA                          *ProcTraceData;
  BOOLEAN                                  FirstIn;
  MSR_IA32_RTIT_CTL_REGISTER               CtrlReg;
  MSR_IA32_RTIT_STATUS_REGISTER            StatusReg;
  MSR_IA32_RTIT_OUTPUT_BASE_REGISTER       OutputBaseReg;
  MSR_IA32_RTIT_OUTPUT_MASK_PTRS_REGISTER  OutputMaskPtrsReg;
  RTIT_TOPA_TABLE_ENTRY                    *TopaEntryPtr;

  //
  // The scope of the MSR_IA32_RTIT_* is core for below processor type, only program
  // MSR_IA32_RTIT_* for thread 0 in each core.
  //
  if (IS_GOLDMONT_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_GOLDMONT_PLUS_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel))
  {
    if (CpuInfo->ProcessorInfo.Location.Thread != 0) {
      return RETURN_SUCCESS;
    }
  }

  ProcTraceData = (PROC_TRACE_DATA *)ConfigData;
  ASSERT (ProcTraceData != NULL);

  //
  // Clear MSR_IA32_RTIT_CTL[0] and IA32_RTIT_STS only if MSR_IA32_RTIT_CTL[0]==1b
  //
  CtrlReg.Uint64 = ProcTraceData->ProcessorData[ProcessorNumber].RtitCtrl.Uint64;
  if (CtrlReg.Bits.TraceEn != 0) {
    ///
    /// Clear bit 0 in MSR IA32_RTIT_CTL (570)
    ///
    CtrlReg.Bits.TraceEn = 0;
    CPU_REGISTER_TABLE_WRITE64 (
      ProcessorNumber,
      Msr,
      MSR_IA32_RTIT_CTL,
      CtrlReg.Uint64
      );

    ///
    /// Clear MSR IA32_RTIT_STS (571h) to all zeros
    ///
    StatusReg.Uint64 = 0x0;
    CPU_REGISTER_TABLE_WRITE64 (
      ProcessorNumber,
      Msr,
      MSR_IA32_RTIT_STATUS,
      StatusReg.Uint64
      );
  }

  if (!State) {
    return RETURN_SUCCESS;
  }

  MemRegionBaseAddr = 0;
  FirstIn           = FALSE;

  if (ProcTraceData->ThreadMemRegionTable == NULL) {
    FirstIn = TRUE;
    DEBUG ((DEBUG_INFO, "Initialize Processor Trace\n"));
  }

  ///
  /// Refer to PROC_TRACE_MEM_SIZE Table for Size Encoding
  ///
  MemRegionSize = (UINT32)(1 << (ProcTraceData->ProcTraceMemSize + 12));
  if (FirstIn) {
    DEBUG ((DEBUG_INFO, "ProcTrace: MemSize requested: 0x%X \n", MemRegionSize));
  }

  if (FirstIn) {
    //
    //   Let BSP allocate and create the necessary memory region (Aligned to the size of
    //   the memory region from setup option(ProcTraceMemSize) which is an integral multiple of 4kB)
    //   for all the enabled threads to store Processor Trace debug data. Then Configure the trace
    //   address base in MSR, IA32_RTIT_OUTPUT_BASE (560h) bits 47:12. Note that all regions must be
    //   aligned based on their size, not just 4K. Thus a 2M region must have bits 20:12 cleared.
    //
    ThreadMemRegionTable = (UINTN *)AllocatePool (ProcTraceData->NumberOfProcessors * sizeof (UINTN *));
    if (ThreadMemRegionTable == NULL) {
      DEBUG ((DEBUG_ERROR, "Allocate ProcTrace ThreadMemRegionTable Failed\n"));
      return RETURN_OUT_OF_RESOURCES;
    }

    ProcTraceData->ThreadMemRegionTable = ThreadMemRegionTable;

    for (Index = 0; Index < ProcTraceData->NumberOfProcessors; Index++, ProcTraceData->AllocatedThreads++) {
      Pages          = EFI_SIZE_TO_PAGES (MemRegionSize);
      Alignment      = MemRegionSize;
      AlignedAddress = (UINTN)AllocateAlignedReservedPages (Pages, Alignment);
      if (AlignedAddress == 0) {
        DEBUG ((DEBUG_ERROR, "ProcTrace: Out of mem, allocated only for %d threads\n", ProcTraceData->AllocatedThreads));
        if (Index == 0) {
          //
          // Could not allocate for BSP even
          //
          FreePool ((VOID *)ThreadMemRegionTable);
          ThreadMemRegionTable = NULL;
          return RETURN_OUT_OF_RESOURCES;
        }

        break;
      }

      ThreadMemRegionTable[Index] = AlignedAddress;
      DEBUG ((DEBUG_INFO, "ProcTrace: PT MemRegionBaseAddr(aligned) for thread %d: 0x%llX \n", Index, (UINT64)ThreadMemRegionTable[Index]));
    }

    DEBUG ((DEBUG_INFO, "ProcTrace: Allocated PT mem for %d thread \n", ProcTraceData->AllocatedThreads));
  }

  if (ProcessorNumber < ProcTraceData->AllocatedThreads) {
    MemRegionBaseAddr = ProcTraceData->ThreadMemRegionTable[ProcessorNumber];
  } else {
    return RETURN_SUCCESS;
  }

  ///
  /// Check Processor Trace output scheme: Single Range output or ToPA table
  ///

  //
  //  Single Range output scheme
  //
  if (ProcTraceData->ProcessorData[ProcessorNumber].SingleRangeSupported &&
      (ProcTraceData->ProcTraceOutputScheme == RtitOutputSchemeSingleRange))
  {
    if (FirstIn) {
      DEBUG ((DEBUG_INFO, "ProcTrace: Enabling Single Range Output scheme \n"));
    }

    //
    // Clear MSR IA32_RTIT_CTL (0x570) ToPA (Bit 8)
    //
    CtrlReg.Bits.ToPA = 0;
    CPU_REGISTER_TABLE_WRITE64 (
      ProcessorNumber,
      Msr,
      MSR_IA32_RTIT_CTL,
      CtrlReg.Uint64
      );

    //
    // Program MSR IA32_RTIT_OUTPUT_BASE (0x560) bits[63:7] with the allocated Memory Region
    //
    OutputBaseReg.Uint64      = ProcTraceData->ProcessorData[ProcessorNumber].RtitOutputBase.Uint64;
    OutputBaseReg.Bits.Base   = (MemRegionBaseAddr >> 7) & 0x01FFFFFF;
    OutputBaseReg.Bits.BaseHi = RShiftU64 ((UINT64)MemRegionBaseAddr, 32) & 0xFFFFFFFF;
    CPU_REGISTER_TABLE_WRITE64 (
      ProcessorNumber,
      Msr,
      MSR_IA32_RTIT_OUTPUT_BASE,
      OutputBaseReg.Uint64
      );

    //
    // Program the Mask bits for the Memory Region to MSR IA32_RTIT_OUTPUT_MASK_PTRS (561h)
    //
    OutputMaskPtrsReg.Uint64                 = ProcTraceData->ProcessorData[ProcessorNumber].RtitOutputMaskPtrs.Uint64;
    OutputMaskPtrsReg.Bits.MaskOrTableOffset = ((MemRegionSize - 1) >> 7) & 0x01FFFFFF;
    OutputMaskPtrsReg.Bits.OutputOffset      = RShiftU64 (MemRegionSize - 1, 32) & 0xFFFFFFFF;
    CPU_REGISTER_TABLE_WRITE64 (
      ProcessorNumber,
      Msr,
      MSR_IA32_RTIT_OUTPUT_MASK_PTRS,
      OutputMaskPtrsReg.Uint64
      );
  }

  //
  //  ToPA(Table of physical address) scheme
  //
  if (ProcTraceData->ProcessorData[ProcessorNumber].TopaSupported &&
      (ProcTraceData->ProcTraceOutputScheme == RtitOutputSchemeToPA))
  {
    //
    //  Create ToPA structure aligned at 4KB for each logical thread
    //  with at least 2 entries by 8 bytes size each. The first entry
    //  should have the trace output base address in bits 47:12, 6:9
    //  for Size, bits 4,2 and 0 must be cleared. The second entry
    //  should have the base address of the table location in bits
    //  47:12, bits 4 and 2 must be cleared and bit 0 must be set.
    //
    if (FirstIn) {
      DEBUG ((DEBUG_INFO, "ProcTrace: Enabling ToPA scheme \n"));
      //
      // Let BSP allocate ToPA table mem for all threads
      //
      TopaMemArray = (UINTN *)AllocatePool (ProcTraceData->AllocatedThreads * sizeof (UINTN *));
      if (TopaMemArray == NULL) {
        DEBUG ((DEBUG_ERROR, "ProcTrace: Allocate mem for ToPA Failed\n"));
        return RETURN_OUT_OF_RESOURCES;
      }

      ProcTraceData->TopaMemArray = TopaMemArray;

      for (Index = 0; Index < ProcTraceData->AllocatedThreads; Index++) {
        Pages          = EFI_SIZE_TO_PAGES (sizeof (PROC_TRACE_TOPA_TABLE));
        Alignment      = 0x1000;
        AlignedAddress = (UINTN)AllocateAlignedReservedPages (Pages, Alignment);
        if (AlignedAddress == 0) {
          if (Index < ProcTraceData->AllocatedThreads) {
            ProcTraceData->AllocatedThreads = Index;
          }

          DEBUG ((DEBUG_ERROR, "ProcTrace:  Out of mem, allocated ToPA mem only for %d threads\n", ProcTraceData->AllocatedThreads));
          if (Index == 0) {
            //
            // Could not allocate for BSP even
            //
            FreePool ((VOID *)TopaMemArray);
            TopaMemArray = NULL;
            return RETURN_OUT_OF_RESOURCES;
          }

          break;
        }

        TopaMemArray[Index] = AlignedAddress;
        DEBUG ((DEBUG_INFO, "ProcTrace: Topa table address(aligned) for thread %d is 0x%llX \n", Index, (UINT64)TopaMemArray[Index]));
      }

      DEBUG ((DEBUG_INFO, "ProcTrace: Allocated ToPA mem for %d thread \n", ProcTraceData->AllocatedThreads));
    }

    if (ProcessorNumber < ProcTraceData->AllocatedThreads) {
      TopaTableBaseAddr = ProcTraceData->TopaMemArray[ProcessorNumber];
    } else {
      return RETURN_SUCCESS;
    }

    TopaTable                 = (PROC_TRACE_TOPA_TABLE *)TopaTableBaseAddr;
    TopaEntryPtr              = &TopaTable->TopaEntry[0];
    TopaEntryPtr->Uint64      = 0;
    TopaEntryPtr->Bits.Base   = (MemRegionBaseAddr >> 12) & 0x000FFFFF;
    TopaEntryPtr->Bits.BaseHi = RShiftU64 ((UINT64)MemRegionBaseAddr, 32) & 0xFFFFFFFF;
    TopaEntryPtr->Bits.Size   = ProcTraceData->ProcTraceMemSize;
    TopaEntryPtr->Bits.END    = 0;

    TopaEntryPtr              = &TopaTable->TopaEntry[1];
    TopaEntryPtr->Uint64      = 0;
    TopaEntryPtr->Bits.Base   = (TopaTableBaseAddr >> 12) & 0x000FFFFF;
    TopaEntryPtr->Bits.BaseHi = RShiftU64 ((UINT64)TopaTableBaseAddr, 32) & 0xFFFFFFFF;
    TopaEntryPtr->Bits.END    = 1;

    //
    // Program the MSR IA32_RTIT_OUTPUT_BASE (0x560) bits[63:7] with ToPA base
    //
    OutputBaseReg.Uint64      = ProcTraceData->ProcessorData[ProcessorNumber].RtitOutputBase.Uint64;
    OutputBaseReg.Bits.Base   = (TopaTableBaseAddr >> 7) & 0x01FFFFFF;
    OutputBaseReg.Bits.BaseHi = RShiftU64 ((UINT64)TopaTableBaseAddr, 32) & 0xFFFFFFFF;
    CPU_REGISTER_TABLE_WRITE64 (
      ProcessorNumber,
      Msr,
      MSR_IA32_RTIT_OUTPUT_BASE,
      OutputBaseReg.Uint64
      );

    //
    // Set the MSR IA32_RTIT_OUTPUT_MASK (0x561) bits[63:7] to 0
    //
    OutputMaskPtrsReg.Uint64                 = ProcTraceData->ProcessorData[ProcessorNumber].RtitOutputMaskPtrs.Uint64;
    OutputMaskPtrsReg.Bits.MaskOrTableOffset = 0;
    OutputMaskPtrsReg.Bits.OutputOffset      = 0;
    CPU_REGISTER_TABLE_WRITE64 (
      ProcessorNumber,
      Msr,
      MSR_IA32_RTIT_OUTPUT_MASK_PTRS,
      OutputMaskPtrsReg.Uint64
      );
    //
    // Enable ToPA output scheme by enabling MSR IA32_RTIT_CTL (0x570) ToPA (Bit 8)
    //
    CtrlReg.Bits.ToPA = 1;
    CPU_REGISTER_TABLE_WRITE64 (
      ProcessorNumber,
      Msr,
      MSR_IA32_RTIT_CTL,
      CtrlReg.Uint64
      );
  }

  ///
  /// Enable the Processor Trace feature from MSR IA32_RTIT_CTL (570h)
  ///
  CtrlReg.Bits.OS       = 1;
  CtrlReg.Bits.User     = 1;
  CtrlReg.Bits.BranchEn = 1;
  CtrlReg.Bits.TraceEn  = 1;
  CPU_REGISTER_TABLE_WRITE64 (
    ProcessorNumber,
    Msr,
    MSR_IA32_RTIT_CTL,
    CtrlReg.Uint64
    );

  return RETURN_SUCCESS;
}
