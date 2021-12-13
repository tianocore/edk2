/** @file
  Instance of the SBI ecall library.

  It allows calling an SBI function via an ecall from S-Mode.

  The legacy extensions are not included because they are not necessary.
  They would be:
  - SbiLegacySetTimer            -> Use SbiSetTimer
  - SbiLegacyConsolePutChar      -> No replacement - Use regular UEFI functions
  - SbiLegacyConsoleGetChar      -> No replacement - Use regular UEFI functions
  - SbiLegacyClearIpi            -> Write 0 to SSIP
  - SbiLegacySendIpi             -> Use SbiSendIpi
  - SbiLegacyRemoteFenceI        -> Use SbiRemoteFenceI
  - SbiLegacyRemoteSfenceVma     -> Use SbiRemoteSfenceVma
  - SbiLegacyRemoteSfenceVmaAsid -> Use SbiRemoteSfenceVmaAsid
  - SbiLegacyShutdown            -> Wait for new System Reset extension

  Copyright (c) 2020, Hewlett Packard Development LP. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - OpenSBI Version 0.6
**/

#include <IndustryStandard/RiscVOpensbi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/RiscVEdk2SbiLib.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_init.h>


//
// Maximum arguments for SBI ecall
// It's possible to pass more but no SBI call uses more as of SBI 0.2.
// The additional arguments would have to be passed on the stack instead of as
// registers, like it's done now.
//
#define SBI_CALL_MAX_ARGS 6

/**
  Call SBI call using ecall instruction.

  Asserts when NumArgs exceeds SBI_CALL_MAX_ARGS.

  @param[in] ExtId    SBI extension ID.
  @param[in] FuncId   SBI function ID.
  @param[in] NumArgs  Number of arguments to pass to the ecall.
  @param[in] ...      Argument list for the ecall.

  @retval  Returns SbiRet structure with value and error code.

**/
STATIC
SbiRet
EFIAPI
SbiCall(
  IN  UINTN ExtId,
  IN  UINTN FuncId,
  IN  UINTN NumArgs,
  ...
  )
{
    UINTN I;
    SbiRet Ret;
    UINTN Args[SBI_CALL_MAX_ARGS];
    VA_LIST ArgList;
    VA_START (ArgList, NumArgs);

    ASSERT (NumArgs <= SBI_CALL_MAX_ARGS);

    for (I = 0; I < SBI_CALL_MAX_ARGS; I++) {
      if (I < NumArgs) {
        Args[I] = VA_ARG (ArgList, UINTN);
      } else {
        // Default to 0 for all arguments that are not given
        Args[I] = 0;
      }
    }

    VA_END(ArgList);

    register UINTN a0 asm ("a0") = Args[0];
    register UINTN a1 asm ("a1") = Args[1];
    register UINTN a2 asm ("a2") = Args[2];
    register UINTN a3 asm ("a3") = Args[3];
    register UINTN a4 asm ("a4") = Args[4];
    register UINTN a5 asm ("a5") = Args[5];
    register UINTN a6 asm ("a6") = (UINTN)(FuncId);
    register UINTN a7 asm ("a7") = (UINTN)(ExtId);
    asm volatile ("ecall" \
         : "+r" (a0), "+r" (a1) \
         : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7) \
         : "memory"); \
    Ret.Error = a0;
    Ret.Value = a1;
    return Ret;
}

/**
  Translate SBI error code to EFI status.

  @param[in] SbiError   SBI error code
  @retval EFI_STATUS
**/

STATIC
EFI_STATUS
EFIAPI
TranslateError(
  IN  UINTN SbiError
  )
{
  switch (SbiError) {
    case SBI_SUCCESS:
      return EFI_SUCCESS;
    case SBI_ERR_FAILED:
      return EFI_DEVICE_ERROR;
      break;
    case SBI_ERR_NOT_SUPPORTED:
      return EFI_UNSUPPORTED;
      break;
    case SBI_ERR_INVALID_PARAM:
      return EFI_INVALID_PARAMETER;
      break;
    case SBI_ERR_DENIED:
      return EFI_ACCESS_DENIED;
      break;
    case SBI_ERR_INVALID_ADDRESS:
      return EFI_LOAD_ERROR;
      break;
    case SBI_ERR_ALREADY_AVAILABLE:
      return EFI_ALREADY_STARTED;
      break;
    default:
      //
      // Reaches here only if SBI has defined a new error type
      //
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
      break;
  }
}

