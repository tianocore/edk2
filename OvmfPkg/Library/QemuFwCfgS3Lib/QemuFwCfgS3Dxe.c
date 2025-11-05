/** @file
  Full functionality QemuFwCfgS3Lib instance, for DXE phase modules.

  Copyright (C) 2017, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/QemuFwCfgS3Lib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/S3SaveState.h>

//
// Event to signal when the S3SaveState protocol interface is installed.
//
STATIC EFI_EVENT  mS3SaveStateInstalledEvent;

//
// Reference to the S3SaveState protocol interface, after it is installed.
//
STATIC EFI_S3_SAVE_STATE_PROTOCOL  *mS3SaveState;

//
// The control structure is allocated in reserved memory, aligned at 8 bytes.
// The client-requested ScratchBuffer will be allocated adjacently, also
// aligned at 8 bytes.
//
#define RESERVED_MEM_ALIGNMENT  8

STATIC FW_CFG_DMA_ACCESS  *mDmaAccess;
STATIC VOID               *mScratchBuffer;
STATIC UINTN              mScratchBufferSize;

//
// Callback provided by the client, for appending ACPI S3 Boot Script opcodes.
// To be called from S3SaveStateInstalledNotify().
//
STATIC FW_CFG_BOOT_SCRIPT_CALLBACK_FUNCTION  *mCallback;

/**
  Event notification function for mS3SaveStateInstalledEvent.
**/
STATIC
VOID
EFIAPI
S3SaveStateInstalledNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  ASSERT (Event == mS3SaveStateInstalledEvent);

  Status = gBS->LocateProtocol (
                  &gEfiS3SaveStateProtocolGuid,
                  NULL /* Registration */,
                  (VOID **)&mS3SaveState
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  ASSERT (mCallback != NULL);

  DEBUG ((
    DEBUG_INFO,
    "%a: %a: DmaAccess@0x%Lx ScratchBuffer@[0x%Lx+0x%Lx]\n",
    gEfiCallerBaseName,
    __func__,
    (UINT64)(UINTN)mDmaAccess,
    (UINT64)(UINTN)mScratchBuffer,
    (UINT64)mScratchBufferSize
    ));
  mCallback (Context, mScratchBuffer);

  gBS->CloseEvent (mS3SaveStateInstalledEvent);
  mS3SaveStateInstalledEvent = NULL;
}

