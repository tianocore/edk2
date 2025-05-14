/** @file
  SVSM Support Library.

  Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/AmdSvsmLib.h>
#include <Register/Amd/Msr.h>
#include <Register/Amd/Svsm.h>
#include <Register/Amd/SvsmMsr.h>

#define PAGES_PER_2MB_ENTRY  512

/**
  Issue a GHCB termination request for termination.

  Request termination using the GHCB MSR protocol.

**/
STATIC
VOID
SnpTerminate (
  VOID
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;

  //
  // Use the GHCB MSR Protocol to request termination by the hypervisor
  //
  Msr.Uint64                      = 0;
  Msr.GhcbTerminate.Function      = GHCB_INFO_TERMINATE_REQUEST;
  Msr.GhcbTerminate.ReasonCodeSet = GHCB_TERMINATE_GHCB;
  Msr.GhcbTerminate.ReasonCode    = GHCB_TERMINATE_GHCB_GENERAL;
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, Msr.Uint64);

  AsmVmgExit ();

  ASSERT (FALSE);
  CpuDeadLoop ();
}

/**
  Issue an SVSM request.

  Invokes the SVSM to process a request on behalf of the guest.

  @param[in,out]  SvsmCallData  Pointer to the SVSM call data

  @return                       Contents of RAX upon return from VMGEXIT
**/
STATIC
UINTN
SvsmMsrProtocol (
  IN OUT SVSM_CALL_DATA  *SvsmCallData
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  UINT64                    CurrentMsr;
  UINT8                     Pending;
  BOOLEAN                   InterruptState;
  UINTN                     Ret;

  do {
    //
    // Be sure that an interrupt can't cause a #VC while the GHCB MSR protocol
    // is being used (#VC handler will ASSERT if lower 12-bits are not zero).
    //
    InterruptState = GetInterruptState ();
    if (InterruptState) {
      DisableInterrupts ();
    }

    Pending                   = 0;
    SvsmCallData->CallPending = &Pending;

    CurrentMsr = AsmReadMsr64 (MSR_SEV_ES_GHCB);

    Msr.Uint64                  = 0;
    Msr.SnpVmplRequest.Function = GHCB_INFO_SNP_VMPL_REQUEST;
    Msr.SnpVmplRequest.Vmpl     = 0;
    AsmWriteMsr64 (MSR_SEV_ES_GHCB, Msr.Uint64);

    //
    // Guest memory is used for the guest-SVSM communication, so fence the
    // invocation of the VMGEXIT instruction to ensure VMSA accesses are
    // synchronized properly.
    //
    MemoryFence ();
    Ret = AsmVmgExitSvsm (SvsmCallData);
    MemoryFence ();

    Msr.Uint64 = AsmReadMsr64 (MSR_SEV_ES_GHCB);

    AsmWriteMsr64 (MSR_SEV_ES_GHCB, CurrentMsr);

    if (InterruptState) {
      EnableInterrupts ();
    }

    if (Pending != 0) {
      SnpTerminate ();
    }

    if ((Msr.SnpVmplResponse.Function != GHCB_INFO_SNP_VMPL_RESPONSE) ||
        (Msr.SnpVmplResponse.ErrorCode != 0))
    {
      SnpTerminate ();
    }
  } while (Ret == SVSM_ERR_INCOMPLETE || Ret == SVSM_ERR_BUSY);

  return Ret;
}

/**
  Report the presence of an Secure Virtual Services Module (SVSM).

  Determines the presence of an SVSM.

  @retval  TRUE                   An SVSM is present
  @retval  FALSE                  An SVSM is not present

**/
BOOLEAN
EFIAPI
AmdSvsmIsSvsmPresent (
  VOID
  )
{
  SVSM_INFORMATION  *SvsmInfo;

  SvsmInfo = (SVSM_INFORMATION *)(UINTN)PcdGet32 (PcdOvmfSnpSecretsBase);

  return (SvsmInfo != NULL && SvsmInfo->SvsmSize != 0);
}

/**
  Report the VMPL level at which the SEV-SNP guest is running.

  Determines the VMPL level at which the guest is running. If an SVSM is
  not present, then it must be VMPL0, otherwise return what is reported
  by the SVSM.

  @return                         The VMPL level

**/
UINT8
EFIAPI
AmdSvsmSnpGetVmpl (
  VOID
  )
{
  SVSM_INFORMATION  *SvsmInfo;

  SvsmInfo = (SVSM_INFORMATION *)(UINTN)PcdGet32 (PcdOvmfSnpSecretsBase);

  return AmdSvsmIsSvsmPresent () ? SvsmInfo->SvsmGuestVmpl : 0;
}

