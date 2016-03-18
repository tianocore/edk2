/** @file
  Implementation of Generic Memory Test Protocol which does not perform real memory test.

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "NullMemoryTest.h"

UINT64                            mTestedSystemMemory = 0;
UINT64                            mTotalSystemMemory  = 0;
EFI_HANDLE                        mGenericMemoryTestHandle;

EFI_GENERIC_MEMORY_TEST_PROTOCOL  mGenericMemoryTest = {
  InitializeMemoryTest,
  GenPerformMemoryTest,
  GenMemoryTestFinished,
  GenCompatibleRangeTest
};

/**
  Entry point of the NULL memory test driver.
  
  This function is the entry point of the NULL memory test driver.
  It simply installs the Generic Memory Test Protocol.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    Generic Memory Test Protocol is successfully installed.

**/
EFI_STATUS
EFIAPI
GenericMemoryTestEntryPoint (
  IN  EFI_HANDLE           ImageHandle,
  IN  EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallProtocolInterface (
                  &mGenericMemoryTestHandle,
                  &gEfiGenericMemTestProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mGenericMemoryTest
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

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
  )
{
  UINTN                           NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR *MemorySpaceMap;
  UINTN                           Index;

  gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeReserved &&
        (MemorySpaceMap[Index].Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
          (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED)
          ) {
      //
      // For those reserved memory that have not been tested, simply promote to system memory.
      //
      gDS->RemoveMemorySpace (
            MemorySpaceMap[Index].BaseAddress,
            MemorySpaceMap[Index].Length
            );

      gDS->AddMemorySpace (
            EfiGcdMemoryTypeSystemMemory,
            MemorySpaceMap[Index].BaseAddress,
            MemorySpaceMap[Index].Length,
            MemorySpaceMap[Index].Capabilities &~
            (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED | EFI_MEMORY_RUNTIME)
            );

      mTestedSystemMemory += MemorySpaceMap[Index].Length;
      mTotalSystemMemory += MemorySpaceMap[Index].Length;
    } else if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeSystemMemory) {
      mTotalSystemMemory += MemorySpaceMap[Index].Length;
    }
  }

  FreePool (MemorySpaceMap);

  *RequireSoftECCInit = FALSE;
  return EFI_SUCCESS;
}

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
  )
{
  *ErrorOut         = FALSE;
  *TestedMemorySize = mTestedSystemMemory;
  *TotalMemorySize  = mTotalSystemMemory;

  return EFI_NOT_FOUND;

}

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
  )
{
  return EFI_SUCCESS;
}

/**
  Provide capability to test compatible range which used by some special
  driver required using memory range before BDS perform memory test.

  This function implements EFI_GENERIC_MEMORY_TEST_PROTOCOL.CompatibleRangeTest.
  It simply sets the memory range to system memory.

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
  )
{
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR Descriptor;

  gDS->GetMemorySpaceDescriptor (StartAddress, &Descriptor);

  gDS->RemoveMemorySpace (StartAddress, Length);

  gDS->AddMemorySpace (
        EfiGcdMemoryTypeSystemMemory,
        StartAddress,
        Length,
        Descriptor.Capabilities &~(EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED | EFI_MEMORY_RUNTIME)
        );

  return EFI_SUCCESS;
}
