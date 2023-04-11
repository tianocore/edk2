/** @file
  Unit tests of the CpuExceptionHandlerLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuExceptionHandlerTest.h"

//
// Length of the assembly falut instruction.
//
UINTN               mFaultInstructionLength = 0;
EFI_EXCEPTION_TYPE  mExceptionType          = 256;
UINTN               mNumberOfProcessors     = 1;
UINTN               mRspAddress[2]          = { 0 };

//
// Error code flag indicating whether or not an error code will be
// pushed on the stack if an exception occurs.
//
// 1 means an error code will be pushed, otherwise 0
//
CONST UINT32  mErrorCodeExceptionFlag = 0x20227d00;

/**
  Special handler for exception triggered by INTn instruction.
  This hanlder only modifies a global variable for check.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
INTnExceptionHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  mExceptionType = ExceptionType;
}

/**
  Restore cpu original registers before exit test case.

  @param[in] Buffer  Argument of the procedure.
**/
VOID
EFIAPI
RestoreRegistersPerCpu (
  IN VOID  *Buffer
  )
{
  CPU_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  UINT16               Tr;
  IA32_TSS_DESCRIPTOR  *Tss;

  CpuOriginalRegisterBuffer = (CPU_REGISTER_BUFFER *)Buffer;

  AsmWriteGdtr (&(CpuOriginalRegisterBuffer->OriginalGdtr));
  AsmWriteIdtr (&(CpuOriginalRegisterBuffer->OriginalIdtr));
  Tr = CpuOriginalRegisterBuffer->Tr;
  if ((Tr != 0) && (Tr < CpuOriginalRegisterBuffer->OriginalGdtr.Limit)) {
    Tss = (IA32_TSS_DESCRIPTOR *)(CpuOriginalRegisterBuffer->OriginalGdtr.Base + Tr);
    if (Tss->Bits.P == 1) {
      //
      // Clear busy bit of TSS before write Tr
      //
      Tss->Bits.Type &= 0xD;
      AsmWriteTr (Tr);
    }
  }
}

