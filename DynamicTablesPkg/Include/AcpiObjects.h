/** @file

  Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>
  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_CPC_INFO_H_
#define AML_CPC_INFO_H_

#include <IndustryStandard/Acpi.h>

#pragma pack(1)

/** A structure that describes the Cpc information.

  Continuous Performance Control is described in DSDT/SSDT and associated
  to cpus/clusters in the cpu topology.

  Unsupported Optional registers should be encoded with NULL resource
  Register {(SystemMemory, 0, 0, 0, 0)}

  For values that support Integer or Buffer, integer will be used
  if buffer is NULL resource.
  If resource is not NULL then Integer must be 0

  Cf. ACPI 6.4, s8.4.7.1 _CPC (Continuous Performance Control)

**/

typedef struct AmlCpcInfo {
  /// The revision number of the _CPC package format.
  UINT32                                    Revision;

  /// Indicates the highest level of performance the processor
  /// is theoretically capable of achieving.
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    HighestPerformanceBuffer;
  UINT32                                    HighestPerformanceInteger;

  /// Indicates the highest sustained performance level of the processor.
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    NominalPerformanceBuffer;
  UINT32                                    NominalPerformanceInteger;

  /// Indicates the lowest performance level of the processor with non-linear power savings.
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    LowestNonlinearPerformanceBuffer;
  UINT32                                    LowestNonlinearPerformanceInteger;

  /// Indicates the lowest performance level of the processor..
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    LowestPerformanceBuffer;
  UINT32                                    LowestPerformanceInteger;

  /// Guaranteed Performance Register Buffer.
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    GuaranteedPerformanceRegister;

  /// Desired Performance Register Buffer.
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    DesiredPerformanceRegister;

  /// Minimum Performance Register Buffer.
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    MinimumPerformanceRegister;

  /// Maximum Performance Register Buffer.
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    MaximumPerformanceRegister;

  /// Performance Reduction Tolerance Register.
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    PerformanceReductionToleranceRegister;

  /// Time Window Register.
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    TimeWindowRegister;

  /// Counter Wraparound Time
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    CounterWraparoundTimeBuffer;
  UINT32                                    CounterWraparoundTimeInteger;

  /// Reference Performance Counter Register
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    ReferencePerformanceCounterRegister;

  /// Delivered Performance Counter Register
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    DeliveredPerformanceCounterRegister;

  /// Performance Limited Register
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    PerformanceLimitedRegister;

  /// CPPC EnableRegister
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    CPPCEnableRegister;

  /// Autonomous Selection Enable
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    AutonomousSelectionEnableBuffer;
  UINT32                                    AutonomousSelectionEnableInteger;

  /// AutonomousActivity-WindowRegister
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    AutonomousActivityWindowRegister;

  /// EnergyPerformance-PreferenceRegister
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    EnergyPerformancePreferenceRegister;

  /// Reference Performance
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    ReferencePerformanceBuffer;
  UINT32                                    ReferencePerformanceInteger;

  /// Lowest Frequency
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    LowestFrequencyBuffer;
  UINT32                                    LowestFrequencyInteger;

  /// Nominal Frequency
  /// Optional
  EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE    NominalFrequencyBuffer;
  UINT32                                    NominalFrequencyInteger;
} AML_CPC_INFO;

/** A structure that describes a
    P-State Dependency (PSD) Info.

  Cf. ACPI 6.5, s8.4.5.5 _PSD (P-State Dependency).
*/
typedef struct AmlPsdInfo {
  /// Revision.
  UINT8     Revision;

  /// Domain Id.
  UINT32    Domain;

  /// Coordination type.
  UINT32    CoordType;

  /// Number of processors belonging to the Domain.
  UINT32    NumProc;
} AML_PSD_INFO;

#pragma pack()

#endif //AML_CPC_INFO_H_
