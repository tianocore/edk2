/** @file
  CPU Features Initialize functions.

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RegisterCpuFeatures.h"

CHAR16 *mDependTypeStr[]   = {L"None", L"Thread", L"Core", L"Package", L"Invalid" };
CHAR16 *mRegisterTypeStr[] = {L"MSR", L"CR", L"MMIO", L"CACHE", L"SEMAP", L"INVALID" };

/**
  Worker function to save PcdCpuFeaturesCapability.

  @param[in]  SupportedFeatureMask  The pointer to CPU feature bits mask buffer
  @param[in]  FeatureMaskSize       CPU feature bits mask buffer size.

**/
VOID
SetCapabilityPcd (
  IN UINT8               *SupportedFeatureMask,
  IN UINT32              FeatureMaskSize
  )
{
  EFI_STATUS             Status;
  UINTN                  BitMaskSize;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesCapability);
  ASSERT (FeatureMaskSize == BitMaskSize);

  Status = PcdSetPtrS (PcdCpuFeaturesCapability, &BitMaskSize, SupportedFeatureMask);
  ASSERT_EFI_ERROR (Status);
}

/**
  Worker function to save PcdCpuFeaturesSetting.

  @param[in]  SupportedFeatureMask  The pointer to CPU feature bits mask buffer
**/
VOID
SetSettingPcd (
  IN UINT8               *SupportedFeatureMask
  )
{
  EFI_STATUS             Status;
  UINTN                  BitMaskSize;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSetting);
  Status = PcdSetPtrS (PcdCpuFeaturesSetting, &BitMaskSize, SupportedFeatureMask);
  ASSERT_EFI_ERROR (Status);
}

/**
  Collects CPU type and feature information.

  @param[in, out]  CpuInfo  The pointer to CPU feature information
**/
VOID
FillProcessorInfo (
  IN OUT REGISTER_CPU_FEATURE_INFORMATION        *CpuInfo
  )
{
  CPUID_VERSION_INFO_EAX Eax;
  CPUID_VERSION_INFO_ECX Ecx;
  CPUID_VERSION_INFO_EDX Edx;
  UINT32                 DisplayedFamily;
  UINT32                 DisplayedModel;

  AsmCpuid (CPUID_VERSION_INFO, &Eax.Uint32, NULL, &Ecx.Uint32, &Edx.Uint32);

  DisplayedFamily = Eax.Bits.FamilyId;
  if (Eax.Bits.FamilyId == 0x0F) {
    DisplayedFamily |= (Eax.Bits.ExtendedFamilyId << 4);
  }

  DisplayedModel = Eax.Bits.Model;
  if (Eax.Bits.FamilyId == 0x06 || Eax.Bits.FamilyId == 0x0f) {
    DisplayedModel |= (Eax.Bits.ExtendedModelId << 4);
  }

  CpuInfo->DisplayFamily = DisplayedFamily;
  CpuInfo->DisplayModel  = DisplayedModel;
  CpuInfo->SteppingId    = Eax.Bits.SteppingId;
  CpuInfo->ProcessorType = Eax.Bits.ProcessorType;
  CpuInfo->CpuIdVersionInfoEcx.Uint32 = Ecx.Uint32;
  CpuInfo->CpuIdVersionInfoEdx.Uint32 = Edx.Uint32;
}

