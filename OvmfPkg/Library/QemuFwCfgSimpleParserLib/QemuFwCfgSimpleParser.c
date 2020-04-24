/** @file
  Parse the contents of named fw_cfg files as simple (scalar) data types.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>

//
// Size of the longest valid UINT64 string, including the terminating NUL.
//
#define UINT64_STRING_MAX_SIZE \
  MAX (sizeof "18446744073709551615", sizeof "0xFFFFFFFFFFFFFFFF")

//
// Size of the longest valid BOOL string (see the "mTrueString" and
// "mFalseString" arrays below), including the terminating NUL.
//
#define BOOL_STRING_MAX_SIZE (sizeof "disabled")

//
// Length of "\r\n", not including the terminating NUL.
//
#define CRLF_LENGTH (sizeof "\r\n" - 1)

//
// Words recognized as representing TRUE or FALSE.
//
STATIC CONST CHAR8 * CONST mTrueString[] = {
  "true", "yes", "y", "enable", "enabled", "1"
};
STATIC CONST CHAR8 * CONST mFalseString[] = {
  "false", "no", "n", "disable", "disabled", "0"
};

//
// Helper functions.
//

/**
  Look up FileName with QemuFwCfgFindFile() from QemuFwCfgLib. Read the fw_cfg
  file into the caller-provided CHAR8 array. NUL-terminate the array.

  @param[in] FileName        The name of the fw_cfg file to look up and read.

  @param[in,out] BufferSize  On input, number of bytes available in Buffer.

                             On output, the number of bytes that have been
                             stored to Buffer.

                             On error, BufferSize is indeterminate.

  @param[out] Buffer         The buffer to read the fw_cfg file into. If the
                             fw_cfg file contents are not NUL-terminated, then
                             a NUL character is placed into Buffer after the
                             fw_cfg file contents.

                             On error, Buffer is indeterminate.

  @retval RETURN_SUCCESS         Buffer has been populated with the fw_cfg file
                                 contents. Buffer is NUL-terminated regardless
                                 of whether the fw_cfg file itself was
                                 NUL-terminated.

  @retval RETURN_UNSUPPORTED     Firmware configuration is unavailable.

  @retval RETURN_PROTOCOL_ERROR  The fw_cfg file does not fit into Buffer.
                                 (This is considered a QEMU configuration
                                 error; BufferSize is considered authoritative
                                 for the contents of the fw_cfg file identified
                                 by FileName.)

  @retval RETURN_PROTOCOL_ERROR  The fw_cfg file contents are not themselves
                                 NUL-terminated, and an extra NUL byte does not
                                 fit into Buffer. (Again a QEMU configuration
                                 error.)

  @return                        Error codes propagated from
                                 QemuFwCfgFindFile().
**/
STATIC
RETURN_STATUS
QemuFwCfgGetAsString (
  IN     CONST CHAR8 *FileName,
  IN OUT UINTN       *BufferSize,
  OUT    CHAR8       *Buffer
  )
{
  RETURN_STATUS        Status;
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;

  if (!QemuFwCfgIsAvailable ()) {
    return RETURN_UNSUPPORTED;
  }

  Status = QemuFwCfgFindFile (FileName, &FwCfgItem, &FwCfgSize);
  if (RETURN_ERROR (Status)) {
    return Status;
  }
  if (FwCfgSize > *BufferSize) {
    return RETURN_PROTOCOL_ERROR;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, Buffer);

  //
  // If Buffer is already NUL-terminated due to fw_cfg contents, we're done.
  //
  if (FwCfgSize > 0 && Buffer[FwCfgSize - 1] == '\0') {
    *BufferSize = FwCfgSize;
    return RETURN_SUCCESS;
  }
  //
  // Otherwise, append a NUL byte to Buffer (if we have room for it).
  //
  if (FwCfgSize == *BufferSize) {
    return RETURN_PROTOCOL_ERROR;
  }
  Buffer[FwCfgSize] = '\0';
  *BufferSize = FwCfgSize + 1;
  return RETURN_SUCCESS;
}

