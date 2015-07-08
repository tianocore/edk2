## @file
# This file is used to define strings used in the UPT tool
#
# Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
##
"""
This file contains user visible strings in a format that can be used for
localization
"""

import gettext

#
# string table starts here...
#

## strings are classified as following types
#    MSG_...: it is a message string
#    ERR_...: it is a error string
#    WRN_...: it is a warning string
#    HLP_...: it is a help string
#

_ = gettext.gettext

MSG_USAGE_STRING = _("\n"
    "UEFI Packaging Tool (UEFIPT)\n"
    "%prog [options]"
    )

##
# Version and Copyright
#
MSG_VERSION_NUMBER = _("1.0")
MSG_VERSION = _("UEFI Packaging Tool (UEFIPT) - Revision " + \
                MSG_VERSION_NUMBER)
MSG_COPYRIGHT = _("Copyright (c) 2011 - 2015 Intel Corporation All Rights Reserved.")
MSG_VERSION_COPYRIGHT = _("\n  %s\n  %s" % (MSG_VERSION, MSG_COPYRIGHT))
MSG_USAGE = _("%s [options]\n%s" % ("UPT", MSG_VERSION_COPYRIGHT))
MSG_DESCRIPTION = _("The UEFIUPT is used to create, " + \
                    "install or remove a UEFI Distribution Package. " + \
                    "If WORKSPACE environment variable is present, " + \
                    "then UPT will install packages to the location specified by WORKSPACE, " + \
                    "otherwise UPT will install packages to the current directory. " + \
                    "Option -n will override this default installation location")

#
# INF Parser related strings.
#
ERR_INF_PARSER_HEADER_FILE = _(
    "The Header comment section should start with an @file at the top.")
ERR_INF_PARSER_HEADER_MISSGING = _(
    "The Header comment is missing. It must be corrected before continuing.")
ERR_INF_PARSER_UNKNOWN_SECTION = _("An unknown section was found. "
                                   "It must be corrected before continuing. ")
ERR_INF_PARSER_NO_SECTION_ERROR = _("No section was found. "
                            "A section must be included before continuing.")
ERR_INF_PARSER_BUILD_OPTION_FORMAT_INVALID = \
    _("Build Option format incorrect.")
ERR_INF_PARSER_BINARY_ITEM_FORMAT_INVALID = _(
     "The format of binary %s item is incorrect. "
     "It should contain at least %d elements.")
ERR_INF_PARSER_BINARY_ITEM_FORMAT_INVALID_MAX = _(
     "The format of binary %s item is invalid, "
     "it should contain not more than %d elements.")
ERR_INF_PARSER_BINARY_ITEM_INVALID_FILETYPE = _(
     "The Binary FileType is incorrect. It should in %s")
ERR_INF_PARSER_BINARY_ITEM_FILE_NOT_EXIST = _(
     "The Binary File: %s not exist.")
ERR_INF_PARSER_BINARY_ITEM_FILENAME_NOT_EXIST = _(
     "The Binary File Name item not exist")
ERR_INF_PARSER_BINARY_VER_TYPE = _(
     "Only this type is allowed: \"%s\".")
ERR_INF_PARSER_MULTI_DEFINE_SECTION = \
    _("Multiple define sections found. "
      "It must be corrected before continuing.")
ERR_INF_PARSER_DEFINE_ITEM_MORE_THAN_ONE_FOUND = \
    _("More than 1 %s is defined in DEFINES section. "
      "It must be corrected before continuing.")
ERR_INF_PARSER_DEFINE_NAME_INVALID = \
    _("Incorrect name format for : %s")
ERR_INF_PARSER_DEFINE_GUID_INVALID = \
    _("The format of this GUID is incorrect: %s")
ERR_INF_PARSER_DEFINE_MODULETYPE_INVALID = _("Incorrect MODULE_TYPE: %s")
ERR_INF_PARSER_DEFINE_FROMAT_INVALID = _("Incorrect format: %s")
ERR_INF_PARSER_FILE_NOT_EXIST = _("This file does not exist: %s")
ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID = \
    _("The file does not exist or not in sub-directories "
      "or has an incorrect file name of the directory containing the INF or DEC file: %s. "
      "It must be corrected before continuing")
ERR_INF_PARSER_DEFINE_SHADOW_INVALID = \
   _("The SHADOW keyword is only valid for"
                                       " SEC, PEI_CORE and PEIM module types.")
ERR_INF_PARSER_DEFINE_SECTION_HEADER_INVALID = \
    _("The format of the section header is incorrect")
ERR_INF_PARSER_DEPEX_SECTION_INVALID = \
    _("A module can't have a Depex section when its module type is %s")
ERR_INF_PARSER_DEPEX_SECTION_INVALID_FOR_BASE_LIBRARY_CLASS = \
    _("A base type library class can't have a Depex section with module type not defined.")
ERR_INF_PARSER_DEPEX_SECTION_INVALID_FOR_LIBRARY_CLASS = \
    _("A library class can't have a Depex section when its supported module type list is not defined.")
ERR_INF_PARSER_DEPEX_SECTION_INVALID_FOR_DRIVER = \
    _("A driver can't have a Depex section when its module type is UEFI_DRIVER.")
ERR_INF_PARSER_DEPEX_SECTION_NOT_DETERMINED  = \
    _("Cannot determine the module's Depex type. The Depex's module types are conflict")
ERR_INF_PARSER_DEFINE_SECTION_MUST_ITEM_NOT_EXIST = _(
                "No %s found in INF file, please check it.")
ERR_INF_PARSER_DEPEX_SECTION_MODULE_TYPE_ERROR = \
    _("The module type of [Depex] section is invalid, not support type of %s")
ERR_INF_PARSER_DEPEX_SECTION_CONTENT_MISSING = \
    _("Missing content in: %s")
ERR_INF_PARSER_DEPEX_SECTION_CONTENT_ERROR  = \
    _("The [Depex] section contains invalid content: %s")                    
ERR_INF_PARSER_DEPEX_SECTION_SEC_TYPE_ERROR = \
    _("The format is incorrect. The section type keyword of the content in the"
      " [Depex] section is only for 'PEI_DEPEX', 'DXE_DEPEX', 'SMM_DEPEX', "
      "it does not support type:  %s")
ERR_INF_PARSER_UE_SECTION_USER_ID_ERROR = \
    _("This format is incorrect. "
      "The UserID: %s in [UserExtension] section is incorrect.")
ERR_INF_PARSER_UE_SECTION_ID_STRING_ERROR = \
    _("This format is incorrect. "
      "IdString: %s in [UserExtension] section is incorrect.")
ERR_INF_PARSER_LIBRARY_SECTION_CONTENT_ERROR = \
    _("The format is incorrect. "
      "You can only have a Library name and a Feature flag in one line.")
