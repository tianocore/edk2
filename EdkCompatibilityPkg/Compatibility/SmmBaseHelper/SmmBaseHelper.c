/** @file
  SMM Base Helper SMM driver.

  This driver is the counterpart of the SMM Base On SMM Base2 Thunk driver. It
  provides helping services in SMM to the SMM Base On SMM Base2 Thunk driver.

  Caution: This module requires additional review when modified.
  This driver will have external input - communicate buffer in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  SmmHandlerEntry() will receive untrusted input and do validation.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiSmm.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/DevicePathLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/CpuLib.h>
#include <Guid/SmmBaseThunkCommunication.h>
#include <Protocol/SmmBaseHelperReady.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SmmCpuSaveState.h>
#include <Protocol/MpService.h>
#include <Protocol/LoadPe32Image.h>
#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmAccess2.h>

/**
  Register SMM image to SMRAM profile.

  @param[in] FilePath           File path of the image.
  @param[in] ImageBuffer        Image base address.
  @param[in] NumberOfPage       Number of page.

  @retval TRUE                  Register success.
  @retval FALSE                 Register fail.

**/
BOOLEAN
RegisterSmramProfileImage (
  IN EFI_DEVICE_PATH_PROTOCOL   *FilePath,
  IN PHYSICAL_ADDRESS           ImageBuffer,
  IN UINTN                      NumberOfPage
  );

/**
  Unregister SMM image from SMRAM profile.

  @param[in] FilePath           File path of the image.
  @param[in] ImageBuffer        Image base address.
  @param[in] NumberOfPage       Number of page.

  @retval TRUE                  Unregister success.
  @retval FALSE                 Unregister fail.

**/
BOOLEAN
UnregisterSmramProfileImage (
  IN EFI_DEVICE_PATH_PROTOCOL   *FilePath,
  IN PHYSICAL_ADDRESS           ImageBuffer,
  IN UINTN                      NumberOfPage
  );

///
/// Structure for tracking paired information of registered Framework SMI handler
/// and correpsonding dispatch handle for SMI handler thunk.
///
typedef struct {
  LIST_ENTRY                    Link;
  EFI_HANDLE                    DispatchHandle;
  EFI_HANDLE                    SmmImageHandle;
  EFI_SMM_CALLBACK_ENTRY_POINT  CallbackAddress;
  VOID                          *CommunicationBuffer;
  UINTN                         *SourceSize;
} CALLBACK_INFO;

typedef struct {
  ///
  /// PI SMM CPU Save State register index
  ///
  EFI_SMM_SAVE_STATE_REGISTER   Register;
  ///
  /// Offset in Framework SMST
  ///
  UINTN                         Offset;
} CPU_SAVE_STATE_CONVERSION;

#define CPU_SAVE_STATE_GET_OFFSET(Field)  (UINTN)(&(((EFI_SMM_CPU_SAVE_STATE *) 0)->Ia32SaveState.Field))


EFI_HANDLE                         mDispatchHandle;
EFI_SMM_CPU_PROTOCOL               *mSmmCpu;
EFI_PE32_IMAGE_PROTOCOL            *mLoadPe32Image;
EFI_GUID                           mEfiSmmCpuIoGuid = EFI_SMM_CPU_IO_GUID;
EFI_SMM_BASE_HELPER_READY_PROTOCOL *mSmmBaseHelperReady;
EFI_SMM_SYSTEM_TABLE               *mFrameworkSmst;
UINTN                              mNumberOfProcessors;
BOOLEAN                            mLocked = FALSE;
BOOLEAN                            mPageTableHookEnabled;
BOOLEAN                            mHookInitialized;
UINT64                             *mCpuStatePageTable;
SPIN_LOCK                          mPFLock;
UINT64                             mPhyMask;
VOID                               *mOriginalHandler;
EFI_SMM_CPU_SAVE_STATE             *mShadowSaveState;
EFI_SMRAM_DESCRIPTOR               *mSmramRanges;
UINTN                              mSmramRangeCount;

LIST_ENTRY mCallbackInfoListHead = INITIALIZE_LIST_HEAD_VARIABLE (mCallbackInfoListHead);

CPU_SAVE_STATE_CONVERSION mCpuSaveStateConvTable[] = {
  {EFI_SMM_SAVE_STATE_REGISTER_LDTBASE  , CPU_SAVE_STATE_GET_OFFSET(LDTBase)},
  {EFI_SMM_SAVE_STATE_REGISTER_ES       , CPU_SAVE_STATE_GET_OFFSET(ES)},
  {EFI_SMM_SAVE_STATE_REGISTER_CS       , CPU_SAVE_STATE_GET_OFFSET(CS)},
  {EFI_SMM_SAVE_STATE_REGISTER_SS       , CPU_SAVE_STATE_GET_OFFSET(SS)},
  {EFI_SMM_SAVE_STATE_REGISTER_DS       , CPU_SAVE_STATE_GET_OFFSET(DS)},
  {EFI_SMM_SAVE_STATE_REGISTER_FS       , CPU_SAVE_STATE_GET_OFFSET(FS)},
  {EFI_SMM_SAVE_STATE_REGISTER_GS       , CPU_SAVE_STATE_GET_OFFSET(GS)},
  {EFI_SMM_SAVE_STATE_REGISTER_TR_SEL   , CPU_SAVE_STATE_GET_OFFSET(TR)},
  {EFI_SMM_SAVE_STATE_REGISTER_DR7      , CPU_SAVE_STATE_GET_OFFSET(DR7)},
  {EFI_SMM_SAVE_STATE_REGISTER_DR6      , CPU_SAVE_STATE_GET_OFFSET(DR6)},
  {EFI_SMM_SAVE_STATE_REGISTER_RAX      , CPU_SAVE_STATE_GET_OFFSET(EAX)},
  {EFI_SMM_SAVE_STATE_REGISTER_RBX      , CPU_SAVE_STATE_GET_OFFSET(EBX)},
  {EFI_SMM_SAVE_STATE_REGISTER_RCX      , CPU_SAVE_STATE_GET_OFFSET(ECX)},
  {EFI_SMM_SAVE_STATE_REGISTER_RDX      , CPU_SAVE_STATE_GET_OFFSET(EDX)},
  {EFI_SMM_SAVE_STATE_REGISTER_RSP      , CPU_SAVE_STATE_GET_OFFSET(ESP)},
  {EFI_SMM_SAVE_STATE_REGISTER_RBP      , CPU_SAVE_STATE_GET_OFFSET(EBP)},
  {EFI_SMM_SAVE_STATE_REGISTER_RSI      , CPU_SAVE_STATE_GET_OFFSET(ESI)},
  {EFI_SMM_SAVE_STATE_REGISTER_RDI      , CPU_SAVE_STATE_GET_OFFSET(EDI)},
  {EFI_SMM_SAVE_STATE_REGISTER_RIP      , CPU_SAVE_STATE_GET_OFFSET(EIP)},
  {EFI_SMM_SAVE_STATE_REGISTER_RFLAGS   , CPU_SAVE_STATE_GET_OFFSET(EFLAGS)},
  {EFI_SMM_SAVE_STATE_REGISTER_CR0      , CPU_SAVE_STATE_GET_OFFSET(CR0)},
  {EFI_SMM_SAVE_STATE_REGISTER_CR3      , CPU_SAVE_STATE_GET_OFFSET(CR3)}
};

