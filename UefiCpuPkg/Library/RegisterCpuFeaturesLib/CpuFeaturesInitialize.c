/** @file
  CPU Features Initialize functions.

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
  Worker function to save PcdCpuFeaturesCapability.

  @param[in]  SupportedFeatureMask  The pointer to CPU feature bits mask buffer
**/
VOID
SetCapabilityPcd (
  IN UINT8               *SupportedFeatureMask
  )
{
  EFI_STATUS             Status;
  UINTN                  BitMaskSize;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesCapability);
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
  Worker function to get PcdCpuFeaturesSupport.

  @return  The pointer to CPU feature bits mask buffer.
**/
UINT8 *
GetSupportPcds (
  VOID
  )
{
  UINT8                  *SupportBitMask;

  SupportBitMask = AllocateCopyPool (
          PcdGetSize (PcdCpuFeaturesSupport), 
          PcdGetPtr (PcdCpuFeaturesSupport)
          );
  ASSERT (SupportBitMask != NULL);

  return SupportBitMask;
}

/**
  Worker function to get PcdCpuFeaturesUserConfiguration.

  @return  The pointer to CPU feature bits mask buffer.
**/
UINT8 *
GetConfigurationPcds (
  VOID
  )
{
  UINT8                  *SupportBitMask;

  SupportBitMask = AllocateCopyPool (
          PcdGetSize (PcdCpuFeaturesUserConfiguration), 
          PcdGetPtr (PcdCpuFeaturesUserConfiguration)
          );
  ASSERT (SupportBitMask != NULL);

  return SupportBitMask;
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

  @param[in]  NumberOfCpus  Number of processor in system
**/
VOID
CpuInitDataInitialize (
  IN UINTN                             NumberOfCpus
  )
{
  EFI_STATUS                           Status;
  UINTN                                ProcessorNumber;
  EFI_PROCESSOR_INFORMATION            ProcessorInfoBuffer;
  CPU_FEATURES_ENTRY                   *CpuFeature;
  CPU_FEATURES_INIT_ORDER              *InitOrder;
  CPU_FEATURES_DATA                    *CpuFeaturesData;
  LIST_ENTRY                           *Entry;

  CpuFeaturesData = GetCpuFeaturesData ();
  CpuFeaturesData->InitOrder = AllocateZeroPool (sizeof (CPU_FEATURES_INIT_ORDER) * NumberOfCpus);
  ASSERT (CpuFeaturesData->InitOrder != NULL);
  CpuFeaturesData->BitMaskSize = (UINT32) PcdGetSize (PcdCpuFeaturesSupport);

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
  }
  //
  // Get support and configuration PCDs
  //
  CpuFeaturesData->SupportPcds       = GetSupportPcds ();
  CpuFeaturesData->ConfigurationPcds = GetConfigurationPcds ();
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

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSupport);
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
  IN UINT8               *SupportedFeatureMask,
  IN UINT8               *AndFeatureBitMask
  )
{
  UINTN                  Index;
  UINTN                  BitMaskSize;
  UINT8                  *Data1;
  UINT8                  *Data2;

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSupport);
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

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSupport);
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

  BitMaskSize = PcdGetSize (PcdCpuFeaturesSupport);

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

  CpuFeaturesData = GetCpuFeaturesData ();
  ProcessorNumber = GetProcessorIndex ();
  CpuInfo = &CpuFeaturesData->InitOrder[ProcessorNumber].CpuInfo;
  //
  // collect processor information
  //
  FillProcessorInfo (CpuInfo);
  Entry = GetFirstNode (&CpuFeaturesData->FeatureList);
  while (!IsNull (&CpuFeaturesData->FeatureList, Entry)) {
    CpuFeature = CPU_FEATURE_ENTRY_FROM_LINK (Entry);
    if (IsBitMaskMatch (CpuFeaturesData->SupportPcds, CpuFeature->FeatureMask)) {
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
        "Processor: %d:   MSR: %x, Bit Start: %d, Bit Length: %d, Value: %lx\r\n",
        ProcessorNumber,
        RegisterTableEntry->Index,
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitLength,
        RegisterTableEntry->Value
        ));
      break;
    case ControlRegister:
      DEBUG ((
        DebugPrintErrorLevel,
        "Processor: %d:    CR: %x, Bit Start: %d, Bit Length: %d, Value: %lx\r\n",
        ProcessorNumber,
        RegisterTableEntry->Index,
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitLength,
        RegisterTableEntry->Value
        ));
      break;
    case MemoryMapped:
      DEBUG ((
        DebugPrintErrorLevel,
        "Processor: %d:  MMIO: %lx, Bit Start: %d, Bit Length: %d, Value: %lx\r\n",
        ProcessorNumber,
        RegisterTableEntry->Index | LShiftU64 (RegisterTableEntry->HighIndex, 32),
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitLength,
        RegisterTableEntry->Value
        ));
      break;
    case CacheControl:
      DEBUG ((
        DebugPrintErrorLevel,
        "Processor: %d: CACHE: %x, Bit Start: %d, Bit Length: %d, Value: %lx\r\n",
        ProcessorNumber,
        RegisterTableEntry->Index,
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitLength,
        RegisterTableEntry->Value
        ));
      break;
    default:
      break;
    }
  }
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

  CpuFeaturesData = GetCpuFeaturesData ();
  CpuFeaturesData->CapabilityPcds = AllocatePool (CpuFeaturesData->BitMaskSize);
  ASSERT (CpuFeaturesData->CapabilityPcds != NULL);
  SetMem (CpuFeaturesData->CapabilityPcds, CpuFeaturesData->BitMaskSize, 0xFF);
  for (ProcessorNumber = 0; ProcessorNumber < NumberOfCpus; ProcessorNumber++) {
    CpuInitOrder = &CpuFeaturesData->InitOrder[ProcessorNumber];
    //
    // Calculate the last capability on all processors
    //
    SupportedMaskAnd (CpuFeaturesData->CapabilityPcds, CpuInitOrder->FeaturesSupportedMask);
  }
  //
  // Calculate the last setting
  //

  CpuFeaturesData->SettingPcds = AllocateCopyPool (CpuFeaturesData->BitMaskSize, CpuFeaturesData->CapabilityPcds);
  ASSERT (CpuFeaturesData->SettingPcds != NULL);
  SupportedMaskAnd (CpuFeaturesData->SettingPcds, CpuFeaturesData->ConfigurationPcds);

  //
  // Save PCDs and display CPU PCDs
  //
  SetCapabilityPcd (CpuFeaturesData->CapabilityPcds);
  SetSettingPcd (CpuFeaturesData->SettingPcds);

  //
  // Dump the last CPU feature list
  //
  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "Last CPU features list...\n"));
    Entry = GetFirstNode (&CpuFeaturesData->FeatureList);
    while (!IsNull (&CpuFeaturesData->FeatureList, Entry)) {
      CpuFeature = CPU_FEATURE_ENTRY_FROM_LINK (Entry);
      if (IsBitMaskMatch (CpuFeature->FeatureMask, CpuFeaturesData->CapabilityPcds)) {
        if (IsBitMaskMatch (CpuFeature->FeatureMask, CpuFeaturesData->SettingPcds)) {
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
    DEBUG ((DEBUG_INFO, "PcdCpuFeaturesSupport:\n"));
    DumpCpuFeatureMask (CpuFeaturesData->SupportPcds);
    DEBUG ((DEBUG_INFO, "PcdCpuFeaturesUserConfiguration:\n"));
    DumpCpuFeatureMask (CpuFeaturesData->ConfigurationPcds);
    DEBUG ((DEBUG_INFO, "PcdCpuFeaturesCapability:\n"));
    DumpCpuFeatureMask (CpuFeaturesData->CapabilityPcds);
    DEBUG ((DEBUG_INFO, "PcdCpuFeaturesSetting:\n"));
    DumpCpuFeatureMask (CpuFeaturesData->SettingPcds);
  );

  for (ProcessorNumber = 0; ProcessorNumber < NumberOfCpus; ProcessorNumber++) {
    CpuInitOrder = &CpuFeaturesData->InitOrder[ProcessorNumber];
    Entry = GetFirstNode (&CpuFeaturesData->FeatureList);
    while (!IsNull (&CpuFeaturesData->FeatureList, Entry)) {
      //
      // Insert each feature into processor's order list
      //
      CpuFeature = CPU_FEATURE_ENTRY_FROM_LINK (Entry);
      if (IsBitMaskMatch (CpuFeature->FeatureMask, CpuFeaturesData->CapabilityPcds)) {
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
      if (IsBitMaskMatch (CpuFeatureInOrder->FeatureMask, CpuFeaturesData->SettingPcds)) {
        Status = CpuFeatureInOrder->InitializeFunc (ProcessorNumber, CpuInfo, CpuFeatureInOrder->ConfigData, TRUE);
        if (EFI_ERROR (Status)) {
          //
          // Clean the CpuFeatureInOrder->FeatureMask in setting PCD.
          //
          SupportedMaskCleanBit (CpuFeaturesData->SettingPcds, CpuFeatureInOrder->FeatureMask);
          if (CpuFeatureInOrder->FeatureName != NULL) {
            DEBUG ((DEBUG_WARN, "Warning :: Failed to enable Feature: Name = %a.\n", CpuFeatureInOrder->FeatureName));
          } else {
            DEBUG ((DEBUG_WARN, "Warning :: Failed to enable Feature: Mask = "));
            DumpCpuFeatureMask (CpuFeatureInOrder->FeatureMask);
          }
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
        }
      }
      Entry = Entry->ForwardLink;
    }

    //
    // Dump PcdCpuFeaturesSetting again because this value maybe updated
    // again during initialize the features.
    //
    DEBUG ((DEBUG_INFO, "Dump final value for PcdCpuFeaturesSetting:\n"));
    DumpCpuFeatureMask (CpuFeaturesData->SettingPcds);

    //
    // Dump the RegisterTable
    //
    DumpRegisterTableOnProcessor (ProcessorNumber);
  }
}

/**
  Initialize the CPU registers from a register table.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.

  @note This service could be called by BSP/APs.
**/
VOID
ProgramProcessorRegister (
  IN UINTN  ProcessorNumber
  )
{
  CPU_FEATURES_DATA         *CpuFeaturesData;
  CPU_REGISTER_TABLE        *RegisterTable;
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntry;
  UINTN                     Index;
  UINTN                     Value;
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntryHead;

  CpuFeaturesData = GetCpuFeaturesData ();
  RegisterTable = &CpuFeaturesData->RegisterTable[ProcessorNumber];

  //
  // Traverse Register Table of this logical processor
  //
  RegisterTableEntryHead = (CPU_REGISTER_TABLE_ENTRY *) (UINTN) RegisterTable->RegisterTableEntry;

  for (Index = 0; Index < RegisterTable->TableLength; Index++) {

    RegisterTableEntry = &RegisterTableEntryHead[Index];

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
      //
      // Get lock to avoid Package/Core scope MSRs programming issue in parallel execution mode
      //
      AcquireSpinLock (&CpuFeaturesData->MsrLock);
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
      ReleaseSpinLock (&CpuFeaturesData->MsrLock);
      break;
    //
    // MemoryMapped operations
    //
    case MemoryMapped:
      AcquireSpinLock (&CpuFeaturesData->MemoryMappedLock);
      MmioBitFieldWrite32 (
        (UINTN)(RegisterTableEntry->Index | LShiftU64 (RegisterTableEntry->HighIndex, 32)),
        RegisterTableEntry->ValidBitStart,
        RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
        (UINT32)RegisterTableEntry->Value
        );
      ReleaseSpinLock (&CpuFeaturesData->MemoryMappedLock);
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
  UINTN                  ProcessorNumber;

  ProcessorNumber = GetProcessorIndex ();
  ProgramProcessorRegister (ProcessorNumber);
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
  UINTN                  NumberOfCpus;
  UINTN                  NumberOfEnabledProcessors;

  GetNumberOfProcessor (&NumberOfCpus, &NumberOfEnabledProcessors);

  CpuInitDataInitialize (NumberOfCpus);

  //
  // Wakeup all APs for data collection.
  //
  StartupAPsWorker (CollectProcessorData);

  //
  // Collect data on BSP
  //
  CollectProcessorData (NULL);

  AnalysisProcessorFeatures (NumberOfCpus);
}

/**
  Performs CPU features Initialization.

  This service will invoke MP service to perform CPU features
  initialization on BSP/APs per user configuration.

  @note This service could be called by BSP only.
**/
VOID
EFIAPI
CpuFeaturesInitialize (
  VOID
  )
{
  CPU_FEATURES_DATA      *CpuFeaturesData;
  UINTN                  OldBspNumber;

  CpuFeaturesData = GetCpuFeaturesData ();

  OldBspNumber = GetProcessorIndex();
  CpuFeaturesData->BspNumber = OldBspNumber;
  //
  // Wakeup all APs for programming.
  //
  StartupAPsWorker (SetProcessorRegister);
  //
  // Programming BSP
  //
  SetProcessorRegister (NULL);
  //
  // Switch to new BSP if required
  //
  if (CpuFeaturesData->BspNumber != OldBspNumber) {
    SwitchNewBsp (CpuFeaturesData->BspNumber);
  }
}
