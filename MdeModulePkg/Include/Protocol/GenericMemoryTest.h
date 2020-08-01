/** @file
  This protocol defines the generic memory test interfaces in Dxe phase.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __GENERIC_MEMORY_TEST_H__
#define __GENERIC_MEMORY_TEST_H__

#define EFI_GENERIC_MEMORY_TEST_PROTOCOL_GUID  \
  { 0x309de7f1, 0x7f5e, 0x4ace, {0xb4, 0x9c, 0x53, 0x1b, 0xe5, 0xaa, 0x95, 0xef} }

typedef struct _EFI_GENERIC_MEMORY_TEST_PROTOCOL  EFI_GENERIC_MEMORY_TEST_PROTOCOL;

///
/// Memory test coverage level.
/// Ignore chooses not to test memory. Quick and Sparse test some memory, and Extensive performs a detailed memory test.
///
typedef enum {
  IGNORE,
  QUICK,
  SPARSE,
  EXTENSIVE,
  MAXLEVEL
} EXTENDMEM_COVERAGE_LEVEL;


/**
  Initialize the generic memory test.

  @param  This                The protocol instance pointer.
  @param  Level               The coverage level of the memory test.
  @param  RequireSoftECCInit  Indicate if the memory need software ECC init.

  @retval EFI_SUCCESS         The generic memory test is initialized correctly.
  @retval EFI_NO_MEDIA        The system had no memory to be tested.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MEMORY_TEST_INIT)(
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN  EXTENDMEM_COVERAGE_LEVEL                 Level,
  OUT BOOLEAN                                  *RequireSoftECCInit
  );


/**
  Perform the memory test.

  @param  This                The protocol instance pointer.
  @param  TestedMemorySize    Return the tested extended memory size.
  @param  TotalMemorySize     Return the whole system physical memory size.
                              The total memory size does not include memory in a slot with a disabled DIMM.
  @param  ErrorOut            TRUE if the memory error occurred.
  @param  IfTestAbort         Indicates that the user pressed "ESC" to skip the memory test.

  @retval EFI_SUCCESS         One block of memory passed the test.
  @retval EFI_NOT_FOUND       All memory blocks have already been tested.
  @retval EFI_DEVICE_ERROR    Memory device error occurred, and no agent can handle it.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PERFORM_MEMORY_TEST)(
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  OUT UINT64                                   *TestedMemorySize,
  OUT UINT64                                   *TotalMemorySize,
  OUT BOOLEAN                                  *ErrorOut,
  IN BOOLEAN                                   IfTestAbort
  );


/**
  Finish the memory test.

  @param  This                The protocol instance pointer.

  @retval EFI_SUCCESS         Success. All resources used in the memory test are freed.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MEMORY_TEST_FINISHED)(
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This
  );

/**
  Provides the capability to test the compatible range used by some special drivers.

  @param  This                The protocol instance pointer.
  @param  StartAddress        The start address of the compatible memory range that
                              must be below 16M.
  @param  Length              The compatible memory range's length.

  @retval EFI_SUCCESS           The compatible memory range pass the memory test.
  @retval EFI_INVALID_PARAMETER The compatible memory range are not below Low 16M.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MEMORY_TEST_COMPATIBLE_RANGE)(
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN  EFI_PHYSICAL_ADDRESS                     StartAddress,
  IN  UINT64                                   Length
  );

struct _EFI_GENERIC_MEMORY_TEST_PROTOCOL {
  EFI_MEMORY_TEST_INIT              MemoryTestInit;
  EFI_PERFORM_MEMORY_TEST           PerformMemoryTest;
  EFI_MEMORY_TEST_FINISHED          Finished;
  EFI_MEMORY_TEST_COMPATIBLE_RANGE  CompatibleRangeTest;
};

extern EFI_GUID gEfiGenericMemTestProtocolGuid;

#endif