//
// OpenSBI library interface function for the base extension
//

/**
  Get the implemented SBI specification version

  The minor number of the SBI specification is encoded in the low 24 bits,
  with the major number encoded in the next 7 bits.  Bit 32 must be 0 and is
  reserved for future expansion.

  @param[out] SpecVersion          The Version of the SBI specification.
**/
VOID
EFIAPI
SbiGetSpecVersion (
  OUT UINTN                       *SpecVersion
  )
{
  SbiRet Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_SPEC_VERSION, 0);

  if (!Ret.Error) {
    *SpecVersion = (UINTN)Ret.Value;
  }
}

/**
  Get the SBI implementation ID

  This ID is used to idenetify a specific SBI implementation in order to work
  around any quirks it might have.

  @param[out] ImplId               The ID of the SBI implementation.
**/
VOID
EFIAPI
SbiGetImplId (
  OUT UINTN                       *ImplId
  )
{
  SbiRet Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_IMP_ID, 0);
  *ImplId = (UINTN)Ret.Value;
}

/**
  Get the SBI implementation version

  The version of this SBI implementation.
  The encoding of this number is determined by the specific SBI implementation.

  @param[out] ImplVersion          The version of the SBI implementation.
**/
VOID
EFIAPI
SbiGetImplVersion (
  OUT UINTN                       *ImplVersion
  )
{
  SbiRet Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_IMP_VERSION, 0);
  *ImplVersion = (UINTN)Ret.Value;
}

/**
  Probe whether an SBI extension is available

  ProbeResult is set to 0 if the extension is not available or to an extension
  specified value if it is available.

  @param[in]  ExtensionId          The extension ID.
  @param[out] ProbeResult          The return value of the probe.
**/
VOID
EFIAPI
SbiProbeExtension (
  IN  INTN                         ExtensionId,
  OUT INTN                        *ProbeResult
  )
{
  SbiRet Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_PROBE_EXT, 0);
  *ProbeResult = (UINTN)Ret.Value;
}

/**
  Get the CPU's vendor ID

  Reads the mvendorid CSR.

  @param[out] MachineVendorId      The CPU's vendor ID.
**/
VOID
EFIAPI
SbiGetMachineVendorId (
  OUT UINTN                       *MachineVendorId
  )
{
  SbiRet Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_MVENDORID, 0);
  *MachineVendorId = (UINTN)Ret.Value;
}

/**
  Get the CPU's architecture ID

  Reads the marchid CSR.

  @param[out] MachineArchId        The CPU's architecture ID.
**/
VOID
EFIAPI
SbiGetMachineArchId (
  OUT UINTN                       *MachineArchId
  )
{
  SbiRet Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_MARCHID, 0);
  *MachineArchId = (UINTN)Ret.Value;
}

/**
  Get the CPU's architecture ID

  Reads the marchid CSR.

  @param[out] MachineImplId        The CPU's implementation ID.
**/
VOID
EFIAPI
SbiGetMachineImplId (
  OUT UINTN                       *MachineImplId
  )
{
  SbiRet Ret = SbiCall (SBI_EXT_BASE, SBI_EXT_BASE_GET_MIMPID, 0);
  *MachineImplId = (UINTN)Ret.Value;
}

//
// SBI interface function for the hart state management extension
//

