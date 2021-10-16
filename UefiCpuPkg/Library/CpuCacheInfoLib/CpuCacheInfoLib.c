/** @file
  Provides cache info for each package, core type, cache level and cache type.

  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCpuCacheInfoLib.h"

/**
  Print CpuCacheInfo array.

  @param[in]  CpuCacheInfo        Pointer to the CpuCacheInfo array.
  @param[in]  CpuCacheInfoCount   The length of CpuCacheInfo array.

**/
VOID
CpuCacheInfoPrintCpuCacheInfoTable (
  IN CPU_CACHE_INFO         *CpuCacheInfo,
  IN UINTN                  CpuCacheInfoCount
  )
{
  UINTN                     Index;

  DEBUG ((DEBUG_INFO, "+-------+--------------------------------------------------------------------------------------+\n"));
  DEBUG ((DEBUG_INFO, "| Index | Packge  CoreType  CacheLevel  CacheType  CacheWays (FA|DM) CacheSizeinKB  CacheCount |\n"));
  DEBUG ((DEBUG_INFO, "+-------+--------------------------------------------------------------------------------------+\n"));

  for (Index = 0; Index < CpuCacheInfoCount; Index++) {
    DEBUG ((DEBUG_INFO, "| %4x  | %4x       %2x        %2x          %2x       %4x     ( %x| %x) %8x         %4x     |\n",
        Index, CpuCacheInfo[Index].Package, CpuCacheInfo[Index].CoreType, CpuCacheInfo[Index].CacheLevel,
        CpuCacheInfo[Index].CacheType, CpuCacheInfo[Index].CacheWays, CpuCacheInfo[Index].FullyAssociativeCache,
        CpuCacheInfo[Index].DirectMappedCache, CpuCacheInfo[Index].CacheSizeinKB, CpuCacheInfo[Index].CacheCount));
  }

  DEBUG ((DEBUG_INFO, "+-------+--------------------------------------------------------------------------------------+\n"));
}

/**
  Function to compare CPU package ID, core type, cache level and cache type for use in QuickSort.

  @param[in]  Buffer1             pointer to CPU_CACHE_INFO poiner to compare
  @param[in]  Buffer2             pointer to second CPU_CACHE_INFO pointer to compare

  @retval  0                      Buffer1 equal to Buffer2
  @retval  1                      Buffer1 is greater than Buffer2
  @retval  -1                     Buffer1 is less than Buffer2
**/
INTN
EFIAPI
CpuCacheInfoCompare (
  IN CONST VOID             *Buffer1,
  IN CONST VOID             *Buffer2
  )
{
  CPU_CACHE_INFO_COMPARATOR Comparator1, Comparator2;

  ZeroMem (&Comparator1, sizeof (Comparator1));
  ZeroMem (&Comparator2, sizeof (Comparator2));

  Comparator1.Bits.Package    = ((CPU_CACHE_INFO*)Buffer1)->Package;
  Comparator1.Bits.CoreType   = ((CPU_CACHE_INFO*)Buffer1)->CoreType;
  Comparator1.Bits.CacheLevel = ((CPU_CACHE_INFO*)Buffer1)->CacheLevel;
  Comparator1.Bits.CacheType  = ((CPU_CACHE_INFO*)Buffer1)->CacheType;

  Comparator2.Bits.Package    = ((CPU_CACHE_INFO*)Buffer2)->Package;
  Comparator2.Bits.CoreType   = ((CPU_CACHE_INFO*)Buffer2)->CoreType;
  Comparator2.Bits.CacheLevel = ((CPU_CACHE_INFO*)Buffer2)->CacheLevel;
  Comparator2.Bits.CacheType  = ((CPU_CACHE_INFO*)Buffer2)->CacheType;

  if (Comparator1.Uint64 == Comparator2.Uint64) {
    return 0;
  } else if (Comparator1.Uint64 > Comparator2.Uint64) {
    return 1;
  } else {
    return -1;
  }
}

