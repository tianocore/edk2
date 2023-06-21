/** @file
  Append an ACPI S3 Boot Script fragment from the QEMU_LOADER_WRITE_POINTER
  commands of QEMU's fully processed table linker/loader script.

  Copyright (C) 2017, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/AcpiPlatformLib.h>
#include <Library/BaseLib.h>             // CpuDeadLoop()
#include <Library/DebugLib.h>            // DEBUG()
#include <Library/MemoryAllocationLib.h> // AllocatePool()
#include <Library/QemuFwCfgS3Lib.h>      // QemuFwCfgS3ScriptSkipBytes()

//
// Condensed structure for capturing the fw_cfg operations -- select, skip,
// write -- inherent in executing a QEMU_LOADER_WRITE_POINTER command.
//
typedef struct {
  UINT16    PointerItem;   // resolved from QEMU_LOADER_WRITE_POINTER.PointerFile
  UINT8     PointerSize;   // copied as-is from QEMU_LOADER_WRITE_POINTER
  UINT32    PointerOffset; // copied as-is from QEMU_LOADER_WRITE_POINTER
  UINT64    PointerValue;  // resolved from QEMU_LOADER_WRITE_POINTER.PointeeFile
                           //   and QEMU_LOADER_WRITE_POINTER.PointeeOffset
} CONDENSED_WRITE_POINTER;

//
// Context structure to accumulate CONDENSED_WRITE_POINTER objects from
// QEMU_LOADER_WRITE_POINTER commands.
//
// Any pointers in this structure own the pointed-to objects; that is, when the
// context structure is released, all pointed-to objects must be released too.
//
struct S3_CONTEXT {
  CONDENSED_WRITE_POINTER    *WritePointers; // one array element per processed
                                             //   QEMU_LOADER_WRITE_POINTER
                                             //   command
  UINTN                      Allocated;      // number of elements allocated for
                                             //   WritePointers
  UINTN                      Used;           // number of elements populated in
                                             //   WritePointers
};

//
// Scratch buffer, allocated in EfiReservedMemoryType type memory, for the ACPI
// S3 Boot Script opcodes to work on.
//
#pragma pack (1)
typedef union {
  UINT64    PointerValue; // filled in from CONDENSED_WRITE_POINTER.PointerValue
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
  OUT S3_CONTEXT  **S3Context,
  IN  UINTN       WritePointerCount
  )
{
  EFI_STATUS  Status;
  S3_CONTEXT  *Context;

  if (WritePointerCount == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Context = AllocateZeroPool (sizeof *Context);
  if (Context == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Context->WritePointers = AllocatePool (
                             WritePointerCount *
                             sizeof *Context->WritePointers
                             );
  if (Context->WritePointers == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeContext;
  }

  Context->Allocated = WritePointerCount;
  *S3Context         = Context;
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
  IN S3_CONTEXT  *S3Context
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
                            QEMU_LOADER_WRITE_POINTER.PointeeFile, plus
                            QEMU_LOADER_WRITE_POINTER.PointeeOffset.

  @retval EFI_SUCCESS           The information derived from
                                QEMU_LOADER_WRITE_POINTER has been successfully
                                absorbed into S3Context.

  @retval EFI_OUT_OF_RESOURCES  No room available in S3Context.
**/
EFI_STATUS
SaveCondensedWritePointerToS3Context (
  IN OUT S3_CONTEXT  *S3Context,
  IN     UINT16      PointerItem,
  IN     UINT8       PointerSize,
  IN     UINT32      PointerOffset,
  IN     UINT64      PointerValue
  )
{
  CONDENSED_WRITE_POINTER  *Condensed;

  if (S3Context->Used == S3Context->Allocated) {
    return EFI_OUT_OF_RESOURCES;
  }

  Condensed                = S3Context->WritePointers + S3Context->Used;
  Condensed->PointerItem   = PointerItem;
  Condensed->PointerSize   = PointerSize;
  Condensed->PointerOffset = PointerOffset;
  Condensed->PointerValue  = PointerValue;
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: 0x%04x/[0x%08x+%d] := 0x%Lx (%Lu)\n",
    __func__,
    PointerItem,
    PointerOffset,
    PointerSize,
    PointerValue,
    (UINT64)S3Context->Used
    ));
  ++S3Context->Used;
  return EFI_SUCCESS;
}

/**
  FW_CFG_BOOT_SCRIPT_CALLBACK_FUNCTION provided to QemuFwCfgS3Lib.
**/
STATIC
VOID
EFIAPI
AppendFwCfgBootScript (
  IN OUT VOID  *Context               OPTIONAL,
  IN OUT VOID  *ExternalScratchBuffer
  )
{
  S3_CONTEXT      *S3Context;
  SCRATCH_BUFFER  *ScratchBuffer;
  UINTN           Index;

  S3Context     = Context;
  ScratchBuffer = ExternalScratchBuffer;

  for (Index = 0; Index < S3Context->Used; ++Index) {
    CONST CONDENSED_WRITE_POINTER  *Condensed;
    RETURN_STATUS                  Status;

    Condensed = &S3Context->WritePointers[Index];

    Status = QemuFwCfgS3ScriptSkipBytes (
               Condensed->PointerItem,
               Condensed->PointerOffset
               );
    if (RETURN_ERROR (Status)) {
      goto FatalError;
    }

    ScratchBuffer->PointerValue = Condensed->PointerValue;
    Status                      = QemuFwCfgS3ScriptWriteBytes (-1, Condensed->PointerSize);
    if (RETURN_ERROR (Status)) {
      goto FatalError;
    }
  }

  DEBUG ((DEBUG_VERBOSE, "%a: boot script fragment saved\n", __func__));

  ReleaseS3Context (S3Context);
  return;

FatalError:
  ASSERT (FALSE);
  CpuDeadLoop ();
}

/**
  Translate and append the information from an S3_CONTEXT object to the ACPI S3
  Boot Script.

  The effects of a successful call to this function cannot be undone.

  @param[in] S3Context  The S3_CONTEXT object to translate to ACPI S3 Boot
                        Script opcodes. If the function returns successfully,
                        the caller must set the S3Context pointer -- originally
                        returned by AllocateS3Context() -- immediately to NULL,
                        because the ownership of S3Context has been transferred.

  @retval EFI_SUCCESS The translation of S3Context to ACPI S3 Boot Script
                      opcodes has been successfully executed or queued. (This
                      includes the case when S3Context was empty on input and
                      no ACPI S3 Boot Script opcodes have been necessary to
                      produce.)

  @return             Error codes from underlying functions.
**/
EFI_STATUS
TransferS3ContextToBootScript (
  IN S3_CONTEXT  *S3Context
  )
{
  RETURN_STATUS  Status;

  if (S3Context->Used == 0) {
    ReleaseS3Context (S3Context);
    return EFI_SUCCESS;
  }

  Status = QemuFwCfgS3CallWhenBootScriptReady (
             AppendFwCfgBootScript,
             S3Context,
             sizeof (SCRATCH_BUFFER)
             );
  return (EFI_STATUS)Status;
}
