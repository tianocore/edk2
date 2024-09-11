/** @file
Enable SMM profile.

Copyright (c) 2012 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017 - 2020, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"
#include "SmmProfileInternal.h"

UINT32  mSmmProfileCr3;

SMM_PROFILE_HEADER  *mSmmProfileBase;
MSR_DS_AREA_STRUCT  *mMsrDsAreaBase;
//
// The buffer to store SMM profile data.
//
UINTN  mSmmProfileSize;

//
// The buffer to enable branch trace store.
//
UINTN  mMsrDsAreaSize = SMM_PROFILE_DTS_SIZE;

//
// The flag indicates if execute-disable is supported by processor.
//
BOOLEAN  mXdSupported = TRUE;

//
// The flag indicates if execute-disable is enabled on processor.
//
BOOLEAN  mXdEnabled = FALSE;

//
// The flag indicates if BTS is supported by processor.
//
BOOLEAN  mBtsSupported = TRUE;

//
// The flag indicates if SMM profile is enabled.
//
BOOLEAN  mSmmProfileEnabled = FALSE;

//
// The flag indicates if SMM profile starts to record data.
//
BOOLEAN  mSmmProfileStart = FALSE;

//
// The flag indicates if #DB will be setup in #PF handler.
//
BOOLEAN  mSetupDebugTrap = FALSE;

//
// Record the page fault exception count for one instruction execution.
//
UINTN  *mPFEntryCount;

UINT64 (*mLastPFEntryValue)[MAX_PF_ENTRY_COUNT];
UINT64                    *(*mLastPFEntryPointer)[MAX_PF_ENTRY_COUNT];

MSR_DS_AREA_STRUCT   **mMsrDsArea;
BRANCH_TRACE_RECORD  **mMsrBTSRecord;
UINTN                mBTSRecordNumber;
PEBS_RECORD          **mMsrPEBSRecord;

//
// These memory ranges are always present, they does not generate the access type of page fault exception,
// but they possibly generate instruction fetch type of page fault exception.
//
MEMORY_PROTECTION_RANGE  *mProtectionMemRange     = NULL;
UINTN                    mProtectionMemRangeCount = 0;

//
// Some predefined memory ranges.
//
MEMORY_PROTECTION_RANGE  mProtectionMemRangeTemplate[] = {
  //
  // SMRAM range (to be fixed in runtime).
  // It is always present and instruction fetches are allowed.
  //
  {
    { 0x00000000, 0x00000000 }, TRUE, FALSE
  },

  //
  // SMM profile data range( to be fixed in runtime).
  // It is always present and instruction fetches are not allowed.
  //
  {
    { 0x00000000, 0x00000000 }, TRUE, TRUE
  },

  //
  // SMRAM ranges not covered by mCpuHotPlugData.SmrrBase/mCpuHotPlugData.SmrrSiz (to be fixed in runtime).
  // It is always present and instruction fetches are allowed.
  // {{0x00000000, 0x00000000},TRUE,FALSE},
  //

  //
  // Future extended range could be added here.
  //

  //
  // PCI MMIO ranges (to be added in runtime).
  // They are always present and instruction fetches are not allowed.
  //
};

//
// These memory ranges are mapped by 4KB-page instead of 2MB-page.
//
MEMORY_RANGE  *mSplitMemRange     = NULL;
UINTN         mSplitMemRangeCount = 0;

//
// SMI command port.
//
UINT32  mSmiCommandPort;

/**
  Disable branch trace store.

**/
VOID
DisableBTS (
  VOID
  )
{
  AsmMsrAnd64 (MSR_DEBUG_CTL, ~((UINT64)(MSR_DEBUG_CTL_BTS | MSR_DEBUG_CTL_TR)));
}

/**
  Enable branch trace store.

**/
VOID
EnableBTS (
  VOID
  )
{
  AsmMsrOr64 (MSR_DEBUG_CTL, (MSR_DEBUG_CTL_BTS | MSR_DEBUG_CTL_TR));
}

/**
  Get CPU Index from APIC ID.

**/
UINTN
GetCpuIndex (
  VOID
  )
{
  UINTN   Index;
  UINT32  ApicId;

  ApicId = GetApicId ();

  for (Index = 0; Index < mMaxNumberOfCpus; Index++) {
    if (gSmmCpuPrivate->ProcessorInfo[Index].ProcessorId == ApicId) {
      return Index;
    }
  }

  ASSERT (FALSE);
  return 0;
}

/**
  Get the source of IP after execute-disable exception is triggered.

  @param  CpuIndex        The index of CPU.
  @param  DestinationIP   The destination address.

**/
UINT64
GetSourceFromDestinationOnBts (
  UINTN   CpuIndex,
  UINT64  DestinationIP
  )
{
  BRANCH_TRACE_RECORD  *CurrentBTSRecord;
  UINTN                Index;
  BOOLEAN              FirstMatch;

  FirstMatch = FALSE;

  CurrentBTSRecord = (BRANCH_TRACE_RECORD *)mMsrDsArea[CpuIndex]->BTSIndex;
  for (Index = 0; Index < mBTSRecordNumber; Index++) {
    if ((UINTN)CurrentBTSRecord < (UINTN)mMsrBTSRecord[CpuIndex]) {
      //
      // Underflow
      //
      CurrentBTSRecord = (BRANCH_TRACE_RECORD *)((UINTN)mMsrDsArea[CpuIndex]->BTSAbsoluteMaximum - 1);
      CurrentBTSRecord--;
    }

    if (CurrentBTSRecord->LastBranchTo == DestinationIP) {
      //
      // Good! find 1st one, then find 2nd one.
      //
      if (!FirstMatch) {
        //
        // The first one is DEBUG exception
        //
        FirstMatch = TRUE;
      } else {
        //
        // Good find proper one.
        //
        return CurrentBTSRecord->LastBranchFrom;
      }
    }

    CurrentBTSRecord--;
  }

  return 0;
}

