/** @file
  The generic memory test driver definition

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _NULL_MEMORY_TEST_H
#define _NULL_MEMORY_TEST_H


#include <PiDxe.h>


#include <Protocol/GenericMemoryTest.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

//
// attributes for reserved memory before it is promoted to system memory
//
#define EFI_MEMORY_PRESENT      0x0100000000000000ULL
#define EFI_MEMORY_INITIALIZED  0x0200000000000000ULL
#define EFI_MEMORY_TESTED       0x0400000000000000ULL


//
// Some global define
//
#define GENERIC_CACHELINE_SIZE  0x40

//
// The SPARSE_SPAN_SIZE size can not small then the MonoTestSize
//
#define TEST_BLOCK_SIZE   0x2000000
#define QUICK_SPAN_SIZE   (TEST_BLOCK_SIZE >> 2)
#define SPARSE_SPAN_SIZE  (TEST_BLOCK_SIZE >> 4)

//
// This structure records every nontested memory range parsed through GCD
// service.
//
#define EFI_NONTESTED_MEMORY_RANGE_SIGNATURE  EFI_SIGNATURE_32 ('N', 'T', 'M', 'E')
typedef struct {
  UINTN                 Signature;
  LIST_ENTRY            Link;
  EFI_PHYSICAL_ADDRESS  StartAddress;
  UINT64                Length;
  UINT64                Capabilities;
  BOOLEAN               Above4G;
  BOOLEAN               AlreadyMapped;
} NONTESTED_MEMORY_RANGE;

#define NONTESTED_MEMORY_RANGE_FROM_LINK(link) \
        CR(link, NONTESTED_MEMORY_RANGE, Link, EFI_NONTESTED_MEMORY_RANGE_SIGNATURE)

//
// This is the memory test driver's structure definition
//
#define EFI_GENERIC_MEMORY_TEST_PRIVATE_SIGNATURE EFI_SIGNATURE_32 ('G', 'E', 'M', 'T')

//
// Function Prototypes
//
EFI_STATUS
EFIAPI
InitializeMemoryTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN  EXTENDMEM_COVERAGE_LEVEL                 Level,
  OUT BOOLEAN                                  *RequireSoftECCInit
  );

EFI_STATUS
EFIAPI
GenPerformMemoryTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN OUT UINT64                                *TestedMemorySize,
  OUT UINT64                                   *TotalMemorySize,
  OUT BOOLEAN                                  *ErrorOut,
  IN BOOLEAN                                   TestAbort
  );

EFI_STATUS
EFIAPI
GenMemoryTestFinished (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This
  );

EFI_STATUS
EFIAPI
GenCompatibleRangeTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN  EFI_PHYSICAL_ADDRESS                     StartAddress,
  IN  UINT64                                   Length
  );

#endif