ERR_INF_PARSER_LIBRARY_SECTION_LIBNAME_MISSING = \
    _("Format invalid. Please specify a library name.")
ERR_INF_PARSER_SOURCES_SECTION_CONTENT_ERROR = \
    _("The format is incorrect. It should be formated as follows: "
      "FileName, Family | TagName | ToolCode | FeatureFlagExpr.")
ERR_INF_PARSER_PCD_SECTION_TYPE_ERROR = \
    _("The PCD section type is incorrect. The value should be this list: %s")
ERR_INF_PARSER_PCD_SECTION_CONTENT_ERROR = \
    _("PcdName format invalid." 
      "Should like following: PcdName | Value | FeatureFlag.")
ERR_INF_PARSER_PCD_NAME_FORMAT_ERROR = \
    _("Format invalid." 
      "Should like following: <TokenSpaceGuidCName>.<PcdCName> ")   
ERR_INF_PARSER_GUID_PPI_PROTOCOL_SECTION_CONTENT_ERROR = \
    _("The format is incorrect. "
      "It should be formated as follows: CName | FeatureFlag.")
ERR_INF_PARSER_PACKAGE_SECTION_CONTENT_ERROR = \
    _("The format is incorrect. "
      "It should be formated as follows:  <TokenSpaceGuidCName>.<PcdCName>")
ERR_INF_PARSER_PCD_TAIL_COMMENTS_INVALID = \
    _("The format is incorrect. "
      "Multiple usage descriptions must be described on subsequent lines.")
ERR_INF_PARSER_MODULE_SECTION_TYPE_ERROR = \
    _("This section format is incorrect: %s.")
ERR_INF_PARSER_SECTION_NAME_DUPLICATE = \
    _("This section has multiple section names, "
      "only one section name is permitted.")
ERR_INF_PARSER_SECTION_ARCH_CONFLICT = \
    _("The 'common' ARCH must not be used with the specified ARCHs.")
ERR_INF_PARSER_SOURCE_SECTION_TAGNAME_INVALID = \
    _("This TagName is incorrect: %s. "
      "It must be corrected before continuing.")
ERR_INF_PARSER_TAGNAME_NOT_PERMITTED = \
    _("TagName is not permitted: %s. "
      "It must be corrected before continuing.")
ERR_INF_PARSER_TOOLCODE_NOT_PERMITTED = \
    _("ToolCode is not permitted: %s. "
      "It must be corrected before continuing.")
ERR_INF_PARSER_SOURCE_SECTION_FAMILY_INVALID = \
    _("This family is incorrect: %s. "
      "It must be corrected before continuing. ")
ERR_INF_PARSER_SOURCE_SECTION_SECTIONNAME_INVALID = \
    _("This SectionName is incorrect: %s. "
      "It must be corrected before continuing.")
ERR_INF_PARSER_PCD_CVAR_GUID = \
    _("TokenSpaceGuidCName must be valid C variable format.")
ERR_INF_PARSER_PCD_CVAR_PCDCNAME = \
    _("PcdCName must be valid C variable format.")
ERR_INF_PARSER_PCD_VALUE_INVALID = \
    _("The PCD value is incorrect. It must be corrected before continuing.")    
ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID = \
    _("Incorrect feature flag expression: %s")
ERR_INF_PARSER_FEATURE_FLAG_EXP_MISSING = \
    _("The feature flag expression is missing. Please specify a feature flag.")
ERR_INF_PARSER_INVALID_CNAME = \
    _("Incorrect CName: %s. You must specify a valid C variable name.")
ERR_INF_PARSER_CNAME_MISSING = \
    _("Missing CName. Specify a valid C variable name.")
ERR_INF_PARSER_DEFINE_SECTION_KEYWORD_INVALID = \
    _("The Define section contains an invalid keyword:  \"%s\"."  
    "It must be corrected before continuing.")
ERR_INF_PARSER_FILE_MISS_DEFINE = \
    _("The following file listed in the module "
      "directory is not listed in the INF: %s")
ERR_INF_PARSER_VERSION_NUMBER_DEPRICATED = \
    _("VERSION_NUMBER depricated.  "
      "The INF file %s should be modified to use the VERSION_STRING instead.")
ERR_INF_PARSER_VER_EXIST_BOTH_NUM_STR = \
    _("The INF file %s defines both VERSION_NUMBER and VERSION_STRING, "
      "using VERSION_STRING")
ERR_INF_PARSER_NOT_SUPPORT_EDKI_INF = _("EDKI INF is not supported")
ERR_INF_PARSER_EDKI_COMMENT_IN_EDKII = _("The EDKI style comment is not supported in EDKII modules")

ERR_INF_PARSER_FEATUREPCD_USAGE_INVALID = _("The usage for FeaturePcd can only"
    " be type of \"CONSUMES\".") 

ERR_INF_PARSER_DEFINE_ITEM_NO_NAME = _("No name specified")
ERR_INF_PARSER_DEFINE_ITEM_NO_VALUE = _("No value specified")

ERR_INF_PARSER_MODULETYPE_INVALID = _("Drivers and applications are not allowed to have a MODULE_TYPE of \"BASE\". "
"Only libraries are permitted to a have a MODULE_TYPE of \"BASE\".")
ERR_INF_GET_PKG_DEPENDENCY_FAIL = _("Failed to get PackageDependencies information from file %s")
ERR_INF_NO_PKG_DEPENDENCY_INFO = _("There are no packages defined that use the AsBuilt PCD information.")

#
# Item duplicate
#
ERR_INF_PARSER_ITEM_DUPLICATE_IN_DEC = \
_('"%s" is redefined in its dependent DEC files')
ERR_INF_PARSER_ITEM_DUPLICATE = _("%s define duplicated! " 
                                  "It must be corrected before continuing.")
ERR_INF_PARSER_ITEM_DUPLICATE_COMMON = _("%s define duplicated! Item listed" 
"in an architectural section must not be listed in the common architectural"
"section.It must be corrected before continuing.")
ERR_INF_PARSER_UE_SECTION_DUPLICATE_ERROR = \
_("%s define duplicated! Each UserExtensions section header must have a "
  "unique set of UserId, IdString and Arch values. "
  "It must be corrected before continuing.")

ERR_INF_PARSER_DEFINE_LIB_NAME_INVALID = \
_("The name 'NULL' for LibraryClass is a reserved word."
"Please don't use it.")

ERR_GLOBAL_MARCO_INVALID = \
_("Using global MACRO in INF/DEC is not permitted: %s . "
"It must be corrected before continuing.")                                          

ERR_MARCO_DEFINITION_MISS_ERROR = \
_("MACRO expand incorrectly, can not find the MACRO definition. "
"It must be corrected before continuing.") 

