/** @file
  CPU Register Table Library functions.

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RegisterCpuFeatures.h"

/**
  Checks if two CPU feature bit masks are equal.

  @param[in]  FirstFeatureMask  The first input CPU feature bit mask
  @param[in]  SecondFeatureMask The second input CPU feature bit mask

  @retval TRUE  Two CPU feature bit masks are equal.
  @retval FALSE Two CPU feature bit masks are not equal.
**/
BOOLEAN
IsCpuFeatureMatch (
  IN UINT8               *FirstFeatureMask,
  IN UINT8               *SecondFeatureMask
  )
{
  UINTN                 BitMaskSize;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSetting);
  if (CompareMem (FirstFeatureMask, SecondFeatureMask, BitMaskSize) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Function that uses DEBUG() macros to display the contents of a a CPU feature bit mask.

  @param[in]  FeatureMask  A pointer to the CPU feature bit mask.
**/
VOID
DumpCpuFeatureMask (
  IN UINT8               *FeatureMask
  )
{
  UINTN                  Index;
  UINT8                  *Data8;
  UINTN                  BitMaskSize;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSetting);
  Data8       = (UINT8 *) FeatureMask;
  for (Index = 0; Index < BitMaskSize; Index++) {
    DEBUG ((DEBUG_INFO, " %02x ", *Data8++));
  }
  DEBUG ((DEBUG_INFO, "\n"));
}

/**
  Dump CPU feature name or CPU feature bit mask.

  @param[in]  CpuFeature   Pointer to CPU_FEATURES_ENTRY
**/
VOID
DumpCpuFeature (
  IN CPU_FEATURES_ENTRY  *CpuFeature
  )
{

  if (CpuFeature->FeatureName != NULL) {
    DEBUG ((DEBUG_INFO, "FeatureName: %a\n", CpuFeature->FeatureName));
  } else {
    DEBUG ((DEBUG_INFO, "FeatureMask = "));
    DumpCpuFeatureMask (CpuFeature->FeatureMask);
  }
}

