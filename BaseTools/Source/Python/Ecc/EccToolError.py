## @file
# Standardized Error Handling infrastructures.
#
# Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

ERROR_GENERAL_CHECK_ALL = 1000
ERROR_GENERAL_CHECK_NO_TAB = 1001
ERROR_GENERAL_CHECK_INDENTATION = 1002
ERROR_GENERAL_CHECK_LINE = 1003
ERROR_GENERAL_CHECK_NO_ASM = 1004
ERROR_GENERAL_CHECK_NO_PROGMA = 1005
ERROR_GENERAL_CHECK_CARRIAGE_RETURN = 1006
ERROR_GENERAL_CHECK_FILE_EXISTENCE = 1007
ERROR_GENERAL_CHECK_NON_ACSII = 1008
ERROR_GENERAL_CHECK_UNI = 1009
ERROR_GENERAL_CHECK_UNI_HELP_INFO = 1010
ERROR_GENERAL_CHECK_INVALID_LINE_ENDING = 1011
ERROR_GENERAL_CHECK_TRAILING_WHITE_SPACE_LINE = 1012

ERROR_SPACE_CHECK_ALL = 2000

ERROR_PREDICATE_EXPRESSION_CHECK_ALL = 3000
ERROR_PREDICATE_EXPRESSION_CHECK_BOOLEAN_VALUE = 3001
ERROR_PREDICATE_EXPRESSION_CHECK_NO_BOOLEAN_OPERATOR = 3002
ERROR_PREDICATE_EXPRESSION_CHECK_COMPARISON_NULL_TYPE = 3003

ERROR_HEADER_CHECK_ALL = 4000
ERROR_HEADER_CHECK_FILE = 4001
ERROR_HEADER_CHECK_FUNCTION = 4002

ERROR_C_FUNCTION_LAYOUT_CHECK_ALL = 5000
ERROR_C_FUNCTION_LAYOUT_CHECK_RETURN_TYPE = 5001
ERROR_C_FUNCTION_LAYOUT_CHECK_OPTIONAL_FUNCTIONAL_MODIFIER = 5002
ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME = 5003
ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE = 5004
ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_BODY = 5005
ERROR_C_FUNCTION_LAYOUT_CHECK_DATA_DECLARATION = 5006
ERROR_C_FUNCTION_LAYOUT_CHECK_NO_INIT_OF_VARIABLE = 5007
ERROR_C_FUNCTION_LAYOUT_CHECK_NO_STATIC = 5008
ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_2 = 5009
ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_3 = 5010

ERROR_INCLUDE_FILE_CHECK_ALL = 6000
ERROR_INCLUDE_FILE_CHECK_IFNDEF_STATEMENT_1 = 6001
ERROR_INCLUDE_FILE_CHECK_IFNDEF_STATEMENT_2 = 6002
ERROR_INCLUDE_FILE_CHECK_IFNDEF_STATEMENT_3 = 6003
ERROR_INCLUDE_FILE_CHECK_DATA = 6004
ERROR_INCLUDE_FILE_CHECK_NAME = 6005

ERROR_DECLARATION_DATA_TYPE_CHECK_ALL = 7000
ERROR_DECLARATION_DATA_TYPE_CHECK_NO_USE_C_TYPE = 7001
ERROR_DECLARATION_DATA_TYPE_CHECK_IN_OUT_MODIFIER = 7002
ERROR_DECLARATION_DATA_TYPE_CHECK_EFI_API_MODIFIER = 7003
ERROR_DECLARATION_DATA_TYPE_CHECK_ENUMERATED_TYPE = 7004
ERROR_DECLARATION_DATA_TYPE_CHECK_STRUCTURE_DECLARATION = 7005
ERROR_DECLARATION_DATA_TYPE_CHECK_SAME_STRUCTURE = 7007
ERROR_DECLARATION_DATA_TYPE_CHECK_UNION_TYPE = 7006
ERROR_DECLARATION_DATA_TYPE_CHECK_NESTED_STRUCTURE = 7008

ERROR_NAMING_CONVENTION_CHECK_ALL = 8000
ERROR_NAMING_CONVENTION_CHECK_DEFINE_STATEMENT = 8001
ERROR_NAMING_CONVENTION_CHECK_TYPEDEF_STATEMENT = 8002
ERROR_NAMING_CONVENTION_CHECK_IFNDEF_STATEMENT = 8003
ERROR_NAMING_CONVENTION_CHECK_PATH_NAME = 8004
ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME = 8005
ERROR_NAMING_CONVENTION_CHECK_FUNCTION_NAME = 8006
ERROR_NAMING_CONVENTION_CHECK_SINGLE_CHARACTER_VARIABLE = 8007