#
# AsBuilt related
#
ERR_LIB_CONTATIN_ASBUILD_AND_COMMON = _("A binary INF file should not contain both AsBuilt LIB_INSTANCES information "
                                        "and a common library entry.")
ERR_LIB_INSTANCE_MISS_GUID = _("Could not get FILE_GUID definition from instance INF file.")

ERR_BO_CONTATIN_ASBUILD_AND_COMMON = _("A binary INF file should contain either AsBuilt information "
                                       "or a common build option entry, not both.")

ERR_ASBUILD_PCD_SECTION_TYPE = _("The AsBuilt INF file contains a PCD section type that is not permitted: %s.")
ERR_ASBUILD_PATCHPCD_FORMAT_INVALID = _("The AsBuilt PatchPcd entry must contain 3 elements: PcdName|Value|Offset")
ERR_ASBUILD_PCDEX_FORMAT_INVALID = _("The AsBuilt PcdEx entry must contain one element: PcdName")
ERR_ASBUILD_PCD_VALUE_INVALID = \
    _("The AsBuilt PCD value %s is incorrect or not align with it's datum type %s. "
      "It must be corrected before continuing.")
ERR_ASBUILD_PCD_TOKENSPACE_GUID_VALUE_MISS = _("Package file value could not be retrieved for %s.")
ERR_ASBUILD_PCD_DECLARITION_MISS = _("PCD Declaration in DEC files could not be found for: %s.")
ERR_ASBUILD_PCD_OFFSET_FORMAT_INVALID = _("PCD offset format invalid, number of (0-4294967295) or"
"Hex number of UINT32 allowed : %s.")

#
# XML parser related strings
#
ERR_XML_PARSER_REQUIRED_ITEM_MISSING = \
    _("The XML section/attribute '%s' is required under %s, it can't be missing or empty")
ERR_XML_INVALID_VARIABLENAME = \
    _("The VariableName of the GUID in the XML tree does not conform to the packaging specification.  "
      "Only a Hex Byte Array of UCS-2 format or L\"string\" is allowed): %s %s %s")
ERR_XML_INVALID_LIB_SUPMODLIST = _("The LIBRARY_CLASS entry %s must have the list appended using the format as: \n"
"BASE SEC PEI_CORE PEIM DXE_CORE DXE_DRIVER SMM_CORE DXE_SMM_DRIVER DXE_RUNTIME_DRIVER "
"DXE_SAL_DRIVER UEFI_DRIVER UEFI_APPLICATION USER_DEFINED\n Current is %s.")
ERR_XML_INVALID_EXTERN_SUPARCHLIST = \
    _("There is a mismatch of SupArchList %s between the EntryPoint, UnloadImage, Constructor, "
      "and Destructor elements in the ModuleSurfaceArea.ModuleProperties: SupArchList: %s. ")
ERR_XML_INVALID_EXTERN_SUPMODLIST = _("The SupModList attribute of the CONSTRUCTOR or DESTRUCTOR element: %s does not "
"match the Supported Module Types listed after LIBRARY_CLASS = <Keyword> | %s")
ERR_XML_INVALID_EXTERN_SUPMODLIST_NOT_LIB = _("The module is not a library module. "
                                              "The MODULE_TYPE : %s listed in the ModuleSurfaceArea.Header "
                                              "must match the SupModList attribute %s")
ERR_XML_INVALID_BINARY_FILE_TYPE = _("Invalid binary file type %s.")

#
# Verbosity related strings.
#
MSG_DISTRIBUTION_PACKAGE_FILE_EXISTS = _(
    "The distribution package file %s already exists.\nPress Y to override it."
    " To exit the application, press any other key.")
MSG_CHECK_MODULE_EXIST         = _(
    "\nChecking to see if module exists in workspace started ...")
MSG_CHECK_MODULE_EXIST_FINISH  = \
    _("Checking to see if  module exists in workspace ... Done.")
MSG_CHECK_MODULE_DEPEX_START   = _(
    "\nChecking to see if module depex met by workspace started ...")
MSG_CHECK_MODULE_DEPEX_FINISH  = _(
    "Checking to see if module depex met by workspace ... Done.")
MSG_CHECK_PACKAGE_START        = _(
    "\nChecking to see if  package exists in workspace started ...")
MSG_CHECK_PACKAGE_FINISH       = _(
    "Checking to see if  package exists in workspace ... Done.")
MSG_CHECK_DP_START             = \
    _("\nChecking to see if DP exists in workspace ... Done.")
MSG_CHECK_DP_FINISH            = _("Check DP exists in workspace ... Done.")
MSG_MODULE_DEPEND_ON           = _("Module %s depends on Package %s")
MSG_INIT_IPI_START             = _("\nInitialize IPI database started ...")
MSG_INIT_IPI_FINISH            = _("Initialize IPI database ... Done.")
MSG_GET_DP_INSTALL_LIST        = _(
    "\nGetting list of DP install information started ...")
MSG_GET_DP_INSTALL_INFO_START  = _(
    "\nGetting list of DP install information started ...")
MSG_GET_DP_INSTALL_INFO_FINISH = _("Getting DP install information ... Done.")
MSG_UZIP_PARSE_XML             = _(
    "Unzipping and parsing distribution package XML file ... ")
MSG_INSTALL_PACKAGE            = _("Installing package ... %s")
MSG_INSTALL_MODULE             = _("Installing module ... %s")
MSG_NEW_FILE_NAME_FOR_DIST      = _(
    "Provide new filename for distribution file to be saved:\n")
MSG_UPDATE_PACKAGE_DATABASE    = _("Update Distribution Package Database ...")
MSG_PYTHON_ON                  = _("(Python %s on %s) ")
MSG_SEARCH_FOR_HELP            = _(
    "\n(Please send email to edk2-devel@lists.sourceforge.net for\n"
    " help, attach the following call stack trace.)\n")
MSG_REMOVE_TEMP_FILE_STARTED   = _("Removing temp files started ... ")
MSG_REMOVE_TEMP_FILE_DONE   = _("Removing temp files ... Done.")
MSG_FINISH                     = _("Successfully Done.")
MSG_COMPRESS_DISTRIBUTION_PKG  = _("Compressing Distribution Package File ...")
MSG_CONFIRM_REMOVE             = _(
    "Some packages or modules depend on this distribution package.\n"
    "Do you really want to remove it?")
MSG_CONFIRM_REMOVE2            = _(
    "This file has been modified: %s. Do you want to remove it?"
    "Press Y to remove or other key to keep it")
MSG_CONFIRM_REMOVE3            = _(
    "This is a newly created file: %s.  Are you sure you want to remove it?  "
    "Press Y to remove or any other key to keep it")
