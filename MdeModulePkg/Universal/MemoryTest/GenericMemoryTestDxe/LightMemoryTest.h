/** @file
  The generic memory test driver definition

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _GENERIC_MEMORY_TEST_H_
#define _GENERIC_MEMORY_TEST_H_

#include <Guid/StatusCodeDataTypeId.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/Cpu.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

//
// Some global define
//
#define GENERIC_CACHELINE_SIZE  0x40

//
// attributes for reserved memory before it is promoted to system memory
//
#define EFI_MEMORY_PRESENT      0x0100000000000000ULL
#define EFI_MEMORY_INITIALIZED  0x0200000000000000ULL
#define EFI_MEMORY_TESTED       0x0400000000000000ULL

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
#define EFI_NONTESTED_MEMORY_RANGE_SIGNATURE  SIGNATURE_32 ('N', 'T', 'M', 'E')

typedef struct {
  UINTN                 Signature;
  LIST_ENTRY        Link;
  EFI_PHYSICAL_ADDRESS  StartAddress;
  UINT64                Length;
  UINT64                Capabilities;
  BOOLEAN               Above4G;
  BOOLEAN               AlreadyMapped;
} NONTESTED_MEMORY_RANGE;

#define NONTESTED_MEMORY_RANGE_FROM_LINK(link) \
  CR ( \
  link, \
  NONTESTED_MEMORY_RANGE, \
  Link, \
  EFI_NONTESTED_MEMORY_RANGE_SIGNATURE \
  )

//
// This is the memory test driver's structure definition
//
#define EFI_GENERIC_MEMORY_TEST_PRIVATE_SIGNATURE SIGNATURE_32 ('G', 'E', 'M', 'T')

typedef struct {

  UINTN                             Signature;
  EFI_HANDLE                        Handle;

  //
  // Cpu arch protocol's pointer
  //
  EFI_CPU_ARCH_PROTOCOL             *Cpu;

  //
  // generic memory test driver's protocol
  //
  EFI_GENERIC_MEMORY_TEST_PROTOCOL  GenericMemoryTest;

  //
  // memory test covered spans
  //
  EXTENDMEM_COVERAGE_LEVEL          CoverLevel;
  UINTN                             CoverageSpan;
  UINT64                            BdsBlockSize;

  //
  // the memory test pattern and size every time R/W/V memory
  //
  VOID                              *MonoPattern;
  UINTN                             MonoTestSize;

  //
  // base memory's size which tested in PEI phase
  //
  UINT64                            BaseMemorySize;

  //
  // memory range list
  //
  LIST_ENTRY                    NonTestedMemRanList;

} GENERIC_MEMORY_TEST_PRIVATE;

#define GENERIC_MEMORY_TEST_PRIVATE_FROM_THIS(a) \
  CR ( \
  a, \
  GENERIC_MEMORY_TEST_PRIVATE, \
  GenericMemoryTest, \
  EFI_GENERIC_MEMORY_TEST_PRIVATE_SIGNATURE \
  )

//
// Function Prototypes
//

/**
  Construct the system base memory range through GCD service.

  @param[in] Private  Point to generic memory test driver's private data.

  @retval EFI_SUCCESS          Successful construct the base memory range through GCD service.
  @retval EFI_OUT_OF_RESOURCE  Could not allocate needed resource from base memory.
  @retval Others               Failed to construct base memory range through GCD service.

**/
EFI_STATUS
ConstructBaseMemoryRange (
  IN  GENERIC_MEMORY_TEST_PRIVATE  *Private
  );

/**
  Construct the system non-tested memory range through GCD service.

  @param[in] Private  Point to generic memory test driver's private data.

  @retval EFI_SUCCESS          Successful construct the non-tested memory range through GCD service.
  @retval EFI_OUT_OF_RESOURCE  Could not allocate needed resource from base memory.
  @retval Others               Failed to construct non-tested memory range through GCD service.

**/
EFI_STATUS
ConstructNonTestedMemoryRange (
  IN  GENERIC_MEMORY_TEST_PRIVATE  *Private
  );

/**
  Perform the address line walking ones test.

  @param[in] Private  Point to generic memory test driver's private data.

  @retval EFI_SUCCESS          Successful finished walking ones test.
  @retval EFI_OUT_OF_RESOURCE  Could not get resource in base memory.
  @retval EFI_ACCESS_DENIED    Code may can not run here because if walking one test
                               failed, system may be already halt.

**/
EFI_STATUS
PerformAddressDataLineTest (
  IN  GENERIC_MEMORY_TEST_PRIVATE      *Private
  );

/**
  Destroy the link list base on the correspond link list type.

  @param[in] Private  Point to generic memory test driver's private data.

**/
VOID
DestroyLinkList (
  IN  GENERIC_MEMORY_TEST_PRIVATE  *Private
  );

