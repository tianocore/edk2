/** @file

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LightMemoryTest.h"

//
// Global:
// Since this driver will only ever produce one instance of the memory test
// protocol, so we do not need to dynamically allocate the PrivateData.
//
EFI_PHYSICAL_ADDRESS    mCurrentAddress;
LIST_ENTRY          *mCurrentLink;
NONTESTED_MEMORY_RANGE  *mCurrentRange;
UINT64                  mTestedSystemMemory;
UINT64                  mNonTestedSystemMemory;

UINT32                  GenericMemoryTestMonoPattern[GENERIC_CACHELINE_SIZE / 4] = {
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5
};

/**
  Compares the contents of two buffers.

  This function compares Length bytes of SourceBuffer to Length bytes of DestinationBuffer.
  If all Length bytes of the two buffers are identical, then 0 is returned.  Otherwise, the
  value returned is the first mismatched byte in SourceBuffer subtracted from the first
  mismatched byte in DestinationBuffer.

  If Length = 0, then ASSERT().

  @param[in] DestinationBuffer The pointer to the destination buffer to compare.
  @param[in] SourceBuffer      The pointer to the source buffer to compare.
  @param[in] Length            The number of bytes to compare.

  @return 0                 All Length bytes of the two buffers are identical.
  @retval Non-zero          The first mismatched byte in SourceBuffer subtracted from the first
                            mismatched byte in DestinationBuffer.

**/
INTN
EFIAPI
CompareMemWithoutCheckArgument (
  IN      CONST VOID                *DestinationBuffer,
  IN      CONST VOID                *SourceBuffer,
  IN      UINTN                     Length
  )
{
  ASSERT (Length > 0);
  while ((--Length != 0) &&
         (*(INT8*)DestinationBuffer == *(INT8*)SourceBuffer)) {
    DestinationBuffer = (INT8*)DestinationBuffer + 1;
    SourceBuffer = (INT8*)SourceBuffer + 1;
  }
  return (INTN)*(UINT8*)DestinationBuffer - (INTN)*(UINT8*)SourceBuffer;
}

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
  )
{
  UINTN                           NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR *MemorySpaceMap;
  UINTN                           Index;

  //
  // Base memory will always below 4G
  //
  gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);

  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if ((MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeSystemMemory) ||
        (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeMoreReliable)) {
      Private->BaseMemorySize += MemorySpaceMap[Index].Length;
    }
  }

  return EFI_SUCCESS;
}

/**
  Destroy the link list base on the correspond link list type.

  @param[in] Private  Point to generic memory test driver's private data.

**/
VOID
DestroyLinkList (
  IN  GENERIC_MEMORY_TEST_PRIVATE  *Private
  )
{
  LIST_ENTRY          *Link;
  NONTESTED_MEMORY_RANGE  *NontestedRange;

  Link = Private->NonTestedMemRanList.BackLink;

  while (Link != &Private->NonTestedMemRanList) {
    RemoveEntryList (Link);
    NontestedRange = NONTESTED_MEMORY_RANGE_FROM_LINK (Link);
    gBS->FreePool (NontestedRange);
    Link = Private->NonTestedMemRanList.BackLink;;
  }
}

/**
  Convert the memory range to tested.

  @param BaseAddress  Base address of the memory range.
  @param Length       Length of the memory range.
  @param Capabilities Capabilities of the memory range.

  @retval EFI_SUCCESS The memory range is converted to tested.
  @retval others      Error happens.
**/
EFI_STATUS
ConvertToTestedMemory (
  IN UINT64           BaseAddress,
  IN UINT64           Length,
  IN UINT64           Capabilities
  )
{
  EFI_STATUS Status;
  Status = gDS->RemoveMemorySpace (
                  BaseAddress,
                  Length
                  );
  if (!EFI_ERROR (Status)) {
    Status = gDS->AddMemorySpace (
                    ((Capabilities & EFI_MEMORY_MORE_RELIABLE) == EFI_MEMORY_MORE_RELIABLE) ?
                    EfiGcdMemoryTypeMoreReliable : EfiGcdMemoryTypeSystemMemory,
                    BaseAddress,
                    Length,
                    Capabilities &~
                    (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED | EFI_MEMORY_RUNTIME)
                    );
  }
  return Status;
}

