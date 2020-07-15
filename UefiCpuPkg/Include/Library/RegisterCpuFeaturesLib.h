/** @file
  Register CPU Features Library to register and manage CPU features.

  Copyright (c) 2017 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __REGISTER_CPU_FEATURES_LIB_H__
#define __REGISTER_CPU_FEATURES_LIB_H__

#include <AcpiCpuData.h>
#include <Register/Intel/Cpuid.h>
#include <Protocol/MpService.h>

///
/// Defines used to identify a CPU feature.  The lower 16-bits are used to
/// identify a unique CPU feature and the value represents a bit number in
/// a bit mask.  The upper 16-bits are bit mask values that are used as
/// modifiers of a CPU feature.  When used in a list, the define value
/// CPU_FEATURE_END is used to terminate a list of CPU feature values.
/// @{
#define CPU_FEATURE_AESNI                           0
#define CPU_FEATURE_TURBO_MODE                      1
#define CPU_FEATURE_MWAIT                           2
#define CPU_FEATURE_ACPI                            3
#define CPU_FEATURE_EIST                            4
#define CPU_FEATURE_RESERVED_5                      5
#define CPU_FEATURE_FASTSTRINGS                     6
#define CPU_FEATURE_VMX                             7
#define CPU_FEATURE_SMX                             8
#define CPU_FEATURE_LMCE                            9
#define CPU_FEATURE_LOCK_FEATURE_CONTROL_REGISTER   10
#define CPU_FEATURE_LIMIT_CPUID_MAX_VAL             11
#define CPU_FEATURE_MCE                             12
#define CPU_FEATURE_MCA                             13
#define CPU_FEATURE_MCG_CTL                         14
#define CPU_FEATURE_PENDING_BREAK                   15
#define CPU_FEATURE_C1E                             16
#define CPU_FEATURE_C1_AUTO_DEMOTION                17
#define CPU_FEATURE_C3_AUTO_DEMOTION                18
#define CPU_FEATURE_C1_UNDEMOTION                   19
#define CPU_FEATURE_C3_UNDEMOTION                   20
#define CPU_FEATURE_C_STATE                         21
#define CPU_FEATURE_TM                              22
#define CPU_FEATURE_TM2                             23
#define CPU_FEATURE_X2APIC                          24
#define CPU_FEATURE_RESERVED_25                     25
#define CPU_FEATURE_RESERVED_26                     26
#define CPU_FEATURE_RESERVED_27                     27
#define CPU_FEATURE_RESERVED_28                     28
#define CPU_FEATURE_RESERVED_29                     29
#define CPU_FEATURE_RESERVED_30                     30
#define CPU_FEATURE_RESERVED_31                     31

#define CPU_FEATURE_L2_PREFETCHER                   (32+0)
#define CPU_FEATURE_L1_DATA_PREFETCHER              (32+1)
#define CPU_FEATURE_HARDWARE_PREFETCHER             (32+2)
#define CPU_FEATURE_ADJACENT_CACHE_LINE_PREFETCH    (32+3)
#define CPU_FEATURE_DCU_PREFETCHER                  (32+4)
#define CPU_FEATURE_IP_PREFETCHER                   (32+5)
#define CPU_FEATURE_MLC_STREAMER_PREFETCHER         (32+6)
#define CPU_FEATURE_MLC_SPATIAL_PREFETCHER          (32+7)
#define CPU_FEATURE_THREE_STRIKE_COUNTER            (32+8)
#define CPU_FEATURE_APIC_TPR_UPDATE_MESSAGE         (32+9)
#define CPU_FEATURE_ENERGY_PERFORMANCE_BIAS         (32+10)
#define CPU_FEATURE_PPIN                            (32+11)
#define CPU_FEATURE_PROC_TRACE                      (32+12)

#define CPU_FEATURE_BEFORE_ALL                      BIT23
#define CPU_FEATURE_AFTER_ALL                       BIT24
#define CPU_FEATURE_THREAD_BEFORE                   BIT25
#define CPU_FEATURE_THREAD_AFTER                    BIT26
#define CPU_FEATURE_CORE_BEFORE                     BIT27
#define CPU_FEATURE_CORE_AFTER                      BIT28
#define CPU_FEATURE_PACKAGE_BEFORE                  BIT29
#define CPU_FEATURE_PACKAGE_AFTER                   BIT30
#define CPU_FEATURE_END                             MAX_UINT32
/// @}

///
/// The bit field to indicate whether the processor is the first in its parent scope.
///
typedef struct {
  //
  // Set to 1 when current processor is the first thread in the core it resides in.
  //
  UINT32 Thread   : 1;
  //
  // Set to 1 when current processor is a thread of the first core in the module it resides in.
  //
  UINT32 Core     : 1;
  //
  // Set to 1 when current processor is a thread of the first module in the tile it resides in.
  //
  UINT32 Module   : 1;
  //
  // Set to 1 when current processor is a thread of the first tile in the die it resides in.
  //
  UINT32 Tile     : 1;
  //
  // Set to 1 when current processor is a thread of the first die in the package it resides in.
  //
  UINT32 Die      : 1;
  //
  // Set to 1 when current processor is a thread of the first package in the system.
  //
  UINT32 Package  : 1;
  UINT32 Reserved : 26;
} REGISTER_CPU_FEATURE_FIRST_PROCESSOR;

///
/// CPU Information passed into the SupportFunc and InitializeFunc of the
/// RegisterCpuFeature() library function.  This structure contains information
/// that is commonly used during CPU feature detection and initialization.
///
typedef struct {
  ///
  /// The package that the CPU resides
  ///
  EFI_PROCESSOR_INFORMATION            ProcessorInfo;

  ///
  /// The bit flag indicating whether the CPU is the first Thread/Core/Module/Tile/Die/Package in its parent scope.
  ///
  REGISTER_CPU_FEATURE_FIRST_PROCESSOR First;
  ///
  /// The Display Family of the CPU computed from CPUID leaf CPUID_VERSION_INFO
  ///
  UINT32                               DisplayFamily;
  ///
  /// The Display Model of the CPU computed from CPUID leaf CPUID_VERSION_INFO
  ///
  UINT32                               DisplayModel;
  ///
  /// The Stepping ID of the CPU computed from CPUID leaf CPUID_VERSION_INFO
  ///
  UINT32                               SteppingId;
  ///
  /// The Processor Type of the CPU computed from CPUID leaf CPUID_VERSION_INFO
  ///
  UINT32                               ProcessorType;
  ///
  /// Bit field structured returned in ECX from CPUID leaf CPUID_VERSION_INFO
  ///
  CPUID_VERSION_INFO_ECX               CpuIdVersionInfoEcx;
  ///
  /// Bit field structured returned in EDX from CPUID leaf CPUID_VERSION_INFO
  ///
  CPUID_VERSION_INFO_EDX               CpuIdVersionInfoEdx;
} REGISTER_CPU_FEATURE_INFORMATION;

/**
  Determines if a CPU feature is enabled in PcdCpuFeaturesSupport bit mask.
  If a CPU feature is disabled in PcdCpuFeaturesSupport then all the code/data
  associated with that feature should be optimized away if compiler
  optimizations are enabled.

  @param[in]  Feature  The bit number of the CPU feature to check in the PCD
                       PcdCpuFeaturesSupport.

  @retval  TRUE   The CPU feature is set in PcdCpuFeaturesSupport.
  @retval  FALSE  The CPU feature is not set in PcdCpuFeaturesSupport.

  @note This service could be called by BSP only.
**/
BOOLEAN
EFIAPI
IsCpuFeatureSupported (
  IN UINT32              Feature
  );