MSG_USER_DELETE_OP             = _(
    "Press Y to delete all files or press any other key to quit:")
MSG_REMOVE_FILE                = _("Removing file: %s ...")

MSG_INITIALIZE_ECC_STARTED     = _("\nInitialize ECC database started ...")
MSG_INITIALIZE_ECC_DONE        = _("Initialize ECC database ... Done.")
MSG_DEFINE_STATEMENT_FOUND     = _("DEFINE statement '%s' found in section %s")
MSG_PARSING                    = _("Parsing %s ...")

MSG_REPKG_CONFLICT             = \
_("Repackaging is not allowed on this file: %s. "
  "It was installed from distribution %s(Guid %s Version %s).")

MSG_INVALID_MODULE_INTRODUCED  = _("Some modules are not valid after removal.")
MSG_CHECK_LOG_FILE             = _("Please check log file %s for full list")
MSG_NEW_FILE_NAME      = _(
    "Provide new filename:\n")
MSG_RELATIVE_PATH_ONLY = _("Please specify a relative path, full path is not allowed: %s")
MSG_NEW_PKG_PATH  = _(
    "Select package location.  To quit with no input, press [Enter].")
MSG_CHECK_DP_FOR_REPLACE = _("Verifying the dependency rule for replacement of distributions:\n %s replaces %s")
MSG_CHECK_DP_FOR_INSTALL = _("Verifying the dependency rule for installation of distribution:\n %s")
MSG_REPLACE_ALREADY_INSTALLED_DP = _("Distribution with the same GUID/Version is already installed, "
                                     "replace would result in two instances, which is not allowed")
MSG_RECOVER_START = _('An error was detected, recovery started ...')
MSG_RECOVER_DONE = _('Recovery completed.')
MSG_RECOVER_FAIL = _('Recovery failed.')
#
# Error related strings.
#

ERR_DEPENDENCY_NOT_MATCH         = _(
    "Module %s's dependency on package %s (GUID %s Version %s) " 
    "cannot be satisfied")
ERR_MODULE_NOT_INSTALLED         = _(
    "This module is not installed in the workspace: %s\n")
ERR_DIR_ALREADY_EXIST            = _(
    "This directory already exists: %s.\n"
    "Select another location.  Press [Enter] with no input to quit:")
ERR_USER_INTERRUPT               = _("The user has paused the application")
ERR_DIST_FILE_TOOMANY            = _(
    "Only one .content and one .pkg file in ZIP file are allowed.")
ERR_DIST_FILE_TOOFEW             = _(
    "Must have one .content and one .pkg file in the ZIP file.")
ERR_FILE_ALREADY_EXIST           = _(
    "This file already exists: %s.\n"
    "Select another path to continue. To quit with no input press [Enter]:")
ERR_SPECIFY_PACKAGE              = _(
    "One distribution package must be specified")
ERR_FILE_BROKEN                  = _(
    "This file is invalid in the distribution package: %s")
ERR_PACKAGE_NOT_MATCH_DEPENDENCY = _(
    "This distribution package does not meet the dependency requirements")
ERR_UNKNOWN_FATAL_INSTALL_ERR    = \
_("Unknown unrecoverable error when installing: %s")
ERR_UNKNOWN_FATAL_REPLACE_ERR    = \
_("Unknown unrecoverable error during replacement of distributions: %s replaces %s")
ERR_OPTION_NOT_FOUND             = _("Options not found")
ERR_INVALID_PACKAGE_NAME         = _("Incorrect package name: %s. ")
ERR_INVALID_PACKAGE_PATH         = \
_("Incorrect package path: %s. The path must be a relative path.")
ERR_NOT_FOUND                    = _("This was not found: %s")
ERR_INVALID_MODULE_NAME          = _("This is not a valid module name: %s")
ERR_INVALID_METAFILE_PATH        = _('This file must be in sub-directory of WORKSPACE: %s.')
ERR_INVALID_MODULE_PATH          = \
_("Incorrect module path: %s. The path must be a relative path.")
ERR_UNKNOWN_FATAL_CREATING_ERR   = _("Unknown error when creating: %s")
ERR_PACKAGE_NOT_INSTALLED        = _(
    "This distribution package not installed: %s")
ERR_DISTRIBUTION_NOT_INSTALLED   = _(
    "The distribution package is not installed.")
ERR_UNKNOWN_FATAL_REMOVING_ERR   = _("Unknown error when removing package")
ERR_UNKNOWN_FATAL_INVENTORYWS_ERR   = _("Unknown error when inventorying WORKSPACE")
ERR_NOT_CONFIGURE_WORKSPACE_ENV  = _(
    "The WORKSPACE environment variable must be configured.")
ERR_NO_TEMPLATE_FILE             = _("This package information data file is not found: %s")
ERR_DEBUG_LEVEL                  = _(
    "Not supported debug level. Use default level instead.")
ERR_REQUIRE_T_OPTION             = _(
    "Option -t is required during distribution creation.")
ERR_REQUIRE_O_OPTION             = _(
    "Option -o is required during distribution replacement.")
ERR_REQUIRE_U_OPTION             = _(
    "Option -u is required during distribution replacement.")
ERR_REQUIRE_I_C_R_OPTION         = _(
    "Options -i, -c and -r are mutually exclusive.")
ERR_I_C_EXCLUSIVE                = \
_("Option -c and -i are mutually exclusive.")
ERR_I_R_EXCLUSIVE                = \
_("Option -i and -r are mutually exclusive.")
ERR_C_R_EXCLUSIVE                = \
_("Option -c and -r are mutually exclusive.")
ERR_U_ICR_EXCLUSIVE                = \
_("Option -u and -c/-i/-r are mutually exclusive.")

ERR_L_OA_EXCLUSIVE                = \
_("Option -l and -c/-i/-r/-u are mutually exclusive.")

ERR_FAILED_LOAD                  = _("Failed to load %s\n\t%s")
ERR_PLACEHOLDER_DIFFERENT_REPEAT = _(
    "${%s} has different repeat time from others.")
ERR_KEY_NOTALLOWED               = _("This keyword is not allowed: %s")
ERR_NOT_FOUND_ENVIRONMENT        = _("Environment variable not found")
ERR_WORKSPACE_NOTEXIST           = _("WORKSPACE doesn't exist")
ERR_SPACE_NOTALLOWED             = _(
    "Whitespace characters are not allowed in the WORKSPACE path. ")
ERR_MACRONAME_NOGIVEN            = _("No MACRO name given")
ERR_MACROVALUE_NOGIVEN           = _("No MACRO value given")
ERR_MACRONAME_INVALID            = _("Incorrect MACRO name: %s")
ERR_MACROVALUE_INVALID            = _("Incorrect MACRO value: %s")
ERR_NAME_ONLY_DEFINE             = _(
    "This variable can only be defined via environment variable: %s")