ERROR_DOXYGEN_CHECK_ALL = 9000
ERROR_DOXYGEN_CHECK_FILE_HEADER = 9001
ERROR_DOXYGEN_CHECK_FUNCTION_HEADER = 9002
ERROR_DOXYGEN_CHECK_COMMENT_DESCRIPTION = 9003
ERROR_DOXYGEN_CHECK_COMMENT_FORMAT = 9004
ERROR_DOXYGEN_CHECK_COMMAND = 9005

ERROR_META_DATA_FILE_CHECK_ALL = 10000
ERROR_META_DATA_FILE_CHECK_PATH_NAME = 10001
ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_1 = 10002
ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_2 = 10003
ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_DEPENDENT = 10004
ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_ORDER = 10005
ERROR_META_DATA_FILE_CHECK_LIBRARY_NO_USE = 10006
ERROR_META_DATA_FILE_CHECK_BINARY_INF_IN_FDF = 10007
ERROR_META_DATA_FILE_CHECK_PCD_DUPLICATE = 10008
ERROR_META_DATA_FILE_CHECK_PCD_FLASH = 10009
ERROR_META_DATA_FILE_CHECK_PCD_NO_USE = 10010
ERROR_META_DATA_FILE_CHECK_DUPLICATE_GUID = 10011
ERROR_META_DATA_FILE_CHECK_DUPLICATE_PROTOCOL = 10012
ERROR_META_DATA_FILE_CHECK_DUPLICATE_PPI = 10013
ERROR_META_DATA_FILE_CHECK_MODULE_FILE_NO_USE = 10014
ERROR_META_DATA_FILE_CHECK_PCD_TYPE = 10015
ERROR_META_DATA_FILE_CHECK_MODULE_FILE_GUID_DUPLICATION = 10016
ERROR_META_DATA_FILE_CHECK_LIBRARY_NAME_DUPLICATE = 10017
ERROR_META_DATA_FILE_CHECK_FORMAT_GUID = 10018
ERROR_META_DATA_FILE_CHECK_FORMAT_PROTOCOL = 10019
ERROR_META_DATA_FILE_CHECK_FORMAT_PPI = 10020
ERROR_META_DATA_FILE_CHECK_FORMAT_PCD = 10021
ERROR_META_DATA_FILE_CHECK_LIBRARY_NOT_DEFINED = 10022

ERROR_SPELLING_CHECK_ALL = 11000

ERROR_SMM_COMM_PARA_CHECK_BUFFER_TYPE = 12001

