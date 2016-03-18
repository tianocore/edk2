/** @file
  Include file of the NULL memory test driver.

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _NULL_MEMORY_TEST_H_
#define _NULL_MEMORY_TEST_H_


#include <PiDxe.h>


#include <Protocol/GenericMemoryTest.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

//
// Definition of memory status.
//
#define EFI_MEMORY_PRESENT      0x0100000000000000ULL
#define EFI_MEMORY_INITIALIZED  0x0200000000000000ULL
#define EFI_MEMORY_TESTED       0x0400000000000000ULL

/**
  Initialize the generic memory test.

  This function implements EFI_GENERIC_MEMORY_TEST_PROTOCOL.MemoryTestInit.
  It simply promotes untested reserved memory to system memory without real test.

  @param  This                Protocol instance pointer. 
  @param  Level               The coverage level of the memory test. 
  @param  RequireSoftECCInit  Indicate if the memory need software ECC init. 

  @retval EFI_SUCCESS         The generic memory test initialized correctly. 
  @retval EFI_NO_MEDIA        There is not any non-tested memory found, in this 
                              function if not any non-tesed memory found means  
                              that the memory test driver have not detect any 
                              non-tested extended memory of current system. 

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

  This function implements EFI_GENERIC_MEMORY_TEST_PROTOCOL.PerformMemoryTest.
  It simply returns EFI_NOT_FOUND.

  @param  This                Protocol instance pointer. 
  @param  TestedMemorySize    Return the tested extended memory size. 
  @param  TotalMemorySize     Return the whole system physical memory size, this  
                              value may be changed if in some case some error  
                              DIMMs be disabled. 
  @param  ErrorOut            Any time the memory error occurs, this will be 
                              TRUE. 
  @param  IfTestAbort         Indicate if the user press "ESC" to skip the memory 
                              test. 

  @retval EFI_SUCCESS         One block of memory test ok, the block size is hide 
                              internally. 
  @retval EFI_NOT_FOUND       Indicate all the non-tested memory blocks have  
                              already go through. 
  @retval EFI_DEVICE_ERROR    Mis-compare error, and no agent can handle it

**/
EFI_STATUS
EFIAPI
GenPerformMemoryTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN OUT UINT64                                *TestedMemorySize,
  OUT UINT64                                   *TotalMemorySize,
  OUT BOOLEAN                                  *ErrorOut,
  IN BOOLEAN                                   TestAbort
  );

/**
  The memory test finished.

  This function implements EFI_GENERIC_MEMORY_TEST_PROTOCOL.Finished.
  It simply returns EFI_SUCCESS.

  @param  This                Protocol instance pointer. 

  @retval EFI_SUCCESS         Successful free all the generic memory test driver 
                              allocated resource and notify to platform memory 
                              test driver that memory test finished. 

**/
EFI_STATUS
EFIAPI
GenMemoryTestFinished (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This
  );

/**
  Provide capability to test compatible range which used by some special
  driver required using memory range before BDS perform memory test.

  This function implements EFI_GENERIC_MEMORY_TEST_PROTOCOL.CompatibleRangeTest.
  It simply set the memory range to system memory.

  @param  This                Protocol instance pointer. 
  @param  StartAddress        The start address of the memory range. 
  @param  Length              The memory range's length. 
  
  @retval EFI_SUCCESS           The compatible memory range pass the memory test. 
  @retval EFI_INVALID_PARAMETER The compatible memory range must be below 16M.

**/
EFI_STATUS
EFIAPI
GenCompatibleRangeTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN  EFI_PHYSICAL_ADDRESS                     StartAddress,
  IN  UINT64                                   Length
  );

#endif