ERR_EDK_GLOBAL_SAMENAME          = _(
    "EDK_GLOBAL defined a macro with the same name as one defined by 'DEFINE'")
ERR_SECTIONNAME_INVALID          = _(
    "An incorrect section name was found: %s. 'The correct file is '%s' .")
ERR_CHECKFILE_NOTFOUND           = _(
    "Can't find file '%s' defined in section '%s'")
ERR_INVALID_NOTFOUND             = _(
    "Incorrect statement '%s' was found in section '%s'")
ERR_TEMPLATE_NOTFOUND            = _("This package information data file is not found: %s")
ERR_SECTION_NAME_INVALID         = _('Incorrect section name: %s')
ERR_SECTION_REDEFINE             = _(
    "This section already defined: %s.")
ERR_SECTION_NAME_NONE            = \
    _('The section needs to be specified first.')
ERR_KEYWORD_INVALID              = _('Invalid keyword: %s')
ERR_VALUE_INVALID                = _("Invalid \"%s\" value in section [%s].")
ERR_FILELIST_LOCATION            = _(
    'The directory "%s" must contain this file: "%s".')
ERR_KEYWORD_REDEFINE             = _(
    "Keyword in this section can only be used once: %s.")
ERR_FILELIST_EXIST               = _(
    'This file does not exist: %s.')
ERR_COPYRIGHT_CONTENT            = _(
    "The copyright content must contain the word \"Copyright\" (case insensitive).")
ERR_WRONG_FILELIST_FORMAT        = \
_('File list format is incorrect.' 
  'The correct format is: filename|key=value[|key=value]')
ERR_FILELIST_ATTR                = _(
    "The value of attribute \"%s\" includes illegal character.")
ERR_UNKNOWN_FILELIST_ATTR        = _(
    'Unknown attribute name: %s.')
ERR_EMPTY_VALUE                  = _("Empty value is not allowed")
ERR_KEYWORD_MANDATORY            = _('This keyword is mandatory: %s')
ERR_BOOLEAN_VALUE                = _(
    'Value of key [%s] must be true or false, current: [%s]')
ERR_GUID_VALUE                   = _(
    'GUID must have the format of 8-4-4-4-12 with HEX value. '
    'Current value: [%s]')
ERR_VERSION_VALUE                = _(
    'The value of key [%s] must be a decimal number. Found: [%s]')
ERR_VERSION_XMLSPEC              = _(
    'XmlSpecification value must be 1.1, current: %s.')

ERR_INVALID_GUID                 = _("Incorrect GUID value string: %s")

ERR_FILE_NOT_FOUND               = \
    _("File or directory not found in workspace")
ERR_FILE_OPEN_FAILURE            = _("Could not open file")
ERR_FILE_WRITE_FAILURE           = _("Could not write file.")
ERR_FILE_PARSE_FAILURE           = _("Could not parse file")
ERR_FILE_READ_FAILURE            = _("Could not read file")
ERR_FILE_CREATE_FAILURE          = _("Could not create file")
ERR_FILE_CHECKSUM_FAILURE        = _("Checksum of file is incorrect")
ERR_FILE_COMPRESS_FAILURE        = _("File compression did not correctly")
ERR_FILE_DECOMPRESS_FAILURE      = \
    _("File decompression did not complete correctly")
ERR_FILE_MOVE_FAILURE            = _("Move file did not complete successfully")
ERR_FILE_DELETE_FAILURE          = _("File could not be deleted")
ERR_FILE_COPY_FAILURE            = _("File did not copy correctly")
ERR_FILE_POSITIONING_FAILURE     = _("Could not find file seek position")
ERR_FILE_TYPE_MISMATCH           = _("Incorrect file type")
ERR_FILE_CASE_MISMATCH           = _("File name case mismatch")
ERR_FILE_DUPLICATED              = _("Duplicate file found")
ERR_FILE_UNKNOWN_ERROR           = _("Unknown error encountered on file")
ERR_FILE_NAME_INVALIDE           = _("This file name is invalid, it must not be an absolute path or "
                                     "contain a period \".\" or \"..\":  %s.")
ERR_OPTION_UNKNOWN               = _("Unknown option")
ERR_OPTION_MISSING               = _("Missing option")
ERR_OPTION_CONFLICT              = _("Options conflict")
ERR_OPTION_VALUE_INVALID         = _("Invalid option value")
ERR_OPTION_DEPRECATED            = _("Deprecated option")
ERR_OPTION_NOT_SUPPORTED         = _("Unsupported option")
ERR_OPTION_UNKNOWN_ERROR         = _("Unknown error when processing options")
ERR_PARAMETER_INVALID            = _("Invalid parameter")
ERR_PARAMETER_MISSING            = _("Missing parameter")
ERR_PARAMETER_UNKNOWN_ERROR      = _("Unknown error in parameters")
ERR_FORMAT_INVALID               = _("Invalid syntax/format")
ERR_FORMAT_NOT_SUPPORTED         = _("Syntax/format not supported")
ERR_FORMAT_UNKNOWN               = _("Unknown format")
ERR_FORMAT_UNKNOWN_ERROR         = _("Unknown error in syntax/format ")
ERR_RESOURCE_NOT_AVAILABLE       = _("Not available")
ERR_RESOURCE_ALLOCATE_FAILURE    = _("A resource allocation has failed")
ERR_RESOURCE_FULL                = _("Full")
ERR_RESOURCE_OVERFLOW            = _("Overflow")
ERR_RESOURCE_UNDERRUN            = _("Underrun")
ERR_RESOURCE_UNKNOWN_ERROR       = _("Unknown error")
ERR_ATTRIBUTE_NOT_AVAILABLE      = _("Not available")
ERR_ATTRIBUTE_RETRIEVE_FAILURE   = _("Unable to retrieve")
ERR_ATTRIBUTE_SET_FAILURE        = _("Unable to set")
ERR_ATTRIBUTE_UPDATE_FAILURE     = _("Unable to update")
ERR_ATTRIBUTE_ACCESS_DENIED      = _("Access denied")
ERR_ATTRIBUTE_UNKNOWN_ERROR      = _("Unknown error when accessing")
ERR_COMMAND_FAILURE              = _("Unable to execute command")
ERR_IO_NOT_READY                 = _("Not ready")
ERR_IO_BUSY                      = _("Busy")
ERR_IO_TIMEOUT                   = _("Timeout")
ERR_IO_UNKNOWN_ERROR             = _("Unknown error in IO operation")
ERR_UNKNOWN_ERROR                = _("Unknown error")
ERR_UPT_ALREADY_INSTALLED_ERROR  = _("Already installed")
ERR_UPT_ENVIRON_MISSING_ERROR    = _("Environ missing")
ERR_UPT_REPKG_ERROR              = _("File not allowed for RePackage")
ERR_UPT_DB_UPDATE_ERROR          = _("Update database did not complete successfully")
ERR_UPT_INI_PARSE_ERROR          = _("INI file parse error")
ERR_COPYRIGHT_MISSING            = \
_("Header comment section must have copyright information")
ERR_LICENSE_MISSING              = \
_("Header comment section must have license information")
ERR_INVALID_BINARYHEADER_FORMAT  = \
_("Binary Header comment section must have abstract,description,copyright,license information")
ERR_MULTIPLE_BINARYHEADER_EXIST = \
_("the inf file at most support one BinaryHeader at the fileheader section.")
ERR_INVALID_COMMENT_FORMAT       = _("Comment must start with #")
ERR_USER_ABORT                   = _("User has stopped the application")
ERR_DIST_EXT_ERROR               = \
_("Distribution file extension should be '.dist'. Current given: '%s'.")
ERR_DIST_FILENAME_ONLY_FOR_REMOVE               = \
_("Only distribution filename without path allowed during remove. Current given: '%s'.")
ERR_NOT_STANDALONE_MODULE_ERROR  = \
    _("Module %s is not a standalone module (found in Package %s)")