/**
  Politely ask the SBI to start a given hart.

  This call may return before the hart has actually started executing, if the
  SBI implementation can guarantee that the hart is actually going to start.

  Before the hart jumps to StartAddr, the hart MUST configure PMP if present
  and switch to S-mode.

  @param[in]  HartId               The id of the hart to start.
  @param[in]  StartAddr            The physical address, where the hart starts
                                   executing from.
  @param[in]  Priv                 An XLEN-bit value, which will be in register
                                   a1 when the hart starts.
  @retval EFI_SUCCESS              Hart was stopped and will start executing from StartAddr.
  @retval EFI_LOAD_ERROR           StartAddr is not valid, possibly due to following reasons:
                                     - It is not a valid physical address.
                                     - The address is prohibited by PMP to run in
                                       supervisor mode.
  @retval EFI_INVALID_PARAMETER    HartId is not a valid hart id
  @retval EFI_ALREADY_STARTED      The hart is already running.
  @retval other                    The start request failed for unknown reasons.
**/
EFI_STATUS
EFIAPI
SbiHartStart (
  IN  UINTN                          HartId,
  IN  UINTN                          StartAddr,
  IN  UINTN                          Priv
  )
{
  SbiRet Ret = SbiCall (
                 SBI_EXT_HSM,
                 SBI_EXT_HSM_HART_START,
                 3,
                 HartId,
                 StartAddr,
                 Priv
                 );
  return TranslateError (Ret.Error);
}

/**
  Return execution of the calling hart to SBI.

  MUST be called in S-Mode with user interrupts disabled.
  This call is not expected to return, unless a failure occurs.

  @retval     EFI_SUCCESS          Never occurs. When successful, the call does not return.
  @retval     other                Failed to stop hard for an unknown reason.
**/
EFI_STATUS
EFIAPI
SbiHartStop (
  )
{
  SbiRet Ret = SbiCall (SBI_EXT_HSM, SBI_EXT_HSM_HART_STOP, 0);
  return TranslateError (Ret.Error);
}

/**
  Get the current status of a hart.

  Since harts can transition between states at any time, the status retrieved
  by this function may already be out of date, once it returns.

  Possible values for HartStatus are:
  0: STARTED
  1: STOPPED
  2: START_REQUEST_PENDING
  3: STOP_REQUEST_PENDING

  @param[out] HartStatus           The pointer in which the hart's status is
                                   stored.
  @retval EFI_SUCCESS              The operation succeeds.
  @retval EFI_INVALID_PARAMETER    A parameter is invalid.
**/
EFI_STATUS
EFIAPI
SbiHartGetStatus (
  IN  UINTN                          HartId,
  OUT UINTN                         *HartStatus
  )
{
  SbiRet Ret = SbiCall (SBI_EXT_HSM, SBI_EXT_HSM_HART_GET_STATUS, 1, HartId);

  if (!Ret.Error) {
    *HartStatus = (UINTN)Ret.Value;
  }

  return TranslateError (Ret.Error);
}

/**
  Clear pending timer interrupt bit and set timer for next event after Time.

  To clear the timer without scheduling a timer event, set Time to a
  practically infinite value or mask the timer interrupt by clearing sie.STIE.

  @param[in]  Time                 The time offset to the next scheduled timer interrupt.
**/
VOID
EFIAPI
SbiSetTimer (
  IN  UINT64                         Time
  )
{
  SbiCall (SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, 1, Time);
}

EFI_STATUS
EFIAPI
SbiSendIpi (
  IN  UINTN                         *HartMask,
  IN  UINTN                          HartMaskBase
  )
{
  SbiRet Ret = SbiCall (
                 SBI_EXT_IPI,
                 SBI_EXT_IPI_SEND_IPI,
                 2,
                 (UINTN)HartMask,
                 HartMaskBase
                 );
  return TranslateError (Ret.Error);
}

/**
  Instructs remote harts to execute a FENCE.I instruction.

  @param[in]  HartMask             Scalar bit-vector containing hart ids
  @param[in]  HartMaskBase         The starting hartid from which the bit-vector
                                   must be computed. If set to -1, HartMask is
                                   ignored and all harts are considered.
  @retval EFI_SUCCESS              IPI was sent to all the targeted harts.
  @retval EFI_INVALID_PARAMETER    Either hart_mask_base or any of the hartid
                                   from hart_mask is not valid i.e. either the
                                   hartid is not enabled by the platform or is
                                   not available to the supervisor.
**/
EFI_STATUS
EFIAPI
SbiRemoteFenceI (
  IN  UINTN                         *HartMask,
  IN  UINTN                          HartMaskBase
  )
{
  SbiRet Ret = SbiCall (
                 SBI_EXT_RFENCE,
                 SBI_EXT_RFENCE_REMOTE_FENCE_I,
                 2,
                 (UINTN)HartMask,
                 HartMaskBase
                 );
  return TranslateError (Ret.Error);
}