/**
  Determines if the feature bit mask is in dependent CPU feature bit mask buffer.

  @param[in]  FeatureMask        Pointer to CPU feature bit mask
  @param[in]  DependentBitMask   Pointer to dependent CPU feature bit mask buffer

  @retval TRUE  The feature bit mask is in dependent CPU feature bit mask buffer.
  @retval FALSE The feature bit mask is not in dependent CPU feature bit mask buffer.
**/
BOOLEAN
IsBitMaskMatchCheck (
  IN UINT8        *FeatureMask,
  IN UINT8        *DependentBitMask
  )
{
  UINTN      Index;
  UINTN      BitMaskSize;
  UINT8      *Data1;
  UINT8      *Data2;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSetting);

  Data1 = FeatureMask;
  Data2 = DependentBitMask;
  for (Index = 0; Index < BitMaskSize; Index++) {
    if (((*(Data1++)) & (*(Data2++))) != 0) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
  Try to find the specify cpu featuren in former/after feature list.

  @param[in]  FeatureList        Pointer to dependent CPU feature list
  @param[in]  CurrentEntry       Pointer to current CPU feature entry.
  @param[in]  SearchFormer       Find in former feature or after features.
  @param[in]  FeatureMask        Pointer to CPU feature bit mask

  @retval TRUE  The feature bit mask is in dependent CPU feature bit mask buffer.
  @retval FALSE The feature bit mask is not in dependent CPU feature bit mask buffer.
**/
BOOLEAN
FindSpecifyFeature (
  IN LIST_ENTRY              *FeatureList,
  IN LIST_ENTRY              *CurrentEntry,
  IN BOOLEAN                 SearchFormer,
  IN UINT8                   *FeatureMask
  )
{
  CPU_FEATURES_ENTRY         *CpuFeature;
  LIST_ENTRY                 *NextEntry;

  //
  // Check whether exist the not neighborhood entry first.
  // If not exist, return FALSE means not found status.
  //
  if (SearchFormer) {
    NextEntry = CurrentEntry->BackLink;
    if (IsNull (FeatureList, NextEntry)) {
      return FALSE;
    }

    NextEntry = NextEntry->BackLink;
    if (IsNull (FeatureList, NextEntry)) {
      return FALSE;
    }

    NextEntry = CurrentEntry->BackLink->BackLink;
  } else {
    NextEntry = CurrentEntry->ForwardLink;
    if (IsNull (FeatureList, NextEntry)) {
      return FALSE;
    }

    NextEntry = NextEntry->ForwardLink;
    if (IsNull (FeatureList, NextEntry)) {
      return FALSE;
    }

    NextEntry = CurrentEntry->ForwardLink->ForwardLink;
  }

  while (!IsNull (FeatureList, NextEntry)) {
    CpuFeature = CPU_FEATURE_ENTRY_FROM_LINK (NextEntry);

    if (IsBitMaskMatchCheck (FeatureMask, CpuFeature->FeatureMask)) {
      return TRUE;
    }

    if (SearchFormer) {
      NextEntry = NextEntry->BackLink;
    } else {
      NextEntry = NextEntry->ForwardLink;
    }
  }

  return FALSE;
}

/**
  Return feature dependence result.

  @param[in]  CpuFeature            Pointer to CPU feature.
  @param[in]  Before                Check before dependence or after.
  @param[in]  NextCpuFeatureMask    Pointer to next CPU feature Mask.

  @retval     return the dependence result.
**/
CPU_FEATURE_DEPENDENCE_TYPE
DetectFeatureScope (
  IN CPU_FEATURES_ENTRY         *CpuFeature,
  IN BOOLEAN                    Before,
  IN UINT8                      *NextCpuFeatureMask
  )
{
  //
  // if need to check before type dependence but the feature after current feature is not
  // exist, means this before type dependence not valid, just return NoneDepType.
  // Just like Feature A has a dependence of feature B, but Feature B not installed, so
  // Feature A maybe insert to the last entry of the list. In this case, for below code,
  // Featrure A has depend of feature B, but it is the last entry of the list, so the
  // NextCpuFeatureMask is NULL, so the dependence for feature A here is useless and code
  // just return NoneDepType.
  //
  if (NextCpuFeatureMask == NULL) {
    return NoneDepType;
  }

  if (Before) {
    if ((CpuFeature->PackageBeforeFeatureBitMask != NULL) &&
        IsBitMaskMatchCheck (NextCpuFeatureMask, CpuFeature->PackageBeforeFeatureBitMask)) {
      return PackageDepType;
    }

    if ((CpuFeature->CoreBeforeFeatureBitMask != NULL) &&
        IsBitMaskMatchCheck (NextCpuFeatureMask, CpuFeature->CoreBeforeFeatureBitMask)) {
      return CoreDepType;
    }

    if ((CpuFeature->BeforeFeatureBitMask != NULL) &&
        IsBitMaskMatchCheck (NextCpuFeatureMask, CpuFeature->BeforeFeatureBitMask)) {
      return ThreadDepType;
    }

    return NoneDepType;
  }

  if ((CpuFeature->PackageAfterFeatureBitMask != NULL) &&
      IsBitMaskMatchCheck (NextCpuFeatureMask, CpuFeature->PackageAfterFeatureBitMask)) {
    return PackageDepType;
  }

  if ((CpuFeature->CoreAfterFeatureBitMask != NULL) &&
      IsBitMaskMatchCheck (NextCpuFeatureMask, CpuFeature->CoreAfterFeatureBitMask)) {
    return CoreDepType;
  }

  if ((CpuFeature->AfterFeatureBitMask != NULL) &&
      IsBitMaskMatchCheck (NextCpuFeatureMask, CpuFeature->AfterFeatureBitMask)) {
    return ThreadDepType;
  }

  return NoneDepType;
}

/**
  Return feature dependence result.

  @param[in]  CpuFeature            Pointer to CPU feature.
  @param[in]  Before                Check before dependence or after.
  @param[in]  FeatureList           Pointer to CPU feature list.

  @retval     return the dependence result.
**/
CPU_FEATURE_DEPENDENCE_TYPE
DetectNoneNeighborhoodFeatureScope (
  IN CPU_FEATURES_ENTRY         *CpuFeature,
  IN BOOLEAN                    Before,
  IN LIST_ENTRY                 *FeatureList
  )
{
  if (Before) {
    if ((CpuFeature->PackageBeforeFeatureBitMask != NULL) &&
        FindSpecifyFeature(FeatureList, &CpuFeature->Link, FALSE, CpuFeature->PackageBeforeFeatureBitMask)) {
      return PackageDepType;
    }

    if ((CpuFeature->CoreBeforeFeatureBitMask != NULL) &&
        FindSpecifyFeature(FeatureList, &CpuFeature->Link, FALSE, CpuFeature->CoreBeforeFeatureBitMask)) {
      return CoreDepType;
    }

    if ((CpuFeature->BeforeFeatureBitMask != NULL) &&
        FindSpecifyFeature(FeatureList, &CpuFeature->Link, FALSE, CpuFeature->BeforeFeatureBitMask)) {
      return ThreadDepType;
    }

    return NoneDepType;
  }

  if ((CpuFeature->PackageAfterFeatureBitMask != NULL) &&
      FindSpecifyFeature(FeatureList, &CpuFeature->Link, TRUE, CpuFeature->PackageAfterFeatureBitMask)) {
    return PackageDepType;
  }

  if ((CpuFeature->CoreAfterFeatureBitMask != NULL) &&
      FindSpecifyFeature(FeatureList, &CpuFeature->Link, TRUE, CpuFeature->CoreAfterFeatureBitMask)) {
    return CoreDepType;
  }

  if ((CpuFeature->AfterFeatureBitMask != NULL) &&
      FindSpecifyFeature(FeatureList, &CpuFeature->Link, TRUE, CpuFeature->AfterFeatureBitMask)) {
    return ThreadDepType;
  }

  return NoneDepType;
}

/**
  Base on dependence relationship to asjust feature dependence.

  ONLY when the feature before(or after) the find feature also has
  dependence with the find feature. In this case, driver need to base
  on dependce relationship to decide how to insert current feature and
  adjust the feature dependence.

  @param[in, out]  PreviousFeature    CPU feature current before the find one.
  @param[in, out]  CurrentFeature     Cpu feature need to adjust.
  @param[in]       FindFeature        Cpu feature which current feature depends.
  @param[in]       Before             Before or after dependence relationship.

  @retval   TRUE   means the current feature dependence has been adjusted.

  @retval   FALSE  means the previous feature dependence has been adjusted.
                   or previous feature has no dependence with the find one.

**/
BOOLEAN
AdjustFeaturesDependence (
  IN OUT CPU_FEATURES_ENTRY         *PreviousFeature,
  IN OUT CPU_FEATURES_ENTRY         *CurrentFeature,
  IN     CPU_FEATURES_ENTRY         *FindFeature,
  IN     BOOLEAN                    Before
  )
{
  CPU_FEATURE_DEPENDENCE_TYPE            PreDependType;
  CPU_FEATURE_DEPENDENCE_TYPE            CurrentDependType;

  PreDependType     = DetectFeatureScope(PreviousFeature, Before, FindFeature->FeatureMask);
  CurrentDependType = DetectFeatureScope(CurrentFeature, Before, FindFeature->FeatureMask);

  //
  // If previous feature has no dependence with the find featue.
  // return FALSE.
  //
  if (PreDependType == NoneDepType) {
    return FALSE;
  }

  //
  // If both feature have dependence, keep the one which needs use more
  // processors and clear the dependence for the other one.
  //
  if (PreDependType >= CurrentDependType) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Base on dependence relationship to asjust feature order.

  @param[in]       FeatureList        Pointer to CPU feature list
  @param[in, out]  FindEntry          The entry this feature depend on.
  @param[in, out]  CurrentEntry       The entry for this feature.
  @param[in]       Before             Before or after dependence relationship.

**/
VOID
AdjustEntry (
  IN      LIST_ENTRY                *FeatureList,
  IN OUT  LIST_ENTRY                *FindEntry,
  IN OUT  LIST_ENTRY                *CurrentEntry,
  IN      BOOLEAN                   Before
  )
{
  LIST_ENTRY                *PreviousEntry;
  CPU_FEATURES_ENTRY        *PreviousFeature;
  CPU_FEATURES_ENTRY        *CurrentFeature;
  CPU_FEATURES_ENTRY        *FindFeature;

  //
  // For CPU feature which has core or package type dependence, later code need to insert
  // AcquireSpinLock/ReleaseSpinLock logic to sequency the execute order.
  // So if driver finds both feature A and B need to execute before feature C, driver will
  // base on dependence type of feature A and B to update the logic here.
  // For example, feature A has package type dependence and feature B has core type dependence,
  // because package type dependence need to wait for more processors which has strong dependence
  // than core type dependence. So driver will adjust the feature order to B -> A -> C. and driver
  // will remove the feature dependence in feature B.
  // Driver just needs to make sure before feature C been executed, feature A has finished its task
  // in all all thread. Feature A finished in all threads also means feature B have finshed in all
  // threads.
  //
  if (Before) {
    PreviousEntry = GetPreviousNode (FeatureList, FindEntry);
  } else {

    PreviousEntry = GetNextNode (FeatureList, FindEntry);
  }

  CurrentFeature  = CPU_FEATURE_ENTRY_FROM_LINK (CurrentEntry);
  RemoveEntryList (CurrentEntry);

  if (IsNull (FeatureList, PreviousEntry)) {
    //
    // If not exist the previous or next entry, just insert the current entry.
    //
    if (Before) {
      InsertTailList (FindEntry, CurrentEntry);
    } else {
      InsertHeadList (FindEntry, CurrentEntry);
    }
  } else {
    //
    // If exist the previous or next entry, need to check it before insert curent entry.
    //
    PreviousFeature = CPU_FEATURE_ENTRY_FROM_LINK (PreviousEntry);
    FindFeature     = CPU_FEATURE_ENTRY_FROM_LINK (FindEntry);

    if (AdjustFeaturesDependence (PreviousFeature, CurrentFeature, FindFeature, Before)) {
      //
      // Return TRUE means current feature dependence has been cleared and the previous
      // feature dependence has been kept and used. So insert current feature before (or after)
      // the previous feature.
      //
      if (Before) {
        InsertTailList (PreviousEntry, CurrentEntry);
      } else {
        InsertHeadList (PreviousEntry, CurrentEntry);
      }
    } else {
      if (Before) {
        InsertTailList (FindEntry, CurrentEntry);
      } else {
        InsertHeadList (FindEntry, CurrentEntry);
      }
    }
  }
}


/**
  Checks and adjusts current CPU features per dependency relationship.

  @param[in]  FeatureList        Pointer to CPU feature list
  @param[in]  CurrentEntry       Pointer to current checked CPU feature
  @param[in]  FeatureMask        The feature bit mask.

  @retval     return Swapped info.
**/
BOOLEAN
InsertToBeforeEntry (
  IN LIST_ENTRY              *FeatureList,
  IN LIST_ENTRY              *CurrentEntry,
  IN UINT8                   *FeatureMask
  )
{
  LIST_ENTRY                 *CheckEntry;
  CPU_FEATURES_ENTRY         *CheckFeature;
  BOOLEAN                    Swapped;

  Swapped = FALSE;

  //
  // Check all features dispatched before this entry
  //
  CheckEntry = GetFirstNode (FeatureList);
  while (CheckEntry != CurrentEntry) {
    CheckFeature = CPU_FEATURE_ENTRY_FROM_LINK (CheckEntry);
    if (IsBitMaskMatchCheck (CheckFeature->FeatureMask, FeatureMask)) {
      AdjustEntry (FeatureList, CheckEntry, CurrentEntry, TRUE);
      Swapped = TRUE;
      break;
    }
    CheckEntry = CheckEntry->ForwardLink;
  }

  return Swapped;
}

/**
  Checks and adjusts current CPU features per dependency relationship.

  @param[in]  FeatureList        Pointer to CPU feature list
  @param[in]  CurrentEntry       Pointer to current checked CPU feature
  @param[in]  FeatureMask        The feature bit mask.

  @retval     return Swapped info.
**/
BOOLEAN
InsertToAfterEntry (
  IN LIST_ENTRY              *FeatureList,
  IN LIST_ENTRY              *CurrentEntry,
  IN UINT8                   *FeatureMask
  )
{
  LIST_ENTRY                 *CheckEntry;
  CPU_FEATURES_ENTRY         *CheckFeature;
  BOOLEAN                    Swapped;

  Swapped = FALSE;

  //
  // Check all features dispatched after this entry
  //
  CheckEntry = GetNextNode (FeatureList, CurrentEntry);
  while (!IsNull (FeatureList, CheckEntry)) {
    CheckFeature = CPU_FEATURE_ENTRY_FROM_LINK (CheckEntry);
    if (IsBitMaskMatchCheck (CheckFeature->FeatureMask, FeatureMask)) {
      AdjustEntry (FeatureList, CheckEntry, CurrentEntry, FALSE);
      Swapped = TRUE;
      break;
    }
    CheckEntry = CheckEntry->ForwardLink;
  }

  return Swapped;
}

/**
  Checks and adjusts CPU features order per dependency relationship.

  @param[in]  FeatureList        Pointer to CPU feature list
**/
VOID
CheckCpuFeaturesDependency (
  IN LIST_ENTRY              *FeatureList
  )
{
  LIST_ENTRY                 *CurrentEntry;
  CPU_FEATURES_ENTRY         *CpuFeature;
  LIST_ENTRY                 *CheckEntry;
  CPU_FEATURES_ENTRY         *CheckFeature;
  BOOLEAN                    Swapped;
  LIST_ENTRY                 *TempEntry;
  LIST_ENTRY                 *NextEntry;

  CurrentEntry = GetFirstNode (FeatureList);
  while (!IsNull (FeatureList, CurrentEntry)) {
    Swapped = FALSE;
    CpuFeature = CPU_FEATURE_ENTRY_FROM_LINK (CurrentEntry);
    NextEntry = CurrentEntry->ForwardLink;
    if (CpuFeature->BeforeAll) {
      //
      // Check all features dispatched before this entry
      //
      CheckEntry = GetFirstNode (FeatureList);
      while (CheckEntry != CurrentEntry) {
        CheckFeature = CPU_FEATURE_ENTRY_FROM_LINK (CheckEntry);
        if (!CheckFeature->BeforeAll) {
          //
          // If this feature has no BeforeAll flag and is dispatched before CpuFeature,
          // insert currentEntry before Checked feature
          //
          RemoveEntryList (CurrentEntry);
          InsertTailList (CheckEntry, CurrentEntry);
          Swapped = TRUE;
          break;
        }
        CheckEntry = CheckEntry->ForwardLink;
      }
      if (Swapped) {
        CurrentEntry = NextEntry;
        continue;
      }
    }

    if (CpuFeature->AfterAll) {
      //
      // Check all features dispatched after this entry
      //
      CheckEntry = GetNextNode (FeatureList, CurrentEntry);
      while (!IsNull (FeatureList, CheckEntry)) {
        CheckFeature = CPU_FEATURE_ENTRY_FROM_LINK (CheckEntry);
        if (!CheckFeature->AfterAll) {
          //
          // If this feature has no AfterAll flag and is dispatched after CpuFeature,
          // insert currentEntry after Checked feature
          //
          TempEntry = GetNextNode (FeatureList, CurrentEntry);
          RemoveEntryList (CurrentEntry);
          InsertHeadList (CheckEntry, CurrentEntry);
          CurrentEntry = TempEntry;
          Swapped = TRUE;
          break;
        }
        CheckEntry = CheckEntry->ForwardLink;
      }
      if (Swapped) {
        CurrentEntry = NextEntry;
        continue;
      }
    }

    if (CpuFeature->BeforeFeatureBitMask != NULL) {
      Swapped = InsertToBeforeEntry (FeatureList, CurrentEntry, CpuFeature->BeforeFeatureBitMask);
      if (Swapped) {
        continue;
      }
    }

    if (CpuFeature->AfterFeatureBitMask != NULL) {
      Swapped = InsertToAfterEntry (FeatureList, CurrentEntry, CpuFeature->AfterFeatureBitMask);
      if (Swapped) {
        continue;
      }
    }

    if (CpuFeature->CoreBeforeFeatureBitMask != NULL) {
      Swapped = InsertToBeforeEntry (FeatureList, CurrentEntry, CpuFeature->CoreBeforeFeatureBitMask);
      if (Swapped) {
        continue;
      }
    }

    if (CpuFeature->CoreAfterFeatureBitMask != NULL) {
      Swapped = InsertToAfterEntry (FeatureList, CurrentEntry, CpuFeature->CoreAfterFeatureBitMask);
      if (Swapped) {
        continue;
      }
    }

    if (CpuFeature->PackageBeforeFeatureBitMask != NULL) {
      Swapped = InsertToBeforeEntry (FeatureList, CurrentEntry, CpuFeature->PackageBeforeFeatureBitMask);
      if (Swapped) {
        continue;
      }
    }

    if (CpuFeature->PackageAfterFeatureBitMask != NULL) {
      Swapped = InsertToAfterEntry (FeatureList, CurrentEntry, CpuFeature->PackageAfterFeatureBitMask);
      if (Swapped) {
        continue;
      }
    }

    CurrentEntry = CurrentEntry->ForwardLink;
  }
}

/**
  Worker function to register CPU Feature.

  @param[in]  CpuFeature            Pointer to CPU feature entry

  @retval  RETURN_SUCCESS           The CPU feature was successfully registered.
  @retval  RETURN_OUT_OF_RESOURCES  There are not enough resources to register
                                    the CPU feature.
  @retval  RETURN_UNSUPPORTED       Registration of the CPU feature is not
                                    supported due to a circular dependency between
                                    BEFORE and AFTER features.
**/
RETURN_STATUS
RegisterCpuFeatureWorker (
  IN CPU_FEATURES_ENTRY      *CpuFeature
  )
{
  EFI_STATUS                 Status;
  CPU_FEATURES_DATA          *CpuFeaturesData;
  CPU_FEATURES_ENTRY         *CpuFeatureEntry;
  LIST_ENTRY                 *Entry;
  UINTN                      BitMaskSize;
  BOOLEAN                    FeatureExist;

  BitMaskSize     = PcdGetSize (PcdCpuFeaturesSetting);
  CpuFeaturesData = GetCpuFeaturesData ();
  if (CpuFeaturesData->FeaturesCount == 0) {
    InitializeListHead (&CpuFeaturesData->FeatureList);
    InitializeSpinLock (&CpuFeaturesData->CpuFlags.MemoryMappedLock);
    InitializeSpinLock (&CpuFeaturesData->CpuFlags.ConsoleLogLock);
    CpuFeaturesData->BitMaskSize = (UINT32) BitMaskSize;
  }
  ASSERT (CpuFeaturesData->BitMaskSize == BitMaskSize);

  FeatureExist = FALSE;
  CpuFeatureEntry = NULL;
  Entry = GetFirstNode (&CpuFeaturesData->FeatureList);
  while (!IsNull (&CpuFeaturesData->FeatureList, Entry)) {
    CpuFeatureEntry = CPU_FEATURE_ENTRY_FROM_LINK (Entry);
    if (IsCpuFeatureMatch (CpuFeature->FeatureMask, CpuFeatureEntry->FeatureMask)) {
      //
      // If this feature already registered
      //
      FeatureExist = TRUE;
      break;
    }
    Entry = Entry->ForwardLink;
  }

  if (!FeatureExist) {
    DEBUG ((DEBUG_INFO, "[NEW] "));
    DumpCpuFeature (CpuFeature);
    InsertTailList (&CpuFeaturesData->FeatureList, &CpuFeature->Link);
    CpuFeaturesData->FeaturesCount++;
  } else {
    DEBUG ((DEBUG_INFO, "[OVERRIDE] "));
    DumpCpuFeature (CpuFeature);
    ASSERT (CpuFeatureEntry != NULL);
    //
    // Overwrite original parameters of CPU feature
    //
    if (CpuFeature->GetConfigDataFunc != NULL) {
      CpuFeatureEntry->GetConfigDataFunc = CpuFeature->GetConfigDataFunc;
    }
    if (CpuFeature->SupportFunc != NULL) {
      CpuFeatureEntry->SupportFunc = CpuFeature->SupportFunc;
    }
    if (CpuFeature->InitializeFunc != NULL) {
      CpuFeatureEntry->InitializeFunc = CpuFeature->InitializeFunc;
    }
    if (CpuFeature->FeatureName != NULL) {
      if (CpuFeatureEntry->FeatureName == NULL) {
        CpuFeatureEntry->FeatureName = AllocatePool (CPU_FEATURE_NAME_SIZE);
        ASSERT (CpuFeatureEntry->FeatureName != NULL);
      }
      Status = AsciiStrCpyS (CpuFeatureEntry->FeatureName, CPU_FEATURE_NAME_SIZE, CpuFeature->FeatureName);
      ASSERT_EFI_ERROR (Status);
      FreePool (CpuFeature->FeatureName);
    }
    if (CpuFeature->BeforeFeatureBitMask != NULL) {
      if (CpuFeatureEntry->BeforeFeatureBitMask != NULL) {
        FreePool (CpuFeatureEntry->BeforeFeatureBitMask);
      }
      CpuFeatureEntry->BeforeFeatureBitMask = CpuFeature->BeforeFeatureBitMask;
    }
    if (CpuFeature->AfterFeatureBitMask != NULL) {
      if (CpuFeatureEntry->AfterFeatureBitMask != NULL) {
        FreePool (CpuFeatureEntry->AfterFeatureBitMask);
      }
      CpuFeatureEntry->AfterFeatureBitMask = CpuFeature->AfterFeatureBitMask;
    }
    if (CpuFeature->CoreBeforeFeatureBitMask != NULL) {
      if (CpuFeatureEntry->CoreBeforeFeatureBitMask != NULL) {
        FreePool (CpuFeatureEntry->CoreBeforeFeatureBitMask);
      }
      CpuFeatureEntry->CoreBeforeFeatureBitMask = CpuFeature->CoreBeforeFeatureBitMask;
    }
    if (CpuFeature->CoreAfterFeatureBitMask != NULL) {
      if (CpuFeatureEntry->CoreAfterFeatureBitMask != NULL) {
        FreePool (CpuFeatureEntry->CoreAfterFeatureBitMask);
      }
      CpuFeatureEntry->CoreAfterFeatureBitMask = CpuFeature->CoreAfterFeatureBitMask;
    }
    if (CpuFeature->PackageBeforeFeatureBitMask != NULL) {
      if (CpuFeatureEntry->PackageBeforeFeatureBitMask != NULL) {
        FreePool (CpuFeatureEntry->PackageBeforeFeatureBitMask);
      }
      CpuFeatureEntry->PackageBeforeFeatureBitMask = CpuFeature->PackageBeforeFeatureBitMask;
    }
    if (CpuFeature->PackageAfterFeatureBitMask != NULL) {
      if (CpuFeatureEntry->PackageAfterFeatureBitMask != NULL) {
        FreePool (CpuFeatureEntry->PackageAfterFeatureBitMask);
      }
      CpuFeatureEntry->PackageAfterFeatureBitMask = CpuFeature->PackageAfterFeatureBitMask;
    }

    CpuFeatureEntry->BeforeAll = CpuFeature->BeforeAll;
    CpuFeatureEntry->AfterAll  = CpuFeature->AfterAll;

    FreePool (CpuFeature->FeatureMask);
    FreePool (CpuFeature);
  }
  //
  // Verify CPU features dependency can change CPU feature order
  //
  CheckCpuFeaturesDependency (&CpuFeaturesData->FeatureList);
  return RETURN_SUCCESS;
}

/**
  Sets CPU feature bit mask in CPU feature bit mask buffer.

  @param[in]  FeaturesBitMask       Pointer to CPU feature bit mask buffer
  @param[in]  Feature               The bit number of the CPU feature
  @param[in]  BitMaskSize           CPU feature bit mask buffer size
**/
VOID
SetCpuFeaturesBitMask (
  IN UINT8               **FeaturesBitMask,
  IN UINT32              Feature,
  IN UINTN               BitMaskSize
  )
{
  UINT8                  *CpuFeaturesBitMask;

  ASSERT (FeaturesBitMask != NULL);
  CpuFeaturesBitMask = *FeaturesBitMask;
  if (CpuFeaturesBitMask == NULL) {
    CpuFeaturesBitMask = AllocateZeroPool (BitMaskSize);
    ASSERT (CpuFeaturesBitMask != NULL);
    *FeaturesBitMask = CpuFeaturesBitMask;
  }

  CpuFeaturesBitMask  += (Feature / 8);
  *CpuFeaturesBitMask |= (UINT8) (1 << (Feature % 8));
}

/**
  Registers a CPU Feature.

  @param[in]  FeatureName        A Null-terminated Ascii string indicates CPU feature
                                 name.
  @param[in]  GetConfigDataFunc  CPU feature get configuration data function.  This
                                 is an optional parameter that may be NULL.  If NULL,
                                 then the most recently registered function for the
                                 CPU feature is used.  If no functions are registered
                                 for a CPU feature, then the CPU configuration data
                                 for the registered feature is NULL.
  @param[in]  SupportFunc        CPU feature support function.  This is an optional
                                 parameter that may be NULL.  If NULL, then the most
                                 recently registered function for the CPU feature is
                                 used. If no functions are registered for a CPU
                                 feature, then the CPU feature is assumed to be
                                 supported by all CPUs.
  @param[in]  InitializeFunc     CPU feature initialize function.  This is an optional
                                 parameter that may be NULL.  If NULL, then the most
                                 recently registered function for the CPU feature is
                                 used. If no functions are registered for a CPU
                                 feature, then the CPU feature initialization is
                                 skipped.
  @param[in]  ...                Variable argument list of UINT32 CPU feature value.
                                 Values with no modifiers are the features provided
                                 by the registered functions.
                                 Values with CPU_FEATURE_BEFORE modifier are features
                                 that must be initialized after the features provided
                                 by the registered functions are used.
                                 Values with CPU_FEATURE_AFTER modifier are features
                                 that must be initialized before the features provided
                                 by the registered functions are used.
                                 The last argument in this variable argument list must
                                 always be CPU_FEATURE_END.

  @retval  RETURN_SUCCESS           The CPU feature was successfully registered.
  @retval  RETURN_OUT_OF_RESOURCES  There are not enough resources to register
                                    the CPU feature.
  @retval  RETURN_UNSUPPORTED       Registration of the CPU feature is not
                                    supported due to a circular dependency between
                                    BEFORE and AFTER features.
  @retval  RETURN_NOT_READY         CPU feature PCD PcdCpuFeaturesUserConfiguration
                                    not updated by Platform driver yet.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
RegisterCpuFeature (
  IN CHAR8                             *FeatureName,       OPTIONAL
  IN CPU_FEATURE_GET_CONFIG_DATA       GetConfigDataFunc,  OPTIONAL
  IN CPU_FEATURE_SUPPORT               SupportFunc,        OPTIONAL
  IN CPU_FEATURE_INITIALIZE            InitializeFunc,     OPTIONAL
  ...
  )
{
  EFI_STATUS                 Status;
  VA_LIST                    Marker;
  UINT32                     Feature;
  UINTN                      BitMaskSize;
  CPU_FEATURES_ENTRY         *CpuFeature;
  UINT8                      *FeatureMask;
  UINT8                      *BeforeFeatureBitMask;
  UINT8                      *AfterFeatureBitMask;
  UINT8                      *CoreBeforeFeatureBitMask;
  UINT8                      *CoreAfterFeatureBitMask;
  UINT8                      *PackageBeforeFeatureBitMask;
  UINT8                      *PackageAfterFeatureBitMask;
  BOOLEAN                    BeforeAll;
  BOOLEAN                    AfterAll;

  FeatureMask                 = NULL;
  BeforeFeatureBitMask        = NULL;
  AfterFeatureBitMask         = NULL;
  CoreBeforeFeatureBitMask    = NULL;
  CoreAfterFeatureBitMask     = NULL;
  PackageBeforeFeatureBitMask = NULL;
  PackageAfterFeatureBitMask  = NULL;
  BeforeAll            = FALSE;
  AfterAll             = FALSE;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSetting);

  VA_START (Marker, InitializeFunc);
  Feature = VA_ARG (Marker, UINT32);
  while (Feature != CPU_FEATURE_END) {
    ASSERT ((Feature & (CPU_FEATURE_BEFORE | CPU_FEATURE_AFTER))
                    != (CPU_FEATURE_BEFORE | CPU_FEATURE_AFTER));
    ASSERT ((Feature & (CPU_FEATURE_BEFORE_ALL | CPU_FEATURE_AFTER_ALL))
                    != (CPU_FEATURE_BEFORE_ALL | CPU_FEATURE_AFTER_ALL));
    ASSERT ((Feature & (CPU_FEATURE_CORE_BEFORE | CPU_FEATURE_CORE_AFTER))
                    != (CPU_FEATURE_CORE_BEFORE | CPU_FEATURE_CORE_AFTER));
    ASSERT ((Feature & (CPU_FEATURE_PACKAGE_BEFORE | CPU_FEATURE_PACKAGE_AFTER))
                    != (CPU_FEATURE_PACKAGE_BEFORE | CPU_FEATURE_PACKAGE_AFTER));
    if (Feature < CPU_FEATURE_BEFORE) {
      BeforeAll = ((Feature & CPU_FEATURE_BEFORE_ALL) != 0) ? TRUE : FALSE;
      AfterAll  = ((Feature & CPU_FEATURE_AFTER_ALL) != 0) ? TRUE : FALSE;
      Feature  &= ~(CPU_FEATURE_BEFORE_ALL | CPU_FEATURE_AFTER_ALL);
      ASSERT (FeatureMask == NULL);
      SetCpuFeaturesBitMask (&FeatureMask, Feature, BitMaskSize);
    } else if ((Feature & CPU_FEATURE_BEFORE) != 0) {
      SetCpuFeaturesBitMask (&BeforeFeatureBitMask, Feature & ~CPU_FEATURE_BEFORE, BitMaskSize);
    } else if ((Feature & CPU_FEATURE_AFTER) != 0) {
      SetCpuFeaturesBitMask (&AfterFeatureBitMask, Feature & ~CPU_FEATURE_AFTER, BitMaskSize);
    } else if ((Feature & CPU_FEATURE_CORE_BEFORE) != 0) {
      SetCpuFeaturesBitMask (&CoreBeforeFeatureBitMask, Feature & ~CPU_FEATURE_CORE_BEFORE, BitMaskSize);
    } else if ((Feature & CPU_FEATURE_CORE_AFTER) != 0) {
      SetCpuFeaturesBitMask (&CoreAfterFeatureBitMask, Feature & ~CPU_FEATURE_CORE_AFTER, BitMaskSize);
    } else if ((Feature & CPU_FEATURE_PACKAGE_BEFORE) != 0) {
      SetCpuFeaturesBitMask (&PackageBeforeFeatureBitMask, Feature & ~CPU_FEATURE_PACKAGE_BEFORE, BitMaskSize);
    } else if ((Feature & CPU_FEATURE_PACKAGE_AFTER) != 0) {
      SetCpuFeaturesBitMask (&PackageAfterFeatureBitMask, Feature & ~CPU_FEATURE_PACKAGE_AFTER, BitMaskSize);
    }
    Feature = VA_ARG (Marker, UINT32);
  }
  VA_END (Marker);

  CpuFeature = AllocateZeroPool (sizeof (CPU_FEATURES_ENTRY));
  ASSERT (CpuFeature != NULL);
  CpuFeature->Signature                   = CPU_FEATURE_ENTRY_SIGNATURE;
  CpuFeature->FeatureMask                 = FeatureMask;
  CpuFeature->BeforeFeatureBitMask        = BeforeFeatureBitMask;
  CpuFeature->AfterFeatureBitMask         = AfterFeatureBitMask;
  CpuFeature->CoreBeforeFeatureBitMask    = CoreBeforeFeatureBitMask;
  CpuFeature->CoreAfterFeatureBitMask     = CoreAfterFeatureBitMask;
  CpuFeature->PackageBeforeFeatureBitMask = PackageBeforeFeatureBitMask;
  CpuFeature->PackageAfterFeatureBitMask  = PackageAfterFeatureBitMask;
  CpuFeature->BeforeAll                   = BeforeAll;
  CpuFeature->AfterAll                    = AfterAll;
  CpuFeature->GetConfigDataFunc           = GetConfigDataFunc;
  CpuFeature->SupportFunc                 = SupportFunc;
  CpuFeature->InitializeFunc              = InitializeFunc;
  if (FeatureName != NULL) {
    CpuFeature->FeatureName          = AllocatePool (CPU_FEATURE_NAME_SIZE);
    ASSERT (CpuFeature->FeatureName != NULL);
    Status = AsciiStrCpyS (CpuFeature->FeatureName, CPU_FEATURE_NAME_SIZE, FeatureName);
    ASSERT_EFI_ERROR (Status);
  }

  Status = RegisterCpuFeatureWorker (CpuFeature);
  ASSERT_EFI_ERROR (Status);

  return RETURN_SUCCESS;
}

/**
  Return ACPI_CPU_DATA data.

  @return  Pointer to ACPI_CPU_DATA data.
**/
ACPI_CPU_DATA *
GetAcpiCpuData (
  VOID
  )
{
  EFI_STATUS                           Status;
  UINTN                                NumberOfCpus;
  UINTN                                NumberOfEnabledProcessors;
  ACPI_CPU_DATA                        *AcpiCpuData;
  UINTN                                TableSize;
  CPU_REGISTER_TABLE                   *RegisterTable;
  UINTN                                Index;
  EFI_PROCESSOR_INFORMATION            ProcessorInfoBuffer;

  AcpiCpuData = (ACPI_CPU_DATA *) (UINTN) PcdGet64 (PcdCpuS3DataAddress);
  if (AcpiCpuData != NULL) {
    return AcpiCpuData;
  }

  AcpiCpuData  = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (ACPI_CPU_DATA)));
  ASSERT (AcpiCpuData != NULL);

  //
  // Set PcdCpuS3DataAddress to the base address of the ACPI_CPU_DATA structure
  //
  Status = PcdSet64S (PcdCpuS3DataAddress, (UINT64)(UINTN)AcpiCpuData);
  ASSERT_EFI_ERROR (Status);

  GetNumberOfProcessor (&NumberOfCpus, &NumberOfEnabledProcessors);
  AcpiCpuData->NumberOfCpus = (UINT32)NumberOfCpus;

  //
  // Allocate buffer for empty RegisterTable and PreSmmInitRegisterTable for all CPUs
  //
  TableSize = 2 * NumberOfCpus * sizeof (CPU_REGISTER_TABLE);
  RegisterTable  = AllocatePages (EFI_SIZE_TO_PAGES (TableSize));
  ASSERT (RegisterTable != NULL);

  for (Index = 0; Index < NumberOfCpus; Index++) {
    Status = GetProcessorInformation (Index, &ProcessorInfoBuffer);
    ASSERT_EFI_ERROR (Status);

    RegisterTable[Index].InitialApicId      = (UINT32)ProcessorInfoBuffer.ProcessorId;
    RegisterTable[Index].TableLength        = 0;
    RegisterTable[Index].AllocatedSize      = 0;
    RegisterTable[Index].RegisterTableEntry = 0;

    RegisterTable[NumberOfCpus + Index].InitialApicId      = (UINT32)ProcessorInfoBuffer.ProcessorId;
    RegisterTable[NumberOfCpus + Index].TableLength        = 0;
    RegisterTable[NumberOfCpus + Index].AllocatedSize      = 0;
    RegisterTable[NumberOfCpus + Index].RegisterTableEntry = 0;
  }
  AcpiCpuData->RegisterTable           = (EFI_PHYSICAL_ADDRESS)(UINTN)RegisterTable;
  AcpiCpuData->PreSmmInitRegisterTable = (EFI_PHYSICAL_ADDRESS)(UINTN)(RegisterTable + NumberOfCpus);

  return AcpiCpuData;
}