/**
  SMM profile specific INT 1 (single-step) exception handler.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.
**/
VOID
EFIAPI
DebugExceptionHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINTN  CpuIndex;
  UINTN  PFEntry;

  if (!mSmmProfileStart &&
      !HEAP_GUARD_NONSTOP_MODE &&
      !NULL_DETECTION_NONSTOP_MODE)
  {
    return;
  }

  CpuIndex = GetCpuIndex ();

  //
  // Clear last PF entries
  //
  for (PFEntry = 0; PFEntry < mPFEntryCount[CpuIndex]; PFEntry++) {
    *mLastPFEntryPointer[CpuIndex][PFEntry] = mLastPFEntryValue[CpuIndex][PFEntry];
  }

  //
  // Reset page fault exception count for next page fault.
  //
  mPFEntryCount[CpuIndex] = 0;

  //
  // Flush TLB
  //
  CpuFlushTlb ();

  //
  // Clear TF in EFLAGS
  //
  ClearTrapFlag (SystemContext);
}

/**
  Check if the input address is in SMM ranges.

  @param[in]  Address       The input address.

  @retval TRUE     The input address is in SMM.
  @retval FALSE    The input address is not in SMM.
**/
BOOLEAN
IsInSmmRanges (
  IN EFI_PHYSICAL_ADDRESS  Address
  )
{
  UINTN  Index;

  if ((Address >= mCpuHotPlugData.SmrrBase) && (Address < mCpuHotPlugData.SmrrBase + mCpuHotPlugData.SmrrSize)) {
    return TRUE;
  }

  for (Index = 0; Index < mSmmCpuSmramRangeCount; Index++) {
    if ((Address >= mSmmCpuSmramRanges[Index].CpuStart) &&
        (Address < mSmmCpuSmramRanges[Index].CpuStart + mSmmCpuSmramRanges[Index].PhysicalSize))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Check if the SMM profile page fault address above 4GB is in protected range or not.

  @param[in]   Address  The address of Memory.
  @param[out]  Nx       The flag indicates if the memory is execute-disable.

  @retval TRUE     The input address is in protected range.
  @retval FALSE    The input address is not in protected range.

**/
BOOLEAN
IsSmmProfilePFAddressAbove4GValid (
  IN  EFI_PHYSICAL_ADDRESS  Address,
  OUT BOOLEAN               *Nx
  )
{
  UINTN  Index;

  //
  // Check configuration
  //
  for (Index = 0; Index < mProtectionMemRangeCount; Index++) {
    if ((Address >= mProtectionMemRange[Index].Range.Base) && (Address < mProtectionMemRange[Index].Range.Top)) {
      *Nx = mProtectionMemRange[Index].Nx;
      return mProtectionMemRange[Index].Present;
    }
  }

  *Nx = TRUE;
  return FALSE;
}

/**
  Check if the memory address will be mapped by 4KB-page.

  @param  Address  The address of Memory.

**/
BOOLEAN
IsAddressSplit (
  IN EFI_PHYSICAL_ADDRESS  Address
  )
{
  UINTN  Index;

  if (mSmmProfileEnabled) {
    //
    // Check configuration
    //
    for (Index = 0; Index < mSplitMemRangeCount; Index++) {
      if ((Address >= mSplitMemRange[Index].Base) && (Address < mSplitMemRange[Index].Top)) {
        return TRUE;
      }
    }
  } else {
    if (Address < mCpuHotPlugData.SmrrBase) {
      if ((mCpuHotPlugData.SmrrBase - Address) < BASE_2MB) {
        return TRUE;
      }
    } else if (Address > (mCpuHotPlugData.SmrrBase + mCpuHotPlugData.SmrrSize - BASE_2MB)) {
      if ((Address - (mCpuHotPlugData.SmrrBase + mCpuHotPlugData.SmrrSize - BASE_2MB)) < BASE_2MB) {
        return TRUE;
      }
    }
  }

  //
  // Return default
  //
  return FALSE;
}

/**
  Function to compare 2 MEMORY_PROTECTION_RANGE based on range base.

  @param[in] Buffer1            pointer to Device Path poiner to compare
  @param[in] Buffer2            pointer to second DevicePath pointer to compare

  @retval 0                     Buffer1 equal to Buffer2
  @retval <0                    Buffer1 is less than Buffer2
  @retval >0                    Buffer1 is greater than Buffer2
**/
INTN
EFIAPI
ProtectionRangeCompare (
  IN  CONST VOID  *Buffer1,
  IN  CONST VOID  *Buffer2
  )
{
  if (((MEMORY_PROTECTION_RANGE *)Buffer1)->Range.Base > ((MEMORY_PROTECTION_RANGE *)Buffer2)->Range.Base) {
    return 1;
  } else if (((MEMORY_PROTECTION_RANGE *)Buffer1)->Range.Base < ((MEMORY_PROTECTION_RANGE *)Buffer2)->Range.Base) {
    return -1;
  }

  return 0;
}

/**
  Initialize the protected memory ranges and the 4KB-page mapped memory ranges.

**/
VOID
InitProtectedMemRange (
  VOID
  )
{
  UINTN                    Index;
  MM_CPU_MEMORY_REGION     *MemoryRegion;
  UINTN                    MemoryRegionCount;
  UINTN                    NumberOfAddedDescriptors;
  UINTN                    NumberOfProtectRange;
  UINTN                    NumberOfSpliteRange;
  UINTN                    TotalSize;
  EFI_PHYSICAL_ADDRESS     ProtectBaseAddress;
  EFI_PHYSICAL_ADDRESS     ProtectEndAddress;
  EFI_PHYSICAL_ADDRESS     Top2MBAlignedAddress;
  EFI_PHYSICAL_ADDRESS     Base2MBAlignedAddress;
  UINT64                   High4KBPageSize;
  UINT64                   Low4KBPageSize;
  MEMORY_PROTECTION_RANGE  MemProtectionRange;

  MemoryRegion             = NULL;
  MemoryRegionCount        = 0;
  NumberOfAddedDescriptors = mSmmCpuSmramRangeCount;
  NumberOfSpliteRange      = 0;

  //
  // Create extended protection MemoryRegion and add them into protected memory ranges.
  // Retrieve the accessible regions when SMM profile is enabled.
  // In SMM: only MMIO is accessible.
  // In MM: all regions described by resource HOBs are accessible.
  //
  CreateExtendedProtectionRange (&MemoryRegion, &MemoryRegionCount);
  ASSERT (MemoryRegion != NULL);

  NumberOfAddedDescriptors += MemoryRegionCount;

  ASSERT (NumberOfAddedDescriptors != 0);

  TotalSize           = NumberOfAddedDescriptors * sizeof (MEMORY_PROTECTION_RANGE) + sizeof (mProtectionMemRangeTemplate);
  mProtectionMemRange = (MEMORY_PROTECTION_RANGE *)AllocateZeroPool (TotalSize);
  ASSERT (mProtectionMemRange != NULL);
  mProtectionMemRangeCount = TotalSize / sizeof (MEMORY_PROTECTION_RANGE);

  //
  // Copy existing ranges.
  //
  CopyMem (mProtectionMemRange, mProtectionMemRangeTemplate, sizeof (mProtectionMemRangeTemplate));

  //
  // Create split ranges which come from protected ranges.
  //
  TotalSize      = (TotalSize / sizeof (MEMORY_PROTECTION_RANGE)) * sizeof (MEMORY_RANGE);
  mSplitMemRange = (MEMORY_RANGE *)AllocateZeroPool (TotalSize);
  ASSERT (mSplitMemRange != NULL);

  //
  // Create SMM ranges which are set to present and execution-enable.
  //
  NumberOfProtectRange = sizeof (mProtectionMemRangeTemplate) / sizeof (MEMORY_PROTECTION_RANGE);
  for (Index = 0; Index < mSmmCpuSmramRangeCount; Index++) {
    if ((mSmmCpuSmramRanges[Index].CpuStart >= mProtectionMemRange[0].Range.Base) &&
        (mSmmCpuSmramRanges[Index].CpuStart + mSmmCpuSmramRanges[Index].PhysicalSize < mProtectionMemRange[0].Range.Top))
    {
      //
      // If the address have been already covered by mCpuHotPlugData.SmrrBase/mCpuHotPlugData.SmrrSiz
      //
      break;
    }

    mProtectionMemRange[NumberOfProtectRange].Range.Base = mSmmCpuSmramRanges[Index].CpuStart;
    mProtectionMemRange[NumberOfProtectRange].Range.Top  = mSmmCpuSmramRanges[Index].CpuStart + mSmmCpuSmramRanges[Index].PhysicalSize;
    mProtectionMemRange[NumberOfProtectRange].Present    = TRUE;
    mProtectionMemRange[NumberOfProtectRange].Nx         = FALSE;
    NumberOfProtectRange++;
  }

  //
  // Create protection ranges which are set to present and execution-disable.
  //
  for (Index = 0; Index < MemoryRegionCount; Index++) {
    mProtectionMemRange[NumberOfProtectRange].Range.Base = MemoryRegion[Index].Base;
    mProtectionMemRange[NumberOfProtectRange].Range.Top  = MemoryRegion[Index].Base +  MemoryRegion[Index].Length;
    mProtectionMemRange[NumberOfProtectRange].Present    = TRUE;
    mProtectionMemRange[NumberOfProtectRange].Nx         = TRUE;
    NumberOfProtectRange++;
  }

  //
  // Free the MemoryRegion
  //
  if (MemoryRegion != NULL) {
    FreePool (MemoryRegion);
  }

  //
  // Check and updated actual protected memory ranges count
  //
  ASSERT (NumberOfProtectRange <= mProtectionMemRangeCount);
  mProtectionMemRangeCount = NumberOfProtectRange;

  //
  // According to protected ranges, create the ranges which will be mapped by 2KB page.
  //
  NumberOfSpliteRange  = 0;
  NumberOfProtectRange = mProtectionMemRangeCount;
  for (Index = 0; Index < NumberOfProtectRange; Index++) {
    //
    // If base address is not 2MB alignment, make 2MB alignment for create 4KB page in page table.
    //
    ProtectBaseAddress = mProtectionMemRange[Index].Range.Base;
    ProtectEndAddress  = mProtectionMemRange[Index].Range.Top;
    if (((ProtectBaseAddress & (SIZE_2MB - 1)) != 0) || ((ProtectEndAddress  & (SIZE_2MB - 1)) != 0)) {
      //
      // Check if it is possible to create 4KB-page for not 2MB-aligned range and to create 2MB-page for 2MB-aligned range.
      // A mix of 4KB and 2MB page could save SMRAM space.
      //
      Top2MBAlignedAddress  = ProtectEndAddress & ~(SIZE_2MB - 1);
      Base2MBAlignedAddress = (ProtectBaseAddress + SIZE_2MB - 1) & ~(SIZE_2MB - 1);
      if ((Top2MBAlignedAddress > Base2MBAlignedAddress) &&
          ((Top2MBAlignedAddress - Base2MBAlignedAddress) >= SIZE_2MB))
      {
        //
        // There is an range which could be mapped by 2MB-page.
        //
        High4KBPageSize = ((ProtectEndAddress + SIZE_2MB - 1) & ~(SIZE_2MB - 1)) - (ProtectEndAddress & ~(SIZE_2MB - 1));
        Low4KBPageSize  = ((ProtectBaseAddress + SIZE_2MB - 1) & ~(SIZE_2MB - 1)) - (ProtectBaseAddress & ~(SIZE_2MB - 1));
        if (High4KBPageSize != 0) {
          //
          // Add not 2MB-aligned range to be mapped by 4KB-page.
          //
          mSplitMemRange[NumberOfSpliteRange].Base = ProtectEndAddress & ~(SIZE_2MB - 1);
          mSplitMemRange[NumberOfSpliteRange].Top  = (ProtectEndAddress + SIZE_2MB - 1) & ~(SIZE_2MB - 1);
          NumberOfSpliteRange++;
        }

        if (Low4KBPageSize != 0) {
          //
          // Add not 2MB-aligned range to be mapped by 4KB-page.
          //
          mSplitMemRange[NumberOfSpliteRange].Base = ProtectBaseAddress & ~(SIZE_2MB - 1);
          mSplitMemRange[NumberOfSpliteRange].Top  = (ProtectBaseAddress + SIZE_2MB - 1) & ~(SIZE_2MB - 1);
          NumberOfSpliteRange++;
        }
      } else {
        //
        // The range could only be mapped by 4KB-page.
        //
        mSplitMemRange[NumberOfSpliteRange].Base = ProtectBaseAddress & ~(SIZE_2MB - 1);
        mSplitMemRange[NumberOfSpliteRange].Top  = (ProtectEndAddress + SIZE_2MB - 1) & ~(SIZE_2MB - 1);
        NumberOfSpliteRange++;
      }
    }
  }

  mSplitMemRangeCount = NumberOfSpliteRange;

  //
  // Sort the mProtectionMemRange
  //
  QuickSort (mProtectionMemRange, mProtectionMemRangeCount, sizeof (MEMORY_PROTECTION_RANGE), (BASE_SORT_COMPARE)ProtectionRangeCompare, &MemProtectionRange);

  DEBUG ((DEBUG_INFO, "SMM Profile Memory Ranges:\n"));
  for (Index = 0; Index < mProtectionMemRangeCount; Index++) {
    DEBUG ((DEBUG_INFO, "mProtectionMemRange[%d].Base = %lx\n", Index, mProtectionMemRange[Index].Range.Base));
    DEBUG ((DEBUG_INFO, "mProtectionMemRange[%d].Top  = %lx\n", Index, mProtectionMemRange[Index].Range.Top));
  }

  for (Index = 0; Index < mSplitMemRangeCount; Index++) {
    DEBUG ((DEBUG_INFO, "mSplitMemRange[%d].Base = %lx\n", Index, mSplitMemRange[Index].Base));
    DEBUG ((DEBUG_INFO, "mSplitMemRange[%d].Top  = %lx\n", Index, mSplitMemRange[Index].Top));
  }
}

/**
  This function updates memory attribute according to mProtectionMemRangeCount.

**/
VOID
SmmProfileUpdateMemoryAttributes (
  VOID
  )
{
  RETURN_STATUS  Status;
  UINTN          Index;
  UINTN          PageTable;
  UINT64         Base;
  UINT64         Length;
  UINT64         Limit;
  UINT64         PreviousAddress;
  UINT64         MemoryAttrMask;
  BOOLEAN        WriteProtect;
  BOOLEAN        CetEnabled;

  DEBUG ((DEBUG_INFO, "SmmProfileUpdateMemoryAttributes Start...\n"));

  WRITE_UNPROTECT_RO_PAGES (WriteProtect, CetEnabled);

  PageTable = AsmReadCr3 ();
  Limit     = LShiftU64 (1, mPhysicalAddressBits);

  //
  // [0, 4k] may be non-present.
  //
  PreviousAddress = ((PcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT1) != 0) ? BASE_4KB : 0;

  for (Index = 0; Index < mProtectionMemRangeCount; Index++) {
    MemoryAttrMask = 0;
    if (mProtectionMemRange[Index].Nx == TRUE) {
      MemoryAttrMask = EFI_MEMORY_XP;
    }

    if (mProtectionMemRange[Index].Present == FALSE) {
      MemoryAttrMask = EFI_MEMORY_RP;
    }

    Base   = mProtectionMemRange[Index].Range.Base;
    Length = mProtectionMemRange[Index].Range.Top - Base;
    if (MemoryAttrMask != 0) {
      Status = ConvertMemoryPageAttributes (PageTable, mPagingMode, Base, Length, MemoryAttrMask, TRUE, NULL);
      ASSERT_RETURN_ERROR (Status);
    }

    if (Base > PreviousAddress) {
      //
      // Mark the ranges not in mProtectionMemRange as non-present.
      //
      Status = ConvertMemoryPageAttributes (PageTable, mPagingMode, PreviousAddress, Base - PreviousAddress, EFI_MEMORY_RP, TRUE, NULL);
      ASSERT_RETURN_ERROR (Status);
    }

    PreviousAddress = Base + Length;
  }

  //
  // Set the last remaining range
  //
  if (PreviousAddress < Limit) {
    Status = ConvertMemoryPageAttributes (PageTable, mPagingMode, PreviousAddress, Limit - PreviousAddress, EFI_MEMORY_RP, TRUE, NULL);
    ASSERT_RETURN_ERROR (Status);
  }

  //
  // Flush TLB
  //
  CpuFlushTlb ();

  //
  // Set execute-disable flag
  //
  mXdEnabled = TRUE;

  WRITE_PROTECT_RO_PAGES (WriteProtect, CetEnabled);

  DEBUG ((DEBUG_INFO, "SmmProfileUpdateMemoryAttributes Done.\n"));
}

/**
  Updates page table to make some memory ranges (like system memory) absent
  and make some memory ranges (like MMIO) present and execute disable. It also
  update 2MB-page to 4KB-page for some memory ranges.

**/
VOID
SmmProfileStart (
  VOID
  )
{
  //
  // The flag indicates SMM profile starts to work.
  //
  mSmmProfileStart = TRUE;

  //
  // Tell #PF handler to prepare a #DB subsequently.
  //
  mSetupDebugTrap = TRUE;
}

/**
  Initialize SMM profile in SmmReadyToLock protocol callback function.

  @param  Protocol   Points to the protocol's unique identifier.
  @param  Interface  Points to the interface instance.
  @param  Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS SmmReadyToLock protocol callback runs successfully.
**/
EFI_STATUS
EFIAPI
InitSmmProfileCallBack (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS                 Status;
  EFI_SMM_VARIABLE_PROTOCOL  *SmmProfileVariable;

  //
  // Locate SmmVariableProtocol.
  //
  Status = gMmst->MmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID **)&SmmProfileVariable);
  ASSERT_EFI_ERROR (Status);

  //
  // Save to variable so that SMM profile data can be found.
  //
  SmmProfileVariable->SmmSetVariable (
                        SMM_PROFILE_NAME,
                        &gEfiCallerIdGuid,
                        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                        sizeof (mSmmProfileBase),
                        &mSmmProfileBase
                        );

  return EFI_SUCCESS;
}