/**
  Remove a trailing \r\n or \n sequence from a string.

  @param[in,out] BufferSize  On input, the number of bytes in Buffer, including
                             the terminating NUL.

                             On output, the adjusted string size (including the
                             terminating NUL), after stripping the \r\n or \n
                             suffix.

  @param[in,out] Buffer      The NUL-terminated string to trim.
**/
STATIC
VOID
StripNewline (
  IN OUT UINTN *BufferSize,
  IN OUT CHAR8 *Buffer
  )
{
  UINTN InSize, OutSize;

  InSize = *BufferSize;
  OutSize = InSize;

  if (InSize >= 3 &&
      Buffer[InSize - 3] == '\r' && Buffer[InSize - 2] == '\n') {
    OutSize = InSize - 2;
  } else if (InSize >= 2 && Buffer[InSize - 2] == '\n') {
    OutSize = InSize - 1;
  }

  if (OutSize < InSize) {
    Buffer[OutSize - 1] = '\0';
    *BufferSize = OutSize;
  }
}

/**
  Read the fw_cfg file identified by FileName as a string into a small array
  with automatic storage duration, using QemuFwCfgGetAsString(). Parse the
  string as a UINT64. Perform a range-check on the parsed value.

  @param[in] FileName    The name of the fw_cfg file to look up and parse.

  @param[in] ParseAsHex  If TRUE, call BaseLib's AsciiStrHexToUint64S() for
                         parsing the fw_cfg file.

                         If FALSE, call BaseLib's AsciiStrDecimalToUint64S()
                         for parsing the fw_cfg file.

  @param[in] Limit       The inclusive upper bound on the parsed UINT64 value.

  @param[out] Value      On success, Value has been parsed with the BaseLib
                         function determined by ParseAsHex, and has been
                         range-checked against [0, Limit].

                         On failure, Value is not changed.

  @retval RETURN_SUCCESS         Parsing successful. Value has been set.

  @retval RETURN_UNSUPPORTED     Firmware configuration is unavailable.

  @retval RETURN_PROTOCOL_ERROR  Parsing failed. Value has not been changed.

  @retval RETURN_PROTOCOL_ERROR  Parsing succeeded, but the result does not fit
                                 in the [0, Limit] range. Value has not been
                                 changed.

  @return                        Error codes propagated from
                                 QemuFwCfgFindFile() and from the BaseLib
                                 function selected by ParseAsHex. Value has not
                                 been changed.
**/
STATIC
RETURN_STATUS
QemuFwCfgParseUint64WithLimit (
  IN  CONST CHAR8 *FileName,
  IN  BOOLEAN     ParseAsHex,
  IN  UINT64      Limit,
  OUT UINT64      *Value
  )
{
  UINTN         Uint64StringSize;
  CHAR8         Uint64String[UINT64_STRING_MAX_SIZE + CRLF_LENGTH];
  RETURN_STATUS Status;
  CHAR8         *EndPointer;
  UINT64        Uint64;

  Uint64StringSize = sizeof Uint64String;
  Status = QemuFwCfgGetAsString (FileName, &Uint64StringSize, Uint64String);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  StripNewline (&Uint64StringSize, Uint64String);

  if (ParseAsHex) {
    Status = AsciiStrHexToUint64S (Uint64String, &EndPointer, &Uint64);
  } else {
    Status = AsciiStrDecimalToUint64S (Uint64String, &EndPointer, &Uint64);
  }
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  //
  // Report a wire protocol error if the subject sequence is empty, or trailing
  // garbage is present, or Limit is not honored.
  //
  if (EndPointer == Uint64String || *EndPointer != '\0' || Uint64 > Limit) {
    return RETURN_PROTOCOL_ERROR;
  }

  *Value = Uint64;
  return RETURN_SUCCESS;
}

