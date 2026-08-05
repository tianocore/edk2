/** @file
  Parse the contents of named fw_cfg files as simple (scalar) data types.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Base.h>

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

  @retval RETURN_PROTOCOL_ERROR  The fw_cfg file contents are not themselves
                                 NUL-terminated, and an extra NUL byte does not
                                 fit into Buffer.

  @return                        Error codes propagated from
                                 QemuFwCfgFindFile().
**/
RETURN_STATUS
EFIAPI
QemuFwCfgGetAsString (
  IN     CONST CHAR8  *FileName,
  IN OUT UINTN        *BufferSize,
  OUT    CHAR8        *Buffer
  );

/**
  Look up FileName with QemuFwCfgFindFile() from QemuFwCfgLib. Read the fw_cfg
  file into a small array with automatic storage duration. Parse the array as
  the textual representation of a BOOLEAN.

  @param[in] FileName  The name of the fw_cfg file to look up and parse.

  @param[out] Value    On success, Value is TRUE if the contents of the fw_cfg
                       file case-insensitively match "true", "yes", "y",
                       "enable", "enabled", "1".

                       On success, Value is FALSE if the contents of the fw_cfg
                       file case-insensitively match "false", "no", "n",
                       "disable", "disabled", "0".

                       On failure, Value is not changed.

  @retval RETURN_SUCCESS         Parsing successful. Value has been set.

  @retval RETURN_UNSUPPORTED     Firmware configuration is unavailable.

  @retval RETURN_PROTOCOL_ERROR  Parsing failed. Value has not been changed.

  @return                        Error codes propagated from
                                 QemuFwCfgFindFile(). Value has not been
                                 changed.
**/
RETURN_STATUS
EFIAPI
QemuFwCfgParseBool (
  IN  CONST CHAR8  *FileName,
  OUT BOOLEAN      *Value
  );

/**
  Look up FileName with QemuFwCfgFindFile() from QemuFwCfgLib. Read the fw_cfg
  file into a small array with automatic storage duration. Parse the array as
  the textual representation of a UINT8.

  @param[in] FileName    The name of the fw_cfg file to look up and parse.

  @param[in] ParseAsHex  If TRUE, call BaseLib's AsciiStrHexToUint64S() for
                         parsing the fw_cfg file.

                         If FALSE, call BaseLib's AsciiStrDecimalToUint64S()
                         for parsing the fw_cfg file.

  @param[out] Value      On success, Value has been parsed with the BaseLib
                         function determined by ParseAsHex, and also
                         range-checked for [0, MAX_UINT8].

                         On failure, Value is not changed.

  @retval RETURN_SUCCESS         Parsing successful. Value has been set.

  @retval RETURN_UNSUPPORTED     Firmware configuration is unavailable.

  @retval RETURN_PROTOCOL_ERROR  Parsing failed. Value has not been changed.

  @retval RETURN_PROTOCOL_ERROR  Parsing succeeded, but the result does not fit
                                 in the [0, MAX_UINT8] range. Value has not
                                 been changed.

  @return                        Error codes propagated from
                                 QemuFwCfgFindFile() and from the BaseLib
                                 function selected by ParseAsHex. Value has not
                                 been changed.
**/
RETURN_STATUS
EFIAPI
QemuFwCfgParseUint8 (
  IN  CONST CHAR8  *FileName,
  IN  BOOLEAN      ParseAsHex,
  OUT UINT8        *Value
  );

//
// The following functions behave identically to QemuFwCfgParseUint8(),
// only their range checks use MAX_UINT16, MAX_UINT32, MAX_UINT64, MAX_UINTN,
// respectively.
//

RETURN_STATUS
EFIAPI
QemuFwCfgParseUint16 (
  IN  CONST CHAR8  *FileName,
  IN  BOOLEAN      ParseAsHex,
  OUT UINT16       *Value
  );

RETURN_STATUS
EFIAPI
QemuFwCfgParseUint32 (
  IN  CONST CHAR8  *FileName,
  IN  BOOLEAN      ParseAsHex,
  OUT UINT32       *Value
  );

RETURN_STATUS
EFIAPI
QemuFwCfgParseUint64 (
  IN  CONST CHAR8  *FileName,
  IN  BOOLEAN      ParseAsHex,
  OUT UINT64       *Value
  );

RETURN_STATUS
EFIAPI
QemuFwCfgParseUintn (
  IN  CONST CHAR8  *FileName,
  IN  BOOLEAN      ParseAsHex,
  OUT UINTN        *Value
  );