/**
  Prepares for private data used for CPU features.

**/
VOID
CpuInitDataInitialize (
  VOID
  )
{
  EFI_STATUS                           Status;
  UINTN                                ProcessorNumber;
  EFI_PROCESSOR_INFORMATION            ProcessorInfoBuffer;
  CPU_FEATURES_ENTRY                   *CpuFeature;
  CPU_FEATURES_INIT_ORDER              *InitOrder;
  CPU_FEATURES_DATA                    *CpuFeaturesData;
  LIST_ENTRY                           *Entry;
  UINT32                               Core;
  UINT32                               Package;
  UINT32                               Thread;
  EFI_CPU_PHYSICAL_LOCATION            *Location;
  BOOLEAN                              *CoresVisited;
  UINTN                                Index;
  ACPI_CPU_DATA                        *AcpiCpuData;
  CPU_STATUS_INFORMATION               *CpuStatus;
  UINT32                               *ValidCoreCountPerPackage;
  UINTN                                NumberOfCpus;
  UINTN                                NumberOfEnabledProcessors;

  Core    = 0;
  Package = 0;
  Thread  = 0;

  CpuFeaturesData = GetCpuFeaturesData ();

  //
  // Initialize CpuFeaturesData->MpService as early as possile, so later function can use it.
  //
  CpuFeaturesData->MpService = GetMpService ();

  GetNumberOfProcessor (&NumberOfCpus, &NumberOfEnabledProcessors);

  CpuFeaturesData->InitOrder = AllocateZeroPool (sizeof (CPU_FEATURES_INIT_ORDER) * NumberOfCpus);
  ASSERT (CpuFeaturesData->InitOrder != NULL);

  //
  // Collect CPU Features information
  //
  Entry = GetFirstNode (&CpuFeaturesData->FeatureList);
  while (!IsNull (&CpuFeaturesData->FeatureList, Entry)) {
    CpuFeature = CPU_FEATURE_ENTRY_FROM_LINK (Entry);
    ASSERT (CpuFeature->InitializeFunc != NULL);
    if (CpuFeature->GetConfigDataFunc != NULL) {
      CpuFeature->ConfigData = CpuFeature->GetConfigDataFunc (NumberOfCpus);
    }
    Entry = Entry->ForwardLink;
  }

  CpuFeaturesData->NumberOfCpus = (UINT32) NumberOfCpus;

  AcpiCpuData = GetAcpiCpuData ();
  ASSERT (AcpiCpuData != NULL);
  CpuFeaturesData->AcpiCpuData= AcpiCpuData;

  CpuStatus = &AcpiCpuData->CpuStatus;
  Location = AllocateZeroPool (sizeof (EFI_CPU_PHYSICAL_LOCATION) * NumberOfCpus);
  ASSERT (Location != NULL);
  AcpiCpuData->ApLocation = (EFI_PHYSICAL_ADDRESS)(UINTN)Location;

  for (ProcessorNumber = 0; ProcessorNumber < NumberOfCpus; ProcessorNumber++) {
    InitOrder = &CpuFeaturesData->InitOrder[ProcessorNumber];
    InitOrder->FeaturesSupportedMask = AllocateZeroPool (CpuFeaturesData->BitMaskSize);
    ASSERT (InitOrder->FeaturesSupportedMask != NULL);
    InitializeListHead (&InitOrder->OrderList);
    Status = GetProcessorInformation (ProcessorNumber, &ProcessorInfoBuffer);
    ASSERT_EFI_ERROR (Status);
    CopyMem (
      &InitOrder->CpuInfo.ProcessorInfo,
      &ProcessorInfoBuffer,
      sizeof (EFI_PROCESSOR_INFORMATION)
      );
    CopyMem (
      &Location[ProcessorNumber],
      &ProcessorInfoBuffer.Location,
      sizeof (EFI_CPU_PHYSICAL_LOCATION)
      );

    //
    // Collect CPU package count info.
    //
    if (Package < ProcessorInfoBuffer.Location.Package) {
      Package = ProcessorInfoBuffer.Location.Package;
    }
    //
    // Collect CPU max core count info.
    //
    if (Core < ProcessorInfoBuffer.Location.Core) {
      Core = ProcessorInfoBuffer.Location.Core;
    }
    //
    // Collect CPU max thread count info.
    //
    if (Thread < ProcessorInfoBuffer.Location.Thread) {
      Thread = ProcessorInfoBuffer.Location.Thread;
    }
  }
  CpuStatus->PackageCount    = Package + 1;
  CpuStatus->MaxCoreCount    = Core + 1;
  CpuStatus->MaxThreadCount  = Thread + 1;
  DEBUG ((DEBUG_INFO, "Processor Info: Package: %d, MaxCore : %d, MaxThread: %d\n",
         CpuStatus->PackageCount,
         CpuStatus->MaxCoreCount,
         CpuStatus->MaxThreadCount));

  //
  // Collect valid core count in each package because not all cores are valid.
  //
  ValidCoreCountPerPackage= AllocateZeroPool (sizeof (UINT32) * CpuStatus->PackageCount);
  ASSERT (ValidCoreCountPerPackage != 0);
  CpuStatus->ValidCoreCountPerPackage = (EFI_PHYSICAL_ADDRESS)(UINTN)ValidCoreCountPerPackage;
  CoresVisited = AllocatePool (sizeof (BOOLEAN) * CpuStatus->MaxCoreCount);
  ASSERT (CoresVisited != NULL);

  for (Index = 0; Index < CpuStatus->PackageCount; Index ++ ) {
    ZeroMem (CoresVisited, sizeof (BOOLEAN) * CpuStatus->MaxCoreCount);
    //
    // Collect valid cores in Current package.
    //
    for (ProcessorNumber = 0; ProcessorNumber < NumberOfCpus; ProcessorNumber++) {
      Location = &CpuFeaturesData->InitOrder[ProcessorNumber].CpuInfo.ProcessorInfo.Location;
      if (Location->Package == Index && !CoresVisited[Location->Core] ) {
        //
        // The ValidCores position for Location->Core is valid.
        // The possible values in ValidCores[Index] are 0 or 1.
        // FALSE means no valid threads in this Core.
        // TRUE means have valid threads in this core, no matter the thead count is 1 or more.
        //
        CoresVisited[Location->Core] = TRUE;
        ValidCoreCountPerPackage[Index]++;
      }
    }
  }
  FreePool (CoresVisited);

  for (Index = 0; Index <= Package; Index++) {
    DEBUG ((DEBUG_INFO, "Package: %d, Valid Core : %d\n", Index, ValidCoreCountPerPackage[Index]));
  }

  CpuFeaturesData->CpuFlags.CoreSemaphoreCount = AllocateZeroPool (sizeof (UINT32) * CpuStatus->PackageCount * CpuStatus->MaxCoreCount * CpuStatus->MaxThreadCount);
  ASSERT (CpuFeaturesData->CpuFlags.CoreSemaphoreCount != NULL);
  CpuFeaturesData->CpuFlags.PackageSemaphoreCount = AllocateZeroPool (sizeof (UINT32) * CpuStatus->PackageCount * CpuStatus->MaxCoreCount * CpuStatus->MaxThreadCount);
  ASSERT (CpuFeaturesData->CpuFlags.PackageSemaphoreCount != NULL);
}

