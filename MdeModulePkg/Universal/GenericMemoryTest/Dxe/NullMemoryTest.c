/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  NullMemoryTest.c
  
Abstract:

--*/


#include "NullMemoryTest.h"

//
// Module global members
//
UINT64                            mTestedSystemMemory = 0;
UINT64                            mTotalSystemMemory  = 0;
EFI_HANDLE                        mGenericMemoryTestHandle;

//
// Driver entry here
//
EFI_GENERIC_MEMORY_TEST_PROTOCOL  mGenericMemoryTest = {
  InitializeMemoryTest,
  GenPerformMemoryTest,
  GenMemoryTestFinished,
  GenCompatibleRangeTest
};

EFI_STATUS
EFIAPI
GenericMemoryTestEntryPoint (
  IN  EFI_HANDLE           ImageHandle,
  IN  EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  The generic memory test driver's entry point, it can initialize private data
  to default value

Arguments:

  ImageHandle of the loaded driver
  Pointer to the System Table

Returns:

  Status

  EFI_SUCCESS           - Protocol successfully installed
  EFI_OUT_OF_RESOURCES  - Can not allocate protocol data structure in base
                          memory

--*/
{
  EFI_STATUS  Status;

  //
  // Install the protocol
  //
  Status = gBS->InstallProtocolInterface (
                  &mGenericMemoryTestHandle,
                  &gEfiGenericMemTestProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mGenericMemoryTest
                  );

  return Status;
}
//
// EFI_GENERIC_MEMORY_TEST_PROTOCOL implementation
//
EFI_STATUS
EFIAPI
InitializeMemoryTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN  EXTENDMEM_COVERAGE_LEVEL                 Level,
  OUT BOOLEAN                                  *RequireSoftECCInit
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
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

EFI_STATUS
EFIAPI
GenPerformMemoryTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN OUT UINT64                                *TestedMemorySize,
  OUT UINT64                                   *TotalMemorySize,
  OUT BOOLEAN                                  *ErrorOut,
  IN BOOLEAN                                   TestAbort
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  *ErrorOut         = FALSE;
  *TestedMemorySize = mTestedSystemMemory;
  *TotalMemorySize  = mTotalSystemMemory;

  return EFI_NOT_FOUND;

}

EFI_STATUS
EFIAPI
GenMemoryTestFinished (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GenCompatibleRangeTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN  EFI_PHYSICAL_ADDRESS                     StartAddress,
  IN  UINT64                                   Length
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR descriptor;

  gDS->GetMemorySpaceDescriptor (StartAddress, &descriptor);

  gDS->RemoveMemorySpace (StartAddress, Length);

  gDS->AddMemorySpace (
        EfiGcdMemoryTypeSystemMemory,
        StartAddress,
        Length,
        descriptor.Capabilities &~(EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED | EFI_MEMORY_RUNTIME)
        );

  return EFI_SUCCESS;
}