/**
  Get the total number of package and package ID in the platform.

  @param[in]      ProcessorInfo       Pointer to the ProcessorInfo array.
  @param[in]      NumberOfProcessors  Total number of logical processors in the platform.
  @param[in, out] Package             Pointer to the Package array.

  @retval  Return the total number of package and package ID in the platform.
**/
UINT32
CpuCacheInfoGetNumberOfPackages (
  IN CPUID_PROCESSOR_INFO   *ProcessorInfo,
  IN UINTN                  NumberOfProcessors,
  IN OUT UINT32             *Package
  )
{
  UINTN                     ProcessorIndex;
  UINT32                    PackageIndex;
  UINT32                    PackageCount;
  UINT32                    CurrentPackage;

  PackageCount = 0;

  for (ProcessorIndex = 0; ProcessorIndex < NumberOfProcessors; ProcessorIndex++) {
    CurrentPackage = ProcessorInfo[ProcessorIndex].Package;

    //
    // For the package that already exists in Package array, break out the loop.
    //
    for (PackageIndex = 0; PackageIndex < PackageCount; PackageIndex++) {
      if (CurrentPackage == Package[PackageIndex]) {
        break;
      }
    }

    //
    // For the new package, save it in Package array.
    //
    if (PackageIndex == PackageCount) {
      ASSERT (PackageCount < MAX_NUM_OF_PACKAGE);
      Package[PackageCount++] = CurrentPackage;
    }
  }

  return PackageCount;
}

/**
  Get the number of CoreType of requested package.

  @param[in]  ProcessorInfo       Pointer to the ProcessorInfo array.
  @param[in]  NumberOfProcessors  Total number of logical processors in the platform.
  @param[in]  Package             The requested package number.

  @retval  Return the number of CoreType of requested package.
**/
UINTN
CpuCacheInfoGetNumberOfCoreTypePerPackage(
  IN CPUID_PROCESSOR_INFO   *ProcessorInfo,
  IN UINTN                  NumberOfProcessors,
  IN UINTN                  Package
  )
{
  UINTN                     ProcessorIndex;
  //
  // Core Type value comes from CPUID.1Ah.EAX[31:24].
  // So max number of core types should be MAX_UINT8.
  //
  UINT8                     CoreType[MAX_UINT8];
  UINTN                     CoreTypeIndex;
  UINTN                     CoreTypeCount;
  UINT8                     CurrentCoreType;

  //
  // CoreType array is empty.
  //
  CoreTypeCount = 0;

  for (ProcessorIndex = 0; ProcessorIndex < NumberOfProcessors; ProcessorIndex++) {
    CurrentCoreType = ProcessorInfo[ProcessorIndex].CoreType;

    if (ProcessorInfo[ProcessorIndex].Package != Package) {
      continue;
    }

    //
    // For the type that already exists in CoreType array, break out the loop.
    //
    for (CoreTypeIndex = 0; CoreTypeIndex < CoreTypeCount; CoreTypeIndex++) {
      if (CurrentCoreType == CoreType[CoreTypeIndex]) {
        break;
      }
    }

    //
    // For the new type, save it in CoreType array.
    //
    if (CoreTypeIndex == CoreTypeCount) {
      ASSERT (CoreTypeCount < MAX_UINT8);
      CoreType[CoreTypeCount++] = CurrentCoreType;
    }
  }

  return CoreTypeCount;
}