/**
  Install the client module's FW_CFG_BOOT_SCRIPT_CALLBACK_FUNCTION callback for
  when the production of ACPI S3 Boot Script opcodes becomes possible.

  Take ownership of the client-provided Context, and pass it to the callback
  function, when the latter is invoked.

  Allocate scratch space for those ACPI S3 Boot Script opcodes to work upon
  that the client will produce in the callback function.

  @param[in] Callback           FW_CFG_BOOT_SCRIPT_CALLBACK_FUNCTION to invoke
                                when the production of ACPI S3 Boot Script
                                opcodes becomes possible. Callback() may be
                                called immediately from
                                QemuFwCfgS3CallWhenBootScriptReady().

  @param[in,out] Context        Client-provided data structure for the
                                Callback() callback function to consume.

                                If Context points to dynamically allocated
                                memory, then Callback() must release it.

                                If Context points to dynamically allocated
                                memory, and
                                QemuFwCfgS3CallWhenBootScriptReady() returns
                                successfully, then the caller of
                                QemuFwCfgS3CallWhenBootScriptReady() must
                                neither dereference nor even evaluate Context
                                any longer, as ownership of the referenced area
                                has been transferred to Callback().

  @param[in] ScratchBufferSize  The size of the scratch buffer that will hold,
                                in reserved memory, all client data read,
                                written, and checked by the ACPI S3 Boot Script
                                opcodes produced by Callback().

  @retval RETURN_UNSUPPORTED       The library instance does not support this
                                   function.

  @retval RETURN_NOT_FOUND         The fw_cfg DMA interface to QEMU is
                                   unavailable.

  @retval RETURN_BAD_BUFFER_SIZE   ScratchBufferSize is too large.

  @retval RETURN_OUT_OF_RESOURCES  Memory allocation failed.

  @retval RETURN_SUCCESS           Callback() has been installed, and the
                                   ownership of Context has been transferred.
                                   Reserved memory has been allocated for the
                                   scratch buffer.

                                   A successful invocation of
                                   QemuFwCfgS3CallWhenBootScriptReady() cannot
                                   be rolled back.

  @return                          Error codes from underlying functions.
**/
RETURN_STATUS
EFIAPI
QemuFwCfgS3CallWhenBootScriptReady (
  IN     FW_CFG_BOOT_SCRIPT_CALLBACK_FUNCTION  *Callback,
  IN OUT VOID                                  *Context           OPTIONAL,
  IN     UINTN                                 ScratchBufferSize
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  //
  // Basic fw_cfg is certainly available, as we can only be here after a
  // successful call to QemuFwCfgS3Enabled(). Check fw_cfg DMA availability.
  //
  ASSERT (QemuFwCfgIsAvailable ());
  QemuFwCfgSelectItem (QemuFwCfgItemInterfaceVersion);
  if ((QemuFwCfgRead32 () & FW_CFG_F_DMA) == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: fw_cfg DMA unavailable\n",
      gEfiCallerBaseName,
      __func__
      ));
    return RETURN_NOT_FOUND;
  }

  //
  // Allocate a reserved buffer for the DMA access control structure and the
  // client data together.
  //
  if (ScratchBufferSize >
      MAX_UINT32 - (RESERVED_MEM_ALIGNMENT - 1) - sizeof *mDmaAccess)
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: ScratchBufferSize too big: %Lu\n",
      gEfiCallerBaseName,
      __func__,
      (UINT64)ScratchBufferSize
      ));
    return RETURN_BAD_BUFFER_SIZE;
  }

  mDmaAccess = AllocateReservedPool (
                 (RESERVED_MEM_ALIGNMENT - 1) +
                 sizeof *mDmaAccess + ScratchBufferSize
                 );
  if (mDmaAccess == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: AllocateReservedPool(): out of resources\n",
      gEfiCallerBaseName,
      __func__
      ));
    return RETURN_OUT_OF_RESOURCES;
  }

  mDmaAccess = ALIGN_POINTER (mDmaAccess, RESERVED_MEM_ALIGNMENT);

  //
  // Set up a protocol notify for EFI_S3_SAVE_STATE_PROTOCOL. Forward the
  // client's Context to the callback.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  S3SaveStateInstalledNotify,
                  Context,
                  &mS3SaveStateInstalledEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: CreateEvent(): %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    goto FreeDmaAccess;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEfiS3SaveStateProtocolGuid,
                  mS3SaveStateInstalledEvent,
                  &Registration
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: RegisterProtocolNotify(): %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    goto CloseEvent;
  }

  //
  // Set the remaining global variables. For the alignment guarantee on
  // mScratchBuffer, we rely on the fact that *mDmaAccess has a size that is an
  // integral multiple of RESERVED_MEM_ALIGNMENT.
  //
  ASSERT (sizeof *mDmaAccess % RESERVED_MEM_ALIGNMENT == 0);
  mScratchBuffer     = mDmaAccess + 1;
  mScratchBufferSize = ScratchBufferSize;
  mCallback          = Callback;

  //
  // Kick the event; EFI_S3_SAVE_STATE_PROTOCOL could be available already.
  //
  Status = gBS->SignalEvent (mS3SaveStateInstalledEvent);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: SignalEvent(): %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    goto NullGlobals;
  }

  return RETURN_SUCCESS;

NullGlobals:
  mScratchBuffer     = NULL;
  mScratchBufferSize = 0;
  mCallback          = NULL;

CloseEvent:
  gBS->CloseEvent (mS3SaveStateInstalledEvent);
  mS3SaveStateInstalledEvent = NULL;

FreeDmaAccess:
  FreePool (mDmaAccess);
  mDmaAccess = NULL;

  return (RETURN_STATUS)Status;
}