/**
  Worker function to do OR operation on CPU feature supported bits mask buffer.

  @param[in]  SupportedFeatureMask  The pointer to CPU feature bits mask buffer
  @param[in]  OrFeatureBitMask      The feature bit mask to do OR operation
**/
VOID
SupportedMaskOr (
  IN UINT8               *SupportedFeatureMask,
  IN UINT8               *OrFeatureBitMask
  )
{
  UINTN                  Index;
  UINTN                  BitMaskSize;
  UINT8                  *Data1;
  UINT8                  *Data2;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSetting);
  Data1 = SupportedFeatureMask;
  Data2 = OrFeatureBitMask;
  for (Index = 0; Index < BitMaskSize; Index++) {
    *(Data1++) |=  *(Data2++);
  }
}

/**
  Worker function to do AND operation on CPU feature supported bits mask buffer.

  @param[in]  SupportedFeatureMask  The pointer to CPU feature bits mask buffer
  @param[in]  AndFeatureBitMask     The feature bit mask to do AND operation
**/
VOID
SupportedMaskAnd (
  IN       UINT8               *SupportedFeatureMask,
  IN CONST UINT8               *AndFeatureBitMask
  )
{
  UINTN                  Index;
  UINTN                  BitMaskSize;
  UINT8                  *Data1;
  CONST UINT8            *Data2;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSetting);
  Data1 = SupportedFeatureMask;
  Data2 = AndFeatureBitMask;
  for (Index = 0; Index < BitMaskSize; Index++) {
    *(Data1++) &=  *(Data2++);
  }
}

/**
  Worker function to clean bit operation on CPU feature supported bits mask buffer.

  @param[in]  SupportedFeatureMask  The pointer to CPU feature bits mask buffer
  @param[in]  AndFeatureBitMask     The feature bit mask to do XOR operation
**/
VOID
SupportedMaskCleanBit (
  IN UINT8               *SupportedFeatureMask,
  IN UINT8               *AndFeatureBitMask
  )
{
  UINTN                  Index;
  UINTN                  BitMaskSize;
  UINT8                  *Data1;
  UINT8                  *Data2;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSetting);
  Data1 = SupportedFeatureMask;
  Data2 = AndFeatureBitMask;
  for (Index = 0; Index < BitMaskSize; Index++) {
    *(Data1++) &=  ~(*(Data2++));
  }
}

