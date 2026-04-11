## @file
# Standardized Error Handling infrastructures.
#
# Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

FILE_OPEN_FAILURE = 1
FILE_WRITE_FAILURE = 2
FILE_PARSE_FAILURE = 3
FILE_READ_FAILURE = 4
FILE_CREATE_FAILURE = 5
FILE_CHECKSUM_FAILURE = 6
FILE_COMPRESS_FAILURE = 7
FILE_DECOMPRESS_FAILURE = 8
FILE_MOVE_FAILURE = 9
FILE_DELETE_FAILURE = 10
FILE_COPY_FAILURE = 11
FILE_POSITIONING_FAILURE = 12
FILE_ALREADY_EXIST = 13
FILE_NOT_FOUND = 14
FILE_TYPE_MISMATCH = 15
FILE_CASE_MISMATCH = 16
FILE_DUPLICATED = 17
FILE_UNKNOWN_ERROR = 0x0FFF

OPTION_UNKNOWN = 0x1000
OPTION_MISSING = 0x1001
OPTION_CONFLICT = 0x1002
OPTION_VALUE_INVALID = 0x1003
OPTION_DEPRECATED = 0x1004
OPTION_NOT_SUPPORTED = 0x1005
OPTION_UNKNOWN_ERROR = 0x1FFF

PARAMETER_INVALID = 0x2000
PARAMETER_MISSING = 0x2001
PARAMETER_UNKNOWN_ERROR =0x2FFF

FORMAT_INVALID = 0x3000
FORMAT_NOT_SUPPORTED = 0x3001
FORMAT_UNKNOWN = 0x3002
FORMAT_UNKNOWN_ERROR = 0x3FFF

RESOURCE_NOT_AVAILABLE = 0x4000
RESOURCE_ALLOCATE_FAILURE = 0x4001
RESOURCE_FULL = 0x4002
RESOURCE_OVERFLOW = 0x4003
RESOURCE_UNDERRUN = 0x4004
RESOURCE_UNKNOWN_ERROR = 0x4FFF

ATTRIBUTE_NOT_AVAILABLE = 0x5000
ATTRIBUTE_GET_FAILURE = 0x5001
ATTRIBUTE_SET_FAILURE = 0x5002
ATTRIBUTE_UPDATE_FAILURE = 0x5003
ATTRIBUTE_ACCESS_DENIED = 0x5004
ATTRIBUTE_UNKNOWN_ERROR = 0x5FFF

IO_NOT_READY = 0x6000
IO_BUSY = 0x6001
IO_TIMEOUT = 0x6002
IO_UNKNOWN_ERROR = 0x6FFF

COMMAND_FAILURE = 0x7000

PERMISSION_FAILURE = 0x8000

FV_FREESIZE_ERROR = 0x9000

CODE_ERROR = 0xC0DE

AUTOGEN_ERROR = 0xF000
PARSER_ERROR = 0xF001
BUILD_ERROR = 0xF002
GENFDS_ERROR = 0xF003
ECC_ERROR = 0xF004
EOT_ERROR = 0xF005
PREBUILD_ERROR = 0xF007
POSTBUILD_ERROR = 0xF008
DDC_ERROR = 0xF009
WARNING_AS_ERROR = 0xF006
MIGRATION_ERROR = 0xF010
PCD_VALIDATION_INFO_ERROR = 0xF011
PCD_VARIABLE_ATTRIBUTES_ERROR = 0xF012
PCD_VARIABLE_INFO_ERROR = 0xF016
PCD_VARIABLE_ATTRIBUTES_CONFLICT_ERROR = 0xF013
PCD_STRUCTURE_PCD_INVALID_FIELD_ERROR = 0xF014
PCD_STRUCTURE_PCD_ERROR = 0xF015
ERROR_STATEMENT = 0xFFFD
ABORT_ERROR = 0xFFFE
UNKNOWN_ERROR = 0xFFFF

## Error message of each error code
gErrorMessage = {
    FILE_NOT_FOUND          :   "File/directory not found in workspace",
    FILE_OPEN_FAILURE       :   "File open failure",
    FILE_WRITE_FAILURE      :   "File write failure",
    FILE_PARSE_FAILURE      :   "File parse failure",
    FILE_READ_FAILURE       :   "File read failure",
    FILE_CREATE_FAILURE     :   "File create failure",
    FILE_CHECKSUM_FAILURE   :   "Invalid checksum of file",
    FILE_COMPRESS_FAILURE   :   "File compress failure",
    FILE_DECOMPRESS_FAILURE :   "File decompress failure",
    FILE_MOVE_FAILURE       :   "File move failure",
    FILE_DELETE_FAILURE     :   "File delete failure",
    FILE_COPY_FAILURE       :   "File copy failure",
    FILE_POSITIONING_FAILURE:   "Failed to seeking position",
    FILE_ALREADY_EXIST      :   "File or directory already exists",
    FILE_TYPE_MISMATCH      :   "Incorrect file type",
    FILE_CASE_MISMATCH      :   "File name case mismatch",
    FILE_DUPLICATED         :   "Duplicated file found",
    FILE_UNKNOWN_ERROR      :   "Unknown error encountered on file",

    OPTION_UNKNOWN          :   "Unknown option",
    OPTION_MISSING          :   "Missing option",
    OPTION_CONFLICT         :   "Conflict options",
    OPTION_VALUE_INVALID    :   "Invalid value of option",
    OPTION_DEPRECATED       :   "Deprecated option",
    OPTION_NOT_SUPPORTED    :   "Unsupported option",
    OPTION_UNKNOWN_ERROR    :   "Unknown error when processing options",

    PARAMETER_INVALID       :   "Invalid parameter",
    PARAMETER_MISSING       :   "Missing parameter",
    PARAMETER_UNKNOWN_ERROR :   "Unknown error in parameters",

    FORMAT_INVALID          :   "Invalid syntax/format",
    FORMAT_NOT_SUPPORTED    :   "Not supported syntax/format",
    FORMAT_UNKNOWN          :   "Unknown format",
    FORMAT_UNKNOWN_ERROR    :   "Unknown error in syntax/format ",

    RESOURCE_NOT_AVAILABLE  :   "Not available",
    RESOURCE_ALLOCATE_FAILURE :   "Allocate failure",
    RESOURCE_FULL           :   "Full",
    RESOURCE_OVERFLOW       :   "Overflow",
    RESOURCE_UNDERRUN       :   "Underrun",
    RESOURCE_UNKNOWN_ERROR  :   "Unknown error",

    ATTRIBUTE_NOT_AVAILABLE :   "Not available",
    ATTRIBUTE_GET_FAILURE   :   "Failed to retrieve",
    ATTRIBUTE_SET_FAILURE   :   "Failed to set",
    ATTRIBUTE_UPDATE_FAILURE:   "Failed to update",
    ATTRIBUTE_ACCESS_DENIED :   "Access denied",
    ATTRIBUTE_UNKNOWN_ERROR :   "Unknown error when accessing",

    COMMAND_FAILURE         :   "Failed to execute command",

    IO_NOT_READY            :   "Not ready",
    IO_BUSY                 :   "Busy",
    IO_TIMEOUT              :   "Timeout",
    IO_UNKNOWN_ERROR        :   "Unknown error in IO operation",

    ERROR_STATEMENT         :   "!error statement",
    UNKNOWN_ERROR           :   "Unknown error",
}

## Exception indicating a fatal error
class FatalError(Exception):
    pass

if __name__ == "__main__":
    pass
