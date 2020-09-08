/** @file
  CPU Register Table Library definitions.

  Copyright (c) 2017 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _REGISTER_CPU_FEATURES_H_
#define _REGISTER_CPU_FEATURES_H_
#include <PiPei.h>
#include <PiDxe.h>
#include <Ppi/MpServices2.h>
#include <Protocol/MpService.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/RegisterCpuFeaturesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/IoLib.h>
#include <Library/LocalApicLib.h>

#include <AcpiCpuData.h>

#define CPU_FEATURE_ENTRY_SIGNATURE  SIGNATURE_32 ('C', 'F', 'E', 'S')

#define CPU_FEATURE_NAME_SIZE        128

typedef struct {
  REGISTER_CPU_FEATURE_INFORMATION     CpuInfo;
  UINT8                                *FeaturesSupportedMask;
  LIST_ENTRY                           OrderList;
} CPU_FEATURES_INIT_ORDER;

typedef struct {
  UINT32                       Signature;
  LIST_ENTRY                   Link;
  UINT8                        *FeatureMask;
  CHAR8                        *FeatureName;
  CPU_FEATURE_GET_CONFIG_DATA  GetConfigDataFunc;
  CPU_FEATURE_SUPPORT          SupportFunc;
  CPU_FEATURE_INITIALIZE       InitializeFunc;
  UINT8                        *ThreadBeforeFeatureBitMask;
  UINT8                        *ThreadAfterFeatureBitMask;
  UINT8                        *CoreBeforeFeatureBitMask;
  UINT8                        *CoreAfterFeatureBitMask;
  UINT8                        *PackageBeforeFeatureBitMask;
  UINT8                        *PackageAfterFeatureBitMask;
  VOID                         *ConfigData;
  BOOLEAN                      BeforeAll;
  BOOLEAN                      AfterAll;
} CPU_FEATURES_ENTRY;

//
// Flags used when program the register.
//
typedef struct {
  volatile UINTN           MemoryMappedLock;        // Spinlock used to program mmio
  volatile UINT32          *CoreSemaphoreCount;     // Semaphore containers used to program Core semaphore.
  volatile UINT32          *PackageSemaphoreCount;  // Semaphore containers used to program Package semaphore.
} PROGRAM_CPU_REGISTER_FLAGS;

typedef union {
  EFI_MP_SERVICES_PROTOCOL   *Protocol;
  EDKII_PEI_MP_SERVICES2_PPI *Ppi;
} MP_SERVICES;

typedef struct {
  UINTN                    FeaturesCount;
  UINT32                   BitMaskSize;
  LIST_ENTRY               FeatureList;

  CPU_FEATURES_INIT_ORDER  *InitOrder;
  UINT8                    *CapabilityPcd;
  UINT8                    *SettingPcd;

  UINT32                   NumberOfCpus;
  ACPI_CPU_DATA            *AcpiCpuData;

  CPU_REGISTER_TABLE       *RegisterTable;
  CPU_REGISTER_TABLE       *PreSmmRegisterTable;
  UINTN                    BspNumber;

  PROGRAM_CPU_REGISTER_FLAGS  CpuFlags;

  MP_SERVICES              MpService;
} CPU_FEATURES_DATA;

#define CPU_FEATURE_ENTRY_FROM_LINK(a) \
  CR ( \
  (a), \
  CPU_FEATURES_ENTRY, \
  Link, \
  CPU_FEATURE_ENTRY_SIGNATURE \
  )

/**
  Worker function to get CPU_FEATURES_DATA pointer.

  @return Pointer to CPU_FEATURES_DATA.
**/
CPU_FEATURES_DATA *
GetCpuFeaturesData (
  VOID
  );

/**
  Worker function to return processor index.

  @param  CpuFeaturesData    Cpu Feature Data structure.

  @return  The processor index.
**/
UINTN
GetProcessorIndex (
  IN CPU_FEATURES_DATA        *CpuFeaturesData
  );

/**
  Gets detailed MP-related information on the requested processor at the
  instant this call is made.

  @param[in]  ProcessorNumber       The handle number of processor.
  @param[out] ProcessorInfoBuffer   A pointer to the buffer where information for
                                    the requested processor is deposited.

  @return Status of MpServices->GetProcessorInfo().
**/
EFI_STATUS
GetProcessorInformation (
  IN  UINTN                            ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION        *ProcessorInfoBuffer
  );

/**
  Worker function to execute a caller provided function on all enabled APs.

  @param[in]  Procedure               A pointer to the function to be run on
                                      enabled APs of the system.
  @param[in]  MpEvent                 A pointer to the event to be used later
                                      to check whether procedure has done.
**/
VOID
StartupAllAPsWorker (
  IN  EFI_AP_PROCEDURE                 Procedure,
  IN  EFI_EVENT                        MpEvent
  );

/**
  Worker function to retrieve the number of logical processor in the platform.

  @param[out] NumberOfCpus                Pointer to the total number of logical
                                          processors in the system, including the BSP
                                          and disabled APs.
  @param[out] NumberOfEnabledProcessors   Pointer to the number of enabled logical
                                          processors that exist in system, including
                                          the BSP.
**/
VOID
GetNumberOfProcessor (
  OUT UINTN                            *NumberOfCpus,
  OUT UINTN                            *NumberOfEnabledProcessors
  );

/**
  Worker function to switch the requested AP to be the BSP from that point onward.

  @param[in] ProcessorNumber   The handle number of AP that is to become the new BSP.
**/
VOID
SwitchNewBsp (
  IN  UINTN                            ProcessorNumber
  );

/**
  Function that uses DEBUG() macros to display the contents of a a CPU feature bit mask.

  @param[in]  FeatureMask  A pointer to the CPU feature bit mask.
  @param[in]  BitMaskSize  CPU feature bits mask buffer size.

**/
VOID
DumpCpuFeatureMask (
  IN UINT8               *FeatureMask,
  IN UINT32              BitMaskSize
  );

/**
  Dump CPU feature name or CPU feature bit mask.

  @param[in]  CpuFeature   Pointer to CPU_FEATURES_ENTRY
  @param[in]  BitMaskSize  CPU feature bits mask buffer size.

**/
VOID
DumpCpuFeature (
  IN CPU_FEATURES_ENTRY  *CpuFeature,
  IN UINT32              BitMaskSize
  );

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
  );

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
  );

/**
  Programs registers for the calling processor.

  @param[in,out] Buffer  The pointer to private data buffer.

**/
VOID
EFIAPI
SetProcessorRegister (
  IN OUT VOID            *Buffer
  );

/**
  Return ACPI_CPU_DATA data.

  @return  Pointer to ACPI_CPU_DATA data.
**/
ACPI_CPU_DATA *
GetAcpiCpuData (
  VOID
  );

/**
  Worker function to get MP service pointer.

  @return MP_SERVICES variable.
**/
MP_SERVICES
GetMpService (
  VOID
  );

#endif
