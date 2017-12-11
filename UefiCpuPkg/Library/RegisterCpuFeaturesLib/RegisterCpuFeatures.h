/** @file
  CPU Register Table Library definitions.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _REGISTER_CPU_FEATURES_H_
#define _REGISTER_CPU_FEATURES_H_

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/RegisterCpuFeaturesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/IoLib.h>

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
  UINT8                        *BeforeFeatureBitMask;
  UINT8                        *AfterFeatureBitMask;
  VOID                         *ConfigData;
  BOOLEAN                      BeforeAll;
  BOOLEAN                      AfterAll;
} CPU_FEATURES_ENTRY;

typedef struct {
  UINTN                    FeaturesCount;
  UINT32                   BitMaskSize;
  SPIN_LOCK                MsrLock;
  SPIN_LOCK                MemoryMappedLock;
  LIST_ENTRY               FeatureList;

  CPU_FEATURES_INIT_ORDER  *InitOrder;
  UINT8                    *SupportPcd;
  UINT8                    *CapabilityPcd;
  UINT8                    *ConfigurationPcd;
  UINT8                    *SettingPcd;

  CPU_REGISTER_TABLE       *RegisterTable;
  CPU_REGISTER_TABLE       *PreSmmRegisterTable;
  UINTN                    BspNumber;
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
  Enlarges CPU register table for each processor.

  @param[in, out]  RegisterTable   Pointer processor's CPU register table
**/
VOID
EnlargeRegisterTable (
  IN OUT CPU_REGISTER_TABLE            *RegisterTable
  );

/**
  Allocates ACPI NVS memory to save ACPI_CPU_DATA.

  @return  Pointer to allocated ACPI_CPU_DATA.
**/
ACPI_CPU_DATA *
AllocateAcpiCpuData (
  VOID
  );

/**
  Worker function to return processor index.

  @return  The processor index.
**/
UINTN
GetProcessorIndex (
  VOID
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
**/
VOID
StartupAPsWorker (
  IN  EFI_AP_PROCEDURE                 Procedure
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
**/
VOID
DumpCpuFeatureMask (
  IN UINT8               *FeatureMask
  );

/**
  Dump CPU feature name or CPU feature bit mask.

  @param[in]  CpuFeature   Pointer to CPU_FEATURES_ENTRY
**/
VOID
DumpCpuFeature (
  IN CPU_FEATURES_ENTRY  *CpuFeature
  );

#endif