/**
  Produce ACPI S3 Boot Script opcodes that (optionally) select an fw_cfg item,
  and transfer data to it.

  The opcodes produced by QemuFwCfgS3ScriptWriteBytes() will first restore
  NumberOfBytes bytes in ScratchBuffer in-place, in reserved memory, then write
  them to fw_cfg using DMA.

  If the operation fails during S3 resume, the boot script will hang.

  This function may only be called from the client module's
  FW_CFG_BOOT_SCRIPT_CALLBACK_FUNCTION, which was passed to
  QemuFwCfgS3CallWhenBootScriptReady() as Callback.

  @param[in] FirmwareConfigItem  The UINT16 selector key of the firmware config
                                 item to write, expressed as INT32. If
                                 FirmwareConfigItem is -1, no selection is
                                 made, the write will occur to the currently
                                 selected item, at its currently selected
                                 offset. Otherwise, the specified item will be
                                 selected, and the write will occur at offset
                                 0.

  @param[in] NumberOfBytes       Size of the data to restore in ScratchBuffer,
                                 and to write from ScratchBuffer, during S3
                                 resume. NumberOfBytes must not exceed
                                 ScratchBufferSize, which was passed to
                                 QemuFwCfgS3CallWhenBootScriptReady().

  @retval RETURN_SUCCESS            The opcodes were appended to the ACPI S3
                                    Boot Script successfully. There is no way
                                    to undo this action.

  @retval RETURN_INVALID_PARAMETER  FirmwareConfigItem is invalid.

  @retval RETURN_BAD_BUFFER_SIZE    NumberOfBytes is larger than
                                    ScratchBufferSize.

  @return                           Error codes from underlying functions.
**/
RETURN_STATUS
EFIAPI
QemuFwCfgS3ScriptWriteBytes (
  IN INT32  FirmwareConfigItem,
  IN UINTN  NumberOfBytes
  )
{
  UINTN       Count;
  EFI_STATUS  Status;
  UINT64      AccessAddress;
  UINT32      ControlPollData;
  UINT32      ControlPollMask;

  ASSERT (mDmaAccess != NULL);
  ASSERT (mS3SaveState != NULL);

  if ((FirmwareConfigItem < -1) || (FirmwareConfigItem > MAX_UINT16)) {
    return RETURN_INVALID_PARAMETER;
  }

  if (NumberOfBytes > mScratchBufferSize) {
    return RETURN_BAD_BUFFER_SIZE;
  }

  //
  // Set up a write[+select] fw_cfg DMA command.
  //
  mDmaAccess->Control = FW_CFG_DMA_CTL_WRITE;
  if (FirmwareConfigItem != -1) {
    mDmaAccess->Control |= FW_CFG_DMA_CTL_SELECT;
    mDmaAccess->Control |= (UINT32)FirmwareConfigItem << 16;
  }

  mDmaAccess->Control = SwapBytes32 (mDmaAccess->Control);

  //
  // We ensured the following constraint via mScratchBufferSize in
  // QemuFwCfgS3CallWhenBootScriptReady().
  //
  ASSERT (NumberOfBytes <= MAX_UINT32);
  mDmaAccess->Length = SwapBytes32 ((UINT32)NumberOfBytes);

  mDmaAccess->Address = SwapBytes64 ((UINTN)mScratchBuffer);

  //
  // Copy mDmaAccess and NumberOfBytes bytes from mScratchBuffer into the boot
  // script. When executed at S3 resume, this opcode will restore all of them
  // in-place.
  //
  Count  = (UINTN)mScratchBuffer + NumberOfBytes - (UINTN)mDmaAccess;
  Status = mS3SaveState->Write (
                           mS3SaveState,                     // This
                           EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE, // OpCode
                           EfiBootScriptWidthUint8,          // Width
                           (UINT64)(UINTN)mDmaAccess,        // Address
                           Count,                            // Count
                           (VOID *)mDmaAccess                // Buffer
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE: %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    return (RETURN_STATUS)Status;
  }

  //
  // Append an opcode that will write the address of the fw_cfg DMA command to
  // the fw_cfg DMA address register, which consists of two 32-bit IO ports.
  // The second (highest address, least significant) write will start the
  // transfer.
  //
  AccessAddress = SwapBytes64 ((UINTN)mDmaAccess);
  Status        = mS3SaveState->Write (
                                  mS3SaveState,                    // This
                                  EFI_BOOT_SCRIPT_IO_WRITE_OPCODE, // OpCode
                                  EfiBootScriptWidthUint32,        // Width
                                  (UINT64)FW_CFG_IO_DMA_ADDRESS,   // Address
                                  (UINTN)2,                        // Count
                                  (VOID *)&AccessAddress           // Buffer
                                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: EFI_BOOT_SCRIPT_IO_WRITE_OPCODE: %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    return (RETURN_STATUS)Status;
  }

  //
  // The following opcode will wait until the Control word reads as zero
  // (transfer complete). As timeout we use MAX_UINT64 * 100ns, which is
  // approximately 58494 years.
  //
  ControlPollData = 0;
  ControlPollMask = MAX_UINT32;
  Status          = mS3SaveState->Write (
                                    mS3SaveState,                        // This
                                    EFI_BOOT_SCRIPT_MEM_POLL_OPCODE,     // OpCode
                                    EfiBootScriptWidthUint32,            // Width
                                    (UINT64)(UINTN)&mDmaAccess->Control, // Address
                                    (VOID *)&ControlPollData,            // Data
                                    (VOID *)&ControlPollMask,            // DataMask
                                    MAX_UINT64                           // Delay
                                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: EFI_BOOT_SCRIPT_MEM_POLL_OPCODE: %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    return (RETURN_STATUS)Status;
  }

  return RETURN_SUCCESS;
}

