/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Four test cases are created in this Unit Test module.
  a.Test if exception handler can be registered/unregistered for no error code exception
    In this test case, only no error code exception is triggered and tested by INTn instruction.
    The special hanlder for these exception will modify a global variable for check.

  b.Test if exception handler can be registered/unregistered for GP and PF.
    In this test case, GP exception is triggered and tested by setting CR4_RESERVED_BIT to 1.
    PF exception is triggered and tested by writting to not-present or RO addres.
    The special hanlder for these exceptions will set a global vartiable for check and adjust Rip to return from fault exception.

  c.Test if Cpu Context is consistent before and after exception.
    In this test case:
      1. Set Cpu register to mExpectedContextInHandler before exception.
      2. Trigger exception specified by ExceptionType.
      3. Store SystemContext in mActualContextInHandler and set SystemContext to mExpectedContextAfterException in handler.
      4. After return from exception, store Cpu registers in mActualContextAfterException.
    The expectation is:
      1. Register values in mActualContextInHandler are the same with register values in mExpectedContextInHandler.
      2. Register values in mActualContextAfterException are the same with register values mActualContextAfterException.

  d.Test if stack overflow can be captured by CpuStackGuard in both Bsp and AP.
    In this test case, stack overflow is triggered by a funtion which calls itself continuously. This test case triggers stack
    overflow in both BSP and AP. All AP use same Idt with Bsp. The expectation is:
      1. PF exception is triggered (leading to a DF if sepereated stack is not prepared for PF) when Rsp <= StackBase + SIZE_4KB
         since [StackBase, StackBase + SIZE_4KB] is marked as not present in page table when PcdCpuStackGuard is TRUE.
      2. Stack for PF/DF exception handler in both Bsp and AP is succussfully switched by InitializeSeparateExceptionStacks.

**/

#ifndef CPU_EXCEPTION_HANDLER_TEST_H_
#define CPU_EXCEPTION_HANDLER_TEST_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestHostBaseLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/UefiLib.h>
#include <Library/SerialPortLib.h>
#include <Library/HobLib.h>
#include <Library/CpuPageTableLib.h>
#include <Guid/MemoryAllocationHob.h>
#include <Protocol/MpService.h>
#include <PiPei.h>
#include <Ppi/MpServices2.h>

#define UNIT_TEST_APP_NAME     "Cpu Exception Handler Lib Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

#define  CPU_INTERRUPT_NUM       256
#define  SPEC_MAX_EXCEPTION_NUM  22
#define  CR4_RESERVED_BIT        BIT15

typedef struct {
  IA32_DESCRIPTOR    OriginalGdtr;
  IA32_DESCRIPTOR    OriginalIdtr;
  UINT16             Tr;
} CPU_REGISTER_BUFFER;

typedef union {
  EDKII_PEI_MP_SERVICES2_PPI    *Ppi;
  EFI_MP_SERVICES_PROTOCOL      *Protocol;
} MP_SERVICES;

typedef struct {
  VOID          *Buffer;
  UINTN         BufferSize;
  EFI_STATUS    Status;
} EXCEPTION_STACK_SWITCH_CONTEXT;

typedef struct {
  UINT64    Rdi;
  UINT64    Rsi;
  UINT64    Rbx;
  UINT64    Rdx;
  UINT64    Rcx;
  UINT64    Rax;
  UINT64    R8;
  UINT64    R9;
  UINT64    R10;
  UINT64    R11;
  UINT64    R12;
  UINT64    R13;
  UINT64    R14;
  UINT64    R15;
} GENERAL_REGISTER;

typedef struct {
  UINT32    Edi;
  UINT32    Esi;
  UINT32    Ebx;
  UINT32    Edx;
  UINT32    Ecx;
  UINT32    Eax;
} GENERAL_REGISTER_IA32;

extern UINTN               mFaultInstructionLength;
extern EFI_EXCEPTION_TYPE  mExceptionType;
extern UINTN               mRspAddress[];

/**
  Initialize Bsp Idt with a new Idt table and return the IA32_DESCRIPTOR buffer.
  In PEIM, store original PeiServicePointer before new Idt table.

  @return Pointer to the allocated IA32_DESCRIPTOR buffer.
**/
VOID *
InitializeBspIdt (
  VOID
  );

/**
  Trigger no error code exception by INT n instruction.

  @param[in]  ExceptionType  No error code exception type.
**/
VOID
EFIAPI
TriggerINTnException (
  IN  EFI_EXCEPTION_TYPE  ExceptionType
  );

/**
  Trigger GP exception by setting CR4_RESERVED_BIT to 1.

  @param[in]  Cr4ReservedBit  Cr4 reserved bit.
**/
VOID
EFIAPI
TriggerGPException (
  UINTN  Cr4ReservedBit
  );

/**
  Trigger PF exception by write to not present or ReadOnly address.

  @param[in]  PFAddress  Not present or ReadOnly address in page table.
**/
VOID
EFIAPI
TriggerPFException (
  UINTN  PFAddress
  );