/**
  Instructs the remote harts to execute one or more SFENCE.VMA instructions.

  The SFENCE.VMA covers the range of virtual addresses between StartAaddr and Size.

  The remote fence function acts as a full tlb flush if * StartAddr and size
  are both 0 * size is equal to 2^XLEN-1

  @param[in]  HartMask             Scalar bit-vector containing hart ids
  @param[in]  HartMaskBase         The starting hartid from which the bit-vector
                                   must be computed. If set to -1, HartMask is
                                   ignored and all harts are considered.
  @param[in]  StartAddr            The first address of the affected range.
  @param[in]  Size                 How many addresses are affected.
  @retval EFI_SUCCESS              IPI was sent to all the targeted harts.
  @retval EFI_LOAD_ERROR           StartAddr or Size is not valid.
  @retval EFI_INVALID_PARAMETER    Either hart_mask_base or any of the hartid
                                   from hart_mask is not valid i.e. either the
                                   hartid is not enabled by the platform or is
                                   not available to the supervisor.
**/
EFI_STATUS
EFIAPI
SbiRemoteSfenceVma (
  IN  UINTN                         *HartMask,
  IN  UINTN                          HartMaskBase,
  IN  UINTN                          StartAddr,
  IN  UINTN                          Size
  )
{
  SbiRet Ret = SbiCall (
                 SBI_EXT_RFENCE,
                 SBI_EXT_RFENCE_REMOTE_SFENCE_VMA,
                 4,
                 (UINTN)HartMask,
                 HartMaskBase,
                 StartAddr,
                 Size
                 );
  return TranslateError (Ret.Error);
}

/**
  Instructs the remote harts to execute one or more SFENCE.VMA instructions.

  The SFENCE.VMA covers the range of virtual addresses between StartAaddr and Size.
  Covers only the given ASID.

  The remote fence function acts as a full tlb flush if * StartAddr and size
  are both 0 * size is equal to 2^XLEN-1

  @param[in]  HartMask             Scalar bit-vector containing hart ids
  @param[in]  HartMaskBase         The starting hartid from which the bit-vector
                                   must be computed. If set to -1, HartMask is
                                   ignored and all harts are considered.
  @param[in]  StartAddr            The first address of the affected range.
  @param[in]  Size                 How many addresses are affected.
  @retval EFI_SUCCESS              IPI was sent to all the targeted harts.
  @retval EFI_LOAD_ERROR           StartAddr or Size is not valid.
  @retval EFI_INVALID_PARAMETER    Either hart_mask_base or any of the hartid
                                   from hart_mask is not valid i.e. either the
                                   hartid is not enabled by the platform or is
                                   not available to the supervisor.
**/
EFI_STATUS
EFIAPI
SbiRemoteSfenceVmaAsid (
  IN  UINTN                         *HartMask,
  IN  UINTN                          HartMaskBase,
  IN  UINTN                          StartAddr,
  IN  UINTN                          Size,
  IN  UINTN                          Asid
  )
{
  SbiRet Ret = SbiCall (
                 SBI_EXT_RFENCE,
                 SBI_EXT_RFENCE_REMOTE_SFENCE_VMA_ASID,
                 5,
                 (UINTN)HartMask,
                 HartMaskBase,
                 StartAddr,
                 Size,
                 Asid
                 );
  return TranslateError (Ret.Error);
}