/**
  Produce ACPI S3 Boot Script opcodes that (optionally) select an fw_cfg item,
  and transfer data from it.

  The opcodes produced by QemuFwCfgS3ScriptReadBytes() will read NumberOfBytes
  bytes from fw_cfg using DMA, storing the result in ScratchBuffer, in reserved
  memory.

  If the operation fails during S3 resume, the boot script will hang.

  This function may only be called from the client module's
  FW_CFG_BOOT_SCRIPT_CALLBACK_FUNCTION, which was passed to
  QemuFwCfgS3CallWhenBootScriptReady() as Callback.

  @param[in] FirmwareConfigItem  The UINT16 selector key of the firmware config
                                 item to read, expressed as INT32. If
                                 FirmwareConfigItem is -1, no selection is
                                 made, the read will occur from the currently
                                 selected item, from its currently selected
                                 offset. Otherwise, the specified item will be
                                 selected, and the read will occur from offset
                                 0.

  @param[in] NumberOfBytes       Size of the data to read during S3 resume.
                                 NumberOfBytes must not exceed
                                 ScratchBufferSize, which was passed to
                                 QemuFwCfgS3CallWhenBootScriptReady().

  @retval RETURN_SUCCESS            The opcodes were appended to the ACPI S3
                                    Boot Script successfully. There is no way
                                    to undo this action.

  @retval RETURN_INVALID_PARAMETER  FirmwareConfigItem is invalid.

  @retval RETURN_BAD_BUFFER_SIZE    NumberOfBytes is larger than
                                    ScratchBufferSize.

  @return                           Error codes from underlying functions.
**/
RETURN_STATUS
EFIAPI
QemuFwCfgS3ScriptReadBytes (
  IN INT32  FirmwareConfigItem,
  IN UINTN  NumberOfBytes
  )
{
  EFI_STATUS  Status;
  UINT64      AccessAddress;
  UINT32      ControlPollData;
  UINT32      ControlPollMask;

  ASSERT (mDmaAccess != NULL);
  ASSERT (mS3SaveState != NULL);

  if ((FirmwareConfigItem < -1) || (FirmwareConfigItem > MAX_UINT16)) {
    return RETURN_INVALID_PARAMETER;
  }

  if (NumberOfBytes > mScratchBufferSize) {
    return RETURN_BAD_BUFFER_SIZE;
  }

  //
  // Set up a read[+select] fw_cfg DMA command.
  //
  mDmaAccess->Control = FW_CFG_DMA_CTL_READ;
  if (FirmwareConfigItem != -1) {
    mDmaAccess->Control |= FW_CFG_DMA_CTL_SELECT;
    mDmaAccess->Control |= (UINT32)FirmwareConfigItem << 16;
  }

  mDmaAccess->Control = SwapBytes32 (mDmaAccess->Control);

  //
  // We ensured the following constraint via mScratchBufferSize in
  // QemuFwCfgS3CallWhenBootScriptReady().
  //
  ASSERT (NumberOfBytes <= MAX_UINT32);
  mDmaAccess->Length = SwapBytes32 ((UINT32)NumberOfBytes);

  mDmaAccess->Address = SwapBytes64 ((UINTN)mScratchBuffer);

  //
  // Copy mDmaAccess into the boot script. When executed at S3 resume, this
  // opcode will restore it in-place.
  //
  Status = mS3SaveState->Write (
                           mS3SaveState,                     // This
                           EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE, // OpCode
                           EfiBootScriptWidthUint8,          // Width
                           (UINT64)(UINTN)mDmaAccess,        // Address
                           sizeof *mDmaAccess,               // Count
                           (VOID *)mDmaAccess                // Buffer
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE: %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    return (RETURN_STATUS)Status;
  }

  //
  // Append an opcode that will write the address of the fw_cfg DMA command to
  // the fw_cfg DMA address register, which consists of two 32-bit IO ports.
  // The second (highest address, least significant) write will start the
  // transfer.
  //
  AccessAddress = SwapBytes64 ((UINTN)mDmaAccess);
  Status        = mS3SaveState->Write (
                                  mS3SaveState,                    // This
                                  EFI_BOOT_SCRIPT_IO_WRITE_OPCODE, // OpCode
                                  EfiBootScriptWidthUint32,        // Width
                                  (UINT64)FW_CFG_IO_DMA_ADDRESS,   // Address
                                  (UINTN)2,                        // Count
                                  (VOID *)&AccessAddress           // Buffer
                                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: EFI_BOOT_SCRIPT_IO_WRITE_OPCODE: %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    return (RETURN_STATUS)Status;
  }

  //
  // The following opcode will wait until the Control word reads as zero
  // (transfer complete). As timeout we use MAX_UINT64 * 100ns, which is
  // approximately 58494 years.
  //
  ControlPollData = 0;
  ControlPollMask = MAX_UINT32;
  Status          = mS3SaveState->Write (
                                    mS3SaveState,                        // This
                                    EFI_BOOT_SCRIPT_MEM_POLL_OPCODE,     // OpCode
                                    EfiBootScriptWidthUint32,            // Width
                                    (UINT64)(UINTN)&mDmaAccess->Control, // Address
                                    (VOID *)&ControlPollData,            // Data
                                    (VOID *)&ControlPollMask,            // DataMask
                                    MAX_UINT64                           // Delay
                                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: EFI_BOOT_SCRIPT_MEM_POLL_OPCODE: %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    return (RETURN_STATUS)Status;
  }

  return RETURN_SUCCESS;
}