/**
  Enlarges CPU register table for each processor.

  @param[in, out]  RegisterTable   Pointer processor's CPU register table
**/
STATIC
VOID
EnlargeRegisterTable (
  IN OUT CPU_REGISTER_TABLE            *RegisterTable
  )
{
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 UsedPages;

  UsedPages = RegisterTable->AllocatedSize / EFI_PAGE_SIZE;
  Address  = (UINTN)AllocatePages (UsedPages + 1);
  ASSERT (Address != 0);

  //
  // If there are records existing in the register table, then copy its contents
  // to new region and free the old one.
  //
  if (RegisterTable->AllocatedSize > 0) {
    CopyMem (
      (VOID *) (UINTN) Address,
      (VOID *) (UINTN) RegisterTable->RegisterTableEntry,
      RegisterTable->AllocatedSize
      );

    FreePages ((VOID *)(UINTN)RegisterTable->RegisterTableEntry, UsedPages);
  }

  //
  // Adjust the allocated size and register table base address.
  //
  RegisterTable->AllocatedSize     += EFI_PAGE_SIZE;
  RegisterTable->RegisterTableEntry = Address;
}

/**
  Add an entry in specified register table.

  This function adds an entry in specified register table, with given register type,
  register index, bit section and value.

  @param[in]  PreSmmFlag       If TRUE, entry will be added into PreSmm register table
                               If FALSE, entry will be added into register table
  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry
  @param[in]  RegisterType     Type of the register to program
  @param[in]  Index            Index of the register to program
  @param[in]  ValidBitStart    Start of the bit section
  @param[in]  ValidBitLength   Length of the bit section
  @param[in]  Value            Value to write
**/
VOID
CpuRegisterTableWriteWorker (
  IN BOOLEAN                 PreSmmFlag,
  IN UINTN                   ProcessorNumber,
  IN REGISTER_TYPE           RegisterType,
  IN UINT64                  Index,
  IN UINT8                   ValidBitStart,
  IN UINT8                   ValidBitLength,
  IN UINT64                  Value
  )
{
  CPU_FEATURES_DATA        *CpuFeaturesData;
  ACPI_CPU_DATA            *AcpiCpuData;
  CPU_REGISTER_TABLE       *RegisterTable;
  CPU_REGISTER_TABLE_ENTRY *RegisterTableEntry;

  CpuFeaturesData = GetCpuFeaturesData ();
  if (CpuFeaturesData->RegisterTable == NULL) {
    AcpiCpuData = GetAcpiCpuData ();
    ASSERT ((AcpiCpuData != NULL) && (AcpiCpuData->RegisterTable != 0));
    CpuFeaturesData->RegisterTable = (CPU_REGISTER_TABLE *) (UINTN) AcpiCpuData->RegisterTable;
    CpuFeaturesData->PreSmmRegisterTable = (CPU_REGISTER_TABLE *) (UINTN) AcpiCpuData->PreSmmInitRegisterTable;
  }

  if (PreSmmFlag) {
    RegisterTable = &CpuFeaturesData->PreSmmRegisterTable[ProcessorNumber];
  } else {
    RegisterTable = &CpuFeaturesData->RegisterTable[ProcessorNumber];
  }

  if (RegisterTable->TableLength == RegisterTable->AllocatedSize / sizeof (CPU_REGISTER_TABLE_ENTRY)) {
    EnlargeRegisterTable (RegisterTable);
  }

  //
  // Append entry in the register table.
  //
  RegisterTableEntry = (CPU_REGISTER_TABLE_ENTRY *) (UINTN) RegisterTable->RegisterTableEntry;
  RegisterTableEntry[RegisterTable->TableLength].RegisterType   = RegisterType;
  RegisterTableEntry[RegisterTable->TableLength].Index          = (UINT32) Index;
  RegisterTableEntry[RegisterTable->TableLength].HighIndex      = (UINT32) RShiftU64 (Index, 32);
  RegisterTableEntry[RegisterTable->TableLength].ValidBitStart  = ValidBitStart;
  RegisterTableEntry[RegisterTable->TableLength].ValidBitLength = ValidBitLength;
  RegisterTableEntry[RegisterTable->TableLength].Value          = Value;

  RegisterTable->TableLength++;
}