/**
  Restore cpu original registers before exit test case.

  @param[in] MpServices                 MpServices.
  @param[in] CpuOriginalRegisterBuffer  Address of CpuOriginalRegisterBuffer.
  @param[in] BspProcessorNum            Bsp processor number.
**/
VOID
RestoreAllCpuRegisters (
  MP_SERVICES *MpServices, OPTIONAL
  CPU_REGISTER_BUFFER  *CpuOriginalRegisterBuffer,
  UINTN                         BspProcessorNum
  )
{
  UINTN       Index;
  EFI_STATUS  Status;

  for (Index = 0; Index < mNumberOfProcessors; ++Index) {
    if (Index == BspProcessorNum) {
      RestoreRegistersPerCpu ((VOID *)&CpuOriginalRegisterBuffer[Index]);
      continue;
    }

    ASSERT (MpServices != NULL);
    Status = MpServicesUnitTestStartupThisAP (
               *MpServices,
               (EFI_AP_PROCEDURE)RestoreRegistersPerCpu,
               Index,
               0,
               (VOID *)&CpuOriginalRegisterBuffer[Index]
               );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Store cpu registers before the test case starts.

  @param[in] Buffer  Argument of the procedure.
**/
VOID
EFIAPI
SaveRegisterPerCpu (
  IN VOID  *Buffer
  )
{
  CPU_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  IA32_DESCRIPTOR      Gdtr;
  IA32_DESCRIPTOR      Idtr;

  CpuOriginalRegisterBuffer = (CPU_REGISTER_BUFFER *)Buffer;

  AsmReadGdtr (&Gdtr);
  AsmReadIdtr (&Idtr);
  CpuOriginalRegisterBuffer->OriginalGdtr.Base  = Gdtr.Base;
  CpuOriginalRegisterBuffer->OriginalGdtr.Limit = Gdtr.Limit;
  CpuOriginalRegisterBuffer->OriginalIdtr.Base  = Idtr.Base;
  CpuOriginalRegisterBuffer->OriginalIdtr.Limit = Idtr.Limit;
  CpuOriginalRegisterBuffer->Tr                 = AsmReadTr ();
}

/**
  Store cpu registers before the test case starts.

  @param[in] MpServices       MpServices.
  @param[in] BspProcessorNum  Bsp processor number.

  @return Pointer to the allocated CPU_REGISTER_BUFFER.
**/
CPU_REGISTER_BUFFER *
SaveAllCpuRegisters (
  MP_SERVICES *MpServices, OPTIONAL
  UINTN       BspProcessorNum
  )
{
  CPU_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  EFI_STATUS           Status;
  UINTN                Index;

  CpuOriginalRegisterBuffer = AllocateZeroPool (mNumberOfProcessors * sizeof (CPU_REGISTER_BUFFER));
  ASSERT (CpuOriginalRegisterBuffer != NULL);

  for (Index = 0; Index < mNumberOfProcessors; ++Index) {
    if (Index == BspProcessorNum) {
      SaveRegisterPerCpu ((VOID *)&CpuOriginalRegisterBuffer[Index]);
      continue;
    }

    ASSERT (MpServices != NULL);
    Status = MpServicesUnitTestStartupThisAP (
               *MpServices,
               (EFI_AP_PROCEDURE)SaveRegisterPerCpu,
               Index,
               0,
               (VOID *)&CpuOriginalRegisterBuffer[Index]
               );
    ASSERT_EFI_ERROR (Status);
  }

  return CpuOriginalRegisterBuffer;
}

/**
  Initialize Ap Idt Procedure.

  @param[in] Buffer  Argument of the procedure.
**/
VOID
EFIAPI
InitializeIdtPerAp (
  IN VOID  *Buffer
  )
{
  AsmWriteIdtr (Buffer);
}

/**
  Initialize all Ap Idt.

  @param[in] MpServices MpServices.
  @param[in] BspIdtr    Pointer to IA32_DESCRIPTOR allocated by Bsp.
**/
VOID
InitializeApIdt (
  MP_SERVICES  MpServices,
  VOID         *BspIdtr
  )
{
  EFI_STATUS  Status;

  Status = MpServicesUnitTestStartupAllAPs (
             MpServices,
             (EFI_AP_PROCEDURE)InitializeIdtPerAp,
             FALSE,
             0,
             BspIdtr
             );
  ASSERT_EFI_ERROR (Status);
}

/**
  Check if exception handler can registered/unregistered for no error code exception.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestRegisterHandlerForNoErrorCodeException (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS           Status;
  UINTN                Index;
  CPU_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  VOID                 *NewIdtr;

  CpuOriginalRegisterBuffer = SaveAllCpuRegisters (NULL, 0);
  NewIdtr                   = InitializeBspIdt ();
  Status                    = InitializeCpuExceptionHandlers (NULL);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

  for (Index = 0; Index < SPEC_MAX_EXCEPTION_NUM; Index++) {
    //
    // Only test no error code exception by INT n instruction.
    //
    if ((mErrorCodeExceptionFlag & (1 << Index)) != 0) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "TestCase1: ExceptionType is %d\n", Index));
    Status = RegisterCpuInterruptHandler (Index, INTnExceptionHandler);
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

    TriggerINTnException (Index);
    UT_ASSERT_EQUAL (mExceptionType, Index);
    Status = RegisterCpuInterruptHandler (Index, NULL);
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  }

  RestoreAllCpuRegisters (NULL, CpuOriginalRegisterBuffer, 0);
  FreePool (CpuOriginalRegisterBuffer);
  FreePool (NewIdtr);
  return UNIT_TEST_PASSED;
}

/**
  Get Bsp stack base.

  @param[out] StackBase  Pointer to stack base of BSP.
**/
VOID
GetBspStackBase (
  OUT UINTN  *StackBase
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryHob;

  //
  // Get the base of stack from Hob.
  //
  ASSERT (StackBase != NULL);
  Hob.Raw = GetHobList ();
  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
    MemoryHob = Hob.MemoryAllocation;
    if (CompareGuid (&gEfiHobMemoryAllocStackGuid, &MemoryHob->AllocDescriptor.Name)) {
      DEBUG ((
        DEBUG_INFO,
        "%a: Bsp StackBase = 0x%016lx  StackSize = 0x%016lx\n",
        __func__,
        MemoryHob->AllocDescriptor.MemoryBaseAddress,
        MemoryHob->AllocDescriptor.MemoryLength
        ));

      *StackBase = (UINTN)MemoryHob->AllocDescriptor.MemoryBaseAddress;
      //
      // Ensure the base of the stack is page-size aligned.
      //
      ASSERT ((*StackBase & EFI_PAGE_MASK) == 0);
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  ASSERT (*StackBase != 0);
}

/**
  Get Ap stack base procedure.

  @param[out] ApStackBase  Pointer to Ap stack base.
**/
VOID
EFIAPI
GetStackBasePerAp (
  OUT VOID  *ApStackBase
  )
{
  UINTN  ApTopOfStack;

  ApTopOfStack          = ALIGN_VALUE ((UINTN)&ApTopOfStack, (UINTN)PcdGet32 (PcdCpuApStackSize));
  *(UINTN *)ApStackBase = ApTopOfStack - (UINTN)PcdGet32 (PcdCpuApStackSize);
}

/**
  Get all Cpu stack base.

  @param[in] MpServices       MpServices.
  @param[in] BspProcessorNum  Bsp processor number.

  @return Pointer to the allocated CpuStackBaseBuffer.
**/
UINTN *
GetAllCpuStackBase (
  MP_SERVICES  *MpServices,
  UINTN        BspProcessorNum
  )
{
  UINTN       *CpuStackBaseBuffer;
  EFI_STATUS  Status;
  UINTN       Index;

  CpuStackBaseBuffer = AllocateZeroPool (mNumberOfProcessors * sizeof (UINTN));
  ASSERT (CpuStackBaseBuffer != NULL);

  for (Index = 0; Index < mNumberOfProcessors; ++Index) {
    if (Index == BspProcessorNum) {
      GetBspStackBase (&CpuStackBaseBuffer[Index]);
      continue;
    }

    ASSERT (MpServices != NULL);
    Status = MpServicesUnitTestStartupThisAP (
               *MpServices,
               (EFI_AP_PROCEDURE)GetStackBasePerAp,
               Index,
               0,
               (VOID *)&CpuStackBaseBuffer[Index]
               );
    ASSERT_EFI_ERROR (Status);
    DEBUG ((DEBUG_INFO, "AP[%d] StackBase = 0x%x\n", Index, CpuStackBaseBuffer[Index]));
  }

  return CpuStackBaseBuffer;
}

/**
  Find not present or ReadOnly address in page table.

  @param[out] PFAddress  Access to the address which is not permitted will trigger PF exceptions.

  @retval TRUE   Found not present or ReadOnly address in page table.
  @retval FALSE  Failed to found PFAddress in page table.
**/
BOOLEAN
FindPFAddressInPageTable (
  OUT UINTN  *PFAddress
  )
{
  IA32_CR0        Cr0;
  IA32_CR4        Cr4;
  UINTN           PageTable;
  PAGING_MODE     PagingMode;
  BOOLEAN         Enable5LevelPaging;
  RETURN_STATUS   Status;
  IA32_MAP_ENTRY  *Map;
  UINTN           MapCount;
  UINTN           Index;
  UINTN           PreviousAddress;

  ASSERT (PFAddress != NULL);

  Cr0.UintN = AsmReadCr0 ();
  if (Cr0.Bits.PG == 0) {
    return FALSE;
  }

  PageTable = AsmReadCr3 ();
  Cr4.UintN = AsmReadCr4 ();
  if (sizeof (UINTN) == sizeof (UINT32)) {
    ASSERT (Cr4.Bits.PAE == 1);
    PagingMode = PagingPae;
  } else {
    Enable5LevelPaging = (BOOLEAN)(Cr4.Bits.LA57 == 1);
    PagingMode         = Enable5LevelPaging ? Paging5Level : Paging4Level;
  }

  MapCount = 0;
  Status   = PageTableParse (PageTable, PagingMode, NULL, &MapCount);
  ASSERT (Status == RETURN_BUFFER_TOO_SMALL);
  Map    = AllocatePages (EFI_SIZE_TO_PAGES (MapCount * sizeof (IA32_MAP_ENTRY)));
  Status = PageTableParse (PageTable, PagingMode, Map, &MapCount);
  ASSERT (Status == RETURN_SUCCESS);

  PreviousAddress = 0;
  for (Index = 0; Index < MapCount; Index++) {
    DEBUG ((
      DEBUG_ERROR,
      "%02d: %016lx - %016lx, %016lx\n",
      Index,
      Map[Index].LinearAddress,
      Map[Index].LinearAddress + Map[Index].Length,
      Map[Index].Attribute.Uint64
      ));

    //
    // Not present address in page table.
    //
    if (Map[Index].LinearAddress > PreviousAddress) {
      *PFAddress = PreviousAddress;
      return TRUE;
    }

    PreviousAddress = (UINTN)(Map[Index].LinearAddress + Map[Index].Length);

    //
    // ReadOnly address in page table.
    //
    if ((Cr0.Bits.WP != 0) && (Map[Index].Attribute.Bits.ReadWrite == 0)) {
      *PFAddress = (UINTN)Map[Index].LinearAddress;
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Test if exception handler can registered/unregistered for GP and PF.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestRegisterHandlerForGPAndPF (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS           Status;
  CPU_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  UINTN                PFAddress;
  VOID                 *NewIdtr;

  PFAddress                 = 0;
  CpuOriginalRegisterBuffer = SaveAllCpuRegisters (NULL, 0);
  NewIdtr                   = InitializeBspIdt ();
  Status                    = InitializeCpuExceptionHandlers (NULL);

  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

  //
  // GP exception.
  //
  DEBUG ((DEBUG_INFO, "TestCase2: ExceptionType is %d\n", EXCEPT_IA32_GP_FAULT));
  Status = RegisterCpuInterruptHandler (EXCEPT_IA32_GP_FAULT, AdjustRipForFaultHandler);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

  TriggerGPException (CR4_RESERVED_BIT);
  UT_ASSERT_EQUAL (mExceptionType, EXCEPT_IA32_GP_FAULT);
  Status = RegisterCpuInterruptHandler (EXCEPT_IA32_GP_FAULT, NULL);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

  //
  // PF exception.
  //
  if (FindPFAddressInPageTable (&PFAddress)) {
    DEBUG ((DEBUG_INFO, "TestCase2: ExceptionType is %d\n", EXCEPT_IA32_PAGE_FAULT));
    Status = RegisterCpuInterruptHandler (EXCEPT_IA32_PAGE_FAULT, AdjustRipForFaultHandler);
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
    TriggerPFException (PFAddress);

    UT_ASSERT_EQUAL (mExceptionType, EXCEPT_IA32_PAGE_FAULT);
    Status = RegisterCpuInterruptHandler (EXCEPT_IA32_PAGE_FAULT, NULL);
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  }

  RestoreAllCpuRegisters (NULL, CpuOriginalRegisterBuffer, 0);
  FreePool (CpuOriginalRegisterBuffer);
  FreePool (NewIdtr);
  return UNIT_TEST_PASSED;
}

/**
  Test if Cpu Context is consistent before and after exception.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestCpuContextConsistency (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS           Status;
  UINTN                Index;
  CPU_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  UINTN                FaultParameter;
  VOID                 *NewIdtr;

  FaultParameter            = 0;
  CpuOriginalRegisterBuffer = SaveAllCpuRegisters (NULL, 0);
  NewIdtr                   = InitializeBspIdt ();
  Status                    = InitializeCpuExceptionHandlers (NULL);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

  for (Index = 0; Index < 22; Index++) {
    if (Index == EXCEPT_IA32_PAGE_FAULT) {
      if (!FindPFAddressInPageTable (&FaultParameter)) {
        continue;
      }
    } else if (Index == EXCEPT_IA32_GP_FAULT) {
      FaultParameter = CR4_RESERVED_BIT;
    } else {
      if ((mErrorCodeExceptionFlag & (1 << Index)) != 0) {
        continue;
      }
    }

    DEBUG ((DEBUG_INFO, "TestCase3: ExceptionType is %d\n", Index));
    Status = RegisterCpuInterruptHandler (Index, AdjustCpuContextHandler);
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

    //
    // Trigger different type exception and compare different stage cpu context.
    //
    AsmTestConsistencyOfCpuContext (Index, FaultParameter);
    CompareCpuContext ();
    Status = RegisterCpuInterruptHandler (Index, NULL);
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  }

  RestoreAllCpuRegisters (NULL, CpuOriginalRegisterBuffer, 0);
  FreePool (CpuOriginalRegisterBuffer);
  FreePool (NewIdtr);
  return UNIT_TEST_PASSED;
}

/**
  Initializes CPU exceptions handlers for the sake of stack switch requirement.

  This function is a wrapper of InitializeSeparateExceptionStacks. It's mainly
  for the sake of AP's init because of EFI_AP_PROCEDURE API requirement.

  @param[in,out] Buffer  The pointer to private data buffer.

**/
VOID
EFIAPI
InitializeExceptionStackSwitchHandlersPerAp (
  IN OUT VOID  *Buffer
  )
{
  EXCEPTION_STACK_SWITCH_CONTEXT  *CpuSwitchStackData;

  CpuSwitchStackData = (EXCEPTION_STACK_SWITCH_CONTEXT *)Buffer;

  //
  // This may be called twice for each Cpu. Only run InitializeSeparateExceptionStacks
  // if this is the first call or the first call failed because of size too small.
  //
  if ((CpuSwitchStackData->Status == EFI_NOT_STARTED) || (CpuSwitchStackData->Status == EFI_BUFFER_TOO_SMALL)) {
    CpuSwitchStackData->Status = InitializeSeparateExceptionStacks (CpuSwitchStackData->Buffer, &CpuSwitchStackData->BufferSize);
  }
}

/**
  Initializes MP exceptions handlers for the sake of stack switch requirement.

  This function will allocate required resources required to setup stack switch
  and pass them through SwitchStackData to each logic processor.

  @param[in, out] MpServices       MpServices.
  @param[in, out] BspProcessorNum  Bsp processor number.

  @return Pointer to the allocated SwitchStackData.
**/
EXCEPTION_STACK_SWITCH_CONTEXT  *
InitializeMpExceptionStackSwitchHandlers (
  MP_SERVICES  MpServices,
  UINTN        BspProcessorNum
  )
{
  UINTN                           Index;
  EXCEPTION_STACK_SWITCH_CONTEXT  *SwitchStackData;
  UINTN                           BufferSize;
  EFI_STATUS                      Status;
  UINT8                           *Buffer;

  SwitchStackData = AllocateZeroPool (mNumberOfProcessors * sizeof (EXCEPTION_STACK_SWITCH_CONTEXT));
  ASSERT (SwitchStackData != NULL);
  for (Index = 0; Index < mNumberOfProcessors; ++Index) {
    //
    // Because the procedure may runs multiple times, use the status EFI_NOT_STARTED
    // to indicate the procedure haven't been run yet.
    //
    SwitchStackData[Index].Status = EFI_NOT_STARTED;
    if (Index == BspProcessorNum) {
      InitializeExceptionStackSwitchHandlersPerAp ((VOID *)&SwitchStackData[Index]);
      continue;
    }

    Status = MpServicesUnitTestStartupThisAP (
               MpServices,
               InitializeExceptionStackSwitchHandlersPerAp,
               Index,
               0,
               (VOID *)&SwitchStackData[Index]
               );
    ASSERT_EFI_ERROR (Status);
  }

  BufferSize = 0;
  for (Index = 0; Index < mNumberOfProcessors; ++Index) {
    if (SwitchStackData[Index].Status == EFI_BUFFER_TOO_SMALL) {
      ASSERT (SwitchStackData[Index].BufferSize != 0);
      BufferSize += SwitchStackData[Index].BufferSize;
    } else {
      ASSERT (SwitchStackData[Index].Status == EFI_SUCCESS);
      ASSERT (SwitchStackData[Index].BufferSize == 0);
    }
  }

  if (BufferSize != 0) {
    Buffer = AllocateZeroPool (BufferSize);
    ASSERT (Buffer != NULL);
    BufferSize = 0;
    for (Index = 0; Index < mNumberOfProcessors; ++Index) {
      if (SwitchStackData[Index].Status == EFI_BUFFER_TOO_SMALL) {
        SwitchStackData[Index].Buffer = (VOID *)(&Buffer[BufferSize]);
        BufferSize                   += SwitchStackData[Index].BufferSize;
        DEBUG ((
          DEBUG_INFO,
          "Buffer[cpu%lu] for InitializeExceptionStackSwitchHandlersPerAp: 0x%lX with size 0x%lX\n",
          (UINT64)(UINTN)Index,
          (UINT64)(UINTN)SwitchStackData[Index].Buffer,
          (UINT64)(UINTN)SwitchStackData[Index].BufferSize
          ));
      }
    }

    for (Index = 0; Index < mNumberOfProcessors; ++Index) {
      if (Index == BspProcessorNum) {
        InitializeExceptionStackSwitchHandlersPerAp ((VOID *)&SwitchStackData[Index]);
        continue;
      }

      Status = MpServicesUnitTestStartupThisAP (
                 MpServices,
                 InitializeExceptionStackSwitchHandlersPerAp,
                 Index,
                 0,
                 (VOID *)&SwitchStackData[Index]
                 );
      ASSERT_EFI_ERROR (Status);
    }

    for (Index = 0; Index < mNumberOfProcessors; ++Index) {
      ASSERT (SwitchStackData[Index].Status == EFI_SUCCESS);
    }
  }

  return SwitchStackData;
}

/**
  Test if stack overflow is captured by CpuStackGuard in both Bsp and AP.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestCpuStackGuardInBspAndAp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                      Status;
  UINTN                           OriginalStackBase;
  UINTN                           NewStackTop;
  UINTN                           NewStackBase;
  EXCEPTION_STACK_SWITCH_CONTEXT  *SwitchStackData;
  MP_SERVICES                     MpServices;
  UINTN                           ProcessorNumber;
  UINTN                           EnabledProcessorNum;
  CPU_REGISTER_BUFFER             *CpuOriginalRegisterBuffer;
  UINTN                           Index;
  UINTN                           BspProcessorNum;
  VOID                            *NewIdtr;
  UINTN                           *CpuStackBaseBuffer;

  if (!PcdGetBool (PcdCpuStackGuard)) {
    return UNIT_TEST_PASSED;
  }

  //
  // Get MP Service Protocol
  //
  Status = GetMpServices (&MpServices);
  Status = MpServicesUnitTestGetNumberOfProcessors (MpServices, &ProcessorNumber, &EnabledProcessorNum);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  Status = MpServicesUnitTestWhoAmI (MpServices, &BspProcessorNum);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  mNumberOfProcessors = ProcessorNumber;

  CpuOriginalRegisterBuffer = SaveAllCpuRegisters (&MpServices, BspProcessorNum);

  //
  // Initialize Bsp and AP Idt.
  // Idt buffer should not be empty or it will hang in MP API.
  //
  NewIdtr = InitializeBspIdt ();
  Status  = InitializeCpuExceptionHandlers (NULL);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  InitializeApIdt (MpServices, NewIdtr);

  //
  // Get BSP and AP original stack base.
  //
  CpuStackBaseBuffer = GetAllCpuStackBase (&MpServices, BspProcessorNum);

  //
  // InitializeMpExceptionStackSwitchHandlers and register exception handler.
  //
  SwitchStackData = InitializeMpExceptionStackSwitchHandlers (MpServices, BspProcessorNum);
  Status          = RegisterCpuInterruptHandler (EXCEPT_IA32_PAGE_FAULT, CpuStackGuardExceptionHandler);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  Status = RegisterCpuInterruptHandler (EXCEPT_IA32_DOUBLE_FAULT, AdjustRipForFaultHandler);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

  for (Index = 0; Index < mNumberOfProcessors; Index++) {
    OriginalStackBase = CpuStackBaseBuffer[Index];
    NewStackTop       = (UINTN)(SwitchStackData[Index].Buffer) + SwitchStackData[Index].BufferSize;
    NewStackBase      = (UINTN)(SwitchStackData[Index].Buffer);
    if (Index == BspProcessorNum) {
      TriggerStackOverflow ();
    } else {
      MpServicesUnitTestStartupThisAP (
        MpServices,
        (EFI_AP_PROCEDURE)TriggerStackOverflow,
        Index,
        0,
        NULL
        );
    }

    DEBUG ((DEBUG_INFO, "TestCase4: mRspAddress[0] is 0x%x, mRspAddress[1] is 0x%x\n", mRspAddress[0], mRspAddress[1]));
    UT_ASSERT_TRUE ((mRspAddress[0] >= OriginalStackBase) && (mRspAddress[0] <= (OriginalStackBase + SIZE_4KB)));
    UT_ASSERT_TRUE ((mRspAddress[1] >= NewStackBase) && (mRspAddress[1] < NewStackTop));
  }

  Status = RegisterCpuInterruptHandler (EXCEPT_IA32_PAGE_FAULT, NULL);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  Status = RegisterCpuInterruptHandler (EXCEPT_IA32_DOUBLE_FAULT, NULL);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  RestoreAllCpuRegisters (&MpServices, CpuOriginalRegisterBuffer, BspProcessorNum);
  FreePool (SwitchStackData);
  FreePool (CpuOriginalRegisterBuffer);
  FreePool (NewIdtr);

  return UNIT_TEST_PASSED;
}

/**
  Create CpuExceptionLibUnitTestSuite and add test case.

  @param[in]  FrameworkHandle    Unit test framework.

  @return  EFI_SUCCESS           The unit test suite was created.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit test suite.
**/
EFI_STATUS
AddCommonTestCase (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  Framework
  )
{
  EFI_STATUS              Status;
  UNIT_TEST_SUITE_HANDLE  CpuExceptionLibUnitTestSuite;

  //
  // Populate the Manual Test Cases.
  //
  Status = CreateUnitTestSuite (&CpuExceptionLibUnitTestSuite, Framework, "Test CpuExceptionHandlerLib", "CpuExceptionHandlerLib.Manual", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for CpuExceptionHandlerLib Test Cases\n"));
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  AddTestCase (CpuExceptionLibUnitTestSuite, "Check if exception handler can be registered/unregistered for no error code exception", "TestRegisterHandlerForNoErrorCodeException", TestRegisterHandlerForNoErrorCodeException, NULL, NULL, NULL);
  AddTestCase (CpuExceptionLibUnitTestSuite, "Check if exception handler can be registered/unregistered for GP and PF", "TestRegisterHandlerForGPAndPF", TestRegisterHandlerForGPAndPF, NULL, NULL, NULL);

  AddTestCase (CpuExceptionLibUnitTestSuite, "Check if Cpu Context is consistent before and after exception.", "TestCpuContextConsistency", TestCpuContextConsistency, NULL, NULL, NULL);
  AddTestCase (CpuExceptionLibUnitTestSuite, "Check if stack overflow is captured by CpuStackGuard in Bsp and AP", "TestCpuStackGuardInBspAndAp", TestCpuStackGuardInBspAndAp, NULL, NULL, NULL);

  return EFI_SUCCESS;
}