/**
  Produce ACPI S3 Boot Script opcodes that (optionally) select an fw_cfg item,
  and increase its offset.

  If the operation fails during S3 resume, the boot script will hang.

  This function may only be called from the client module's
  FW_CFG_BOOT_SCRIPT_CALLBACK_FUNCTION, which was passed to
  QemuFwCfgS3CallWhenBootScriptReady() as Callback.

  @param[in] FirmwareConfigItem  The UINT16 selector key of the firmware config
                                 item to advance the offset of, expressed as
                                 INT32. If FirmwareConfigItem is -1, no
                                 selection is made, and the offset for the
                                 currently selected item is increased.
                                 Otherwise, the specified item will be
                                 selected, and the offset increment will occur
                                 from offset 0.

  @param[in] NumberOfBytes       The number of bytes to skip in the subject
                                 fw_cfg item.

  @retval RETURN_SUCCESS            The opcodes were appended to the ACPI S3
                                    Boot Script successfully. There is no way
                                    to undo this action.

  @retval RETURN_INVALID_PARAMETER  FirmwareConfigItem is invalid.

  @retval RETURN_BAD_BUFFER_SIZE    NumberOfBytes is too large.

  @return                           Error codes from underlying functions.
**/
RETURN_STATUS
EFIAPI
QemuFwCfgS3ScriptSkipBytes (
  IN INT32  FirmwareConfigItem,
  IN UINTN  NumberOfBytes
  )
{
  EFI_STATUS  Status;
  UINT64      AccessAddress;
  UINT32      ControlPollData;
  UINT32      ControlPollMask;

  ASSERT (mDmaAccess != NULL);
  ASSERT (mS3SaveState != NULL);

  if ((FirmwareConfigItem < -1) || (FirmwareConfigItem > MAX_UINT16)) {
    return RETURN_INVALID_PARAMETER;
  }

  if (NumberOfBytes > MAX_UINT32) {
    return RETURN_BAD_BUFFER_SIZE;
  }

  //
  // Set up a skip[+select] fw_cfg DMA command.
  //
  mDmaAccess->Control = FW_CFG_DMA_CTL_SKIP;
  if (FirmwareConfigItem != -1) {
    mDmaAccess->Control |= FW_CFG_DMA_CTL_SELECT;
    mDmaAccess->Control |= (UINT32)FirmwareConfigItem << 16;
  }

  mDmaAccess->Control = SwapBytes32 (mDmaAccess->Control);

  mDmaAccess->Length  = SwapBytes32 ((UINT32)NumberOfBytes);
  mDmaAccess->Address = 0;

  //
  // Copy mDmaAccess into the boot script. When executed at S3 resume, this
  // opcode will restore it in-place.
  //
  Status = mS3SaveState->Write (
                           mS3SaveState,                     // This
                           EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE, // OpCode
                           EfiBootScriptWidthUint8,          // Width
                           (UINT64)(UINTN)mDmaAccess,        // Address
                           sizeof *mDmaAccess,               // Count
                           (VOID *)mDmaAccess                // Buffer
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE: %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    return (RETURN_STATUS)Status;
  }

  //
  // Append an opcode that will write the address of the fw_cfg DMA command to
  // the fw_cfg DMA address register, which consists of two 32-bit IO ports.
  // The second (highest address, least significant) write will start the
  // transfer.
  //
  AccessAddress = SwapBytes64 ((UINTN)mDmaAccess);
  Status        = mS3SaveState->Write (
                                  mS3SaveState,                    // This
                                  EFI_BOOT_SCRIPT_IO_WRITE_OPCODE, // OpCode
                                  EfiBootScriptWidthUint32,        // Width
                                  (UINT64)FW_CFG_IO_DMA_ADDRESS,   // Address
                                  (UINTN)2,                        // Count
                                  (VOID *)&AccessAddress           // Buffer
                                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: EFI_BOOT_SCRIPT_IO_WRITE_OPCODE: %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    return (RETURN_STATUS)Status;
  }

  //
  // The following opcode will wait until the Control word reads as zero
  // (transfer complete). As timeout we use MAX_UINT64 * 100ns, which is
  // approximately 58494 years.
  //
  ControlPollData = 0;
  ControlPollMask = MAX_UINT32;
  Status          = mS3SaveState->Write (
                                    mS3SaveState,                        // This
                                    EFI_BOOT_SCRIPT_MEM_POLL_OPCODE,     // OpCode
                                    EfiBootScriptWidthUint32,            // Width
                                    (UINT64)(UINTN)&mDmaAccess->Control, // Address
                                    (VOID *)&ControlPollData,            // Data
                                    (VOID *)&ControlPollMask,            // DataMask
                                    MAX_UINT64                           // Delay
                                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: EFI_BOOT_SCRIPT_MEM_POLL_OPCODE: %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    return (RETURN_STATUS)Status;
  }

  return RETURN_SUCCESS;
}

