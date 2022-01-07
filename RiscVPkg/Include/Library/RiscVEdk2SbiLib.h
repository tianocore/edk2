/** @file
  Library to call the RISC-V SBI ecalls

  Copyright (c) 2021-2022, Hewlett Packard Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Hart - Hardware Thread, similar to a CPU core
**/

#ifndef RISCV_SBI_LIB_H_
#define RISCV_SBI_LIB_H_

#include <Uefi.h>
#include <IndustryStandard/RiscVOpensbi.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_ecall.h>

//
// EDK2 OpenSBI Firmware extension.
//
#define SBI_EDK2_FW_EXT  (SBI_EXT_FIRMWARE_START | SBI_OPENSBI_IMPID)
//
// EDK2 OpenSBI Firmware extension functions.
//
#define SBI_EXT_FW_MSCRATCH_FUNC         0
#define SBI_EXT_FW_MSCRATCH_HARTID_FUNC  1

//
// EDK2 OpenSBI firmware extension return status.
//
typedef struct {
  UINTN    Error; ///< SBI status code
  UINTN    Value; ///< Value returned
} SBI_RET;

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
  OUT UINTN  *SpecVersion
  );

/**
  Get the SBI implementation ID

  This ID is used to identify a specific SBI implementation in order to work
  around any quirks it might have.

  @param[out] ImplId               The ID of the SBI implementation.
**/
VOID
EFIAPI
SbiGetImplId (
  OUT UINTN  *ImplId
  );

/**
  Get the SBI implementation version

  The version of this SBI implementation.
  The encoding of this number is determined by the specific SBI implementation.

  @param[out] ImplVersion          The version of the SBI implementation.
**/
VOID
EFIAPI
SbiGetImplVersion (
  OUT UINTN  *ImplVersion
  );

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
  IN  INTN  ExtensionId,
  OUT INTN  *ProbeResult
  );

/**
  Get the CPU's vendor ID

  Reads the mvendorid CSR.

  @param[out] MachineVendorId      The CPU's vendor ID.
**/
VOID
EFIAPI
SbiGetMachineVendorId (
  OUT UINTN  *MachineVendorId
  );

/**
  Get the CPU's architecture ID

  Reads the marchid CSR.

  @param[out] MachineArchId        The CPU's architecture ID.
**/
VOID
EFIAPI
SbiGetMachineArchId (
  OUT UINTN  *MachineArchId
  );

/**
  Get the CPU's implementation ID

  Reads the mimpid CSR.

  @param[out] MachineImplId        The CPU's implementation ID.
**/
VOID
EFIAPI
SbiGetMachineImplId (
  OUT UINTN  *MachineImplId
  );

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
  IN  UINTN  HartId,
  IN  UINTN  StartAddr,
  IN  UINTN  Priv
  );

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
  );

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
  IN  UINTN  HartId,
  OUT UINTN  *HartStatus
  );

///
/// Timer extension
///

/**
  Clear pending timer interrupt bit and set timer for next event after Time.

  To clear the timer without scheduling a timer event, set Time to a
  practically infinite value or mask the timer interrupt by clearing sie.STIE.

  @param[in]  Time                 The time offset to the next scheduled timer interrupt.
**/
VOID
EFIAPI
SbiSetTimer (
  IN  UINT64  Time
  );

///
/// IPI extension
///

