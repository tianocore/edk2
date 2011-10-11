## @file
# This file is used to parse meta files
#
# Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
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
import time
import copy

import Common.EdkLogger as EdkLogger
from CommonDataClass.DataClass import *
from Common.DataType import *
from Common.String import *
from Common.Misc import Blist, GuidStructureStringToGuidString, CheckPcdDatum

## Base class of parser
#
#  This class is used for derivation purpose. The specific parser for one kind
# type file must derive this class and implement some public interfaces.
#
#   @param      FilePath        The path of platform description file
#   @param      FileType        The raw data of DSC file
#   @param      Table           Database used to retrieve module/package information
#   @param      Macros          Macros used for replacement in file
#   @param      Owner           Owner ID (for sub-section parsing)
#   @param      From            ID from which the data comes (for !INCLUDE directive)
#
class MetaFileParser(object):
    # data type (file content) for specific file type
    DataType = {}

    # Parser objects used to implement singleton
    MetaFiles = {}

    ## Factory method
    #
    # One file, one parser object. This factory method makes sure that there's
    # only one object constructed for one meta file.
    #
    #   @param  Class           class object of real AutoGen class
    #                           (InfParser, DecParser or DscParser)
    #   @param  FilePath        The path of meta file
    #   @param  *args           The specific class related parameters
    #   @param  **kwargs        The specific class related dict parameters
    #
    def __new__(Class, FilePath, *args, **kwargs):
        if FilePath in Class.MetaFiles:
            return Class.MetaFiles[FilePath]
        else:
            ParserObject = super(MetaFileParser, Class).__new__(Class)
            Class.MetaFiles[FilePath] = ParserObject
            return ParserObject

    ## Constructor of MetaFileParser
    #
    #  Initialize object of MetaFileParser
    #
    #   @param      FilePath        The path of platform description file
    #   @param      FileType        The raw data of DSC file
    #   @param      Table           Database used to retrieve module/package information
    #   @param      Macros          Macros used for replacement in file
    #   @param      Owner           Owner ID (for sub-section parsing)
    #   @param      From            ID from which the data comes (for !INCLUDE directive)
    #
    def __init__(self, FilePath, FileType, Table, Macros=None, Owner=-1, From=-1):
        # prevent re-initialization
        if hasattr(self, "_Table"):
            return
        self._Table = Table
        self._FileType = FileType
        self.MetaFile = FilePath
        self._FileDir = os.path.dirname(self.MetaFile)
        self._Macros = copy.copy(Macros)
        self._Macros["WORKSPACE"] = os.environ["WORKSPACE"]

        # for recursive parsing
        self._Owner = Owner
        self._From = From

        # parsr status for parsing
        self._Content = None
        self._ValueList = ['', '', '', '', '']
        self._Scope = []
        self._LineIndex = 0
        self._CurrentLine = ''
        self._SectionType = MODEL_UNKNOWN
        self._SectionName = ''
        self._InSubsection = False
        self._SubsectionType = MODEL_UNKNOWN
        self._SubsectionName = ''
        self._LastItem = -1
        self._Enabled = 0
        self._Finished = False

    ## Store the parsed data in table
    def _Store(self, *Args):
        return self._Table.Insert(*Args)

    ## Virtual method for starting parse
    def Start(self):
        raise NotImplementedError

    ## Set parsing complete flag in both class and table
    def _Done(self):
        self._Finished = True
        ## Do not set end flag when processing included files
        if self._From == -1:
            self._Table.SetEndFlag()

    ## Return the table containg parsed data
    #
    #   If the parse complete flag is not set, this method will try to parse the
    # file before return the table
    #
    def _GetTable(self):
        if not self._Finished:
            self.Start()
        return self._Table

    ## Get the parse complete flag
    def _GetFinished(self):
        return self._Finished

    ## Set the complete flag
    def _SetFinished(self, Value):
        self._Finished = Value

    ## Use [] style to query data in table, just for readability
    #
    #   DataInfo = [data_type, scope1(arch), scope2(platform,moduletype)]
    #
    def __getitem__(self, DataInfo):
        if type(DataInfo) != type(()):
            DataInfo = (DataInfo,)
        return self.Table.Query(*DataInfo)

    ## Data parser for the common format in different type of file
    #
    #   The common format in the meatfile is like
    #
    #       xxx1 | xxx2 | xxx3
    #
    def _CommonParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT)
        self._ValueList[0:len(TokenList)] = TokenList

    ## Data parser for the format in which there's path
    #
    #   Only path can have macro used. So we need to replace them before use.
    #
    def _PathParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT)
        self._ValueList[0:len(TokenList)] = TokenList
        if len(self._Macros) > 0:
            for Index in range(0, len(self._ValueList)):
                Value = self._ValueList[Index]
                if Value == None or Value == '':
                    continue
                self._ValueList[Index] = NormPath(Value, self._Macros)

    ## Skip unsupported data
    def _Skip(self):
        EdkLogger.warn("Parser", "Unrecognized content", File=self.MetaFile,
                        Line=self._LineIndex+1, ExtraData=self._CurrentLine);
        self._ValueList[0:1] = [self._CurrentLine]

    ## Section header parser
    #
    #   The section header is always in following format:
    #
    #       [section_name.arch<.platform|module_type>]
    #
    def _SectionHeaderParser(self):
        self._Scope = []
        self._SectionName = ''
        ArchList = set()
        for Item in GetSplitValueList(self._CurrentLine[1:-1], TAB_COMMA_SPLIT):
            if Item == '':
                continue
            ItemList = GetSplitValueList(Item, TAB_SPLIT)
            # different section should not mix in one section
            if self._SectionName != '' and self._SectionName != ItemList[0].upper():
                EdkLogger.error('Parser', FORMAT_INVALID, "Different section names in the same section",
                                File=self.MetaFile, Line=self._LineIndex+1, ExtraData=self._CurrentLine)
            self._SectionName = ItemList[0].upper()
            if self._SectionName in self.DataType:
                self._SectionType = self.DataType[self._SectionName]
            else:
                self._SectionType = MODEL_UNKNOWN
                EdkLogger.warn("Parser", "Unrecognized section", File=self.MetaFile,
                                Line=self._LineIndex+1, ExtraData=self._CurrentLine)
            # S1 is always Arch
            if len(ItemList) > 1:
                S1 = ItemList[1].upper()
            else:
                S1 = 'COMMON'
            ArchList.add(S1)
            # S2 may be Platform or ModuleType
            if len(ItemList) > 2:
                S2 = ItemList[2].upper()
            else:
                S2 = 'COMMON'
            self._Scope.append([S1, S2])

        # 'COMMON' must not be used with specific ARCHs at the same section
        if 'COMMON' in ArchList and len(ArchList) > 1:
            EdkLogger.error('Parser', FORMAT_INVALID, "'common' ARCH must not be used with specific ARCHs",
                            File=self.MetaFile, Line=self._LineIndex+1, ExtraData=self._CurrentLine)

    ## [defines] section parser
    def _DefineParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_EQUAL_SPLIT, 1)
        self._ValueList[0:len(TokenList)] = TokenList
        if self._ValueList[1] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No value specified",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)

    ## DEFINE name=value parser
    def _MacroParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, ' ', 1)
        MacroType = TokenList[0]
        if len(TokenList) < 2 or TokenList[1] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No macro name/value given",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        TokenList = GetSplitValueList(TokenList[1], TAB_EQUAL_SPLIT, 1)
        if TokenList[0] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No macro name given",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)

        # Macros defined in the command line override ones defined in the meta-data file
        if not TokenList[0] in self._Macros:
            if len(TokenList) == 1:
                self._Macros[TokenList[0]] = ''
            else:
                # keep the macro definition for later use
                self._Macros[TokenList[0]] = ReplaceMacro(TokenList[1], self._Macros, False)

        return TokenList[0], self._Macros[TokenList[0]]

    ## [BuildOptions] section parser
    def _BuildOptionParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_EQUAL_SPLIT, 1)
        TokenList2 = GetSplitValueList(TokenList[0], ':', 1)
        if len(TokenList2) == 2:
            self._ValueList[0] = TokenList2[0]  # toolchain family
            self._ValueList[1] = TokenList2[1]  # keys
        else:
            self._ValueList[1] = TokenList[0]
        if len(TokenList) == 2:                 # value
            self._ValueList[2] = ReplaceMacro(TokenList[1], self._Macros)

        if self._ValueList[1].count('_') != 4:
            EdkLogger.error(
                'Parser',
                FORMAT_INVALID,
                "'%s' must be in format of <TARGET>_<TOOLCHAIN>_<ARCH>_<TOOL>_FLAGS" % self._ValueList[1],
                ExtraData=self._CurrentLine,
                File=self.MetaFile,
                Line=self._LineIndex+1
                )

    _SectionParser  = {}
    Table           = property(_GetTable)
    Finished        = property(_GetFinished, _SetFinished)