//
// Public functions.
//

RETURN_STATUS
EFIAPI
QemuFwCfgSimpleParserInit (
  VOID
  )
{
  //
  // Do nothing, just participate in constructor dependency ordering.
  //
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
QemuFwCfgParseBool (
  IN  CONST CHAR8 *FileName,
  OUT BOOLEAN     *Value
  )
{
  UINTN         BoolStringSize;
  CHAR8         BoolString[BOOL_STRING_MAX_SIZE + CRLF_LENGTH];
  RETURN_STATUS Status;
  UINTN         Idx;

  BoolStringSize = sizeof BoolString;
  Status = QemuFwCfgGetAsString (FileName, &BoolStringSize, BoolString);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  StripNewline (&BoolStringSize, BoolString);

  for (Idx = 0; Idx < ARRAY_SIZE (mTrueString); ++Idx) {
    if (AsciiStriCmp (BoolString, mTrueString[Idx]) == 0) {
      *Value = TRUE;
      return RETURN_SUCCESS;
    }
  }

  for (Idx = 0; Idx < ARRAY_SIZE (mFalseString); ++Idx) {
    if (AsciiStriCmp (BoolString, mFalseString[Idx]) == 0) {
      *Value = FALSE;
      return RETURN_SUCCESS;
    }
  }

  return RETURN_PROTOCOL_ERROR;
}

RETURN_STATUS
EFIAPI
QemuFwCfgParseUint8 (
  IN  CONST CHAR8 *FileName,
  IN  BOOLEAN     ParseAsHex,
  OUT UINT8       *Value
  )
{
  RETURN_STATUS Status;
  UINT64        Uint64;

  Status = QemuFwCfgParseUint64WithLimit (FileName, ParseAsHex, MAX_UINT8,
             &Uint64);
  if (RETURN_ERROR (Status)) {
    return Status;
  }
  *Value = (UINT8)Uint64;
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
QemuFwCfgParseUint16 (
  IN  CONST CHAR8 *FileName,
  IN  BOOLEAN     ParseAsHex,
  OUT UINT16      *Value
  )
{
  RETURN_STATUS Status;
  UINT64        Uint64;

  Status = QemuFwCfgParseUint64WithLimit (FileName, ParseAsHex, MAX_UINT16,
             &Uint64);
  if (RETURN_ERROR (Status)) {
    return Status;
  }
  *Value = (UINT16)Uint64;
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
QemuFwCfgParseUint32 (
  IN  CONST CHAR8 *FileName,
  IN  BOOLEAN     ParseAsHex,
  OUT UINT32      *Value
  )
{
  RETURN_STATUS Status;
  UINT64        Uint64;

  Status = QemuFwCfgParseUint64WithLimit (FileName, ParseAsHex, MAX_UINT32,
             &Uint64);
  if (RETURN_ERROR (Status)) {
    return Status;
  }
  *Value = (UINT32)Uint64;
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
QemuFwCfgParseUint64 (
  IN  CONST CHAR8 *FileName,
  IN  BOOLEAN     ParseAsHex,
  OUT UINT64      *Value
  )
{
  RETURN_STATUS Status;
  UINT64        Uint64;

  Status = QemuFwCfgParseUint64WithLimit (FileName, ParseAsHex, MAX_UINT64,
             &Uint64);
  if (RETURN_ERROR (Status)) {
    return Status;
  }
  *Value = Uint64;
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
QemuFwCfgParseUintn (
  IN  CONST CHAR8 *FileName,
  IN  BOOLEAN     ParseAsHex,
  OUT UINTN       *Value
  )
{
  RETURN_STATUS Status;
  UINT64        Uint64;

  Status = QemuFwCfgParseUint64WithLimit (FileName, ParseAsHex, MAX_UINTN,
             &Uint64);
  if (RETURN_ERROR (Status)) {
    return Status;
  }
  *Value = (UINTN)Uint64;
  return RETURN_SUCCESS;
}
