/** @file
  This protocol defines the generic memory test interfaces in Dxe phase.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __GENERIC_MEMORY_TEST_H__
#define __GENERIC_MEMORY_TEST_H__

#define EFI_GENERIC_MEMORY_TEST_PROTOCOL_GUID  \
  { 0x309de7f1, 0x7f5e, 0x4ace, {0xb4, 0x9c, 0x53, 0x1b, 0xe5, 0xaa, 0x95, 0xef} }

typedef struct _EFI_GENERIC_MEMORY_TEST_PROTOCOL  EFI_GENERIC_MEMORY_TEST_PROTOCOL;

///
/// Memory test coverage level
/// Ignore op not test memory, Quick and Sparse op test memory quickly, Extensive op test memory detailedly.
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

  @param  This                Protocol instance pointer. 
  @param  Level               The coverage level of the memory test. 
  @param  RequireSoftECCInit  Indicate if the memory need software ECC init. 

  @retval EFI_SUCCESS         The generic memory test is initialized correctly. 
  @retval EFI_NO_MEDIA        There is not any non-tested memory found, which means  
                              that the memory test driver have not detect any 
                              non-tested extended memory in current system. 

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

  @param  This                Protocol instance pointer. 
  @param  TestedMemorySize    Return the tested extended memory size. 
  @param  TotalMemorySize     Return the whole system physical memory size, this  
                              value may be changed if some error DIMMs is disabled in some case. 
  @param  ErrorOut            TRUE if the memory error occurs.
  @param  IfTestAbort         Indicate if the user press "ESC" to skip the memory test. 

  @retval EFI_SUCCESS         One block of memory pass test.
  @retval EFI_NOT_FOUND       Indicate all the non-tested memory blocks have been
                              already gone through.
  @retval EFI_DEVICE_ERROR    Memory device error occurs and no agent can handle it.

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

  @param  This                Protocol instance pointer. 

  @retval EFI_SUCCESS         Success. Then free all the generic memory test driver
                              allocated resource and notify to platform memory
                              test driver that memory test finished.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MEMORY_TEST_FINISHED)(
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This
  );

/**
  Provide capability to test compatible range used by some sepcial
  driver before BDS perform memory test.

  @param  This                Protocol instance pointer. 
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

