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
  SystemContext.SystemContextIa32->Eip += mFaultInstructionLength;
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
  mOriginIa32GeneralRegister[0].Edi = SystemContext.SystemContextIa32->Edi;
  mOriginIa32GeneralRegister[0].Esi = SystemContext.SystemContextIa32->Esi;
  mOriginIa32GeneralRegister[0].Ebx = SystemContext.SystemContextIa32->Ebx;
  mOriginIa32GeneralRegister[0].Edx = SystemContext.SystemContextIa32->Edx;
  mOriginIa32GeneralRegister[0].Ecx = SystemContext.SystemContextIa32->Ecx;
  mOriginIa32GeneralRegister[0].Eax = SystemContext.SystemContextIa32->Eax;

  //
  // Modify cpu context which will be stored in mOriginIa32GeneralRegister[1].
  //
  SystemContext.SystemContextIa32->Edi = 0x11;
  SystemContext.SystemContextIa32->Esi = 0x12;
  SystemContext.SystemContextIa32->Ebx = 0x14;
  SystemContext.SystemContextIa32->Edx = 0x15;
  SystemContext.SystemContextIa32->Ecx = 0x16;
  SystemContext.SystemContextIa32->Eax = 0x17;
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
  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[0].Edi, 1);
  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[0].Esi, 2);
  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[0].Ebx, 4);
  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[0].Edx, 5);
  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[0].Ecx, CxValue);
  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[0].Eax, 7);

  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[1].Edi, 0x11);
  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[1].Esi, 0x12);
  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[1].Ebx, 0x14);
  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[1].Edx, 0x15);
  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[1].Ecx, 0x16);
  UT_ASSERT_EQUAL (mOriginIa32GeneralRegister[1].Eax, 0x17);
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
  mRspAddress[0] = (UINTN)SystemContext.SystemContextIa32->Esp;
  mRspAddress[1] = (UINTN)(&LocalVariable);

  return;
}
