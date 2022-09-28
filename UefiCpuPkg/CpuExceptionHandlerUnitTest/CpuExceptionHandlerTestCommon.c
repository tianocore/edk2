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
BOOLEAN             mIsDxeDriver            = FALSE;
EFI_GUID            mCpuInitMpLibHobGuid    = CPU_INIT_MP_LIB_HOB_GUID;
UINTN               mRspAddress[2]          = { 0 };

//
// Error code flag indicating whether or not an error code will be
// pushed on the stack if an exception occurs.
//
// 1 means an error code will be pushed, otherwise 0
//
CONST UINT32           mErrorCodeExceptionFlag = 0x20227d00;
GENERAL_REGISTER       mOriginGeneralRegister[2];
GENERAL_REGISTER_IA32  mOriginIa32GeneralRegister[2];

/**
  Special handler for trap exception.

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
  Special handler for ConsistencyOfCpuContext test case.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
ConsistencyOfCpuContextRealExceptionHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  ConsistencyOfCpuContextExceptionHandler (ExceptionType, SystemContext);
  ErrorCodeFaultExceptionHandler (ExceptionType, SystemContext);
}

/**
  Restore all cpu original register before test case.

  @param[in] Buffer  Argument of the procedure.
**/
VOID
RestoreAllCpuOriginRegisterProcedure (
  IN VOID  *Buffer
  )
{
  CPU_ORIGINAL_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  UINT16                        Tr;
  IA32_TSS_DESCRIPTOR           *Tss;

  CpuOriginalRegisterBuffer = (CPU_ORIGINAL_REGISTER_BUFFER *)Buffer;

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
  Restore all cpu original register before test case.

  @param[in] MpServices                 MpServices.
  @param[in] CpuOriginalRegisterBuffer  Address of CpuOriginalRegisterBuffer.
  @param[in] BspProcessorNum            Bsp processor number.
**/
VOID
RestoreAllCpuOriginRegister (
  MP_SERVICES *MpServices, OPTIONAL
  CPU_ORIGINAL_REGISTER_BUFFER  *CpuOriginalRegisterBuffer,
  UINTN                         BspProcessorNum
  )
{
  CPU_ORIGINAL_REGISTER_BUFFER  *AllCpuOriginalRegisterBuffer;
  UINTN                         Index;
  EFI_STATUS                    Status;

  for (Index = 0; Index < mNumberOfProcessors; ++Index) {
    AllCpuOriginalRegisterBuffer = CpuOriginalRegisterBuffer + Index;
    if ((Index == BspProcessorNum) || (mNumberOfProcessors == 1)) {
      RestoreAllCpuOriginRegisterProcedure ((VOID *)AllCpuOriginalRegisterBuffer);
      continue;
    }

    ASSERT (MpServices != NULL);
    Status = MpServicesUnitTestStartupThisAP (
               *MpServices,
               (EFI_AP_PROCEDURE)RestoreAllCpuOriginRegisterProcedure,
               Index,
               0,
               (VOID *)AllCpuOriginalRegisterBuffer
               );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Store all cpu original register before test case.

  @param[in] Buffer  Argument of the procedure.
**/
VOID
StoreCpuOriginalRegisterProcedure (
  IN VOID  *Buffer
  )
{
  CPU_ORIGINAL_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  IA32_DESCRIPTOR               Gdtr;
  IA32_DESCRIPTOR               Idtr;

  CpuOriginalRegisterBuffer = (CPU_ORIGINAL_REGISTER_BUFFER *)Buffer;

  AsmReadGdtr (&Gdtr);
  AsmReadIdtr (&Idtr);
  CpuOriginalRegisterBuffer->OriginalGdtr.Base  = Gdtr.Base;
  CpuOriginalRegisterBuffer->OriginalGdtr.Limit = Gdtr.Limit;
  CpuOriginalRegisterBuffer->OriginalIdtr.Base  = Idtr.Base;
  CpuOriginalRegisterBuffer->OriginalIdtr.Limit = Idtr.Limit;
  CpuOriginalRegisterBuffer->Tr                 = AsmReadTr ();
}

/**
  Store all cpu original register before test case.

  @param[in] MpServices       MpServices.
  @param[in] BspProcessorNum  Bsp processor number.

  @return Pointer to the allocated CPU_ORIGINAL_REGISTER_BUFFER.
**/
CPU_ORIGINAL_REGISTER_BUFFER *
StoreAllCpuOriginRegister (
  MP_SERVICES *MpServices, OPTIONAL
  UINTN       BspProcessorNum
  )
{
  CPU_ORIGINAL_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  CPU_ORIGINAL_REGISTER_BUFFER  *AllCpuOriginalRegisterBuffer;
  EFI_STATUS                    Status;
  UINTN                         Index;

  CpuOriginalRegisterBuffer = AllocateZeroPool (mNumberOfProcessors * sizeof (CPU_ORIGINAL_REGISTER_BUFFER));
  ASSERT (CpuOriginalRegisterBuffer != NULL);

  for (Index = 0; Index < mNumberOfProcessors; ++Index) {
    AllCpuOriginalRegisterBuffer = CpuOriginalRegisterBuffer + Index;
    if ((Index == BspProcessorNum) || (mNumberOfProcessors == 1)) {
      StoreCpuOriginalRegisterProcedure ((VOID *)AllCpuOriginalRegisterBuffer);
      continue;
    }

    ASSERT (MpServices != NULL);
    Status = MpServicesUnitTestStartupThisAP (
               *MpServices,
               (EFI_AP_PROCEDURE)StoreCpuOriginalRegisterProcedure,
               Index,
               0,
               (VOID *)AllCpuOriginalRegisterBuffer
               );
    ASSERT_EFI_ERROR (Status);
  }

  return CpuOriginalRegisterBuffer;
}

/**
  Initialize Bsp Idt.

  @return Pointer to the allocated IA32_DESCRIPTOR buffer.
**/
VOID *
EFIAPI
InitializeBspIdt (
  VOID
  )
{
  UINTN            *NewIdtTable;
  UINTN            IdtDescNum;
  IA32_DESCRIPTOR  *Idtr;
  IA32_DESCRIPTOR  OriginalIdtr;

  Idtr = AllocateZeroPool (sizeof (IA32_DESCRIPTOR));
  ASSERT (Idtr != NULL);
  if (mIsDxeDriver) {
    IdtDescNum  = CPU_INTERRUPT_NUM;
    NewIdtTable = AllocateZeroPool (sizeof (IA32_IDT_GATE_DESCRIPTOR) * IdtDescNum);
  } else {
    AsmReadIdtr (&OriginalIdtr);
    IdtDescNum  = CPU_EXCEPTION_NUM;
    NewIdtTable = AllocateZeroPool (sizeof (IA32_IDT_GATE_DESCRIPTOR) * IdtDescNum + sizeof (UINTN));
    //
    // Store original PeiServicePointer before new Idt table
    //
    *NewIdtTable = *(UINTN *)(OriginalIdtr.Base - sizeof (UINTN));
    NewIdtTable  = (UINTN *)((UINTN)NewIdtTable + sizeof (UINTN));
  }

  ASSERT (NewIdtTable != NULL);
  Idtr->Base  = (UINTN)NewIdtTable;
  Idtr->Limit = (UINT16)(sizeof (IA32_IDT_GATE_DESCRIPTOR) * IdtDescNum - 1);

  AsmWriteIdtr (Idtr);
  return Idtr;
}

/**
  Initialize Ap Idt Procedure.

  @param[in] Buffer  Argument of the procedure.
**/
VOID
InitializeApIdtProcedure (
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
             (EFI_AP_PROCEDURE)InitializeApIdtProcedure,
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
  EFI_STATUS                    Status;
  UINTN                         Index;
  CPU_ORIGINAL_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  VOID                          *NewIdtr;

  CpuOriginalRegisterBuffer = StoreAllCpuOriginRegister (NULL, 0);
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
    UT_ASSERT_NOT_EQUAL (mExceptionType, Index);

    TriggerINTnException (Index);
    UT_ASSERT_EQUAL (mExceptionType, Index);
    Status = RegisterCpuInterruptHandler (Index, NULL);
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  }

  RestoreAllCpuOriginRegister (NULL, CpuOriginalRegisterBuffer, 0);
  FreePool (CpuOriginalRegisterBuffer);
  FreePool (NewIdtr);
  return UNIT_TEST_PASSED;
}

/**
  Get Cpu stack Info.

  @param[out] StackBase            Pointer to stack base of BSP.
  @param[out] CpuInfoInHobAddress  Address of CpuInfoInHob which contain the ApTopOfStack.
**/
VOID
GetCpuStackInfo (
  OUT UINTN   *StackBase,
  OUT UINT64  *CpuInfoInHobAddress OPTIONAL
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  VOID               *DataInHob;

  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryHob;

  if (CpuInfoInHobAddress != NULL) {
    //
    // Get CpuInfoInHob address from Hob.
    //
    GuidHob = GetFirstGuidHob (&mCpuInitMpLibHobGuid);
    if (GuidHob != NULL) {
      DataInHob            = GET_GUID_HOB_DATA (GuidHob);
      *CpuInfoInHobAddress = *(UINT64 *)(*(UINTN *)DataInHob);
    }

    ASSERT (*CpuInfoInHobAddress != 0);
  }

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
        __FUNCTION__,
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
TestRegisterHandlerForExceptionWithErrorCode (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                    Status;
  CPU_ORIGINAL_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  UINTN                         ReadOnlyAddr;
  VOID                          *NewIdtr;

  ReadOnlyAddr              = 0;
  CpuOriginalRegisterBuffer = StoreAllCpuOriginRegister (NULL, 0);
  NewIdtr                   = InitializeBspIdt ();
  Status                    = InitializeCpuExceptionHandlers (NULL);

  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

  //
  // GP exception.
  //
  DEBUG ((DEBUG_INFO, "TestCase2: ExceptionType is %d\n", EXCEPT_IA32_GP_FAULT));
  Status = RegisterCpuInterruptHandler (EXCEPT_IA32_GP_FAULT, ErrorCodeFaultExceptionHandler);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_NOT_EQUAL (mExceptionType, EXCEPT_IA32_GP_FAULT);

  TriggerGPException (GP_CR4_RESERVED_BIT);
  UT_ASSERT_EQUAL (mExceptionType, EXCEPT_IA32_GP_FAULT);
  Status = RegisterCpuInterruptHandler (EXCEPT_IA32_GP_FAULT, NULL);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

  //
  // PF exception.
  // In PEI phase, PG is enabled only when PcdCpuStackGuard is TURE.
  //
  if (mIsDxeDriver || PcdGetBool (PcdCpuStackGuard)) {
    DEBUG ((DEBUG_INFO, "TestCase2: ExceptionType is %d\n", EXCEPT_IA32_PAGE_FAULT));
    Status = RegisterCpuInterruptHandler (EXCEPT_IA32_PAGE_FAULT, ErrorCodeFaultExceptionHandler);
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
    UT_ASSERT_NOT_EQUAL (mExceptionType, EXCEPT_IA32_PAGE_FAULT);

    if (mIsDxeDriver) {
      ReadOnlyAddr = AsmReadCr3 ();
    } else {
      GetCpuStackInfo (&ReadOnlyAddr, NULL);
    }

    TriggerPFException (ReadOnlyAddr);

    UT_ASSERT_EQUAL (mExceptionType, EXCEPT_IA32_PAGE_FAULT);
    Status = RegisterCpuInterruptHandler (EXCEPT_IA32_PAGE_FAULT, NULL);
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  }

  RestoreAllCpuOriginRegister (NULL, CpuOriginalRegisterBuffer, 0);
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
  EFI_STATUS                    Status;
  UINTN                         Index;
  CPU_ORIGINAL_REGISTER_BUFFER  *CpuOriginalRegisterBuffer;
  UINTN                         CxValue;
  UINTN                         OptionalPeiStackBase;
  VOID                          *NewIdtr;

  OptionalPeiStackBase      = 0;
  CxValue                   = 0;
  CpuOriginalRegisterBuffer = StoreAllCpuOriginRegister (NULL, 0);
  NewIdtr                   = InitializeBspIdt ();
  Status                    = InitializeCpuExceptionHandlers (NULL);

  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

  for (Index = 0; Index < 22; Index++) {
    if (Index == 0xd) {
      CxValue = GP_CR4_RESERVED_BIT;
      Status  = RegisterCpuInterruptHandler (Index, ConsistencyOfCpuContextRealExceptionHandler);
      UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
    } else if (Index == 0xe) {
      if (!mIsDxeDriver && !PcdGetBool (PcdCpuStackGuard)) {
        continue;
      }

      if (mIsDxeDriver) {
        CxValue = AsmReadCr3 ();
      } else {
        GetCpuStackInfo (&CxValue, NULL);
        //
        // PeiStackBase is needed to trigger PF exception in PEIM.
        //
        OptionalPeiStackBase = CxValue;
      }

      Status = RegisterCpuInterruptHandler (Index, ConsistencyOfCpuContextRealExceptionHandler);
      UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
    } else {
      if ((mErrorCodeExceptionFlag & (1 << Index)) != 0) {
        continue;
      }

      CxValue = Index;
      Status  = RegisterCpuInterruptHandler (Index, ConsistencyOfCpuContextExceptionHandler);
      UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
    }

    DEBUG ((DEBUG_INFO, "TestCase3: ExceptionType is %d\n", Index));
    //
    // Rcx/Ecx is set to CxValue before exception in AsmTestConsistencyOfCpuContext.
    // This is due to Rcx is needed during X64 assembly code flow.
    //
    AsmTestConsistencyOfCpuContext (Index, OptionalPeiStackBase);
    CompareCpuContext (CxValue);
    Status = RegisterCpuInterruptHandler (Index, NULL);
    UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  }

  RestoreAllCpuOriginRegister (NULL, CpuOriginalRegisterBuffer, 0);
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
InitializeExceptionStackSwitchHandlers (
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
  EXCEPTION_STACK_SWITCH_CONTEXT  *CpuSwitchStackData;
  UINTN                           BufferSize;
  EFI_STATUS                      Status;
  UINT8                           *Buffer;

  SwitchStackData = AllocateZeroPool (mNumberOfProcessors * sizeof (EXCEPTION_STACK_SWITCH_CONTEXT));
  ASSERT (SwitchStackData != NULL);
  for (Index = 0; Index < mNumberOfProcessors; ++Index) {
    CpuSwitchStackData = SwitchStackData + Index;
    //
    // Because the procedure may runs multiple times, use the status EFI_NOT_STARTED
    // to indicate the procedure haven't been run yet.
    //
    SwitchStackData[Index].Status = EFI_NOT_STARTED;
    if (Index == BspProcessorNum) {
      InitializeExceptionStackSwitchHandlers ((VOID *)CpuSwitchStackData);
      continue;
    }

    Status = MpServicesUnitTestStartupThisAP (
               MpServices,
               InitializeExceptionStackSwitchHandlers,
               Index,
               0,
               (VOID *)CpuSwitchStackData
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
    Buffer = AllocateRuntimeZeroPool (BufferSize);
    ASSERT (Buffer != NULL);
    BufferSize = 0;
    for (Index = 0; Index < mNumberOfProcessors; ++Index) {
      if (SwitchStackData[Index].Status == EFI_BUFFER_TOO_SMALL) {
        SwitchStackData[Index].Buffer = (VOID *)(&Buffer[BufferSize]);
        BufferSize                   += SwitchStackData[Index].BufferSize;
        DEBUG ((
          DEBUG_INFO,
          "Buffer[cpu%lu] for InitializeExceptionStackSwitchHandlers: 0x%lX with size 0x%lX\n",
          (UINT64)(UINTN)Index,
          (UINT64)(UINTN)SwitchStackData[Index].Buffer,
          (UINT64)(UINTN)SwitchStackData[Index].BufferSize
          ));
      }
    }

    for (Index = 0; Index < mNumberOfProcessors; ++Index) {
      CpuSwitchStackData = SwitchStackData + Index;
      if (Index == BspProcessorNum) {
        InitializeExceptionStackSwitchHandlers ((VOID *)CpuSwitchStackData);
        continue;
      }

      Status = MpServicesUnitTestStartupThisAP (
                 MpServices,
                 InitializeExceptionStackSwitchHandlers,
                 Index,
                 0,
                 (VOID *)CpuSwitchStackData
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
  Test if stack overflow is captured by CpuStackGuard in Bsp and AP.

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
  UINT64                          CpuInfoInHobAddress;
  CPU_INFO_IN_HOB                 *CpuInfoInHob;
  UINTN                           StackBase;
  UINTN                           ApStackTop;
  UINTN                           ApStackBase;
  UINTN                           NewStackTop;
  UINTN                           NewStackBase;
  EXCEPTION_STACK_SWITCH_CONTEXT  *SwitchStackData;
  MP_SERVICES                     MpServices;
  UINTN                           ProcessorNumber;
  UINTN                           EnabledProcessorNum;
  CPU_ORIGINAL_REGISTER_BUFFER    *CpuOriginalRegisterBuffer;
  UINTN                           Index;
  UINTN                           BspProcessorNum;
  VOID                            *NewIdtr;

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

  CpuOriginalRegisterBuffer = StoreAllCpuOriginRegister (&MpServices, BspProcessorNum);

  //
  // Initialize Bsp and AP Idt.
  // Idt buffer should not be empty or it will hang in MP API.
  //
  NewIdtr = InitializeBspIdt ();
  Status  = InitializeCpuExceptionHandlers (NULL);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  InitializeApIdt (MpServices, NewIdtr);

  //
  // Get BSP stack base and CpuInfoInHob address.
  //
  StackBase           = 0;
  CpuInfoInHobAddress = 0;
  GetCpuStackInfo (&StackBase, &CpuInfoInHobAddress);
  CpuInfoInHob = (CPU_INFO_IN_HOB *)(UINTN)CpuInfoInHobAddress;

  //
  // InitializeMpExceptionStackSwitchHandlers and register exception handler.
  //
  SwitchStackData = InitializeMpExceptionStackSwitchHandlers (MpServices, BspProcessorNum);
  Status          = RegisterCpuInterruptHandler (EXCEPT_IA32_PAGE_FAULT, CpuStackGuardExceptionHandler);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  Status = RegisterCpuInterruptHandler (EXCEPT_IA32_DOUBLE_FAULT, ErrorCodeFaultExceptionHandler);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);

  for (Index = 0; Index < mNumberOfProcessors; Index++) {
    NewStackTop  = (UINTN)(SwitchStackData[Index].Buffer) + SwitchStackData[Index].BufferSize;
    NewStackBase = (UINTN)(SwitchStackData[Index].Buffer);
    if ((Index == BspProcessorNum) || (mNumberOfProcessors == 1)) {
      TriggerStackOverflowbyCpuStackGuard ();
      DEBUG ((DEBUG_INFO, "TestCase4: mRspAddress[0] is 0x%x, mRspAddress[1] is 0x%x\n", mRspAddress[0], mRspAddress[1]));
      UT_ASSERT_TRUE ((mRspAddress[0] >= StackBase) && (mRspAddress[0] <= (StackBase + SIZE_4KB)));
      UT_ASSERT_TRUE ((mRspAddress[1] >= NewStackBase) && (mRspAddress[1] < NewStackTop));
      continue;
    }

    ApStackTop = (UINTN)CpuInfoInHob[Index].ApTopOfStack;
    ASSERT (ApStackTop != 0);
    ApStackBase = ApStackTop - PcdGet32 (PcdCpuApStackSize);
    MpServicesUnitTestStartupThisAP (
      MpServices,
      (EFI_AP_PROCEDURE)TriggerStackOverflowbyCpuStackGuard,
      Index,
      0,
      NULL
      );
    DEBUG ((DEBUG_INFO, "TestCase4: mRspAddress[0] is 0x%x, mRspAddress[1] is 0x%x\n", mRspAddress[0], mRspAddress[1]));
    UT_ASSERT_TRUE ((mRspAddress[0] >= ApStackBase) && (mRspAddress[0] <= ApStackTop));
    UT_ASSERT_TRUE ((mRspAddress[1] >= NewStackBase) && (mRspAddress[1] < NewStackTop));
  }

  Status = RegisterCpuInterruptHandler (EXCEPT_IA32_PAGE_FAULT, NULL);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  Status = RegisterCpuInterruptHandler (EXCEPT_IA32_DOUBLE_FAULT, NULL);
  UT_ASSERT_EQUAL (Status, EFI_SUCCESS);
  RestoreAllCpuOriginRegister (&MpServices, CpuOriginalRegisterBuffer, BspProcessorNum);
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
  AddTestCase (CpuExceptionLibUnitTestSuite, "Check if exception handler can be registered/unregistered for exception with error code", "TestRegisterHandlerForExceptionWithErrorCode", TestRegisterHandlerForExceptionWithErrorCode, NULL, NULL, NULL);

  AddTestCase (CpuExceptionLibUnitTestSuite, "Check if Cpu Context is consistent before and after exception.", "TestCpuContextConsistency", TestCpuContextConsistency, NULL, NULL, NULL);
  AddTestCase (CpuExceptionLibUnitTestSuite, "Check if stack overflow is captured by CpuStackGuard in Bsp and AP", "TestCpuStackGuardInBspAndAp", TestCpuStackGuardInBspAndAp, NULL, NULL, NULL);

  return EFI_SUCCESS;
}