/**
  Special handler for fault exception.
  This handler sets Rip/Eip in SystemContext to the instruction address after the exception instruction.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
AdjustRipForFaultHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
  Test consistency of Cpu context. Four steps:
  1. Set Cpu register to mExpectedContextInHandler before exception.
  2. Trigger exception specified by ExceptionType.
  3. Store SystemContext in mActualContextInHandler and set SystemContext to mExpectedContextAfterException in handler.
  4. After return from exception, store Cpu registers in mActualContextAfterException.

  Rcx/Ecx in mExpectedContextInHandler is decided by different exception type runtime since Rcx/Ecx is needed in assembly code.
  For GP and PF, Rcx/Ecx is set to FaultParameter. For other exception triggered by INTn, Rcx/Ecx is set to ExceptionType.

  @param[in] ExceptionType   Exception type.
  @param[in] FaultParameter  Parameter for GP and PF. OPTIONAL
**/
VOID
EFIAPI
AsmTestConsistencyOfCpuContext (
  IN  EFI_EXCEPTION_TYPE  ExceptionType,
  IN  UINTN               FaultParameter   OPTIONAL
  );

/**
  Special handler for ConsistencyOfCpuContext test case. General register in SystemContext
  is modified to mExpectedContextInHandler in this handler.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
AdjustCpuContextHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

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
  );

/**
  Get EFI_MP_SERVICES_PROTOCOL/EDKII_PEI_MP_SERVICES2_PPI pointer.

  @param[out] MpServices    Pointer to the MP_SERVICES buffer

  @retval EFI_SUCCESS       EFI_MP_SERVICES_PROTOCOL/PPI interface is returned
  @retval EFI_NOT_FOUND     EFI_MP_SERVICES_PROTOCOL/PPI interface is not found
**/
EFI_STATUS
GetMpServices (
  OUT MP_SERVICES  *MpServices
  );

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
  );

/**
  Execute a caller provided function on all enabled APs.

  @param[in]  MpServices    MP_SERVICES structure.
  @param[in]  Procedure     Pointer to the function to be run on enabled APs of the system.
  @param[in]  SingleThread  If TRUE, then all the enabled APs execute the function specified by Procedure
                            one by one, in ascending order of processor handle number.
                            If FALSE, then all the enabled APs execute the function specified by Procedure
                            simultaneously.
  @param[in]  TimeoutInMicroseconds Indicates the time limit in microseconds for APs to return from Procedure,
                                    for blocking mode only. Zero means infinity.
  @param[in]  ProcedureArgument     The parameter passed into Procedure for all APs.

  @retval EFI_SUCCESS       Execute a caller provided function on all enabled APs successfully
  @retval Others            Execute a caller provided function on all enabled APs unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestStartupAllAPs (
  IN MP_SERVICES       MpServices,
  IN EFI_AP_PROCEDURE  Procedure,
  IN BOOLEAN           SingleThread,
  IN UINTN             TimeoutInMicroSeconds,
  IN VOID              *ProcedureArgument
  );

/**
  Caller gets one enabled AP to execute a caller-provided function.

  @param[in]  MpServices    MP_SERVICES structure.
  @param[in]  Procedure     Pointer to the function to be run on enabled APs of the system.
  @param[in]  ProcessorNumber       The handle number of the AP.
  @param[in]  TimeoutInMicroseconds Indicates the time limit in microseconds for APs to return from Procedure,
                                    for blocking mode only. Zero means infinity.
  @param[in]  ProcedureArgument     The parameter passed into Procedure for all APs.


  @retval EFI_SUCCESS       Caller gets one enabled AP to execute a caller-provided function successfully
  @retval Others            Caller gets one enabled AP to execute a caller-provided function unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestStartupThisAP (
  IN MP_SERVICES       MpServices,
  IN EFI_AP_PROCEDURE  Procedure,
  IN UINTN             ProcessorNumber,
  IN UINTN             TimeoutInMicroSeconds,
  IN VOID              *ProcedureArgument
  );

/**
  Get the handle number for the calling processor.

  @param[in]  MpServices      MP_SERVICES structure.
  @param[out] ProcessorNumber The handle number for the calling processor.

  @retval EFI_SUCCESS       Get the handle number for the calling processor successfully.
  @retval Others            Get the handle number for the calling processor unsuccessfully.
**/
EFI_STATUS
MpServicesUnitTestWhoAmI (
  IN MP_SERVICES  MpServices,
  OUT UINTN       *ProcessorNumber
  );

/**
  Retrieve the number of logical processor in the platform and the number of those logical processors that
  are enabled on this boot.

  @param[in]  MpServices          MP_SERVICES structure.
  @param[out] NumberOfProcessors  Pointer to the total number of logical processors in the system, including
                                  the BSP and disabled APs.
  @param[out] NumberOfEnabledProcessors Pointer to the number of processors in the system that are enabled.

  @retval EFI_SUCCESS       Retrieve the number of logical processor successfully
  @retval Others            Retrieve the number of logical processor unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestGetNumberOfProcessors (
  IN MP_SERVICES  MpServices,
  OUT UINTN       *NumberOfProcessors,
  OUT UINTN       *NumberOfEnabledProcessors
  );

/**
  Trigger stack overflow by calling itself continuously.
**/
VOID
EFIAPI
TriggerStackOverflow (
  VOID
  );

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
  );

#endif
