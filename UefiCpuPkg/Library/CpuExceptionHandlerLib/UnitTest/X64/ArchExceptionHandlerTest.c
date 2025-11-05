/** @file
  Unit tests of the CpuExceptionHandlerLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuExceptionHandlerTest.h"

GENERAL_REGISTER  mActualContextInHandler;
GENERAL_REGISTER  mActualContextAfterException;

//
// In TestCpuContextConsistency, Cpu registers will be set to mExpectedContextInHandler/mExpectedContextAfterException.
// Rcx in mExpectedContextInHandler is set runtime since Rcx is needed in assembly code.
// For GP and PF, Rcx is set to FaultParameter. For other exception triggered by INTn, Rcx is set to ExceptionType.
//
GENERAL_REGISTER  mExpectedContextInHandler      = { 1, 2, 3, 4, 5, 0, 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe };
GENERAL_REGISTER  mExpectedContextAfterException = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e };

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
  mExceptionType                       = ExceptionType;
  SystemContext.SystemContextX64->Rip += mFaultInstructionLength;
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
  // Store SystemContext in mActualContextInHandler.
  //
  mActualContextInHandler.Rdi = SystemContext.SystemContextX64->Rdi;
  mActualContextInHandler.Rsi = SystemContext.SystemContextX64->Rsi;
  mActualContextInHandler.Rbx = SystemContext.SystemContextX64->Rbx;
  mActualContextInHandler.Rdx = SystemContext.SystemContextX64->Rdx;
  mActualContextInHandler.Rcx = SystemContext.SystemContextX64->Rcx;
  mActualContextInHandler.Rax = SystemContext.SystemContextX64->Rax;
  mActualContextInHandler.R8  = SystemContext.SystemContextX64->R8;
  mActualContextInHandler.R9  = SystemContext.SystemContextX64->R9;
  mActualContextInHandler.R10 = SystemContext.SystemContextX64->R10;
  mActualContextInHandler.R11 = SystemContext.SystemContextX64->R11;
  mActualContextInHandler.R12 = SystemContext.SystemContextX64->R12;
  mActualContextInHandler.R13 = SystemContext.SystemContextX64->R13;
  mActualContextInHandler.R14 = SystemContext.SystemContextX64->R14;
  mActualContextInHandler.R15 = SystemContext.SystemContextX64->R15;

  //
  // Modify cpu context. These registers will be stored in mActualContextAfterException.
  // Do not handle Rsp and Rbp. CpuExceptionHandlerLib doesn't set Rsp and Rbp register
  // to the value in SystemContext.
  //
  SystemContext.SystemContextX64->Rdi = mExpectedContextAfterException.Rdi;
  SystemContext.SystemContextX64->Rsi = mExpectedContextAfterException.Rsi;
  SystemContext.SystemContextX64->Rbx = mExpectedContextAfterException.Rbx;
  SystemContext.SystemContextX64->Rdx = mExpectedContextAfterException.Rdx;
  SystemContext.SystemContextX64->Rcx = mExpectedContextAfterException.Rcx;
  SystemContext.SystemContextX64->Rax = mExpectedContextAfterException.Rax;
  SystemContext.SystemContextX64->R8  = mExpectedContextAfterException.R8;
  SystemContext.SystemContextX64->R9  = mExpectedContextAfterException.R9;
  SystemContext.SystemContextX64->R10 = mExpectedContextAfterException.R10;
  SystemContext.SystemContextX64->R11 = mExpectedContextAfterException.R11;
  SystemContext.SystemContextX64->R12 = mExpectedContextAfterException.R12;
  SystemContext.SystemContextX64->R13 = mExpectedContextAfterException.R13;
  SystemContext.SystemContextX64->R14 = mExpectedContextAfterException.R14;
  SystemContext.SystemContextX64->R15 = mExpectedContextAfterException.R15;

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
  2.Compare mActualContextAfterException with mActualContextAfterException.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and it was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
CompareCpuContext (
  VOID
  )
{
  UT_ASSERT_EQUAL (mActualContextInHandler.Rdi, mExpectedContextInHandler.Rdi);
  UT_ASSERT_EQUAL (mActualContextInHandler.Rsi, mExpectedContextInHandler.Rsi);
  UT_ASSERT_EQUAL (mActualContextInHandler.Rbx, mExpectedContextInHandler.Rbx);
  UT_ASSERT_EQUAL (mActualContextInHandler.Rdx, mExpectedContextInHandler.Rdx);
  UT_ASSERT_EQUAL (mActualContextInHandler.Rcx, mExpectedContextInHandler.Rcx);
  UT_ASSERT_EQUAL (mActualContextInHandler.Rax, mExpectedContextInHandler.Rax);
  UT_ASSERT_EQUAL (mActualContextInHandler.R8, mExpectedContextInHandler.R8);
  UT_ASSERT_EQUAL (mActualContextInHandler.R9, mExpectedContextInHandler.R9);
  UT_ASSERT_EQUAL (mActualContextInHandler.R10, mExpectedContextInHandler.R10);
  UT_ASSERT_EQUAL (mActualContextInHandler.R11, mExpectedContextInHandler.R11);
  UT_ASSERT_EQUAL (mActualContextInHandler.R12, mExpectedContextInHandler.R12);
  UT_ASSERT_EQUAL (mActualContextInHandler.R13, mExpectedContextInHandler.R13);
  UT_ASSERT_EQUAL (mActualContextInHandler.R14, mExpectedContextInHandler.R14);
  UT_ASSERT_EQUAL (mActualContextInHandler.R15, mExpectedContextInHandler.R15);

  UT_ASSERT_EQUAL (mActualContextAfterException.Rdi, mExpectedContextAfterException.Rdi);
  UT_ASSERT_EQUAL (mActualContextAfterException.Rsi, mExpectedContextAfterException.Rsi);
  UT_ASSERT_EQUAL (mActualContextAfterException.Rbx, mExpectedContextAfterException.Rbx);
  UT_ASSERT_EQUAL (mActualContextAfterException.Rdx, mExpectedContextAfterException.Rdx);
  UT_ASSERT_EQUAL (mActualContextAfterException.Rcx, mExpectedContextAfterException.Rcx);
  UT_ASSERT_EQUAL (mActualContextAfterException.Rax, mExpectedContextAfterException.Rax);
  UT_ASSERT_EQUAL (mActualContextAfterException.R8, mExpectedContextAfterException.R8);
  UT_ASSERT_EQUAL (mActualContextAfterException.R9, mExpectedContextAfterException.R9);
  UT_ASSERT_EQUAL (mActualContextAfterException.R10, mExpectedContextAfterException.R10);
  UT_ASSERT_EQUAL (mActualContextAfterException.R11, mExpectedContextAfterException.R11);
  UT_ASSERT_EQUAL (mActualContextAfterException.R12, mExpectedContextAfterException.R12);
  UT_ASSERT_EQUAL (mActualContextAfterException.R13, mExpectedContextAfterException.R13);
  UT_ASSERT_EQUAL (mActualContextAfterException.R14, mExpectedContextAfterException.R14);
  UT_ASSERT_EQUAL (mActualContextAfterException.R15, mExpectedContextAfterException.R15);
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
  mRspAddress[0] = (UINTN)SystemContext.SystemContextX64->Rsp;
  mRspAddress[1] = (UINTN)(&LocalVariable);

  return;
}
