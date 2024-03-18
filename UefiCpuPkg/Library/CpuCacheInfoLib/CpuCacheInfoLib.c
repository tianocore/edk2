/** @file
  Provides cache info for each package, core type, cache level and cache type.

  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCpuCacheInfoLib.h"

/**
  Print CpuidCacheData array.

  @param[in]  CpuidCacheData      Pointer to the CpuidCacheData array.
  @param[in]  CpuidCacheDataCount The length of CpuidCacheData array.

**/
VOID
PrintCpuidCacheDataTable (
  IN CPUID_CACHE_DATA       *CpuidCacheData,
  IN UINTN                  CpuidCacheDataCount
  )
{
  UINTN                     Index;

#if 0
  DEBUG ((DEBUG_INFO, "+-------+----------------------------------------------------------------------+\n"));
  DEBUG ((DEBUG_INFO, "| Index | ApicId (Pkg/Core/Thread) CoreType Level&Type Ways ShareBits SizeinKB |\n"));
  DEBUG ((DEBUG_INFO, "+-------+----------------------------------------------------------------------+\n"));

  for (Index = 0; Index < CpuidCacheDataCount; Index++) {
    if (Index % MAX_NUM_OF_CACHE_PARAMS_LEAF == 0) {
      DEBUG ((DEBUG_INFO, "|*"));
    } else {
      DEBUG ((DEBUG_INFO, "| "));
    }

    DEBUG ((DEBUG_INFO, "%4x  |  %4x  %4x/%4x/%4x       %2x     %2x    %2x  %3x     %3x   %8x  |\n", Index, \
        CpuidCacheData[Index].ApicId, CpuidCacheData[Index].Location.Package, CpuidCacheData[Index].Location.Core, \
        CpuidCacheData[Index].Location.Thread, CpuidCacheData[Index].CoreType, CpuidCacheData[Index].CacheLevel, \
        CpuidCacheData[Index].CacheType, CpuidCacheData[Index].CacheWays, CpuidCacheData[Index].CacheShareBits, \
        CpuidCacheData[Index].CacheSizeinKB));
  }

  DEBUG ((DEBUG_INFO, "+-------+----------------------------------------------------------------------+\n"));
#else
  DEBUG ((DEBUG_INFO, "+-------+----------------------------------------------------------------------+\n"));
  DEBUG ((DEBUG_INFO, "| Index | Level&Type Ways ShareBits SizeinKB |\n"));
  DEBUG ((DEBUG_INFO, "+-------+----------------------------------------------------------------------+\n"));

  for (Index = 0; Index < CpuidCacheDataCount; Index++) {
    if (Index % MAX_NUM_OF_CACHE_PARAMS_LEAF == 0) {
      DEBUG ((DEBUG_INFO, "|*"));
    } else {
      DEBUG ((DEBUG_INFO, "| "));
    }

    DEBUG ((DEBUG_INFO, "%4x  |       %2x    %2x  %3x     %3x   %8x  |\n", Index, \
        CpuidCacheData[Index].CacheLevel, \
        CpuidCacheData[Index].CacheType, CpuidCacheData[Index].CacheWays, CpuidCacheData[Index].CacheShareBits, \
        CpuidCacheData[Index].CacheSizeinKB));
  }

  DEBUG ((DEBUG_INFO, "+-------+----------------------------------------------------------------------+\n"));
#endif
}