/**
  Report the Calling Area address (CAA) for the BSP of the SEV-SNP guest.

  If an SVSM is present, the CAA for the BSP is returned.

  @return                         The CAA

**/
UINT64
EFIAPI
AmdSvsmSnpGetCaa (
  VOID
  )
{
  SVSM_INFORMATION  *SvsmInfo;

  SvsmInfo = (SVSM_INFORMATION *)(UINTN)PcdGet32 (PcdOvmfSnpSecretsBase);

  return AmdSvsmIsSvsmPresent () ? SvsmInfo->SvsmCaa : 0;
}

/**
  Issue an SVSM request to perform the PVALIDATE instruction.

  Invokes the SVSM to process the PVALIDATE instruction on behalf of the
  guest to validate or invalidate the memory range specified.

  @param[in]       Info           Pointer to a page state change structure

**/
STATIC
VOID
SvsmPvalidate (
  IN SNP_PAGE_STATE_CHANGE_INFO  *Info
  )
{
  SVSM_CALL_DATA          SvsmCallData;
  SVSM_CAA                *Caa;
  SVSM_PVALIDATE_REQUEST  *Request;
  SVSM_FUNCTION           Function;
  BOOLEAN                 Validate;
  UINTN                   Entry;
  UINTN                   EntryLimit;
  UINTN                   Index;
  UINTN                   EndIndex;
  UINT64                  Gfn;
  UINT64                  GfnEnd;
  UINTN                   Ret;

  Caa = (SVSM_CAA *)AmdSvsmSnpGetCaa ();
  ZeroMem (Caa->SvsmBuffer, sizeof (Caa->SvsmBuffer));

  Function.Id.Protocol = SVSM_PROTOCOL_CORE;
  Function.Id.CallId   = SVSM_CORE_PVALIDATE;

  Request    = (SVSM_PVALIDATE_REQUEST *)Caa->SvsmBuffer;
  EntryLimit = ((sizeof (Caa->SvsmBuffer) - sizeof (*Request)) /
                sizeof (Request->Entry[0])) - 1;

  SvsmCallData.Caa   = Caa;
  SvsmCallData.RaxIn = Function.Uint64;
  SvsmCallData.RcxIn = (UINT64)(UINTN)Request;

  Entry    = 0;
  Index    = Info->Header.CurrentEntry;
  EndIndex = Info->Header.EndEntry;

  while (Index <= EndIndex) {
    Validate = Info->Entry[Index].Operation == SNP_PAGE_STATE_PRIVATE;

    Request->Header.Entries++;
    Request->Entry[Entry].Bits.PageSize = Info->Entry[Index].PageSize;
    Request->Entry[Entry].Bits.Action   = (Validate == TRUE) ? 1 : 0;
    Request->Entry[Entry].Bits.IgnoreCf = 0;
    Request->Entry[Entry].Bits.Address  = Info->Entry[Index].GuestFrameNumber;

    Entry++;
    if ((Entry > EntryLimit) || (Index == EndIndex)) {
      Ret = SvsmMsrProtocol (&SvsmCallData);
      if ((Ret == SVSM_ERR_PVALIDATE_FAIL_SIZE_MISMATCH) &&
          (Request->Entry[Request->Header.Next].Bits.PageSize != 0))
      {
        // Calculate the Index of the entry after the entry that failed
        // before clearing the buffer so that processing can continue
        // from that point
        Index = Index - (Entry - Request->Header.Next) + 2;

        // Obtain the failing GFN before clearing the buffer
        Gfn = Request->Entry[Request->Header.Next].Bits.Address;

        // Clear the buffer in prep for creating all new entries
        ZeroMem (Caa->SvsmBuffer, sizeof (Caa->SvsmBuffer));
        Entry = 0;

        GfnEnd = Gfn + PAGES_PER_2MB_ENTRY - 1;
        for ( ; Gfn <= GfnEnd; Gfn++) {
          Request->Header.Entries++;
          Request->Entry[Entry].Bits.PageSize = 0;
          Request->Entry[Entry].Bits.Action   = (Validate == TRUE) ? 1 : 0;
          Request->Entry[Entry].Bits.IgnoreCf = 0;
          Request->Entry[Entry].Bits.Address  = Gfn;

          Entry++;
          if ((Entry > EntryLimit) || (Gfn == GfnEnd)) {
            Ret = SvsmMsrProtocol (&SvsmCallData);
            if (Ret != 0) {
              SnpTerminate ();
            }

            ZeroMem (Caa->SvsmBuffer, sizeof (Caa->SvsmBuffer));
            Entry = 0;
          }
        }

        continue;
      }

      if (Ret != 0) {
        SnpTerminate ();
      }

      ZeroMem (Caa->SvsmBuffer, sizeof (Caa->SvsmBuffer));
      Entry = 0;
    }

    Index++;
  }
}