/**
  Initialize SMM profile data structures.

**/
VOID
InitSmmProfileInternal (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;
  UINTN       Index;
  UINTN       MsrDsAreaSizePerCpu;
  UINT64      SmmProfileSize;

  mPFEntryCount = (UINTN *)AllocateZeroPool (sizeof (UINTN) * mMaxNumberOfCpus);
  ASSERT (mPFEntryCount != NULL);
  mLastPFEntryValue = (UINT64 (*)[MAX_PF_ENTRY_COUNT])AllocateZeroPool (
                                                        sizeof (mLastPFEntryValue[0]) * mMaxNumberOfCpus
                                                        );
  ASSERT (mLastPFEntryValue != NULL);
  mLastPFEntryPointer = (UINT64 *(*)[MAX_PF_ENTRY_COUNT])AllocateZeroPool (
                                                           sizeof (mLastPFEntryPointer[0]) * mMaxNumberOfCpus
                                                           );
  ASSERT (mLastPFEntryPointer != NULL);

  //
  // Get Smm Profile Base
  //
  mSmmProfileBase = (SMM_PROFILE_HEADER *)(UINTN)GetSmmProfileData (&SmmProfileSize);
  DEBUG ((DEBUG_ERROR, "SmmProfileBase = 0x%016x.\n", (UINTN)mSmmProfileBase));
  DEBUG ((DEBUG_ERROR, "SmmProfileSize = 0x%016x.\n", (UINTN)SmmProfileSize));

  if (mBtsSupported) {
    ASSERT (SmmProfileSize > mMsrDsAreaSize);
    mSmmProfileSize = (UINTN)SmmProfileSize - mMsrDsAreaSize;
  } else {
    mSmmProfileSize = (UINTN)SmmProfileSize;
  }

  ASSERT ((mSmmProfileSize & 0xFFF) == 0);

  //
  // Initialize SMM profile data header.
  //
  mSmmProfileBase->HeaderSize     = sizeof (SMM_PROFILE_HEADER);
  mSmmProfileBase->MaxDataEntries = (UINT64)((mSmmProfileSize - sizeof (SMM_PROFILE_HEADER)) / sizeof (SMM_PROFILE_ENTRY));
  mSmmProfileBase->MaxDataSize    = MultU64x64 (mSmmProfileBase->MaxDataEntries, sizeof (SMM_PROFILE_ENTRY));
  mSmmProfileBase->CurDataEntries = 0;
  mSmmProfileBase->CurDataSize    = 0;
  mSmmProfileBase->TsegStart      = mCpuHotPlugData.SmrrBase;
  mSmmProfileBase->TsegSize       = mCpuHotPlugData.SmrrSize;
  mSmmProfileBase->NumSmis        = 0;
  mSmmProfileBase->NumCpus        = gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;

  if (mBtsSupported) {
    mMsrDsArea = (MSR_DS_AREA_STRUCT **)AllocateZeroPool (sizeof (MSR_DS_AREA_STRUCT *) * mMaxNumberOfCpus);
    ASSERT (mMsrDsArea != NULL);
    mMsrBTSRecord = (BRANCH_TRACE_RECORD **)AllocateZeroPool (sizeof (BRANCH_TRACE_RECORD *) * mMaxNumberOfCpus);
    ASSERT (mMsrBTSRecord != NULL);
    mMsrPEBSRecord = (PEBS_RECORD **)AllocateZeroPool (sizeof (PEBS_RECORD *) * mMaxNumberOfCpus);
    ASSERT (mMsrPEBSRecord != NULL);

    mMsrDsAreaBase      = (MSR_DS_AREA_STRUCT *)((UINTN)mSmmProfileBase + mSmmProfileSize);
    MsrDsAreaSizePerCpu = mMsrDsAreaSize / mMaxNumberOfCpus;
    mBTSRecordNumber    = (MsrDsAreaSizePerCpu - sizeof (PEBS_RECORD) * PEBS_RECORD_NUMBER - sizeof (MSR_DS_AREA_STRUCT)) / sizeof (BRANCH_TRACE_RECORD);
    for (Index = 0; Index < mMaxNumberOfCpus; Index++) {
      mMsrDsArea[Index]     = (MSR_DS_AREA_STRUCT *)((UINTN)mMsrDsAreaBase + MsrDsAreaSizePerCpu * Index);
      mMsrBTSRecord[Index]  = (BRANCH_TRACE_RECORD *)((UINTN)mMsrDsArea[Index] + sizeof (MSR_DS_AREA_STRUCT));
      mMsrPEBSRecord[Index] = (PEBS_RECORD *)((UINTN)mMsrDsArea[Index] + MsrDsAreaSizePerCpu - sizeof (PEBS_RECORD) * PEBS_RECORD_NUMBER);

      mMsrDsArea[Index]->BTSBufferBase         = (UINTN)mMsrBTSRecord[Index];
      mMsrDsArea[Index]->BTSIndex              = mMsrDsArea[Index]->BTSBufferBase;
      mMsrDsArea[Index]->BTSAbsoluteMaximum    = mMsrDsArea[Index]->BTSBufferBase + mBTSRecordNumber * sizeof (BRANCH_TRACE_RECORD) + 1;
      mMsrDsArea[Index]->BTSInterruptThreshold = mMsrDsArea[Index]->BTSAbsoluteMaximum + 1;

      mMsrDsArea[Index]->PEBSBufferBase         = (UINTN)mMsrPEBSRecord[Index];
      mMsrDsArea[Index]->PEBSIndex              = mMsrDsArea[Index]->PEBSBufferBase;
      mMsrDsArea[Index]->PEBSAbsoluteMaximum    = mMsrDsArea[Index]->PEBSBufferBase + PEBS_RECORD_NUMBER * sizeof (PEBS_RECORD) + 1;
      mMsrDsArea[Index]->PEBSInterruptThreshold = mMsrDsArea[Index]->PEBSAbsoluteMaximum + 1;
    }
  }

  mProtectionMemRange      = mProtectionMemRangeTemplate;
  mProtectionMemRangeCount = sizeof (mProtectionMemRangeTemplate) / sizeof (MEMORY_PROTECTION_RANGE);

  //
  // Update TSeg entry.
  //
  mProtectionMemRange[0].Range.Base = mCpuHotPlugData.SmrrBase;
  mProtectionMemRange[0].Range.Top  = mCpuHotPlugData.SmrrBase + mCpuHotPlugData.SmrrSize;

  //
  // Update SMM profile entry.
  //
  mProtectionMemRange[1].Range.Base = (EFI_PHYSICAL_ADDRESS)(UINTN)mSmmProfileBase;
  mProtectionMemRange[1].Range.Top  = (EFI_PHYSICAL_ADDRESS)(UINTN)mSmmProfileBase + SmmProfileSize;

  //
  // Allocate memory reserved for creating 4KB pages.
  //
  InitPagesForPFHandler ();

  //
  // Start SMM profile when SmmReadyToLock protocol is installed.
  //
  Status = gMmst->MmRegisterProtocolNotify (
                    &gEfiSmmReadyToLockProtocolGuid,
                    InitSmmProfileCallBack,
                    &Registration
                    );
  ASSERT_EFI_ERROR (Status);

  return;
}

