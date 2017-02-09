/** @file
  Append an ACPI S3 Boot Script fragment from the QEMU_LOADER_WRITE_POINTER
  commands of QEMU's fully processed table linker/loader script.

  Copyright (C) 2017, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Protocol/S3SaveState.h>

#include "AcpiPlatform.h"


//
// Condensed structure for capturing the fw_cfg operations -- select, skip,
// write -- inherent in executing a QEMU_LOADER_WRITE_POINTER command.
//
typedef struct {
  UINT16 PointerItem;   // resolved from QEMU_LOADER_WRITE_POINTER.PointerFile
  UINT8  PointerSize;   // copied as-is from QEMU_LOADER_WRITE_POINTER
  UINT32 PointerOffset; // copied as-is from QEMU_LOADER_WRITE_POINTER
  UINT64 PointerValue;  // resolved from QEMU_LOADER_WRITE_POINTER.PointeeFile
} CONDENSED_WRITE_POINTER;


//
// Context structure to accumulate CONDENSED_WRITE_POINTER objects from
// QEMU_LOADER_WRITE_POINTER commands.
//
// Any pointers in this structure own the pointed-to objects; that is, when the
// context structure is released, all pointed-to objects must be released too.
//
struct S3_CONTEXT {
  CONDENSED_WRITE_POINTER *WritePointers; // one array element per processed
                                          //   QEMU_LOADER_WRITE_POINTER
                                          //   command
  UINTN                   Allocated;      // number of elements allocated for
                                          //   WritePointers
  UINTN                   Used;           // number of elements populated in
                                          //   WritePointers
};


//
// Scratch buffer, allocated in EfiReservedMemoryType type memory, for the ACPI
// S3 Boot Script opcodes to work on. We use the buffer to compose and to
// replay several fw_cfg select+skip and write operations, using the DMA access
// method. The fw_cfg operations will implement the actions dictated by
// CONDENSED_WRITE_POINTER objects.
//
#pragma pack (1)
typedef struct {
  FW_CFG_DMA_ACCESS Access;       // filled in from
                                  //   CONDENSED_WRITE_POINTER.PointerItem,
                                  //   CONDENSED_WRITE_POINTER.PointerSize,
                                  //   CONDENSED_WRITE_POINTER.PointerOffset
  UINT64            PointerValue; // filled in from
                                  //   CONDENSED_WRITE_POINTER.PointerValue
} SCRATCH_BUFFER;
#pragma pack ()


/**
  Allocate an S3_CONTEXT object.

  @param[out] S3Context         The allocated S3_CONTEXT object is returned
                                through this parameter.

  @param[in] WritePointerCount  Number of CONDENSED_WRITE_POINTER elements to
                                allocate room for. WritePointerCount must be
                                positive.

  @retval EFI_SUCCESS            Allocation successful.

  @retval EFI_OUT_OF_RESOURCES   Out of memory.

  @retval EFI_INVALID_PARAMETER  WritePointerCount is zero.
**/
EFI_STATUS
AllocateS3Context (
  OUT S3_CONTEXT **S3Context,
  IN  UINTN      WritePointerCount
  )
{
  EFI_STATUS Status;
  S3_CONTEXT *Context;

  if (WritePointerCount == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Context = AllocateZeroPool (sizeof *Context);
  if (Context == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Context->WritePointers = AllocatePool (WritePointerCount *
                             sizeof *Context->WritePointers);
  if (Context->WritePointers == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeContext;
  }

  Context->Allocated = WritePointerCount;
  *S3Context = Context;
  return EFI_SUCCESS;

FreeContext:
  FreePool (Context);

  return Status;
}


/**
  Release an S3_CONTEXT object.

  @param[in] S3Context  The object to release.
**/
VOID
ReleaseS3Context (
  IN S3_CONTEXT *S3Context
  )
{
  FreePool (S3Context->WritePointers);
  FreePool (S3Context);
}


/**
  Save the information necessary to replicate a QEMU_LOADER_WRITE_POINTER
  command during S3 resume, in condensed format.

  This function is to be called from ProcessCmdWritePointer(), after all the
  sanity checks have passed, and before the fw_cfg operations are performed.

  @param[in,out] S3Context  The S3_CONTEXT object into which the caller wants
                            to save the information that was derived from
                            QEMU_LOADER_WRITE_POINTER.

  @param[in] PointerItem    The FIRMWARE_CONFIG_ITEM that
                            QEMU_LOADER_WRITE_POINTER.PointerFile was resolved
                            to, expressed as a UINT16 value.

  @param[in] PointerSize    Copied directly from
                            QEMU_LOADER_WRITE_POINTER.PointerSize.

  @param[in] PointerOffset  Copied directly from
                            QEMU_LOADER_WRITE_POINTER.PointerOffset.

  @param[in] PointerValue   The base address of the allocated / downloaded
                            fw_cfg blob that is identified by
                            QEMU_LOADER_WRITE_POINTER.PointeeFile.

  @retval EFI_SUCCESS           The information derived from
                                QEMU_LOADER_WRITE_POINTER has been successfully
                                absorbed into S3Context.

  @retval EFI_OUT_OF_RESOURCES  No room available in S3Context.
**/
EFI_STATUS
SaveCondensedWritePointerToS3Context (
  IN OUT S3_CONTEXT *S3Context,
  IN     UINT16     PointerItem,
  IN     UINT8      PointerSize,
  IN     UINT32     PointerOffset,
  IN     UINT64     PointerValue
  )
{
  CONDENSED_WRITE_POINTER *Condensed;

  if (S3Context->Used == S3Context->Allocated) {
    return EFI_OUT_OF_RESOURCES;
  }
  Condensed = S3Context->WritePointers + S3Context->Used;
  Condensed->PointerItem   = PointerItem;
  Condensed->PointerSize   = PointerSize;
  Condensed->PointerOffset = PointerOffset;
  Condensed->PointerValue  = PointerValue;
  DEBUG ((DEBUG_VERBOSE, "%a: 0x%04x/[0x%08x+%d] := 0x%Lx (%Lu)\n",
    __FUNCTION__, PointerItem, PointerOffset, PointerSize, PointerValue,
    (UINT64)S3Context->Used));
  ++S3Context->Used;
  return EFI_SUCCESS;
}


/**
  Translate and append the information from an S3_CONTEXT object to the ACPI S3
  Boot Script.

  The effects of a successful call to this function cannot be undone.

  @param[in] S3Context  The S3_CONTEXT object to translate to ACPI S3 Boot
                        Script opcodes.

  @retval EFI_OUT_OF_RESOURCES  Out of memory.

  @retval EFI_SUCCESS           The translation of S3Context to ACPI S3 Boot
                                Script opcodes has been successful.

  @return                       Error codes from underlying functions.
**/
EFI_STATUS
TransferS3ContextToBootScript (
  IN CONST S3_CONTEXT *S3Context
  )
{
  EFI_STATUS                 Status;
  EFI_S3_SAVE_STATE_PROTOCOL *S3SaveState;
  SCRATCH_BUFFER             *ScratchBuffer;
  FW_CFG_DMA_ACCESS          *Access;
  UINT64                     BigEndianAddressOfAccess;
  UINT32                     ControlPollData;
  UINT32                     ControlPollMask;
  UINTN                      Index;

  //
  // If the following protocol lookup fails, it shall not happen due to an
  // unexpected DXE driver dispatch order.
  //
  // Namely, this function is only invoked on QEMU. Therefore it is only
  // reached after Platform BDS signals gRootBridgesConnectedEventGroupGuid
  // (see OnRootBridgesConnected() in "EntryPoint.c"). Hence, because
  // TransferS3ContextToBootScript() is invoked in BDS, all DXE drivers,
  // including S3SaveStateDxe (producing EFI_S3_SAVE_STATE_PROTOCOL), have been
  // dispatched by the time we get here. (S3SaveStateDxe is not expected to
  // have any stricter-than-TRUE DEPEX -- not a DEPEX that gets unblocked only
  // within BDS anyway.)
  //
  // Reaching this function also depends on QemuFwCfgS3Enabled(). That implies
  // S3SaveStateDxe has not exited immediately due to S3 being disabled. Thus
  // EFI_S3_SAVE_STATE_PROTOCOL can only be missing for genuinely unforeseeable
  // reasons.
  //
  Status = gBS->LocateProtocol (&gEfiS3SaveStateProtocolGuid,
                  NULL /* Registration */, (VOID **)&S3SaveState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: LocateProtocol(): %r\n", __FUNCTION__, Status));
    return Status;
  }

  ScratchBuffer = AllocateReservedPool (sizeof *ScratchBuffer);
  if (ScratchBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set up helper variables that we'll use identically for all
  // CONDENSED_WRITE_POINTER elements.
  //
  Access = &ScratchBuffer->Access;
  BigEndianAddressOfAccess = SwapBytes64 ((UINTN)Access);
  ControlPollData = 0;
  ControlPollMask = MAX_UINT32;

  //
  // For each CONDENSED_WRITE_POINTER, we need six ACPI S3 Boot Script opcodes:
  // (1) restore an FW_CFG_DMA_ACCESS object in reserved memory that selects
  //     the writeable fw_cfg file PointerFile (through PointerItem), and skips
  //     to PointerOffset in it,
  // (2) call QEMU with the FW_CFG_DMA_ACCESS object,
  // (3) wait for the select+skip to finish,
  // (4) restore a SCRATCH_BUFFER object in reserved memory that writes
  //     PointerValue (base address of the allocated / downloaded PointeeFile),
  //     of size PointerSize, into the fw_cfg file selected in (1), at the
  //     offset sought to in (1),
  // (5) call QEMU with the FW_CFG_DMA_ACCESS object,
  // (6) wait for the write to finish.
  //
  // EFI_S3_SAVE_STATE_PROTOCOL does not allow rolling back opcode additions,
  // therefore we treat any failure here as fatal.
  //
  for (Index = 0; Index < S3Context->Used; ++Index) {
    CONST CONDENSED_WRITE_POINTER *Condensed;

    Condensed = &S3Context->WritePointers[Index];

    //
    // (1) restore an FW_CFG_DMA_ACCESS object in reserved memory that selects
    //     the writeable fw_cfg file PointerFile (through PointerItem), and
    //     skips to PointerOffset in it,
    //
    Access->Control = SwapBytes32 ((UINT32)Condensed->PointerItem << 16 |
                        FW_CFG_DMA_CTL_SELECT | FW_CFG_DMA_CTL_SKIP);
    Access->Length = SwapBytes32 (Condensed->PointerOffset);
    Access->Address = 0;
    Status = S3SaveState->Write (
                            S3SaveState,                      // This
                            EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE, // OpCode
                            EfiBootScriptWidthUint8,          // Width
                            (UINT64)(UINTN)Access,            // Address
                            sizeof *Access,                   // Count
                            Access                            // Buffer
                            );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Index %Lu opcode 1: %r\n", __FUNCTION__,
        (UINT64)Index, Status));
      goto FatalError;
    }

    //
    // (2) call QEMU with the FW_CFG_DMA_ACCESS object,
    //
    Status = S3SaveState->Write (
                            S3SaveState,                     // This
                            EFI_BOOT_SCRIPT_IO_WRITE_OPCODE, // OpCode
                            EfiBootScriptWidthUint32,        // Width
                            (UINT64)0x514,                   // Address
                            (UINTN)2,                        // Count
                            &BigEndianAddressOfAccess        // Buffer
                            );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Index %Lu opcode 2: %r\n", __FUNCTION__,
        (UINT64)Index, Status));
      goto FatalError;
    }

    //
    // (3) wait for the select+skip to finish,
    //
    Status = S3SaveState->Write (
                            S3SaveState,                     // This
                            EFI_BOOT_SCRIPT_MEM_POLL_OPCODE, // OpCode
                            EfiBootScriptWidthUint32,        // Width
                            (UINT64)(UINTN)&Access->Control, // Address
                            &ControlPollData,                // Data
                            &ControlPollMask,                // DataMask
                            MAX_UINT64                       // Delay
                            );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Index %Lu opcode 3: %r\n", __FUNCTION__,
        (UINT64)Index, Status));
      goto FatalError;
    }

    //
    // (4) restore a SCRATCH_BUFFER object in reserved memory that writes
    //     PointerValue (base address of the allocated / downloaded
    //     PointeeFile), of size PointerSize, into the fw_cfg file selected in
    //     (1), at the offset sought to in (1),
    //
    Access->Control = SwapBytes32 (FW_CFG_DMA_CTL_WRITE);
    Access->Length = SwapBytes32 (Condensed->PointerSize);
    Access->Address = SwapBytes64 ((UINTN)&ScratchBuffer->PointerValue);
    ScratchBuffer->PointerValue = Condensed->PointerValue;
    Status = S3SaveState->Write (
                            S3SaveState,                      // This
                            EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE, // OpCode
                            EfiBootScriptWidthUint8,          // Width
                            (UINT64)(UINTN)ScratchBuffer,     // Address
                            sizeof *ScratchBuffer,            // Count
                            ScratchBuffer                     // Buffer
                            );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Index %Lu opcode 4: %r\n", __FUNCTION__,
        (UINT64)Index, Status));
      goto FatalError;
    }

    //
    // (5) call QEMU with the FW_CFG_DMA_ACCESS object,
    //
    Status = S3SaveState->Write (
                            S3SaveState,                     // This
                            EFI_BOOT_SCRIPT_IO_WRITE_OPCODE, // OpCode
                            EfiBootScriptWidthUint32,        // Width
                            (UINT64)0x514,                   // Address
                            (UINTN)2,                        // Count
                            &BigEndianAddressOfAccess        // Buffer
                            );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Index %Lu opcode 5: %r\n", __FUNCTION__,
        (UINT64)Index, Status));
      goto FatalError;
    }

    //
    // (6) wait for the write to finish.
    //
    Status = S3SaveState->Write (
                            S3SaveState,                     // This
                            EFI_BOOT_SCRIPT_MEM_POLL_OPCODE, // OpCode
                            EfiBootScriptWidthUint32,        // Width
                            (UINT64)(UINTN)&Access->Control, // Address
                            &ControlPollData,                // Data
                            &ControlPollMask,                // DataMask
                            MAX_UINT64                       // Delay
                            );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Index %Lu opcode 6: %r\n", __FUNCTION__,
        (UINT64)Index, Status));
      goto FatalError;
    }
  }

  DEBUG ((DEBUG_VERBOSE, "%a: boot script fragment saved, ScratchBuffer=%p\n",
    __FUNCTION__, (VOID *)ScratchBuffer));
  return EFI_SUCCESS;

FatalError:
  ASSERT (FALSE);
  CpuDeadLoop ();
  return Status;
}
