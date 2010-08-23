/** @file
Include file for PI MP Services Protocol Thunk.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

**/

#ifndef _MP_SERVICES_ON_FRAMEWORK_MP_SERVICES_THUNK_
#define _MP_SERVICES_ON_FRAMEWORK_MP_SERVICES_THUNK_

#include <Protocol/MpService.h>
#include <Protocol/FrameworkMpService.h>
#include <Protocol/GenericMemoryTest.h>

#include <Library/BaseLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/LocalApicLib.h>

#define AP_STACK_SIZE                         0x8000
#define MAX_CPU_NUMBER                        256

//
// Bit definition for IPI
//
#define BROADCAST_MODE_ALL_EXCLUDING_SELF_BIT  0xC0000
#define SPECIFY_CPU_MODE_BIT                   0x00000
#define TRIGGER_MODE_LEVEL_BIT                 0x08000
#define ASSERT_BIT                             0x04000

//
// Local APIC register definition for IPI.
//
#define APIC_REGISTER_SPURIOUS_VECTOR_OFFSET  0xF0
#define APIC_REGISTER_ICR_LOW_OFFSET          0x300
#define APIC_REGISTER_ICR_HIGH_OFFSET         0x310
#define APIC_REGISTER_LVT_TIMER               0x320
#define APIC_REGISTER_TIMER_INIT_COUNT        0x380
#define APIC_REGISTER_LINT0_VECTOR_OFFSET     0x350
#define APIC_REGISTER_LINT1_VECTOR_OFFSET     0x360
#define APIC_REGISTER_TIMER_COUNT             0x390
#define APIC_REGISTER_TIMER_DIVIDE            0x3E0

//
// Definition for MSR address
//
#define MSR_IA32_TIME_STAMP_COUNTER           0x10
#define MSR_IA32_APIC_BASE                    0x1B

typedef struct {
  UINTN             Lock;
  VOID              *StackStart;
  UINTN             StackSize;
  VOID              *ApFunction;
  IA32_DESCRIPTOR   GdtrProfile;
  IA32_DESCRIPTOR   IdtrProfile;
  UINT32            BufferStart;
  UINT32            Cr3;
  UINT32            ProcessorNumber[MAX_CPU_NUMBER];
} MP_CPU_EXCHANGE_INFO;

typedef struct {
  UINT8 *RendezvousFunnelAddress;
  UINTN PModeEntryOffset;
  UINTN FlatJumpOffset;
  UINTN LModeEntryOffset;
  UINTN LongJumpOffset;
  UINTN Size;
} MP_ASSEMBLY_ADDRESS_MAP;

typedef enum {
  CpuStateIdle,
  CpuStateReady,
  CpuStateBusy,
  CpuStateFinished,
  CpuStateDisabled
} CPU_STATE;

//
// Define Individual Processor Data block.
//
typedef struct {
  EFI_AP_PROCEDURE volatile      Procedure;
  VOID* volatile                 Parameter;

  EFI_EVENT                      WaitEvent;
  BOOLEAN                        *Finished;
  UINT64                         ExpectedTime;
  UINT64                         CurrentTime;
  UINT64                         TotalTime;

  SPIN_LOCK                      CpuDataLock;
  CPU_STATE volatile             State;

} CPU_DATA_BLOCK;

//
// Define MP data block which consumes individual processor block.
//
typedef struct {
  SPIN_LOCK                   APSerializeLock;

  EFI_EVENT                   CheckAPsEvent;

  UINTN                       FinishCount;
  UINTN                       StartCount;

  BOOLEAN                     CpuList[MAX_CPU_NUMBER];

  EFI_AP_PROCEDURE            Procedure;
  VOID                        *ProcArguments;
  BOOLEAN                     SingleThread;
  EFI_EVENT                   WaitEvent;
  UINTN                       **FailedCpuList;
  UINT64                      ExpectedTime;
  UINT64                      CurrentTime;
  UINT64                      TotalTime;

  CPU_DATA_BLOCK              CpuData[MAX_CPU_NUMBER];
} MP_SYSTEM_DATA;