/**
  Perform a native PVALIDATE operation for the page ranges specified.

  Validate or rescind the validation of the specified pages.

  @param[in]       Info           Pointer to a page state change structure

**/
STATIC
VOID
BasePvalidate (
  IN  SNP_PAGE_STATE_CHANGE_INFO  *Info
  )
{
  UINTN                 RmpPageSize;
  UINTN                 StartIndex;
  UINTN                 EndIndex;
  UINTN                 Index;
  UINTN                 Ret;
  EFI_PHYSICAL_ADDRESS  Address;
  BOOLEAN               Validate;

  StartIndex = Info->Header.CurrentEntry;
  EndIndex   = Info->Header.EndEntry;

  for ( ; StartIndex <= EndIndex; StartIndex++) {
    //
    // Get the address and the page size from the Info.
    //
    Address     = ((EFI_PHYSICAL_ADDRESS)Info->Entry[StartIndex].GuestFrameNumber) << EFI_PAGE_SHIFT;
    RmpPageSize = Info->Entry[StartIndex].PageSize;
    Validate    = Info->Entry[StartIndex].Operation == SNP_PAGE_STATE_PRIVATE;

    Ret = AsmPvalidate (RmpPageSize, Validate, Address);

    //
    // If we fail to validate due to size mismatch then try with the
    // smaller page size. This senario will occur if the backing page in
    // the RMP entry is 4K and we are validating it as a 2MB.
    //
    if ((Ret == PVALIDATE_RET_SIZE_MISMATCH) && (RmpPageSize == PvalidatePageSize2MB)) {
      for (Index = 0; Index < PAGES_PER_2MB_ENTRY; Index++) {
        Ret = AsmPvalidate (PvalidatePageSize4K, Validate, Address);
        if (Ret) {
          break;
        }

        Address = Address + EFI_PAGE_SIZE;
      }
    }

    //
    // If validation failed then do not continue.
    //
    if (Ret) {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%a: Failed to %a address 0x%Lx Error code %d\n",
        gEfiCallerBaseName,
        __func__,
        Validate ? "Validate" : "Invalidate",
        Address,
        Ret
        ));

      SnpTerminate ();
    }
  }
}

/**
  Perform a PVALIDATE operation for the page ranges specified.

  Validate or rescind the validation of the specified pages.

  @param[in]       Info           Pointer to a page state change structure

**/
VOID
EFIAPI
AmdSvsmSnpPvalidate (
  IN SNP_PAGE_STATE_CHANGE_INFO  *Info
  )
{
  AmdSvsmIsSvsmPresent () ? SvsmPvalidate (Info) : BasePvalidate (Info);
}

/**
  Perform an RMPADJUST operation to alter the VMSA setting of a page.

  Add or remove the VMSA attribute for a page.

  @param[in]       Vmsa           Pointer to an SEV-ES save area page
  @param[in]       ApicId         APIC ID associated with the VMSA
  @param[in]       SetVmsa        Boolean indicator as to whether to set or
                                  or clear the VMSA setting for the page

  @retval  EFI_SUCCESS            RMPADJUST operation successful
  @retval  EFI_UNSUPPORTED        Operation is not supported
  @retval  EFI_INVALID_PARAMETER  RMPADJUST operation failed, an invalid
                                  parameter was supplied

**/
STATIC
EFI_STATUS
SvsmVmsaRmpAdjust (
  IN SEV_ES_SAVE_AREA  *Vmsa,
  IN UINT32            ApicId,
  IN BOOLEAN           SetVmsa
  )
{
  SVSM_CALL_DATA  SvsmCallData;
  SVSM_FUNCTION   Function;
  UINTN           Ret;

  SvsmCallData.Caa = (SVSM_CAA *)AmdSvsmSnpGetCaa ();

  Function.Id.Protocol = SVSM_PROTOCOL_CORE;

  if (SetVmsa) {
    Function.Id.CallId = SVSM_CORE_CREATE_VCPU;

    SvsmCallData.RaxIn = Function.Uint64;
    SvsmCallData.RcxIn = (UINT64)(UINTN)Vmsa;
    SvsmCallData.RdxIn = (UINT64)(UINTN)Vmsa + SIZE_4KB;
    SvsmCallData.R8In  = ApicId;
  } else {
    Function.Id.CallId = SVSM_CORE_DELETE_VCPU;

    SvsmCallData.RaxIn = Function.Uint64;
    SvsmCallData.RcxIn = (UINT64)(UINTN)Vmsa;
  }

  Ret = SvsmMsrProtocol (&SvsmCallData);

  return (Ret == 0) ? EFI_SUCCESS : EFI_INVALID_PARAMETER;
}