/**
  Instructs the remote harts to execute one or more SFENCE.GVMA instructions.

  The SFENCE.GVMA covers the range of virtual addresses between StartAaddr and Size.
  Covers only the given VMID.
  This function call is only valid for harts implementing the hypervisor extension.

  The remote fence function acts as a full tlb flush if * StartAddr and size
  are both 0 * size is equal to 2^XLEN-1

  @param[in]  HartMask             Scalar bit-vector containing hart ids
  @param[in]  HartMaskBase         The starting hartid from which the bit-vector
                                   must be computed. If set to -1, HartMask is
                                   ignored and all harts are considered.
  @param[in]  StartAddr            The first address of the affected range.
  @param[in]  Size                 How many addresses are affected.
  @retval EFI_SUCCESS              IPI was sent to all the targeted harts.
  @retval EFI_LOAD_ERROR           StartAddr or Size is not valid.
  @retval EFI_UNSUPPORTED          SBI does not implement this function or one
                                   of the target harts does not support the
                                   hypervisor extension.
  @retval EFI_INVALID_PARAMETER    Either hart_mask_base or any of the hartid
                                   from hart_mask is not valid i.e. either the
                                   hartid is not enabled by the platform or is
                                   not available to the supervisor.
**/
EFI_STATUS
EFIAPI
SbiRemoteHFenceGvmaVmid (
  IN  UINTN                         *HartMask,
  IN  UINTN                          HartMaskBase,
  IN  UINTN                          StartAddr,
  IN  UINTN                          Size,
  IN  UINTN                          Vmid
  )
{
  SbiRet Ret = SbiCall (
                 SBI_EXT_RFENCE,
                 SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA,
                 5,
                 (UINTN)HartMask,
                 HartMaskBase,
                 StartAddr,
                 Size,
                 Vmid
                 );
  return TranslateError (Ret.Error);
}

/**
  Instructs the remote harts to execute one or more SFENCE.GVMA instructions.

  The SFENCE.GVMA covers the range of virtual addresses between StartAaddr and Size.
  This function call is only valid for harts implementing the hypervisor extension.

  The remote fence function acts as a full tlb flush if * StartAddr and size
  are both 0 * size is equal to 2^XLEN-1

  @param[in]  HartMask             Scalar bit-vector containing hart ids
  @param[in]  HartMaskBase         The starting hartid from which the bit-vector
                                   must be computed. If set to -1, HartMask is
                                   ignored and all harts are considered.
  @param[in]  StartAddr            The first address of the affected range.
  @param[in]  Size                 How many addresses are affected.
  @retval EFI_SUCCESS              IPI was sent to all the targeted harts.
  @retval EFI_LOAD_ERROR           StartAddr or Size is not valid.
  @retval EFI_UNSUPPORTED          SBI does not implement this function or one
                                   of the target harts does not support the
                                   hypervisor extension.
  @retval EFI_INVALID_PARAMETER    Either hart_mask_base or any of the hartid
                                   from hart_mask is not valid i.e. either the
                                   hartid is not enabled by the platform or is
                                   not available to the supervisor.
**/
EFI_STATUS
EFIAPI
SbiRemoteHFenceGvma (
  IN  UINTN                         *HartMask,
  IN  UINTN                          HartMaskBase,
  IN  UINTN                          StartAddr,
  IN  UINTN                          Size
  )
{
  SbiRet Ret = SbiCall (
                 SBI_EXT_RFENCE,
                 SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA_VMID,
                 4,
                 (UINTN)HartMask,
                 HartMaskBase,
                 StartAddr,
                 Size
                 );
  return TranslateError (Ret.Error);
}

/**
  Instructs the remote harts to execute one or more SFENCE.VVMA instructions.

  The SFENCE.GVMA covers the range of virtual addresses between StartAaddr and Size.
  Covers only the given ASID.
  This function call is only valid for harts implementing the hypervisor extension.

  The remote fence function acts as a full tlb flush if * StartAddr and size
  are both 0 * size is equal to 2^XLEN-1

  @param[in]  HartMask             Scalar bit-vector containing hart ids
  @param[in]  HartMaskBase         The starting hartid from which the bit-vector
                                   must be computed. If set to -1, HartMask is
                                   ignored and all harts are considered.
  @param[in]  StartAddr            The first address of the affected range.
  @param[in]  Size                 How many addresses are affected.
  @retval EFI_SUCCESS              IPI was sent to all the targeted harts.
  @retval EFI_LOAD_ERROR           StartAddr or Size is not valid.
  @retval EFI_UNSUPPORTED          SBI does not implement this function or one
                                   of the target harts does not support the
                                   hypervisor extension.
  @retval EFI_INVALID_PARAMETER    Either hart_mask_base or any of the hartid
                                   from hart_mask is not valid i.e. either the
                                   hartid is not enabled by the platform or is
                                   not available to the supervisor.
**/
EFI_STATUS
EFIAPI
SbiRemoteHFenceVvmaAsid (
  IN  UINTN                         *HartMask,
  IN  UINTN                          HartMaskBase,
  IN  UINTN                          StartAddr,
  IN  UINTN                          Size,
  IN  UINTN                          Asid
  )
{
  SbiRet Ret = SbiCall (
                 SBI_EXT_RFENCE,
                 SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA,
                 5,
                 (UINTN)HartMask,
                 HartMaskBase,
                 StartAddr,
                 Size,
                 Asid
                 );
  return TranslateError (Ret.Error);
}

