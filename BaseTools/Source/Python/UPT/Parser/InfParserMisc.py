## @file
# This file contained the miscellaneous functions for INF parser
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
InfParserMisc
'''

##
# Import Modules
#
import re


from Library import DataType as DT


from Library.StringUtils import gMACRO_PATTERN
from Library.StringUtils import ReplaceMacro
from Object.Parser.InfMisc import ErrorInInf
from Logger.StringTable import ERR_MARCO_DEFINITION_MISS_ERROR

#
# Global variable
#

#
# Sections can exist in INF file
#
gINF_SECTION_DEF = {
       DT.TAB_UNKNOWN.upper()          : DT.MODEL_UNKNOWN,
       DT.TAB_HEADER.upper()           : DT.MODEL_META_DATA_FILE_HEADER,
       DT.TAB_INF_DEFINES.upper()      : DT.MODEL_META_DATA_DEFINE,
       DT.TAB_BUILD_OPTIONS.upper()    : DT.MODEL_META_DATA_BUILD_OPTION,
       DT.TAB_LIBRARY_CLASSES.upper()  : DT.MODEL_EFI_LIBRARY_CLASS,
       DT.TAB_PACKAGES.upper()         : DT.MODEL_META_DATA_PACKAGE,
       DT.TAB_INF_FIXED_PCD.upper()    : DT.MODEL_PCD_FIXED_AT_BUILD,
       DT.TAB_INF_PATCH_PCD.upper()    : DT.MODEL_PCD_PATCHABLE_IN_MODULE,
       DT.TAB_INF_FEATURE_PCD.upper()  : DT.MODEL_PCD_FEATURE_FLAG,
       DT.TAB_INF_PCD_EX.upper()       : DT.MODEL_PCD_DYNAMIC_EX,
       DT.TAB_INF_PCD.upper()          : DT.MODEL_PCD_DYNAMIC,
       DT.TAB_SOURCES.upper()          : DT.MODEL_EFI_SOURCE_FILE,
       DT.TAB_GUIDS.upper()            : DT.MODEL_EFI_GUID,
       DT.TAB_PROTOCOLS.upper()        : DT.MODEL_EFI_PROTOCOL,
       DT.TAB_PPIS.upper()             : DT.MODEL_EFI_PPI,
       DT.TAB_DEPEX.upper()            : DT.MODEL_EFI_DEPEX,
       DT.TAB_BINARIES.upper()         : DT.MODEL_EFI_BINARY_FILE,
       DT.TAB_USER_EXTENSIONS.upper()  : DT.MODEL_META_DATA_USER_EXTENSION
       #
       # EDK1 section
       # TAB_NMAKE.upper()            : MODEL_META_DATA_NMAKE
       #
       }

## InfExpandMacro
#
# Expand MACRO definition with MACROs defined in [Defines] section and specific section.
# The MACROs defined in specific section has high priority and will be expanded firstly.
#
# @param LineInfo      Contain information of FileName, LineContent, LineNo
# @param GlobalMacros  MACROs defined in INF [Defines] section
# @param SectionMacros MACROs defined in INF specific section
# @param Flag          If the flag set to True, need to skip macros in a quoted string
#
def InfExpandMacro(Content, LineInfo, GlobalMacros=None, SectionMacros=None, Flag=False):
    if GlobalMacros is None:
        GlobalMacros = {}
    if SectionMacros is None:
        SectionMacros = {}

    FileName = LineInfo[0]
    LineContent = LineInfo[1]
    LineNo = LineInfo[2]

    # Don't expand macros in comments
    if LineContent.strip().startswith("#"):
        return Content

    NewLineInfo = (FileName, LineNo, LineContent)

    #
    # First, replace MARCOs with value defined in specific section
    #
    Content = ReplaceMacro (Content,
                            SectionMacros,
                            False,
                            (LineContent, LineNo),
                            FileName,
                            Flag)
    #
    # Then replace MARCOs with value defined in [Defines] section
    #
    Content = ReplaceMacro (Content,
                            GlobalMacros,
                            False,
                            (LineContent, LineNo),
                            FileName,
                            Flag)

    MacroUsed = gMACRO_PATTERN.findall(Content)
    #
    # no macro found in String, stop replacing
    #
    if len(MacroUsed) == 0:
        return Content
    else:
        for Macro in MacroUsed:
            gQuotedMacro = re.compile(r".*\".*\$\(%s\).*\".*"%(Macro))
            if not gQuotedMacro.match(Content):
                #
                # Still have MACROs can't be expanded.
                #
                ErrorInInf (ERR_MARCO_DEFINITION_MISS_ERROR,
                            LineInfo=NewLineInfo)

    return Content


## IsBinaryInf
#
# Judge whether the INF file is Binary INF or Common INF
#
# @param FileLineList     A list contain all INF file content.
#
def IsBinaryInf(FileLineList):
    if not FileLineList:
        return False

    ReIsSourcesSection = re.compile(r"^\s*\[Sources.*\]\s.*$", re.IGNORECASE)
    ReIsBinarySection = re.compile(r"^\s*\[Binaries.*\]\s.*$", re.IGNORECASE)
    BinarySectionFoundFlag = False

    for Line in FileLineList:
        if ReIsSourcesSection.match(Line):
            return False
        if ReIsBinarySection.match(Line):
            BinarySectionFoundFlag = True

    if BinarySectionFoundFlag:
        return True

    return False


## IsLibInstanceInfo
#
# Judge whether the string contain the information of ## @LIB_INSTANCES.
#
# @param  String
#
# @return Flag
#
def IsLibInstanceInfo(String):
    ReIsLibInstance = re.compile(r"^\s*##\s*@LIB_INSTANCES\s*$")
    if ReIsLibInstance.match(String):
        return True
    else:
        return False


## IsAsBuildOptionInfo
#
# Judge whether the string contain the information of ## @ASBUILD.
#
# @param  String
#
# @return Flag
#
def IsAsBuildOptionInfo(String):
    ReIsAsBuildInstance = re.compile(r"^\s*##\s*@AsBuilt\s*$")
    if ReIsAsBuildInstance.match(String):
        return True
    else:
        return False


class InfParserSectionRoot(object):
    def __init__(self):
        #
        # Macros defined in [Define] section are file scope global
        #
        self.FileLocalMacros = {}

        #
        # Current Section Header content.
        #
        self.SectionHeaderContent = []

        #
        # Last time Section Header content.
        #
        self.LastSectionHeaderContent = []

        self.FullPath = ''

        self.InfDefSection              = None
        self.InfBuildOptionSection      = None
        self.InfLibraryClassSection     = None
        self.InfPackageSection          = None
        self.InfPcdSection              = None
        self.InfSourcesSection          = None
        self.InfUserExtensionSection    = None
        self.InfProtocolSection         = None
        self.InfPpiSection              = None
        self.InfGuidSection             = None
        self.InfDepexSection            = None
        self.InfPeiDepexSection         = None
        self.InfDxeDepexSection         = None
        self.InfSmmDepexSection         = None
        self.InfBinariesSection         = None
        self.InfHeader                  = None
        self.InfSpecialCommentSection   = None
