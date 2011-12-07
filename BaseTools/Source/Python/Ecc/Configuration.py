## @file
# This file is used to define class Configuration
#
# Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import os
import Common.EdkLogger as EdkLogger
from Common.DataType import *
from Common.String import *

## Configuration
#
# This class is used to define all items in configuration file
#
# @param Filename:  The name of configuration file, the default is config.ini
#
class Configuration(object):
    def __init__(self, Filename):
        self.Filename = Filename

        self.Version = 0.1

        ## Identify to if check all items
        # 1 - Check all items and ignore all other detailed items
        # 0 - Not check all items, the tool will go through all other detailed items to decide to check or not
        #
        self.CheckAll = 0

        ## Identify to if automatically correct mistakes
        # 1 - Automatically correct
        # 0 - Not automatically correct
        # Only the following check points can be automatically corrected, others not listed below are not supported even it is 1
        #
        # GeneralCheckTab
        # GeneralCheckIndentation
        # GeneralCheckLine
        # GeneralCheckCarriageReturn
        # SpaceCheckAll
        #
        self.AutoCorrect = 0

        # List customized Modifer here, split with ','
        # Defaultly use the definition in class DataType
        self.ModifierList = MODIFIER_LIST

        ## General Checking
        self.GeneralCheckAll = 0

        # Check whether NO Tab is used, replaced with spaces
        self.GeneralCheckNoTab = 1
        # The width of Tab
        self.GeneralCheckTabWidth = 2
        # Check whether the indentation is followed coding style
        self.GeneralCheckIndentation = 1
        # The width of indentation
        self.GeneralCheckIndentationWidth = 2
        # Check whether no line is exceeding defined widty
        self.GeneralCheckLine = 1
        # The width of a line
        self.GeneralCheckLineWidth = 120
        # Check whether no use of _asm in the source file
        self.GeneralCheckNo_Asm = 1
        # Check whether no use of "#progma" in source file except "#pragma pack(#)".
        self.GeneralCheckNoProgma = 1
        # Check whether there is a carriage return at the end of the file
        self.GeneralCheckCarriageReturn = 1
        # Check whether the file exists
        self.GeneralCheckFileExistence = 1
        # Check whether file has non ACSII char
        self.GeneralCheckNonAcsii = 1

        ## Space Checking
        self.SpaceCheckAll = 1

        ## Predicate Expression Checking
        self.PredicateExpressionCheckAll = 0

        # Check whether Boolean values, variable type BOOLEAN not use explicit comparisons to TRUE or FALSE
        self.PredicateExpressionCheckBooleanValue = 1
        # Check whether Non-Boolean comparisons use a compare operator (==, !=, >, < >=, <=).
        self.PredicateExpressionCheckNonBooleanOperator = 1
        # Check whether a comparison of any pointer to zero must be done via the NULL type
        self.PredicateExpressionCheckComparisonNullType = 1

        ## Headers Checking
        self.HeaderCheckAll = 0

        # Check whether File header exists
        self.HeaderCheckFile = 1
        # Check whether Function header exists
        self.HeaderCheckFunction = 1
        # Check whether Meta data File header Comment End with '##'
        self.HeaderCheckFileCommentEnd = 1
        # Check whether C File header Comment content start with two spaces
        self.HeaderCheckCFileCommentStartSpacesNum = 1
        # Check whether C File header Comment's each reference at list should begin with a bullet character '-'
        self.HeaderCheckCFileCommentReferenceFormat = 1
        # Check whether C File header Comment have the License immediately after the ""Copyright"" line
        self.HeaderCheckCFileCommentLicenseFormat = 1
  
        ## C Function Layout Checking
        self.CFunctionLayoutCheckAll = 0

        # Check whether return type exists and in the first line
        self.CFunctionLayoutCheckReturnType = 1
        # Check whether any optional functional modifiers exist and next to the return type
        self.CFunctionLayoutCheckOptionalFunctionalModifier = 1
        # Check whether the next line contains the function name, left justified, followed by the beginning of the parameter list
        # Check whether the closing parenthesis is on its own line and also indented two spaces
        self.CFunctionLayoutCheckFunctionName = 1
        # Check whether the function prototypes in include files have the same form as function definitions
        self.CFunctionLayoutCheckFunctionPrototype = 1
        # Check whether the body of a function is contained by open and close braces that must be in the first column
        self.CFunctionLayoutCheckFunctionBody = 1
        # Check whether the data declarations is the first code in a module.
        self.CFunctionLayoutCheckDataDeclaration = 1
        # Check whether no initialization of a variable as part of its declaration
        self.CFunctionLayoutCheckNoInitOfVariable = 1
        # Check whether no use of STATIC for functions
        self.CFunctionLayoutCheckNoStatic = 1

        ## Include Files Checking
        self.IncludeFileCheckAll = 0

        #Check whether having include files with same name
        self.IncludeFileCheckSameName = 1
        # Check whether all include file contents is guarded by a #ifndef statement.
        # the #ifndef must be the first line of code following the file header comment
        # the #endif must appear on the last line in the file
        self.IncludeFileCheckIfndefStatement = 1
        # Check whether include files contain only public or only private data
        # Check whether include files NOT contain code or define data variables
        self.IncludeFileCheckData = 1

        ## Declarations and Data Types Checking
        self.DeclarationDataTypeCheckAll = 0

        # Check whether no use of int, unsigned, char, void, static, long in any .c, .h or .asl files.
        self.DeclarationDataTypeCheckNoUseCType = 1
        # Check whether the modifiers IN, OUT, OPTIONAL, and UNALIGNED are used only to qualify arguments to a function and should not appear in a data type declaration
        self.DeclarationDataTypeCheckInOutModifier = 1
        # Check whether the EFIAPI modifier should be used at the entry of drivers, events, and member functions of protocols
        self.DeclarationDataTypeCheckEFIAPIModifier = 1
        # Check whether Enumerated Type has a 'typedef' and the name is capital
        self.DeclarationDataTypeCheckEnumeratedType = 1
        # Check whether Structure Type has a 'typedef' and the name is capital
        self.DeclarationDataTypeCheckStructureDeclaration = 1
        # Check whether having same Structure
        self.DeclarationDataTypeCheckSameStructure = 1
        # Check whether Union Type has a 'typedef' and the name is capital
        self.DeclarationDataTypeCheckUnionType = 1

        ## Naming Conventions Checking
        self.NamingConventionCheckAll = 0

        # Check whether only capital letters are used for #define declarations
        self.NamingConventionCheckDefineStatement = 1
        # Check whether only capital letters are used for typedef declarations
        self.NamingConventionCheckTypedefStatement = 1
        # Check whether the #ifndef at the start of an include file uses both prefix and postfix underscore characters, '_'.
        self.NamingConventionCheckIfndefStatement = 1
        # Rule for path name, variable name and function name
        # 1. First character should be upper case
        # 2. Existing lower case in a word
        # 3. No space existence
        # Check whether the path name followed the rule
        self.NamingConventionCheckPathName = 1
        # Check whether the variable name followed the rule
        self.NamingConventionCheckVariableName = 1
        # Check whether the function name followed the rule
        self.NamingConventionCheckFunctionName = 1
        # Check whether NO use short variable name with single character
        self.NamingConventionCheckSingleCharacterVariable = 1

        ## Doxygen Checking
        self.DoxygenCheckAll = 0

        # Check whether the file headers are followed Doxygen special documentation blocks in section 2.3.5
        self.DoxygenCheckFileHeader = 1
        # Check whether the function headers are followed Doxygen special documentation blocks in section 2.3.5
        self.DoxygenCheckFunctionHeader = 1
        # Check whether the first line of text in a comment block is a brief description of the element being documented.
        # The brief description must end with a period.
        self.DoxygenCheckCommentDescription = 1
        # Check whether comment lines with '///< ... text ...' format, if it is used, it should be after the code section.
        self.DoxygenCheckCommentFormat = 1
        # Check whether only Doxygen commands allowed to mark the code are @bug and @todo.
        self.DoxygenCheckCommand = 1

        ## Meta-Data File Processing Checking
        self.MetaDataFileCheckAll = 0

        # Check whether each file defined in meta-data exists
        self.MetaDataFileCheckPathName = 1
        # Generate a list for all files defined in meta-data files
        self.MetaDataFileCheckGenerateFileList = 1
        # The path of log file
        self.MetaDataFileCheckPathOfGenerateFileList = 'File.log'
        # Check whether all Library Instances defined for a given module (or dependent library instance) match the module's type.
        # Each Library Instance must specify the Supported Module Types in its INF file,
        # and any module specifying the library instance must be one of the supported types.
        self.MetaDataFileCheckLibraryInstance = 1
        # Check whether a Library Instance has been defined for all dependent library classes
        self.MetaDataFileCheckLibraryInstanceDependent = 1
        # Check whether the Library Instances specified by the LibraryClasses sections are listed in order of dependencies
        self.MetaDataFileCheckLibraryInstanceOrder = 1
        # Check whether the unnecessary inclusion of library classes in the INF file
        self.MetaDataFileCheckLibraryNoUse = 1
        # Check whether an INF file is specified in the FDF file, but not in the DSC file, then the INF file must be for a Binary module only
        self.MetaDataFileCheckBinaryInfInFdf = 1
        # Not to report error and warning related OS include file such as "windows.h" and "stdio.h"
        # Check whether a PCD is set in a DSC file or the FDF file, but not in both.
        self.MetaDataFileCheckPcdDuplicate = 1
        # Check whether PCD settings in the FDF file can only be related to flash.
        self.MetaDataFileCheckPcdFlash = 1
        # Check whether PCDs used in INF files but not specified in DSC or FDF files
        self.MetaDataFileCheckPcdNoUse = 1
        # Check whether having duplicate guids defined for Guid/Protocol/Ppi
        self.MetaDataFileCheckGuidDuplicate = 1
        # Check whether all files under module directory are described in INF files
        self.MetaDataFileCheckModuleFileNoUse = 1
        # Check whether the PCD is correctly used in C function via its type
        self.MetaDataFileCheckPcdType = 1
        # Check whether there are FILE_GUID duplication among different INF files
        self.MetaDataFileCheckModuleFileGuidDuplication = 1

        #
        # The check points in this section are reserved
        #
        # GotoStatementCheckAll = 0
        #
        self.SpellingCheckAll = 0

        # The directory listed here will not be parsed, split with ','
        self.SkipDirList = []

        # A list for binary file ext name
        self.BinaryExtList = []

        self.ParseConfig()

    def ParseConfig(self):
        Filepath = os.path.normpath(self.Filename)
        if not os.path.isfile(Filepath):
            ErrorMsg = "Can't find configuration file '%s'" % Filepath
            EdkLogger.error("Ecc", EdkLogger.ECC_ERROR, ErrorMsg, File = Filepath)

        LineNo = 0
        for Line in open(Filepath, 'r'):
            LineNo = LineNo + 1
            Line = CleanString(Line)
            if Line != '':
                List = GetSplitValueList(Line, TAB_EQUAL_SPLIT)
                if List[0] not in self.__dict__:
                    ErrorMsg = "Invalid configuration option '%s' was found" % List[0]
                    EdkLogger.error("Ecc", EdkLogger.ECC_ERROR, ErrorMsg, File = Filepath, Line = LineNo)
                if List[0] == 'ModifierList':
                    List[1] = GetSplitValueList(List[1], TAB_COMMA_SPLIT)
                if List[0] == 'MetaDataFileCheckPathOfGenerateFileList' and List[1] == "":
                    continue
                if List[0] == 'SkipDirList':
                    List[1] = GetSplitValueList(List[1], TAB_COMMA_SPLIT)
                if List[0] == 'BinaryExtList':
                    List[1] = GetSplitValueList(List[1], TAB_COMMA_SPLIT)
                self.__dict__[List[0]] = List[1]

    def ShowMe(self):
        print self.Filename
        for Key in self.__dict__.keys():
            print Key, '=', self.__dict__[Key]