/**
  Check if feature is supported by a processor.

  @param CpuIndex        The index of the CPU.
**/
VOID
CheckFeatureSupported (
  IN UINTN  CpuIndex
  )
{
  UINT32                         RegEax;
  UINT32                         RegEcx;
  UINT32                         RegEdx;
  MSR_IA32_MISC_ENABLE_REGISTER  MiscEnableMsr;

  if ((PcdGet32 (PcdControlFlowEnforcementPropertyMask) != 0) && mCetSupported) {
    AsmCpuid (CPUID_SIGNATURE, &RegEax, NULL, NULL, NULL);
    if (RegEax >= CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS) {
      AsmCpuidEx (CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS, CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO, NULL, NULL, &RegEcx, NULL);
      if ((RegEcx & CPUID_CET_SS) == 0) {
        mCetSupported = FALSE;
        PatchInstructionX86 (mPatchCetSupported, mCetSupported, 1);
      }
    } else {
      mCetSupported = FALSE;
      PatchInstructionX86 (mPatchCetSupported, mCetSupported, 1);
    }
  }

  if (mBtsSupported) {
    AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &RegEdx);
    if ((RegEdx & CPUID1_EDX_BTS_AVAILABLE) != 0) {
      //
      // Per IA32 manuals:
      // When CPUID.1:EDX[21] is set, the following BTS facilities are available:
      // 1. The BTS_UNAVAILABLE flag in the IA32_MISC_ENABLE MSR indicates the
      //    availability of the BTS facilities, including the ability to set the BTS and
      //    BTINT bits in the MSR_DEBUGCTLA MSR.
      // 2. The IA32_DS_AREA MSR can be programmed to point to the DS save area.
      //
      MiscEnableMsr.Uint64 = AsmReadMsr64 (MSR_IA32_MISC_ENABLE);
      if (MiscEnableMsr.Bits.BTS == 1) {
        //
        // BTS facilities is not supported if MSR_IA32_MISC_ENABLE.BTS bit is set.
        //
        mBtsSupported = FALSE;
      }
    }
  }

  if (mSmmCodeAccessCheckEnable) {
    //
    // Check to see if the CPU supports the SMM Code Access Check feature
    // Do not access this MSR unless the CPU supports the SmmRegFeatureControl
    //
    if (!SmmCpuFeaturesIsSmmRegisterSupported (CpuIndex, SmmRegFeatureControl) ||
        ((AsmReadMsr64 (EFI_MSR_SMM_MCA_CAP) & SMM_CODE_ACCESS_CHK_BIT) == 0))
    {
      mSmmCodeAccessCheckEnable = FALSE;
    }
  }
}

