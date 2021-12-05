/** @file
  S3 support for QEMU fw_cfg

  This library class enables driver modules (a) to query whether S3 support was
  enabled on the QEMU command line, (b) to produce fw_cfg DMA operations that
  are to be replayed at S3 resume time.

  Copyright (C) 2017, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __FW_CFG_S3_LIB__
#define __FW_CFG_S3_LIB__

#include <Base.h>

/**
  Determine if S3 support is explicitly enabled.

  @retval  TRUE   If S3 support is explicitly enabled. Other functions in this
                  library may be called (subject to their individual
                  restrictions).

           FALSE  Otherwise. This includes unavailability of the firmware
                  configuration interface. No other function in this library
                  must be called.
**/
BOOLEAN
EFIAPI
QemuFwCfgS3Enabled (
  VOID
  );

/**
  Prototype for the callback function that the client module provides.

  In the callback function, the client module calls the
  QemuFwCfgS3ScriptWriteBytes(), QemuFwCfgS3ScriptReadBytes(),
  QemuFwCfgS3ScriptSkipBytes(), and QemuFwCfgS3ScriptCheckValue() functions.
  Those functions produce ACPI S3 Boot Script opcodes that will perform fw_cfg
  DMA operations, and will check any desired values that were read, during S3
  resume.

  The callback function is invoked when the production of ACPI S3 Boot Script
  opcodes becomes possible. This may occur directly on the call stack of
  QemuFwCfgS3CallWhenBootScriptReady() (see below), or after
  QemuFwCfgS3CallWhenBootScriptReady() has successfully returned.

  The callback function must not return if it fails -- in the general case,
  there is noone to propagate any errors to. Therefore, on error, an error
  message should be logged, and CpuDeadLoop() must be called.

  @param[in,out] Context        Carries information from the client module
                                itself (i.e., from the invocation of
                                QemuFwCfgS3CallWhenBootScriptReady()) to the
                                callback function.

                                If Context points to dynamically allocated
                                storage, then the callback function must
                                release it.

  @param[in,out] ScratchBuffer  Points to reserved memory, allocated by
                                QemuFwCfgS3CallWhenBootScriptReady()
                                internally.

                                ScratchBuffer is typed and sized by the client
                                module when it calls
                                QemuFwCfgS3CallWhenBootScriptReady(). The
                                client module defines a union type of
                                structures for ScratchBuffer such that the
                                union can hold client data for any desired
                                fw_cfg DMA read and write operations, and value
                                checking.

                                The callback function casts ScratchBuffer to
                                the union type described above. It passes union
                                member sizes as NumberOfBytes to
                                QemuFwCfgS3ScriptReadBytes() and
                                QemuFwCfgS3ScriptWriteBytes(). It passes field
                                addresses and sizes in structures in the union
                                as ScratchData and ValueSize to
                                QemuFwCfgS3ScriptCheckValue().

                                ScratchBuffer is aligned at 8 bytes.
**/
typedef
VOID(EFIAPI FW_CFG_BOOT_SCRIPT_CALLBACK_FUNCTION)(
  IN OUT VOID *Context       OPTIONAL,
  IN OUT VOID *ScratchBuffer
  );

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
  );

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
  );

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
  );

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
  );

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
  );

#endif