/**
  Worker function to check if the compared CPU feature set in the CPU feature
  supported bits mask buffer.

  @param[in]  SupportedFeatureMask   The pointer to CPU feature bits mask buffer
  @param[in]  ComparedFeatureBitMask The feature bit mask to be compared

  @retval TRUE   The ComparedFeatureBitMask is set in CPU feature supported bits
                 mask buffer.
  @retval FALSE  The ComparedFeatureBitMask is not set in CPU feature supported bits
                 mask buffer.
**/
BOOLEAN
IsBitMaskMatch (
  IN UINT8               *SupportedFeatureMask,
  IN UINT8               *ComparedFeatureBitMask
  )
{
  UINTN                  Index;
  UINTN                  BitMaskSize;
  UINT8                  *Data1;
  UINT8                  *Data2;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSetting);

  Data1 = SupportedFeatureMask;
  Data2 = ComparedFeatureBitMask;
  for (Index = 0; Index < BitMaskSize; Index++) {
    if (((*(Data1++)) & (*(Data2++))) != 0) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
  Collects processor data for calling processor.

  @param[in,out]  Buffer  The pointer to private data buffer.
**/
VOID
EFIAPI
CollectProcessorData (
  IN OUT VOID                          *Buffer
  )
{
  UINTN                                ProcessorNumber;
  CPU_FEATURES_ENTRY                   *CpuFeature;
  REGISTER_CPU_FEATURE_INFORMATION     *CpuInfo;
  LIST_ENTRY                           *Entry;
  CPU_FEATURES_DATA                    *CpuFeaturesData;

  CpuFeaturesData = (CPU_FEATURES_DATA *)Buffer;
  ProcessorNumber = GetProcessorIndex (CpuFeaturesData);
  CpuInfo = &CpuFeaturesData->InitOrder[ProcessorNumber].CpuInfo;
  //
  // collect processor information
  //
  FillProcessorInfo (CpuInfo);
  Entry = GetFirstNode (&CpuFeaturesData->FeatureList);
  while (!IsNull (&CpuFeaturesData->FeatureList, Entry)) {
    CpuFeature = CPU_FEATURE_ENTRY_FROM_LINK (Entry);
    if (CpuFeature->SupportFunc == NULL) {
      //
      // If SupportFunc is NULL, then the feature is supported.
      //
      SupportedMaskOr (
        CpuFeaturesData->InitOrder[ProcessorNumber].FeaturesSupportedMask,
        CpuFeature->FeatureMask
        );
    } else if (CpuFeature->SupportFunc (ProcessorNumber, CpuInfo, CpuFeature->ConfigData)) {
      SupportedMaskOr (
        CpuFeaturesData->InitOrder[ProcessorNumber].FeaturesSupportedMask,
        CpuFeature->FeatureMask
        );
    }
    Entry = Entry->ForwardLink;
  }
}

/**
  Dump the contents of a CPU register table.

  @param[in]  ProcessorNumber  The index of the CPU to show the register table contents

  @note This service could be called by BSP only.
**/
VOID
DumpRegisterTableOnProcessor (
  IN UINTN                             ProcessorNumber
  )
{
  CPU_FEATURES_DATA                    *CpuFeaturesData;
  UINTN                                FeatureIndex;
  CPU_REGISTER_TABLE                   *RegisterTable;
  CPU_REGISTER_TABLE_ENTRY             *RegisterTableEntry;
  CPU_REGISTER_TABLE_ENTRY             *RegisterTableEntryHead;
  UINT32                               DebugPrintErrorLevel;

  DebugPrintErrorLevel = (ProcessorNumber == 0) ? DEBUG_INFO : DEBUG_VERBOSE;
  CpuFeaturesData      = GetCpuFeaturesData ();
  //
  // Debug information
  //
  RegisterTable = &CpuFeaturesData->RegisterTable[ProcessorNumber];
  DEBUG ((DebugPrintErrorLevel, "RegisterTable->TableLength = %d\n", RegisterTable->TableLength));

  RegisterTableEntryHead = (CPU_REGISTER_TABLE_ENTRY *) (UINTN) RegisterTable->RegisterTableEntry;

  for (FeatureIndex = 0; FeatureIndex < RegisterTable->TableLength; FeatureIndex++) {
    RegisterTableEntry = &RegisterTableEntryHead[FeatureIndex];
    switch (RegisterTableEntry->RegisterType) {
    case Msr:
      DEBUG ((
        DebugPrintErrorLevel,
        "Processor: %04d: Index %04d, MSR  : %08x, Bit Start: %02d, Bit Length: %02d, Value: %016lx\r\n",
        ProcessorNumber,
        FeatureIndex,
        RegisterTableEntry->Index,
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitLength,
        RegisterTableEntry->Value
        ));
      break;
    case ControlRegister:
      DEBUG ((
        DebugPrintErrorLevel,
        "Processor: %04d: Index %04d, CR   : %08x, Bit Start: %02d, Bit Length: %02d, Value: %016lx\r\n",
        ProcessorNumber,
        FeatureIndex,
        RegisterTableEntry->Index,
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitLength,
        RegisterTableEntry->Value
        ));
      break;
    case MemoryMapped:
      DEBUG ((
        DebugPrintErrorLevel,
        "Processor: %04d: Index %04d, MMIO : %08lx, Bit Start: %02d, Bit Length: %02d, Value: %016lx\r\n",
        ProcessorNumber,
        FeatureIndex,
        RegisterTableEntry->Index | LShiftU64 (RegisterTableEntry->HighIndex, 32),
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitLength,
        RegisterTableEntry->Value
        ));
      break;
    case CacheControl:
      DEBUG ((
        DebugPrintErrorLevel,
        "Processor: %04d: Index %04d, CACHE: %08lx, Bit Start: %02d, Bit Length: %02d, Value: %016lx\r\n",
        ProcessorNumber,
        FeatureIndex,
        RegisterTableEntry->Index,
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitLength,
        RegisterTableEntry->Value
        ));
      break;
    case Semaphore:
      DEBUG ((
        DebugPrintErrorLevel,
        "Processor: %04d: Index %04d, SEMAP: %s\r\n",
        ProcessorNumber,
        FeatureIndex,
        mDependTypeStr[MIN ((UINT32)RegisterTableEntry->Value, InvalidDepType)]
        ));
      break;

    default:
      break;
    }
  }
}

/**
  Get the biggest dependence type.
  PackageDepType > CoreDepType > ThreadDepType > NoneDepType.

  @param[in]  BeforeDep           Before dependence type.
  @param[in]  AfterDep            After dependence type.
  @param[in]  NoneNeibBeforeDep   Before dependence type for not neighborhood features.
  @param[in]  NoneNeibAfterDep    After dependence type for not neighborhood features.

  @retval  Return the biggest dependence type.
**/
CPU_FEATURE_DEPENDENCE_TYPE
BiggestDep (
  IN CPU_FEATURE_DEPENDENCE_TYPE  BeforeDep,
  IN CPU_FEATURE_DEPENDENCE_TYPE  AfterDep,
  IN CPU_FEATURE_DEPENDENCE_TYPE  NoneNeibBeforeDep,
  IN CPU_FEATURE_DEPENDENCE_TYPE  NoneNeibAfterDep
  )
{
  CPU_FEATURE_DEPENDENCE_TYPE Bigger;

  Bigger = MAX (BeforeDep, AfterDep);
  Bigger = MAX (Bigger, NoneNeibBeforeDep);
  return MAX(Bigger, NoneNeibAfterDep);
}

/**
  Analysis register CPU features on each processor and save CPU setting in CPU register table.

  @param[in]  NumberOfCpus  Number of processor in system

**/
VOID
AnalysisProcessorFeatures (
  IN UINTN                             NumberOfCpus
  )
{
  EFI_STATUS                           Status;
  UINTN                                ProcessorNumber;
  CPU_FEATURES_ENTRY                   *CpuFeature;
  CPU_FEATURES_ENTRY                   *CpuFeatureInOrder;
  CPU_FEATURES_INIT_ORDER              *CpuInitOrder;
  REGISTER_CPU_FEATURE_INFORMATION     *CpuInfo;
  LIST_ENTRY                           *Entry;
  CPU_FEATURES_DATA                    *CpuFeaturesData;
  LIST_ENTRY                           *NextEntry;
  CPU_FEATURES_ENTRY                   *NextCpuFeatureInOrder;
  BOOLEAN                              Success;
  CPU_FEATURE_DEPENDENCE_TYPE          BeforeDep;
  CPU_FEATURE_DEPENDENCE_TYPE          AfterDep;
  CPU_FEATURE_DEPENDENCE_TYPE          NoneNeibBeforeDep;
  CPU_FEATURE_DEPENDENCE_TYPE          NoneNeibAfterDep;

  CpuFeaturesData = GetCpuFeaturesData ();
  CpuFeaturesData->CapabilityPcd = AllocatePool (CpuFeaturesData->BitMaskSize);
  ASSERT (CpuFeaturesData->CapabilityPcd != NULL);
  SetMem (CpuFeaturesData->CapabilityPcd, CpuFeaturesData->BitMaskSize, 0xFF);
  for (ProcessorNumber = 0; ProcessorNumber < NumberOfCpus; ProcessorNumber++) {
    CpuInitOrder = &CpuFeaturesData->InitOrder[ProcessorNumber];
    //
    // Calculate the last capability on all processors
    //
    SupportedMaskAnd (CpuFeaturesData->CapabilityPcd, CpuInitOrder->FeaturesSupportedMask);
  }
  //
  // Calculate the last setting
  //
  CpuFeaturesData->SettingPcd = AllocateCopyPool (CpuFeaturesData->BitMaskSize, CpuFeaturesData->CapabilityPcd);
  ASSERT (CpuFeaturesData->SettingPcd != NULL);
  SupportedMaskAnd (CpuFeaturesData->SettingPcd, PcdGetPtr (PcdCpuFeaturesSetting));

  //
  // Dump the last CPU feature list
  //
  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "Last CPU features list...\n"));
    Entry = GetFirstNode (&CpuFeaturesData->FeatureList);
    while (!IsNull (&CpuFeaturesData->FeatureList, Entry)) {
      CpuFeature = CPU_FEATURE_ENTRY_FROM_LINK (Entry);
      if (IsBitMaskMatch (CpuFeature->FeatureMask, CpuFeaturesData->CapabilityPcd)) {
        if (IsBitMaskMatch (CpuFeature->FeatureMask, CpuFeaturesData->SettingPcd)) {
          DEBUG ((DEBUG_INFO, "[Enable   ] "));
        } else {
          DEBUG ((DEBUG_INFO, "[Disable  ] "));
        }
      } else {
        DEBUG ((DEBUG_INFO, "[Unsupport] "));
      }
      DumpCpuFeature (CpuFeature);
      Entry = Entry->ForwardLink;
    }
    DEBUG ((DEBUG_INFO, "PcdCpuFeaturesCapability:\n"));
    DumpCpuFeatureMask (CpuFeaturesData->CapabilityPcd);
    DEBUG ((DEBUG_INFO, "Origin PcdCpuFeaturesSetting:\n"));
    DumpCpuFeatureMask (PcdGetPtr (PcdCpuFeaturesSetting));
    DEBUG ((DEBUG_INFO, "Final PcdCpuFeaturesSetting:\n"));
    DumpCpuFeatureMask (CpuFeaturesData->SettingPcd);
  );

  //
  // Save PCDs and display CPU PCDs
  //
  SetCapabilityPcd (CpuFeaturesData->CapabilityPcd, CpuFeaturesData->BitMaskSize);
  SetSettingPcd (CpuFeaturesData->SettingPcd);

  for (ProcessorNumber = 0; ProcessorNumber < NumberOfCpus; ProcessorNumber++) {
    CpuInitOrder = &CpuFeaturesData->InitOrder[ProcessorNumber];
    Entry = GetFirstNode (&CpuFeaturesData->FeatureList);
    while (!IsNull (&CpuFeaturesData->FeatureList, Entry)) {
      //
      // Insert each feature into processor's order list
      //
      CpuFeature = CPU_FEATURE_ENTRY_FROM_LINK (Entry);
      if (IsBitMaskMatch (CpuFeature->FeatureMask, CpuFeaturesData->CapabilityPcd)) {
        CpuFeatureInOrder = AllocateCopyPool (sizeof (CPU_FEATURES_ENTRY), CpuFeature);
        ASSERT (CpuFeatureInOrder != NULL);
        InsertTailList (&CpuInitOrder->OrderList, &CpuFeatureInOrder->Link);
      }
      Entry = Entry->ForwardLink;
    }
    //
    // Go through ordered feature list to initialize CPU features
    //
    CpuInfo = &CpuFeaturesData->InitOrder[ProcessorNumber].CpuInfo;
    Entry = GetFirstNode (&CpuInitOrder->OrderList);
    while (!IsNull (&CpuInitOrder->OrderList, Entry)) {
      CpuFeatureInOrder = CPU_FEATURE_ENTRY_FROM_LINK (Entry);

      Success = FALSE;
      if (IsBitMaskMatch (CpuFeatureInOrder->FeatureMask, CpuFeaturesData->SettingPcd)) {
        Status = CpuFeatureInOrder->InitializeFunc (ProcessorNumber, CpuInfo, CpuFeatureInOrder->ConfigData, TRUE);
        if (EFI_ERROR (Status)) {
          //
          // Clean the CpuFeatureInOrder->FeatureMask in setting PCD.
          //
          SupportedMaskCleanBit (CpuFeaturesData->SettingPcd, CpuFeatureInOrder->FeatureMask);
          if (CpuFeatureInOrder->FeatureName != NULL) {
            DEBUG ((DEBUG_WARN, "Warning :: Failed to enable Feature: Name = %a.\n", CpuFeatureInOrder->FeatureName));
          } else {
            DEBUG ((DEBUG_WARN, "Warning :: Failed to enable Feature: Mask = "));
            DumpCpuFeatureMask (CpuFeatureInOrder->FeatureMask);
          }
        } else {
          Success = TRUE;
        }
      } else {
        Status = CpuFeatureInOrder->InitializeFunc (ProcessorNumber, CpuInfo, CpuFeatureInOrder->ConfigData, FALSE);
        if (EFI_ERROR (Status)) {
          if (CpuFeatureInOrder->FeatureName != NULL) {
            DEBUG ((DEBUG_WARN, "Warning :: Failed to disable Feature: Name = %a.\n", CpuFeatureInOrder->FeatureName));
          } else {
            DEBUG ((DEBUG_WARN, "Warning :: Failed to disable Feature: Mask = "));
            DumpCpuFeatureMask (CpuFeatureInOrder->FeatureMask);
          }
        } else {
          Success = TRUE;
        }
      }

      if (Success) {
        NextEntry = Entry->ForwardLink;
        if (!IsNull (&CpuInitOrder->OrderList, NextEntry)) {
          NextCpuFeatureInOrder = CPU_FEATURE_ENTRY_FROM_LINK (NextEntry);

          //
          // If feature has dependence with the next feature (ONLY care core/package dependency).
          // and feature initialize succeed, add sync semaphere here.
          //
          BeforeDep = DetectFeatureScope (CpuFeatureInOrder, TRUE, NextCpuFeatureInOrder->FeatureMask);
          AfterDep  = DetectFeatureScope (NextCpuFeatureInOrder, FALSE, CpuFeatureInOrder->FeatureMask);
          //
          // Check whether next feature has After type dependence with not neighborhood CPU
          // Features in former CPU features.
          //
          NoneNeibAfterDep = DetectNoneNeighborhoodFeatureScope(NextCpuFeatureInOrder, FALSE, &CpuInitOrder->OrderList);
        } else {
          BeforeDep        = NoneDepType;
          AfterDep         = NoneDepType;
          NoneNeibAfterDep = NoneDepType;
        }
        //
        // Check whether current feature has Before type dependence with none neighborhood
        // CPU features in after Cpu features.
        //
        NoneNeibBeforeDep = DetectNoneNeighborhoodFeatureScope(CpuFeatureInOrder, TRUE, &CpuInitOrder->OrderList);

        //
        // Get the biggest dependence and add semaphore for it.
        // PackageDepType > CoreDepType > ThreadDepType > NoneDepType.
        //
        BeforeDep = BiggestDep(BeforeDep, AfterDep, NoneNeibBeforeDep, NoneNeibAfterDep);
        if (BeforeDep > ThreadDepType) {
          CPU_REGISTER_TABLE_WRITE32 (ProcessorNumber, Semaphore, 0, BeforeDep);
        }
      }

      Entry = Entry->ForwardLink;
    }

    //
    // Dump PcdCpuFeaturesSetting again because this value maybe updated
    // again during initialize the features.
    //
    DEBUG ((DEBUG_INFO, "Dump final value for PcdCpuFeaturesSetting:\n"));
    DumpCpuFeatureMask (CpuFeaturesData->SettingPcd);

    //
    // Dump the RegisterTable
    //
    DumpRegisterTableOnProcessor (ProcessorNumber);
  }
}