## INF file parser class
#
#   @param      FilePath        The path of platform description file
#   @param      FileType        The raw data of DSC file
#   @param      Table           Database used to retrieve module/package information
#   @param      Macros          Macros used for replacement in file
#
class InfParser(MetaFileParser):
    # INF file supported data types (one type per section)
    DataType = {
        TAB_UNKNOWN.upper() : MODEL_UNKNOWN,
        TAB_INF_DEFINES.upper() : MODEL_META_DATA_HEADER,
        TAB_BUILD_OPTIONS.upper() : MODEL_META_DATA_BUILD_OPTION,
        TAB_INCLUDES.upper() : MODEL_EFI_INCLUDE,
        TAB_LIBRARIES.upper() : MODEL_EFI_LIBRARY_INSTANCE,
        TAB_LIBRARY_CLASSES.upper() : MODEL_EFI_LIBRARY_CLASS,
        TAB_PACKAGES.upper() : MODEL_META_DATA_PACKAGE,
        TAB_NMAKE.upper() : MODEL_META_DATA_NMAKE,
        TAB_INF_FIXED_PCD.upper() : MODEL_PCD_FIXED_AT_BUILD,
        TAB_INF_PATCH_PCD.upper() : MODEL_PCD_PATCHABLE_IN_MODULE,
        TAB_INF_FEATURE_PCD.upper() : MODEL_PCD_FEATURE_FLAG,
        TAB_INF_PCD_EX.upper() : MODEL_PCD_DYNAMIC_EX,
        TAB_INF_PCD.upper() : MODEL_PCD_DYNAMIC,
        TAB_SOURCES.upper() : MODEL_EFI_SOURCE_FILE,
        TAB_GUIDS.upper() : MODEL_EFI_GUID,
        TAB_PROTOCOLS.upper() : MODEL_EFI_PROTOCOL,
        TAB_PPIS.upper() : MODEL_EFI_PPI,
        TAB_DEPEX.upper() : MODEL_EFI_DEPEX,
        TAB_BINARIES.upper() : MODEL_EFI_BINARY_FILE,
        TAB_USER_EXTENSIONS.upper() : MODEL_META_DATA_USER_EXTENSION
    }

    ## Constructor of InfParser
    #
    #  Initialize object of InfParser
    #
    #   @param      FilePath        The path of module description file
    #   @param      FileType        The raw data of DSC file
    #   @param      Table           Database used to retrieve module/package information
    #   @param      Macros          Macros used for replacement in file
    #
    def __init__(self, FilePath, FileType, Table, Macros=None):
        MetaFileParser.__init__(self, FilePath, FileType, Table, Macros)

    ## Parser starter
    def Start(self):
        NmakeLine = ''
        try:
            self._Content = open(self.MetaFile, 'r').readlines()
        except:
            EdkLogger.error("Parser", FILE_READ_FAILURE, ExtraData=self.MetaFile)

        # parse the file line by line
        IsFindBlockComment = False

        for Index in range(0, len(self._Content)):
            # skip empty, commented, block commented lines
            Line = CleanString(self._Content[Index], AllowCppStyleComment=True)
            NextLine = ''
            if Index + 1 < len(self._Content):
                NextLine = CleanString(self._Content[Index + 1])
            if Line == '':
                continue
            if Line.find(DataType.TAB_COMMENT_EDK_START) > -1:
                IsFindBlockComment = True
                continue
            if Line.find(DataType.TAB_COMMENT_EDK_END) > -1:
                IsFindBlockComment = False
                continue
            if IsFindBlockComment:
                continue

            self._LineIndex = Index
            self._CurrentLine = Line

            # section header
            if Line[0] == TAB_SECTION_START and Line[-1] == TAB_SECTION_END:
                self._SectionHeaderParser()
                continue
            # merge two lines specified by '\' in section NMAKE
            elif self._SectionType == MODEL_META_DATA_NMAKE:
                if Line[-1] == '\\':
                    if NextLine == '':
                        self._CurrentLine = NmakeLine + Line[0:-1]
                        NmakeLine = ''
                    else:
                        if NextLine[0] == TAB_SECTION_START and NextLine[-1] == TAB_SECTION_END:
                            self._CurrentLine = NmakeLine + Line[0:-1]
                            NmakeLine = ''
                        else:
                            NmakeLine = NmakeLine + ' ' + Line[0:-1]
                            continue
                else:
                    self._CurrentLine = NmakeLine + Line
                    NmakeLine = ''
            elif Line.upper().startswith('DEFINE '):
                # file private macros
                self._MacroParser()
                continue

            # section content
            self._ValueList = ['','','']
            # parse current line, result will be put in self._ValueList
            self._SectionParser[self._SectionType](self)
            if self._ValueList == None:
                continue
            #
            # Model, Value1, Value2, Value3, Arch, Platform, BelongsToItem=-1,
            # LineBegin=-1, ColumnBegin=-1, LineEnd=-1, ColumnEnd=-1, Enabled=-1
            #
            for Arch, Platform in self._Scope:
                self._Store(self._SectionType,
                            self._ValueList[0],
                            self._ValueList[1],
                            self._ValueList[2],
                            Arch,
                            Platform,
                            self._Owner,
                            self._LineIndex+1,
                            -1,
                            self._LineIndex+1,
                            -1,
                            0
                            )
        if IsFindBlockComment:
            EdkLogger.error("Parser", FORMAT_INVALID, "Open block comments (starting with /*) are expected to end with */", 
                            File=self.MetaFile)
        self._Done()

    ## Data parser for the format in which there's path
    #
    #   Only path can have macro used. So we need to replace them before use.
    #
    def _IncludeParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT)
        self._ValueList[0:len(TokenList)] = TokenList
        if len(self._Macros) > 0:
            for Index in range(0, len(self._ValueList)):
                Value = self._ValueList[Index]
                if Value.upper().find('$(EFI_SOURCE)\Edk'.upper()) > -1 or Value.upper().find('$(EFI_SOURCE)/Edk'.upper()) > -1:
                    Value = '$(EDK_SOURCE)' + Value[17:]
                if Value.find('$(EFI_SOURCE)') > -1 or Value.find('$(EDK_SOURCE)') > -1:
                    pass
                elif Value.startswith('.'):
                    pass
                elif Value.startswith('$('):
                    pass
                else:
                    Value = '$(EFI_SOURCE)/' + Value

                if Value == None or Value == '':
                    continue
                self._ValueList[Index] = NormPath(Value, self._Macros)

    ## Parse [Sources] section
    #
    #   Only path can have macro used. So we need to replace them before use.
    #
    def _SourceFileParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT)
        self._ValueList[0:len(TokenList)] = TokenList
        # For Acpi tables, remove macro like ' TABLE_NAME=Sata1'
        if 'COMPONENT_TYPE' in self._Macros:
            if self._Macros['COMPONENT_TYPE'].upper() == 'ACPITABLE':
                self._ValueList[0] = GetSplitValueList(self._ValueList[0], ' ', 1)[0]
        if self._Macros['BASE_NAME'] == 'Microcode':
            pass
        if len(self._Macros) > 0:
            for Index in range(0, len(self._ValueList)):
                Value = self._ValueList[Index]
                if Value == None or Value == '':
                    continue
                self._ValueList[Index] = NormPath(Value, self._Macros)

    ## Parse [Binaries] section
    #
    #   Only path can have macro used. So we need to replace them before use.
    #
    def _BinaryFileParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT, 2)
        if len(TokenList) < 2:
            EdkLogger.error('Parser', FORMAT_INVALID, "No file type or path specified",
                            ExtraData=self._CurrentLine + " (<FileType> | <FilePath> [| <Target>])",
                            File=self.MetaFile, Line=self._LineIndex+1)
        if not TokenList[0]:
            EdkLogger.error('Parser', FORMAT_INVALID, "No file type specified",
                            ExtraData=self._CurrentLine + " (<FileType> | <FilePath> [| <Target>])",
                            File=self.MetaFile, Line=self._LineIndex+1)
        if not TokenList[1]:
            EdkLogger.error('Parser', FORMAT_INVALID, "No file path specified",
                            ExtraData=self._CurrentLine + " (<FileType> | <FilePath> [| <Target>])",
                            File=self.MetaFile, Line=self._LineIndex+1)
        self._ValueList[0:len(TokenList)] = TokenList
        self._ValueList[1] = NormPath(self._ValueList[1], self._Macros)

    ## [defines] section parser
    def _DefineParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_EQUAL_SPLIT, 1)
        self._ValueList[0:len(TokenList)] = TokenList
        if self._ValueList[1] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No value specified",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        self._Macros[TokenList[0]] = ReplaceMacro(TokenList[1], self._Macros, False)
        
    ## [nmake] section parser (EDK.x style only)
    def _NmakeParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_EQUAL_SPLIT, 1)
        self._ValueList[0:len(TokenList)] = TokenList
        # remove macros
        self._ValueList[1] = ReplaceMacro(self._ValueList[1], self._Macros, False)
        # remove self-reference in macro setting
        #self._ValueList[1] = ReplaceMacro(self._ValueList[1], {self._ValueList[0]:''})

    ## [FixedPcd], [FeaturePcd], [PatchPcd], [Pcd] and [PcdEx] sections parser
    def _PcdParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT, 1)
        ValueList = GetSplitValueList(TokenList[0], TAB_SPLIT)
        if len(ValueList) != 2:
            EdkLogger.error('Parser', FORMAT_INVALID, "Illegal token space GUID and PCD name format",
                            ExtraData=self._CurrentLine + " (<TokenSpaceGuidCName>.<PcdCName>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        self._ValueList[0:1] = ValueList
        if len(TokenList) > 1:
            self._ValueList[2] = TokenList[1]
        if self._ValueList[0] == '' or self._ValueList[1] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No token space GUID or PCD name specified",
                            ExtraData=self._CurrentLine + " (<TokenSpaceGuidCName>.<PcdCName>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        # if value are 'True', 'true', 'TRUE' or 'False', 'false', 'FALSE', replace with integer 1 or 0.
        if self._ValueList[2] != '':
            InfPcdValueList = GetSplitValueList(TokenList[1], TAB_VALUE_SPLIT, 1)
            if InfPcdValueList[0] in ['True', 'true', 'TRUE']:
                self._ValueList[2] = TokenList[1].replace(InfPcdValueList[0], '1', 1);
            elif InfPcdValueList[0] in ['False', 'false', 'FALSE']:
                self._ValueList[2] = TokenList[1].replace(InfPcdValueList[0], '0', 1);

    ## [depex] section parser
    def _DepexParser(self):
        self._ValueList[0:1] = [self._CurrentLine]

    _SectionParser = {
        MODEL_UNKNOWN                   :   MetaFileParser._Skip,
        MODEL_META_DATA_HEADER          :   _DefineParser,
        MODEL_META_DATA_BUILD_OPTION    :   MetaFileParser._BuildOptionParser,
        MODEL_EFI_INCLUDE               :   _IncludeParser,                 # for EDK.x modules
        MODEL_EFI_LIBRARY_INSTANCE      :   MetaFileParser._CommonParser,   # for EDK.x modules
        MODEL_EFI_LIBRARY_CLASS         :   MetaFileParser._PathParser,
        MODEL_META_DATA_PACKAGE         :   MetaFileParser._PathParser,
        MODEL_META_DATA_NMAKE           :   _NmakeParser,                   # for EDK.x modules
        MODEL_PCD_FIXED_AT_BUILD        :   _PcdParser,
        MODEL_PCD_PATCHABLE_IN_MODULE   :   _PcdParser,
        MODEL_PCD_FEATURE_FLAG          :   _PcdParser,
        MODEL_PCD_DYNAMIC_EX            :   _PcdParser,
        MODEL_PCD_DYNAMIC               :   _PcdParser,
        MODEL_EFI_SOURCE_FILE           :   _SourceFileParser,
        MODEL_EFI_GUID                  :   MetaFileParser._CommonParser,
        MODEL_EFI_PROTOCOL              :   MetaFileParser._CommonParser,
        MODEL_EFI_PPI                   :   MetaFileParser._CommonParser,
        MODEL_EFI_DEPEX                 :   _DepexParser,
        MODEL_EFI_BINARY_FILE           :   _BinaryFileParser,
        MODEL_META_DATA_USER_EXTENSION  :   MetaFileParser._Skip,
    }

## DSC file parser class
#
#   @param      FilePath        The path of platform description file
#   @param      FileType        The raw data of DSC file
#   @param      Table           Database used to retrieve module/package information
#   @param      Macros          Macros used for replacement in file
#   @param      Owner           Owner ID (for sub-section parsing)
#   @param      From            ID from which the data comes (for !INCLUDE directive)
#
class DscParser(MetaFileParser):
    # DSC file supported data types (one type per section)
    DataType = {
        TAB_SKUIDS.upper()                          :   MODEL_EFI_SKU_ID,
        TAB_LIBRARIES.upper()                       :   MODEL_EFI_LIBRARY_INSTANCE,
        TAB_LIBRARY_CLASSES.upper()                 :   MODEL_EFI_LIBRARY_CLASS,
        TAB_BUILD_OPTIONS.upper()                   :   MODEL_META_DATA_BUILD_OPTION,
        TAB_PCDS_FIXED_AT_BUILD_NULL.upper()        :   MODEL_PCD_FIXED_AT_BUILD,
        TAB_PCDS_PATCHABLE_IN_MODULE_NULL.upper()   :   MODEL_PCD_PATCHABLE_IN_MODULE,
        TAB_PCDS_FEATURE_FLAG_NULL.upper()          :   MODEL_PCD_FEATURE_FLAG,
        TAB_PCDS_DYNAMIC_DEFAULT_NULL.upper()       :   MODEL_PCD_DYNAMIC_DEFAULT,
        TAB_PCDS_DYNAMIC_HII_NULL.upper()           :   MODEL_PCD_DYNAMIC_HII,
        TAB_PCDS_DYNAMIC_VPD_NULL.upper()           :   MODEL_PCD_DYNAMIC_VPD,
        TAB_PCDS_DYNAMIC_EX_DEFAULT_NULL.upper()    :   MODEL_PCD_DYNAMIC_EX_DEFAULT,
        TAB_PCDS_DYNAMIC_EX_HII_NULL.upper()        :   MODEL_PCD_DYNAMIC_EX_HII,
        TAB_PCDS_DYNAMIC_EX_VPD_NULL.upper()        :   MODEL_PCD_DYNAMIC_EX_VPD,
        TAB_COMPONENTS.upper()                      :   MODEL_META_DATA_COMPONENT,
        TAB_COMPONENTS_SOURCE_OVERRIDE_PATH.upper() :   MODEL_META_DATA_COMPONENT_SOURCE_OVERRIDE_PATH,
        TAB_DSC_DEFINES.upper()                     :   MODEL_META_DATA_HEADER,
        TAB_INCLUDE.upper()                         :   MODEL_META_DATA_INCLUDE,
        TAB_IF.upper()                              :   MODEL_META_DATA_CONDITIONAL_STATEMENT_IF,
        TAB_IF_DEF.upper()                          :   MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF,
        TAB_IF_N_DEF.upper()                        :   MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF,
        TAB_ELSE_IF.upper()                         :   MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSEIF,
        TAB_ELSE.upper()                            :   MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE,
        TAB_END_IF.upper()                          :   MODEL_META_DATA_CONDITIONAL_STATEMENT_ENDIF,
    }

    # sections which allow "!include" directive
    _IncludeAllowedSection = [
        TAB_COMMON_DEFINES.upper(),
        TAB_LIBRARIES.upper(),
        TAB_LIBRARY_CLASSES.upper(),
        TAB_SKUIDS.upper(),
        TAB_COMPONENTS.upper(),
        TAB_BUILD_OPTIONS.upper(),
        TAB_PCDS_FIXED_AT_BUILD_NULL.upper(),
        TAB_PCDS_PATCHABLE_IN_MODULE_NULL.upper(),
        TAB_PCDS_FEATURE_FLAG_NULL.upper(),
        TAB_PCDS_DYNAMIC_DEFAULT_NULL.upper(),
        TAB_PCDS_DYNAMIC_HII_NULL.upper(),
        TAB_PCDS_DYNAMIC_VPD_NULL.upper(),
        TAB_PCDS_DYNAMIC_EX_DEFAULT_NULL.upper(),
        TAB_PCDS_DYNAMIC_EX_HII_NULL.upper(),
        TAB_PCDS_DYNAMIC_EX_VPD_NULL.upper(),
        ]

    # operators which can be used in "!if/!ifdef/!ifndef" directives
    _OP_ = {
        "!"     :   lambda a:   not a,
        "!="    :   lambda a,b: a!=b,
        "=="    :   lambda a,b: a==b,
        ">"     :   lambda a,b: a>b,
        "<"     :   lambda a,b: a<b,
        "=>"    :   lambda a,b: a>=b,
        ">="    :   lambda a,b: a>=b,
        "<="    :   lambda a,b: a<=b,
        "=<"    :   lambda a,b: a<=b,
    }

    ## Constructor of DscParser
    #
    #  Initialize object of DscParser
    #
    #   @param      FilePath        The path of platform description file
    #   @param      FileType        The raw data of DSC file
    #   @param      Table           Database used to retrieve module/package information
    #   @param      Macros          Macros used for replacement in file
    #   @param      Owner           Owner ID (for sub-section parsing)
    #   @param      From            ID from which the data comes (for !INCLUDE directive)
    #
    def __init__(self, FilePath, FileType, Table, Macros=None, Owner=-1, From=-1):
        MetaFileParser.__init__(self, FilePath, FileType, Table, Macros, Owner, From)
        # to store conditional directive evaluation result
        self._Eval = Blist()

    ## Parser starter
    def Start(self):
        try:
            if self._Content == None:
                self._Content = open(self.MetaFile, 'r').readlines()
        except:
            EdkLogger.error("Parser", FILE_READ_FAILURE, ExtraData=self.MetaFile)

        for Index in range(0, len(self._Content)):
            Line = CleanString(self._Content[Index])
            # skip empty line
            if Line == '':
                continue
            self._CurrentLine = Line
            self._LineIndex = Index
            if self._InSubsection and self._Owner == -1:
                self._Owner = self._LastItem
            
            # section header
            if Line[0] == TAB_SECTION_START and Line[-1] == TAB_SECTION_END:
                self._SectionHeaderParser()
                continue
            # subsection ending
            elif Line[0] == '}':
                self._InSubsection = False
                self._SubsectionType = MODEL_UNKNOWN
                self._SubsectionName = ''
                self._Owner = -1
                continue
            # subsection header
            elif Line[0] == TAB_OPTION_START and Line[-1] == TAB_OPTION_END:
                self._SubsectionHeaderParser()
                continue
            # directive line
            elif Line[0] == '!':
                self._DirectiveParser()
                continue
            # file private macros
            elif Line.upper().startswith('DEFINE '):
                if self._Enabled < 0:
                    # Do not parse the macro and add it to self._Macros dictionary if directives
                    # statement is evaluated to false.
                    continue
                
                (Name, Value) = self._MacroParser()
                # Make the defined macro in DSC [Defines] section also
                # available for FDF file.
                if self._SectionName == TAB_COMMON_DEFINES.upper():
                    self._LastItem = self._Store(
                    MODEL_META_DATA_GLOBAL_DEFINE,
                    Name,
                    Value,
                    '',
                    'COMMON',
                    'COMMON',
                    self._Owner,
                    self._From,
                    self._LineIndex+1,
                    -1,
                    self._LineIndex+1,
                    -1,
                    self._Enabled
                    )
                continue
            elif Line.upper().startswith('EDK_GLOBAL '):
                if self._Enabled < 0:
                    # Do not parse the macro and add it to self._Macros dictionary
                    # if previous directives statement is evaluated to false.
                    continue
                
                (Name, Value) = self._MacroParser()
                for Arch, ModuleType in self._Scope:
                    self._LastItem = self._Store(
                    MODEL_META_DATA_DEFINE,
                    Name,
                    Value,
                    '',
                    Arch,
                    'COMMON',
                    self._Owner,
                    self._From,
                    self._LineIndex+1,
                    -1,
                    self._LineIndex+1,
                    -1,
                    self._Enabled
                    )
                continue

            # section content
            if self._InSubsection:
                SectionType = self._SubsectionType
                SectionName = self._SubsectionName
            else:
                SectionType = self._SectionType
                SectionName = self._SectionName

            self._ValueList = ['', '', '']
            self._SectionParser[SectionType](self)
            if self._ValueList == None:
                continue

            #
            # Model, Value1, Value2, Value3, Arch, ModuleType, BelongsToItem=-1, BelongsToFile=-1,
            # LineBegin=-1, ColumnBegin=-1, LineEnd=-1, ColumnEnd=-1, Enabled=-1
            #
            for Arch, ModuleType in self._Scope:
                self._LastItem = self._Store(
                    SectionType,
                    self._ValueList[0],
                    self._ValueList[1],
                    self._ValueList[2],
                    Arch,
                    ModuleType,
                    self._Owner,
                    self._From,
                    self._LineIndex+1,
                    -1,
                    self._LineIndex+1,
                    -1,
                    self._Enabled
                    )
        self._Done()

    ## [defines] section parser
    def _DefineParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_EQUAL_SPLIT, 1)
        if len(TokenList) < 2:
            EdkLogger.error('Parser', FORMAT_INVALID, "No value specified",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        # 'FLASH_DEFINITION', 'OUTPUT_DIRECTORY' need special processing
        if TokenList[0] in ['FLASH_DEFINITION', 'OUTPUT_DIRECTORY']:
            TokenList[1] = NormPath(TokenList[1], self._Macros)
        self._ValueList[0:len(TokenList)] = TokenList
        # Treat elements in the [defines] section as global macros for FDF file.
        self._LastItem = self._Store(
                            MODEL_META_DATA_GLOBAL_DEFINE,
                            TokenList[0],
                            TokenList[1],
                            '',
                            'COMMON',
                            'COMMON',
                            self._Owner,
                            self._From,
                            self._LineIndex+1,
                            -1,
                            self._LineIndex+1,
                            -1,
                            self._Enabled
                            )

    ## <subsection_header> parser
    def _SubsectionHeaderParser(self):
        self._SubsectionName = self._CurrentLine[1:-1].upper()
        if self._SubsectionName in self.DataType:
            self._SubsectionType = self.DataType[self._SubsectionName]
        else:
            self._SubsectionType = MODEL_UNKNOWN
            EdkLogger.warn("Parser", "Unrecognized sub-section", File=self.MetaFile,
                            Line=self._LineIndex+1, ExtraData=self._CurrentLine)

    ## Directive statement parser
    def _DirectiveParser(self):
        self._ValueList = ['','','']
        TokenList = GetSplitValueList(self._CurrentLine, ' ', 1)
        self._ValueList[0:len(TokenList)] = TokenList
        DirectiveName = self._ValueList[0].upper()
        if DirectiveName not in self.DataType:
            EdkLogger.error("Parser", FORMAT_INVALID, "Unknown directive [%s]" % DirectiveName,
                            File=self.MetaFile, Line=self._LineIndex+1)
        if DirectiveName in ['!IF', '!IFDEF', '!INCLUDE', '!IFNDEF', '!ELSEIF'] and self._ValueList[1] == '':
            EdkLogger.error("Parser", FORMAT_INVALID, "Missing expression",
                            File=self.MetaFile, Line=self._LineIndex+1,
                            ExtraData=self._CurrentLine)
        # keep the directive in database first
        self._LastItem = self._Store(
            self.DataType[DirectiveName],
            self._ValueList[0],
            self._ValueList[1],
            self._ValueList[2],
            'COMMON',
            'COMMON',
            self._Owner,
            self._From,
            self._LineIndex + 1,
            -1,
            self._LineIndex + 1,
            -1,
            0
            )

        # process the directive
        if DirectiveName == "!INCLUDE":
            if not self._SectionName in self._IncludeAllowedSection:
                EdkLogger.error("Parser", FORMAT_INVALID, File=self.MetaFile, Line=self._LineIndex+1,
                                ExtraData="'!include' is not allowed under section [%s]" % self._SectionName)
            # the included file must be relative to workspace
            IncludedFile = os.path.join(os.environ["WORKSPACE"], NormPath(self._ValueList[1], self._Macros))
            Parser = DscParser(IncludedFile, self._FileType, self._Table, self._Macros, From=self._LastItem)
            # set the parser status with current status
            Parser._SectionName = self._SectionName
            Parser._SectionType = self._SectionType
            Parser._Scope = self._Scope
            Parser._Enabled = self._Enabled
            try:
                Parser.Start()
            except:
                EdkLogger.error("Parser", PARSER_ERROR, File=self.MetaFile, Line=self._LineIndex+1,
                                ExtraData="Failed to parse content in file %s" % IncludedFile)
            # insert an imaginary token in the DSC table to indicate its external dependency on another file
            self._Store(MODEL_EXTERNAL_DEPENDENCY, IncludedFile, str(os.stat(IncludedFile)[8]), "")
            # update current status with sub-parser's status
            self._SectionName = Parser._SectionName
            self._SectionType = Parser._SectionType
            self._Scope       = Parser._Scope
            self._Enabled     = Parser._Enabled
            self._Macros.update(Parser._Macros)
        else:
            if DirectiveName in ["!IF", "!IFDEF", "!IFNDEF"]:
                # evaluate the expression
                Result = self._Evaluate(self._ValueList[1])
                if DirectiveName == "!IFNDEF":
                    Result = not Result
                self._Eval.append(Result)
            elif DirectiveName in ["!ELSEIF"]:
                # evaluate the expression
                self._Eval[-1] = (not self._Eval[-1]) & self._Evaluate(self._ValueList[1])
            elif DirectiveName in ["!ELSE"]:
                self._Eval[-1] = not self._Eval[-1]
            elif DirectiveName in ["!ENDIF"]:
                if len(self._Eval) > 0:
                    self._Eval.pop()
                else:
                    EdkLogger.error("Parser", FORMAT_INVALID, "!IF..[!ELSE]..!ENDIF doesn't match",
                                    File=self.MetaFile, Line=self._LineIndex+1)
            if self._Eval.Result == False:
                self._Enabled = 0 - len(self._Eval)
            else:
                self._Enabled = len(self._Eval)

    ## Evaluate the Token for its value; for now only macros are supported.
    def _EvaluateToken(self, TokenName, Expression):
        if TokenName.startswith("$(") and TokenName.endswith(")"):
            Name = TokenName[2:-1]
            return self._Macros.get(Name)
        else:
            EdkLogger.error('Parser', FORMAT_INVALID, "Unknown operand '%(Token)s', "
                            "please use '$(%(Token)s)' if '%(Token)s' is a macro" % {"Token" : TokenName},
                            File=self.MetaFile, Line=self._LineIndex+1, ExtraData=Expression)
    
    ## Evaluate the value of expression in "if/ifdef/ifndef" directives
    def _Evaluate(self, Expression):
        TokenList = Expression.split()
        TokenNumber = len(TokenList)
        # one operand, guess it's just a macro name
        if TokenNumber == 1:
            TokenValue =  self._EvaluateToken(TokenList[0], Expression)
            return TokenValue != None
        # two operands, suppose it's "!xxx" format
        elif TokenNumber == 2:
            Op = TokenList[0]
            if Op not in self._OP_:
                EdkLogger.error('Parser', FORMAT_INVALID, "Unsupported operator [%s]" % Op, File=self.MetaFile,
                                Line=self._LineIndex+1, ExtraData=Expression)
            if TokenList[1].upper() == 'TRUE':
                Value = True
            else:
                Value = False
            return self._OP_[Op](Value)
        # three operands
        elif TokenNumber == 3:
            TokenValue = TokenList[0] 
            if TokenValue != "":
                if TokenValue[0] in ["'", '"'] and TokenValue[-1] in ["'", '"']:
                    TokenValue = TokenValue[1:-1]             
                if TokenValue.startswith("$(") and TokenValue.endswith(")"):
                    TokenValue = self._EvaluateToken(TokenValue, Expression)
                    if TokenValue == None:
                        return False
                if TokenValue != "":                
                    if TokenValue[0] in ["'", '"'] and TokenValue[-1] in ["'", '"']:
                        TokenValue = TokenValue[1:-1]
               
            Value = TokenList[2]
            if Value != "":
                if Value[0] in ["'", '"'] and Value[-1] in ["'", '"']:
                    Value = Value[1:-1]         
                if Value.startswith("$(") and Value.endswith(")"):
                    Value = self._EvaluateToken(Value, Expression)          
                    if Value == None:
                        return False
                if Value != "":                
                    if Value[0] in ["'", '"'] and Value[-1] in ["'", '"']:
                        Value = Value[1:-1]
            Op = TokenList[1]
            if Op not in self._OP_:
                EdkLogger.error('Parser', FORMAT_INVALID, "Unsupported operator [%s]" % Op, File=self.MetaFile,
                                Line=self._LineIndex+1, ExtraData=Expression)
            return self._OP_[Op](TokenValue, Value)
        else:
            EdkLogger.error('Parser', FORMAT_INVALID, File=self.MetaFile, Line=self._LineIndex+1,
                            ExtraData=Expression)

    ## PCD sections parser
    #
    #   [PcdsFixedAtBuild]
    #   [PcdsPatchableInModule]
    #   [PcdsFeatureFlag]
    #   [PcdsDynamicEx
    #   [PcdsDynamicExDefault]
    #   [PcdsDynamicExVpd]
    #   [PcdsDynamicExHii]
    #   [PcdsDynamic]
    #   [PcdsDynamicDefault]
    #   [PcdsDynamicVpd]
    #   [PcdsDynamicHii]
    #
    def _PcdParser(self):
        TokenList = GetSplitValueList(ReplaceMacro(self._CurrentLine, self._Macros), TAB_VALUE_SPLIT, 1)
        self._ValueList[0:1] = GetSplitValueList(TokenList[0], TAB_SPLIT)
        if len(TokenList) == 2:
            self._ValueList[2] = TokenList[1]
        if self._ValueList[0] == '' or self._ValueList[1] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No token space GUID or PCD name specified",
                            ExtraData=self._CurrentLine + " (<TokenSpaceGuidCName>.<TokenCName>|<PcdValue>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        if self._ValueList[2] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No PCD value given",
                            ExtraData=self._CurrentLine + " (<TokenSpaceGuidCName>.<TokenCName>|<PcdValue>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        # if value are 'True', 'true', 'TRUE' or 'False', 'false', 'FALSE', replace with integer 1 or 0.
        DscPcdValueList = GetSplitValueList(TokenList[1], TAB_VALUE_SPLIT, 1)
        if DscPcdValueList[0] in ['True', 'true', 'TRUE']:
            self._ValueList[2] = TokenList[1].replace(DscPcdValueList[0], '1', 1);
        elif DscPcdValueList[0] in ['False', 'false', 'FALSE']:
            self._ValueList[2] = TokenList[1].replace(DscPcdValueList[0], '0', 1);
            
    ## [components] section parser
    def _ComponentParser(self):
        if self._CurrentLine[-1] == '{':
            self._ValueList[0] = self._CurrentLine[0:-1].strip()
            self._InSubsection = True
        else:
            self._ValueList[0] = self._CurrentLine
        if len(self._Macros) > 0:
            self._ValueList[0] = NormPath(self._ValueList[0], self._Macros)

    def _LibraryClassParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT)
        if len(TokenList) < 2:
            EdkLogger.error('Parser', FORMAT_INVALID, "No library class or instance specified",
                            ExtraData=self._CurrentLine + " (<LibraryClassName>|<LibraryInstancePath>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        if TokenList[0] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No library class specified",
                            ExtraData=self._CurrentLine + " (<LibraryClassName>|<LibraryInstancePath>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        if TokenList[1] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No library instance specified",
                            ExtraData=self._CurrentLine + " (<LibraryClassName>|<LibraryInstancePath>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        self._ValueList[0:len(TokenList)] = TokenList
        if len(self._Macros) > 0:
            self._ValueList[1] = NormPath(self._ValueList[1], self._Macros)

    def _CompponentSourceOverridePathParser(self):
        if len(self._Macros) > 0:
            self._ValueList[0] = NormPath(self._CurrentLine, self._Macros)

    _SectionParser = {
        MODEL_META_DATA_HEADER                         :   _DefineParser,
        MODEL_EFI_SKU_ID                               :   MetaFileParser._CommonParser,
        MODEL_EFI_LIBRARY_INSTANCE                     :   MetaFileParser._PathParser,
        MODEL_EFI_LIBRARY_CLASS                        :   _LibraryClassParser,
        MODEL_PCD_FIXED_AT_BUILD                       :   _PcdParser,
        MODEL_PCD_PATCHABLE_IN_MODULE                  :   _PcdParser,
        MODEL_PCD_FEATURE_FLAG                         :   _PcdParser,
        MODEL_PCD_DYNAMIC_DEFAULT                      :   _PcdParser,
        MODEL_PCD_DYNAMIC_HII                          :   _PcdParser,
        MODEL_PCD_DYNAMIC_VPD                          :   _PcdParser,
        MODEL_PCD_DYNAMIC_EX_DEFAULT                   :   _PcdParser,
        MODEL_PCD_DYNAMIC_EX_HII                       :   _PcdParser,
        MODEL_PCD_DYNAMIC_EX_VPD                       :   _PcdParser,
        MODEL_META_DATA_COMPONENT                      :   _ComponentParser,
        MODEL_META_DATA_COMPONENT_SOURCE_OVERRIDE_PATH :   _CompponentSourceOverridePathParser,
        MODEL_META_DATA_BUILD_OPTION                   :   MetaFileParser._BuildOptionParser,
        MODEL_UNKNOWN                                  :   MetaFileParser._Skip,
        MODEL_META_DATA_USER_EXTENSION                 :   MetaFileParser._Skip,
    }

## DEC file parser class
#
#   @param      FilePath        The path of platform description file
#   @param      FileType        The raw data of DSC file
#   @param      Table           Database used to retrieve module/package information
#   @param      Macros          Macros used for replacement in file
#
class DecParser(MetaFileParser):
    # DEC file supported data types (one type per section)
    DataType = {
        TAB_DEC_DEFINES.upper()                     :   MODEL_META_DATA_HEADER,
        TAB_INCLUDES.upper()                        :   MODEL_EFI_INCLUDE,
        TAB_LIBRARY_CLASSES.upper()                 :   MODEL_EFI_LIBRARY_CLASS,
        TAB_GUIDS.upper()                           :   MODEL_EFI_GUID,
        TAB_PPIS.upper()                            :   MODEL_EFI_PPI,
        TAB_PROTOCOLS.upper()                       :   MODEL_EFI_PROTOCOL,
        TAB_PCDS_FIXED_AT_BUILD_NULL.upper()        :   MODEL_PCD_FIXED_AT_BUILD,
        TAB_PCDS_PATCHABLE_IN_MODULE_NULL.upper()   :   MODEL_PCD_PATCHABLE_IN_MODULE,
        TAB_PCDS_FEATURE_FLAG_NULL.upper()          :   MODEL_PCD_FEATURE_FLAG,
        TAB_PCDS_DYNAMIC_NULL.upper()               :   MODEL_PCD_DYNAMIC,
        TAB_PCDS_DYNAMIC_EX_NULL.upper()            :   MODEL_PCD_DYNAMIC_EX,
    }

    ## Constructor of DecParser
    #
    #  Initialize object of DecParser
    #
    #   @param      FilePath        The path of platform description file
    #   @param      FileType        The raw data of DSC file
    #   @param      Table           Database used to retrieve module/package information
    #   @param      Macros          Macros used for replacement in file
    #
    def __init__(self, FilePath, FileType, Table, Macro=None):
        MetaFileParser.__init__(self, FilePath, FileType, Table, Macro, -1)
        self._Comments = []

    ## Parser starter
    def Start(self):
        try:
            if self._Content == None:
                self._Content = open(self.MetaFile, 'r').readlines()
        except:
            EdkLogger.error("Parser", FILE_READ_FAILURE, ExtraData=self.MetaFile)

        for Index in range(0, len(self._Content)):
            Line, Comment = CleanString2(self._Content[Index])
            self._CurrentLine = Line
            self._LineIndex = Index

            # save comment for later use
            if Comment:
                self._Comments.append((Comment, self._LineIndex+1))
            # skip empty line
            if Line == '':
                continue

            # section header
            if Line[0] == TAB_SECTION_START and Line[-1] == TAB_SECTION_END:
                self._SectionHeaderParser()
                self._Comments = []
                continue
            elif Line.startswith('DEFINE '):
                self._MacroParser()
                continue
            elif len(self._SectionType) == 0:
                self._Comments = []
                continue

            # section content
            self._ValueList = ['','','']
            self._SectionParser[self._SectionType[0]](self)
            if self._ValueList == None:
                self._Comments = []
                continue

            #
            # Model, Value1, Value2, Value3, Arch, BelongsToItem=-1, LineBegin=-1,
            # ColumnBegin=-1, LineEnd=-1, ColumnEnd=-1, FeatureFlag='', Enabled=-1
            #
            for Arch, ModuleType, Type in self._Scope:
                self._LastItem = self._Store(
                    Type,
                    self._ValueList[0],
                    self._ValueList[1],
                    self._ValueList[2],
                    Arch,
                    ModuleType,
                    self._Owner,
                    self._LineIndex+1,
                    -1,
                    self._LineIndex+1,
                    -1,
                    0
                    )
                for Comment, LineNo in self._Comments:
                    self._Store(
                        MODEL_META_DATA_COMMENT,
                        Comment,
                        self._ValueList[0],
                        self._ValueList[1],
                        Arch,
                        ModuleType,
                        self._LastItem,
                        LineNo,
                        -1,
                        LineNo,
                        -1,
                        0
                        )
            self._Comments = []
        self._Done()

    ## Section header parser
    #
    #   The section header is always in following format:
    #
    #       [section_name.arch<.platform|module_type>]
    #
    def _SectionHeaderParser(self):
        self._Scope = []
        self._SectionName = ''
        self._SectionType = []
        ArchList = set()
        for Item in GetSplitValueList(self._CurrentLine[1:-1], TAB_COMMA_SPLIT):
            if Item == '':
                continue
            ItemList = GetSplitValueList(Item, TAB_SPLIT)

            # different types of PCD are permissible in one section
            self._SectionName = ItemList[0].upper()
            if self._SectionName in self.DataType:
                if self.DataType[self._SectionName] not in self._SectionType:
                    self._SectionType.append(self.DataType[self._SectionName])
            else:
                EdkLogger.warn("Parser", "Unrecognized section", File=self.MetaFile,
                                Line=self._LineIndex+1, ExtraData=self._CurrentLine)
                continue

            if MODEL_PCD_FEATURE_FLAG in self._SectionType and len(self._SectionType) > 1:
                EdkLogger.error(
                            'Parser',
                            FORMAT_INVALID,
                            "%s must not be in the same section of other types of PCD" % TAB_PCDS_FEATURE_FLAG_NULL,
                            File=self.MetaFile,
                            Line=self._LineIndex+1,
                            ExtraData=self._CurrentLine
                            )
            # S1 is always Arch
            if len(ItemList) > 1:
                S1 = ItemList[1].upper()
            else:
                S1 = 'COMMON'
            ArchList.add(S1)
            # S2 may be Platform or ModuleType
            if len(ItemList) > 2:
                S2 = ItemList[2].upper()
            else:
                S2 = 'COMMON'
            if [S1, S2, self.DataType[self._SectionName]] not in self._Scope:
                self._Scope.append([S1, S2, self.DataType[self._SectionName]])

        # 'COMMON' must not be used with specific ARCHs at the same section
        if 'COMMON' in ArchList and len(ArchList) > 1:
            EdkLogger.error('Parser', FORMAT_INVALID, "'common' ARCH must not be used with specific ARCHs",
                            File=self.MetaFile, Line=self._LineIndex+1, ExtraData=self._CurrentLine)

    ## [guids], [ppis] and [protocols] section parser
    def _GuidParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_EQUAL_SPLIT, 1)
        if len(TokenList) < 2:
            EdkLogger.error('Parser', FORMAT_INVALID, "No GUID name or value specified",
                            ExtraData=self._CurrentLine + " (<CName> = <GuidValueInCFormat>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        if TokenList[0] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No GUID name specified",
                            ExtraData=self._CurrentLine + " (<CName> = <GuidValueInCFormat>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        if TokenList[1] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No GUID value specified",
                            ExtraData=self._CurrentLine + " (<CName> = <GuidValueInCFormat>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        if TokenList[1][0] != '{' or TokenList[1][-1] != '}' or GuidStructureStringToGuidString(TokenList[1]) == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "Invalid GUID value format",
                            ExtraData=self._CurrentLine + \
                                      " (<CName> = <GuidValueInCFormat:{8,4,4,{2,2,2,2,2,2,2,2}}>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        self._ValueList[0] = TokenList[0]
        self._ValueList[1] = TokenList[1]

    ## PCD sections parser
    #
    #   [PcdsFixedAtBuild]
    #   [PcdsPatchableInModule]
    #   [PcdsFeatureFlag]
    #   [PcdsDynamicEx
    #   [PcdsDynamic]
    #
    def _PcdParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT, 1)
        self._ValueList[0:1] = GetSplitValueList(TokenList[0], TAB_SPLIT)
        # check PCD information
        if self._ValueList[0] == '' or self._ValueList[1] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No token space GUID or PCD name specified",
                            ExtraData=self._CurrentLine + \
                                      " (<TokenSpaceGuidCName>.<PcdCName>|<DefaultValue>|<DatumType>|<Token>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        # check PCD datum information
        if len(TokenList) < 2 or TokenList[1] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "No PCD Datum information given",
                            ExtraData=self._CurrentLine + \
                                      " (<TokenSpaceGuidCName>.<PcdCName>|<DefaultValue>|<DatumType>|<Token>)",
                            File=self.MetaFile, Line=self._LineIndex+1)

        
        ValueRe  = re.compile(r'^\s*L?\".*\|.*\"')
        PtrValue = ValueRe.findall(TokenList[1])
        
        # Has VOID* type string, may contain "|" character in the string. 
        if len(PtrValue) != 0:
            ptrValueList = re.sub(ValueRe, '', TokenList[1])
            ValueList    = GetSplitValueList(ptrValueList)
            ValueList[0] = PtrValue[0]
        else:
            ValueList = GetSplitValueList(TokenList[1])
            
        
        # check if there's enough datum information given
        if len(ValueList) != 3:
            EdkLogger.error('Parser', FORMAT_INVALID, "Invalid PCD Datum information given",
                            ExtraData=self._CurrentLine + \
                                      " (<TokenSpaceGuidCName>.<PcdCName>|<DefaultValue>|<DatumType>|<Token>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        # check default value
        if ValueList[0] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "Missing DefaultValue in PCD Datum information",
                            ExtraData=self._CurrentLine + \
                                      " (<TokenSpaceGuidCName>.<PcdCName>|<DefaultValue>|<DatumType>|<Token>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        # check datum type
        if ValueList[1] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "Missing DatumType in PCD Datum information",
                            ExtraData=self._CurrentLine + \
                                      " (<TokenSpaceGuidCName>.<PcdCName>|<DefaultValue>|<DatumType>|<Token>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        # check token of the PCD
        if ValueList[2] == '':
            EdkLogger.error('Parser', FORMAT_INVALID, "Missing Token in PCD Datum information",
                            ExtraData=self._CurrentLine + \
                                      " (<TokenSpaceGuidCName>.<PcdCName>|<DefaultValue>|<DatumType>|<Token>)",
                            File=self.MetaFile, Line=self._LineIndex+1)
        # check format of default value against the datum type
        IsValid, Cause = CheckPcdDatum(ValueList[1], ValueList[0])
        if not IsValid:
            EdkLogger.error('Parser', FORMAT_INVALID, Cause, ExtraData=self._CurrentLine,
                            File=self.MetaFile, Line=self._LineIndex+1)
        if ValueList[0] in ['True', 'true', 'TRUE']:
            ValueList[0] = '1'
        elif ValueList[0] in ['False', 'false', 'FALSE']:
            ValueList[0] = '0'

        self._ValueList[2] = ValueList[0].strip() + '|' + ValueList[1].strip() + '|' + ValueList[2].strip()

    _SectionParser = {
        MODEL_META_DATA_HEADER          :   MetaFileParser._DefineParser,
        MODEL_EFI_INCLUDE               :   MetaFileParser._PathParser,
        MODEL_EFI_LIBRARY_CLASS         :   MetaFileParser._PathParser,
        MODEL_EFI_GUID                  :   _GuidParser,
        MODEL_EFI_PPI                   :   _GuidParser,
        MODEL_EFI_PROTOCOL              :   _GuidParser,
        MODEL_PCD_FIXED_AT_BUILD        :   _PcdParser,
        MODEL_PCD_PATCHABLE_IN_MODULE   :   _PcdParser,
        MODEL_PCD_FEATURE_FLAG          :   _PcdParser,
        MODEL_PCD_DYNAMIC               :   _PcdParser,
        MODEL_PCD_DYNAMIC_EX            :   _PcdParser,
        MODEL_UNKNOWN                   :   MetaFileParser._Skip,
        MODEL_META_DATA_USER_EXTENSION  :   MetaFileParser._Skip,
    }

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    pass

