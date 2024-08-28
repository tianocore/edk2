/** @file
SMM MP protocol implementation

Copyright (c) 2019 - 2024, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"
#include "SmmMp.h"

///
/// SMM MP Protocol instance
///
EFI_MM_MP_PROTOCOL  mSmmMp = {
  EFI_MM_MP_PROTOCOL_REVISION,
  0,
  SmmMpGetNumberOfProcessors,
  SmmMpDispatchProcedure,
  SmmMpBroadcastProcedure,
  SmmMpSetStartupProcedure,
  SmmMpCheckForProcedure,
  SmmMpWaitForProcedure
};

/**
  Service to retrieves the number of logical processor in the platform.

  @param[in]  This                The EFI_MM_MP_PROTOCOL instance.
  @param[out] NumberOfProcessors  Pointer to the total number of logical processors in the system,
                                  including the BSP and all APs.

  @retval EFI_SUCCESS             The number of processors was retrieved successfully
  @retval EFI_INVALID_PARAMETER   NumberOfProcessors is NULL
**/
EFI_STATUS
EFIAPI
SmmMpGetNumberOfProcessors (
  IN CONST EFI_MM_MP_PROTOCOL  *This,
  OUT      UINTN               *NumberOfProcessors
  )
{
  if (NumberOfProcessors == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *NumberOfProcessors = gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;

  return EFI_SUCCESS;
}

/**
  This service allows the caller to invoke a procedure one of the application processors (AP). This
  function uses an optional token parameter to support blocking and non-blocking modes. If the token
  is passed into the call, the function will operate in a non-blocking fashion and the caller can
  check for completion with CheckOnProcedure or WaitForProcedure.

  @param[in]     This                   The EFI_MM_MP_PROTOCOL instance.
  @param[in]     Procedure              A pointer to the procedure to be run on the designated target
                                        AP of the system. Type EFI_AP_PROCEDURE2 is defined below in
                                        related definitions.
  @param[in]     CpuNumber              The zero-based index of the processor number of the target
                                        AP, on which the code stream is supposed to run. If the number
                                        points to the calling processor then it will not run the
                                        supplied code.
  @param[in]     TimeoutInMicroseconds  Indicates the time limit in microseconds for this AP to
                                        finish execution of Procedure, either for blocking or
                                        non-blocking mode. Zero means infinity. If the timeout
                                        expires before this AP returns from Procedure, then Procedure
                                        on the AP is terminated. If the timeout expires in blocking
                                        mode, the call returns EFI_TIMEOUT. If the timeout expires
                                        in non-blocking mode, the timeout determined can be through
                                        CheckOnProcedure or WaitForProcedure.
                                        Note that timeout support is optional. Whether an
                                        implementation supports this feature, can be determined via
                                        the Attributes data member.
  @param[in,out] ProcedureArguments     Allows the caller to pass a list of parameters to the code
                                        that is run by the AP. It is an optional common mailbox
                                        between APs and the caller to share information.
  @param[in,out] Token                  This is parameter is broken into two components:
                                        1.Token->Completion is an optional parameter that allows the
                                        caller to execute the procedure in a blocking or non-blocking
                                        fashion. If it is NULL the call is blocking, and the call will
                                        not return until the AP has completed the procedure. If the
                                        token is not NULL, the call will return immediately. The caller
                                        can check whether the procedure has completed with
                                        CheckOnProcedure or WaitForProcedure.
                                        2.Token->Status The implementation updates the address pointed
                                        at by this variable with the status code returned by Procedure
                                        when it completes execution on the target AP, or with EFI_TIMEOUT
                                        if the Procedure fails to complete within the optional timeout.
                                        The implementation will update this variable with EFI_NOT_READY
                                        prior to starting Procedure on the target AP
  @param[in,out] CPUStatus              This optional pointer may be used to get the status code returned
                                        by Procedure when it completes execution on the target AP, or with
                                        EFI_TIMEOUT if the Procedure fails to complete within the optional
                                        timeout. The implementation will update this variable with
                                        EFI_NOT_READY prior to starting Procedure on the target AP.

  @retval EFI_SUCCESS                   In the blocking case, this indicates that Procedure has completed
                                        execution on the target AP.
                                        In the non-blocking case this indicates that the procedure has
                                        been successfully scheduled for execution on the target AP.
  @retval EFI_INVALID_PARAMETER         The input arguments are out of range. Either the target AP is the
                                        caller of the function, or the Procedure or Token is NULL
  @retval EFI_NOT_READY                 If the target AP is busy executing another procedure
  @retval EFI_ALREADY_STARTED           Token is already in use for another procedure
  @retval EFI_TIMEOUT                   In blocking mode, the timeout expired before the specified AP
                                        has finished
  @retval EFI_OUT_OF_RESOURCES          Could not allocate a required resource.

**/
EFI_STATUS
EFIAPI
SmmMpDispatchProcedure (
  IN CONST EFI_MM_MP_PROTOCOL  *This,
  IN       EFI_AP_PROCEDURE2   Procedure,
  IN       UINTN               CpuNumber,
  IN       UINTN               TimeoutInMicroseconds,
  IN OUT   VOID                *ProcedureArguments OPTIONAL,
  IN OUT   MM_COMPLETION       *Token,
  IN OUT   EFI_STATUS          *CPUStatus
  )
{
  return InternalSmmStartupThisAp (
           Procedure,
           CpuNumber,
           ProcedureArguments,
           Token,
           TimeoutInMicroseconds,
           CPUStatus
           );
}

/**
  This service allows the caller to invoke a procedure on all running application processors (AP)
  except the caller. This function uses an optional token parameter to support blocking and
  nonblocking modes. If the token is passed into the call, the function will operate in a non-blocking
  fashion and the caller can check for completion with CheckOnProcedure or WaitForProcedure.

  It is not necessary for the implementation to run the procedure on every processor on the platform.
  Processors that are powered down in such a way that they cannot respond to interrupts, may be
  excluded from the broadcast.


  @param[in]     This                   The EFI_MM_MP_PROTOCOL instance.
  @param[in]     Procedure              A pointer to the code stream to be run on the APs that have
                                        entered MM. Type EFI_AP_PROCEDURE is defined below in related
                                        definitions.
  @param[in]     TimeoutInMicroseconds  Indicates the time limit in microseconds for the APs to finish
                                        execution of Procedure, either for blocking or non-blocking mode.
                                        Zero means infinity. If the timeout expires before all APs return
                                        from Procedure, then Procedure on the failed APs is terminated. If
                                        the timeout expires in blocking mode, the call returns EFI_TIMEOUT.
                                        If the timeout expires in non-blocking mode, the timeout determined
                                        can be through CheckOnProcedure or WaitForProcedure.
                                        Note that timeout support is optional. Whether an implementation
                                        supports this feature can be determined via the Attributes data
                                        member.
  @param[in,out] ProcedureArguments     Allows the caller to pass a list of parameters to the code
                                        that is run by the AP. It is an optional common mailbox
                                        between APs and the caller to share information.
  @param[in,out] Token                  This is parameter is broken into two components:
                                        1.Token->Completion is an optional parameter that allows the
                                        caller to execute the procedure in a blocking or non-blocking
                                        fashion. If it is NULL the call is blocking, and the call will
                                        not return until the AP has completed the procedure. If the
                                        token is not NULL, the call will return immediately. The caller
                                        can check whether the procedure has completed with
                                        CheckOnProcedure or WaitForProcedure.
                                        2.Token->Status The implementation updates the address pointed
                                        at by this variable with the status code returned by Procedure
                                        when it completes execution on the target AP, or with EFI_TIMEOUT
                                        if the Procedure fails to complete within the optional timeout.
                                        The implementation will update this variable with EFI_NOT_READY
                                        prior to starting Procedure on the target AP
  @param[in,out] CPUStatus              This optional pointer may be used to get the individual status
                                        returned by every AP that participated in the broadcast. This
                                        parameter if used provides the base address of an array to hold
                                        the EFI_STATUS value of each AP in the system. The size of the
                                        array can be ascertained by the GetNumberOfProcessors function.
                                        As mentioned above, the broadcast may not include every processor
                                        in the system. Some implementations may exclude processors that
                                        have been powered down in such a way that they are not responsive
                                        to interrupts. Additionally the broadcast excludes the processor
                                        which is making the BroadcastProcedure call. For every excluded
                                        processor, the array entry must contain a value of EFI_NOT_STARTED

  @retval EFI_SUCCESS                   In the blocking case, this indicates that Procedure has completed
                                        execution on the APs.
                                        In the non-blocking case this indicates that the procedure has
                                        been successfully scheduled for execution on the APs.
  @retval EFI_INVALID_PARAMETER         The Procedure or Token is NULL
  @retval EFI_NOT_READY                 If the target AP is busy executing another procedure
  @retval EFI_ALREADY_STARTED           Token is already in use for another procedure
  @retval EFI_TIMEOUT                   In blocking mode, the timeout expired before the specified AP
                                        has finished.
  @retval EFI_OUT_OF_RESOURCES          Could not allocate a required resource.

**/
EFI_STATUS
EFIAPI
SmmMpBroadcastProcedure (
  IN CONST EFI_MM_MP_PROTOCOL  *This,
  IN       EFI_AP_PROCEDURE2   Procedure,
  IN       UINTN               TimeoutInMicroseconds,
  IN OUT   VOID                *ProcedureArguments OPTIONAL,
  IN OUT   MM_COMPLETION       *Token,
  IN OUT   EFI_STATUS          *CPUStatus
  )
{
  return InternalSmmStartupAllAPs (
           Procedure,
           TimeoutInMicroseconds,
           ProcedureArguments,
           Token,
           CPUStatus
           );
}

/**
  This service allows the caller to set a startup procedure that will be executed when an AP powers
  up from a state where core configuration and context is lost. The procedure is execution has the
  following properties:
  1. The procedure executes before the processor is handed over to the operating system.
  2. All processors execute the same startup procedure.
  3. The procedure may run in parallel with other procedures invoked through the functions in this
  protocol, or with processors that are executing an MM handler or running in the operating system.


  @param[in]      This                 The EFI_MM_MP_PROTOCOL instance.
  @param[in]      Procedure            A pointer to the code stream to be run on the designated target AP
                                       of the system. Type EFI_AP_PROCEDURE is defined below in Volume 2
                                       with the related definitions of
                                       EFI_MP_SERVICES_PROTOCOL.StartupAllAPs.
                                       If caller may pass a value of NULL to deregister any existing
                                       startup procedure.
  @param[in,out]  ProcedureArguments   Allows the caller to pass a list of parameters to the code that is
                                       run by the AP. It is an optional common mailbox between APs and
                                       the caller to share information

  @retval EFI_SUCCESS                  The Procedure has been set successfully.
  @retval EFI_INVALID_PARAMETER        The Procedure is NULL but ProcedureArguments not NULL.

**/
EFI_STATUS
EFIAPI
SmmMpSetStartupProcedure (
  IN CONST EFI_MM_MP_PROTOCOL  *This,
  IN       EFI_AP_PROCEDURE    Procedure,
  IN OUT   VOID                *ProcedureArguments OPTIONAL
  )
{
  return RegisterStartupProcedure (Procedure, ProcedureArguments);
}

/**
  When non-blocking execution of a procedure on an AP is invoked with DispatchProcedure,
  via the use of a token, this function can be used to check for completion of the procedure on the AP.
  The function takes the token that was passed into the DispatchProcedure call. If the procedure
  is complete, and therefore it is now possible to run another procedure on the same AP, this function
  returns EFI_SUCESS. In this case the status returned by the procedure that executed on the AP is
  returned in the token's Status field. If the procedure has not yet completed, then this function
  returns EFI_NOT_READY.

  When a non-blocking execution of a procedure is invoked with BroadcastProcedure, via the
  use of a token, this function can be used to check for completion of the procedure on all the
  broadcast APs. The function takes the token that was passed into the BroadcastProcedure
  call. If the procedure is complete on all broadcast APs this function returns EFI_SUCESS. In this
  case the Status field in the token passed into the function reflects the overall result of the
  invocation, which may be EFI_SUCCESS, if all executions succeeded, or the first observed failure.
  If the procedure has not yet completed on the broadcast APs, the function returns
  EFI_NOT_READY.

  @param[in]      This                 The EFI_MM_MP_PROTOCOL instance.
  @param[in]      Token                This parameter describes the token that was passed into
                                       DispatchProcedure or BroadcastProcedure.

  @retval EFI_SUCCESS                  Procedure has completed.
  @retval EFI_NOT_READY                The Procedure has not completed.
  @retval EFI_INVALID_PARAMETER        Token or Token->Completion is NULL
  @retval EFI_NOT_FOUND                Token is not currently in use for a non-blocking call

**/
EFI_STATUS
EFIAPI
SmmMpCheckForProcedure (
  IN CONST EFI_MM_MP_PROTOCOL  *This,
  IN       MM_COMPLETION       Token
  )
{
  if (Token == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsTokenInUse ((SPIN_LOCK *)Token)) {
    return EFI_NOT_FOUND;
  }

  return IsApReady ((SPIN_LOCK *)Token);
}

/**
  When a non-blocking execution of a procedure on an AP is invoked via DispatchProcedure,
  this function will block the caller until the remote procedure has completed on the designated AP.
  The non-blocking procedure invocation is identified by the Token parameter, which must match the
  token that used when DispatchProcedure was called. Upon completion the status returned by
  the procedure that executed on the AP is used to update the token's Status field.

  When a non-blocking execution of a procedure on an AP is invoked via BroadcastProcedure
  this function will block the caller until the remote procedure has completed on all of the APs that
  entered MM. The non-blocking procedure invocation is identified by the Token parameter, which
  must match the token that used when BroadcastProcedure was called. Upon completion the
  overall status returned by the procedures that executed on the broadcast AP is used to update the
  token's Status field. The overall status may be EFI_SUCCESS, if all executions succeeded, or the
  first observed failure.


  @param[in]      This                 The EFI_MM_MP_PROTOCOL instance.
  @param[in]      Token                This parameter describes the token that was passed into
                                       DispatchProcedure or BroadcastProcedure.

  @retval EFI_SUCCESS                  Procedure has completed.
  @retval EFI_INVALID_PARAMETER        Token or Token->Completion is NULL
  @retval EFI_NOT_FOUND                Token is not currently in use for a non-blocking call

**/
EFI_STATUS
EFIAPI
SmmMpWaitForProcedure (
  IN CONST EFI_MM_MP_PROTOCOL  *This,
  IN       MM_COMPLETION       Token
  )
{
  EFI_STATUS  Status;

  do {
    Status = SmmMpCheckForProcedure (This, Token);
  } while (Status == EFI_NOT_READY);

  return Status;
}