/**
  Determines if a CPU feature is set in PcdCpuFeaturesSetting bit mask.

  @param[in]  Feature  The bit number of the CPU feature to check in the PCD
                       PcdCpuFeaturesSetting.

  @retval  TRUE   The CPU feature is set in PcdCpuFeaturesSetting.
  @retval  FALSE  The CPU feature is not set in PcdCpuFeaturesSetting.

  @note This service could be called by BSP only.
**/
BOOLEAN
EFIAPI
IsCpuFeatureInSetting (
  IN UINT32              Feature
  );

/**
  Prepares for the data used by CPU feature detection and initialization.

  @param[in]  NumberOfProcessors  The number of CPUs in the platform.

  @return  Pointer to a buffer of CPU related configuration data.

  @note This service could be called by BSP only.
**/
typedef
VOID *
(EFIAPI *CPU_FEATURE_GET_CONFIG_DATA)(
  IN UINTN               NumberOfProcessors
  );

/**
  Detects if CPU feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     CPU feature is supported.
  @retval FALSE    CPU feature is not supported.

  @note This service could be called by BSP/APs.
**/
typedef
BOOLEAN
(EFIAPI *CPU_FEATURE_SUPPORT)(
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  );

/**
  Initializes CPU feature to specific state.

  This service does not initialize hardware and only produces entries in the
  Register Table for specified processor. Hardware initialization on BSP/APs
  will be done in CpuFeaturesInitialize().

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the CPU feature must be enabled.
                               If FALSE, then the CPU feature must be disabled.

  @retval RETURN_SUCCESS       CPU feature is initialized.

  @note This service could be called by BSP only.
**/
typedef
RETURN_STATUS
(EFIAPI *CPU_FEATURE_INITIALIZE)(
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  );

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
  );

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
  );

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
  );