gEccErrorMessage = {
    ERROR_GENERAL_CHECK_ALL : "",
    ERROR_GENERAL_CHECK_NO_TAB : "'TAB' character is not allowed in source code, please replace each 'TAB' with two spaces.",
    ERROR_GENERAL_CHECK_INDENTATION : "Indentation does not follow coding style",
    ERROR_GENERAL_CHECK_LINE : "The width of each line does not follow coding style",
    ERROR_GENERAL_CHECK_NO_ASM : "There should be no use of _asm in the source file",
    ERROR_GENERAL_CHECK_NO_PROGMA : """There should be no use of "#progma" in source file except "#pragma pack(#)\"""",
    ERROR_GENERAL_CHECK_CARRIAGE_RETURN : "There should be a carriage return at the end of the file",
    ERROR_GENERAL_CHECK_FILE_EXISTENCE : "File not found",
    ERROR_GENERAL_CHECK_NON_ACSII : "File has invalid Non-ACSII char",
    ERROR_GENERAL_CHECK_UNI : "File is not a valid UTF-16 UNI file",
    ERROR_GENERAL_CHECK_UNI_HELP_INFO : "UNI file that is associated by INF or DEC file need define the prompt and help information.",
    ERROR_GENERAL_CHECK_INVALID_LINE_ENDING : "Only CRLF (Carriage Return Line Feed) is allowed to line ending.",
    ERROR_GENERAL_CHECK_TRAILING_WHITE_SPACE_LINE : "There should be no trailing white space in one line.",

    ERROR_SPACE_CHECK_ALL : "",

    ERROR_PREDICATE_EXPRESSION_CHECK_ALL : "",
    ERROR_PREDICATE_EXPRESSION_CHECK_BOOLEAN_VALUE : "Boolean values and variable type BOOLEAN should not use explicit comparisons to TRUE or FALSE",
    ERROR_PREDICATE_EXPRESSION_CHECK_NO_BOOLEAN_OPERATOR : "Non-Boolean comparisons should use a compare operator (==, !=, >, < >=, <=)",
    ERROR_PREDICATE_EXPRESSION_CHECK_COMPARISON_NULL_TYPE : "A comparison of any pointer to zero must be done via the NULL type",

    ERROR_HEADER_CHECK_ALL : "",
    ERROR_HEADER_CHECK_FILE : "File header doesn't exist",
    ERROR_HEADER_CHECK_FUNCTION : "Function header doesn't exist",

    ERROR_C_FUNCTION_LAYOUT_CHECK_ALL : "",
    ERROR_C_FUNCTION_LAYOUT_CHECK_RETURN_TYPE : "Return type of a function should exist and in the first line",
    ERROR_C_FUNCTION_LAYOUT_CHECK_OPTIONAL_FUNCTIONAL_MODIFIER : "Any optional functional modifiers should exist and next to the return type",
    ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME : """Function name should be left justified, followed by the beginning of the parameter list, with the closing parenthesis on its own line, indented two spaces""",
    ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE : "Function prototypes in include files have the same form as function definitions",
    ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_2 : "Function prototypes in include files have different parameter number with function definitions",
    ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_3 : "Function prototypes in include files have different parameter modifier with function definitions",
    ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_BODY : "The body of a function should be contained by open and close braces that must be in the first column",
    ERROR_C_FUNCTION_LAYOUT_CHECK_DATA_DECLARATION : "The data declarations should be the first code in a module",
    ERROR_C_FUNCTION_LAYOUT_CHECK_NO_INIT_OF_VARIABLE : "There should be no initialization of a variable as part of its declaration",
    ERROR_C_FUNCTION_LAYOUT_CHECK_NO_STATIC : "There should be no use of STATIC for functions",

    ERROR_INCLUDE_FILE_CHECK_ALL : "",
    ERROR_INCLUDE_FILE_CHECK_IFNDEF_STATEMENT_1 : "All include file contents should be guarded by a #ifndef statement.",
    ERROR_INCLUDE_FILE_CHECK_IFNDEF_STATEMENT_2 : "The #ifndef must be the first line of code following the file header comment",
    ERROR_INCLUDE_FILE_CHECK_IFNDEF_STATEMENT_3 : "The #endif must appear on the last line in the file",
    ERROR_INCLUDE_FILE_CHECK_DATA : "Include files should contain only public or only private data and cannot contain code or define data variables",
    ERROR_INCLUDE_FILE_CHECK_NAME : "No permission for the include file with same names",

    ERROR_DECLARATION_DATA_TYPE_CHECK_ALL : "",
    ERROR_DECLARATION_DATA_TYPE_CHECK_NO_USE_C_TYPE : "There should be no use of int, unsigned, char, void, long in any .c, .h or .asl files",
    ERROR_DECLARATION_DATA_TYPE_CHECK_IN_OUT_MODIFIER : """The modifiers IN, OUT, OPTIONAL, and UNALIGNED should be used only to qualify arguments to a function and should not appear in a data type declaration""",
    ERROR_DECLARATION_DATA_TYPE_CHECK_EFI_API_MODIFIER : "The EFIAPI modifier should be used at the entry of drivers, events, and member functions of protocols",
    ERROR_DECLARATION_DATA_TYPE_CHECK_ENUMERATED_TYPE : "Enumerated Type should have a 'typedef' and the name must be in capital letters",
    ERROR_DECLARATION_DATA_TYPE_CHECK_STRUCTURE_DECLARATION : "Structure Type should have a 'typedef' and the name must be in capital letters",
    ERROR_DECLARATION_DATA_TYPE_CHECK_SAME_STRUCTURE : "No permission for the structure with same names",
    ERROR_DECLARATION_DATA_TYPE_CHECK_UNION_TYPE : "Union Type should have a 'typedef' and the name must be in capital letters",
    ERROR_DECLARATION_DATA_TYPE_CHECK_NESTED_STRUCTURE : "Complex types should be typedef-ed",

    ERROR_NAMING_CONVENTION_CHECK_ALL : "",
    ERROR_NAMING_CONVENTION_CHECK_DEFINE_STATEMENT : "Only capital letters are allowed to be used for #define declarations",
    ERROR_NAMING_CONVENTION_CHECK_TYPEDEF_STATEMENT : "Only capital letters are allowed to be used for typedef declarations",
    ERROR_NAMING_CONVENTION_CHECK_IFNDEF_STATEMENT : "The #ifndef at the start of an include file should have one postfix underscore, and no prefix underscore character '_'",
    ERROR_NAMING_CONVENTION_CHECK_PATH_NAME : """Path name does not follow the rules: 1. First character should be upper case 2. Must contain lower case characters 3. No white space characters""",
    ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME : """Variable name does not follow the rules: 1. First character should be upper case 2. Must contain lower case characters 3. No white space characters 4. Global variable name must start with a 'g'""",
    ERROR_NAMING_CONVENTION_CHECK_FUNCTION_NAME : """Function name does not follow the rules: 1. First character should be upper case 2. Must contain lower case characters 3. No white space characters""",
    ERROR_NAMING_CONVENTION_CHECK_SINGLE_CHARACTER_VARIABLE : "There should be no use of short (single character) variable names",

    ERROR_DOXYGEN_CHECK_ALL : "",
    ERROR_DOXYGEN_CHECK_FILE_HEADER : "The file headers should follow Doxygen special documentation blocks in section 2.3.5",
    ERROR_DOXYGEN_CHECK_FUNCTION_HEADER : "The function headers should follow Doxygen special documentation blocks in section 2.3.5",
    ERROR_DOXYGEN_CHECK_COMMENT_DESCRIPTION : """The first line of text in a comment block should be a brief description of the element being documented and the brief description must end with a period.""",
    ERROR_DOXYGEN_CHECK_COMMENT_FORMAT : "For comment line with '///< ... text ...' format, if it is used, it should be after the code section",
    ERROR_DOXYGEN_CHECK_COMMAND : "Only Doxygen commands '@bug', '@todo', '@example', '@file', '@attention', '@param', '@post', '@pre', '@retval', '@return', '@sa', '@since', '@test', '@note', '@par', '@endcode', '@code', '@{', '@}' are allowed to mark the code",

    ERROR_META_DATA_FILE_CHECK_ALL : "",
    ERROR_META_DATA_FILE_CHECK_PATH_NAME : "The file defined in meta-data does not exist",
    ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_1 : "A library instances defined for a given module (or dependent library instance) doesn't match the module's type.",
    ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_2 : "A library instance must specify the Supported Module Types in its INF file",
    ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_DEPENDENT : "A library instance must be defined for all dependent library classes",
    ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_ORDER : "The library Instances specified by the LibraryClasses sections should be listed in order of dependencies",
    ERROR_META_DATA_FILE_CHECK_LIBRARY_NO_USE : "There should be no unnecessary inclusion of library classes in the INF file",
    ERROR_META_DATA_FILE_CHECK_LIBRARY_NAME_DUPLICATE : "Duplicate Library Class Name found",
    ERROR_META_DATA_FILE_CHECK_BINARY_INF_IN_FDF : "An INF file is specified in the FDF file, but not in the DSC file, therefore the INF file must be for a Binary module only",
    ERROR_META_DATA_FILE_CHECK_PCD_DUPLICATE : "Duplicate PCDs found",
    ERROR_META_DATA_FILE_CHECK_PCD_FLASH : "PCD settings in the FDF file should only be related to flash",
    ERROR_META_DATA_FILE_CHECK_PCD_NO_USE : "There should be no PCDs declared in INF files that are not specified in in either a DSC or FDF file",
    ERROR_META_DATA_FILE_CHECK_DUPLICATE_GUID : "Duplicate GUID found",
    ERROR_META_DATA_FILE_CHECK_DUPLICATE_PROTOCOL : "Duplicate PROTOCOL found",
    ERROR_META_DATA_FILE_CHECK_DUPLICATE_PPI : "Duplicate PPI found",
    ERROR_META_DATA_FILE_CHECK_MODULE_FILE_NO_USE : "No used module files found",
    ERROR_META_DATA_FILE_CHECK_PCD_TYPE : "Wrong C code function used for this kind of PCD",
    ERROR_META_DATA_FILE_CHECK_MODULE_FILE_GUID_DUPLICATION : "Module file has FILE_GUID collision with other module file",
    ERROR_META_DATA_FILE_CHECK_FORMAT_GUID : "Wrong GUID Format used in Module file",
    ERROR_META_DATA_FILE_CHECK_FORMAT_PROTOCOL : "Wrong Protocol Format used in Module file",
    ERROR_META_DATA_FILE_CHECK_FORMAT_PPI : "Wrong Ppi Format used in Module file",
    ERROR_META_DATA_FILE_CHECK_FORMAT_PCD : "Wrong Pcd Format used in Module file",
    ERROR_META_DATA_FILE_CHECK_LIBRARY_NOT_DEFINED : "Not defined LibraryClass used in the Module file.",
    ERROR_SPELLING_CHECK_ALL : "",

    ERROR_SMM_COMM_PARA_CHECK_BUFFER_TYPE : "SMM communication function may use wrong parameter type",
    }