/**
  Adds an entry in specified register table.

  This function adds an entry in specified register table, with given register type,
  register index, bit section and value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry
  @param[in]  RegisterType     Type of the register to program
  @param[in]  Index            Index of the register to program
  @param[in]  ValueMask        Mask of bits in register to write
  @param[in]  Value            Value to write

  @note This service could be called by BSP only.
**/
VOID
EFIAPI
CpuRegisterTableWrite (
  IN UINTN               ProcessorNumber,
  IN REGISTER_TYPE       RegisterType,
  IN UINT64              Index,
  IN UINT64              ValueMask,
  IN UINT64              Value
  )
{
  UINT8                   Start;
  UINT8                   End;
  UINT8                   Length;

  Start  = (UINT8)LowBitSet64  (ValueMask);
  End    = (UINT8)HighBitSet64 (ValueMask);
  Length = End - Start + 1;
  CpuRegisterTableWriteWorker (FALSE, ProcessorNumber, RegisterType, Index, Start, Length, Value);
}

/**
  Adds an entry in specified Pre-SMM register table.

  This function adds an entry in specified register table, with given register type,
  register index, bit section and value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry.
  @param[in]  RegisterType     Type of the register to program
  @param[in]  Index            Index of the register to program
  @param[in]  ValueMask        Mask of bits in register to write
  @param[in]  Value            Value to write

  @note This service could be called by BSP only.
**/
VOID
EFIAPI
PreSmmCpuRegisterTableWrite (
  IN UINTN               ProcessorNumber,
  IN REGISTER_TYPE       RegisterType,
  IN UINT64              Index,
  IN UINT64              ValueMask,
  IN UINT64              Value
  )
{
  UINT8  Start;
  UINT8  End;
  UINT8  Length;

  Start  = (UINT8)LowBitSet64  (ValueMask);
  End    = (UINT8)HighBitSet64 (ValueMask);
  Length = End - Start + 1;
  CpuRegisterTableWriteWorker (TRUE, ProcessorNumber, RegisterType, Index, Start, Length, Value);
}