/**
  Send IPI to all harts specified in the mask.

  The interrupts are registered as supervisor software interrupts at the
  receiving hart.

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
SbiSendIpi (
  IN  UINTN  *HartMask,
  IN  UINTN  HartMaskBase
  );

///
/// Remote fence extension
///

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
  IN  UINTN  *HartMask,
  IN  UINTN  HartMaskBase
  );

/**
  Instructs the remote harts to execute one or more SFENCE.VMA instructions.

  The SFENCE.VMA covers the range of virtual addresses between StartAddr and Size.

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
  IN  UINTN  *HartMask,
  IN  UINTN  HartMaskBase,
  IN  UINTN  StartAddr,
  IN  UINTN  Size
  );

/**
  Instructs the remote harts to execute one or more SFENCE.VMA instructions.

  The SFENCE.VMA covers the range of virtual addresses between StartAddr and Size.
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
  IN  UINTN  *HartMask,
  IN  UINTN  HartMaskBase,
  IN  UINTN  StartAddr,
  IN  UINTN  Size,
  IN  UINTN  Asid
  );

/**
  Instructs the remote harts to execute one or more SFENCE.GVMA instructions.

  The SFENCE.GVMA covers the range of virtual addresses between StartAddr and Size.
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
SbiRemoteHfenceGvmaVmid (
  IN  UINTN  *HartMask,
  IN  UINTN  HartMaskBase,
  IN  UINTN  StartAddr,
  IN  UINTN  Size,
  IN  UINTN  Vmid
  );

/**
  Instructs the remote harts to execute one or more SFENCE.GVMA instructions.

  The SFENCE.GVMA covers the range of virtual addresses between StartAddr and Size.
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
SbiRemoteHfenceGvma (
  IN  UINTN  *HartMask,
  IN  UINTN  HartMaskBase,
  IN  UINTN  StartAddr,
  IN  UINTN  Size
  );

/**
  Instructs the remote harts to execute one or more SFENCE.VVMA instructions.

  The SFENCE.GVMA covers the range of virtual addresses between StartAddr and Size.
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
SbiRemoteHfenceVvmaAsid (
  IN  UINTN  *HartMask,
  IN  UINTN  HartMaskBase,
  IN  UINTN  StartAddr,
  IN  UINTN  Size,
  IN  UINTN  Asid
  );

/**
  Instructs the remote harts to execute one or more SFENCE.VVMA instructions.

  The SFENCE.GVMA covers the range of virtual addresses between StartAddr and Size.
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
SbiRemoteHfenceVvma (
  IN  UINTN  *HartMask,
  IN  UINTN  HartMaskBase,
  IN  UINTN  StartAddr,
  IN  UINTN  Size
  );

///
/// Firmware System Reset (SRST) Extension
///

/**
  Reset the system

  The System Reset Extension provides a function that allow the supervisor
  software to request system-level reboot or shutdown. The term "system" refers
  to the world-view of supervisor software and the underlying SBI
  implementation could be machine mode firmware or hypervisor.

  Valid parameters for ResetType and ResetReason are defined in sbi_ecall_interface.h

  #define SBI_SRST_RESET_TYPE_SHUTDOWN    0x0
  #define SBI_SRST_RESET_TYPE_COLD_REBOOT 0x1
  #define SBI_SRST_RESET_TYPE_WARM_REBOOT 0x2

  #define SBI_SRST_RESET_REASON_NONE      0x0
  #define SBI_SRST_RESET_REASON_SYSFAIL   0x1

  When the call is successful, it will not return.

  @param[in]  ResetType            Typ of reset: Shutdown, cold-, or warm-reset.
  @param[in]  ResetReason          Why the system resets. No reason or system failure.
  @retval EFI_INVALID_PARAMETER    Either ResetType or ResetReason is invalid.
  @retval EFI_UNSUPPORTED          ResetType is valid but not implemented on the platform.
  @retval EFI_DEVICE_ERROR         Unknown error.
**/
EFI_STATUS
EFIAPI
SbiSystemReset (
  IN  UINTN  ResetType,
  IN  UINTN  ResetReason
  );

///
/// Vendor Specific extension space: Extension Ids 0x09000000 through 0x09FFFFFF
///

/**
  Call a function in a vendor defined SBI extension

  ASSERT() if the ExtensionId is not in the designated SBI Vendor Extension
  Space.

  @param[in]  ExtensionId          The SBI vendor extension ID.
  @param[in]  FunctionId           The function ID to call in this extension.
  @param[in]  NumArgs              How many arguments are passed.
  @param[in]  ...                  Actual Arguments to the function.
  @retval EFI_SUCCESS if the SBI function was called and it was successful
  @retval EFI_INVALID_PARAMETER if NumArgs exceeds 6
  @retval others if the called SBI function returns an error
**/
EFI_STATUS
EFIAPI
SbiVendorCall (
  IN  UINTN  ExtensionId,
  IN  UINTN  FunctionId,
  IN  UINTN  NumArgs,
  ...
  );

///
/// Firmware SBI Extension
///
/// This SBI Extension is defined and used by EDK2 only in order to be able to
/// run PI and DXE phase in S-Mode.
///

/**
  Get scratch space of the current hart.

  Please consider using the wrapper SbiGetFirmwareContext if you only need to
  access the firmware context.

  @param[out] ScratchSpace         The scratch space pointer.
**/
VOID
EFIAPI
SbiGetMscratch (
  OUT SBI_SCRATCH  **ScratchSpace
  );

/**
  Get scratch space of the given hart id.

  @param[in]  HartId               The hart id.
  @param[out] ScratchSpace         The scratch space pointer.
**/
VOID
EFIAPI
SbiGetMscratchHartid (
  IN  UINTN        HartId,
  OUT SBI_SCRATCH  **ScratchSpace
  );

/**
  Get firmware context of the calling hart.

  @param[out] FirmwareContext      The firmware context pointer.
**/
VOID
EFIAPI
SbiGetFirmwareContext (
  OUT EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT  **FirmwareContext
  );

/**
  Set firmware context of the calling hart.

  @param[in] FirmwareContext       The firmware context pointer.
**/
VOID
EFIAPI
SbiSetFirmwareContext (
  IN EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT  *FirmwareContext
  );

#endif
