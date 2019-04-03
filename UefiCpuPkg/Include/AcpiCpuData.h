/** @file
Definitions for CPU S3 data.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ACPI_CPU_DATA_H_
#define _ACPI_CPU_DATA_H_

//
// Register types in register table
//
typedef enum {
  Msr,
  ControlRegister,
  MemoryMapped,
  CacheControl,

  //
  // Semaphore type used to control the execute sequence of the Msr.
  // It will be insert between two Msr which has execute dependence.
  //
  Semaphore,
  InvalidReg
} REGISTER_TYPE;

//
// Describe the dependency type for different features.
// The value set to CPU_REGISTER_TABLE_ENTRY.Value when the REGISTER_TYPE is Semaphore.
//
typedef enum {
  NoneDepType,
  ThreadDepType,
  CoreDepType,
  PackageDepType,
  InvalidDepType
} CPU_FEATURE_DEPENDENCE_TYPE;

//
// CPU information.
//
typedef struct {
  //
  // Record the package count in this CPU.
  //
  UINT32                      PackageCount;
  //
  // Record the max core count in this CPU.
  // Different packages may have different core count, this value
  // save the max core count in all the packages.
  //
  UINT32                      MaxCoreCount;
  //
  // Record the max thread count in this CPU.
  // Different cores may have different thread count, this value
  // save the max thread count in all the cores.
  //
  UINT32                      MaxThreadCount;
  //
  // This field points to an array.
  // This array saves valid core count (type UINT32) of each package.
  // The array has PackageCount elements.
  //
  // If the platform does not support MSR setting at S3 resume, and
  // therefore it doesn't need the dependency semaphores, it should set
  // this field to 0.
  //
  EFI_PHYSICAL_ADDRESS        ValidCoreCountPerPackage;
} CPU_STATUS_INFORMATION;

//
// Element of register table entry
//
typedef struct {
  REGISTER_TYPE  RegisterType;          // offset 0 - 3
  UINT32         Index;                 // offset 4 - 7
  UINT8          ValidBitStart;         // offset 8
  UINT8          ValidBitLength;        // offset 9
  UINT16         Reserved;              // offset 10 - 11
  UINT32         HighIndex;             // offset 12-15, only valid for MemoryMapped
  UINT64         Value;                 // offset 16-23
} CPU_REGISTER_TABLE_ENTRY;

//
// Register table definition, including current table length,
// allocated size of this table, and pointer to the list of table entries.
//
typedef struct {
  //
  // The number of valid entries in the RegisterTableEntry buffer
  //
  UINT32                    TableLength;
  UINT32                    NumberBeforeReset;
  //
  // The size, in bytes, of the RegisterTableEntry buffer
  //
  UINT32                    AllocatedSize;
  //
  // The initial APIC ID of the CPU this register table applies to
  //
  UINT32                    InitialApicId;
  //
  // Physical address of CPU_REGISTER_TABLE_ENTRY structures.
  //
  EFI_PHYSICAL_ADDRESS      RegisterTableEntry;
} CPU_REGISTER_TABLE;

//
// Data structure that is required for ACPI S3 resume. The PCD
// PcdCpuS3DataAddress must be set to the physical address where this structure
// is allocated
//
typedef struct {
  //
  // Physical address of 4KB buffer allocated below 1MB from memory of type
  // EfiReservedMemoryType.  The buffer is not required to be initialized, but
  // it is recommended that the buffer be zero-filled.  This buffer is used to
  // wake APs during an ACPI S3 resume.
  //
  EFI_PHYSICAL_ADDRESS  StartupVector;
  //
  // Physical address of structure of type IA32_DESCRIPTOR. The
  // IA32_DESCRIPTOR structure provides the base address and length of a GDT
  // The GDT must be filled in with the GDT contents that are
  // used during an ACPI S3 resume.  This is typically the contents of the GDT
  // used by the boot processor when the platform is booted.
  //
  EFI_PHYSICAL_ADDRESS  GdtrProfile;
  //
  // Physical address of structure of type IA32_DESCRIPTOR.  The
  // IA32_DESCRIPTOR structure provides the base address and length of an IDT.
  // The IDT must be filled in with the IDT contents that are
  // used during an ACPI S3 resume.  This is typically the contents of the IDT
  // used by the boot processor when the platform is booted.
  //
  EFI_PHYSICAL_ADDRESS  IdtrProfile;
  //
  // Physical address of a buffer that is used as stacks during ACPI S3 resume.
  // The total size of this buffer, in bytes, is NumberOfCpus * StackSize.  This
  // structure must be allocated from memory of type EfiACPIMemoryNVS.
  //
  EFI_PHYSICAL_ADDRESS  StackAddress;
  //
  // The size, in bytes, of the stack provided to each CPU during ACPI S3 resume.
  //
  UINT32                StackSize;
  //
  // The number of CPUs.  If a platform does not support hot plug CPUs, then
  // this is the number of CPUs detected when the platform is booted, regardless
  // of being enabled or disabled.  If a platform does support hot plug CPUs,
  // then this is the maximum number of CPUs that the platform supports.
  //
  UINT32                NumberOfCpus;
  //
  // Physical address of structure of type MTRR_SETTINGS that contains a copy
  // of the MTRR settings that are compatible with the MTRR settings used by
  // the boot processor when the platform was booted.  These MTRR settings are
  // used during an ACPI S3 resume.
  //
  EFI_PHYSICAL_ADDRESS  MtrrTable;
  //
  // Physical address of an array of CPU_REGISTER_TABLE structures, with
  // NumberOfCpus entries.  If a register table is not required, then the
  // TableLength and AllocatedSize fields of CPU_REGISTER_TABLE are set to 0.
  // If TableLength is > 0, then elements of RegisterTableEntry are used to
  // initialize the CPU that matches InitialApicId, during an ACPI S3 resume,
  // before SMBASE relocation is performed.
  //
  EFI_PHYSICAL_ADDRESS  PreSmmInitRegisterTable;
  //
  // Physical address of an array of CPU_REGISTER_TABLE structures, with
  // NumberOfCpus entries.  If a register table is not required, then the
  // TableLength and AllocatedSize fields of CPU_REGISTER_TABLE are set to 0.
  // If TableLength is > 0, then elements of RegisterTableEntry are used to
  // initialize the CPU that matches InitialApicId, during an ACPI S3 resume,
  // after SMBASE relocation is performed.
  //
  EFI_PHYSICAL_ADDRESS  RegisterTable;
  //
  // Physical address of a buffer that contains the machine check handler that
  // is used during an ACPI S3 Resume.  In order for this machine check
  // handler to be active on an AP during an ACPI S3 resume, the machine check
  // vector in the IDT provided by IdtrProfile must be initialized to transfer
  // control to this physical address.
  //
  EFI_PHYSICAL_ADDRESS  ApMachineCheckHandlerBase;
  //
  // The size, in bytes, of the machine check handler that is used during an
  // ACPI S3 Resume.  If this field is 0, then a machine check handler is not
  // provided.
  //
  UINT32                ApMachineCheckHandlerSize;
  //
  // CPU information which is required when set the register table.
  //
  CPU_STATUS_INFORMATION     CpuStatus;
  //
  // Location info for each AP.
  // It points to an array which saves all APs location info.
  // The array count is the AP count in this CPU.
  //
  // If the platform does not support MSR setting at S3 resume, and
  // therefore it doesn't need the dependency semaphores, it should set
  // this field to 0.
  //
  EFI_PHYSICAL_ADDRESS  ApLocation;
} ACPI_CPU_DATA;

#endif