/**
  Print CpuCacheInfo array.

  @param[in]  CpuCacheInfo        Pointer to the CpuCacheInfo array.
  @param[in]  CpuCacheInfoCount   The length of CpuCacheInfo array.

**/
VOID
CpuCacheInfoPrintCpuCacheInfoTable (
  IN CPU_CACHE_INFO  *CpuCacheInfo,
  IN UINTN           CpuCacheInfoCount
  )
{
  UINTN  Index;

  DEBUG ((DEBUG_INFO, "+-------+--------------------------------------------------------------------------------------+\n"));
  DEBUG ((DEBUG_INFO, "| Index | Packge  CoreType  CacheLevel  CacheType  CacheWays (FA|DM) CacheSizeinKB  CacheCount |\n"));
  DEBUG ((DEBUG_INFO, "+-------+--------------------------------------------------------------------------------------+\n"));

  for (Index = 0; Index < CpuCacheInfoCount; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "| %4x  | %4x       %2x        %2x          %2x       %4x     ( %x| %x) %8x         %4x     |\n",
      Index,
      CpuCacheInfo[Index].Package,
      CpuCacheInfo[Index].CoreType,
      CpuCacheInfo[Index].CacheLevel,
      CpuCacheInfo[Index].CacheType,
      CpuCacheInfo[Index].CacheWays,
      CpuCacheInfo[Index].FullyAssociativeCache,
      CpuCacheInfo[Index].DirectMappedCache,
      CpuCacheInfo[Index].CacheSizeinKB,
      CpuCacheInfo[Index].CacheCount
      ));
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
  IN CONST VOID  *Buffer1,
  IN CONST VOID  *Buffer2
  )
{
  CPU_CACHE_INFO_COMPARATOR  Comparator1, Comparator2;

  ZeroMem (&Comparator1, sizeof (Comparator1));
  ZeroMem (&Comparator2, sizeof (Comparator2));

  Comparator1.Bits.Package    = ((CPU_CACHE_INFO *)Buffer1)->Package;
  Comparator1.Bits.CoreType   = ((CPU_CACHE_INFO *)Buffer1)->CoreType;
  Comparator1.Bits.CacheLevel = ((CPU_CACHE_INFO *)Buffer1)->CacheLevel;
  Comparator1.Bits.CacheType  = ((CPU_CACHE_INFO *)Buffer1)->CacheType;

  Comparator2.Bits.Package    = ((CPU_CACHE_INFO *)Buffer2)->Package;
  Comparator2.Bits.CoreType   = ((CPU_CACHE_INFO *)Buffer2)->CoreType;
  Comparator2.Bits.CacheLevel = ((CPU_CACHE_INFO *)Buffer2)->CacheLevel;
  Comparator2.Bits.CacheType  = ((CPU_CACHE_INFO *)Buffer2)->CacheType;

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
  IN CPUID_PROCESSOR_INFO  *ProcessorInfo,
  IN UINTN                 NumberOfProcessors,
  IN OUT UINT32            *Package
  )
{
  UINTN   ProcessorIndex;
  UINT32  PackageIndex;
  UINT32  PackageCount;
  UINT32  CurrentPackage;

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
CpuCacheInfoGetNumberOfCoreTypePerPackage (
  IN CPUID_PROCESSOR_INFO  *ProcessorInfo,
  IN UINTN                 NumberOfProcessors,
  IN UINTN                 Package
  )
{
  UINTN  ProcessorIndex;
  //
  // Core Type value comes from CPUID.1Ah.EAX[31:24].
  // So max number of core types should be MAX_UINT8.
  //
  UINT8  CoreType[MAX_UINT8];
  UINTN  CoreTypeIndex;
  UINTN  CoreTypeCount;
  UINT8  CurrentCoreType;

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
  IN OUT VOID  *Buffer
  )
{
  UINTN                                    ProcessorIndex;
  UINT32                                   CpuidMaxInput;
  UINT8                                    CacheParamLeafIndex;
  CPUID_CACHE_PARAMS_EAX                   CacheParamEax;
  CPUID_CACHE_PARAMS_EBX                   CacheParamEbx;
  UINT32                                   CacheParamEcx;
  CPUID_CACHE_PARAMS_EDX                   CacheParamEdx;
  CPUID_NATIVE_MODEL_ID_AND_CORE_TYPE_EAX  NativeModelIdAndCoreTypeEax;
  COLLECT_CPUID_CACHE_DATA_CONTEXT         *Context;
  CPUID_CACHE_DATA                         *CacheData;

  Context        = (COLLECT_CPUID_CACHE_DATA_CONTEXT *)Buffer;
  ProcessorIndex = CpuCacheInfoWhoAmI (Context->MpServices);
  CacheData      = &Context->CacheData[MAX_NUM_OF_CACHE_PARAMS_LEAF * ProcessorIndex];

  AsmCpuid (CPUID_SIGNATURE, &CpuidMaxInput, NULL, NULL, NULL);

  //
  // get CoreType if CPUID_HYBRID_INFORMATION leaf is supported.
  //
  Context->ProcessorInfo[ProcessorIndex].CoreType = 0;
  if (CpuidMaxInput >= CPUID_HYBRID_INFORMATION) {
    AsmCpuidEx (CPUID_HYBRID_INFORMATION, CPUID_HYBRID_INFORMATION_MAIN_LEAF, &NativeModelIdAndCoreTypeEax.Uint32, NULL, NULL, NULL);
    Context->ProcessorInfo[ProcessorIndex].CoreType = (UINT8)NativeModelIdAndCoreTypeEax.Bits.CoreType;
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

  CacheData[0].CacheParamLeafCount = CacheParamLeafIndex;
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
  IN CPUID_CACHE_DATA      *CacheData,
  IN CPUID_PROCESSOR_INFO  *ProcessorInfo,
  IN UINTN                 NumberOfProcessors,
  IN OUT CPU_CACHE_INFO    *CacheInfo,
  IN OUT UINTN             *CacheInfoCount
  )
{
  CPU_CACHE_INFO  *LocalCacheInfo;
  UINTN           LocalCacheInfoIndex;
  UINTN           OldLocalCacheInfoIndex;
  UINTN           LocalCacheInfoCount;
  UINTN           Index;
  UINTN           NextIndex;
  UINTN           Index2;
  UINT8           CacheParamLeafIndex;
  BOOLEAN         *CacheDataShareFlagArray;
  BOOLEAN         *ProcessorCheckedFlagArray;
  UINTN           ProcessorNumber;
  UINTN           *ProcessorNumberArray;
  UINTN           ProcessorNumberArrayCount;

  ProcessorCheckedFlagArray = AllocatePages (EFI_SIZE_TO_PAGES (NumberOfProcessors * (sizeof (*ProcessorCheckedFlagArray) + sizeof (*ProcessorNumberArray))));
  ASSERT (ProcessorCheckedFlagArray != NULL);
  if (ProcessorCheckedFlagArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  ZeroMem (ProcessorCheckedFlagArray, NumberOfProcessors * (sizeof (*ProcessorCheckedFlagArray) + sizeof (*ProcessorNumberArray)));

  //
  // Find different sets of CPU cache info.
  //
  ProcessorNumberArray = (UINTN *)&ProcessorCheckedFlagArray[NumberOfProcessors];
  ProcessorNumberArrayCount = 0;
  for (Index = 0; Index < NumberOfProcessors - 1; Index++) {
    if (ProcessorCheckedFlagArray[Index]) {
      continue;
    }

    //
    // Save the 1st processor number(Index) with the same set of cache info.
    //
    ProcessorNumberArray[ProcessorNumberArrayCount++] = Index;

    for (NextIndex = Index + 1; NextIndex < NumberOfProcessors; NextIndex++) {
      if (ProcessorCheckedFlagArray[NextIndex]) {
        continue;
      }

      if ((ProcessorInfo[Index].Package == ProcessorInfo[NextIndex].Package) &&
          (ProcessorInfo[Index].CoreType == ProcessorInfo[NextIndex].CoreType) &&
          (CacheData[Index * MAX_NUM_OF_CACHE_PARAMS_LEAF].CacheParamLeafCount == CacheData[NextIndex * MAX_NUM_OF_CACHE_PARAMS_LEAF].CacheParamLeafCount) &&
          (CompareMem (&(CacheData[Index * MAX_NUM_OF_CACHE_PARAMS_LEAF]), &(CacheData[NextIndex * MAX_NUM_OF_CACHE_PARAMS_LEAF]),
            CacheData[Index * MAX_NUM_OF_CACHE_PARAMS_LEAF].CacheParamLeafCount * sizeof (*CacheData)) == 0)) {
        //
        // Set the flag if processor has the same set of cache info to speed up the computation.
        //
        ProcessorCheckedFlagArray[NextIndex] = TRUE;
      }
    }
  }

  //
  // Count how many CPU_CACHE_INFO entries are needed.
  //
  LocalCacheInfoCount = 0;
  for (Index = 0; Index < ProcessorNumberArrayCount; Index++) {
    DEBUG ((DEBUG_INFO, "[%a] ProcessorNumberArray[%d] = 0x%x\n", __func__, Index, ProcessorNumberArray[Index]));
    ProcessorNumber = ProcessorNumberArray[Index];
    LocalCacheInfoCount += CacheData[ProcessorNumber * MAX_NUM_OF_CACHE_PARAMS_LEAF].CacheParamLeafCount;
  }
  DEBUG ((DEBUG_INFO, "[%a] LocalCacheInfoCount = 0x%x\n", __func__, LocalCacheInfoCount));

  if (*CacheInfoCount < LocalCacheInfoCount) {
    *CacheInfoCount = LocalCacheInfoCount;
    FreePages (ProcessorCheckedFlagArray, EFI_SIZE_TO_PAGES (NumberOfProcessors * (sizeof (*ProcessorCheckedFlagArray) + sizeof (*ProcessorNumberArray))));
    return EFI_BUFFER_TOO_SMALL;
  }

  CacheDataShareFlagArray = AllocatePages (EFI_SIZE_TO_PAGES (NumberOfProcessors * sizeof (*CacheDataShareFlagArray) * MAX_NUM_OF_CACHE_PARAMS_LEAF + LocalCacheInfoCount * sizeof (*LocalCacheInfo)));
  ASSERT (CacheDataShareFlagArray != NULL);
  if (CacheDataShareFlagArray == NULL) {
    FreePages (ProcessorCheckedFlagArray, EFI_SIZE_TO_PAGES (NumberOfProcessors * (sizeof (*ProcessorCheckedFlagArray) + sizeof (*ProcessorNumberArray))));
    return EFI_OUT_OF_RESOURCES;
  }
  ZeroMem (CacheDataShareFlagArray, NumberOfProcessors * sizeof (*CacheDataShareFlagArray) * MAX_NUM_OF_CACHE_PARAMS_LEAF + LocalCacheInfoCount * sizeof (*LocalCacheInfo));

  //
  // Save CPU cache share state.
  //
  for (Index = 0; Index < (NumberOfProcessors - 1) * MAX_NUM_OF_CACHE_PARAMS_LEAF; Index++) {
    if (CacheData[Index].CacheLevel == 0 || CacheData[Index].CacheType == 0 || CacheDataShareFlagArray[Index]) {
      continue;
    }

    //
    // Compare the cache info of the current processor with the info of the next processor.
    //
    for (NextIndex = (Index / MAX_NUM_OF_CACHE_PARAMS_LEAF + 1) * MAX_NUM_OF_CACHE_PARAMS_LEAF; NextIndex < NumberOfProcessors * MAX_NUM_OF_CACHE_PARAMS_LEAF; NextIndex++) {
      if (CacheData[NextIndex].CacheLevel == 0 || CacheData[NextIndex].CacheType == 0 || CacheDataShareFlagArray[NextIndex]) {
        continue;
      }

      if ((CacheData[Index].CacheLevel == CacheData[NextIndex].CacheLevel) &&
          (CacheData[Index].CacheType == CacheData[NextIndex].CacheType) &&
          //
          // "Package" check is needed. E.g. (ApicId & ~ShareBits) of Package1 may be equal to (ApicId & ~ShareBits) of Package2.
          //
          (ProcessorInfo[Index / MAX_NUM_OF_CACHE_PARAMS_LEAF].Package == ProcessorInfo[NextIndex / MAX_NUM_OF_CACHE_PARAMS_LEAF].Package) &&
          ((ProcessorInfo[Index / MAX_NUM_OF_CACHE_PARAMS_LEAF].ApicId & ~CacheData[Index].CacheShareBits) == (ProcessorInfo[NextIndex / MAX_NUM_OF_CACHE_PARAMS_LEAF].ApicId & ~CacheData[NextIndex].CacheShareBits)))
      {
        //
        // Set the flag if processor uses the sharing cache.
        //
        CacheDataShareFlagArray[NextIndex] = TRUE;
      }
    }
  }

  //
  // Statistics for each level and type of cache info.
  //
  ZeroMem (ProcessorCheckedFlagArray, NumberOfProcessors * (sizeof (*ProcessorCheckedFlagArray)));
  LocalCacheInfo = (CPU_CACHE_INFO *)&CacheDataShareFlagArray[NumberOfProcessors * sizeof (*CacheDataShareFlagArray) * MAX_NUM_OF_CACHE_PARAMS_LEAF];
  LocalCacheInfoIndex = 0;
  for (Index = 0; Index < ProcessorNumberArrayCount; Index++) {
    ProcessorNumber = ProcessorNumberArray[Index];
    OldLocalCacheInfoIndex = LocalCacheInfoIndex;
    for (CacheParamLeafIndex = 0; CacheParamLeafIndex < CacheData[ProcessorNumber * MAX_NUM_OF_CACHE_PARAMS_LEAF].CacheParamLeafCount; CacheParamLeafIndex++) {
      ASSERT (LocalCacheInfoIndex < LocalCacheInfoCount);
      LocalCacheInfo[LocalCacheInfoIndex].Package               = ProcessorInfo[ProcessorNumber].Package;
      LocalCacheInfo[LocalCacheInfoIndex].CoreType              = ProcessorInfo[ProcessorNumber].CoreType;
      LocalCacheInfo[LocalCacheInfoIndex].CacheLevel            = CacheData[ProcessorNumber * MAX_NUM_OF_CACHE_PARAMS_LEAF + CacheParamLeafIndex].CacheLevel;
      LocalCacheInfo[LocalCacheInfoIndex].CacheType             = CacheData[ProcessorNumber * MAX_NUM_OF_CACHE_PARAMS_LEAF + CacheParamLeafIndex].CacheType;
      LocalCacheInfo[LocalCacheInfoIndex].CacheWays             = CacheData[ProcessorNumber * MAX_NUM_OF_CACHE_PARAMS_LEAF + CacheParamLeafIndex].CacheWays;
      LocalCacheInfo[LocalCacheInfoIndex].FullyAssociativeCache = CacheData[ProcessorNumber * MAX_NUM_OF_CACHE_PARAMS_LEAF + CacheParamLeafIndex].FullyAssociativeCache;
      LocalCacheInfo[LocalCacheInfoIndex].DirectMappedCache     = CacheData[ProcessorNumber * MAX_NUM_OF_CACHE_PARAMS_LEAF + CacheParamLeafIndex].DirectMappedCache;
      LocalCacheInfo[LocalCacheInfoIndex].CacheSizeinKB         = CacheData[ProcessorNumber * MAX_NUM_OF_CACHE_PARAMS_LEAF + CacheParamLeafIndex].CacheSizeinKB;
      LocalCacheInfo[LocalCacheInfoIndex].CacheCount            = !CacheDataShareFlagArray[ProcessorNumber * MAX_NUM_OF_CACHE_PARAMS_LEAF + CacheParamLeafIndex];
      LocalCacheInfoIndex++;
    }

    DEBUG ((DEBUG_INFO, "[%a] ProcessorNumber = 0x%x, LocalCacheInfoIndex = %d, OldLocalCacheInfoIndex = %d\n", __func__, ProcessorNumber, LocalCacheInfoIndex, OldLocalCacheInfoIndex));

    for (Index2 = 1; Index2 < NumberOfProcessors; Index2++) {
      if ((Index2 == ProcessorNumber) || (ProcessorCheckedFlagArray[Index2])) {
        continue;
      }

      if ((ProcessorInfo[Index2].Package == ProcessorInfo[ProcessorNumber].Package) &&
          (ProcessorInfo[Index2].CoreType == ProcessorInfo[ProcessorNumber].CoreType) &&
          (CacheData[Index2 * MAX_NUM_OF_CACHE_PARAMS_LEAF].CacheParamLeafCount == CacheData[ProcessorNumber * MAX_NUM_OF_CACHE_PARAMS_LEAF].CacheParamLeafCount) &&
          (CompareMem (&(CacheData[Index2 * MAX_NUM_OF_CACHE_PARAMS_LEAF]), &(CacheData[ProcessorNumber * MAX_NUM_OF_CACHE_PARAMS_LEAF]),
            CacheData[Index2 * MAX_NUM_OF_CACHE_PARAMS_LEAF].CacheParamLeafCount * sizeof (*CacheData)) == 0)) {
        for (CacheParamLeafIndex = 0; CacheParamLeafIndex < CacheData[Index2 * MAX_NUM_OF_CACHE_PARAMS_LEAF].CacheParamLeafCount; CacheParamLeafIndex++) {
          if (!CacheDataShareFlagArray[Index2 * MAX_NUM_OF_CACHE_PARAMS_LEAF + CacheParamLeafIndex]) {
            //
            // CacheCount increases by one if processor has the same set of cache info, and does not use the sharing cache.
            //
            LocalCacheInfo[OldLocalCacheInfoIndex + CacheParamLeafIndex].CacheCount++;
          }
        }

        //
        // Set the flag if processor has the same set of cache info to speed up the computation.
        //
        ProcessorCheckedFlagArray[Index2] = TRUE;
      }
    }
  }

  CopyMem (CacheInfo, LocalCacheInfo, sizeof (*CacheInfo) * LocalCacheInfoCount);
  DEBUG_CODE (
    CpuCacheInfoPrintCpuCacheInfoTable (CacheInfo, LocalCacheInfoCount);
    );

  *CacheInfoCount = LocalCacheInfoCount;

  FreePages (CacheDataShareFlagArray, EFI_SIZE_TO_PAGES (NumberOfProcessors * sizeof (*CacheDataShareFlagArray) * MAX_NUM_OF_CACHE_PARAMS_LEAF + LocalCacheInfoCount * sizeof (*LocalCacheInfo)));
  FreePages (ProcessorCheckedFlagArray, EFI_SIZE_TO_PAGES (NumberOfProcessors * (sizeof (*ProcessorCheckedFlagArray) + sizeof (*ProcessorNumberArray))));

  return EFI_SUCCESS;
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
  IN OUT CPU_CACHE_INFO  *CpuCacheInfo,
  IN OUT UINTN           *CpuCacheInfoCount
  )
{
  EFI_STATUS                        Status;
  UINT32                            CpuidMaxInput;
  UINT32                            NumberOfProcessors;
  UINTN                             CacheDataCount;
  UINTN                             ProcessorIndex;
  EFI_PROCESSOR_INFORMATION         ProcessorInfo;
  COLLECT_CPUID_CACHE_DATA_CONTEXT  Context;

  if (CpuCacheInfoCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*CpuCacheInfoCount != 0) && (CpuCacheInfo == NULL)) {
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
  CacheDataCount    = NumberOfProcessors * MAX_NUM_OF_CACHE_PARAMS_LEAF;
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
    Context.ProcessorInfo[ProcessorIndex].ApicId  = (UINT32)ProcessorInfo.ProcessorId;
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