/**
  Worker function to determine if a CPU feature is set in input CPU feature bit mask buffer.

  @param[in]  CpuBitMask      CPU feature bit mask buffer
  @param[in]  CpuBitMaskSize  The size of CPU feature bit mask buffer
  @param[in]  Feature         The bit number of the CPU feature

  @retval  TRUE   The CPU feature is set in CpuBitMask.
  @retval  FALSE  The CPU feature is not set in CpuBitMask.

**/
BOOLEAN
IsCpuFeatureSetInCpuPcd (
  IN UINT8               *CpuBitMask,
  IN UINTN               CpuBitMaskSize,
  IN UINT32              Feature
  )
{
  if ((Feature >> 3) >= CpuBitMaskSize) {
    return FALSE;
  }
  return ((*(CpuBitMask + (Feature >> 3)) & (1 << (Feature & 0x07))) != 0);
}

/**
  Determines if a CPU feature is enabled in PcdCpuFeaturesSupport bit mask.
  If a CPU feature is disabled in PcdCpuFeaturesSupport then all the code/data
  associated with that feature should be optimized away if compiler
  optimizations are enabled.

  @param[in]  Feature  The bit number of the CPU feature to check in the PCD
                       PcdCpuFeaturesSupport

  @retval  TRUE   The CPU feature is set in PcdCpuFeaturesSupport.
  @retval  FALSE  The CPU feature is not set in PcdCpuFeaturesSupport.

  @note This service could be called by BSP only.
**/
BOOLEAN
EFIAPI
IsCpuFeatureSupported (
  IN UINT32              Feature
  )
{
  return IsCpuFeatureSetInCpuPcd (
           (UINT8 *)PcdGetPtr (PcdCpuFeaturesSupport),
           PcdGetSize (PcdCpuFeaturesSupport),
           Feature
           );
}

/**
  Determines if a CPU feature is set in PcdCpuFeaturesSetting bit mask.

  @param[in]  Feature  The bit number of the CPU feature to check in the PCD
                       PcdCpuFeaturesSetting

  @retval  TRUE   The CPU feature is set in PcdCpuFeaturesSetting.
  @retval  FALSE  The CPU feature is not set in PcdCpuFeaturesSetting.

  @note This service could be called by BSP only.
**/
BOOLEAN
EFIAPI
IsCpuFeatureInSetting (
  IN UINT32              Feature
  )
{
  return IsCpuFeatureSetInCpuPcd (
           (UINT8 *)PcdGetPtr (PcdCpuFeaturesSetting),
           PcdGetSize (PcdCpuFeaturesSetting),
           Feature
           );
}

/**
  Switches to assigned BSP after CPU features initialization.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.

  @note This service could be called by BSP only.
**/
VOID
EFIAPI
SwitchBspAfterFeaturesInitialize (
  IN UINTN               ProcessorNumber
  )
{
  CPU_FEATURES_DATA      *CpuFeaturesData;

  CpuFeaturesData = GetCpuFeaturesData ();
  CpuFeaturesData->BspNumber = ProcessorNumber;
}