/**
  Increment semaphore by 1.

  @param      Sem            IN:  32-bit unsigned integer

**/
VOID
LibReleaseSemaphore (
  IN OUT  volatile UINT32           *Sem
  )
{
  InterlockedIncrement (Sem);
}

/**
  Decrement the semaphore by 1 if it is not zero.

  Performs an atomic decrement operation for semaphore.
  The compare exchange operation must be performed using
  MP safe mechanisms.

  @param      Sem            IN:  32-bit unsigned integer

**/
VOID
LibWaitForSemaphore (
  IN OUT  volatile UINT32           *Sem
  )
{
  UINT32  Value;

  do {
    Value = *Sem;
  } while (Value == 0 ||
           InterlockedCompareExchange32 (
             Sem,
             Value,
             Value - 1
             ) != Value);
}

/**
  Initialize the CPU registers from a register table.

  @param[in]  RegisterTable         The register table for this AP.
  @param[in]  ApLocation            AP location info for this ap.
  @param[in]  CpuStatus             CPU status info for this CPU.
  @param[in]  CpuFlags              Flags data structure used when program the register.

  @note This service could be called by BSP/APs.
**/
VOID
ProgramProcessorRegister (
  IN CPU_REGISTER_TABLE           *RegisterTable,
  IN EFI_CPU_PHYSICAL_LOCATION    *ApLocation,
  IN CPU_STATUS_INFORMATION       *CpuStatus,
  IN PROGRAM_CPU_REGISTER_FLAGS   *CpuFlags
  )
{
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntry;
  UINTN                     Index;
  UINTN                     Value;
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntryHead;
  volatile UINT32           *SemaphorePtr;
  UINT32                    FirstThread;
  UINT32                    PackageThreadsCount;
  UINT32                    CurrentThread;
  UINTN                     ProcessorIndex;
  UINTN                     ThreadIndex;
  UINTN                     ValidThreadCount;
  UINT32                    *ValidCoreCountPerPackage;

  //
  // Traverse Register Table of this logical processor
  //
  RegisterTableEntryHead = (CPU_REGISTER_TABLE_ENTRY *) (UINTN) RegisterTable->RegisterTableEntry;

  for (Index = 0; Index < RegisterTable->TableLength; Index++) {

    RegisterTableEntry = &RegisterTableEntryHead[Index];

    DEBUG_CODE_BEGIN ();
      //
      // Wait for the AP to release the MSR spin lock.
      //
      while (!AcquireSpinLockOrFail (&CpuFlags->ConsoleLogLock)) {
        CpuPause ();
      }
      ThreadIndex = ApLocation->Package * CpuStatus->MaxCoreCount * CpuStatus->MaxThreadCount +
              ApLocation->Core * CpuStatus->MaxThreadCount +
              ApLocation->Thread;
      DEBUG ((
        DEBUG_INFO,
        "Processor = %08lu, Index %08lu, Type = %s!\n",
        (UINT64)ThreadIndex,
        (UINT64)Index,
        mRegisterTypeStr[MIN ((REGISTER_TYPE)RegisterTableEntry->RegisterType, InvalidReg)]
        ));
      ReleaseSpinLock (&CpuFlags->ConsoleLogLock);
    DEBUG_CODE_END ();

    //
    // Check the type of specified register
    //
    switch (RegisterTableEntry->RegisterType) {
    //
    // The specified register is Control Register
    //
    case ControlRegister:
      switch (RegisterTableEntry->Index) {
      case 0:
        Value = AsmReadCr0 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          RegisterTableEntry->Value
                          );
        AsmWriteCr0 (Value);
        break;
      case 2:
        Value = AsmReadCr2 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          RegisterTableEntry->Value
                          );
        AsmWriteCr2 (Value);
        break;
      case 3:
        Value = AsmReadCr3 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          RegisterTableEntry->Value
                          );
        AsmWriteCr3 (Value);
        break;
      case 4:
        Value = AsmReadCr4 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          RegisterTableEntry->Value
                          );
        AsmWriteCr4 (Value);
        break;
      case 8:
        //
        //  Do we need to support CR8?
        //
        break;
      default:
        break;
      }
      break;
    //
    // The specified register is Model Specific Register
    //
    case Msr:
      if (RegisterTableEntry->ValidBitLength >= 64) {
        //
        // If length is not less than 64 bits, then directly write without reading
        //
        AsmWriteMsr64 (
          RegisterTableEntry->Index,
          RegisterTableEntry->Value
          );
      } else {
        //
        // Set the bit section according to bit start and length
        //
        AsmMsrBitFieldWrite64 (
          RegisterTableEntry->Index,
          RegisterTableEntry->ValidBitStart,
          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
          RegisterTableEntry->Value
          );
      }
      break;
    //
    // MemoryMapped operations
    //
    case MemoryMapped:
      AcquireSpinLock (&CpuFlags->MemoryMappedLock);
      MmioBitFieldWrite32 (
        (UINTN)(RegisterTableEntry->Index | LShiftU64 (RegisterTableEntry->HighIndex, 32)),
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
        (UINT32)RegisterTableEntry->Value
        );
      ReleaseSpinLock (&CpuFlags->MemoryMappedLock);
      break;
    //
    // Enable or disable cache
    //
    case CacheControl:
      //
      // If value of the entry is 0, then disable cache.  Otherwise, enable cache.
      //
      if (RegisterTableEntry->Value == 0) {
        AsmDisableCache ();
      } else {
        AsmEnableCache ();
      }
      break;

    case Semaphore:
      // Semaphore works logic like below:
      //
      //  V(x) = LibReleaseSemaphore (Semaphore[FirstThread + x]);
      //  P(x) = LibWaitForSemaphore (Semaphore[FirstThread + x]);
      //
      //  All threads (T0...Tn) waits in P() line and continues running
      //  together.
      //
      //
      //  T0             T1            ...           Tn
      //
      //  V(0...n)       V(0...n)      ...           V(0...n)
      //  n * P(0)       n * P(1)      ...           n * P(n)
      //
      switch (RegisterTableEntry->Value) {
      case CoreDepType:
        SemaphorePtr = CpuFlags->CoreSemaphoreCount;
        //
        // Get Offset info for the first thread in the core which current thread belongs to.
        //
        FirstThread = (ApLocation->Package * CpuStatus->MaxCoreCount + ApLocation->Core) * CpuStatus->MaxThreadCount;
        CurrentThread = FirstThread + ApLocation->Thread;
        //
        // First Notify all threads in current Core that this thread has ready.
        //
        for (ProcessorIndex = 0; ProcessorIndex < CpuStatus->MaxThreadCount; ProcessorIndex ++) {
          LibReleaseSemaphore ((UINT32 *) &SemaphorePtr[FirstThread + ProcessorIndex]);
        }
        //
        // Second, check whether all valid threads in current core have ready.
        //
        for (ProcessorIndex = 0; ProcessorIndex < CpuStatus->MaxThreadCount; ProcessorIndex ++) {
          LibWaitForSemaphore (&SemaphorePtr[CurrentThread]);
        }
        break;

      case PackageDepType:
        SemaphorePtr = CpuFlags->PackageSemaphoreCount;
        ValidCoreCountPerPackage = (UINT32 *)(UINTN)CpuStatus->ValidCoreCountPerPackage;
        //
        // Get Offset info for the first thread in the package which current thread belongs to.
        //
        FirstThread = ApLocation->Package * CpuStatus->MaxCoreCount * CpuStatus->MaxThreadCount;
        //
        // Get the possible threads count for current package.
        //
        PackageThreadsCount = CpuStatus->MaxThreadCount * CpuStatus->MaxCoreCount;
        CurrentThread = FirstThread + CpuStatus->MaxThreadCount * ApLocation->Core + ApLocation->Thread;
        //
        // Get the valid thread count for current package.
        //
        ValidThreadCount = CpuStatus->MaxThreadCount * ValidCoreCountPerPackage[ApLocation->Package];

        //
        // Different packages may have different valid cores in them. If driver maintail clearly
        // cores number in different packages, the logic will be much complicated.
        // Here driver just simply records the max core number in all packages and use it as expect
        // core number for all packages.
        // In below two steps logic, first current thread will Release semaphore for each thread
        // in current package. Maybe some threads are not valid in this package, but driver don't
        // care. Second, driver will let current thread wait semaphore for all valid threads in
        // current package. Because only the valid threads will do release semaphore for this
        // thread, driver here only need to wait the valid thread count.
        //

        //
        // First Notify ALL THREADS in current package that this thread has ready.
        //
        for (ProcessorIndex = 0; ProcessorIndex < PackageThreadsCount ; ProcessorIndex ++) {
          LibReleaseSemaphore ((UINT32 *) &SemaphorePtr[FirstThread + ProcessorIndex]);
        }
        //
        // Second, check whether VALID THREADS (not all threads) in current package have ready.
        //
        for (ProcessorIndex = 0; ProcessorIndex < ValidThreadCount; ProcessorIndex ++) {
          LibWaitForSemaphore (&SemaphorePtr[CurrentThread]);
        }
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }
  }
}

