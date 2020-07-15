## @file
# This file contained the parser for sections in INF file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
InfSectionParser
'''
##
# Import Modules
#
from copy import deepcopy
import re

from Library.StringUtils import GetSplitValueList
from Library.CommentParsing import ParseHeaderCommentSection
from Library.CommentParsing import ParseComment

from Library import DataType as DT

import Logger.Log as Logger
from Logger import StringTable as ST
from Logger.ToolError import FORMAT_INVALID

from Object.Parser.InfDefineObject import InfDefObject
from Object.Parser.InfBuildOptionObject import InfBuildOptionsObject
from Object.Parser.InfLibraryClassesObject import InfLibraryClassObject
from Object.Parser.InfPackagesObject import InfPackageObject
from Object.Parser.InfPcdObject import InfPcdObject
from Object.Parser.InfSoucesObject import InfSourcesObject
from Object.Parser.InfUserExtensionObject import InfUserExtensionObject
from Object.Parser.InfProtocolObject import InfProtocolObject
from Object.Parser.InfPpiObject import InfPpiObject
from Object.Parser.InfGuidObject import InfGuidObject
from Object.Parser.InfDepexObject import InfDepexObject
from Object.Parser.InfBinaryObject import InfBinariesObject
from Object.Parser.InfHeaderObject import InfHeaderObject
from Object.Parser.InfMisc import InfSpecialCommentObject
from Object.Parser.InfMisc import InfHobObject
from Object.Parser.InfMisc import InfBootModeObject
from Object.Parser.InfMisc import InfEventObject
from Parser.InfParserMisc import gINF_SECTION_DEF
from Parser.InfDefineSectionParser import InfDefinSectionParser
from Parser.InfBuildOptionSectionParser import InfBuildOptionSectionParser
from Parser.InfSourceSectionParser import InfSourceSectionParser
from Parser.InfLibrarySectionParser import InfLibrarySectionParser
from Parser.InfPackageSectionParser import InfPackageSectionParser
from Parser.InfGuidPpiProtocolSectionParser import InfGuidPpiProtocolSectionParser
from Parser.InfBinarySectionParser import InfBinarySectionParser
from Parser.InfPcdSectionParser import InfPcdSectionParser
from Parser.InfDepexSectionParser import InfDepexSectionParser

## GetSpecialStr2
#
# GetSpecialStr2
#
def GetSpecialStr2(ItemList, FileName, LineNo, SectionString):
    Str2 = ''
    #
    # S2 may be Platform or ModuleType
    #
    if len(ItemList) == 3:
        #
        # Except [LibraryClass], [Depex]
        # section can has more than 2 items in section header string,
        # others should report error.
        #
        if not (ItemList[0].upper() == DT.TAB_LIBRARY_CLASSES.upper() or \
                ItemList[0].upper() == DT.TAB_DEPEX.upper() or \
                ItemList[0].upper() == DT.TAB_USER_EXTENSIONS.upper()):
            if ItemList[2] != '':
                Logger.Error('Parser',
                             FORMAT_INVALID,
                             ST.ERR_INF_PARSER_SOURCE_SECTION_SECTIONNAME_INVALID % (SectionString),
                             File=FileName,
                             Line=LineNo,
                             ExtraData=SectionString)
        Str2 = ItemList[2]
    elif len(ItemList) == 4:
        #
        # Except [UserExtension]
        # section can has 4 items in section header string,
        # others should report error.
        #
        if not ItemList[0].upper() == DT.TAB_USER_EXTENSIONS.upper() or ItemList[0].upper() == DT.TAB_DEPEX.upper():
            if ItemList[3] != '':
                Logger.Error('Parser', FORMAT_INVALID, ST.ERR_INF_PARSER_SOURCE_SECTION_SECTIONNAME_INVALID \
                             % (SectionString), File=FileName, Line=LineNo, ExtraData=SectionString)

        if not ItemList[0].upper() == DT.TAB_USER_EXTENSIONS.upper():
            Str2 = ItemList[2] + ' | ' + ItemList[3]
        else:
            Str2 = ItemList[2]

    elif len(ItemList) > 4:
        Logger.Error('Parser', FORMAT_INVALID, ST.ERR_INF_PARSER_SOURCE_SECTION_SECTIONNAME_INVALID \
                     % (SectionString), File=FileName, Line=LineNo, ExtraData=SectionString)

    return Str2

## ProcessUseExtHeader
#
#
def ProcessUseExtHeader(ItemList):
    NewItemList = []
    AppendContent = ''
    CompleteFlag = False
    for Item in ItemList:
        if Item.startswith('\"') and not Item.endswith('\"'):
            AppendContent = Item
            CompleteFlag = True
        elif Item.endswith('\"') and not Item.startswith('\"'):
            #
            # Should not have an userId or IdString not starts with " before but ends with ".
            #
            if not CompleteFlag:
                return False, []
            AppendContent = AppendContent + "." + Item
            NewItemList.append(AppendContent)
            CompleteFlag = False
            AppendContent = ''
        elif Item.endswith('\"') and Item.startswith('\"'):
            #
            # Common item, not need to combine the information
            #
            NewItemList.append(Item)
        else:
            if not CompleteFlag:
                NewItemList.append(Item)
            else:
                AppendContent = AppendContent + "." + Item

    if len(NewItemList) > 4:
        return False, []

    return True, NewItemList

## GetArch
#
# GetArch
#
def GetArch(ItemList, ArchList, FileName, LineNo, SectionString):
    #
    # S1 is always Arch
    #
    if len(ItemList) > 1:
        Arch = ItemList[1]
    else:
        Arch = 'COMMON'
    ArchList.add(Arch)

    #
    # 'COMMON' must not be used with specific ARCHs at the same section
    #
    if 'COMMON' in ArchList and len(ArchList) > 1:
        Logger.Error('Parser',
                     FORMAT_INVALID,
                     ST.ERR_INF_PARSER_SECTION_ARCH_CONFLICT,
                     File=FileName,
                     Line=LineNo,
                     ExtraData=SectionString)

    return Arch, ArchList

## InfSectionParser
#
# Inherit from object
#
class InfSectionParser(InfDefinSectionParser,
                       InfBuildOptionSectionParser,
                       InfSourceSectionParser,
                       InfLibrarySectionParser,
                       InfPackageSectionParser,
                       InfGuidPpiProtocolSectionParser,
                       InfBinarySectionParser,
                       InfPcdSectionParser,
                       InfDepexSectionParser):
    #
    # Parser objects used to implement singleton
    #
    MetaFiles = {}

    ## Factory method
    #
    # One file, one parser object. This factory method makes sure that there's
    # only one object constructed for one meta file.
    #
    #   @param  Class           class object of real AutoGen class
    #                           (InfParser, DecParser or DscParser)
    #   @param  FilePath        The path of meta file
    #
    def __new__(cls, FilePath, *args, **kwargs):
        if args:
            pass
        if kwargs:
            pass
        if FilePath in cls.MetaFiles:
            return cls.MetaFiles[FilePath]
        else:
            ParserObject = super(InfSectionParser, cls).__new__(cls)
            cls.MetaFiles[FilePath] = ParserObject
            return ParserObject

    def __init__(self):
        InfDefinSectionParser.__init__(self)
        InfBuildOptionSectionParser.__init__(self)
        InfSourceSectionParser.__init__(self)
        InfLibrarySectionParser.__init__(self)
        InfPackageSectionParser.__init__(self)
        InfGuidPpiProtocolSectionParser.__init__(self)
        InfBinarySectionParser.__init__(self)
        InfPcdSectionParser.__init__(self)
        InfDepexSectionParser.__init__(self)
        #
        # Initialize all objects that an INF file will generated.
        #
        self.InfDefSection = InfDefObject()
        self.InfBuildOptionSection = InfBuildOptionsObject()
        self.InfLibraryClassSection = InfLibraryClassObject()
        self.InfPackageSection = InfPackageObject()
        self.InfPcdSection = InfPcdObject(list(self.MetaFiles.keys())[0])
        self.InfSourcesSection = InfSourcesObject()
        self.InfUserExtensionSection = InfUserExtensionObject()
        self.InfProtocolSection = InfProtocolObject()
        self.InfPpiSection = InfPpiObject()
        self.InfGuidSection = InfGuidObject()
        self.InfDepexSection = InfDepexObject()
        self.InfPeiDepexSection = InfDepexObject()
        self.InfDxeDepexSection = InfDepexObject()
        self.InfSmmDepexSection = InfDepexObject()
        self.InfBinariesSection = InfBinariesObject()
        self.InfHeader = InfHeaderObject()
        self.InfBinaryHeader = InfHeaderObject()
        self.InfSpecialCommentSection = InfSpecialCommentObject()

        #
        # A List for store define section content.
        #
        self._PcdNameList = []
        self._SectionName = ''
        self._SectionType = 0
        self.RelaPath = ''
        self.FileName = ''

    #
    # File Header content parser
    #
    def InfHeaderParser(self, Content, InfHeaderObject2, FileName, IsBinaryHeader = False):
        if IsBinaryHeader:
            (Abstract, Description, Copyright, License) = ParseHeaderCommentSection(Content, FileName, True)
            if not Abstract or not Description or not Copyright or not License:
                Logger.Error('Parser',
                             FORMAT_INVALID,
                             ST.ERR_INVALID_BINARYHEADER_FORMAT,
                             File=FileName)
        else:
            (Abstract, Description, Copyright, License) = ParseHeaderCommentSection(Content, FileName)
        #
        # Not process file name now, for later usage.
        #
        if self.FileName:
            pass

        #
        # Insert Abstract, Description, CopyRight, License into header object
        #
        InfHeaderObject2.SetAbstract(Abstract)
        InfHeaderObject2.SetDescription(Description)
        InfHeaderObject2.SetCopyright(Copyright)
        InfHeaderObject2.SetLicense(License)




    ## Section header parser
    #
    #   The section header is always in following format:
    #
    #       [section_name.arch<.platform|module_type>]
    #
    # @param String    A string contained the content need to be parsed.
    #
    def SectionHeaderParser(self, SectionString, FileName, LineNo):
        _Scope = []
        _SectionName = ''
        ArchList = set()
        _ValueList = []
        _PcdNameList = [DT.TAB_INF_FIXED_PCD.upper(),
                             DT.TAB_INF_FEATURE_PCD.upper(),
                             DT.TAB_INF_PATCH_PCD.upper(),
                             DT.TAB_INF_PCD.upper(),
                             DT.TAB_INF_PCD_EX.upper()
                             ]
        SectionString = SectionString.strip()
        for Item in GetSplitValueList(SectionString[1:-1], DT.TAB_COMMA_SPLIT):
            if Item == '':
                Logger.Error('Parser',
                             FORMAT_INVALID,
                             ST.ERR_INF_PARSER_MODULE_SECTION_TYPE_ERROR % (""),
                             File=FileName,
                             Line=LineNo,
                             ExtraData=SectionString)
            ItemList = GetSplitValueList(Item, DT.TAB_SPLIT)
            #
            # different section should not mix in one section
            # Allow different PCD type sections mixed together
            #
            if _SectionName.upper() not in _PcdNameList:
                if _SectionName != '' and _SectionName.upper() != ItemList[0].upper():
                    Logger.Error('Parser',
                                 FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_SECTION_NAME_DUPLICATE,
                                 File=FileName,
                                 Line=LineNo,
                                 ExtraData=SectionString)
            elif _PcdNameList[1] in [_SectionName.upper(), ItemList[0].upper()] and \
                (_SectionName.upper()!= ItemList[0].upper()):
                Logger.Error('Parser',
                             FORMAT_INVALID,
                             ST.ERR_INF_PARSER_MODULE_SECTION_TYPE_ERROR % (""),
                             File=FileName,
                             Line=LineNo,
                             ExtraData=SectionString)

            _SectionName = ItemList[0]
            if _SectionName.upper() in gINF_SECTION_DEF:
                self._SectionType = gINF_SECTION_DEF[_SectionName.upper()]
            else:
                self._SectionType = DT.MODEL_UNKNOWN
                Logger.Error("Parser",
                             FORMAT_INVALID,
                             ST.ERR_INF_PARSER_UNKNOWN_SECTION,
                             File=FileName,
                             Line=LineNo,
                             ExtraData=SectionString)

            #
            # Get Arch
            #
            Str1, ArchList = GetArch(ItemList, ArchList, FileName, LineNo, SectionString)

            #
            # For [Defines] section, do special check.
            #
            if ItemList[0].upper() == DT.TAB_COMMON_DEFINES.upper():
                if len(ItemList) != 1:
                    Logger.Error('Parser',
                                 FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID % (SectionString),
                                 File=FileName, Line=LineNo, ExtraData=SectionString)

            #
            # For [UserExtension] section, do special check.
            #
            if ItemList[0].upper() == DT.TAB_USER_EXTENSIONS.upper():

                RetValue = ProcessUseExtHeader(ItemList)

                if not RetValue[0]:
                    Logger.Error('Parser',
                                 FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID % (SectionString),
                                 File=FileName, Line=LineNo, ExtraData=SectionString)
                else:
                    ItemList = RetValue[1]

                if len(ItemList) == 3:
                    ItemList.append('COMMON')

                Str1 = ItemList[1]

            #
            # For Library classes, need to check module type.
            #
            if ItemList[0].upper() == DT.TAB_LIBRARY_CLASSES.upper() and len(ItemList) == 3:
                if ItemList[2] != '':
                    ModuleTypeList = GetSplitValueList(ItemList[2], DT.TAB_VALUE_SPLIT)
                    for Item in ModuleTypeList:
                        if Item.strip() not in DT.MODULE_LIST:
                            Logger.Error('Parser',
                                         FORMAT_INVALID,
                                         ST.ERR_INF_PARSER_DEFINE_MODULETYPE_INVALID % (Item),
                                         File=FileName,
                                         Line=LineNo,
                                         ExtraData=SectionString)
            #
            # GetSpecialStr2
            #
            Str2 = GetSpecialStr2(ItemList, FileName, LineNo, SectionString)

            _Scope.append([Str1, Str2])

            _NewValueList = []
            _AppendFlag = True
            if _SectionName.upper() in _PcdNameList:
                for ValueItem in _ValueList:
                    if _SectionName.upper() == ValueItem[0].upper() and Str1.upper() not in ValueItem[1].split():
                        ValueItem[1] = ValueItem[1] + " " + Str1
                        _AppendFlag = False
                    elif _SectionName.upper() == ValueItem[0].upper() and Str1.upper() in ValueItem[1].split():
                        _AppendFlag = False

                    _NewValueList.append(ValueItem)

                _ValueList = _NewValueList

            if _AppendFlag:
                if not ItemList[0].upper() == DT.TAB_USER_EXTENSIONS.upper():
                    _ValueList.append([_SectionName, Str1, Str2, LineNo])
                else:
                    if len(ItemList) == 4:
                        _ValueList.append([_SectionName, Str1, Str2, ItemList[3], LineNo])

        self.SectionHeaderContent = deepcopy(_ValueList)

    ## GenSpecialSectionList
    #
    #  @param SpecialSectionList: a list of list, of which item's format
    #                             (Comment, LineNum)
    #  @param ContainerFile:      Input value for filename of Inf file
    #
    def InfSpecialCommentParser (self, SpecialSectionList, InfSectionObject, ContainerFile, SectionType):
        ReFindSpecialCommentRe = re.compile(r"""#(?:\s*)\[(.*?)\](?:.*)""", re.DOTALL)
        ReFindHobArchRe = re.compile(r"""[Hh][Oo][Bb]\.([^,]*)""", re.DOTALL)
        if self.FileName:
            pass
        SpecialObjectList = []
        ArchList = []
        if SectionType == DT.TYPE_EVENT_SECTION:
            TokenDict = DT.EVENT_TOKENS
        elif SectionType == DT.TYPE_HOB_SECTION:
            TokenDict = DT.HOB_TOKENS
        else:
            TokenDict = DT.BOOTMODE_TOKENS

        for List in SpecialSectionList:
            #
            # Hob has Arch attribute, need to be handled specially here
            #
            if SectionType == DT.TYPE_HOB_SECTION:

                MatchObject = ReFindSpecialCommentRe.search(List[0][0])
                HobSectionStr = MatchObject.group(1)
                ArchList = []
                for Match in ReFindHobArchRe.finditer(HobSectionStr):
                    Arch = Match.groups(1)[0].upper()
                    ArchList.append(Arch)
            CommentSoFar = ''
            for Index in range(1, len(List)):
                Result = ParseComment(List[Index], DT.ALL_USAGE_TOKENS, TokenDict, [], False)
                Usage = Result[0]
                Type = Result[1]
                HelpText = Result[3]

                if Usage == DT.ITEM_UNDEFINED and Type == DT.ITEM_UNDEFINED:
                    if HelpText is None:
                        HelpText = ''
                    if not HelpText.endswith('\n'):
                        HelpText += '\n'
                    CommentSoFar += HelpText
                else:
                    if HelpText:
                        CommentSoFar += HelpText
                    if SectionType == DT.TYPE_EVENT_SECTION:
                        SpecialObject = InfEventObject()
                        SpecialObject.SetEventType(Type)
                        SpecialObject.SetUsage(Usage)
                        SpecialObject.SetHelpString(CommentSoFar)
                    elif SectionType == DT.TYPE_HOB_SECTION:
                        SpecialObject = InfHobObject()
                        SpecialObject.SetHobType(Type)
                        SpecialObject.SetUsage(Usage)
                        SpecialObject.SetHelpString(CommentSoFar)
                        if len(ArchList) >= 1:
                            SpecialObject.SetSupArchList(ArchList)
                    else:
                        SpecialObject = InfBootModeObject()
                        SpecialObject.SetSupportedBootModes(Type)
                        SpecialObject.SetUsage(Usage)
                        SpecialObject.SetHelpString(CommentSoFar)

                    SpecialObjectList.append(SpecialObject)
                    CommentSoFar = ''
        if not InfSectionObject.SetSpecialComments(SpecialObjectList,
                                                   SectionType):
            Logger.Error('InfParser',
                         FORMAT_INVALID,
                         ST.ERR_INF_PARSER_MODULE_SECTION_TYPE_ERROR % (SectionType),
                         ContainerFile
                         )
