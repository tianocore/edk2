/** @file
Definitions for CPU S3 data.

Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  CacheControl
} REGISTER_TYPE;

//
// Element of register table entry
//
typedef struct {
  REGISTER_TYPE  RegisterType;
  UINT32         Index;
  UINT8          ValidBitStart;
  UINT8          ValidBitLength;
  UINT64         Value;
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
  // Buffer of CPU_REGISTER_TABLE_ENTRY structures.  This buffer must be
  // allocated below 4GB from memory of type EfiACPIMemoryNVS.
  //
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntry;
} CPU_REGISTER_TABLE;

//
// Data structure that is required for ACPI S3 resume.  This structure must be
// allocated below 4GB from memory of type EfiACPIMemoryNVS.  The PCD
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
  // Physical address of structure of type IA32_DESCRIPTOR.  This structure must
  // be allocated below 4GB from memory of type EfiACPIMemoryNVS.  The
  // IA32_DESCRIPTOR structure provides the base address and length of a GDT
  // The buffer for GDT must also be allocated below 4GB from memory of type
  // EfiACPIMemoryNVS.  The GDT must be filled in with the GDT contents that are
  // used during an ACPI S3 resume.  This is typically the contents of the GDT
  // used by the boot processor when the platform is booted.
  //
  EFI_PHYSICAL_ADDRESS  GdtrProfile;
  //
  // Physical address of structure of type IA32_DESCRIPTOR.  This structure must
  // be allocated below 4GB from memory of type EfiACPIMemoryNVS.  The
  // IA32_DESCRIPTOR structure provides the base address and length of an IDT.
  // The buffer for IDT must also be allocated below 4GB from memory of type
  // EfiACPIMemoryNVS.  The IDT must be filled in with the IDT contents that are
  // used during an ACPI S3 resume.  This is typically the contents of the IDT
  // used by the boot processor when the platform is booted.
  //
  EFI_PHYSICAL_ADDRESS  IdtrProfile;
  //
  // Physical address of a buffer that is used as stacks during ACPI S3 resume.
  // The total size of this buffer, in bytes, is NumberOfCpus * StackSize.  This
  // structure must be allocated below 4GB from memory of type EfiACPIMemoryNVS.
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
  // used during an ACPI S3 resume.  This structure must be allocated below 4GB
  // from memory of type EfiACPIMemoryNVS.
  //
  EFI_PHYSICAL_ADDRESS  MtrrTable;
  //
  // Physical address of an array of CPU_REGISTER_TABLE structures, with
  // NumberOfCpus entries.  This array must be allocated below 4GB from memory
  // of type EfiACPIMemoryNVS.  If a register table is not required, then the
  // TableLength and AllocatedSize fields of CPU_REGISTER_TABLE are set to 0.
  // If TableLength is > 0, then elements of RegisterTableEntry are used to
  // initialize the CPU that matches InitialApicId, during an ACPI S3 resume,
  // before SMBASE relocation is performed.
  //
  EFI_PHYSICAL_ADDRESS  PreSmmInitRegisterTable;
  //
  // Physical address of an array of CPU_REGISTER_TABLE structures, with
  // NumberOfCpus entries.  This array must be allocated below 4GB from memory
  // of type EfiACPIMemoryNVS.  If a register table is not required, then the
  // TableLength and AllocatedSize fields of CPU_REGISTER_TABLE are set to 0.
  // If TableLength is > 0, then elements of RegisterTableEntry are used to
  // initialize the CPU that matches InitialApicId, during an ACPI S3 resume,
  // after SMBASE relocation is performed.
  //
  EFI_PHYSICAL_ADDRESS  RegisterTable;
  //
  // Physical address of a buffer that contains the machine check handler that
  // is used during an ACPI S3 Resume.  This buffer must be allocated below 4GB
  // from memory of type EfiACPIMemoryNVS.  In order for this machine check
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
} ACPI_CPU_DATA;

#endif