/**
  Switches to assigned BSP after CPU features initialization.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.

  @note This service could be called by BSP only.
**/
VOID
EFIAPI
SwitchBspAfterFeaturesInitialize (
  IN UINTN               ProcessorNumber
  );

/**
  Adds an entry in specified register table.

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
CpuRegisterTableWrite (
  IN UINTN               ProcessorNumber,
  IN REGISTER_TYPE       RegisterType,
  IN UINT64              Index,
  IN UINT64              ValueMask,
  IN UINT64              Value
  );

/**
  Adds an entry in specified register table.

  This function adds an entry in specified register table, with given register type,
  register index, bit section and value.

  Driver will  test the current value before setting new value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry
  @param[in]  RegisterType     Type of the register to program
  @param[in]  Index            Index of the register to program
  @param[in]  ValueMask        Mask of bits in register to write
  @param[in]  Value            Value to write

  @note This service could be called by BSP only.
**/
VOID
EFIAPI
CpuRegisterTableTestThenWrite (
  IN UINTN               ProcessorNumber,
  IN REGISTER_TYPE       RegisterType,
  IN UINT64              Index,
  IN UINT64              ValueMask,
  IN UINT64              Value
  );

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
  );

/**
  Adds a 32-bit register write entry in specified register table.

  This macro adds an entry in specified register table, with given register type,
  register index, and value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry.
  @param[in]  RegisterType     Type of the register to program
  @param[in]  Index            Index of the register to program
  @param[in]  Value            Value to write

  @note This service could be called by BSP only.
**/
#define CPU_REGISTER_TABLE_WRITE32(ProcessorNumber, RegisterType, Index, Value)       \
  do {                                                                                \
    CpuRegisterTableWrite (ProcessorNumber, RegisterType, Index, MAX_UINT32, Value);  \
  } while(FALSE);

/**
  Adds a 32-bit register write entry in specified register table.

  This macro adds an entry in specified register table, with given register type,
  register index, and value.

  Driver will  test the current value before setting new value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry.
  @param[in]  RegisterType     Type of the register to program
  @param[in]  Index            Index of the register to program
  @param[in]  Value            Value to write

  @note This service could be called by BSP only.
**/
#define CPU_REGISTER_TABLE_TEST_THEN_WRITE32(ProcessorNumber, RegisterType, Index, Value)     \
  do {                                                                                        \
    CpuRegisterTableTestThenWrite (ProcessorNumber, RegisterType, Index, MAX_UINT32, Value);  \
  } while(FALSE);

/**
  Adds a 64-bit register write entry in specified register table.

  This macro adds an entry in specified register table, with given register type,
  register index, and value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry.
  @param[in]  RegisterType     Type of the register to program
  @param[in]  Index            Index of the register to program
  @param[in]  Value            Value to write

  @note This service could be called by BSP only.
**/
#define CPU_REGISTER_TABLE_WRITE64(ProcessorNumber, RegisterType, Index, Value)       \
  do {                                                                                \
    CpuRegisterTableWrite (ProcessorNumber, RegisterType, Index, MAX_UINT64, Value);  \
  } while(FALSE);

/**
  Adds a 64-bit register write entry in specified register table.

  This macro adds an entry in specified register table, with given register type,
  register index, and value.

  Driver will  test the current value before setting new value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry.
  @param[in]  RegisterType     Type of the register to program
  @param[in]  Index            Index of the register to program
  @param[in]  Value            Value to write

  @note This service could be called by BSP only.
**/
#define CPU_REGISTER_TABLE_TEST_THEN_WRITE64(ProcessorNumber, RegisterType, Index, Value)     \
  do {                                                                                        \
    CpuRegisterTableTestThenWrite (ProcessorNumber, RegisterType, Index, MAX_UINT64, Value);  \
  } while(FALSE);

