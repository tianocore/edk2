/** @file
  Unit tests of the CpuExceptionHandlerLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuExceptionHandlerTest.h"

/**
  Special handler for fault exception.
  Rip/Eip in SystemContext will be modified to the instruction after the exception instruction.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
ErrorCodeFaultExceptionHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  mExceptionType = ExceptionType;
  SerialPortInitialize ();
  DumpCpuContext (ExceptionType, SystemContext);
  SystemContext.SystemContextX64->Rip += mFaultInstructionLength;
  DumpCpuContext (ExceptionType, SystemContext);
}

/**
  Special handler for ConsistencyOfCpuContext test case.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
ConsistencyOfCpuContextExceptionHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  //
  // Do not handle Esp and ebp. They will not be restored.
  // Store SystemContext modified before exception.
  //
  mOriginGeneralRegister[0].Rdi         = SystemContext.SystemContextX64->Rdi;
  mOriginGeneralRegister[0].Rsi         = SystemContext.SystemContextX64->Rsi;
  mOriginGeneralRegister[0].Rbx         = SystemContext.SystemContextX64->Rbx;
  mOriginGeneralRegister[0].Rdx         = SystemContext.SystemContextX64->Rdx;
  mOriginGeneralRegister[0].Rcx         = SystemContext.SystemContextX64->Rcx;
  mOriginGeneralRegister[0].Rax         = SystemContext.SystemContextX64->Rax;
  mOriginGeneralRegister[0].R8Register  = SystemContext.SystemContextX64->R8;
  mOriginGeneralRegister[0].R9Register  = SystemContext.SystemContextX64->R9;
  mOriginGeneralRegister[0].R10Register = SystemContext.SystemContextX64->R10;
  mOriginGeneralRegister[0].R11Register = SystemContext.SystemContextX64->R11;
  mOriginGeneralRegister[0].R12Register = SystemContext.SystemContextX64->R12;
  mOriginGeneralRegister[0].R13Register = SystemContext.SystemContextX64->R13;
  mOriginGeneralRegister[0].R14Register = SystemContext.SystemContextX64->R14;
  mOriginGeneralRegister[0].R15Register = SystemContext.SystemContextX64->R15;

  //
  // Modify cpu context which will be stored in mOriginGeneralRegister[1].
  //
  SystemContext.SystemContextX64->Rdi = 0x11;
  SystemContext.SystemContextX64->Rsi = 0x12;
  SystemContext.SystemContextX64->Rbx = 0x14;
  SystemContext.SystemContextX64->Rdx = 0x15;
  SystemContext.SystemContextX64->Rcx = 0x16;
  SystemContext.SystemContextX64->Rax = 0x17;
  SystemContext.SystemContextX64->R8  = 0x18;
  SystemContext.SystemContextX64->R9  = 0x19;
  SystemContext.SystemContextX64->R10 = 0x1a;
  SystemContext.SystemContextX64->R11 = 0x1b;
  SystemContext.SystemContextX64->R12 = 0x1c;
  SystemContext.SystemContextX64->R13 = 0x1d;
  SystemContext.SystemContextX64->R14 = 0x1e;
  SystemContext.SystemContextX64->R15 = 0x1f;
}

/**
  Compare cpu context in ConsistencyOfCpuContext test case.
  Rcx/Ecx is set to CxValue before exception in AsmTestConsistencyOfCpuContext.

  @param[in] CxValue  Value of Rcx/Ecx before exception.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and it was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
CompareCpuContext (
  UINTN  CxValue
  )
{
  //
  // Do not handle Esp and ebp. They will not be restored.
  //
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].Rdi, 1);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].Rsi, 2);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].Rbx, 4);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].Rdx, 5);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].Rcx, CxValue);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].Rax, 7);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].R8Register, 8);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].R9Register, 9);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].R10Register, 0xa);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].R11Register, 0xb);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].R12Register, 0xc);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].R13Register, 0xd);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].R14Register, 0xe);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[0].R15Register, 0xf);

  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].Rdi, 0x11);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].Rsi, 0x12);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].Rbx, 0x14);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].Rdx, 0x15);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].Rcx, 0x16);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].Rax, 0x17);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].R8Register, 0x18);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].R9Register, 0x19);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].R10Register, 0x1a);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].R11Register, 0x1b);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].R12Register, 0x1c);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].R13Register, 0x1d);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].R14Register, 0x1e);
  UT_ASSERT_EQUAL (mOriginGeneralRegister[1].R15Register, 0x1f);
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

  ErrorCodeFaultExceptionHandler (ExceptionType, SystemContext);
  mRspAddress[0] = (UINTN)SystemContext.SystemContextX64->Rsp;
  mRspAddress[1] = (UINTN)(&LocalVariable);

  return;
}
