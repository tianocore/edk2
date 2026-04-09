/** @file
  Shared code for the Base Null and PEI fw_cfg instances of the QemuFwCfgS3Lib
  class.

  Copyright (C) 2017, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/QemuFwCfgS3Lib.h>

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
  ASSERT (FALSE);
  return RETURN_UNSUPPORTED;
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
  ASSERT (FALSE);
  return RETURN_UNSUPPORTED;
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
  ASSERT (FALSE);
  return RETURN_UNSUPPORTED;
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
  ASSERT (FALSE);
  return RETURN_UNSUPPORTED;
}