/**
  Instructs the remote harts to execute one or more SFENCE.VVMA instructions.

  The SFENCE.GVMA covers the range of virtual addresses between StartAaddr and Size.
  This function call is only valid for harts implementing the hypervisor extension.

  The remote fence function acts as a full tlb flush if * StartAddr and size
  are both 0 * size is equal to 2^XLEN-1

  @param[in]  HartMask             Scalar bit-vector containing hart ids
  @param[in]  HartMaskBase         The starting hartid from which the bit-vector
                                   must be computed. If set to -1, HartMask is
                                   ignored and all harts are considered.
  @param[in]  StartAddr            The first address of the affected range.
  @param[in]  Size                 How many addresses are affected.
  @retval EFI_SUCCESS              IPI was sent to all the targeted harts.
  @retval EFI_LOAD_ERROR           StartAddr or Size is not valid.
  @retval EFI_UNSUPPORTED          SBI does not implement this function or one
                                   of the target harts does not support the
                                   hypervisor extension.
  @retval EFI_INVALID_PARAMETER    Either hart_mask_base or any of the hartid
                                   from hart_mask is not valid i.e. either the
                                   hartid is not enabled by the platform or is
                                   not available to the supervisor.
**/
EFI_STATUS
EFIAPI
SbiRemoteHFenceVvma (
  IN  UINTN                         *HartMask,
  IN  UINTN                          HartMaskBase,
  IN  UINTN                          StartAddr,
  IN  UINTN                          Size
  )
{
  SbiRet Ret = SbiCall (
                 SBI_EXT_RFENCE,
                 SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA_ASID,
                 4,
                 (UINTN)HartMask,
                 HartMaskBase,
                 StartAddr,
                 Size
                 );
  return TranslateError (Ret.Error);
}

//
// SBI interface function for the vendor extension
//

/**
  Call a function in a vendor defined SBI extension

  ASSERT() if the ExtensionId is not in the designated SBI Vendor Extension
  Space or NumArgs exceeds SBI_CALL_MAX_ARGS.

  @param[in]  ExtensionId          The SBI vendor extension ID.
  @param[in]  FunctionId           The function ID to call in this extension.
  @param[in]  NumArgs              How many arguments are passed.
  @param[in]  ...                  Actual Arguments to the function.
  @retval EFI_SUCCESS if the SBI function was called and it was successful
  @retval others if the called SBI function returns an error
**/
EFI_STATUS
EFIAPI
SbiVendorCall (
  IN  UINTN                          ExtensionId,
  IN  UINTN                          FunctionId,
  IN  UINTN                          NumArgs,
  ...
  )
{
    SbiRet Ret;
    VA_LIST Args;
    VA_START (Args, NumArgs);

    ASSERT (ExtensionId >= SBI_EXT_VENDOR_START && ExtensionId <= SBI_EXT_VENDOR_END);
    ASSERT (NumArgs <= SBI_CALL_MAX_ARGS);

    switch (NumArgs) {
      case 0:
        Ret = SbiCall (ExtensionId, FunctionId, NumArgs);
        break;
      case 1:
        Ret = SbiCall (ExtensionId, FunctionId, NumArgs, VA_ARG (Args, UINTN));
        break;
      case 2:
        Ret = SbiCall (ExtensionId, FunctionId, NumArgs, VA_ARG (Args, UINTN),
                       VA_ARG (Args, UINTN));
        break;
      case 3:
        Ret = SbiCall (ExtensionId, FunctionId, NumArgs, VA_ARG (Args, UINTN),
                       VA_ARG (Args, UINTN), VA_ARG (Args, UINTN));
        break;
      case 4:
        Ret = SbiCall (ExtensionId, FunctionId, NumArgs, VA_ARG (Args, UINTN),
                       VA_ARG (Args, UINTN), VA_ARG (Args, UINTN),
                       VA_ARG (Args, UINTN));
        break;
      case 5:
        Ret = SbiCall (ExtensionId, FunctionId, NumArgs, VA_ARG (Args, UINTN),
                       VA_ARG (Args, UINTN), VA_ARG (Args, UINTN),
                       VA_ARG (Args, UINTN), VA_ARG (Args, UINTN));
        break;
      case 6:
        Ret = SbiCall (ExtensionId, FunctionId, NumArgs, VA_ARG (Args, UINTN),
                       VA_ARG (Args, UINTN), VA_ARG (Args, UINTN),
                       VA_ARG (Args, UINTN), VA_ARG (Args, UINTN),
                       VA_ARG (Args, UINTN));
        break;
      default:
        // Too many args. In theory SBI can handle more arguments when they are
        // passed on the stack but no SBI extension uses this, therefore it's
        // not yet implemented here.
        return EFI_INVALID_PARAMETER;
     }

    VA_END(Args);
    return TranslateError (Ret.Error);
}