/**
  Enable single step.

**/
VOID
ActivateSingleStepDB (
  VOID
  )
{
  UINTN  Dr6;

  Dr6 = AsmReadDr6 ();
  if ((Dr6 & DR6_SINGLE_STEP) != 0) {
    return;
  }

  Dr6 |= DR6_SINGLE_STEP;
  AsmWriteDr6 (Dr6);
}

/**
  Enable last branch.

**/
VOID
ActivateLBR (
  VOID
  )
{
  UINT64  DebugCtl;

  DebugCtl = AsmReadMsr64 (MSR_DEBUG_CTL);
  if ((DebugCtl & MSR_DEBUG_CTL_LBR) != 0) {
    return;
  }

  DebugCtl |= MSR_DEBUG_CTL_LBR;
  AsmWriteMsr64 (MSR_DEBUG_CTL, DebugCtl);
}

/**
  Enable branch trace store.

  @param  CpuIndex  The index of the processor.

**/
VOID
ActivateBTS (
  IN      UINTN  CpuIndex
  )
{
  UINT64  DebugCtl;

  DebugCtl = AsmReadMsr64 (MSR_DEBUG_CTL);
  if ((DebugCtl & MSR_DEBUG_CTL_BTS) != 0) {
    return;
  }

  AsmWriteMsr64 (MSR_DS_AREA, (UINT64)(UINTN)mMsrDsArea[CpuIndex]);
  DebugCtl |= (UINT64)(MSR_DEBUG_CTL_BTS | MSR_DEBUG_CTL_TR);
  DebugCtl &= ~((UINT64)MSR_DEBUG_CTL_BTINT);
  AsmWriteMsr64 (MSR_DEBUG_CTL, DebugCtl);
}