/**
  Collect core and cache information of calling processor via CPUID instructions.

  @param[in, out] Buffer              The pointer to private data buffer.
**/
VOID
EFIAPI
CpuCacheInfoCollectCoreAndCacheData (
  IN OUT VOID               *Buffer
  )
{
  UINTN                     ProcessorIndex;
  UINT32                    CpuidMaxInput;
  UINT8                     CacheParamLeafIndex;
  CPUID_CACHE_PARAMS_EAX    CacheParamEax;
  CPUID_CACHE_PARAMS_EBX    CacheParamEbx;
  UINT32                    CacheParamEcx;
  CPUID_CACHE_PARAMS_EDX    CacheParamEdx;
  CPUID_NATIVE_MODEL_ID_AND_CORE_TYPE_EAX   NativeModelIdAndCoreTypeEax;
  COLLECT_CPUID_CACHE_DATA_CONTEXT  *Context;
  CPUID_CACHE_DATA          *CacheData;

  Context = (COLLECT_CPUID_CACHE_DATA_CONTEXT *)Buffer;
  ProcessorIndex = CpuCacheInfoWhoAmI (Context->MpServices);
  CacheData = &Context->CacheData[MAX_NUM_OF_CACHE_PARAMS_LEAF * ProcessorIndex];

  AsmCpuid (CPUID_SIGNATURE, &CpuidMaxInput, NULL, NULL, NULL);

  //
  // get CoreType if CPUID_HYBRID_INFORMATION leaf is supported.
  //
  Context->ProcessorInfo[ProcessorIndex].CoreType = 0;
  if (CpuidMaxInput >= CPUID_HYBRID_INFORMATION) {
    AsmCpuidEx (CPUID_HYBRID_INFORMATION, CPUID_HYBRID_INFORMATION_MAIN_LEAF, &NativeModelIdAndCoreTypeEax.Uint32, NULL, NULL, NULL);
    Context->ProcessorInfo[ProcessorIndex].CoreType = (UINT8) NativeModelIdAndCoreTypeEax.Bits.CoreType;
  }

  //
  // cache hierarchy starts with an index value of 0.
  //
  CacheParamLeafIndex = 0;

  while (CacheParamLeafIndex < MAX_NUM_OF_CACHE_PARAMS_LEAF) {
    AsmCpuidEx (CPUID_CACHE_PARAMS, CacheParamLeafIndex, &CacheParamEax.Uint32, &CacheParamEbx.Uint32, &CacheParamEcx, &CacheParamEdx.Uint32);

    if (CacheParamEax.Bits.CacheType == 0) {
      break;
    }

    CacheData[CacheParamLeafIndex].CacheLevel            = (UINT8)CacheParamEax.Bits.CacheLevel;
    CacheData[CacheParamLeafIndex].CacheType             = (UINT8)CacheParamEax.Bits.CacheType;
    CacheData[CacheParamLeafIndex].CacheWays             = (UINT16)CacheParamEbx.Bits.Ways;
    CacheData[CacheParamLeafIndex].FullyAssociativeCache = (UINT8)CacheParamEax.Bits.FullyAssociativeCache;
    CacheData[CacheParamLeafIndex].DirectMappedCache     = (UINT8)(CacheParamEdx.Bits.ComplexCacheIndexing == 0);
    CacheData[CacheParamLeafIndex].CacheShareBits        = (UINT16)CacheParamEax.Bits.MaximumAddressableIdsForLogicalProcessors;
    CacheData[CacheParamLeafIndex].CacheSizeinKB         = (CacheParamEbx.Bits.Ways + 1) *
        (CacheParamEbx.Bits.LinePartitions + 1) * (CacheParamEbx.Bits.LineSize + 1) * (CacheParamEcx + 1) / SIZE_1KB;

    CacheParamLeafIndex++;
  }
}

