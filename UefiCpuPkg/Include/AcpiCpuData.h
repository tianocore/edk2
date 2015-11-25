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
typedef enum _REGISTER_TYPE {
  Msr,
  ControlRegister,
  MemoryMapped,
  CacheControl
} REGISTER_TYPE;

//
// Element of register table entry
//
typedef struct {
  REGISTER_TYPE RegisterType;
  UINT32        Index;
  UINT8         ValidBitStart;
  UINT8         ValidBitLength;
  UINT64        Value;
} CPU_REGISTER_TABLE_ENTRY;

//
// Register table definition, including current table length,
// allocated size of this table, and pointer to the list of table entries.
//
typedef struct {
  UINT32                   TableLength;
  UINT32                   NumberBeforeReset;
  UINT32                   AllocatedSize;
  UINT32                   InitialApicId;
  CPU_REGISTER_TABLE_ENTRY *RegisterTableEntry;
} CPU_REGISTER_TABLE;

typedef struct {
  EFI_PHYSICAL_ADDRESS  StartupVector;
  EFI_PHYSICAL_ADDRESS  GdtrProfile;
  EFI_PHYSICAL_ADDRESS  IdtrProfile;
  EFI_PHYSICAL_ADDRESS  StackAddress;
  UINT32                StackSize;
  UINT32                NumberOfCpus;
  EFI_PHYSICAL_ADDRESS  MtrrTable;
  //
  // Physical address of a CPU_REGISTER_TABLE structure
  //
  EFI_PHYSICAL_ADDRESS  PreSmmInitRegisterTable;
  //
  // Physical address of a CPU_REGISTER_TABLE structure
  //
  EFI_PHYSICAL_ADDRESS  RegisterTable;
  EFI_PHYSICAL_ADDRESS  ApMachineCheckHandlerBase;
  UINT32                ApMachineCheckHandlerSize;
} ACPI_CPU_DATA;

#endif
