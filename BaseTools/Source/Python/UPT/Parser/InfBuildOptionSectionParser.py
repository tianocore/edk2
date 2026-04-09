## @file
# This file contained the parser for BuildOption sections in INF file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
InfBuildOptionSectionParser
'''
##
# Import Modules
#
from Library import DataType as DT
from Library import GlobalData
import Logger.Log as Logger
from Logger import StringTable as ST
from Logger.ToolError import FORMAT_INVALID
from Parser.InfParserMisc import InfExpandMacro
from Library.Misc import GetSplitValueList
from Parser.InfParserMisc import IsAsBuildOptionInfo
from Library.Misc import GetHelpStringByRemoveHashKey
from Library.ParserValidate import IsValidFamily
from Library.ParserValidate import IsValidBuildOptionName
from Parser.InfParserMisc import InfParserSectionRoot

class InfBuildOptionSectionParser(InfParserSectionRoot):
    ## InfBuildOptionParser
    #
    #
    def InfBuildOptionParser(self, SectionString, InfSectionObject, FileName):

        BuildOptionList = []
        SectionContent  = ''

        if not GlobalData.gIS_BINARY_INF:
            ValueList       = []
            LineNo          = 0

            for Line in SectionString:
                LineContent = Line[0]
                LineNo      = Line[1]
                TailComments = ''
                ReplaceFlag = False

                if LineContent.strip() == '':
                    SectionContent += LineContent + DT.END_OF_LINE
                    continue
                #
                # Found Comment
                #
                if LineContent.strip().startswith(DT.TAB_COMMENT_SPLIT):
                    SectionContent += LineContent + DT.END_OF_LINE
                    continue

                #
                # Find Tail comment.
                #
                if LineContent.find(DT.TAB_COMMENT_SPLIT) > -1:
                    TailComments = LineContent[LineContent.find(DT.TAB_COMMENT_SPLIT):]
                    LineContent = LineContent[:LineContent.find(DT.TAB_COMMENT_SPLIT)]

                TokenList = GetSplitValueList(LineContent, DT.TAB_DEQUAL_SPLIT, 1)
                if len(TokenList) == 2:
                    #
                    # "Replace" type build option
                    #
                    TokenList.append('True')
                    ReplaceFlag = True
                else:
                    TokenList = GetSplitValueList(LineContent, DT.TAB_EQUAL_SPLIT, 1)
                    #
                    # "Append" type build option
                    #
                    if len(TokenList) == 2:
                        TokenList.append('False')
                    else:
                        Logger.Error('InfParser',
                                     FORMAT_INVALID,
                                     ST.ERR_INF_PARSER_BUILD_OPTION_FORMAT_INVALID,
                                     ExtraData=LineContent,
                                     File=FileName,
                                     Line=LineNo)

                ValueList[0:len(TokenList)] = TokenList

                #
                # Replace with [Defines] section Macro
                #
                ValueList[0] = InfExpandMacro(ValueList[0], (FileName, LineContent, LineNo),
                                              self.FileLocalMacros, None)
                ValueList[1] = InfExpandMacro(ValueList[1], (FileName, LineContent, LineNo),
                                              self.FileLocalMacros, None, True)
                EqualString = ''
                if not ReplaceFlag:
                    EqualString = ' = '
                else:
                    EqualString = ' == '

                SectionContent += ValueList[0] + EqualString + ValueList[1] + TailComments + DT.END_OF_LINE

                Family = GetSplitValueList(ValueList[0], DT.TAB_COLON_SPLIT, 1)
                if len(Family) == 2:
                    if not IsValidFamily(Family[0]):
                        Logger.Error('InfParser',
                                     FORMAT_INVALID,
                                     ST.ERR_INF_PARSER_BUILD_OPTION_FORMAT_INVALID,
                                     ExtraData=LineContent,
                                     File=FileName,
                                     Line=LineNo)
                    if not IsValidBuildOptionName(Family[1]):
                        Logger.Error('InfParser',
                                     FORMAT_INVALID,
                                     ST.ERR_INF_PARSER_BUILD_OPTION_FORMAT_INVALID,
                                     ExtraData=LineContent,
                                     File=FileName,
                                     Line=LineNo)
                if len(Family) == 1:
                    if not IsValidBuildOptionName(Family[0]):
                        Logger.Error('InfParser',
                                     FORMAT_INVALID,
                                     ST.ERR_INF_PARSER_BUILD_OPTION_FORMAT_INVALID,
                                     ExtraData=LineContent,
                                     File=FileName,
                                     Line=LineNo)

                BuildOptionList.append(ValueList)
                ValueList = []
                continue
        else:
            BuildOptionList = InfAsBuiltBuildOptionParser(SectionString, FileName)

        #
        # Current section archs
        #
        ArchList = []
        LastItem = ''
        for Item in self.LastSectionHeaderContent:
            LastItem = Item
            if not (Item[1] == '' or Item[1] == '') and Item[1] not in ArchList:
                ArchList.append(Item[1])
                InfSectionObject.SetSupArchList(Item[1])

        InfSectionObject.SetAllContent(SectionContent)
        if not InfSectionObject.SetBuildOptions(BuildOptionList, ArchList, SectionContent):
            Logger.Error('InfParser',
                         FORMAT_INVALID,
                         ST.ERR_INF_PARSER_MODULE_SECTION_TYPE_ERROR%("[BuilOptions]"),
                         File=FileName,
                         Line=LastItem[3])

## InfBuildOptionParser
#
#
def InfAsBuiltBuildOptionParser(SectionString, FileName):
    BuildOptionList = []
    #
    # AsBuild Binary INF file.
    #
    AsBuildOptionFlag = False
    BuildOptionItem = []
    Count = 0
    for Line in SectionString:
        Count += 1
        LineContent = Line[0]
        LineNo      = Line[1]

        #
        # The last line
        #
        if len(SectionString) == Count:
            if LineContent.strip().startswith("##") and AsBuildOptionFlag:
                BuildOptionList.append(BuildOptionItem)
                BuildOptionList.append([GetHelpStringByRemoveHashKey(LineContent)])
            elif LineContent.strip().startswith("#") and AsBuildOptionFlag:
                BuildOptionInfo = GetHelpStringByRemoveHashKey(LineContent)
                BuildOptionItem.append(BuildOptionInfo)
                BuildOptionList.append(BuildOptionItem)
            else:
                if len(BuildOptionItem) > 0:
                    BuildOptionList.append(BuildOptionItem)

            break

        if LineContent.strip() == '':
            AsBuildOptionFlag = False
            continue

        if LineContent.strip().startswith("##") and AsBuildOptionFlag:
            if len(BuildOptionItem) > 0:
                BuildOptionList.append(BuildOptionItem)

            BuildOptionItem = []

        if not LineContent.strip().startswith("#"):
            Logger.Error('InfParser',
                        FORMAT_INVALID,
                        ST.ERR_BO_CONTATIN_ASBUILD_AND_COMMON,
                        File=FileName,
                        Line=LineNo,
                        ExtraData=LineContent)

        if IsAsBuildOptionInfo(LineContent):
            AsBuildOptionFlag = True
            continue

        if AsBuildOptionFlag:
            BuildOptionInfo = GetHelpStringByRemoveHashKey(LineContent)
            BuildOptionItem.append(BuildOptionInfo)

    return BuildOptionList