/**
  Implementation of GetNumberOfProcessors() service of MP Services Protocol.

  This service retrieves the number of logical processor in the platform
  and the number of those logical processors that are enabled on this boot.
  This service may only be called from the BSP.

  @param  This                       A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  NumberOfProcessors         Pointer to the total number of logical processors in the system,
                                     including the BSP and disabled APs.
  @param  NumberOfEnabledProcessors  Pointer to the number of enabled logical processors that exist
                                     in system, including the BSP.

  @retval EFI_SUCCESS	             Number of logical processors and enabled logical processors retrieved..
  @retval EFI_DEVICE_ERROR           Caller processor is AP.
  @retval EFI_INVALID_PARAMETER      NumberOfProcessors is NULL
  @retval EFI_INVALID_PARAMETER      NumberOfEnabledProcessors is NULL

**/
EFI_STATUS
EFIAPI
GetNumberOfProcessors (
  IN  EFI_MP_SERVICES_PROTOCOL     *This,
  OUT UINTN                        *NumberOfProcessors,
  OUT UINTN                        *NumberOfEnabledProcessors
  );

/**
  Implementation of GetNumberOfProcessors() service of MP Services Protocol.

  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber       The handle number of processor.
  @param  ProcessorInfoBuffer   A pointer to the buffer where information for the requested processor is deposited.

  @retval EFI_SUCCESS           Processor information successfully returned.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_INVALID_PARAMETER ProcessorInfoBuffer is NULL
  @retval EFI_NOT_FOUND         Processor with the handle specified by ProcessorNumber does not exist.

**/
EFI_STATUS
EFIAPI
GetProcessorInfo (
  IN       EFI_MP_SERVICES_PROTOCOL     *This,
  IN       UINTN                        ProcessorNumber,
  OUT      EFI_PROCESSOR_INFORMATION    *ProcessorInfoBuffer
  );

/**
  Implementation of StartupAllAPs() service of MP Services Protocol.

  This service lets the caller get all enabled APs to execute a caller-provided function.
  This service may only be called from the BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  Procedure             A pointer to the function to be run on enabled APs of the system.
  @param  SingleThread          Indicates whether to execute the function simultaneously or one by one..
  @param  WaitEvent             The event created by the caller.
                                If it is NULL, then execute in blocking mode.
                                If it is not NULL, then execute in non-blocking mode.
  @param  TimeoutInMicroSeconds The time limit in microseconds for this AP to finish the function.
                                Zero means infinity.
  @param  ProcedureArgument     Pointer to the optional parameter of the assigned function.
  @param  FailedCpuList         The list of processor numbers that fail to finish the function before
                                TimeoutInMicrosecsond expires.

  @retval EFI_SUCCESS           In blocking mode, all APs have finished before the timeout expired.
  @retval EFI_SUCCESS           In non-blocking mode, function has been dispatched to all enabled APs.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_NOT_STARTED       No enabled AP exists in the system.
  @retval EFI_NOT_READY         Any enabled AP is busy.
  @retval EFI_TIMEOUT           In blocking mode, The timeout expired before all enabled APs have finished.
  @retval EFI_INVALID_PARAMETER Procedure is NULL.

**/
EFI_STATUS
EFIAPI
StartupAllAPs (
  IN  EFI_MP_SERVICES_PROTOCOL   *This,
  IN  EFI_AP_PROCEDURE           Procedure,
  IN  BOOLEAN                    SingleThread,
  IN  EFI_EVENT                  WaitEvent             OPTIONAL,
  IN  UINTN                      TimeoutInMicroSeconds,
  IN  VOID                       *ProcedureArgument    OPTIONAL,
  OUT UINTN                      **FailedCpuList       OPTIONAL
  );

