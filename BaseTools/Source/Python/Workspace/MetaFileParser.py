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
import re
import time
import copy

import Common.EdkLogger as EdkLogger
import Common.GlobalData as GlobalData

from CommonDataClass.DataClass import *
from Common.DataType import *
from Common.String import *
from Common.Misc import GuidStructureStringToGuidString, CheckPcdDatum, PathClass, AnalyzePcdData
from Common.Expression import *
from CommonDataClass.Exceptions import *

from MetaFileTable import MetaFileStorage

## A decorator used to parse macro definition
def ParseMacro(Parser):
    def MacroParser(self):
        Match = gMacroDefPattern.match(self._CurrentLine)
        if not Match:
            # Not 'DEFINE/EDK_GLOBAL' statement, call decorated method
            Parser(self)
            return

        TokenList = GetSplitValueList(self._CurrentLine[Match.end(1):], TAB_EQUAL_SPLIT, 1)
        # Syntax check
        if not TokenList[0]:
            EdkLogger.error('Parser', FORMAT_INVALID, "No macro name given",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        if len(TokenList) < 2:
            TokenList.append('')

        Type = Match.group(1)
        Name, Value = TokenList
        # Global macros can be only defined via environment variable
        if Name in GlobalData.gGlobalDefines:
            EdkLogger.error('Parser', FORMAT_INVALID, "%s can only be defined via environment variable" % Name,
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        # Only upper case letters, digit and '_' are allowed
        if not gMacroNamePattern.match(Name):
            EdkLogger.error('Parser', FORMAT_INVALID, "The macro name must be in the pattern [A-Z][A-Z0-9_]*",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)

        Value = ReplaceMacro(Value, self._Macros)
        if Type in self.DataType:
            self._ItemType = self.DataType[Type]
        else:
            self._ItemType = MODEL_META_DATA_DEFINE
        # DEFINE defined macros
        if Type == TAB_DSC_DEFINES_DEFINE:
            #
            # First judge whether this DEFINE is in conditional directive statements or not.
            #
            if type(self) == DscParser and self._InDirective > -1:
                pass
            else:
                if type(self) == DecParser:
                    if MODEL_META_DATA_HEADER in self._SectionType:
                        self._FileLocalMacros[Name] = Value
                    else:
                        self._ConstructSectionMacroDict(Name, Value)
                elif self._SectionType == MODEL_META_DATA_HEADER:
                    self._FileLocalMacros[Name] = Value
                else:
                    self._ConstructSectionMacroDict(Name, Value)

        # EDK_GLOBAL defined macros
        elif type(self) != DscParser:
            EdkLogger.error('Parser', FORMAT_INVALID, "EDK_GLOBAL can only be used in .dsc file",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        elif self._SectionType != MODEL_META_DATA_HEADER:
            EdkLogger.error('Parser', FORMAT_INVALID, "EDK_GLOBAL can only be used under [Defines] section",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        elif (Name in self._FileLocalMacros) and (self._FileLocalMacros[Name] != Value):
            EdkLogger.error('Parser', FORMAT_INVALID, "EDK_GLOBAL defined a macro with the same name and different value as one defined by 'DEFINE'",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
            
        self._ValueList = [Type, Name, Value]

    return MacroParser

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
    def __init__(self, FilePath, FileType, Table, Owner=-1, From=-1):
        self._Table = Table
        self._RawTable = Table
        self._FileType = FileType
        self.MetaFile = FilePath
        self._FileDir = self.MetaFile.Dir
        self._Defines = {}
        self._FileLocalMacros = {}
        self._SectionsMacroDict = {}

        # for recursive parsing
        self._Owner = [Owner]
        self._From = From

        # parsr status for parsing
        self._ValueList = ['', '', '', '', '']
        self._Scope = []
        self._LineIndex = 0
        self._CurrentLine = ''
        self._SectionType = MODEL_UNKNOWN
        self._SectionName = ''
        self._InSubsection = False
        self._SubsectionType = MODEL_UNKNOWN
        self._SubsectionName = ''
        self._ItemType = MODEL_UNKNOWN
        self._LastItem = -1
        self._Enabled = 0
        self._Finished = False
        self._PostProcessed = False
        # Different version of meta-file has different way to parse.
        self._Version = 0

    ## Store the parsed data in table
    def _Store(self, *Args):
        return self._Table.Insert(*Args)

    ## Virtual method for starting parse
    def Start(self):
        raise NotImplementedError

    ## Notify a post-process is needed
    def DoPostProcess(self):
        self._PostProcessed = False

    ## Set parsing complete flag in both class and table
    def _Done(self):
        self._Finished = True
        ## Do not set end flag when processing included files
        if self._From == -1:
            self._Table.SetEndFlag()

    def _PostProcess(self):
        self._PostProcessed = True

    ## Get the parse complete flag
    def _GetFinished(self):
        return self._Finished

    ## Set the complete flag
    def _SetFinished(self, Value):
        self._Finished = Value

    ## Use [] style to query data in table, just for readability
    #
    #   DataInfo = [data_type, scope1(arch), scope2(platform/moduletype)]
    #
    def __getitem__(self, DataInfo):
        if type(DataInfo) != type(()):
            DataInfo = (DataInfo,)

        # Parse the file first, if necessary
        if not self._Finished:
            if self._RawTable.IsIntegrity():
                self._Finished = True
            else:
                self._Table = self._RawTable
                self._PostProcessed = False
                self.Start()

        # No specific ARCH or Platform given, use raw data
        if self._RawTable and (len(DataInfo) == 1 or DataInfo[1] == None):
            return self._RawTable.Query(*DataInfo)

        # Do post-process if necessary
        if not self._PostProcessed:
            self._PostProcess()

        return self._Table.Query(*DataInfo)

    ## Data parser for the common format in different type of file
    #
    #   The common format in the meatfile is like
    #
    #       xxx1 | xxx2 | xxx3
    #
    @ParseMacro
    def _CommonParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT)
        self._ValueList[0:len(TokenList)] = TokenList

    ## Data parser for the format in which there's path
    #
    #   Only path can have macro used. So we need to replace them before use.
    #
    @ParseMacro
    def _PathParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT)
        self._ValueList[0:len(TokenList)] = TokenList
        # Don't do macro replacement for dsc file at this point
        if type(self) != DscParser:
            Macros = self._Macros
            self._ValueList = [ReplaceMacro(Value, Macros) for Value in self._ValueList]

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
        # If the section information is needed later, it should be stored in database
        self._ValueList[0] = self._SectionName

    ## [defines] section parser
    @ParseMacro
    def _DefineParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_EQUAL_SPLIT, 1)
        self._ValueList[1:len(TokenList)] = TokenList
        if not self._ValueList[1]:
            EdkLogger.error('Parser', FORMAT_INVALID, "No name specified",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        if not self._ValueList[2]:
            EdkLogger.error('Parser', FORMAT_INVALID, "No value specified",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)

        self._ValueList = [ReplaceMacro(Value, self._Macros) for Value in self._ValueList]
        Name, Value = self._ValueList[1], self._ValueList[2]
        # Sometimes, we need to make differences between EDK and EDK2 modules 
        if Name == 'INF_VERSION':
            try:
                self._Version = int(Value, 0)
            except:
                EdkLogger.error('Parser', FORMAT_INVALID, "Invalid version number",
                                ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)

        if type(self) == InfParser and self._Version < 0x00010005:
            # EDK module allows using defines as macros
            self._FileLocalMacros[Name] = Value
        self._Defines[Name] = Value

    ## [BuildOptions] section parser
    @ParseMacro
    def _BuildOptionParser(self):
        self._CurrentLine = CleanString(self._CurrentLine, BuildOption=True)
        TokenList = GetSplitValueList(self._CurrentLine, TAB_EQUAL_SPLIT, 1)
        TokenList2 = GetSplitValueList(TokenList[0], ':', 1)
        if len(TokenList2) == 2:
            self._ValueList[0] = TokenList2[0]              # toolchain family
            self._ValueList[1] = TokenList2[1]              # keys
        else:
            self._ValueList[1] = TokenList[0]
        if len(TokenList) == 2 and type(self) != DscParser: # value
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

    def _GetMacros(self):
        Macros = {}
        Macros.update(self._FileLocalMacros)
        Macros.update(self._GetApplicableSectionMacro())
        return Macros

    ## Construct section Macro dict 
    def _ConstructSectionMacroDict(self, Name, Value):
        ScopeKey = [(Scope[0], Scope[1]) for Scope in self._Scope]
        ScopeKey = tuple(ScopeKey)
        SectionDictKey = self._SectionType, ScopeKey
        #
        # DecParser SectionType is a list, will contain more than one item only in Pcd Section
        # As Pcd section macro usage is not alllowed, so here it is safe
        #
        if type(self) == DecParser:
            SectionDictKey = self._SectionType[0], ScopeKey
        if SectionDictKey not in self._SectionsMacroDict:
            self._SectionsMacroDict[SectionDictKey] = {}
        SectionLocalMacros = self._SectionsMacroDict[SectionDictKey]
        SectionLocalMacros[Name] = Value

    ## Get section Macros that are applicable to current line, which may come from other sections 
    ## that share the same name while scope is wider
    def _GetApplicableSectionMacro(self):
        Macros = {}

        ComComMacroDict = {}
        ComSpeMacroDict = {}
        SpeSpeMacroDict = {}
        
        ActiveSectionType = self._SectionType
        if type(self) == DecParser:
            ActiveSectionType = self._SectionType[0]
            
        for (SectionType, Scope) in self._SectionsMacroDict:
            if SectionType != ActiveSectionType:
                continue

            for ActiveScope in self._Scope:
                Scope0, Scope1 = ActiveScope[0], ActiveScope[1]
                if(Scope0, Scope1) not in Scope:
                    break
            else:
                SpeSpeMacroDict.update(self._SectionsMacroDict[(SectionType, Scope)])
            
            for ActiveScope in self._Scope:
                Scope0, Scope1 = ActiveScope[0], ActiveScope[1]
                if(Scope0, Scope1) not in Scope and (Scope0, "COMMON") not in Scope and ("COMMON", Scope1) not in Scope:
                    break
            else:
                ComSpeMacroDict.update(self._SectionsMacroDict[(SectionType, Scope)])

            if ("COMMON", "COMMON") in Scope:
                ComComMacroDict.update(self._SectionsMacroDict[(SectionType, Scope)])

        Macros.update(ComComMacroDict)
        Macros.update(ComSpeMacroDict)
        Macros.update(SpeSpeMacroDict)

        return Macros

    _SectionParser  = {}
    Finished        = property(_GetFinished, _SetFinished)
    _Macros         = property(_GetMacros)


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
        TAB_DSC_DEFINES_DEFINE : MODEL_META_DATA_DEFINE,
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
    def __init__(self, FilePath, FileType, Table):
        # prevent re-initialization
        if hasattr(self, "_Table"):
            return
        MetaFileParser.__init__(self, FilePath, FileType, Table)

    ## Parser starter
    def Start(self):
        NmakeLine = ''
        Content = ''
        try:
            Content = open(str(self.MetaFile), 'r').readlines()
        except:
            EdkLogger.error("Parser", FILE_READ_FAILURE, ExtraData=self.MetaFile)

        # parse the file line by line
        IsFindBlockComment = False

        for Index in range(0, len(Content)):
            # skip empty, commented, block commented lines
            Line = CleanString(Content[Index], AllowCppStyleComment=True)
            NextLine = ''
            if Index + 1 < len(Content):
                NextLine = CleanString(Content[Index + 1])
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
                # Check invalid sections
                if self._Version < 0x00010005:
                    if self._SectionType in [MODEL_META_DATA_BUILD_OPTION,
                                             MODEL_EFI_LIBRARY_CLASS,
                                             MODEL_META_DATA_PACKAGE,
                                             MODEL_PCD_FIXED_AT_BUILD,
                                             MODEL_PCD_PATCHABLE_IN_MODULE,
                                             MODEL_PCD_FEATURE_FLAG,
                                             MODEL_PCD_DYNAMIC_EX,
                                             MODEL_PCD_DYNAMIC,
                                             MODEL_EFI_GUID,
                                             MODEL_EFI_PROTOCOL,
                                             MODEL_EFI_PPI,
                                             MODEL_META_DATA_USER_EXTENSION]:
                        EdkLogger.error('Parser', FORMAT_INVALID,
                                        "Section [%s] is not allowed in inf file without version" % (self._SectionName),
                                        ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
                elif self._SectionType in [MODEL_EFI_INCLUDE,
                                           MODEL_EFI_LIBRARY_INSTANCE,
                                           MODEL_META_DATA_NMAKE]:
                    EdkLogger.error('Parser', FORMAT_INVALID,
                                    "Section [%s] is not allowed in inf file with version 0x%08x" % (self._SectionName, self._Version),
                                    ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
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

            # section content
            self._ValueList = ['','','']
            # parse current line, result will be put in self._ValueList
            self._SectionParser[self._SectionType](self)
            if self._ValueList == None or self._ItemType == MODEL_META_DATA_DEFINE:
                self._ItemType = -1
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
                            self._Owner[-1],
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
        Macros = self._Macros
        if Macros:
            for Index in range(0, len(self._ValueList)):
                Value = self._ValueList[Index]
                if not Value:
                    continue

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

                self._ValueList[Index] = ReplaceMacro(Value, Macros)

    ## Parse [Sources] section
    #
    #   Only path can have macro used. So we need to replace them before use.
    #
    @ParseMacro
    def _SourceFileParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT)
        self._ValueList[0:len(TokenList)] = TokenList
        Macros = self._Macros
        # For Acpi tables, remove macro like ' TABLE_NAME=Sata1'
        if 'COMPONENT_TYPE' in Macros:
            if self._Defines['COMPONENT_TYPE'].upper() == 'ACPITABLE':
                self._ValueList[0] = GetSplitValueList(self._ValueList[0], ' ', 1)[0]
        if self._Defines['BASE_NAME'] == 'Microcode':
            pass
        self._ValueList = [ReplaceMacro(Value, Macros) for Value in self._ValueList]

    ## Parse [Binaries] section
    #
    #   Only path can have macro used. So we need to replace them before use.
    #
    @ParseMacro
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
        self._ValueList[1] = ReplaceMacro(self._ValueList[1], self._Macros)

    ## [nmake] section parser (Edk.x style only)
    def _NmakeParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_EQUAL_SPLIT, 1)
        self._ValueList[0:len(TokenList)] = TokenList
        # remove macros
        self._ValueList[1] = ReplaceMacro(self._ValueList[1], self._Macros)
        # remove self-reference in macro setting
        #self._ValueList[1] = ReplaceMacro(self._ValueList[1], {self._ValueList[0]:''})

    ## [FixedPcd], [FeaturePcd], [PatchPcd], [Pcd] and [PcdEx] sections parser
    @ParseMacro
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
    @ParseMacro
    def _DepexParser(self):
        self._ValueList[0:1] = [self._CurrentLine]

    _SectionParser = {
        MODEL_UNKNOWN                   :   MetaFileParser._Skip,
        MODEL_META_DATA_HEADER          :   MetaFileParser._DefineParser,
        MODEL_META_DATA_BUILD_OPTION    :   MetaFileParser._BuildOptionParser,
        MODEL_EFI_INCLUDE               :   _IncludeParser,                 # for Edk.x modules
        MODEL_EFI_LIBRARY_INSTANCE      :   MetaFileParser._CommonParser,   # for Edk.x modules
        MODEL_EFI_LIBRARY_CLASS         :   MetaFileParser._PathParser,
        MODEL_META_DATA_PACKAGE         :   MetaFileParser._PathParser,
        MODEL_META_DATA_NMAKE           :   _NmakeParser,                   # for Edk.x modules
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
        TAB_DSC_DEFINES_DEFINE                      :   MODEL_META_DATA_DEFINE,
        TAB_DSC_DEFINES_EDKGLOBAL                   :   MODEL_META_DATA_GLOBAL_DEFINE,
        TAB_INCLUDE.upper()                         :   MODEL_META_DATA_INCLUDE,
        TAB_IF.upper()                              :   MODEL_META_DATA_CONDITIONAL_STATEMENT_IF,
        TAB_IF_DEF.upper()                          :   MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF,
        TAB_IF_N_DEF.upper()                        :   MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF,
        TAB_ELSE_IF.upper()                         :   MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSEIF,
        TAB_ELSE.upper()                            :   MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE,
        TAB_END_IF.upper()                          :   MODEL_META_DATA_CONDITIONAL_STATEMENT_ENDIF,
    }

    # Valid names in define section
    DefineKeywords = [
        "DSC_SPECIFICATION",
        "PLATFORM_NAME",
        "PLATFORM_GUID",
        "PLATFORM_VERSION",
        "SKUID_IDENTIFIER",
        "SUPPORTED_ARCHITECTURES",
        "BUILD_TARGETS",
        "OUTPUT_DIRECTORY",
        "FLASH_DEFINITION",
        "BUILD_NUMBER",
        "RFC_LANGUAGES",
        "ISO_LANGUAGES",
        "TIME_STAMP_FILE",
        "VPD_TOOL_GUID",
        "FIX_LOAD_TOP_MEMORY_ADDRESS"
    ]

    SymbolPattern = ValueExpression.SymbolPattern

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
    def __init__(self, FilePath, FileType, Table, Owner=-1, From=-1):
        # prevent re-initialization
        if hasattr(self, "_Table"):
            return
        MetaFileParser.__init__(self, FilePath, FileType, Table, Owner, From)
        self._Version = 0x00010005  # Only EDK2 dsc file is supported
        # to store conditional directive evaluation result
        self._DirectiveStack = []
        self._DirectiveEvalStack = []
        self._Enabled = 1
        
        #
        # Specify whether current line is in uncertain condition
        #
        self._InDirective = -1
        
        # Final valid replacable symbols
        self._Symbols = {}
        #
        #  Map the ID between the original table and new table to track
        #  the owner item
        #
        self._IdMapping = {-1:-1}

    ## Parser starter
    def Start(self):
        Content = ''
        try:
            Content = open(str(self.MetaFile), 'r').readlines()
        except:
            EdkLogger.error("Parser", FILE_READ_FAILURE, ExtraData=self.MetaFile)

        for Index in range(0, len(Content)):
            Line = CleanString(Content[Index])
            # skip empty line
            if Line == '':
                continue

            self._CurrentLine = Line
            self._LineIndex = Index
            if self._InSubsection and self._Owner[-1] == -1:
                self._Owner.append(self._LastItem)
            
            # section header
            if Line[0] == TAB_SECTION_START and Line[-1] == TAB_SECTION_END:
                self._SectionType = MODEL_META_DATA_SECTION_HEADER
            # subsection ending
            elif Line[0] == '}' and self._InSubsection:
                self._InSubsection = False
                self._SubsectionType = MODEL_UNKNOWN
                self._SubsectionName = ''
                self._Owner[-1] = -1
                continue
            # subsection header
            elif Line[0] == TAB_OPTION_START and Line[-1] == TAB_OPTION_END:
                self._SubsectionType = MODEL_META_DATA_SUBSECTION_HEADER
            # directive line
            elif Line[0] == '!':
                self._DirectiveParser()
                continue

            if self._InSubsection:
                SectionType = self._SubsectionType
            else:
                SectionType = self._SectionType
            self._ItemType = SectionType

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
                                        self._ItemType,
                                        self._ValueList[0],
                                        self._ValueList[1],
                                        self._ValueList[2],
                                        Arch,
                                        ModuleType,
                                        self._Owner[-1],
                                        self._From,
                                        self._LineIndex+1,
                                        -1,
                                        self._LineIndex+1,
                                        -1,
                                        self._Enabled
                                        )

        if self._DirectiveStack:
            Type, Line, Text = self._DirectiveStack[-1]
            EdkLogger.error('Parser', FORMAT_INVALID, "No matching '!endif' found",
                            ExtraData=Text, File=self.MetaFile, Line=Line)
        self._Done()

    ## <subsection_header> parser
    def _SubsectionHeaderParser(self):
        self._SubsectionName = self._CurrentLine[1:-1].upper()
        if self._SubsectionName in self.DataType:
            self._SubsectionType = self.DataType[self._SubsectionName]
        else:
            self._SubsectionType = MODEL_UNKNOWN
            EdkLogger.warn("Parser", "Unrecognized sub-section", File=self.MetaFile,
                           Line=self._LineIndex+1, ExtraData=self._CurrentLine)
        self._ValueList[0] = self._SubsectionName

    ## Directive statement parser
    def _DirectiveParser(self):
        self._ValueList = ['','','']
        TokenList = GetSplitValueList(self._CurrentLine, ' ', 1)
        self._ValueList[0:len(TokenList)] = TokenList

        # Syntax check
        DirectiveName = self._ValueList[0].upper()
        if DirectiveName not in self.DataType:
            EdkLogger.error("Parser", FORMAT_INVALID, "Unknown directive [%s]" % DirectiveName,
                            File=self.MetaFile, Line=self._LineIndex+1)

        if DirectiveName in ['!IF', '!IFDEF', '!IFNDEF']:
            self._InDirective += 1

        if DirectiveName in ['!ENDIF']:
            self._InDirective -= 1

        if DirectiveName in ['!IF', '!IFDEF', '!INCLUDE', '!IFNDEF', '!ELSEIF'] and self._ValueList[1] == '':
            EdkLogger.error("Parser", FORMAT_INVALID, "Missing expression",
                            File=self.MetaFile, Line=self._LineIndex+1,
                            ExtraData=self._CurrentLine)

        ItemType = self.DataType[DirectiveName]
        Scope = [['COMMON', 'COMMON']]
        if ItemType == MODEL_META_DATA_INCLUDE:
            Scope = self._Scope
        if ItemType == MODEL_META_DATA_CONDITIONAL_STATEMENT_ENDIF:
            # Remove all directives between !if and !endif, including themselves
            while self._DirectiveStack:
                # Remove any !else or !elseif
                DirectiveInfo = self._DirectiveStack.pop()
                if DirectiveInfo[0] in [MODEL_META_DATA_CONDITIONAL_STATEMENT_IF,
                                        MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF,
                                        MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF]:
                    break
            else:
                EdkLogger.error("Parser", FORMAT_INVALID, "Redundant '!endif'",
                                File=self.MetaFile, Line=self._LineIndex+1,
                                ExtraData=self._CurrentLine)
        elif ItemType != MODEL_META_DATA_INCLUDE:
            # Break if there's a !else is followed by a !elseif
            if ItemType == MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSEIF and \
               self._DirectiveStack and \
               self._DirectiveStack[-1][0] == MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE:
                EdkLogger.error("Parser", FORMAT_INVALID, "'!elseif' after '!else'",
                                File=self.MetaFile, Line=self._LineIndex+1,
                                ExtraData=self._CurrentLine)
            self._DirectiveStack.append((ItemType, self._LineIndex+1, self._CurrentLine))
        elif self._From > 0:
            EdkLogger.error('Parser', FORMAT_INVALID,
                            "No '!include' allowed in included file",
                            ExtraData=self._CurrentLine, File=self.MetaFile, 
                            Line=self._LineIndex+1)

        #
        # Model, Value1, Value2, Value3, Arch, ModuleType, BelongsToItem=-1, BelongsToFile=-1,
        # LineBegin=-1, ColumnBegin=-1, LineEnd=-1, ColumnEnd=-1, Enabled=-1
        #
        for Arch, ModuleType in Scope:
            self._LastItem = self._Store(
                                    ItemType,
                                    self._ValueList[0],
                                    self._ValueList[1],
                                    self._ValueList[2],
                                    Arch,
                                    ModuleType,
                                    self._Owner[-1],
                                    self._From,
                                    self._LineIndex+1,
                                    -1,
                                    self._LineIndex+1,
                                    -1,
                                    0
                                    )

    ## [defines] section parser
    @ParseMacro
    def _DefineParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_EQUAL_SPLIT, 1)
        self._ValueList[1:len(TokenList)] = TokenList

        # Syntax check
        if not self._ValueList[1]:
            EdkLogger.error('Parser', FORMAT_INVALID, "No name specified",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        if not self._ValueList[2]:
            EdkLogger.error('Parser', FORMAT_INVALID, "No value specified",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        if not self._ValueList[1] in self.DefineKeywords:
            EdkLogger.error('Parser', FORMAT_INVALID,
                            "Unknown keyword found: %s. "
                            "If this is a macro you must "
                            "add it as a DEFINE in the DSC" % self._ValueList[1],
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        self._Defines[self._ValueList[1]] = self._ValueList[2]
        self._ItemType = self.DataType[TAB_DSC_DEFINES.upper()]

    @ParseMacro
    def _SkuIdParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT)
        if len(TokenList) != 2:
            EdkLogger.error('Parser', FORMAT_INVALID, "Correct format is '<Integer>|<UiName>'",
                            ExtraData=self._CurrentLine, File=self.MetaFile, Line=self._LineIndex+1)
        self._ValueList[0:len(TokenList)] = TokenList

    ## Parse Edk style of library modules
    @ParseMacro
    def _LibraryInstanceParser(self):
        self._ValueList[0] = self._CurrentLine

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
    @ParseMacro
    def _PcdParser(self):
        TokenList = GetSplitValueList(self._CurrentLine, TAB_VALUE_SPLIT, 1)
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
    @ParseMacro
    def _ComponentParser(self):
        if self._CurrentLine[-1] == '{':
            self._ValueList[0] = self._CurrentLine[0:-1].strip()
            self._InSubsection = True
        else:
            self._ValueList[0] = self._CurrentLine

    ## [LibraryClasses] section
    @ParseMacro
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

    def _CompponentSourceOverridePathParser(self):
        self._ValueList[0] = self._CurrentLine

    ## [BuildOptions] section parser
    @ParseMacro
    def _BuildOptionParser(self):
        self._CurrentLine = CleanString(self._CurrentLine, BuildOption=True)
        TokenList = GetSplitValueList(self._CurrentLine, TAB_EQUAL_SPLIT, 1)
        TokenList2 = GetSplitValueList(TokenList[0], ':', 1)
        if len(TokenList2) == 2:
            self._ValueList[0] = TokenList2[0]  # toolchain family
            self._ValueList[1] = TokenList2[1]  # keys
        else:
            self._ValueList[1] = TokenList[0]
        if len(TokenList) == 2:                 # value
            self._ValueList[2] = TokenList[1]

        if self._ValueList[1].count('_') != 4:
            EdkLogger.error(
                'Parser',
                FORMAT_INVALID,
                "'%s' must be in format of <TARGET>_<TOOLCHAIN>_<ARCH>_<TOOL>_FLAGS" % self._ValueList[1],
                ExtraData=self._CurrentLine,
                File=self.MetaFile,
                Line=self._LineIndex+1
                )

    ## Override parent's method since we'll do all macro replacements in parser
    def _GetMacros(self):
        Macros = {}
        Macros.update(self._FileLocalMacros)
        Macros.update(self._GetApplicableSectionMacro())
        Macros.update(GlobalData.gEdkGlobal)
        Macros.update(GlobalData.gPlatformDefines)
        Macros.update(GlobalData.gCommandLineDefines)
        # PCD cannot be referenced in macro definition
        if self._ItemType not in [MODEL_META_DATA_DEFINE, MODEL_META_DATA_GLOBAL_DEFINE]:
            Macros.update(self._Symbols)
        return Macros

    def _PostProcess(self):
        Processer = {
            MODEL_META_DATA_SECTION_HEADER                  :   self.__ProcessSectionHeader,
            MODEL_META_DATA_SUBSECTION_HEADER               :   self.__ProcessSubsectionHeader,
            MODEL_META_DATA_HEADER                          :   self.__ProcessDefine,
            MODEL_META_DATA_DEFINE                          :   self.__ProcessDefine,
            MODEL_META_DATA_GLOBAL_DEFINE                   :   self.__ProcessDefine,
            MODEL_META_DATA_INCLUDE                         :   self.__ProcessDirective,
            MODEL_META_DATA_CONDITIONAL_STATEMENT_IF        :   self.__ProcessDirective,
            MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE      :   self.__ProcessDirective,
            MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF     :   self.__ProcessDirective,
            MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF    :   self.__ProcessDirective,
            MODEL_META_DATA_CONDITIONAL_STATEMENT_ENDIF     :   self.__ProcessDirective,
            MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSEIF    :   self.__ProcessDirective,
            MODEL_EFI_SKU_ID                                :   self.__ProcessSkuId,
            MODEL_EFI_LIBRARY_INSTANCE                      :   self.__ProcessLibraryInstance,
            MODEL_EFI_LIBRARY_CLASS                         :   self.__ProcessLibraryClass,
            MODEL_PCD_FIXED_AT_BUILD                        :   self.__ProcessPcd,
            MODEL_PCD_PATCHABLE_IN_MODULE                   :   self.__ProcessPcd,
            MODEL_PCD_FEATURE_FLAG                          :   self.__ProcessPcd,
            MODEL_PCD_DYNAMIC_DEFAULT                       :   self.__ProcessPcd,
            MODEL_PCD_DYNAMIC_HII                           :   self.__ProcessPcd,
            MODEL_PCD_DYNAMIC_VPD                           :   self.__ProcessPcd,
            MODEL_PCD_DYNAMIC_EX_DEFAULT                    :   self.__ProcessPcd,
            MODEL_PCD_DYNAMIC_EX_HII                        :   self.__ProcessPcd,
            MODEL_PCD_DYNAMIC_EX_VPD                        :   self.__ProcessPcd,
            MODEL_META_DATA_COMPONENT                       :   self.__ProcessComponent,
            MODEL_META_DATA_COMPONENT_SOURCE_OVERRIDE_PATH  :   self.__ProcessSourceOverridePath,
            MODEL_META_DATA_BUILD_OPTION                    :   self.__ProcessBuildOption,
            MODEL_UNKNOWN                                   :   self._Skip,
            MODEL_META_DATA_USER_EXTENSION                  :   self._Skip,
        }

        self._Table = MetaFileStorage(self._RawTable.Cur, self.MetaFile, MODEL_FILE_DSC, True)
        self._Table.Create()
        self._DirectiveStack = []
        self._DirectiveEvalStack = []
        self._FileWithError = self.MetaFile
        self._FileLocalMacros = {}
        self._SectionsMacroDict = {}
        GlobalData.gPlatformDefines = {}

        # Get all macro and PCD which has straitforward value
        self.__RetrievePcdValue()
        self._Content = self._RawTable.GetAll()
        self._ContentIndex = 0
        while self._ContentIndex < len(self._Content) :
            Id, self._ItemType, V1, V2, V3, S1, S2, Owner, self._From, \
                LineStart, ColStart, LineEnd, ColEnd, Enabled = self._Content[self._ContentIndex]

            if self._From < 0:
                self._FileWithError = self.MetaFile

            self._ContentIndex += 1

            self._Scope = [[S1, S2]]
            #
            # For !include directive, handle it specially,
            # merge arch and module type in case of duplicate items
            #
            while self._ItemType == MODEL_META_DATA_INCLUDE:
                if self._ContentIndex >= len(self._Content):
                    break
                Record = self._Content[self._ContentIndex]
                if LineStart == Record[9] and LineEnd == Record[11]:
                    if [Record[5], Record[6]] not in self._Scope:
                        self._Scope.append([Record[5], Record[6]])
                    self._ContentIndex += 1
                else:
                    break

            self._LineIndex = LineStart - 1
            self._ValueList = [V1, V2, V3]

            try:
                Processer[self._ItemType]()
            except EvaluationException, Excpt:
                # 
                # Only catch expression evaluation error here. We need to report
                # the precise number of line on which the error occurred
                #
                if hasattr(Excpt, 'Pcd'):
                    if Excpt.Pcd in GlobalData.gPlatformOtherPcds:
                        Info = GlobalData.gPlatformOtherPcds[Excpt.Pcd]
                        EdkLogger.error('Parser', FORMAT_INVALID, "Cannot use this PCD (%s) in an expression as"
                                        " it must be defined in a [PcdsFixedAtBuild] or [PcdsFeatureFlag] section"
                                        " of the DSC file, and it is currently defined in this section:"
                                        " %s, line #: %d." % (Excpt.Pcd, Info[0], Info[1]),
                                    File=self._FileWithError, ExtraData=' '.join(self._ValueList), 
                                    Line=self._LineIndex+1)
                    else:
                        EdkLogger.error('Parser', FORMAT_INVALID, "PCD (%s) is not defined in DSC file" % Excpt.Pcd,
                                    File=self._FileWithError, ExtraData=' '.join(self._ValueList), 
                                    Line=self._LineIndex+1)
                else:
                    EdkLogger.error('Parser', FORMAT_INVALID, "Invalid expression: %s" % str(Excpt),
                                    File=self._FileWithError, ExtraData=' '.join(self._ValueList), 
                                    Line=self._LineIndex+1)
            except MacroException, Excpt:
                EdkLogger.error('Parser', FORMAT_INVALID, str(Excpt),
                                File=self._FileWithError, ExtraData=' '.join(self._ValueList), 
                                Line=self._LineIndex+1)

            if self._ValueList == None:
                continue 

            NewOwner = self._IdMapping.get(Owner, -1)
            self._Enabled = int((not self._DirectiveEvalStack) or (False not in self._DirectiveEvalStack))
            self._LastItem = self._Store(
                                self._ItemType,
                                self._ValueList[0],
                                self._ValueList[1],
                                self._ValueList[2],
                                S1,
                                S2,
                                NewOwner,
                                self._From,
                                self._LineIndex+1,
                                -1,
                                self._LineIndex+1,
                                -1,
                                self._Enabled
                                )
            self._IdMapping[Id] = self._LastItem

        GlobalData.gPlatformDefines.update(self._FileLocalMacros)
        self._PostProcessed = True
        self._Content = None

    def __ProcessSectionHeader(self):
        self._SectionName = self._ValueList[0]
        if self._SectionName in self.DataType:
            self._SectionType = self.DataType[self._SectionName]
        else:
            self._SectionType = MODEL_UNKNOWN

    def __ProcessSubsectionHeader(self):
        self._SubsectionName = self._ValueList[0]
        if self._SubsectionName in self.DataType:
            self._SubsectionType = self.DataType[self._SubsectionName]
        else:
            self._SubsectionType = MODEL_UNKNOWN

    def __RetrievePcdValue(self):
        Records = self._RawTable.Query(MODEL_PCD_FEATURE_FLAG, BelongsToItem=-1.0)
        for TokenSpaceGuid,PcdName,Value,Dummy2,Dummy3,ID,Line in Records:
            Value, DatumType, MaxDatumSize = AnalyzePcdData(Value)
            Name = TokenSpaceGuid + '.' + PcdName
            self._Symbols[Name] = Value

        Records = self._RawTable.Query(MODEL_PCD_FIXED_AT_BUILD, BelongsToItem=-1.0)
        for TokenSpaceGuid,PcdName,Value,Dummy2,Dummy3,ID,Line in Records:
            Value, DatumType, MaxDatumSize = AnalyzePcdData(Value)
            Name = TokenSpaceGuid + '.' + PcdName
            self._Symbols[Name] = Value

        Content = open(str(self.MetaFile), 'r').readlines()
        GlobalData.gPlatformOtherPcds['DSCFILE'] = str(self.MetaFile)
        for PcdType in (MODEL_PCD_PATCHABLE_IN_MODULE, MODEL_PCD_DYNAMIC_DEFAULT, MODEL_PCD_DYNAMIC_HII,
                        MODEL_PCD_DYNAMIC_VPD, MODEL_PCD_DYNAMIC_EX_DEFAULT, MODEL_PCD_DYNAMIC_EX_HII,
                        MODEL_PCD_DYNAMIC_EX_VPD):
            Records = self._RawTable.Query(PcdType, BelongsToItem=-1.0)
            for TokenSpaceGuid,PcdName,Value,Dummy2,Dummy3,ID,Line in Records:
                Name = TokenSpaceGuid + '.' + PcdName
                if Name not in GlobalData.gPlatformOtherPcds:
                    PcdLine = Line
                    while not Content[Line - 1].lstrip().startswith(TAB_SECTION_START):
                        Line -= 1
                    GlobalData.gPlatformOtherPcds[Name] = (CleanString(Content[Line - 1]), PcdLine, PcdType)

    def __ProcessDefine(self):
        if not self._Enabled:
            return

        Type, Name, Value = self._ValueList
        Value = ReplaceMacro(Value, self._Macros, False)
        if self._ItemType == MODEL_META_DATA_DEFINE:
            if self._SectionType == MODEL_META_DATA_HEADER:
                self._FileLocalMacros[Name] = Value
            else:
                self._ConstructSectionMacroDict(Name, Value)
        elif self._ItemType == MODEL_META_DATA_GLOBAL_DEFINE:
            GlobalData.gEdkGlobal[Name] = Value
        
        #
        # Keyword in [Defines] section can be used as Macros
        #
        if (self._ItemType == MODEL_META_DATA_HEADER) and (self._SectionType == MODEL_META_DATA_HEADER):
            self._FileLocalMacros[Name] = Value
            
        self._ValueList = [Type, Name, Value]

    def __ProcessDirective(self):
        Result = None
        if self._ItemType in [MODEL_META_DATA_CONDITIONAL_STATEMENT_IF,
                              MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSEIF]:
            Macros = self._Macros
            Macros.update(GlobalData.gGlobalDefines)
            try:
                Result = ValueExpression(self._ValueList[1], Macros)()
            except SymbolNotFound, Exc:
                EdkLogger.debug(EdkLogger.DEBUG_5, str(Exc), self._ValueList[1])
                Result = False
            except WrnExpression, Excpt:
                # 
                # Catch expression evaluation warning here. We need to report
                # the precise number of line and return the evaluation result
                #
                EdkLogger.warn('Parser', "Suspicious expression: %s" % str(Excpt),
                                File=self._FileWithError, ExtraData=' '.join(self._ValueList), 
                                Line=self._LineIndex+1)
                Result = Excpt.result

        if self._ItemType in [MODEL_META_DATA_CONDITIONAL_STATEMENT_IF,
                              MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF,
                              MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF]:
            self._DirectiveStack.append(self._ItemType)
            if self._ItemType == MODEL_META_DATA_CONDITIONAL_STATEMENT_IF:
                Result = bool(Result)
            else:
                Macro = self._ValueList[1]
                Macro = Macro[2:-1] if (Macro.startswith("$(") and Macro.endswith(")")) else Macro
                Result = Macro in self._Macros
                if self._ItemType == MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF:
                    Result = not Result
            self._DirectiveEvalStack.append(Result)
        elif self._ItemType == MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSEIF:
            self._DirectiveStack.append(self._ItemType)
            self._DirectiveEvalStack[-1] = not self._DirectiveEvalStack[-1]
            self._DirectiveEvalStack.append(bool(Result))
        elif self._ItemType == MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE:
            self._DirectiveStack.append(self._ItemType)
            self._DirectiveEvalStack[-1] = not self._DirectiveEvalStack[-1]
            self._DirectiveEvalStack.append(True)
        elif self._ItemType == MODEL_META_DATA_CONDITIONAL_STATEMENT_ENDIF:
            # Back to the nearest !if/!ifdef/!ifndef
            while self._DirectiveStack:
                self._DirectiveEvalStack.pop()
                Directive = self._DirectiveStack.pop()
                if Directive in [MODEL_META_DATA_CONDITIONAL_STATEMENT_IF,
                                 MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF,
                                 MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF]:
                    break
        elif self._ItemType == MODEL_META_DATA_INCLUDE:
            # The included file must be relative to workspace or same directory as DSC file
            __IncludeMacros = {}
            #
            # Allow using system environment variables  in path after !include
            #
            __IncludeMacros['WORKSPACE'] = GlobalData.gGlobalDefines['WORKSPACE']
            if "ECP_SOURCE" in GlobalData.gGlobalDefines.keys():
                __IncludeMacros['ECP_SOURCE'] = GlobalData.gGlobalDefines['ECP_SOURCE']
            #
            # During GenFds phase call DSC parser, will go into this branch.
            #
            elif "ECP_SOURCE" in GlobalData.gCommandLineDefines.keys():
                __IncludeMacros['ECP_SOURCE'] = GlobalData.gCommandLineDefines['ECP_SOURCE']

            __IncludeMacros['EFI_SOURCE'] = GlobalData.gGlobalDefines['EFI_SOURCE']
            __IncludeMacros['EDK_SOURCE'] = GlobalData.gGlobalDefines['EDK_SOURCE']
            #
            # Allow using MACROs comes from [Defines] section to keep compatible. 
            #
            __IncludeMacros.update(self._Macros)
            
            IncludedFile = NormPath(ReplaceMacro(self._ValueList[1], __IncludeMacros, RaiseError=True))
            #
            # First search the include file under the same directory as DSC file
            #
            IncludedFile1 = PathClass(IncludedFile, self.MetaFile.Dir)
            ErrorCode, ErrorInfo1 = IncludedFile1.Validate()
            if ErrorCode != 0:
                #
                # Also search file under the WORKSPACE directory
                #
                IncludedFile1 = PathClass(IncludedFile, GlobalData.gWorkspace)
                ErrorCode, ErrorInfo2 = IncludedFile1.Validate()
                if ErrorCode != 0:
                    EdkLogger.error('parser', ErrorCode, File=self._FileWithError, 
                                    Line=self._LineIndex+1, ExtraData=ErrorInfo1 + "\n"+ ErrorInfo2)

            self._FileWithError = IncludedFile1

            IncludedFileTable = MetaFileStorage(self._Table.Cur, IncludedFile1, MODEL_FILE_DSC, False)
            Owner = self._Content[self._ContentIndex-1][0]
            Parser = DscParser(IncludedFile1, self._FileType, IncludedFileTable, 
                               Owner=Owner, From=Owner)

            # set the parser status with current status
            Parser._SectionName = self._SectionName
            Parser._SectionType = self._SectionType
            Parser._Scope = self._Scope
            Parser._Enabled = self._Enabled
            # Parse the included file
            Parser.Start()

            # update current status with sub-parser's status
            self._SectionName = Parser._SectionName
            self._SectionType = Parser._SectionType
            self._Scope       = Parser._Scope
            self._Enabled     = Parser._Enabled

            # Insert all records in the table for the included file into dsc file table
            Records = IncludedFileTable.GetAll()
            if Records:
                self._Content[self._ContentIndex:self._ContentIndex] = Records
                self._Content.pop(self._ContentIndex-1)
                self._ValueList = None
                self._ContentIndex -= 1
                
    def __ProcessSkuId(self):
        self._ValueList = [ReplaceMacro(Value, self._Macros, RaiseError=True)
                           for Value in self._ValueList]

    def __ProcessLibraryInstance(self):
        self._ValueList = [ReplaceMacro(Value, self._Macros) for Value in self._ValueList]

    def __ProcessLibraryClass(self):
        self._ValueList[1] = ReplaceMacro(self._ValueList[1], self._Macros, RaiseError=True)

    def __ProcessPcd(self):
        PcdValue = None
        ValueList = GetSplitValueList(self._ValueList[2])
        #
        # PCD value can be an expression
        #
        if len(ValueList) > 1 and ValueList[1] == 'VOID*':
            PcdValue = ValueList[0]      
            try:
                ValueList[0] = ValueExpression(PcdValue, self._Macros)(True)
            except WrnExpression, Value:
                ValueList[0] = Value.result
            PcdValue = ValueList[0]
        else:
            #
            # Int*/Boolean VPD PCD
            # TokenSpace | PcdCName | Offset | [Value]
            # 
            # VOID* VPD PCD
            # TokenSpace | PcdCName | Offset | [Size] | [Value]
            #
            if self._ItemType == MODEL_PCD_DYNAMIC_VPD:
                if len(ValueList) >= 4:
                    PcdValue = ValueList[-1]
            else:
                PcdValue = ValueList[-1]
            #
            # For the VPD PCD, there may not have PcdValue data in DSC file
            #
            if PcdValue:
                try:
                    ValueList[-1] = ValueExpression(PcdValue, self._Macros)(True)
                except WrnExpression, Value:
                    ValueList[-1] = Value.result
                
                if ValueList[-1] == 'True':
                    ValueList[-1] = '1'
                if ValueList[-1] == 'False':
                    ValueList[-1] = '0'
                PcdValue = ValueList[-1]
        if PcdValue and self._ItemType in [MODEL_PCD_FEATURE_FLAG, MODEL_PCD_FIXED_AT_BUILD]:
            GlobalData.gPlatformPcds[TAB_SPLIT.join(self._ValueList[0:2])] = PcdValue
        self._ValueList[2] = '|'.join(ValueList)

    def __ProcessComponent(self):
        self._ValueList[0] = ReplaceMacro(self._ValueList[0], self._Macros)

    def __ProcessSourceOverridePath(self):
        self._ValueList[0] = ReplaceMacro(self._ValueList[0], self._Macros)

    def __ProcessBuildOption(self):
        self._ValueList = [ReplaceMacro(Value, self._Macros, RaiseError=False)
                           for Value in self._ValueList]

    _SectionParser = {
        MODEL_META_DATA_HEADER                          :   _DefineParser,
        MODEL_EFI_SKU_ID                                :   _SkuIdParser,
        MODEL_EFI_LIBRARY_INSTANCE                      :   _LibraryInstanceParser,
        MODEL_EFI_LIBRARY_CLASS                         :   _LibraryClassParser,
        MODEL_PCD_FIXED_AT_BUILD                        :   _PcdParser,
        MODEL_PCD_PATCHABLE_IN_MODULE                   :   _PcdParser,
        MODEL_PCD_FEATURE_FLAG                          :   _PcdParser,
        MODEL_PCD_DYNAMIC_DEFAULT                       :   _PcdParser,
        MODEL_PCD_DYNAMIC_HII                           :   _PcdParser,
        MODEL_PCD_DYNAMIC_VPD                           :   _PcdParser,
        MODEL_PCD_DYNAMIC_EX_DEFAULT                    :   _PcdParser,
        MODEL_PCD_DYNAMIC_EX_HII                        :   _PcdParser,
        MODEL_PCD_DYNAMIC_EX_VPD                        :   _PcdParser,
        MODEL_META_DATA_COMPONENT                       :   _ComponentParser,
        MODEL_META_DATA_COMPONENT_SOURCE_OVERRIDE_PATH  :   _CompponentSourceOverridePathParser,
        MODEL_META_DATA_BUILD_OPTION                    :   _BuildOptionParser,
        MODEL_UNKNOWN                                   :   MetaFileParser._Skip,
        MODEL_META_DATA_USER_EXTENSION                  :   MetaFileParser._Skip,
        MODEL_META_DATA_SECTION_HEADER                  :   MetaFileParser._SectionHeaderParser,
        MODEL_META_DATA_SUBSECTION_HEADER               :   _SubsectionHeaderParser,
    }

    _Macros     = property(_GetMacros)

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
        TAB_DSC_DEFINES_DEFINE                      :   MODEL_META_DATA_DEFINE,
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
    def __init__(self, FilePath, FileType, Table):
        # prevent re-initialization
        if hasattr(self, "_Table"):
            return
        MetaFileParser.__init__(self, FilePath, FileType, Table, -1)
        self._Comments = []
        self._Version = 0x00010005  # Only EDK2 dec file is supported

    ## Parser starter
    def Start(self):
        Content = ''
        try:
            Content = open(str(self.MetaFile), 'r').readlines()
        except:
            EdkLogger.error("Parser", FILE_READ_FAILURE, ExtraData=self.MetaFile)

        for Index in range(0, len(Content)):
            Line, Comment = CleanString2(Content[Index])
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
            elif len(self._SectionType) == 0:
                self._Comments = []
                continue

            # section content
            self._ValueList = ['','','']
            self._SectionParser[self._SectionType[0]](self)
            if self._ValueList == None or self._ItemType == MODEL_META_DATA_DEFINE:
                self._ItemType = -1
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
                    self._Owner[-1],
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
    @ParseMacro
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
    @ParseMacro
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