/**
  Add the extened memory to whole system memory map.

  @param[in] Private  Point to generic memory test driver's private data.

  @retval EFI_SUCCESS Successful add all the extended memory to system memory map.
  @retval Others      Failed to add the tested extended memory.

**/
EFI_STATUS
UpdateMemoryMap (
  IN  GENERIC_MEMORY_TEST_PRIVATE  *Private
  )
{
  LIST_ENTRY          *Link;
  NONTESTED_MEMORY_RANGE  *Range;

  Link = Private->NonTestedMemRanList.ForwardLink;

  while (Link != &Private->NonTestedMemRanList) {
    Range = NONTESTED_MEMORY_RANGE_FROM_LINK (Link);

    ConvertToTestedMemory (
      Range->StartAddress,
      Range->Length,
      Range->Capabilities &~
      (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED | EFI_MEMORY_RUNTIME)
      );
    Link = Link->ForwardLink;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS  Status;

  //
  // Perform a dummy memory test, so directly write the pattern to all range
  //
  WriteMemory (Private, StartAddress, Length);

  //
  // Verify the memory range
  //
  Status = VerifyMemory (Private, StartAddress, Length);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Add the tested compatible memory to system memory using GCD service
  //
  ConvertToTestedMemory (
      StartAddress,
      Length,
      Capabilities &~
      (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED | EFI_MEMORY_RUNTIME)
      );

  return EFI_SUCCESS;
}

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
  )
{
  NONTESTED_MEMORY_RANGE          *Range;
  BOOLEAN                         NoFound;
  UINTN                           NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR *MemorySpaceMap;
  UINTN                           Index;

  //
  // Non tested memory range may be span 4G here
  //
  NoFound = TRUE;

  gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);

  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeReserved &&
        (MemorySpaceMap[Index].Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
          (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED)
          ) {
      NoFound = FALSE;
      //
      // Light version do not need to process >4G memory range
      //
      gBS->AllocatePool (
            EfiBootServicesData,
            sizeof (NONTESTED_MEMORY_RANGE),
            (VOID **) &Range
            );

      Range->Signature    = EFI_NONTESTED_MEMORY_RANGE_SIGNATURE;
      Range->StartAddress = MemorySpaceMap[Index].BaseAddress;
      Range->Length       = MemorySpaceMap[Index].Length;
      Range->Capabilities = MemorySpaceMap[Index].Capabilities;

      mNonTestedSystemMemory += MemorySpaceMap[Index].Length;
      InsertTailList (&Private->NonTestedMemRanList, &Range->Link);
    }
  }

  if (NoFound) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_PHYSICAL_ADDRESS  Address;

  Address = Start;

  //
  // Add 4G memory address check for IA32 platform
  // NOTE: Without page table, there is no way to use memory above 4G.
  //
  if (Start + Size > MAX_ADDRESS) {
    return EFI_SUCCESS;
  }

  while (Address < (Start + Size)) {
    CopyMem ((VOID *) (UINTN) Address, Private->MonoPattern, Private->MonoTestSize);
    Address += Private->CoverageSpan;
  }
  //
  // bug bug: we may need GCD service to make the code cache and data uncache,
  // if GCD do not support it or return fail, then just flush the whole cache.
  //
  if (Private->Cpu != NULL) {
    Private->Cpu->FlushDataCache (Private->Cpu, Start, Size, EfiCpuFlushTypeWriteBackInvalidate);
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_PHYSICAL_ADDRESS            Address;
  INTN                            ErrorFound;
  EFI_MEMORY_EXTENDED_ERROR_DATA  *ExtendedErrorData;

  Address           = Start;
  ExtendedErrorData = NULL;

  //
  // Add 4G memory address check for IA32 platform
  // NOTE: Without page table, there is no way to use memory above 4G.
  //
  if (Start + Size > MAX_ADDRESS) {
    return EFI_SUCCESS;
  }

  //
  // Use the software memory test to check whether have detected miscompare
  // error here. If there is miscompare error here then check if generic
  // memory test driver can disable the bad DIMM.
  //
  while (Address < (Start + Size)) {
    ErrorFound = CompareMemWithoutCheckArgument (
                  (VOID *) (UINTN) (Address),
                  Private->MonoPattern,
                  Private->MonoTestSize
                  );
    if (ErrorFound != 0) {
      //
      // Report uncorrectable errors
      //
      ExtendedErrorData = AllocateZeroPool (sizeof (EFI_MEMORY_EXTENDED_ERROR_DATA));
      if (ExtendedErrorData == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      ExtendedErrorData->DataHeader.HeaderSize  = (UINT16) sizeof (EFI_STATUS_CODE_DATA);
      ExtendedErrorData->DataHeader.Size        = (UINT16) (sizeof (EFI_MEMORY_EXTENDED_ERROR_DATA) - sizeof (EFI_STATUS_CODE_DATA));
      ExtendedErrorData->Granularity            = EFI_MEMORY_ERROR_DEVICE;
      ExtendedErrorData->Operation              = EFI_MEMORY_OPERATION_READ;
      ExtendedErrorData->Syndrome               = 0x0;
      ExtendedErrorData->Address                = Address;
      ExtendedErrorData->Resolution             = 0x40;

      REPORT_STATUS_CODE_EX (
          EFI_ERROR_CODE,
          EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_UNCORRECTABLE,
          0,
          &gEfiGenericMemTestProtocolGuid,
          NULL,
          (UINT8 *) ExtendedErrorData + sizeof (EFI_STATUS_CODE_DATA),
          ExtendedErrorData->DataHeader.Size
          );

      return EFI_DEVICE_ERROR;
    }

    Address += Private->CoverageSpan;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS                  Status;
  GENERIC_MEMORY_TEST_PRIVATE *Private;
  EFI_CPU_ARCH_PROTOCOL       *Cpu;

  Private             = GENERIC_MEMORY_TEST_PRIVATE_FROM_THIS (This);
  *RequireSoftECCInit = FALSE;

  //
  // This is initialize for default value, but some value may be reset base on
  // platform memory test driver.
  //
  Private->CoverLevel   = Level;
  Private->BdsBlockSize = TEST_BLOCK_SIZE;
  Private->MonoPattern  = GenericMemoryTestMonoPattern;
  Private->MonoTestSize = GENERIC_CACHELINE_SIZE;

  //
  // Initialize several internal link list
  //
  InitializeListHead (&Private->NonTestedMemRanList);

  //
  // Construct base memory range
  //
  ConstructBaseMemoryRange (Private);

  //
  // get the cpu arch protocol to support flash cache
  //
  Status = gBS->LocateProtocol (
                  &gEfiCpuArchProtocolGuid,
                  NULL,
                  (VOID **) &Cpu
                  );
  if (!EFI_ERROR (Status)) {
    Private->Cpu = Cpu;
  }
  //
  // Create the CoverageSpan of the memory test base on the coverage level
  //
  switch (Private->CoverLevel) {
  case EXTENSIVE:
    Private->CoverageSpan = GENERIC_CACHELINE_SIZE;
    break;

  case SPARSE:
    Private->CoverageSpan = SPARSE_SPAN_SIZE;
    break;

  //
  // Even the BDS do not need to test any memory, but in some case it
  // still need to init ECC memory.
  //
  default:
    Private->CoverageSpan = QUICK_SPAN_SIZE;
    break;
  }
  //
  // This is the first time we construct the non-tested memory range, if no
  // extended memory found, we know the system have not any extended memory
  // need to be test
  //
  Status = ConstructNonTestedMemoryRange (Private);
  if (Status == EFI_NOT_FOUND) {
    return EFI_NO_MEDIA;
  }
  //
  // ready to perform the R/W/V memory test
  //
  mTestedSystemMemory = Private->BaseMemorySize;
  mCurrentLink        = Private->NonTestedMemRanList.ForwardLink;
  mCurrentRange       = NONTESTED_MEMORY_RANGE_FROM_LINK (mCurrentLink);
  mCurrentAddress     = mCurrentRange->StartAddress;

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS                      Status;
  GENERIC_MEMORY_TEST_PRIVATE     *Private;
  EFI_MEMORY_RANGE_EXTENDED_DATA  *RangeData;
  UINT64                          BlockBoundary;

  Private       = GENERIC_MEMORY_TEST_PRIVATE_FROM_THIS (This);
  *ErrorOut     = FALSE;
  RangeData     = NULL;
  BlockBoundary = 0;

  //
  // In extensive mode the boundary of "mCurrentRange->Length" may will lost
  // some range that is not Private->BdsBlockSize size boundary, so need
  // the software mechanism to confirm all memory location be covered.
  //
  if (mCurrentAddress < (mCurrentRange->StartAddress + mCurrentRange->Length)) {
    if ((mCurrentAddress + Private->BdsBlockSize) <= (mCurrentRange->StartAddress + mCurrentRange->Length)) {
      BlockBoundary = Private->BdsBlockSize;
    } else {
      BlockBoundary = mCurrentRange->StartAddress + mCurrentRange->Length - mCurrentAddress;
    }
    //
    // If TestAbort is true, means user cancel the memory test
    //
    if (!TestAbort && Private->CoverLevel != IGNORE) {
      //
      // Report status code of every memory range
      //
      RangeData                         = AllocateZeroPool (sizeof (EFI_MEMORY_RANGE_EXTENDED_DATA));
      if (RangeData == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      RangeData->DataHeader.HeaderSize  = (UINT16) sizeof (EFI_STATUS_CODE_DATA);
      RangeData->DataHeader.Size        = (UINT16) (sizeof (EFI_MEMORY_RANGE_EXTENDED_DATA) - sizeof (EFI_STATUS_CODE_DATA));
      RangeData->Start                  = mCurrentAddress;
      RangeData->Length                 = BlockBoundary;

      REPORT_STATUS_CODE_EX (
          EFI_PROGRESS_CODE,
          EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_TEST,
          0,
          &gEfiGenericMemTestProtocolGuid,
          NULL,
          (UINT8 *) RangeData + sizeof (EFI_STATUS_CODE_DATA),
          RangeData->DataHeader.Size
          );

      //
      // The software memory test (R/W/V) perform here. It will detect the
      // memory mis-compare error.
      //
      WriteMemory (Private, mCurrentAddress, BlockBoundary);

      Status = VerifyMemory (Private, mCurrentAddress, BlockBoundary);
      if (EFI_ERROR (Status)) {
        //
        // If perform here, means there is mis-compare error, and no agent can
        // handle it, so we return to BDS EFI_DEVICE_ERROR.
        //
        *ErrorOut = TRUE;
        return EFI_DEVICE_ERROR;
      }
    }

    mTestedSystemMemory += BlockBoundary;
    *TestedMemorySize = mTestedSystemMemory;

    //
    // If the memory test restart after the platform driver disable dimms,
    // the NonTestSystemMemory may be changed, but the base memory size will
    // not changed, so we can get the current total memory size.
    //
    *TotalMemorySize = Private->BaseMemorySize + mNonTestedSystemMemory;

    //
    // Update the current test address pointing to next BDS BLOCK
    //
    mCurrentAddress += Private->BdsBlockSize;

    return EFI_SUCCESS;
  }
  //
  // Change to next non tested memory range
  //
  mCurrentLink = mCurrentLink->ForwardLink;
  if (mCurrentLink != &Private->NonTestedMemRanList) {
    mCurrentRange   = NONTESTED_MEMORY_RANGE_FROM_LINK (mCurrentLink);
    mCurrentAddress = mCurrentRange->StartAddress;
    return EFI_SUCCESS;
  } else {
    //
    // Here means all the memory test have finished
    //
    *TestedMemorySize = mTestedSystemMemory;
    *TotalMemorySize  = Private->BaseMemorySize + mNonTestedSystemMemory;
    return EFI_NOT_FOUND;
  }

}

/**
  Finish the memory test.

  @param[in] This             The protocol instance pointer.

  @retval EFI_SUCCESS         Success. All resources used in the memory test are freed.

**/
EFI_STATUS
EFIAPI
GenMemoryTestFinished (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This
  )
{
  EFI_STATUS                  Status;
  GENERIC_MEMORY_TEST_PRIVATE *Private;

  Private = GENERIC_MEMORY_TEST_PRIVATE_FROM_THIS (This);

  //
  // Perform Data and Address line test
  //
  Status = PerformAddressDataLineTest (Private);
  ASSERT_EFI_ERROR (Status);

  //
  // Add the non tested memory range to system memory map through GCD service
  //
  UpdateMemoryMap (Private);

  //
  // we need to free all the memory allocate
  //
  DestroyLinkList (Private);

  return EFI_SUCCESS;
}

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
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL         *This,
  IN EFI_PHYSICAL_ADDRESS                     StartAddress,
  IN UINT64                                   Length
  )
{
  EFI_STATUS                      Status;
  GENERIC_MEMORY_TEST_PRIVATE     *Private;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR Descriptor;
  EFI_PHYSICAL_ADDRESS            CurrentBase;
  UINT64                          CurrentLength;

  Private = GENERIC_MEMORY_TEST_PRIVATE_FROM_THIS (This);

  //
  // Check if the parameter is below 16MB
  //
  if (StartAddress + Length > 0x1000000) {
    return EFI_INVALID_PARAMETER;
  }
  CurrentBase = StartAddress;
  do {
    //
    // Check the required memory range status; if the required memory range span
    // the different GCD memory descriptor, it may be cause different action.
    //
    Status = gDS->GetMemorySpaceDescriptor (
                    CurrentBase,
                    &Descriptor
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Descriptor.GcdMemoryType == EfiGcdMemoryTypeReserved &&
        (Descriptor.Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
          (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED)
          ) {
      CurrentLength = Descriptor.BaseAddress + Descriptor.Length - CurrentBase;
      if (CurrentBase + CurrentLength > StartAddress + Length) {
        CurrentLength = StartAddress + Length - CurrentBase;
      }
      Status = DirectRangeTest (
                 Private,
                 CurrentBase,
                 CurrentLength,
                 Descriptor.Capabilities
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
    CurrentBase = Descriptor.BaseAddress + Descriptor.Length;
  } while (CurrentBase < StartAddress + Length);
  //
  // Here means the required range already be tested, so just return success.
  //
  return EFI_SUCCESS;
}

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
  )
{
  LIST_ENTRY              *ExtendedLink;
  NONTESTED_MEMORY_RANGE  *ExtendedRange;
  BOOLEAN                 InExtendedRange;
  EFI_PHYSICAL_ADDRESS    TestAddress;

  //
  // Light version no data line test, only perform the address line test
  //
  TestAddress = (EFI_PHYSICAL_ADDRESS) 0x1;
  while (TestAddress < MAX_ADDRESS && TestAddress > 0) {
    //
    // only test if the address falls in the enabled range
    //
    InExtendedRange = FALSE;
    ExtendedLink    = Private->NonTestedMemRanList.BackLink;
    while (ExtendedLink != &Private->NonTestedMemRanList) {
      ExtendedRange = NONTESTED_MEMORY_RANGE_FROM_LINK (ExtendedLink);
      if ((TestAddress >= ExtendedRange->StartAddress) &&
          (TestAddress < (ExtendedRange->StartAddress + ExtendedRange->Length))
          ) {
        InExtendedRange = TRUE;
      }

      ExtendedLink = ExtendedLink->BackLink;
    }

    if (InExtendedRange) {
      *(EFI_PHYSICAL_ADDRESS *) (UINTN) TestAddress = TestAddress;
      Private->Cpu->FlushDataCache (Private->Cpu, TestAddress, 1, EfiCpuFlushTypeWriteBackInvalidate);
      if (*(EFI_PHYSICAL_ADDRESS *) (UINTN) TestAddress != TestAddress) {
        return EFI_ACCESS_DENIED;
      }
    }

    TestAddress = LShiftU64 (TestAddress, 1);
  }

  return EFI_SUCCESS;
}
//
// Driver entry here
//
GENERIC_MEMORY_TEST_PRIVATE mGenericMemoryTestPrivate = {
  EFI_GENERIC_MEMORY_TEST_PRIVATE_SIGNATURE,
  NULL,
  NULL,
  {
    InitializeMemoryTest,
    GenPerformMemoryTest,
    GenMemoryTestFinished,
    GenCompatibleRangeTest
  },
  (EXTENDMEM_COVERAGE_LEVEL) 0,
  0,
  0,
  NULL,
  0,
  0,
  {
    NULL,
    NULL
  }
};

/**
  The generic memory test driver's entry point.

  It initializes private data to default value.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval EFI_NOT_FOUND   Can't find HandOff Hob in HobList.
  @retval other           Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
GenericMemoryTestEntryPoint (
  IN  EFI_HANDLE           ImageHandle,
  IN  EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS            Status;
  VOID                  *HobList;
  EFI_BOOT_MODE         BootMode;
  EFI_PEI_HOB_POINTERS  Hob;

  //
  // Use the generic pattern to test compatible memory range
  //
  mGenericMemoryTestPrivate.MonoPattern   = GenericMemoryTestMonoPattern;
  mGenericMemoryTestPrivate.MonoTestSize  = GENERIC_CACHELINE_SIZE;

  //
  // Get the platform boot mode
  //
  HobList = GetHobList ();

  Hob.Raw = HobList;
  if (Hob.Header->HobType != EFI_HOB_TYPE_HANDOFF) {
    return EFI_NOT_FOUND;
  }

  BootMode = Hob.HandoffInformationTable->BootMode;

  //
  // Get the platform boot mode and create the default memory test coverage
  // level and span size for compatible memory test using
  //
  switch (BootMode) {
  case BOOT_WITH_FULL_CONFIGURATION:
  case BOOT_WITH_DEFAULT_SETTINGS:
    mGenericMemoryTestPrivate.CoverageSpan = SPARSE_SPAN_SIZE;
    break;

  case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
    mGenericMemoryTestPrivate.CoverageSpan = GENERIC_CACHELINE_SIZE;
    break;

  default:
    mGenericMemoryTestPrivate.CoverageSpan = QUICK_SPAN_SIZE;
    break;
  }
  //
  // Install the protocol
  //
  Status = gBS->InstallProtocolInterface (
                  &mGenericMemoryTestPrivate.Handle,
                  &gEfiGenericMemTestProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mGenericMemoryTestPrivate.GenericMemoryTest
                  );

  return Status;
}