/**
  Page fault handler.

**/
VOID
PageFaultHandlerHook (
  VOID
  );

/**
  Read CpuSaveStates from PI for Framework use.

  The function reads PI style CpuSaveStates of CpuIndex-th CPU for Framework driver use. If
  ToRead is specified, the CpuSaveStates will be copied to ToRead, otherwise copied to
  mFrameworkSmst->CpuSaveState[CpuIndex].

  @param[in]      CpuIndex        The zero-based CPU index.
  @param[in, out] ToRead          If not NULL, CpuSaveStates will be copied to it.

**/
VOID
ReadCpuSaveState (
  IN     UINTN                   CpuIndex,
  IN OUT EFI_SMM_CPU_SAVE_STATE  *ToRead
  )
{
  EFI_STATUS Status;
  UINTN Index;
  EFI_SMM_CPU_STATE *State;
  EFI_SMI_CPU_SAVE_STATE *SaveState;

  State = (EFI_SMM_CPU_STATE *)gSmst->CpuSaveState[CpuIndex];
  if (ToRead != NULL) {
    SaveState = &ToRead->Ia32SaveState;
  } else {
    SaveState = &mFrameworkSmst->CpuSaveState[CpuIndex].Ia32SaveState;
  }

  //
  // Note that SMBASE/SMMRevId/IORestart/AutoHALTRestart are in same location in IA32 and X64 CPU Save State Map.
  //
  SaveState->SMBASE = State->x86.SMBASE;
  SaveState->SMMRevId = State->x86.SMMRevId;
  SaveState->IORestart = State->x86.IORestart;
  SaveState->AutoHALTRestart = State->x86.AutoHALTRestart;

  for (Index = 0; Index < sizeof (mCpuSaveStateConvTable) / sizeof (CPU_SAVE_STATE_CONVERSION); Index++) {
    ///
    /// Try to use SMM CPU Protocol to access CPU save states if possible
    ///
    Status = mSmmCpu->ReadSaveState (
                        mSmmCpu,
                        (UINTN)sizeof (UINT32),
                        mCpuSaveStateConvTable[Index].Register,
                        CpuIndex,
                        ((UINT8 *)SaveState) + mCpuSaveStateConvTable[Index].Offset
                        );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Write CpuSaveStates from Framework into PI.

  The function writes back CpuSaveStates of CpuIndex-th CPU from PI to Framework. If
  ToWrite is specified, it contains the CpuSaveStates to write from, otherwise CpuSaveStates
  to write from mFrameworkSmst->CpuSaveState[CpuIndex].

  @param[in] CpuIndex      The zero-based CPU index.
  @param[in] ToWrite       If not NULL, CpuSaveStates to write from.

**/
VOID
WriteCpuSaveState (
  IN UINTN                   CpuIndex,
  IN EFI_SMM_CPU_SAVE_STATE  *ToWrite
  )
{
  EFI_STATUS             Status;
  UINTN                  Index;
  EFI_SMM_CPU_STATE      *State;
  EFI_SMI_CPU_SAVE_STATE *SaveState;

  State = (EFI_SMM_CPU_STATE *)gSmst->CpuSaveState[CpuIndex];

  if (ToWrite != NULL) {
    SaveState = &ToWrite->Ia32SaveState;
  } else {
    SaveState = &mFrameworkSmst->CpuSaveState[CpuIndex].Ia32SaveState;
  }

  //
  // SMMRevId is read-only.
  // Note that SMBASE/IORestart/AutoHALTRestart are in same location in IA32 and X64 CPU Save State Map.
  //
  State->x86.SMBASE = SaveState->SMBASE;
  State->x86.IORestart = SaveState->IORestart;
  State->x86.AutoHALTRestart = SaveState->AutoHALTRestart;
  
  for (Index = 0; Index < sizeof (mCpuSaveStateConvTable) / sizeof (CPU_SAVE_STATE_CONVERSION); Index++) {
    Status = mSmmCpu->WriteSaveState (
                        mSmmCpu,
                        (UINTN)sizeof (UINT32),
                        mCpuSaveStateConvTable[Index].Register,
                        CpuIndex,
                        ((UINT8 *)SaveState) + 
                        mCpuSaveStateConvTable[Index].Offset
                        );
  }
}

/**
  Read or write a page that contains CpuSaveStates. Read is from PI to Framework.
  Write is from Framework to PI.

  This function reads or writes a page that contains CpuSaveStates. The page contains Framework
  CpuSaveStates. On read, it reads PI style CpuSaveStates and fill the page up. On write, it
  writes back from the page content to PI CpuSaveStates struct.
  The first Framework CpuSaveStates (for CPU 0) is from mFrameworkSmst->CpuSaveState which is
  page aligned. Because Framework CpuSaveStates are continuous, we can know which CPUs' SaveStates
  are in the page start from PageAddress.

  @param[in] PageAddress   The base address for a page.
  @param[in] IsRead        TRUE for Read, FALSE for Write.

**/
VOID
ReadWriteCpuStatePage (
  IN UINT64  PageAddress,
  IN BOOLEAN IsRead
  )
{
  UINTN          FirstSSIndex;   // Index of first CpuSaveState in the page
  UINTN          LastSSIndex;    // Index of last CpuSaveState in the page
  BOOLEAN        FirstSSAligned; // Whether first CpuSaveState is page-aligned
  BOOLEAN        LastSSAligned;  // Whether the end of last CpuSaveState is page-aligned
  UINTN          ClippedSize;
  UINTN          CpuIndex;

  FirstSSIndex = ((UINTN)PageAddress - (UINTN)mFrameworkSmst->CpuSaveState) / sizeof (EFI_SMM_CPU_SAVE_STATE);
  FirstSSAligned = TRUE;
  if (((UINTN)PageAddress - (UINTN)mFrameworkSmst->CpuSaveState) % sizeof (EFI_SMM_CPU_SAVE_STATE) != 0) {
    FirstSSIndex++;
    FirstSSAligned = FALSE;
  }
  LastSSIndex = ((UINTN)PageAddress + SIZE_4KB - (UINTN)mFrameworkSmst->CpuSaveState - 1) / sizeof (EFI_SMM_CPU_SAVE_STATE);
  LastSSAligned = TRUE;
  if (((UINTN)PageAddress + SIZE_4KB - (UINTN)mFrameworkSmst->CpuSaveState) % sizeof (EFI_SMM_CPU_SAVE_STATE) != 0) {
    LastSSIndex--;
    LastSSAligned = FALSE;
  }
  for (CpuIndex = FirstSSIndex; CpuIndex <= LastSSIndex && CpuIndex < mNumberOfProcessors; CpuIndex++) {
    if (IsRead) {
      ReadCpuSaveState (CpuIndex, NULL);
    } else {
      WriteCpuSaveState (CpuIndex, NULL);
    }
  }
  if (!FirstSSAligned) {
    ReadCpuSaveState (FirstSSIndex - 1, mShadowSaveState);
    ClippedSize = (UINTN)&mFrameworkSmst->CpuSaveState[FirstSSIndex] & (SIZE_4KB - 1);
    if (IsRead) {
      CopyMem ((VOID*)(UINTN)PageAddress, (VOID*)((UINTN)(mShadowSaveState + 1) - ClippedSize), ClippedSize);
    } else {
      CopyMem ((VOID*)((UINTN)(mShadowSaveState + 1) - ClippedSize), (VOID*)(UINTN)PageAddress, ClippedSize);
      WriteCpuSaveState (FirstSSIndex - 1, mShadowSaveState);
    }
  }
  if (!LastSSAligned && LastSSIndex + 1 < mNumberOfProcessors) {
    ReadCpuSaveState (LastSSIndex + 1, mShadowSaveState);
    ClippedSize = SIZE_4KB - ((UINTN)&mFrameworkSmst->CpuSaveState[LastSSIndex + 1] & (SIZE_4KB - 1));
    if (IsRead) {
      CopyMem (&mFrameworkSmst->CpuSaveState[LastSSIndex + 1], mShadowSaveState, ClippedSize);
    } else {
      CopyMem (mShadowSaveState, &mFrameworkSmst->CpuSaveState[LastSSIndex + 1], ClippedSize);
      WriteCpuSaveState (LastSSIndex + 1, mShadowSaveState);
    }
  }
}

/**
  The page fault handler that on-demand read PI CpuSaveStates for framework use. If the fault
  is not targeted to mFrameworkSmst->CpuSaveState range, the function will return FALSE to let
  PageFaultHandlerHook know it needs to pass the fault over to original page fault handler.
  
  @retval TRUE     The page fault is correctly handled.
  @retval FALSE    The page fault is not handled and is passed through to original handler.

**/
BOOLEAN
PageFaultHandler (
  VOID
  )
{
  BOOLEAN        IsHandled;
  UINT64         *PageTable;
  UINT64         PFAddress;
  UINTN          NumCpuStatePages;
  
  ASSERT (mPageTableHookEnabled);
  AcquireSpinLock (&mPFLock);

  PageTable = (UINT64*)(UINTN)(AsmReadCr3 () & mPhyMask);
  PFAddress = AsmReadCr2 ();
  NumCpuStatePages = EFI_SIZE_TO_PAGES (mNumberOfProcessors * sizeof (EFI_SMM_CPU_SAVE_STATE));
  IsHandled = FALSE;
  if (((UINTN)mFrameworkSmst->CpuSaveState & ~(SIZE_2MB-1)) == (PFAddress & ~(SIZE_2MB-1))) {
    if ((UINTN)mFrameworkSmst->CpuSaveState <= PFAddress &&
        PFAddress < (UINTN)mFrameworkSmst->CpuSaveState + EFI_PAGES_TO_SIZE (NumCpuStatePages)
        ) {
      mCpuStatePageTable[BitFieldRead64 (PFAddress, 12, 20)] |= BIT0 | BIT1; // present and rw
      CpuFlushTlb ();
      ReadWriteCpuStatePage (PFAddress & ~(SIZE_4KB-1), TRUE);
      IsHandled = TRUE;
    } else {
      ASSERT (FALSE);
    }
  }

  ReleaseSpinLock (&mPFLock);
  return IsHandled;
}

/**
  Write back the dirty Framework CpuSaveStates to PI.
  
  The function scans the page table for dirty pages in mFrameworkSmst->CpuSaveState
  to write back to PI CpuSaveStates. It is meant to be called on each SmmBaseHelper SMI
  callback after Framework handler is called.

**/
VOID
WriteBackDirtyPages (
  VOID
  )
{
  UINTN  NumCpuStatePages;
  UINTN  PTIndex;
  UINTN  PTStartIndex;
  UINTN  PTEndIndex;

  NumCpuStatePages = EFI_SIZE_TO_PAGES (mNumberOfProcessors * sizeof (EFI_SMM_CPU_SAVE_STATE));
  PTStartIndex = (UINTN)BitFieldRead64 ((UINT64) (UINTN) mFrameworkSmst->CpuSaveState, 12, 20);
  PTEndIndex   = (UINTN)BitFieldRead64 ((UINT64) (UINTN) mFrameworkSmst->CpuSaveState + EFI_PAGES_TO_SIZE(NumCpuStatePages) - 1, 12, 20);
  for (PTIndex = PTStartIndex; PTIndex <= PTEndIndex; PTIndex++) {
    if ((mCpuStatePageTable[PTIndex] & (BIT0|BIT6)) == (BIT0|BIT6)) { // present and dirty?
      ReadWriteCpuStatePage (mCpuStatePageTable[PTIndex] & mPhyMask, FALSE);
    }
  }
}

/**
  Hook IDT with our page fault handler so that the on-demand paging works on page fault.
  
  The function hooks the IDT with PageFaultHandlerHook to get on-demand paging work for
  PI<->Framework CpuSaveStates marshalling. It also saves original handler for pass-through
  purpose.

**/
VOID
HookPageFaultHandler (
  VOID
  )
{
  IA32_DESCRIPTOR           Idtr;
  IA32_IDT_GATE_DESCRIPTOR  *IdtGateDesc;
  UINT32                    OffsetUpper;
  
  InitializeSpinLock (&mPFLock);
  
  AsmReadIdtr (&Idtr);
  IdtGateDesc = (IA32_IDT_GATE_DESCRIPTOR *) Idtr.Base;
  OffsetUpper = *(UINT32*)((UINT64*)IdtGateDesc + 1);
  mOriginalHandler = (VOID *)(UINTN)(LShiftU64 (OffsetUpper, 32) + IdtGateDesc[14].Bits.OffsetLow + (IdtGateDesc[14].Bits.OffsetHigh << 16));
  IdtGateDesc[14].Bits.OffsetLow = (UINT32)((UINTN)PageFaultHandlerHook & ((1 << 16) - 1));
  IdtGateDesc[14].Bits.OffsetHigh = (UINT32)(((UINTN)PageFaultHandlerHook >> 16) & ((1 << 16) - 1));
}

/**
  Initialize page table for pages contain HookData.
  
  The function initialize PDE for 2MB range that contains HookData. If the related PDE points
  to a 2MB page, a page table will be allocated and initialized for 4KB pages. Otherwise we juse
  use the original page table.

  @param[in] HookData   Based on which to initialize page table.

  @return    The pointer to a Page Table that points to 4KB pages which contain HookData.
**/
UINT64 *
InitCpuStatePageTable (
  IN VOID *HookData
  )
{
  UINTN  Index;
  UINT64 *PageTable;
  UINT64 *Pdpte;
  UINT64 HookAddress;
  UINT64 Pde;
  UINT64 Address;
  
  //
  // Initialize physical address mask
  // NOTE: Physical memory above virtual address limit is not supported !!!
  //
  AsmCpuid (0x80000008, (UINT32*)&Index, NULL, NULL, NULL);
  mPhyMask = LShiftU64 (1, (UINT8)Index) - 1;
  mPhyMask &= (1ull << 48) - EFI_PAGE_SIZE;
  
  HookAddress = (UINT64)(UINTN)HookData;
  PageTable   = (UINT64 *)(UINTN)(AsmReadCr3 () & mPhyMask);
  PageTable = (UINT64 *)(UINTN)(PageTable[BitFieldRead64 (HookAddress, 39, 47)] & mPhyMask);
  PageTable = (UINT64 *)(UINTN)(PageTable[BitFieldRead64 (HookAddress, 30, 38)] & mPhyMask);
  
  Pdpte = (UINT64 *)(UINTN)PageTable;
  Pde = Pdpte[BitFieldRead64 (HookAddress, 21, 29)];
  ASSERT ((Pde & BIT0) != 0); // Present and 2M Page
  
  if ((Pde & BIT7) == 0) { // 4KB Page Directory
    PageTable = (UINT64 *)(UINTN)(Pde & mPhyMask);
  } else {
    ASSERT ((Pde & mPhyMask) == (HookAddress & ~(SIZE_2MB-1))); // 2MB Page Point to HookAddress
    PageTable = AllocatePages (1);
    ASSERT (PageTable != NULL);
    Address = HookAddress & ~(SIZE_2MB-1);
    for (Index = 0; Index < 512; Index++) {
      PageTable[Index] = Address | BIT0 | BIT1; // Present and RW
      Address += SIZE_4KB;
    }
    Pdpte[BitFieldRead64 (HookAddress, 21, 29)] = (UINT64)(UINTN)PageTable | BIT0 | BIT1; // Present and RW
  }
  return PageTable;
}

/**
  Mark all the CpuSaveStates as not present.
  
  The function marks all CpuSaveStates memory range as not present so that page fault can be triggered
  on CpuSaveStates access. It is meant to be called on each SmmBaseHelper SMI callback before Framework
  handler is called.

  @param[in] CpuSaveState   The base of CpuSaveStates.

**/
VOID
HookCpuStateMemory (
  IN EFI_SMM_CPU_SAVE_STATE *CpuSaveState
  )
{
  UINT64 Index;
  UINT64 PTStartIndex;
  UINT64 PTEndIndex;

  PTStartIndex = BitFieldRead64 ((UINTN)CpuSaveState, 12, 20);
  PTEndIndex = BitFieldRead64 ((UINTN)CpuSaveState + mNumberOfProcessors * sizeof (EFI_SMM_CPU_SAVE_STATE) - 1, 12, 20);
  for (Index = PTStartIndex; Index <= PTEndIndex; Index++) {
    mCpuStatePageTable[Index] &= ~(BIT0|BIT5|BIT6); // not present nor accessed nor dirty
  }
}  

/**
  Framework SMST SmmInstallConfigurationTable() Thunk.

  This thunk calls the PI SMM SmmInstallConfigurationTable() and then update the configuration
  table related fields in the Framework SMST because the PI SMM SmmInstallConfigurationTable()
  function may modify these fields.

  @param[in] SystemTable         A pointer to the SMM System Table.
  @param[in] Guid                A pointer to the GUID for the entry to add, update, or remove.
  @param[in] Table               A pointer to the buffer of the table to add.
  @param[in] TableSize           The size of the table to install.

  @retval EFI_SUCCESS            The (Guid, Table) pair was added, updated, or removed.
  @retval EFI_INVALID_PARAMETER  Guid is not valid.
  @retval EFI_NOT_FOUND          An attempt was made to delete a non-existent entry.
  @retval EFI_OUT_OF_RESOURCES   There is not enough memory available to complete the operation.
**/
EFI_STATUS
EFIAPI
SmmInstallConfigurationTable (
  IN EFI_SMM_SYSTEM_TABLE  *SystemTable,
  IN EFI_GUID              *Guid,
  IN VOID                  *Table,
  IN UINTN                 TableSize
  )
{
  EFI_STATUS  Status;
  
  Status = gSmst->SmmInstallConfigurationTable (gSmst, Guid, Table, TableSize);
  if (!EFI_ERROR (Status)) {
    mFrameworkSmst->NumberOfTableEntries = gSmst->NumberOfTableEntries;
    mFrameworkSmst->SmmConfigurationTable = gSmst->SmmConfigurationTable;
  }
  return Status;         
}

/**
  Initialize all the stuff needed for on-demand paging hooks for PI<->Framework
  CpuSaveStates marshalling.

  @param[in] FrameworkSmst   Framework SMM system table pointer.

**/
VOID
InitHook (
  IN EFI_SMM_SYSTEM_TABLE  *FrameworkSmst
  )
{
  UINTN                 NumCpuStatePages;
  UINTN                 CpuStatePage;
  UINTN                 Bottom2MPage;
  UINTN                 Top2MPage;
  
  mPageTableHookEnabled = FALSE;
  NumCpuStatePages = EFI_SIZE_TO_PAGES (mNumberOfProcessors * sizeof (EFI_SMM_CPU_SAVE_STATE));
  //
  // Only hook page table for X64 image and less than 2MB needed to hold all CPU Save States
  //
  if (EFI_IMAGE_MACHINE_TYPE_SUPPORTED(EFI_IMAGE_MACHINE_X64) && NumCpuStatePages <= EFI_SIZE_TO_PAGES (SIZE_2MB)) {
    //
    // Allocate double page size to make sure all CPU Save States are in one 2MB page.
    //
    CpuStatePage = (UINTN)AllocatePages (NumCpuStatePages * 2);
    ASSERT (CpuStatePage != 0);
    Bottom2MPage = CpuStatePage & ~(SIZE_2MB-1);
    Top2MPage    = (CpuStatePage + EFI_PAGES_TO_SIZE (NumCpuStatePages * 2) - 1) & ~(SIZE_2MB-1);
    if (Bottom2MPage == Top2MPage ||
        CpuStatePage + EFI_PAGES_TO_SIZE (NumCpuStatePages * 2) - Top2MPage >= EFI_PAGES_TO_SIZE (NumCpuStatePages)
        ) {
      //
      // If the allocated 4KB pages are within the same 2MB page or higher portion is larger, use higher portion pages.
      //
      FrameworkSmst->CpuSaveState = (EFI_SMM_CPU_SAVE_STATE *)(CpuStatePage + EFI_PAGES_TO_SIZE (NumCpuStatePages));
      FreePages ((VOID*)CpuStatePage, NumCpuStatePages);
    } else {
      FrameworkSmst->CpuSaveState = (EFI_SMM_CPU_SAVE_STATE *)CpuStatePage;
      FreePages ((VOID*)(CpuStatePage + EFI_PAGES_TO_SIZE (NumCpuStatePages)), NumCpuStatePages);
    }
    //
    // Add temporary working buffer for hooking
    //
    mShadowSaveState = (EFI_SMM_CPU_SAVE_STATE*) AllocatePool (sizeof (EFI_SMM_CPU_SAVE_STATE));
    ASSERT (mShadowSaveState != NULL);
    //
    // Allocate and initialize 4KB Page Table for hooking CpuSaveState.
    // Replace the original 2MB PDE with new 4KB page table.
    //
    mCpuStatePageTable = InitCpuStatePageTable (FrameworkSmst->CpuSaveState);
    //
    // Mark PTE for CpuSaveState as non-exist.
    //
    HookCpuStateMemory (FrameworkSmst->CpuSaveState);
    HookPageFaultHandler ();
    CpuFlushTlb ();
    mPageTableHookEnabled = TRUE;
  }
  mHookInitialized = TRUE;
}

/**
  Construct a Framework SMST based on the PI SMM SMST.

  @return  Pointer to the constructed Framework SMST.
**/
EFI_SMM_SYSTEM_TABLE *
ConstructFrameworkSmst (
  VOID
  )
{
  EFI_SMM_SYSTEM_TABLE  *FrameworkSmst;

  FrameworkSmst = (EFI_SMM_SYSTEM_TABLE  *)AllocatePool (sizeof (EFI_SMM_SYSTEM_TABLE));
  ASSERT (FrameworkSmst != NULL);

  ///
  /// Copy same things from PI SMST to Framework SMST
  ///
  CopyMem (FrameworkSmst, gSmst, (UINTN)(&((EFI_SMM_SYSTEM_TABLE *)0)->SmmIo));
  CopyMem (
    &FrameworkSmst->SmmIo, 
    &gSmst->SmmIo,
    sizeof (EFI_SMM_SYSTEM_TABLE) - (UINTN)(&((EFI_SMM_SYSTEM_TABLE *)0)->SmmIo)
    );

  ///
  /// Update Framework SMST
  ///
  FrameworkSmst->Hdr.Revision = EFI_SMM_SYSTEM_TABLE_REVISION;
  CopyGuid (&FrameworkSmst->EfiSmmCpuIoGuid, &mEfiSmmCpuIoGuid);

  mHookInitialized = FALSE;
  FrameworkSmst->CpuSaveState = (EFI_SMM_CPU_SAVE_STATE *)AllocateZeroPool (mNumberOfProcessors * sizeof (EFI_SMM_CPU_SAVE_STATE));
  ASSERT (FrameworkSmst->CpuSaveState != NULL);

  ///
  /// Do not support floating point state now
  ///
  FrameworkSmst->CpuOptionalFloatingPointState = NULL;

  FrameworkSmst->SmmInstallConfigurationTable = SmmInstallConfigurationTable;

  return FrameworkSmst;
}

/**
  Load a given Framework SMM driver into SMRAM and invoke its entry point.

  @param[in]   ParentImageHandle     Parent Image Handle.
  @param[in]   FilePath              Location of the image to be installed as the handler.
  @param[in]   SourceBuffer          Optional source buffer in case the image file
                                     is in memory.
  @param[in]   SourceSize            Size of the source image file, if in memory.
  @param[out]  ImageHandle           The handle that the base driver uses to decode 
                                     the handler. Unique among SMM handlers only, 
                                     not unique across DXE/EFI.

  @retval      EFI_SUCCESS           The operation was successful.
  @retval      EFI_OUT_OF_RESOURCES  There were no additional SMRAM resources to load the handler
  @retval      EFI_UNSUPPORTED       Can not find its copy in normal memory.
  @retval      EFI_INVALID_PARAMETER The handlers was not the correct image type
**/
EFI_STATUS
LoadImage (
  IN      EFI_HANDLE                ParentImageHandle,
  IN      EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN      VOID                      *SourceBuffer,
  IN      UINTN                     SourceSize,
  OUT     EFI_HANDLE                *ImageHandle
  )
{
  EFI_STATUS            Status;
  UINTN                 PageCount;
  UINTN                 OrgPageCount;
  EFI_PHYSICAL_ADDRESS  DstBuffer;

  if (FilePath == NULL || ImageHandle == NULL) {    
    return EFI_INVALID_PARAMETER;
  }

  PageCount = 1;
  do {
    OrgPageCount = PageCount;
    DstBuffer = (UINTN)-1;
    Status = gSmst->SmmAllocatePages (
                      AllocateMaxAddress,
                      EfiRuntimeServicesCode,
                      PageCount,
                      &DstBuffer
                      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = mLoadPe32Image->LoadPeImage (
                               mLoadPe32Image,
                               ParentImageHandle,
                               FilePath,
                               SourceBuffer,
                               SourceSize,
                               DstBuffer,
                               &PageCount,
                               ImageHandle,
                               NULL,
                               EFI_LOAD_PE_IMAGE_ATTRIBUTE_NONE
                               );
    if (EFI_ERROR (Status)) {
      FreePages ((VOID *)(UINTN)DstBuffer, OrgPageCount);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  if (!EFI_ERROR (Status)) {
    ///
    /// Update MP state in Framework SMST before transferring control to Framework SMM driver entry point
    ///
    mFrameworkSmst->SmmStartupThisAp      = gSmst->SmmStartupThisAp;
    mFrameworkSmst->NumberOfCpus          = mNumberOfProcessors;
    mFrameworkSmst->CurrentlyExecutingCpu = gSmst->CurrentlyExecutingCpu;

    RegisterSmramProfileImage (FilePath, DstBuffer, PageCount);
    Status = gBS->StartImage (*ImageHandle, NULL, NULL);
    if (EFI_ERROR (Status)) {
      UnregisterSmramProfileImage (FilePath, DstBuffer, PageCount);
      mLoadPe32Image->UnLoadPeImage (mLoadPe32Image, *ImageHandle);
      *ImageHandle = NULL;
      FreePages ((VOID *)(UINTN)DstBuffer, PageCount);
    }
  }

  return Status;
}

/**
  This function check if the address is in SMRAM.

  @param Buffer  the buffer address to be checked.
  @param Length  the buffer length to be checked.

  @retval TRUE  this address is in SMRAM.
  @retval FALSE this address is NOT in SMRAM.
**/
BOOLEAN
IsAddressInSmram (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  UINTN  Index;

  for (Index = 0; Index < mSmramRangeCount; Index ++) {
    if (((Buffer >= mSmramRanges[Index].CpuStart) && (Buffer < mSmramRanges[Index].CpuStart + mSmramRanges[Index].PhysicalSize)) ||
        ((mSmramRanges[Index].CpuStart >= Buffer) && (mSmramRanges[Index].CpuStart < Buffer + Length))) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  This function check if the address refered by Buffer and Length is valid.

  @param Buffer  the buffer address to be checked.
  @param Length  the buffer length to be checked.

  @retval TRUE  this address is valid.
  @retval FALSE this address is NOT valid.
**/
BOOLEAN
IsAddressValid (
  IN UINTN                 Buffer,
  IN UINTN                 Length
  )
{
  if (Buffer > (MAX_ADDRESS - Length)) {
    //
    // Overflow happen
    //
    return FALSE;
  }
  if (IsAddressInSmram ((EFI_PHYSICAL_ADDRESS)Buffer, (UINT64)Length)) {
    return FALSE;
  }
  return TRUE;
}

/** 
  Thunk service of EFI_SMM_BASE_PROTOCOL.Register().

  @param[in, out] FunctionData  Pointer to SMMBASE_FUNCTION_DATA.
**/
VOID
Register (
  IN OUT SMMBASE_FUNCTION_DATA *FunctionData
  )
{
  EFI_STATUS Status;

  if (mLocked || FunctionData->Args.Register.LegacyIA32Binary) {
    Status = EFI_UNSUPPORTED;
  } else {
    Status = LoadImage (
               FunctionData->SmmBaseImageHandle,
               FunctionData->Args.Register.FilePath,
               FunctionData->Args.Register.SourceBuffer,
               FunctionData->Args.Register.SourceSize,
               FunctionData->Args.Register.ImageHandle
               );
  }
  FunctionData->Status = Status;
}

/** 
  Thunk service of EFI_SMM_BASE_PROTOCOL.UnRegister().

  @param[in, out] FunctionData  Pointer to SMMBASE_FUNCTION_DATA.
**/
VOID
UnRegister (
  IN OUT SMMBASE_FUNCTION_DATA *FunctionData
  )
{
  ///
  /// Unregister not supported now
  ///
  FunctionData->Status = EFI_UNSUPPORTED;
}

/**
  Search for Framework SMI handler information according to specific PI SMM dispatch handle.

  @param[in] DispatchHandle  The unique handle assigned by SmiHandlerRegister().  

  @return  Pointer to CALLBACK_INFO. If NULL, no callback info record is found.
**/
CALLBACK_INFO *
GetCallbackInfo (
  IN EFI_HANDLE  DispatchHandle
  )
{
  LIST_ENTRY  *Node;

  Node = GetFirstNode (&mCallbackInfoListHead);
  while (!IsNull (&mCallbackInfoListHead, Node)) {
    if (((CALLBACK_INFO *)Node)->DispatchHandle == DispatchHandle) {
      return (CALLBACK_INFO *)Node;
    }
    Node = GetNextNode (&mCallbackInfoListHead, Node);
  }
  return NULL;
}

/**
  Callback thunk for Framework SMI handler.

  This thunk functions calls the Framework SMI handler and converts the return value
  defined from Framework SMI handlers to a correpsonding return value defined by PI SMM.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     Context         Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer      A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers 
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should 
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still 
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.
**/
EFI_STATUS
EFIAPI
CallbackThunk (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context         OPTIONAL,
  IN OUT VOID    *CommBuffer      OPTIONAL,
  IN OUT UINTN   *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS        Status;
  CALLBACK_INFO     *CallbackInfo;
  UINTN             CpuIndex;

  ///
  /// Before transferring the control into the Framework SMI handler, update CPU Save States
  /// and MP states in the Framework SMST.
  ///

  if (!mHookInitialized) {
    InitHook (mFrameworkSmst);
  }
  if (mPageTableHookEnabled) {
    HookCpuStateMemory (mFrameworkSmst->CpuSaveState);
    CpuFlushTlb ();
  } else {
    for (CpuIndex = 0; CpuIndex < mNumberOfProcessors; CpuIndex++) {
      ReadCpuSaveState (CpuIndex, NULL);
    }
  }

  mFrameworkSmst->SmmStartupThisAp      = gSmst->SmmStartupThisAp;
  mFrameworkSmst->NumberOfCpus          = mNumberOfProcessors;
  mFrameworkSmst->CurrentlyExecutingCpu = gSmst->CurrentlyExecutingCpu;

  ///
  /// Search for Framework SMI handler information
  ///
  CallbackInfo = GetCallbackInfo (DispatchHandle);
  ASSERT (CallbackInfo != NULL);

  ///
  /// Thunk into original Framwork SMI handler
  ///
  Status = (CallbackInfo->CallbackAddress) (
                            CallbackInfo->SmmImageHandle,
                            CallbackInfo->CommunicationBuffer,
                            CallbackInfo->SourceSize
                            );
  ///
  /// Save CPU Save States in case any of them was modified
  ///
  if (mPageTableHookEnabled) {
    WriteBackDirtyPages ();
  } else {
    for (CpuIndex = 0; CpuIndex < mNumberOfProcessors; CpuIndex++) {
      WriteCpuSaveState (CpuIndex, NULL);
    }
  }

  ///
  /// Conversion of returned status code
  ///
  switch (Status) {
    case EFI_HANDLER_SUCCESS:
      Status = EFI_WARN_INTERRUPT_SOURCE_QUIESCED;
      break;
    case EFI_HANDLER_CRITICAL_EXIT:
    case EFI_HANDLER_SOURCE_QUIESCED:
      Status = EFI_SUCCESS;
      break;
    case EFI_HANDLER_SOURCE_PENDING:
      Status = EFI_WARN_INTERRUPT_SOURCE_PENDING;
      break;
  }
  return Status;
}

/** 
  Thunk service of EFI_SMM_BASE_PROTOCOL.RegisterCallback().

  @param[in, out] FunctionData  Pointer to SMMBASE_FUNCTION_DATA.
**/
VOID
RegisterCallback (
  IN OUT SMMBASE_FUNCTION_DATA  *FunctionData
  )
{
  CALLBACK_INFO  *Buffer;

  if (mLocked) {
    FunctionData->Status = EFI_UNSUPPORTED;
    return;
  }

  ///
  /// Note that MakeLast and FloatingPointSave options are not supported in PI SMM
  ///

  ///
  /// Allocate buffer for callback thunk information
  ///
  Buffer = (CALLBACK_INFO *)AllocateZeroPool (sizeof (CALLBACK_INFO));
  if (Buffer == NULL) {
    FunctionData->Status = EFI_OUT_OF_RESOURCES;
    return;
  }

  ///
  /// Fill SmmImageHandle and CallbackAddress into the thunk
  ///
  Buffer->SmmImageHandle = FunctionData->Args.RegisterCallback.SmmImageHandle;
  Buffer->CallbackAddress = FunctionData->Args.RegisterCallback.CallbackAddress;

  ///
  /// Register the thunk code as a root SMI handler
  ///
  FunctionData->Status = gSmst->SmiHandlerRegister (
                                  CallbackThunk,
                                  NULL,
                                  &Buffer->DispatchHandle
                                  );
  if (EFI_ERROR (FunctionData->Status)) {
    FreePool (Buffer);
    return;
  }

  ///
  /// Save this callback info
  ///
  InsertTailList (&mCallbackInfoListHead, &Buffer->Link);
}


/** 
  Thunk service of EFI_SMM_BASE_PROTOCOL.SmmAllocatePool().

  @param[in, out] FunctionData  Pointer to SMMBASE_FUNCTION_DATA.
**/
VOID
HelperAllocatePool (
  IN OUT SMMBASE_FUNCTION_DATA *FunctionData
  )
{
  if (mLocked) {
    FunctionData->Status =  EFI_UNSUPPORTED;
  } else {
    FunctionData->Status = gSmst->SmmAllocatePool (
                                    FunctionData->Args.AllocatePool.PoolType,
                                    FunctionData->Args.AllocatePool.Size,
                                    FunctionData->Args.AllocatePool.Buffer
                                    );
  }
}

/** 
  Thunk service of EFI_SMM_BASE_PROTOCOL.SmmFreePool().

  @param[in, out] FunctionData  Pointer to SMMBASE_FUNCTION_DATA.
**/
VOID
HelperFreePool (
  IN OUT SMMBASE_FUNCTION_DATA *FunctionData
  )
{
  if (mLocked) {
    FunctionData->Status =  EFI_UNSUPPORTED;
  } else {
    FreePool (FunctionData->Args.FreePool.Buffer);
    FunctionData->Status = EFI_SUCCESS;
  }
}

/** 
  Thunk service of EFI_SMM_BASE_PROTOCOL.Communicate().

  @param[in, out] FunctionData  Pointer to SMMBASE_FUNCTION_DATA.
**/
VOID
HelperCommunicate (
  IN OUT SMMBASE_FUNCTION_DATA *FunctionData
  )
{
  LIST_ENTRY     *Node;
  CALLBACK_INFO  *CallbackInfo;

  if (FunctionData->Args.Communicate.CommunicationBuffer == NULL) {
    FunctionData->Status = EFI_INVALID_PARAMETER;
    return;
  }

  Node = GetFirstNode (&mCallbackInfoListHead);
  while (!IsNull (&mCallbackInfoListHead, Node)) {
    CallbackInfo = (CALLBACK_INFO *)Node;

    if (FunctionData->Args.Communicate.ImageHandle == CallbackInfo->SmmImageHandle) {
      CallbackInfo->CommunicationBuffer = FunctionData->Args.Communicate.CommunicationBuffer;
      CallbackInfo->SourceSize          = FunctionData->Args.Communicate.SourceSize;

      ///
      /// The message was successfully posted.
      ///
      FunctionData->Status = EFI_SUCCESS;
      return;
    }
    Node = GetNextNode (&mCallbackInfoListHead, Node);
  }

  FunctionData->Status = EFI_INVALID_PARAMETER;
}

/**
  Communication service SMI Handler entry.

  This SMI handler provides services for the SMM Base Thunk driver.

  Caution: This function may receive untrusted input during runtime.
  The communicate buffer is external input, so this function will do operations only if the communicate
  buffer is outside of SMRAM so that returning the status code in the buffer won't overwrite anywhere in SMRAM.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer      A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers 
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should 
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still 
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.
**/
EFI_STATUS
EFIAPI
SmmHandlerEntry (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *RegisterContext,
  IN OUT VOID                     *CommBuffer,
  IN OUT UINTN                    *CommBufferSize
  )
{
  SMMBASE_FUNCTION_DATA *FunctionData;

  ASSERT (CommBuffer != NULL);
  ASSERT (CommBufferSize != NULL);

  if (*CommBufferSize == sizeof (SMMBASE_FUNCTION_DATA) &&
      IsAddressValid ((UINTN)CommBuffer, *CommBufferSize)) {
    FunctionData = (SMMBASE_FUNCTION_DATA *)CommBuffer;

    switch (FunctionData->Function) {
      case SmmBaseFunctionRegister:
        Register (FunctionData);
        break;
      case SmmBaseFunctionUnregister:
        UnRegister (FunctionData);
        break;
      case SmmBaseFunctionRegisterCallback:
        RegisterCallback (FunctionData);
        break;
      case SmmBaseFunctionAllocatePool:
        HelperAllocatePool (FunctionData);
        break;
      case SmmBaseFunctionFreePool:
        HelperFreePool (FunctionData);
        break;
      case SmmBaseFunctionCommunicate:
        HelperCommunicate (FunctionData);
        break;
      default:
        DEBUG ((EFI_D_WARN, "SmmBaseHelper: invalid SMM Base function.\n"));
        FunctionData->Status = EFI_UNSUPPORTED;
    }
  }
  return EFI_SUCCESS;
}

/**
  Smm Ready To Lock event notification handler.

  It sets a flag indicating that SMRAM has been locked.
  
  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification handler runs successfully.
 **/
EFI_STATUS
EFIAPI
SmmReadyToLockEventNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  mLocked = TRUE;
  return EFI_SUCCESS;
}

/**
  Entry point function of the SMM Base Helper SMM driver.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
SmmBaseHelperMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_MP_SERVICES_PROTOCOL   *MpServices;
  EFI_HANDLE                 Handle;
  UINTN                      NumberOfEnabledProcessors;
  VOID                       *Registration;
  EFI_SMM_ACCESS2_PROTOCOL   *SmmAccess;
  UINTN                      Size;
  
  Handle = NULL;
  ///
  /// Locate SMM CPU Protocol which is used later to retrieve/update CPU Save States
  ///
  Status = gSmst->SmmLocateProtocol (&gEfiSmmCpuProtocolGuid, NULL, (VOID **) &mSmmCpu);
  ASSERT_EFI_ERROR (Status);

  ///
  /// Locate PE32 Image Protocol which is used later to load Framework SMM driver
  ///
  Status = SystemTable->BootServices->LocateProtocol (&gEfiLoadPeImageProtocolGuid, NULL, (VOID **) &mLoadPe32Image);
  ASSERT_EFI_ERROR (Status);

  //
  // Get MP Services Protocol
  //
  Status = SystemTable->BootServices->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpServices);
  ASSERT_EFI_ERROR (Status);

  //
  // Use MP Services Protocol to retrieve the number of processors and number of enabled processors
  //
  Status = MpServices->GetNumberOfProcessors (MpServices, &mNumberOfProcessors, &NumberOfEnabledProcessors);
  ASSERT_EFI_ERROR (Status);
  
  ///
  /// Interface structure of SMM BASE Helper Ready Protocol is allocated from UEFI pool
  /// instead of SMM pool so that SMM Base Thunk driver can access it in Non-SMM mode.
  ///
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_SMM_BASE_HELPER_READY_PROTOCOL),
                  (VOID **)&mSmmBaseHelperReady
                  );
  ASSERT_EFI_ERROR (Status);

  ///
  /// Construct Framework SMST from PI SMST
  ///
  mFrameworkSmst = ConstructFrameworkSmst ();
  mSmmBaseHelperReady->FrameworkSmst = mFrameworkSmst;
  mSmmBaseHelperReady->ServiceEntry = SmmHandlerEntry;

  //
  // Get SMRAM information
  //
  Status = gBS->LocateProtocol (&gEfiSmmAccess2ProtocolGuid, NULL, (VOID **)&SmmAccess);
  ASSERT_EFI_ERROR (Status);

  Size = 0;
  Status = SmmAccess->GetCapabilities (SmmAccess, &Size, NULL);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    Size,
                    (VOID **)&mSmramRanges
                    );
  ASSERT_EFI_ERROR (Status);

  Status = SmmAccess->GetCapabilities (SmmAccess, &Size, mSmramRanges);
  ASSERT_EFI_ERROR (Status);

  mSmramRangeCount = Size / sizeof (EFI_SMRAM_DESCRIPTOR);

  //
  // Register SMM Ready To Lock Protocol notification
  //
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmReadyToLockProtocolGuid,
                    SmmReadyToLockEventNotify,
                    &Registration
                    );
  ASSERT_EFI_ERROR (Status);

  ///
  /// Register SMM Base Helper services for SMM Base Thunk driver
  ///
  Status = gSmst->SmiHandlerRegister (SmmHandlerEntry, &gEfiSmmBaseThunkCommunicationGuid, &mDispatchHandle);
  ASSERT_EFI_ERROR (Status);

  ///
  /// Install EFI SMM Base Helper Protocol in the UEFI handle database
  ///
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiSmmBaseHelperReadyProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  mSmmBaseHelperReady
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