/**
  Perform a native RMPADJUST operation to alter the VMSA setting of a page.

  Add or remove the VMSA attribute for a page.

  @param[in]       Vmsa           Pointer to an SEV-ES save area page
  @param[in]       SetVmsa        Boolean indicator as to whether to set or
                                  or clear the VMSA setting for the page

  @retval  EFI_SUCCESS            RMPADJUST operation successful
  @retval  EFI_INVALID_PARAMETER  RMPADJUST operation failed, an invalid
                                  parameter was supplied

**/
STATIC
EFI_STATUS
BaseVmsaRmpAdjust (
  IN SEV_ES_SAVE_AREA  *Vmsa,
  IN BOOLEAN           SetVmsa
  )
{
  UINT64  Rdx;
  UINT32  Ret;

  //
  // The RMPADJUST instruction is used to set or clear the VMSA bit for a
  // page. The VMSA change is only made when running at VMPL0 and is ignored
  // otherwise. If too low a target VMPL is specified, the instruction can
  // succeed without changing the VMSA bit when not running at VMPL0. Using a
  // target VMPL level of 1, RMPADJUST will return a FAIL_PERMISSION error if
  // not running at VMPL0, thus ensuring that the VMSA bit is set appropriately
  // when no error is returned.
  //
  Rdx = 1;
  if (SetVmsa) {
    Rdx |= RMPADJUST_VMSA_PAGE_BIT;
  }

  Ret = AsmRmpAdjust ((UINT64)(UINTN)Vmsa, 0, Rdx);

  return (Ret == 0) ? EFI_SUCCESS : EFI_INVALID_PARAMETER;
}

/**
  Perform an RMPADJUST operation to alter the VMSA setting of a page.

  Add or remove the VMSA attribute for a page.

  @param[in]       Vmsa           Pointer to an SEV-ES save area page
  @param[in]       ApicId         APIC ID associated with the VMSA
  @param[in]       SetVmsa        Boolean indicator as to whether to set or
                                  or clear the VMSA setting for the page

  @retval  EFI_SUCCESS            RMPADJUST operation successful
  @retval  EFI_UNSUPPORTED        Operation is not supported
  @retval  EFI_INVALID_PARAMETER  RMPADJUST operation failed, an invalid
                                  parameter was supplied

**/
EFI_STATUS
EFIAPI
AmdSvsmSnpVmsaRmpAdjust (
  IN SEV_ES_SAVE_AREA  *Vmsa,
  IN UINT32            ApicId,
  IN BOOLEAN           SetVmsa
  )
{
  return AmdSvsmIsSvsmPresent () ? SvsmVmsaRmpAdjust (Vmsa, ApicId, SetVmsa)
                                : BaseVmsaRmpAdjust (Vmsa, SetVmsa);
}

/**
  Perform a SVSM_VTPM_QUERY operation

  Query the support provided by the SVSM vTPM.

  @param[out] PlatformCommands    It will contain a bitmap indicating the
                                  supported vTPM platform commands.
  @param[out] Features            It will contain a bitmap indicating the
                                  supported vTPM features.

  @retval TRUE                    The query was processed.
  @retval FALSE                   The query was not processed.

**/
BOOLEAN
EFIAPI
AmdSvsmVtpmQuery (
  OUT UINT64  *PlatformCommands,
  OUT UINT64  *Features
  )
{
  SVSM_CALL_DATA  SvsmCallData;
  SVSM_FUNCTION   Function;
  UINTN           Ret;

  if (!PlatformCommands && !Features) {
    return FALSE;
  }

  if (!AmdSvsmIsSvsmPresent ()) {
    return FALSE;
  }

  Function.Id.Protocol = SVSM_PROTOCOL_VTPM;
  Function.Id.CallId   = SVSM_VTPM_QUERY;

  SvsmCallData.Caa   = (SVSM_CAA *)AmdSvsmSnpGetCaa ();
  SvsmCallData.RaxIn = Function.Uint64;

  Ret = SvsmMsrProtocol (&SvsmCallData);
  if (Ret != 0) {
    return FALSE;
  }

  if (PlatformCommands) {
    *PlatformCommands = SvsmCallData.RcxOut;
  }

  if (Features) {
    *Features = SvsmCallData.RdxOut;
  }

  return TRUE;
}

