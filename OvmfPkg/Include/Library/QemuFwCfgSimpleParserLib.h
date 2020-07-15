/** @file
  Parse the contents of named fw_cfg files as simple (scalar) data types.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef QEMU_FW_CFG_SIMPLE_PARSER_LIB_H_
#define QEMU_FW_CFG_SIMPLE_PARSER_LIB_H_

#include <Base.h>

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
  IN  CONST CHAR8 *FileName,
  OUT BOOLEAN     *Value
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
  IN  CONST CHAR8 *FileName,
  IN  BOOLEAN     ParseAsHex,
  OUT UINT8       *Value
  );

//
// The following functions behave identically to QemuFwCfgParseUint8(),
// only their range checks use MAX_UINT16, MAX_UINT32, MAX_UINT64, MAX_UINTN,
// respectively.
//

RETURN_STATUS
EFIAPI
QemuFwCfgParseUint16 (
  IN  CONST CHAR8 *FileName,
  IN  BOOLEAN     ParseAsHex,
  OUT UINT16      *Value
  );

RETURN_STATUS
EFIAPI
QemuFwCfgParseUint32 (
  IN  CONST CHAR8 *FileName,
  IN  BOOLEAN     ParseAsHex,
  OUT UINT32      *Value
  );

RETURN_STATUS
EFIAPI
QemuFwCfgParseUint64 (
  IN  CONST CHAR8 *FileName,
  IN  BOOLEAN     ParseAsHex,
  OUT UINT64      *Value
  );

RETURN_STATUS
EFIAPI
QemuFwCfgParseUintn (
  IN  CONST CHAR8 *FileName,
  IN  BOOLEAN     ParseAsHex,
  OUT UINTN       *Value
  );

#endif // QEMU_FW_CFG_SIMPLE_PARSER_LIB_H_
