/** @file
  CPU Register Table Library functions.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  UINT32                 BitMaskSize;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSupport);
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
  UINT32                 BitMaskSize;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSupport);
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

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSupport);

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

  CurrentEntry = GetFirstNode (FeatureList);
  while (!IsNull (FeatureList, CurrentEntry)) {
    Swapped = FALSE;
    CpuFeature = CPU_FEATURE_ENTRY_FROM_LINK (CurrentEntry);
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
        continue;
      }
    }

    if (CpuFeature->BeforeFeatureBitMask != NULL) {
      //
      // Check all features dispatched before this entry
      //
      CheckEntry = GetFirstNode (FeatureList);
      while (CheckEntry != CurrentEntry) {
        CheckFeature = CPU_FEATURE_ENTRY_FROM_LINK (CheckEntry);
        if (IsBitMaskMatchCheck (CheckFeature->FeatureMask, CpuFeature->BeforeFeatureBitMask)) {
          //
          // If there is dependency, swap them
          //
          RemoveEntryList (CurrentEntry);
          InsertTailList (CheckEntry, CurrentEntry);
          Swapped = TRUE;
          break;
        }
        CheckEntry = CheckEntry->ForwardLink;
      }
      if (Swapped) {
        continue;
      }
    }

    if (CpuFeature->AfterFeatureBitMask != NULL) {
      //
      // Check all features dispatched after this entry
      //
      CheckEntry = GetNextNode (FeatureList, CurrentEntry);
      while (!IsNull (FeatureList, CheckEntry)) {
        CheckFeature = CPU_FEATURE_ENTRY_FROM_LINK (CheckEntry);
        if (IsBitMaskMatchCheck (CheckFeature->FeatureMask, CpuFeature->AfterFeatureBitMask)) {
          //
          // If there is dependency, swap them
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
        continue;
      }
    }
    //
    // No swap happened, check the next feature
    //
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
  UINT32                     BitMaskSize;
  BOOLEAN                    FeatureExist;

  BitMaskSize     = PcdGetSize (PcdCpuFeaturesSupport);
  CpuFeaturesData = GetCpuFeaturesData ();
  if (CpuFeaturesData->FeaturesCount == 0) {
    InitializeListHead (&CpuFeaturesData->FeatureList);
    InitializeSpinLock (&CpuFeaturesData->MsrLock);
    InitializeSpinLock (&CpuFeaturesData->MemoryMappedLock);
    CpuFeaturesData->BitMaskSize = BitMaskSize;
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
  BOOLEAN                    BeforeAll;
  BOOLEAN                    AfterAll;

  FeatureMask          = NULL;
  BeforeFeatureBitMask = NULL;
  AfterFeatureBitMask  = NULL;
  BeforeAll            = FALSE;
  AfterAll             = FALSE;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSupport);

  VA_START (Marker, InitializeFunc);
  Feature = VA_ARG (Marker, UINT32);
  while (Feature != CPU_FEATURE_END) {
    ASSERT ((Feature & (CPU_FEATURE_BEFORE | CPU_FEATURE_AFTER))
                    != (CPU_FEATURE_BEFORE | CPU_FEATURE_AFTER));
    ASSERT ((Feature & (CPU_FEATURE_BEFORE_ALL | CPU_FEATURE_AFTER_ALL))
                    != (CPU_FEATURE_BEFORE_ALL | CPU_FEATURE_AFTER_ALL));
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
    }
    Feature = VA_ARG (Marker, UINT32);
  }
  VA_END (Marker);

  CpuFeature = AllocateZeroPool (sizeof (CPU_FEATURES_ENTRY));
  ASSERT (CpuFeature != NULL);
  CpuFeature->Signature            = CPU_FEATURE_ENTRY_SIGNATURE;
  CpuFeature->FeatureMask          = FeatureMask;
  CpuFeature->BeforeFeatureBitMask = BeforeFeatureBitMask;
  CpuFeature->AfterFeatureBitMask  = AfterFeatureBitMask;
  CpuFeature->BeforeAll            = BeforeAll;
  CpuFeature->AfterAll             = AfterAll;
  CpuFeature->GetConfigDataFunc    = GetConfigDataFunc;
  CpuFeature->SupportFunc          = SupportFunc;
  CpuFeature->InitializeFunc       = InitializeFunc;
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
  EFI_STATUS               Status;
  CPU_FEATURES_DATA        *CpuFeaturesData;
  ACPI_CPU_DATA            *AcpiCpuData;
  CPU_REGISTER_TABLE       *RegisterTable;
  CPU_REGISTER_TABLE_ENTRY *RegisterTableEntry;

  CpuFeaturesData = GetCpuFeaturesData ();
  if (CpuFeaturesData->RegisterTable == NULL) {
    AcpiCpuData = (ACPI_CPU_DATA *) (UINTN) PcdGet64 (PcdCpuS3DataAddress);
    if (AcpiCpuData == NULL) {
      AcpiCpuData = AllocateAcpiCpuData ();
      ASSERT (AcpiCpuData != NULL);
      //
      // Set PcdCpuS3DataAddress to the base address of the ACPI_CPU_DATA structure
      //
      Status = PcdSet64S (PcdCpuS3DataAddress, (UINT64)(UINTN)AcpiCpuData);
      ASSERT_EFI_ERROR (Status);
    }
    ASSERT (AcpiCpuData->RegisterTable != 0);
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

  @retval  TRUE   The CPU feature is set in PcdCpuFeaturesSupport.
  @retval  FALSE  The CPU feature is not set in PcdCpuFeaturesSupport.

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
  Determines if a CPU feature is set in PcdCpuFeaturesCapability bit mask.

  @param[in]  Feature  The bit number of the CPU feature to check in the PCD
                       PcdCpuFeaturesCapability

  @retval  TRUE   The CPU feature is set in PcdCpuFeaturesCapability.
  @retval  FALSE  The CPU feature is not set in PcdCpuFeaturesCapability.

  @note This service could be called by BSP only.
**/
BOOLEAN
EFIAPI
IsCpuFeatureCapability (
  IN UINT32              Feature
  )
{
  return IsCpuFeatureSetInCpuPcd (
           (UINT8 *)PcdGetPtr (PcdCpuFeaturesCapability),
           PcdGetSize (PcdCpuFeaturesCapability),
           Feature
           );

}

/**
  Determines if a CPU feature is set in PcdCpuFeaturesUserConfiguration bit mask.

  @param[in]  Feature  The bit number of the CPU feature to check in the PCD
                       PcdCpuFeaturesUserConfiguration

  @retval  TRUE   The CPU feature is set in PcdCpuFeaturesUserConfiguration.
  @retval  FALSE  The CPU feature is not set in PcdCpuFeaturesUserConfiguration.

  @note This service could be called by BSP only.
**/
BOOLEAN
EFIAPI
IsCpuFeatureUserConfiguration (
  IN UINT32              Feature
  )
{
  return IsCpuFeatureSetInCpuPcd (
           (UINT8 *)PcdGetPtr (PcdCpuFeaturesUserConfiguration),
           PcdGetSize (PcdCpuFeaturesUserConfiguration),
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