ERR_UPT_ALREADY_RUNNING_ERROR    = \
    _("UPT is already running, only one instance is allowed")
ERR_MUL_DEC_ERROR = _("Multiple DEC files found within one package directory tree %s: %s, %s")
ERR_INSTALL_FILE_FROM_EMPTY_CONTENT = _("Error file to be installed is not found in content file: %s")
ERR_INSTALL_FILE_DEC_FILE_ERROR = _("Could not obtain the TokenSpaceGuidCName and the PcdCName from the DEC files "
"that the package depends on for this pcd entry: TokenValue: %s Token: %s")
ERR_NOT_SUPPORTED_SA_MODULE = _("Stand-alone module distribution does not allow EDK 1 INF")
ERR_INSTALL_DIST_NOT_FOUND               = \
_("Distribution file to be installed is not found in current working directory or workspace: %s")
ERR_REPLACE_DIST_NOT_FOUND               = \
_("Distribution file for replace function was not found in the current working directory or workspace: %s")
ERR_DIST_FILENAME_ONLY_FOR_REPLACE_ORIG               = \
_("Only a distribution file name without a path is allowed for "
  "the distribution to be replaced during replace. Current given: '%s'.")
ERR_UNIPARSE_DBLQUOTE_UNMATCHED = \
_("Only Language entry can contain a couple of matched quote in one line")
ERR_UNIPARSE_NO_SECTION_EXIST = _("No PakcageDef or ModuleDef section exists in the UNI file.")
ERR_UNIPARSE_STRNAME_FORMAT_ERROR = _("The String Token Name %s must start with \"STR_\"")
ERR_UNIPARSE_SEP_LANGENTRY_LINE = _("Each <LangEntry> should be in a separate line :%s.")
ERR_UNIPARSE_MULTI_ENTRY_EXIST = \
_("There are same entries : %s in the UNI file, every kind of entry should be only one.")
ERR_UNIPARSE_ENTRY_ORDER_WRONG = \
_("The string entry order in UNI file should be <AbstractStrings>, <DescriptionStrings>, \
<BinaryAbstractStrings>, <BinaryDescriptionStrings>.")
ERR_UNIPARSE_STRTOKEN_FORMAT_ERROR = _("The String Token Type %s must be one of the '_PROMPT', '_HELP' and '_ERR_'.") 
ERR_UNIPARSE_LINEFEED_UNDER_EXIST = _("Line feed should not exist under this line: %s.")
ERR_UNIPARSE_LINEFEED_UP_EXIST = _("Line feed should not exist up this line: %s.")
ERR_UNI_MISS_STRING_ENTRY = _("String entry missed in this Entry, %s.")
ERR_UNI_MISS_LANGENTRY = _("Language entry missed in this Entry, %s.")
ERR_BINARY_HEADER_ORDER           = _("Binary header must follow the file header.")
ERR_NO_SOURCE_HEADER              = _("File header statement \"## @file\" must exist at the first place.")
ERR_UNI_FILE_SUFFIX_WRONG = _("The UNI file must have an extension of '.uni', '.UNI' or '.Uni'")
ERR_UNI_FILE_NAME_INVALID = _("The use of '..', '../' and './' in the UNI file is prohibited.")
ERR_UNI_SUBGUID_VALUE_DEFINE_DEC_NOT_FOUND = _("There are no DEC file to define the GUID value for \
this GUID CName: '%s'.")

#
# Expression error message
#
ERR_EXPR_RIGHT_PAREN            = \
_('Missing ")" in expression "%s".')
ERR_EXPR_FACTOR                 = \
_('"%s" is expected to be HEX, integer, macro, quoted string or PcdName in '
  'expression "%s".')
ERR_EXPR_STRING_ITEM            = \
_('"%s" is expected to be HEX, integer, macro, quoted string or PcdName in '
  'expression [%s].')
ERR_EXPR_EQUALITY               = \
_('"%s" is expected to be ==, EQ, != or NE  in expression "%s".')
ERR_EXPR_BOOLEAN                = \
_('The string "%s" in expression "%s" can not be recognized as a part of the logical expression.')
ERR_EXPR_EMPTY                  = _('Boolean value cannot be empty.')
ERR_EXPRESS_EMPTY               = _('Expression can not be empty.')
ERR_EXPR_LOGICAL                = \
_('The following is not a valid logical expression: "%s".')
ERR_EXPR_OR                     = _('The expression: "%s" must be encapsulated in open "(" and close ")" '
                                    'parenthesis when using | or ||.')
ERR_EXPR_RANGE                  = \
_('The following is not a valid range expression: "%s".')
ERR_EXPR_RANGE_FACTOR           = \
_('"%s" is expected to be HEX, integer in valid range expression "%s".')
ERR_EXPR_RANGE_DOUBLE_PAREN_NESTED = \
_('Double parentheses nested is not allowed in valid range expression: "%s".')
ERR_EXPR_RANGE_EMPTY            = _('Valid range can not be empty.')
ERR_EXPR_LIST_EMPTY             = _('Valid list can not be empty.')
ERR_PAREN_NOT_USED              = _('Parenthesis must be used on both sides of "OR", "AND" in valid range : %s.')
ERR_EXPR_LIST                   = \
_('The following is not a valid list expression: "%s".')