//
// SBI Firmware extension
//

/**
  Get scratch space of the current hart.

  Please consider using the wrapper SbiGetFirmwareContext if you only need to
  access the firmware context.

  @param[out] ScratchSpace         The scratch space pointer.
**/
VOID
EFIAPI
SbiGetMscratch (
  OUT SBI_SCRATCH                    **ScratchSpace
  )
{
  SbiRet Ret = SbiCall (SBI_EDK2_FW_EXT, SBI_EXT_FW_MSCRATCH_FUNC, 0);

  // Our ecall handler never returns an error, only when the func id is invalid
  ASSERT (Ret.Error == SBI_OK);

  *ScratchSpace = (SBI_SCRATCH *)Ret.Value;
}

/**
  Get scratch space of the given hart id.

  @param[in]  HartId               The hart id.
  @param[out] ScratchSpace         The scratch space pointer.
**/
VOID
EFIAPI
SbiGetMscratchHartid (
  IN  UINTN                            HartId,
  OUT SBI_SCRATCH                    **ScratchSpace
  )
{
  SbiRet Ret = SbiCall (
                 SBI_EDK2_FW_EXT,
                 SBI_EXT_FW_MSCRATCH_HARTID_FUNC,
                 1,
                 HartId
                 );

  // Our ecall handler never returns an error, only when the func id is invalid
  ASSERT (Ret.Error == SBI_OK);

  *ScratchSpace = (SBI_SCRATCH *)Ret.Value;
}

/**
  Get firmware context of the calling hart.

  @param[out] FirmwareContext      The firmware context pointer.
  @retval EFI_SUCCESS              The operation succeeds.
**/
VOID
EFIAPI
SbiGetFirmwareContext (
  OUT EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT **FirmwareContext
  )
{
  SBI_SCRATCH  *ScratchSpace;
  SBI_PLATFORM *SbiPlatform;

  SbiGetMscratch(&ScratchSpace);
  SbiPlatform = (SBI_PLATFORM *)sbi_platform_ptr(ScratchSpace);
  *FirmwareContext = (EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *)SbiPlatform->firmware_context;
}

/**
  Set firmware context of the calling hart.

  @param[in] FirmwareContext       The firmware context pointer.
**/
VOID
EFIAPI
SbiSetFirmwareContext (
  IN EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *FirmwareContext
  )
{
  SBI_SCRATCH  *ScratchSpace;
  SBI_PLATFORM *SbiPlatform;

  SbiGetMscratch(&ScratchSpace);

  SbiPlatform = (SBI_PLATFORM *)sbi_platform_ptr (ScratchSpace);
  SbiPlatform->firmware_context = (UINTN)FirmwareContext;
}