/**
  Adds a bit field write entry in specified register table.

  This macro adds an entry in specified register table, with given register type,
  register index, bit field section, and value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry.
  @param[in]  RegisterType     Type of the register to program.
  @param[in]  Index            Index of the register to program.
  @param[in]  Type             The data type name of a register structure.
  @param[in]  Field            The bit fiel name in register structure to write.
  @param[in]  Value            Value to write to the bit field.

  @note This service could be called by BSP only.
**/
#define CPU_REGISTER_TABLE_WRITE_FIELD(ProcessorNumber, RegisterType, Index, Type, Field, Value) \
  do {                                                                                           \
    UINT64  ValueMask;                                                                           \
    ValueMask = MAX_UINT64;                                                                      \
    ((Type *)(&ValueMask))->Field = 0;                                                           \
    CpuRegisterTableWrite (ProcessorNumber, RegisterType, Index, ~ValueMask, Value);             \
  } while(FALSE);

/**
  Adds a bit field write entry in specified register table.

  This macro adds an entry in specified register table, with given register type,
  register index, bit field section, and value.

  Driver will  test the current value before setting new value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry.
  @param[in]  RegisterType     Type of the register to program.
  @param[in]  Index            Index of the register to program.
  @param[in]  Type             The data type name of a register structure.
  @param[in]  Field            The bit fiel name in register structure to write.
  @param[in]  Value            Value to write to the bit field.

  @note This service could be called by BSP only.
**/
#define CPU_REGISTER_TABLE_TEST_THEN_WRITE_FIELD(ProcessorNumber, RegisterType, Index, Type, Field, Value) \
  do {                                                                                                     \
    UINT64  ValueMask;                                                                                     \
    ValueMask = MAX_UINT64;                                                                                \
    ((Type *)(&ValueMask))->Field = 0;                                                                     \
    CpuRegisterTableTestThenWrite (ProcessorNumber, RegisterType, Index, ~ValueMask, Value);               \
  } while(FALSE);

/**
  Adds a 32-bit register write entry in specified register table.

  This macro adds an entry in specified register table, with given register type,
  register index, and value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry.
  @param[in]  RegisterType     Type of the register to program
  @param[in]  Index            Index of the register to program
  @param[in]  Value            Value to write

  @note This service could be called by BSP only.
**/
#define PRE_SMM_CPU_REGISTER_TABLE_WRITE32(ProcessorNumber, RegisterType, Index, Value)    \
  do {                                                                                     \
    PreSmmCpuRegisterTableWrite (ProcessorNumber, RegisterType, Index, MAX_UINT32, Value); \
  } while(FALSE);

/**
  Adds a 64-bit register write entry in specified register table.

  This macro adds an entry in specified register table, with given register type,
  register index, and value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry.
  @param[in]  RegisterType     Type of the register to program
  @param[in]  Index            Index of the register to program
  @param[in]  Value            Value to write

  @note This service could be called by BSP only.
**/
#define PRE_SMM_CPU_REGISTER_TABLE_WRITE64(ProcessorNumber, RegisterType, Index, Value)    \
  do {                                                                                     \
    PreSmmCpuRegisterTableWrite (ProcessorNumber, RegisterType, Index, MAX_UINT64, Value); \
  } while(FALSE);

/**
  Adds a bit field write entry in specified register table.

  This macro adds an entry in specified register table, with given register type,
  register index, bit field section, and value.

  @param[in]  ProcessorNumber  The index of the CPU to add a register table entry.
  @param[in]  RegisterType     Type of the register to program.
  @param[in]  Index            Index of the register to program.
  @param[in]  Type             The data type name of a register structure.
  @param[in]  Field            The bit fiel name in register structure to write.
  @param[in]  Value            Value to write to the bit field.

  @note This service could be called by BSP only.
**/
#define PRE_SMM_CPU_REGISTER_TABLE_WRITE_FIELD(ProcessorNumber, RegisterType, Index, Type, Field, Value) \
  do {                                                                                                   \
    UINT64  ValueMask;                                                                                   \
    ValueMask = MAX_UINT64;                                                                              \
    ((Type *)(&ValueMask))->Field = 0;                                                                   \
    PreSmmCpuRegisterTableWrite (ProcessorNumber, RegisterType, Index, ~ValueMask, Value);                \
  } while(FALSE);

#endif
