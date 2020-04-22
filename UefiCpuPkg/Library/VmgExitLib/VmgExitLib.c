/** @file
  VMGEXIT Support Library.

  Copyright (c) 2019, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Register/Amd/Ghcb.h>
#include <Register/Amd/Msr.h>

/**
  Check for VMGEXIT error

  Check if the hypervisor has returned an error after completion of the VMGEXIT
  by examining the SwExitInfo1 field of the GHCB.

  @param[in]  Ghcb       A pointer to the GHCB

  @retval  0             VMGEXIT succeeded.
  @retval  Others        VMGEXIT processing did not succeed. Exception number to
                         be propagated.

**/
STATIC
UINT64
VmgExitErrorCheck (
  IN GHCB                *Ghcb
  )
{
  GHCB_EVENT_INJECTION  Event;
  GHCB_EXIT_INFO        ExitInfo;
  UINT64                Status;

  ExitInfo.Uint64 = Ghcb->SaveArea.SwExitInfo1;
  ASSERT ((ExitInfo.Elements.Lower32Bits == 0) ||
          (ExitInfo.Elements.Lower32Bits == 1));

  Status = 0;
  if (ExitInfo.Elements.Lower32Bits == 0) {
    return Status;
  }

  if (ExitInfo.Elements.Lower32Bits == 1) {
    ASSERT (Ghcb->SaveArea.SwExitInfo2 != 0);

    // Check that the return event is valid
    Event.Uint64 = Ghcb->SaveArea.SwExitInfo2;
    if (Event.Elements.Valid &&
        Event.Elements.Type == GHCB_EVENT_INJECTION_TYPE_EXCEPTION) {
      switch (Event.Elements.Vector) {
      case GP_EXCEPTION:
      case UD_EXCEPTION:
        // Use returned event as return code
        Status = Event.Uint64;
      }
    }
  }

  if (Status == 0) {
    GHCB_EVENT_INJECTION  Event;

    Event.Uint64 = 0;
    Event.Elements.Vector = GP_EXCEPTION;
    Event.Elements.Type   = GHCB_EVENT_INJECTION_TYPE_EXCEPTION;
    Event.Elements.Valid  = 1;

    Status = Event.Uint64;
  }

  return Status;
}

/**
  Perform VMGEXIT.

  Sets the necessary fields of the GHCB, invokes the VMGEXIT instruction and
  then handles the return actions.

  @param[in, out]  Ghcb       A pointer to the GHCB
  @param[in]       ExitCode   VMGEXIT code to be assigned to the SwExitCode
                              field of the GHCB.
  @param[in]       ExitInfo1  VMGEXIT information to be assigned to the
                              SwExitInfo1 field of the GHCB.
  @param[in]       ExitInfo2  VMGEXIT information to be assigned to the
                              SwExitInfo2 field of the GHCB.

  @retval  0                  VMGEXIT succeeded.
  @retval  Others             VMGEXIT processing did not succeed. Exception
                              number to be propagated.

**/
UINT64
EFIAPI
VmgExit (
  IN OUT GHCB                *Ghcb,
  IN     UINT64              ExitCode,
  IN     UINT64              ExitInfo1,
  IN     UINT64              ExitInfo2
  )
{
  Ghcb->SaveArea.SwExitCode = ExitCode;
  Ghcb->SaveArea.SwExitInfo1 = ExitInfo1;
  Ghcb->SaveArea.SwExitInfo2 = ExitInfo2;

  //
  // Guest memory is used for the guest-hypervisor communication, so fence
  // the invocation of the VMGEXIT instruction to ensure GHCB accesses are
  // synchronized properly.
  //
  MemoryFence ();
  AsmVmgExit ();
  MemoryFence ();

  return VmgExitErrorCheck (Ghcb);
}

/**
  Perform pre-VMGEXIT initialization/preparation.

  Performs the necessary steps in preparation for invoking VMGEXIT. Must be
  called before setting any fields within the GHCB.

  @param[in, out]  Ghcb       A pointer to the GHCB

**/
VOID
EFIAPI
VmgInit (
  IN OUT GHCB                *Ghcb
  )
{
  SetMem (&Ghcb->SaveArea, sizeof (Ghcb->SaveArea), 0);
}

/**
  Perform post-VMGEXIT cleanup.

  Performs the necessary steps to cleanup after invoking VMGEXIT. Must be
  called after obtaining needed fields within the GHCB.

  @param[in, out]  Ghcb       A pointer to the GHCB

**/
VOID
EFIAPI
VmgDone (
  IN OUT GHCB                *Ghcb
  )
{
}