/**
  Increase SMI number in each SMI entry.

**/
VOID
SmmProfileRecordSmiNum (
  VOID
  )
{
  if (mSmmProfileStart) {
    mSmmProfileBase->NumSmis++;
  }
}

/**
  Initialize processor environment for SMM profile.

  @param  CpuIndex  The index of the processor.

**/
VOID
ActivateSmmProfile (
  IN UINTN  CpuIndex
  )
{
  //
  // Enable Single Step DB#
  //
  ActivateSingleStepDB ();

  if (mBtsSupported) {
    //
    // We can not get useful information from LER, so we have to use BTS.
    //
    ActivateLBR ();

    //
    // Enable BTS
    //
    ActivateBTS (CpuIndex);
  }
}

/**
  Initialize SMM profile in SMM CPU entry point.

  @param[in] Cr3  The base address of the page tables to use in SMM.

**/
VOID
InitSmmProfile (
  UINT32  Cr3
  )
{
  //
  // Save Cr3
  //
  mSmmProfileCr3 = Cr3;

  //
  // Skip SMM profile initialization if feature is disabled
  //
  if (!mSmmProfileEnabled &&
      !HEAP_GUARD_NONSTOP_MODE &&
      !NULL_DETECTION_NONSTOP_MODE)
  {
    return;
  }

  //
  // Initialize SmmProfile here
  //
  InitSmmProfileInternal ();

  //
  // Initialize profile IDT.
  //
  InitIdtr ();
}

