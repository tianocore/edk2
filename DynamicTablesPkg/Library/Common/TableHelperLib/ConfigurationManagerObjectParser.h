/** @file
  Configuration Manager Object parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CONFIGURATION_MANAGER_OBJECT_PARSER_H_
#define CONFIGURATION_MANAGER_OBJECT_PARSER_H_

#define OUTPUT_FIELD_COLUMN_WIDTH  32

/** A helper macro for populating the Reserved objects
  like EArmObjReserved, EArmObjMax, etc. in the CM_OBJ_PARSER_ARRAY.
**/
#define CM_PARSER_ADD_OBJECT_RESERVED(ObjectId) \
                  {ObjectId, #ObjectId, NULL, 0}

/** A helper macro for populating the Cm Arm objects
  in the CM_OBJ_PARSER_ARRAY.
**/
#define CM_PARSER_ADD_OBJECT(ObjectId, Parser) \
                  {ObjectId, #ObjectId, Parser, ARRAY_SIZE(Parser) }

/** Function prototype to format a field print.

  @param [in] Format  Format string for tracing the data as specified by
                      the 'Format' member of ACPI_PARSER.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the field
**/
typedef VOID (EFIAPI *FNPTR_PRINT_FORMATTER)(CONST CHAR8 *Format, UINT8 *Ptr, UINT32 Length);

/**
  The CM_OBJ_PARSER structure describes the fields of an CmObject and
  provides means for the parser to interpret and trace appropriately.

  ParseAcpi() uses the format string specified by 'Format' for tracing
  the field data.
*/
typedef struct CmObjParser CM_OBJ_PARSER;
struct CmObjParser {
  /// String describing the Cm Object
  CONST CHAR8              *NameStr;

  /// The length of the field.
  UINT32                   Length;

  /// Optional Print() style format string for tracing the data. If not
  /// used this must be set to NULL.
  CONST CHAR8              *Format;

  /// Optional pointer to a print formatter function which
  /// is typically used to trace complex field information.
  /// If not used this must be set to NULL.
  /// The Format string is passed to the PrintFormatter function
  /// but may be ignored by the implementation code.
  FNPTR_PRINT_FORMATTER    PrintFormatter;

  /// Optional pointer to print the fields of another CM_OBJ_PARSER
  /// structure. This is useful to print sub-structures.
  CONST CM_OBJ_PARSER      *SubObjParser;

  /// Count of items in the SubObj.
  UINTN                    SubObjItemCount;
};

/**
  A structure mapping an array of Configuration Manager Object parsers
  with their object names.
*/
typedef struct CmObjParserArray {
  /// Object ID
  CONST UINTN            ObjectId;

  /// Object name
  CONST CHAR8            *ObjectName;

  /// Function pointer to the parser
  CONST CM_OBJ_PARSER    *Parser;

  /// Count of items
  UINTN                  ItemCount;
} CM_OBJ_PARSER_ARRAY;

#endif // CONFIGURATION_MANAGER_OBJECT_PARSER_H_