/**
  Programs registers for the calling processor.

  @param[in,out] Buffer  The pointer to private data buffer.

**/
VOID
EFIAPI
SetProcessorRegister (
  IN OUT VOID            *Buffer
  )
{
  CPU_FEATURES_DATA         *CpuFeaturesData;
  CPU_REGISTER_TABLE        *RegisterTable;
  CPU_REGISTER_TABLE        *RegisterTables;
  UINT32                    InitApicId;
  UINTN                     ProcIndex;
  UINTN                     Index;
  ACPI_CPU_DATA             *AcpiCpuData;

  CpuFeaturesData = (CPU_FEATURES_DATA *) Buffer;
  AcpiCpuData = CpuFeaturesData->AcpiCpuData;

  RegisterTables = (CPU_REGISTER_TABLE *)(UINTN)AcpiCpuData->RegisterTable;

  InitApicId = GetInitialApicId ();
  RegisterTable = NULL;
  ProcIndex = (UINTN)-1;
  for (Index = 0; Index < AcpiCpuData->NumberOfCpus; Index++) {
    if (RegisterTables[Index].InitialApicId == InitApicId) {
      RegisterTable =  &RegisterTables[Index];
      ProcIndex = Index;
      break;
    }
  }
  ASSERT (RegisterTable != NULL);

  ProgramProcessorRegister (
    RegisterTable,
    (EFI_CPU_PHYSICAL_LOCATION *)(UINTN)AcpiCpuData->ApLocation + ProcIndex,
    &AcpiCpuData->CpuStatus,
    &CpuFeaturesData->CpuFlags
    );
}

/**
  Performs CPU features detection.

  This service will invoke MP service to check CPU features'
  capabilities on BSP/APs.

  @note This service could be called by BSP only.
**/
VOID
EFIAPI
CpuFeaturesDetect (
  VOID
  )
{
  CPU_FEATURES_DATA      *CpuFeaturesData;

  CpuFeaturesData = GetCpuFeaturesData();

  CpuInitDataInitialize ();

  //
  // Wakeup all APs for data collection.
  //
  StartupAPsWorker (CollectProcessorData, NULL);

  //
  // Collect data on BSP
  //
  CollectProcessorData (CpuFeaturesData);

  AnalysisProcessorFeatures (CpuFeaturesData->NumberOfCpus);
}