/**
  Produce ACPI S3 Boot Script opcodes that check a value in ScratchBuffer.

  If the check fails during S3 resume, the boot script will hang.

  This function may only be called from the client module's
  FW_CFG_BOOT_SCRIPT_CALLBACK_FUNCTION, which was passed to
  QemuFwCfgS3CallWhenBootScriptReady() as Callback.

  @param[in] ScratchData  Pointer to the UINT8, UINT16, UINT32 or UINT64 field
                          in ScratchBuffer that should be checked. The caller
                          is responsible for populating the field during S3
                          resume, by calling QemuFwCfgS3ScriptReadBytes() ahead
                          of QemuFwCfgS3ScriptCheckValue().

                          ScratchData must point into ScratchBuffer, which was
                          allocated, and passed to Callback(), by
                          QemuFwCfgS3CallWhenBootScriptReady().

                          ScratchData must be aligned at ValueSize bytes.

  @param[in] ValueSize    One of 1, 2, 4 or 8, specifying the size of the field
                          to check.

  @param[in] ValueMask    The value read from ScratchData is binarily AND-ed
                          with ValueMask, and the result is compared against
                          Value. If the masked data equals Value, the check
                          passes, and the boot script can proceed. Otherwise,
                          the check fails, and the boot script hangs.

  @param[in] Value        Refer to ValueMask.

  @retval RETURN_SUCCESS            The opcodes were appended to the ACPI S3
                                    Boot Script successfully. There is no way
                                    to undo this action.

  @retval RETURN_INVALID_PARAMETER  ValueSize is invalid.

  @retval RETURN_INVALID_PARAMETER  ValueMask or Value cannot be represented in
                                    ValueSize bytes.

  @retval RETURN_INVALID_PARAMETER  ScratchData is not aligned at ValueSize
                                    bytes.

  @retval RETURN_BAD_BUFFER_SIZE    The ValueSize bytes at ScratchData aren't
                                    wholly contained in the ScratchBufferSize
                                    bytes at ScratchBuffer.

  @return                           Error codes from underlying functions.
**/
RETURN_STATUS
EFIAPI
QemuFwCfgS3ScriptCheckValue (
  IN VOID    *ScratchData,
  IN UINT8   ValueSize,
  IN UINT64  ValueMask,
  IN UINT64  Value
  )
{
  EFI_BOOT_SCRIPT_WIDTH  Width;
  EFI_STATUS             Status;

  ASSERT (mS3SaveState != NULL);

  switch (ValueSize) {
    case 1:
      Width = EfiBootScriptWidthUint8;
      break;

    case 2:
      Width = EfiBootScriptWidthUint16;
      break;

    case 4:
      Width = EfiBootScriptWidthUint32;
      break;

    case 8:
      Width = EfiBootScriptWidthUint64;
      break;

    default:
      return RETURN_INVALID_PARAMETER;
  }

  if ((ValueSize < 8) &&
      ((RShiftU64 (ValueMask, ValueSize * 8) > 0) ||
       (RShiftU64 (Value, ValueSize * 8) > 0)))
  {
    return RETURN_INVALID_PARAMETER;
  }

  if ((UINTN)ScratchData % ValueSize > 0) {
    return RETURN_INVALID_PARAMETER;
  }

  if (((UINTN)ScratchData < (UINTN)mScratchBuffer) ||
      ((UINTN)ScratchData > MAX_UINTN - ValueSize) ||
      ((UINTN)ScratchData + ValueSize >
       (UINTN)mScratchBuffer + mScratchBufferSize))
  {
    return RETURN_BAD_BUFFER_SIZE;
  }

  //
  // The following opcode will wait "until" (*ScratchData & ValueMask) reads as
  // Value, considering the least significant ValueSize bytes. As timeout we
  // use MAX_UINT64 * 100ns, which is approximately 58494 years.
  //
  Status = mS3SaveState->Write (
                           mS3SaveState,                    // This
                           EFI_BOOT_SCRIPT_MEM_POLL_OPCODE, // OpCode
                           Width,                           // Width
                           (UINT64)(UINTN)ScratchData,      // Address
                           (VOID *)&Value,                  // Data
                           (VOID *)&ValueMask,              // DataMask
                           MAX_UINT64                       // Delay
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: EFI_BOOT_SCRIPT_MEM_POLL_OPCODE: %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    return (RETURN_STATUS)Status;
  }

  return RETURN_SUCCESS;
}