/**
  Update page table to map the memory correctly in order to make the instruction
  which caused page fault execute successfully. And it also save the original page
  table to be restored in single-step exception.

  @param  PageTable           PageTable Address.
  @param  PFAddress           The memory address which caused page fault exception.
  @param  CpuIndex            The index of the processor.
  @param  ErrorCode           The Error code of exception.

**/
VOID
RestorePageTableBelow4G (
  UINT64  *PageTable,
  UINT64  PFAddress,
  UINTN   CpuIndex,
  UINTN   ErrorCode
  )
{
  UINTN     PTIndex;
  UINTN     PFIndex;
  IA32_CR4  Cr4;
  BOOLEAN   Enable5LevelPaging;

  Cr4.UintN          = AsmReadCr4 ();
  Enable5LevelPaging = (BOOLEAN)(Cr4.Bits.LA57 == 1);

  //
  // PML5
  //
  if (Enable5LevelPaging) {
    PTIndex = (UINTN)BitFieldRead64 (PFAddress, 48, 56);
    ASSERT (PageTable[PTIndex] != 0);
    PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);
  }

  //
  // PML4
  //
  if (sizeof (UINT64) == sizeof (UINTN)) {
    PTIndex = (UINTN)BitFieldRead64 (PFAddress, 39, 47);
    ASSERT (PageTable[PTIndex] != 0);
    PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);
  }

  //
  // PDPTE
  //
  PTIndex = (UINTN)BitFieldRead64 (PFAddress, 30, 38);

  if ((PageTable[PTIndex] & IA32_PG_P) == 0) {
    //
    // For 32-bit case, because a full map page table for 0-4G is created by default,
    // and since the PDPTE must be one non-leaf entry, the PDPTE must always be present.
    // So, ASSERT it must be the 64-bit case running here.
    //
    ASSERT (sizeof (UINT64) == sizeof (UINTN));

    //
    // If the entry is not present, allocate one page from page pool for it
    //
    PageTable[PTIndex] = AllocPage () | mAddressEncMask | PAGE_ATTRIBUTE_BITS;
  }

  ASSERT (PageTable[PTIndex] != 0);
  PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);

  //
  // PD
  //
  PTIndex = (UINTN)BitFieldRead64 (PFAddress, 21, 29);
  if ((PageTable[PTIndex] & IA32_PG_P) == 0) {
    //
    // A 2M page size will be used directly when the 2M entry is marked as non-present.
    //

    //
    // Record old entries with non-present status
    // Old entries include the memory which instruction is at and the memory which instruction access.
    //
    //
    ASSERT (mPFEntryCount[CpuIndex] < MAX_PF_ENTRY_COUNT);
    if (mPFEntryCount[CpuIndex] < MAX_PF_ENTRY_COUNT) {
      PFIndex                                = mPFEntryCount[CpuIndex];
      mLastPFEntryValue[CpuIndex][PFIndex]   = PageTable[PTIndex];
      mLastPFEntryPointer[CpuIndex][PFIndex] = &PageTable[PTIndex];
      mPFEntryCount[CpuIndex]++;
    }

    //
    // Set new entry
    //
    PageTable[PTIndex]  = (PFAddress & ~((1ull << 21) - 1));
    PageTable[PTIndex] |= (UINT64)IA32_PG_PS;
    PageTable[PTIndex] |= (UINT64)PAGE_ATTRIBUTE_BITS;
    if ((ErrorCode & IA32_PF_EC_ID) != 0) {
      PageTable[PTIndex] &= ~IA32_PG_NX;
    }
  } else {
    //
    // If the 2M entry is marked as present, a 4K page size will be utilized.
    // In this scenario, the 2M entry must be a non-leaf entry.
    //
    ASSERT (PageTable[PTIndex] != 0);
    PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & PHYSICAL_ADDRESS_MASK);

    //
    // 4K PTE
    //
    PTIndex = (UINTN)BitFieldRead64 (PFAddress, 12, 20);

    //
    // Record old entries with non-present status
    // Old entries include the memory which instruction is at and the memory which instruction access.
    //
    //
    ASSERT (mPFEntryCount[CpuIndex] < MAX_PF_ENTRY_COUNT);
    if (mPFEntryCount[CpuIndex] < MAX_PF_ENTRY_COUNT) {
      PFIndex                                = mPFEntryCount[CpuIndex];
      mLastPFEntryValue[CpuIndex][PFIndex]   = PageTable[PTIndex];
      mLastPFEntryPointer[CpuIndex][PFIndex] = &PageTable[PTIndex];
      mPFEntryCount[CpuIndex]++;
    }

    //
    // Set new entry
    //
    PageTable[PTIndex]  = (PFAddress & ~((1ull << 12) - 1));
    PageTable[PTIndex] |= (UINT64)PAGE_ATTRIBUTE_BITS;
    if ((ErrorCode & IA32_PF_EC_ID) != 0) {
      PageTable[PTIndex] &= ~IA32_PG_NX;
    }
  }
}

/**
  Handler for Page Fault triggered by Guard page.

  @param  ErrorCode  The Error code of exception.

**/
VOID
GuardPagePFHandler (
  UINTN  ErrorCode
  )
{
  UINT64  *PageTable;
  UINT64  PFAddress;
  UINT64  RestoreAddress;
  UINTN   RestorePageNumber;
  UINTN   CpuIndex;

  PageTable = (UINT64 *)AsmReadCr3 ();
  PFAddress = AsmReadCr2 ();
  CpuIndex  = GetCpuIndex ();

  //
  // Memory operation cross pages, like "rep mov" instruction, will cause
  // infinite loop between this and Debug Trap handler. We have to make sure
  // that current page and the page followed are both in PRESENT state.
  //
  RestorePageNumber = 2;
  RestoreAddress    = PFAddress;
  while (RestorePageNumber > 0) {
    RestorePageTableBelow4G (PageTable, RestoreAddress, CpuIndex, ErrorCode);
    RestoreAddress += EFI_PAGE_SIZE;
    RestorePageNumber--;
  }

  //
  // Flush TLB
  //
  CpuFlushTlb ();
}

