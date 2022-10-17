/** @file
  Unit tests of the CpuExceptionHandlerLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuExceptionHandlerTest.h"

GENERAL_REGISTER_IA32  mActualContextInHandler;
GENERAL_REGISTER_IA32  mActualContextAfterException;

//
// In TestCpuContextConsistency, Cpu registers will be set to mExpectedContextInHandler/mExpectedContextAfterException.
// Ecx in mExpectedContextInHandler is set runtime since Ecx is needed in assembly code.
// For GP and PF, Ecx is set to FaultParameter. For other exception triggered by INTn, Ecx is set to ExceptionType.
//
GENERAL_REGISTER_IA32  mExpectedContextInHandler      = { 1, 2, 3, 4, 5, 0 };
GENERAL_REGISTER_IA32  mExpectedContextAfterException = { 11, 12, 13, 14, 15, 16 };

/**
  Special handler for fault exception.
  Rip/Eip in SystemContext will be modified to the instruction after the exception instruction.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
EFIAPI
AdjustRipForFaultHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  mExceptionType                        = ExceptionType;
  SystemContext.SystemContextIa32->Eip += mFaultInstructionLength;
}

/**
  Special handler for ConsistencyOfCpuContext test case.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
AdjustCpuContextHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  //
  // Store SystemContext in exception handler.
  //
  mActualContextInHandler.Edi = SystemContext.SystemContextIa32->Edi;
  mActualContextInHandler.Esi = SystemContext.SystemContextIa32->Esi;
  mActualContextInHandler.Ebx = SystemContext.SystemContextIa32->Ebx;
  mActualContextInHandler.Edx = SystemContext.SystemContextIa32->Edx;
  mActualContextInHandler.Ecx = SystemContext.SystemContextIa32->Ecx;
  mActualContextInHandler.Eax = SystemContext.SystemContextIa32->Eax;

  //
  // Modify cpu context. These registers will be stored in mActualContextAfterException.
  // Do not handle Esp and Ebp in SystemContext. CpuExceptionHandlerLib doesn't set Esp and
  // Esp register to the value in SystemContext.
  //
  SystemContext.SystemContextIa32->Edi = mExpectedContextAfterException.Edi;
  SystemContext.SystemContextIa32->Esi = mExpectedContextAfterException.Esi;
  SystemContext.SystemContextIa32->Ebx = mExpectedContextAfterException.Ebx;
  SystemContext.SystemContextIa32->Edx = mExpectedContextAfterException.Edx;
  SystemContext.SystemContextIa32->Ecx = mExpectedContextAfterException.Ecx;
  SystemContext.SystemContextIa32->Eax = mExpectedContextAfterException.Eax;

  //
  // When fault exception happens, eip/rip points to the faulting instruction.
  // For now, olny GP and PF are tested in fault exception.
  //
  if ((ExceptionType == EXCEPT_IA32_PAGE_FAULT) || (ExceptionType == EXCEPT_IA32_GP_FAULT)) {
    AdjustRipForFaultHandler (ExceptionType, SystemContext);
  }
}

/**
  Compare cpu context in ConsistencyOfCpuContext test case.
  1.Compare mActualContextInHandler with mExpectedContextInHandler.
  2.Compare mActualContextAfterException with mExpectedContextAfterException.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and it was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
CompareCpuContext (
  VOID
  )
{
  UT_ASSERT_EQUAL (mActualContextInHandler.Edi, mExpectedContextInHandler.Edi);
  UT_ASSERT_EQUAL (mActualContextInHandler.Esi, mExpectedContextInHandler.Esi);
  UT_ASSERT_EQUAL (mActualContextInHandler.Ebx, mExpectedContextInHandler.Ebx);
  UT_ASSERT_EQUAL (mActualContextInHandler.Edx, mExpectedContextInHandler.Edx);
  UT_ASSERT_EQUAL (mActualContextInHandler.Ecx, mExpectedContextInHandler.Ecx);
  UT_ASSERT_EQUAL (mActualContextInHandler.Eax, mExpectedContextInHandler.Eax);

  UT_ASSERT_EQUAL (mActualContextAfterException.Edi, mExpectedContextAfterException.Edi);
  UT_ASSERT_EQUAL (mActualContextAfterException.Esi, mExpectedContextAfterException.Esi);
  UT_ASSERT_EQUAL (mActualContextAfterException.Ebx, mExpectedContextAfterException.Ebx);
  UT_ASSERT_EQUAL (mActualContextAfterException.Edx, mExpectedContextAfterException.Edx);
  UT_ASSERT_EQUAL (mActualContextAfterException.Ecx, mExpectedContextAfterException.Ecx);
  UT_ASSERT_EQUAL (mActualContextAfterException.Eax, mExpectedContextAfterException.Eax);
  return UNIT_TEST_PASSED;
}

/**
  Special handler for CpuStackGuard test case.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
EFIAPI
CpuStackGuardExceptionHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINTN  LocalVariable;

  AdjustRipForFaultHandler (ExceptionType, SystemContext);
  mRspAddress[0] = (UINTN)SystemContext.SystemContextIa32->Esp;
  mRspAddress[1] = (UINTN)(&LocalVariable);

  return;
}