/**
  Add the extened memory to whole system memory map.

  @param[in] Private  Point to generic memory test driver's private data.

  @retval EFI_SUCCESS Successful add all the extended memory to system memory map.
  @retval Others      Failed to add the tested extended memory.

**/
EFI_STATUS
UpdateMemoryMap (
  IN  GENERIC_MEMORY_TEST_PRIVATE  *Private
  );

/**
  Write the memory test pattern into a range of physical memory.

  @param[in] Private  Point to generic memory test driver's private data.
  @param[in] Start    The memory range's start address.
  @param[in] Size     The memory range's size.

  @retval EFI_SUCCESS Successful write the test pattern into the non-tested memory.
  @retval Others      The test pattern may not really write into the physical memory.

**/
EFI_STATUS
WriteMemory (
  IN  GENERIC_MEMORY_TEST_PRIVATE  *Private,
  IN  EFI_PHYSICAL_ADDRESS         Start,
  IN  UINT64                       Size
  );

/**
  Verify the range of physical memory which covered by memory test pattern.

  This function will also do not return any informatin just cause system reset,
  because the handle error encount fatal error and disable the bad DIMMs.

  @param[in] Private  Point to generic memory test driver's private data.
  @param[in] Start    The memory range's start address.
  @param[in] Size     The memory range's size.

  @retval EFI_SUCCESS Successful verify the range of memory, no errors' location found.
  @retval Others      The range of memory have errors contained.

**/
EFI_STATUS
VerifyMemory (
  IN  GENERIC_MEMORY_TEST_PRIVATE  *Private,
  IN  EFI_PHYSICAL_ADDRESS         Start,
  IN  UINT64                       Size
  );

/**
  Test a range of the memory directly .

  @param[in] Private       Point to generic memory test driver's private data.
  @param[in] StartAddress  Starting address of the memory range to be tested.
  @param[in] Length        Length in bytes of the memory range to be tested.
  @param[in] Capabilities  The bit mask of attributes that the memory range supports.

  @retval EFI_SUCCESS      Successful test the range of memory.
  @retval Others           Failed to test the range of memory.

**/
EFI_STATUS
DirectRangeTest (
  IN  GENERIC_MEMORY_TEST_PRIVATE  *Private,
  IN  EFI_PHYSICAL_ADDRESS         StartAddress,
  IN  UINT64                       Length,
  IN  UINT64                       Capabilities
  );

/**
  Initialize the generic memory test.

  @param[in]  This                The protocol instance pointer.
  @param[in]  Level               The coverage level of the memory test.
  @param[out] RequireSoftECCInit  Indicate if the memory need software ECC init.

  @retval EFI_SUCCESS         The generic memory test is initialized correctly.
  @retval EFI_NO_MEDIA        The system had no memory to be tested.

**/
EFI_STATUS
EFIAPI
InitializeMemoryTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN  EXTENDMEM_COVERAGE_LEVEL                 Level,
  OUT BOOLEAN                                  *RequireSoftECCInit
  );

/**
  Perform the memory test.

  @param[in]  This              The protocol instance pointer.
  @param[out] TestedMemorySize  Return the tested extended memory size.
  @param[out] TotalMemorySize   Return the whole system physical memory size.
                                The total memory size does not include memory in a slot with a disabled DIMM.
  @param[out] ErrorOut          TRUE if the memory error occured.
  @param[in]  IfTestAbort       Indicates that the user pressed "ESC" to skip the memory test.

  @retval EFI_SUCCESS         One block of memory passed the test.
  @retval EFI_NOT_FOUND       All memory blocks have already been tested.
  @retval EFI_DEVICE_ERROR    Memory device error occured, and no agent can handle it.

**/
EFI_STATUS
EFIAPI
GenPerformMemoryTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  OUT UINT64                                   *TestedMemorySize,
  OUT UINT64                                   *TotalMemorySize,
  OUT BOOLEAN                                  *ErrorOut,
  IN BOOLEAN                                   TestAbort
  );

/**
  Finish the memory test.

  @param[in] This             The protocol instance pointer.

  @retval EFI_SUCCESS         Success. All resources used in the memory test are freed.

**/
EFI_STATUS
EFIAPI
GenMemoryTestFinished (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This
  );

/**
  Provides the capability to test the compatible range used by some special drivers.

  @param[in]  This              The protocol instance pointer.
  @param[in]  StartAddress      The start address of the compatible memory range that
                                must be below 16M.
  @param[in]  Length            The compatible memory range's length.

  @retval EFI_SUCCESS           The compatible memory range pass the memory test.
  @retval EFI_INVALID_PARAMETER The compatible memory range are not below Low 16M.

**/
EFI_STATUS
EFIAPI
GenCompatibleRangeTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN  EFI_PHYSICAL_ADDRESS                     StartAddress,
  IN  UINT64                                   Length
  );

#endif