# DEC parser error message
#
ERR_DECPARSE_STATEMENT_EMPTY        = \
_('Must have at least one statement in section %s.')
ERR_DECPARSE_DEFINE_DEFINED         = \
_('%s already defined in define section.')
ERR_DECPARSE_DEFINE_SECNAME         = \
_('No arch and others can be followed for define section.')
ERR_DECPARSE_DEFINE_MULTISEC        = \
_('The DEC file does not allow multiple define sections.')
ERR_DECPARSE_DEFINE_REQUIRED        = \
_("Field [%s] is required in define section.")
ERR_DECPARSE_DEFINE_FORMAT          = \
_("Wrong define section format, must be KEY = Value.")
ERR_DECPARSE_DEFINE_UNKNOWKEY       = \
_("Unknown key [%s] in define section.")
ERR_DECPARSE_DEFINE_SPEC            = \
_("Specification value must be HEX numbers.")
ERR_DECPARSE_DEFINE_PKGNAME         = \
_("Package name must be AlphaNumeric characters.")
ERR_DECPARSE_DEFINE_PKGGUID         = \
_("GUID format error, must be HEX value with form 8-4-4-4-12.")
ERR_DECPARSE_DEFINE_PKGVERSION      = \
_("Version number must be decimal number.")
ERR_DECPARSE_DEFINE_PKGVUNI         = \
_("UNI file name format error or file does not exist.")
ERR_DECPARSE_INCLUDE                = \
_("Incorrect path: [%s].")
ERR_DECPARSE_LIBCLASS_SPLIT         = \
_("Library class format error, must be Libraryclass|Headerpath.")
ERR_DECPARSE_LIBCLASS_EMPTY         = \
_("Class name or file name must not be empty.")
ERR_DECPARSE_LIBCLASS_LIB           = \
_("Class name format error, must start with upper case letter followed with " 
  "zero or more alphanumeric characters.")
ERR_DECPARSE_LIBCLASS_PATH_EXT      = _("File name must be end with .h.")
ERR_DECPARSE_LIBCLASS_PATH_DOT      = _("Path must not include '..'.")
ERR_DECPARSE_LIBCLASS_PATH_EXIST    = _("File name [%s] does not exist.")
ERR_DECPARSE_PCD_CVAR_GUID          = \
_("TokenSpaceGuidCName must be valid C variable format.")
ERR_DECPARSE_PCD_SPLIT              = \
_("Incorrect PcdName. The format must be TokenSpaceGuidCName.PcdCName"
                                        "|PcdData|PcdType|Token.")
ERR_DECPARSE_PCD_NAME               = \
_("Incorrect PCD name. The correct format must be "
  "<TokenSpaceGuidCName>.<PcdCName>.")
ERR_DECPARSE_PCD_CVAR_PCDCNAME      = \
_("PcdCName must be valid C variable format.")
ERR_DECPARSE_PCD_TYPE               = \
_('Incorrect PCD data type. A PCD data type  must be one of '
  '"UINT8", "UINT16", "UINT32", "UINT64", "VOID*", "BOOLEAN".')
ERR_DECPARSE_PCD_VOID               = \
_("Incorrect  value [%s] of type [%s].  Value  must be printable and in the "
  "form of{...} for array, or ""..."" for string, or L""..."""
  "for unicode string.")
ERR_DECPARSE_PCD_VALUE_EMPTY        = \
_("Pcd value can not be empty.") 
ERR_DECPARSE_PCD_BOOL               = \
_("Invalid value [%s] of type [%s]; must be expression, TRUE, FALSE, 0 or 1.")
ERR_DECPARSE_PCD_INT                = _("Incorrect value [%s] of type [%s]."\
" Value must be a hexadecimal, decimal or octal in C language format.")
ERR_DECPARSE_PCD_INT_NEGTIVE        = _("Incorrect value [%s] of type [%s];"
                                        " must not be signed number.")
ERR_DECPARSE_PCD_INT_EXCEED         = _("Incorrect value [%s] of type [%s]; "
                                    "the number is too long for this type.")
ERR_DECPARSE_PCD_FEATUREFLAG        = \
_("PcdFeatureFlag only allow BOOLEAN type.")
ERR_DECPARSE_PCD_TOKEN              = \
_("An incorrect PCD token found: [%s].  "
  "It must start with 0x followed by 1 - 8 hexadecimal. ")
ERR_DECPARSE_PCD_TOKEN_INT          = _("Incorrect token number [%s].  "
     "This token number exceeds the maximal value of unsigned 32.")
ERR_DECPARSE_PCD_TOKEN_UNIQUE       = _("Token number must be unique to the token space: %s.")
ERR_DECPARSE_CGUID                  = \
_("No GUID name or value specified, must be <CName> = <GuidValueInCFormat>.")
ERR_DECPARSE_CGUID_NAME             = \
_("No GUID name specified, must be <CName> = <GuidValueInCFormat>.")
ERR_DECPARSE_CGUID_GUID             = \
_("No GUID value specified, must be <CName> = <GuidValueInCFormat>.")
ERR_DECPARSE_CGUID_GUIDFORMAT       = \
_("Incorrect GUID value format, must be <GuidValueInCFormat:" 
  "{8,4,4,{2,2,2,2,2,2,2,2}}>.")
ERR_DECPARSE_CGUID_NOT_FOUND = _("Unable to find the GUID value of this GUID CName : '%s'.")
ERR_DECPARSE_FILEOPEN               = _("Unable to open: [%s].")
ERR_DECPARSE_SECTION_EMPTY          = _("Empty sections are not allowed.")
ERR_DECPARSE_SECTION_UE             = _("Incorrect UserExtentions format. "
                            "Must be UserExtenxions.UserId.IdString[.Arch]+.")
ERR_DECPARSE_SECTION_UE_USERID      = _("Invalid UserId, must be underscore" 
                                        "or alphanumeric characters.")
ERR_DECPARSE_SECTION_UE_IDSTRING    = \
    _("Incorrect IdString, must be \" ... \".")
ERR_DECPARSE_ARCH                   = \
_("Unknown arch, must be 'common' or start with upper case letter followed by"
                            " zero or more upper case letters and numbers.")
ERR_DECPARSE_SECTION_COMMA          = _("Section cannot end with comma.")
ERR_DECPARSE_SECTION_COMMON         = \
_("'COMMON' must not be used with specific ARCHs in the same section.")
ERR_DECPARSE_SECTION_IDENTIFY       = \
_("Section header must start with and end with brackets[].")
ERR_DECPARSE_SECTION_SUBEMPTY       = \
_("Missing a sub-section name in section: [%s]. "
  "All sub-sections need to have names. ")