/**
  Perform a SVSM_VTPM_CMD operation

  Send the specified vTPM platform command to the SVSM vTPM.

  @param[in, out] Buffer  It should contain the vTPM platform command
                          request. The respective response will be returned
                          in the same Buffer, but not all commands specify a
                          response.

  @retval TRUE            The command was processed.
  @retval FALSE           The command was not processed.

**/
BOOLEAN
EFIAPI
AmdSvsmVtpmCmd (
  IN OUT UINT8  *Buffer
  )
{
  SVSM_CALL_DATA  SvsmCallData;
  SVSM_FUNCTION   Function;
  UINTN           Ret;

  if (!AmdSvsmIsSvsmPresent ()) {
    return FALSE;
  }

  Function.Id.Protocol = SVSM_PROTOCOL_VTPM;
  Function.Id.CallId   = SVSM_VTPM_CMD;

  SvsmCallData.Caa   = (SVSM_CAA *)AmdSvsmSnpGetCaa ();
  SvsmCallData.RaxIn = Function.Uint64;
  SvsmCallData.RcxIn = (UINT64)(UINTN)Buffer;

  Ret = SvsmMsrProtocol (&SvsmCallData);

  return (Ret == 0) ? TRUE : FALSE;
}

BOOLEAN
EFIAPI
AmdSvsmQueryProtocol (
  IN  UINT32  ProtocolId,
  IN  UINT32  ProtocolVersion,
  OUT UINT32  *ProtocolMin,
  OUT UINT32  *ProtocolMax
  )
{
  SVSM_CALL_DATA  SvsmCallData;
  SVSM_FUNCTION   Function;
  UINT64          Rcx;
  UINTN           Ret;

  if (!AmdSvsmIsSvsmPresent ()) {
    return FALSE;
  }

  Function.Id.Protocol = SVSM_PROTOCOL_CORE;
  Function.Id.CallId   = SVSM_CORE_QUERY_PROTOCOL;

  Rcx = ((UINT64)ProtocolId << 32) | ProtocolVersion;

  SvsmCallData.Caa   = (SVSM_CAA *)AmdSvsmSnpGetCaa ();
  SvsmCallData.RaxIn = Function.Uint64;
  SvsmCallData.RcxIn = Rcx;

  Ret = SvsmMsrProtocol (&SvsmCallData);
  if (Ret != 0) {
    return FALSE;
  }

  if (SvsmCallData.RcxOut == 0) {
    return FALSE;
  }

  if (ProtocolMin) {
    *ProtocolMin = (UINT32)(SvsmCallData.RcxOut & 0xffffffff);
  }

  if (ProtocolMax) {
    *ProtocolMax = (UINT32)(SvsmCallData.RcxOut >> 32);
  }

  return TRUE;
}

BOOLEAN
EFIAPI
AmdSvsmUefiMmRequest (
  IN  UINT64  BufferAddr,
  IN  UINT64  BufferSize
  )
{
  SVSM_CALL_DATA  SvsmCallData;
  SVSM_FUNCTION   Function;
  UINT64          Caa;
  UINTN           Ret;

  Function.Id.Protocol = SVSM_UEFI_MM_PROTOCOL;
  Function.Id.CallId   = SVSM_UEFI_MM_REQUEST;

  // this works at runtime too
  Caa = AsmReadMsr64 (MSR_SVSM_CAA);

  SvsmCallData.Caa   = (SVSM_CAA *)Caa;
  SvsmCallData.RaxIn = Function.Uint64;
  SvsmCallData.RcxIn = BufferAddr;
  SvsmCallData.RdxIn = BufferSize;

  Ret = SvsmMsrProtocol (&SvsmCallData);
  return (Ret == 0) ? TRUE : FALSE;
}