/**
  Perform VMGEXIT MMIO read or write.

  Performs the requested MMIO read or write using the VMGEXIT instruction.

  For an MMIO read, the data that has been read during the VMGEXIT is placed in
  the SharedBuffer area of the GHCB. This is then copied to the actual
  destination buffer within the guest.

  For an MMIO write, the data to be written is copied into the SharedBuffer area
  of the GHCB by the guest. This is then copied to the actual destination buffer
  by the hypervisor during the VMGEXIT.

  @param[in, out]  MmioAddress  A pointer to the MMIO buffer to be read/written
  @param[in, out]  Buffer       A pointer to the buffer to hold the data thas
                                has been read or hold the data to be written
  @param[in]       Bytes        Number of bytes to read or write
  @param[in]       Write        If set, the request is for an MMIO write, else
                                it is an MMIO read.

  @retval  0                    VMGEXIT succeeded.
  @retval  Others               VMGEXIT processing did not succeed. Exception
                                number to be propagated.

**/
STATIC
UINT64
EFIAPI
VmgMmio (
  IN OUT UINT8               *MmioAddress,
  IN OUT UINT8               *Buffer,
  IN     UINTN               Bytes,
  IN     BOOLEAN             Write
  )
{
  UINT64                    MmioOp, ExitInfo1, ExitInfo2, Status;
  GHCB                      *Ghcb;
  MSR_SEV_ES_GHCB_REGISTER  Msr;

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
  Ghcb = Msr.Ghcb;

  //
  // This function is about to set fields in the GHCB. Do not execute
  // anything that will cause a #VC before issuing the VmgExit(). Any #VC
  // will result in all GHCB settings being overwritten (this means, e.g.,
  // do not add DEBUG() statements).
  //
  VmgInit (Ghcb);

  if (Write) {
    MmioOp = SvmExitMmioWrite;
  } else {
    MmioOp = SvmExitMmioRead;
  }

  ExitInfo1 = (UINT64) (UINTN) MmioAddress;
  ExitInfo2 = Bytes;

  if (Write) {
    CopyMem (Ghcb->SharedBuffer, Buffer, Bytes);
  }

  Ghcb->SaveArea.SwScratch = (UINT64) (UINTN) Ghcb->SharedBuffer;
  Status = VmgExit (Ghcb, MmioOp, ExitInfo1, ExitInfo2);
  if (Status != 0) {
    return Status;
  }

  if (!Write) {
    CopyMem (Buffer, Ghcb->SharedBuffer, Bytes);
  }

  VmgDone (Ghcb);

  return 0;
}

/**
  Perform MMIO write of a buffer to a non-MMIO marked range.

  Performs an MMIO write without taking a #VC. This is useful
  for Flash devices, which are marked read-only.

  @param[in, out]  Dest       A pointer to the destination buffer
  @param[in]       Src        A pointer to the source data to be written
  @param[in]       Bytes      Number of bytes to write

**/
VOID
EFIAPI
VmgMmioWrite (
  IN OUT UINT8               *Dest,
  IN     UINT8               *Src,
  IN     UINTN                Bytes
  )
{
  VmgMmio (Dest, Src, Bytes, TRUE);
}

/**
  Issue the GHCB set AP Jump Table VMGEXIT.

  Performs a VMGEXIT using the GHCB AP Jump Table exit code to save the
  AP Jump Table address with the hypervisor for retrieval at a later time.

  @param[in]  Address  Physical address of the AP Jump Table

  @retval  0           VMGEXIT succeeded.
  @retval  Others      VMGEXIT processing did not succeed. Exception
                       number to be propagated.

**/
UINT64
EFIAPI
VmgExitSetAPJumpTable (
  IN EFI_PHYSICAL_ADDRESS  Address
  )
{
  UINT64                    ExitInfo1, ExitInfo2, Status;
  GHCB                      *Ghcb;
  MSR_SEV_ES_GHCB_REGISTER  Msr;

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
  Ghcb = Msr.Ghcb;

  VmgInit (Ghcb);

  ExitInfo1 = 0;
  ExitInfo2 = (UINT64) (UINTN) Address;

  Status = VmgExit (Ghcb, SvmExitApJumpTable, ExitInfo1, ExitInfo2);

  VmgDone (Ghcb);

  return Status;
}