ERR_DECPARSE_SECTION_SUBTOOMANY     = _("Too many DOT splits in [%s].")
ERR_DECPARSE_SECTION_UNKNOW         = _("Section name [%s] unknown.")
ERR_DECPARSE_SECTION_FEATUREFLAG    = \
_("[%s] must not be in the same section as other types of PCD.")
ERR_DECPARSE_MACRO_PAIR             = _("No macro name/value given.")
ERR_DECPARSE_MACRO_NAME             = _("No macro name given.")
ERR_DECPARSE_MACRO_NAME_UPPER       = \
_("Macro name must start with upper case letter followed "
"by zero or more upper case letters or numbers.  Current macro name is: [%s].")
ERR_DECPARSE_SECTION_NAME           = \
_('Cannot mix different section names %s.')
ERR_DECPARSE_BACKSLASH              = \
_('Backslash must be the last character on a line and '
                                        'preceded by a space character.')
ERR_DECPARSE_BACKSLASH_EMPTY        = \
_('Empty line after previous line that has backslash is not allowed.')
ERR_DECPARSE_REDEFINE               = _(
    "\"%s\" already defined in line %d.")
ERR_DECPARSE_MACRO_RESOLVE          = _("Macro %s in %s cannot be resolved.")
ERR_DECPARSE_UE_DUPLICATE           = \
    _("Duplicated UserExtensions header found.")
ERR_DECPARSE_PCDERRORMSG_MISS_VALUE_SPLIT = \
    _("Missing '|' between Pcd's error code and Pcd's error message.")
ERR_DECPARSE_PCD_MISS_ERRORMSG = \
    _("Missing Pcd's error message.")
ERR_DECPARSE_PCD_UNMATCHED_ERRORCODE = \
    _("There is no error message matched with this Pcd error code : %s in both DEC and UNI file.")
ERR_DECPARSE_PCD_NODEFINED = _("The PCD : %s used in the Expression is undefined.")
#
# Used to print the current line content which cause error raise.
# Be attached to the end of every error message above.
#
ERR_DECPARSE_LINE                   = _(" Parsing line: \"%s\".")

#
# Warning related strings.
#
WRN_PACKAGE_EXISTED       = _(
    "A package with this GUID and Version already exists: "
    "GUID %s, Version %s.")
WRN_MODULE_EXISTED        = _("This module already exists: %s")
WRN_FILE_EXISTED          = _("This file already exists: %s")
WRN_FILE_NOT_OVERWRITTEN  = \
_("This file already exist and cannot be overwritten: %s")
WRN_DIST_PKG_INSTALLED = _("This distribution package has previously been installed.")
WRN_DIST_NOT_FOUND         = _(
    "Distribution is not found at location %s")
WRN_MULTI_PCD_RANGES      = _(
    "A PCD can only have one type of @ValidRange, @ValidList, and @Expression comment")
WRN_MULTI_PCD_VALIDVALUE  = _(
    "A PCD can only have one of @ValidList comment")
WRN_MULTI_PCD_PROMPT      = _(
    "A PCD can only have one of @Prompt comment")
WRN_MISSING_USAGE                = _("Missing usage")
WRN_INVALID_GUID_TYPE            = _("This is and incorrect Guid type: %s")
WRN_MISSING_GUID_TYPE            = _("Missing Guid Type")
WRN_INVALID_USAGE                = _("This is an incorrect Usage: %s")
WRN_INF_PARSER_MODULE_INVALID_HOB_TYPE = \
    _("This is an incorrect HOB type: %s")
WRN_INF_PARSER_MODULE_INVALID_EVENT_TYPE = \
    _("This is an incorrect EVENT type: %s")
WRN_INF_PARSER_MODULE_INVALID_BOOTMODE_TYPE = \
    _("This is an incorrect BOOTMODE type: %s")
WRN_INVALID_MODULE_TYPE = \
    _("This is an incorrect Module type: %s")  
WRN_MODULE_PARSE_FAILED = \
    _("Parsing of this module did not complete correctly: %s.")
WRN_EDK1_INF_FOUND = \
    _("EDK 1 module file found: %s")
WRN_INVALID_COPYRIGHT = \
    _("Copyright information is not right")
WARN_SPECIAL_SECTION_LOCATION_WRONG = _("Warning. A special section should be "
                                        "at the end of a file or at the end of a section.")
WARN_INSTALLED_PACKAGE_NOT_FOUND = \
    _("File not found. The DEC file for a package cannot be found in GUID/Version/Install path: %s %s %s")
WARN_CUSTOMPATH_OVERRIDE_USEGUIDEDPATH = \
    _("option selection of --custom-path will override the option --use-guided-paths")

#
# Help related strings.
#
HLP_PRINT_DEBUG_INFO             = _(
    "Print DEBUG statements, where DEBUG_LEVEL is 0-9")
HLP_PRINT_INFORMATIONAL_STATEMENT = _("Print informational statements")
HLP_RETURN_NO_DISPLAY            = _(
    "Returns only the exit code, informational and error messages are" 
    " not displayed")
HLP_RETURN_AND_DISPLAY           = _(
    "Returns the exit code and displays  error messages only")
HLP_SPECIFY_PACKAGE_NAME_INSTALL = _(
    "Specify the UEFI Distribution Package filename to install")
HLP_SPECIFY_PACKAGE_NAME_CREATE  = _(
    "Specify the UEFI Distribution Package filename to create")
HLP_SPECIFY_PACKAGE_NAME_REMOVE  = _(
    "Specify the UEFI Distribution Package filename to remove")
HLP_SPECIFY_TEMPLATE_NAME_CREATE = _(
    "Specify Package Information Data filename to create package")
HLP_SPECIFY_DEC_NAME_CREATE      = _(
    "Specify dec file names to create package")
HLP_SPECIFY_INF_NAME_CREATE      = _(
    "Specify inf file names to create package")
HLP_LIST_DIST_INSTALLED      = _(
    "List the UEFI Distribution Packages that have been installed")
HLP_NO_SUPPORT_GUI               = _(
    "Starting the tool in graphical mode is not supported in this version")
HLP_DISABLE_PROMPT               = _(
    "Disable user prompts for removing modified files. Valid only when -r is present")
HLP_CUSTOM_PATH_PROMPT           = _(
    "Enable user prompting for alternate installation directories")
HLP_SKIP_LOCK_CHECK              = _(
    "Skip the check for multiple instances")
HLP_SPECIFY_PACKAGE_NAME_REPLACE  = _(
    "Specify the UEFI Distribution Package file name to replace the existing file name")
HLP_SPECIFY_PACKAGE_NAME_TO_BE_REPLACED = _(
    "Specify the UEFI Distribution Package file name to be replaced")
HLP_USE_GUIDED_PATHS = _(
    "Install packages to the following directory path by default: <PackageName>_<PACKAGE_GUID>_<PACKAGE_VERSION>")