/**
  The Page fault handler to save SMM profile data.

  @param  Rip        The RIP when exception happens.
  @param  ErrorCode  The Error code of exception.

**/
VOID
SmmProfilePFHandler (
  UINTN  Rip,
  UINTN  ErrorCode
  )
{
  UINT64                      *PageTable;
  UINT64                      PFAddress;
  UINT64                      RestoreAddress;
  UINTN                       RestorePageNumber;
  UINTN                       CpuIndex;
  UINTN                       Index;
  UINT64                      InstructionAddress;
  UINTN                       MaxEntryNumber;
  UINTN                       CurrentEntryNumber;
  BOOLEAN                     IsValidPFAddress;
  SMM_PROFILE_ENTRY           *SmmProfileEntry;
  UINT64                      SmiCommand;
  EFI_STATUS                  Status;
  UINT8                       SoftSmiValue;
  EFI_SMM_SAVE_STATE_IO_INFO  IoInfo;

  if (mBtsSupported) {
    DisableBTS ();
  }

  IsValidPFAddress = FALSE;
  PageTable        = (UINT64 *)AsmReadCr3 ();
  PFAddress        = AsmReadCr2 ();
  CpuIndex         = GetCpuIndex ();

  //
  // Memory operation cross pages, like "rep mov" instruction, will cause
  // infinite loop between this and Debug Trap handler. We have to make sure
  // that current page and the page followed are both in PRESENT state.
  //
  RestorePageNumber = 2;
  RestoreAddress    = PFAddress;
  while (RestorePageNumber > 0) {
    if (RestoreAddress <= 0xFFFFFFFF) {
      RestorePageTableBelow4G (PageTable, RestoreAddress, CpuIndex, ErrorCode);
    } else {
      RestorePageTableAbove4G (PageTable, RestoreAddress, CpuIndex, ErrorCode, &IsValidPFAddress);
    }

    RestoreAddress += EFI_PAGE_SIZE;
    RestorePageNumber--;
  }

  if (!IsValidPFAddress) {
    InstructionAddress = Rip;
    if (((ErrorCode & IA32_PF_EC_ID) != 0) && (mBtsSupported)) {
      //
      // If it is instruction fetch failure, get the correct IP from BTS.
      //
      InstructionAddress = GetSourceFromDestinationOnBts (CpuIndex, Rip);
      if (InstructionAddress == 0) {
        //
        // It indicates the instruction which caused page fault is not a jump instruction,
        // set instruction address same as the page fault address.
        //
        InstructionAddress = PFAddress;
      }
    }

    //
    // Indicate it is not software SMI
    //
    SmiCommand = 0xFFFFFFFFFFFFFFFFULL;
    for (Index = 0; Index < gMmst->NumberOfCpus; Index++) {
      Status = SmmReadSaveState (&mSmmCpu, sizeof (IoInfo), EFI_SMM_SAVE_STATE_REGISTER_IO, Index, &IoInfo);
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (IoInfo.IoPort == mSmiCommandPort) {
        //
        // A software SMI triggered by SMI command port has been found, get SmiCommand from SMI command port.
        //
        SoftSmiValue = IoRead8 (mSmiCommandPort);
        SmiCommand   = (UINT64)SoftSmiValue;
        break;
      }
    }

    SmmProfileEntry = (SMM_PROFILE_ENTRY *)(UINTN)(mSmmProfileBase + 1);
    //
    // Check if there is already a same entry in profile data.
    //
    for (Index = 0; Index < (UINTN)mSmmProfileBase->CurDataEntries; Index++) {
      if ((SmmProfileEntry[Index].ErrorCode   == (UINT64)ErrorCode) &&
          (SmmProfileEntry[Index].Address     == PFAddress) &&
          (SmmProfileEntry[Index].CpuNum      == (UINT64)CpuIndex) &&
          (SmmProfileEntry[Index].Instruction == InstructionAddress) &&
          (SmmProfileEntry[Index].SmiCmd      == SmiCommand))
      {
        //
        // Same record exist, need not save again.
        //
        break;
      }
    }

    if (Index == mSmmProfileBase->CurDataEntries) {
      CurrentEntryNumber = (UINTN)mSmmProfileBase->CurDataEntries;
      MaxEntryNumber     = (UINTN)mSmmProfileBase->MaxDataEntries;
      if (FeaturePcdGet (PcdCpuSmmProfileRingBuffer)) {
        CurrentEntryNumber = CurrentEntryNumber % MaxEntryNumber;
      }

      if (CurrentEntryNumber < MaxEntryNumber) {
        //
        // Log the new entry
        //
        SmmProfileEntry[CurrentEntryNumber].SmiNum      = mSmmProfileBase->NumSmis;
        SmmProfileEntry[CurrentEntryNumber].ErrorCode   = (UINT64)ErrorCode;
        SmmProfileEntry[CurrentEntryNumber].ApicId      = (UINT64)GetApicId ();
        SmmProfileEntry[CurrentEntryNumber].CpuNum      = (UINT64)CpuIndex;
        SmmProfileEntry[CurrentEntryNumber].Address     = PFAddress;
        SmmProfileEntry[CurrentEntryNumber].Instruction = InstructionAddress;
        SmmProfileEntry[CurrentEntryNumber].SmiCmd      = SmiCommand;
        //
        // Update current entry index and data size in the header.
        //
        mSmmProfileBase->CurDataEntries++;
        mSmmProfileBase->CurDataSize = MultU64x64 (mSmmProfileBase->CurDataEntries, sizeof (SMM_PROFILE_ENTRY));
      }
    }
  }

  //
  // Flush TLB
  //
  CpuFlushTlb ();

  if (mBtsSupported) {
    EnableBTS ();
  }
}

/**
  Replace INT1 exception handler to restore page table to absent/execute-disable state
  in order to trigger page fault again to save SMM profile data..

**/
VOID
InitIdtr (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = SmmRegisterExceptionHandler (&mSmmCpuService, EXCEPT_IA32_DEBUG, DebugExceptionHandler);
  ASSERT_EFI_ERROR (Status);
}