/**
  Implementation of StartupThisAP() service of MP Services Protocol.

  This service lets the caller get one enabled AP to execute a caller-provided function.
  This service may only be called from the BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  Procedure             A pointer to the function to be run on the designated AP.
  @param  ProcessorNumber       The handle number of AP..
  @param  WaitEvent             The event created by the caller.
                                If it is NULL, then execute in blocking mode.
                                If it is not NULL, then execute in non-blocking mode.
  @param  TimeoutInMicroseconds The time limit in microseconds for this AP to finish the function.
                                Zero means infinity.
  @param  ProcedureArgument     Pointer to the optional parameter of the assigned function.
  @param  Finished              Indicates whether AP has finished assigned function.
                                In blocking mode, it is ignored.

  @retval EFI_SUCCESS           In blocking mode, specified AP has finished before the timeout expires.
  @retval EFI_SUCCESS           In non-blocking mode, function has been dispatched to specified AP.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_TIMEOUT           In blocking mode, the timeout expires before specified AP has finished.
  @retval EFI_NOT_READY         Specified AP is busy.
  @retval EFI_NOT_FOUND         Processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_INVALID_PARAMETER Procedure is NULL.

**/
EFI_STATUS
EFIAPI
StartupThisAP (
  IN  EFI_MP_SERVICES_PROTOCOL   *This,
  IN  EFI_AP_PROCEDURE           Procedure,
  IN  UINTN                      ProcessorNumber,
  IN  EFI_EVENT                  WaitEvent OPTIONAL,
  IN  UINTN                      TimeoutInMicroseconds,
  IN  VOID                       *ProcedureArgument OPTIONAL,
  OUT BOOLEAN                    *Finished OPTIONAL
  );

/**
  Implementation of SwitchBSP() service of MP Services Protocol.

  This service switches the requested AP to be the BSP from that point onward.
  This service may only be called from the current BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber       The handle number of processor.
  @param  EnableOldBSP          Whether to enable or disable the original BSP.

  @retval EFI_SUCCESS           BSP successfully switched.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_NOT_FOUND         Processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_NOT_READY         Specified AP is busy.

**/
EFI_STATUS
EFIAPI
SwitchBSP (
  IN  EFI_MP_SERVICES_PROTOCOL            *This,
  IN  UINTN                               ProcessorNumber,
  IN  BOOLEAN                             EnableOldBSP
  );

/**
  Implementation of EnableDisableAP() service of MP Services Protocol.

  This service lets the caller enable or disable an AP.
  This service may only be called from the BSP.

  @param  This                   A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber        The handle number of processor.
  @param  EnableAP               Indicates whether the newstate of the AP is enabled or disabled.
  @param  HealthFlag             Indicates new health state of the AP..

  @retval EFI_SUCCESS            AP successfully enabled or disabled.
  @retval EFI_DEVICE_ERROR       Caller processor is AP.
  @retval EFI_NOT_FOUND          Processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETERS ProcessorNumber specifies the BSP.

**/
EFI_STATUS
EFIAPI
EnableDisableAP (
  IN  EFI_MP_SERVICES_PROTOCOL            *This,
  IN  UINTN                               ProcessorNumber,
  IN  BOOLEAN                             EnableAP,
  IN  UINT32                              *HealthFlag OPTIONAL
  );

/**
  Implementation of WhoAmI() service of MP Services Protocol.

  This service lets the caller processor get its handle number.
  This service may be called from the BSP and APs.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber       Pointer to the handle number of AP.

  @retval EFI_SUCCESS           Processor number successfully returned.
  @retval EFI_INVALID_PARAMETER ProcessorNumber is NULL

**/
EFI_STATUS
EFIAPI
WhoAmI (
  IN  EFI_MP_SERVICES_PROTOCOL            *This,
  OUT UINTN                               *ProcessorNumber
  );

/**
  Checks APs' status periodically.

  This function is triggerred by timer perodically to check the
  state of APs for StartupAllAPs() and StartupThisAP() executed
  in non-blocking mode.

  @param  Event    Event triggered.
  @param  Context  Parameter passed with the event.

**/
VOID
EFIAPI
CheckAPsStatus (
  IN  EFI_EVENT                           Event,
  IN  VOID                                *Context
  );

/**
  Checks status of all APs.

  This function checks whether all APs have finished task assigned by StartupAllAPs(),
  and whether timeout expires.

  @retval EFI_SUCCESS           All APs have finished task assigned by StartupAllAPs().
  @retval EFI_TIMEOUT           The timeout expires.
  @retval EFI_NOT_READY         APs have not finished task and timeout has not expired.

**/
EFI_STATUS
CheckAllAPs (
  VOID
  );