/**
  Collect CacheInfo data from the CacheData.

  @param[in]      CacheData           Pointer to the CacheData array.
  @param[in]      ProcessorInfo       Pointer to the ProcessorInfo array.
  @param[in]      NumberOfProcessors  Total number of logical processors in the platform.
  @param[in, out] CacheInfo           Pointer to the CacheInfo array.
  @param[in, out] CacheInfoCount      As input, point to the length of response CacheInfo array.
                                      As output, point to the actual length of response CacheInfo array.

  @retval         EFI_SUCCESS             Function completed successfully.
  @retval         EFI_OUT_OF_RESOURCES    Required resources could not be allocated.
  @retval         EFI_BUFFER_TOO_SMALL    CacheInfoCount is too small to hold the response CacheInfo
                                          array. CacheInfoCount has been updated with the length needed
                                          to complete the request.
**/
EFI_STATUS
CpuCacheInfoCollectCpuCacheInfoData (
  IN CPUID_CACHE_DATA       *CacheData,
  IN CPUID_PROCESSOR_INFO   *ProcessorInfo,
  IN UINTN                  NumberOfProcessors,
  IN OUT CPU_CACHE_INFO     *CacheInfo,
  IN OUT UINTN              *CacheInfoCount
  )
{
  EFI_STATUS                Status;
  UINT32                    NumberOfPackage;
  UINT32                    Package[MAX_NUM_OF_PACKAGE];
  UINTN                     PackageIndex;
  UINTN                     TotalNumberOfCoreType;
  UINTN                     MaxCacheInfoCount;
  CPU_CACHE_INFO            *LocalCacheInfo;
  UINTN                     CacheInfoIndex;
  UINTN                     LocalCacheInfoCount;
  UINTN                     Index;
  UINTN                     NextIndex;
  CPU_CACHE_INFO            SortBuffer;

  //
  // Get number of Packages and Package ID.
  //
  NumberOfPackage = CpuCacheInfoGetNumberOfPackages (ProcessorInfo, NumberOfProcessors, Package);

  //
  // Get number of core types for each package and count the total number.
  // E.g. If Package1 and Package2 both have 2 core types, the total number is 4.
  //
  TotalNumberOfCoreType = 0;
  for (PackageIndex = 0; PackageIndex < NumberOfPackage; PackageIndex++) {
    TotalNumberOfCoreType += CpuCacheInfoGetNumberOfCoreTypePerPackage (ProcessorInfo, NumberOfProcessors, Package[PackageIndex]);
  }

  MaxCacheInfoCount = TotalNumberOfCoreType * MAX_NUM_OF_CACHE_PARAMS_LEAF;
  LocalCacheInfo = AllocatePages (EFI_SIZE_TO_PAGES (MaxCacheInfoCount * sizeof (*LocalCacheInfo)));
  ASSERT (LocalCacheInfo != NULL);
  if (LocalCacheInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  LocalCacheInfoCount = 0;

  for (Index = 0; Index < NumberOfProcessors * MAX_NUM_OF_CACHE_PARAMS_LEAF; Index++) {
    if (CacheData[Index].CacheSizeinKB == 0) {
      continue;
    }

    //
    // For the sharing caches, clear their CacheSize.
    //
    for (NextIndex = Index + 1; NextIndex < NumberOfProcessors * MAX_NUM_OF_CACHE_PARAMS_LEAF; NextIndex++) {
      if (CacheData[NextIndex].CacheSizeinKB == 0) {
        continue;
      }

      if (CacheData[Index].CacheLevel == CacheData[NextIndex].CacheLevel &&
          CacheData[Index].CacheType == CacheData[NextIndex].CacheType &&
          ProcessorInfo[Index / MAX_NUM_OF_CACHE_PARAMS_LEAF].Package == ProcessorInfo[NextIndex / MAX_NUM_OF_CACHE_PARAMS_LEAF].Package &&
          ProcessorInfo[Index / MAX_NUM_OF_CACHE_PARAMS_LEAF].CoreType == ProcessorInfo[NextIndex / MAX_NUM_OF_CACHE_PARAMS_LEAF].CoreType &&
          (ProcessorInfo[Index / MAX_NUM_OF_CACHE_PARAMS_LEAF].ApicId & ~CacheData[Index].CacheShareBits) ==
          (ProcessorInfo[NextIndex / MAX_NUM_OF_CACHE_PARAMS_LEAF].ApicId & ~CacheData[NextIndex].CacheShareBits)) {
        CacheData[NextIndex].CacheSizeinKB = 0; // uses the sharing cache
      }
    }

    //
    // For the cache that already exists in LocalCacheInfo, increase its CacheCount.
    //
    for (CacheInfoIndex = 0; CacheInfoIndex < LocalCacheInfoCount; CacheInfoIndex++) {
      if (LocalCacheInfo[CacheInfoIndex].Package    == ProcessorInfo[Index / MAX_NUM_OF_CACHE_PARAMS_LEAF].Package &&
          LocalCacheInfo[CacheInfoIndex].CoreType   == ProcessorInfo[Index / MAX_NUM_OF_CACHE_PARAMS_LEAF].CoreType &&
          LocalCacheInfo[CacheInfoIndex].CacheLevel == CacheData[Index].CacheLevel &&
          LocalCacheInfo[CacheInfoIndex].CacheType  == CacheData[Index].CacheType) {
        LocalCacheInfo[CacheInfoIndex].CacheCount++;
        break;
      }
    }

    //
    // For the new cache with different Package, CoreType, CacheLevel or CacheType, copy its
    // data into LocalCacheInfo buffer.
    //
    if (CacheInfoIndex == LocalCacheInfoCount) {
      ASSERT (LocalCacheInfoCount < MaxCacheInfoCount);

      LocalCacheInfo[LocalCacheInfoCount].Package               = ProcessorInfo[Index / MAX_NUM_OF_CACHE_PARAMS_LEAF].Package;
      LocalCacheInfo[LocalCacheInfoCount].CoreType              = ProcessorInfo[Index / MAX_NUM_OF_CACHE_PARAMS_LEAF].CoreType;
      LocalCacheInfo[LocalCacheInfoCount].CacheLevel            = CacheData[Index].CacheLevel;
      LocalCacheInfo[LocalCacheInfoCount].CacheType             = CacheData[Index].CacheType;
      LocalCacheInfo[LocalCacheInfoCount].CacheWays             = CacheData[Index].CacheWays;
      LocalCacheInfo[LocalCacheInfoCount].FullyAssociativeCache = CacheData[Index].FullyAssociativeCache;
      LocalCacheInfo[LocalCacheInfoCount].DirectMappedCache     = CacheData[Index].DirectMappedCache;
      LocalCacheInfo[LocalCacheInfoCount].CacheSizeinKB         = CacheData[Index].CacheSizeinKB;
      LocalCacheInfo[LocalCacheInfoCount].CacheCount            = 1;

      LocalCacheInfoCount++;
    }
  }

  if (*CacheInfoCount < LocalCacheInfoCount) {
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    //
    // Sort LocalCacheInfo array by CPU package ID, core type, cache level and cache type.
    //
    QuickSort (LocalCacheInfo, LocalCacheInfoCount, sizeof (*LocalCacheInfo), CpuCacheInfoCompare, (VOID*) &SortBuffer);
    CopyMem (CacheInfo, LocalCacheInfo, sizeof (*CacheInfo) * LocalCacheInfoCount);
    DEBUG_CODE (
      CpuCacheInfoPrintCpuCacheInfoTable (CacheInfo, LocalCacheInfoCount);
    );
    Status = EFI_SUCCESS;
  }

  *CacheInfoCount = LocalCacheInfoCount;

  FreePages (LocalCacheInfo, EFI_SIZE_TO_PAGES (MaxCacheInfoCount * sizeof (*LocalCacheInfo)));

  return Status;
}

/**
  Get CpuCacheInfo data array. The array is sorted by CPU package ID, core type, cache level and cache type.

  @param[in, out] CpuCacheInfo        Pointer to the CpuCacheInfo array.
  @param[in, out] CpuCacheInfoCount   As input, point to the length of response CpuCacheInfo array.
                                      As output, point to the actual length of response CpuCacheInfo array.

  @retval         EFI_SUCCESS             Function completed successfully.
  @retval         EFI_INVALID_PARAMETER   CpuCacheInfoCount is NULL.
  @retval         EFI_INVALID_PARAMETER   CpuCacheInfo is NULL while CpuCacheInfoCount contains the value
                                          greater than zero.
  @retval         EFI_UNSUPPORTED         Processor does not support CPUID_CACHE_PARAMS Leaf.
  @retval         EFI_OUT_OF_RESOURCES    Required resources could not be allocated.
  @retval         EFI_BUFFER_TOO_SMALL    CpuCacheInfoCount is too small to hold the response CpuCacheInfo
                                          array. CpuCacheInfoCount has been updated with the length needed
                                          to complete the request.
**/
EFI_STATUS
EFIAPI
GetCpuCacheInfo (
  IN OUT CPU_CACHE_INFO     *CpuCacheInfo,
  IN OUT UINTN              *CpuCacheInfoCount
  )
{
  EFI_STATUS                Status;
  UINT32                    CpuidMaxInput;
  UINT32                    NumberOfProcessors;
  UINTN                     CacheDataCount;
  UINTN                     ProcessorIndex;
  EFI_PROCESSOR_INFORMATION ProcessorInfo;
  COLLECT_CPUID_CACHE_DATA_CONTEXT  Context;

  if (CpuCacheInfoCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*CpuCacheInfoCount != 0 && CpuCacheInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AsmCpuid (CPUID_SIGNATURE, &CpuidMaxInput, NULL, NULL, NULL);
  if (CpuidMaxInput < CPUID_CACHE_PARAMS) {
    return EFI_UNSUPPORTED;
  }

  //
  // Initialize COLLECT_CPUID_CACHE_DATA_CONTEXT.MpServices.
  //
  CpuCacheInfoGetMpServices (&Context.MpServices);

  NumberOfProcessors = CpuCacheInfoGetNumberOfProcessors (Context.MpServices);

  //
  // Initialize COLLECT_CPUID_CACHE_DATA_CONTEXT.ProcessorInfo.
  //
  Context.ProcessorInfo = AllocatePages (EFI_SIZE_TO_PAGES (NumberOfProcessors * sizeof (*Context.ProcessorInfo)));
  ASSERT (Context.ProcessorInfo != NULL);
  if (Context.ProcessorInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Initialize COLLECT_CPUID_CACHE_DATA_CONTEXT.CacheData.
  // CacheData array consists of CPUID_CACHE_DATA data structure for each Cpuid Cache Parameter Leaf
  // per logical processor. The array begin with data of each Cache Parameter Leaf of processor 0, followed
  // by data of each Cache Parameter Leaf of processor 1 ...
  //
  CacheDataCount = NumberOfProcessors * MAX_NUM_OF_CACHE_PARAMS_LEAF;
  Context.CacheData = AllocatePages (EFI_SIZE_TO_PAGES (CacheDataCount * sizeof (*Context.CacheData)));
  ASSERT (Context.CacheData != NULL);
  if (Context.CacheData == NULL) {
    FreePages (Context.ProcessorInfo, EFI_SIZE_TO_PAGES (NumberOfProcessors * sizeof (*Context.ProcessorInfo)));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Context.CacheData, CacheDataCount * sizeof (*Context.CacheData));

  //
  // Collect Package ID and APIC ID of all processors.
  //
  for (ProcessorIndex = 0; ProcessorIndex < NumberOfProcessors; ProcessorIndex++) {
    CpuCacheInfoGetProcessorInfo (Context.MpServices, ProcessorIndex, &ProcessorInfo);
    Context.ProcessorInfo[ProcessorIndex].Package = ProcessorInfo.Location.Package;
    Context.ProcessorInfo[ProcessorIndex].ApicId  = (UINT32) ProcessorInfo.ProcessorId;
  }

  //
  // Wakeup all processors for CacheData(core type and cache data) collection.
  //
  CpuCacheInfoStartupAllCPUs (Context.MpServices, CpuCacheInfoCollectCoreAndCacheData, &Context);

  //
  // Collect CpuCacheInfo data from CacheData.
  //
  Status = CpuCacheInfoCollectCpuCacheInfoData (Context.CacheData, Context.ProcessorInfo, NumberOfProcessors, CpuCacheInfo, CpuCacheInfoCount);

  FreePages (Context.CacheData, EFI_SIZE_TO_PAGES (CacheDataCount * sizeof (*Context.CacheData)));
  FreePages (Context.ProcessorInfo, EFI_SIZE_TO_PAGES (NumberOfProcessors * sizeof (*Context.ProcessorInfo)));

  return Status;
}
