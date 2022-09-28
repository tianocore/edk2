/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
#include <Guid/MemoryAllocationHob.h>
#include <Protocol/MpService.h>
#include <PiPei.h>
#include <Ppi/MpServices2.h>

#define UNIT_TEST_APP_NAME     "Cpu Exception Handler Lib Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

#define  CPU_EXCEPTION_NUM       32
#define  CPU_INTERRUPT_NUM       256
#define  SPEC_MAX_EXCEPTION_NUM  22
#define  GP_CR4_RESERVED_BIT     BIT15

#define CPU_INIT_MP_LIB_HOB_GUID \
  { \
    0x58eb6a19, 0x3699, 0x4c68, { 0xa8, 0x36, 0xda, 0xcd, 0x8e, 0xdc, 0xad, 0x4a } \
  }

#pragma pack (1)

typedef union {
  struct {
    UINT32    LimitLow    : 16;
    UINT32    BaseLow     : 16;
    UINT32    BaseMid     : 8;
    UINT32    Type        : 4;
    UINT32    System      : 1;
    UINT32    Dpl         : 2;
    UINT32    Present     : 1;
    UINT32    LimitHigh   : 4;
    UINT32    Software    : 1;
    UINT32    Reserved    : 1;
    UINT32    DefaultSize : 1;
    UINT32    Granularity : 1;
    UINT32    BaseHigh    : 8;
  } Bits;
  UINT64    Uint64;
} IA32_GDT;

typedef struct {
  UINT32    InitialApicId;
  UINT32    ApicId;
  UINT32    Health;
  UINT64    ApTopOfStack;
} CPU_INFO_IN_HOB;
#pragma pack ()

typedef struct {
  IA32_DESCRIPTOR    OriginalGdtr;
  IA32_DESCRIPTOR    OriginalIdtr;
  UINT16             Tr;
} CPU_ORIGINAL_REGISTER_BUFFER;

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
  UINT64    Rbp;
  UINT64    Rbx;
  UINT64    Rdx;
  UINT64    Rcx;
  UINT64    Rax;
  UINT64    R8Register;
  UINT64    R9Register;
  UINT64    R10Register;
  UINT64    R11Register;
  UINT64    R12Register;
  UINT64    R13Register;
  UINT64    R14Register;
  UINT64    R15Register;
} GENERAL_REGISTER;

typedef struct {
  UINT32    Edi;
  UINT32    Esi;
  UINT32    Ebp;
  UINT32    Ebx;
  UINT32    Edx;
  UINT32    Ecx;
  UINT32    Eax;
} GENERAL_REGISTER_IA32;

extern BOOLEAN                mIsDxeDriver;
extern UINTN                  mFaultInstructionLength;
extern EFI_EXCEPTION_TYPE     mExceptionType;
extern UINTN                  mRspAddress[];
extern GENERAL_REGISTER       mOriginGeneralRegister[];
extern GENERAL_REGISTER_IA32  mOriginIa32GeneralRegister[];

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
  Trigger GP exception.

  @param[in]  Cr4ReservedBit  Cr4 reserved bit.
**/
VOID
EFIAPI
TriggerGPException (
  UINTN  Cr4ReservedBit
  );

/**
  Trigger PF exception.

  @param[in]  ReadOnlyAddr  ReadOnly address in page table.
**/
VOID
EFIAPI
TriggerPFException (
  UINTN  ReadOnlyAddr
  );

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
  );

/**
  Test consistency of Cpu context. Four steps:
  1. Push origin Cpu register and modify the Cpu register before exception.
  2. Trigger exception specified by ExceptionType.
  3. Store SystemContext in mOriginGeneralRegister[0] and modify SystemContext in registered exception handler.
  4. Store the Cpu register in mOriginGeneralRegister[1] and Pop origin register after exception.

  @param[in] ExceptionType         Exception type.
  @param[in] OptionalPeiStackBase  Guarded Pei stack base. OPTIONAL
**/
VOID
EFIAPI
AsmTestConsistencyOfCpuContext (
  IN  EFI_EXCEPTION_TYPE  ExceptionType,
  IN  UINTN               OptionalPeiStackBase   OPTIONAL
  );

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
  );

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

  @param[in]  MpServices      Pointer to MP_SERVICES structure.
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

  @param[in]  MpServices          Pointer to MP_SERVICES structure.
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
  Trigger stack overflow by CpuStackGuard.
**/
VOID
EFIAPI
TriggerStackOverflowbyCpuStackGuard (
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