/**
  Checks status of specified AP.

  This function checks whether specified AP has finished task assigned by StartupThisAP(),
  and whether timeout expires.

  @param  ProcessorNumber       The handle number of processor.

  @retval EFI_SUCCESS           Specified AP has finished task assigned by StartupThisAPs().
  @retval EFI_TIMEOUT           The timeout expires.
  @retval EFI_NOT_READY         Specified AP has not finished task and timeout has not expired.

**/
EFI_STATUS
CheckThisAP (
  UINTN  ProcessorNumber
  );

/**
  Calculate timeout value and return the current performance counter value.

  Calculate the number of performance counter ticks required for a timeout.
  If TimeoutInMicroseconds is 0, return value is also 0, which is recognized
  as infinity.

  @param  TimeoutInMicroseconds   Timeout value in microseconds.
  @param  CurrentTime             Returns the current value of the performance counter.

  @return Expected timestamp counter for timeout.
          If TimeoutInMicroseconds is 0, return value is also 0, which is recognized
          as infinity.

**/
UINT64
CalculateTimeout (
  IN  UINTN   TimeoutInMicroseconds,
  OUT UINT64  *CurrentTime
  );

/**
  Checks whether timeout expires.

  Check whether the number of ellapsed performance counter ticks required for a timeout condition
  has been reached.  If Timeout is zero, which means infinity, return value is always FALSE.

  @param  PreviousTime         On input, the value of the performance counter when it was last read.
                               On output, the current value of the performance counter
  @param  TotalTime            The total amount of ellapsed time in performance counter ticks.
  @param  Timeout              The number of performance counter ticks required to reach a timeout condition.

  @retval TRUE                 A timeout condition has been reached.
  @retval FALSE                A timeout condition has not been reached.

**/
BOOLEAN
CheckTimeout (
  IN OUT UINT64  *PreviousTime,
  IN     UINT64  *TotalTime,
  IN     UINT64  Timeout
  );

/**
  Searches for the next waiting AP.

  Search for the next AP that is put in waiting state by single-threaded StartupAllAPs().

  @param  NextProcessorNumber  Pointer to the processor number of the next waiting AP.

  @retval EFI_SUCCESS          The next waiting AP has been found.
  @retval EFI_NOT_FOUND        No waiting AP exists.

**/
EFI_STATUS
GetNextWaitingProcessorNumber (
  OUT UINTN                               *NextProcessorNumber
  );

/**
  Wrapper function for all procedures assigned to AP.

  Wrapper function for all procedures assigned to AP via MP service protocol.
  It controls states of AP and invokes assigned precedure.

**/
VOID
ApProcWrapper (
  VOID
  );

/**
  Function to wake up a specified AP and assign procedure to it.

  @param  ProcessorNumber  Handle number of the specified processor.
  @param  Procedure        Procedure to assign.
  @param  ProcArguments    Argument for Procedure.

**/
VOID
WakeUpAp (
  IN   UINTN                 ProcessorNumber,
  IN   EFI_AP_PROCEDURE      Procedure,
  IN   VOID                  *ProcArguments
  );

/**
  Terminate AP's task and set it to idle state.

  This function terminates AP's task due to timeout by sending INIT-SIPI,
  and sends it to idle state.

  @param  ProcessorNumber  Handle number of the specified processor.

**/
VOID
ResetProcessorToIdleState (
  UINTN      ProcessorNumber
  );

/**
  Worker function of EnableDisableAP ()

  Worker function of EnableDisableAP (). Changes state of specified processor.

  @param  ProcessorNumber Processor number of specified AP.
  @param  NewState        Desired state of the specified AP.

  @retval EFI_SUCCESS     AP's state successfully changed.

**/
EFI_STATUS
ChangeCpuState (
  IN     UINTN                      ProcessorNumber,
  IN     BOOLEAN                    NewState
  );

/**
  Gets the processor number of BSP.

  @return  The processor number of BSP.

**/
UINTN
GetBspNumber (
  VOID
  );

/**
  Get address map of RendezvousFunnelProc.

  This function gets address map of RendezvousFunnelProc.

  @param  AddressMap   Output buffer for address map information

**/
VOID
AsmGetAddressMap (
  OUT MP_ASSEMBLY_ADDRESS_MAP    *AddressMap
  );

#endif
