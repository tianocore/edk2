/** @file

  Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>
  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>
  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

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

/** A structure that describes a
    processor power state control package Info.

  Cf. ACPI 6.5, s8.4.1.1 _CST (Processor Power State Control).
  Package {
    Register  // Buffer (Resource Descriptor)
    Type      // Integer (BYTE)
    Latency   // Integer (WORD)
    Power     // Integer (DWORD)
  }
*/
typedef struct AmlCstInfo {
  /// Information about the C-State register.
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    Register;

  /// Type of C-State, such as c1, c2, c3, etc.
  UINT8                                     Type;

  /// Worst case latency for entering and exiting the C-State, in milliseconds.
  UINT16                                    Latency;

  /// C-State power consumption in mW.
  UINT32                                    Power;
} AML_CST_INFO;

/** A structure that describes a
    C-State Dependency (_CSD) Info.

  Cf. ACPI 6.5, s8.4.1.2 _CSD (C-State Dependency).
  Package {
    NumEntries    // Integer, updated based on Revision
    Revision      // Integer (BYTE)
    Domain        // Integer (DWORD)
    CoordType     // Integer (DWORD)
    NumProcessors // Integer (DWORD)
    Index         // Integer (DWORD)
  }
*/
typedef struct AmlCsdInfo {
  /// The revision of the C-State dependency table.
  UINT8     Revision;

  /// The domain ID.
  UINT32    Domain;

  /// The coordination type.
  UINT32    CoordType;

  /// The number of processors in the domain.
  UINT32    NumProcessors;

  /// The index of the C-State entry in _CST.
  UINT32    Index;
} AML_CSD_INFO;

/** A structure that describes a
    Processor Performance control (_PCT) Info.

  Cf. ACPI 6.5, s8.4.5.1 _PCT (Processor Control).

  Package
  {
    ControlRegister   // Buffer (Resource Descriptor (Register))
    StatusRegister    // Buffer (Resource Descriptor (Register))
  }
*/
typedef struct AmlPctInfo {
  /// The performance control register for the processor.
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    ControlRegister;

  /// The performance status register for the processor.
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    StatusRegister;
} AML_PCT_INFO;

/** A structure that describes a
    Processor Supported Performance States (_PSS) Info.

  Cf. ACPI 6.5, s8.4.5.2 _PSS (Processor Supported Performance States).
  Package {
    CoreFrequency     // Integer (DWORD)
    Power             // Integer (DWORD)
    Latency           // Integer (DWORD)
    BusMasterLatency  // Integer (DWORD)
    Control           // Integer (DWORD)
    Status            // Integer (DWORD)
  }
*/
typedef struct AmlPssInfo {
  /// Processor core frequency in MHz.
  UINT32    CoreFrequency;

  /// Processor power consumption in mW.
  UINT32    Power;

  /// Processor latency in microseconds.
  UINT32    Latency;

  /// Processor bus master latency in microseconds.
  UINT32    BusMasterLatency;

  /// Value to write to the performance control register (_PCT).
  UINT32    Control;

  /// Value to compare with the read value performance status register (_PCT).
  UINT32    Status;
} AML_PSS_INFO;

#pragma pack()

#endif //AML_CPC_INFO_H_
