## @file
# This file contained the parser for [Sources] sections in INF file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
'''
InfSourceSectionParser
'''
##
# Import Modules
#

import Logger.Log as Logger
from Logger import StringTable as ST
from Logger.ToolError import FORMAT_INVALID
from Parser.InfParserMisc import InfExpandMacro
from Library import DataType as DT
from Library.Parsing import MacroParser
from Library.Misc import GetSplitValueList
from Object.Parser.InfCommonObject import InfLineCommentObject
from Parser.InfParserMisc import InfParserSectionRoot

class InfSourceSectionParser(InfParserSectionRoot):
    ## InfSourceParser
    #
    #
    def InfSourceParser(self, SectionString, InfSectionObject, FileName):
        SectionMacros = {}
        ValueList     = []
        SourceList    = []
        StillCommentFalg  = False
        HeaderComments    = []
        LineComment       = None
        SectionContent  = ''
        for Line in SectionString:
            SrcLineContent = Line[0]
            SrcLineNo      = Line[1]

            if SrcLineContent.strip() == '':
                continue

            #
            # Found Header Comments
            #
            if SrcLineContent.strip().startswith(DT.TAB_COMMENT_SPLIT):
                #
                # Last line is comments, and this line go on.
                #
                if StillCommentFalg:
                    HeaderComments.append(Line)
                    SectionContent += SrcLineContent + DT.END_OF_LINE
                    continue
                #
                # First time encounter comment
                #
                else:
                    #
                    # Clear original data
                    #
                    HeaderComments = []
                    HeaderComments.append(Line)
                    StillCommentFalg = True
                    SectionContent += SrcLineContent + DT.END_OF_LINE
                    continue
            else:
                StillCommentFalg = False

            if len(HeaderComments) >= 1:
                LineComment = InfLineCommentObject()
                LineCommentContent = ''
                for Item in HeaderComments:
                    LineCommentContent += Item[0] + DT.END_OF_LINE
                LineComment.SetHeaderComments(LineCommentContent)

            #
            # Find Tail comment.
            #
            if SrcLineContent.find(DT.TAB_COMMENT_SPLIT) > -1:
                TailComments = SrcLineContent[SrcLineContent.find(DT.TAB_COMMENT_SPLIT):]
                SrcLineContent = SrcLineContent[:SrcLineContent.find(DT.TAB_COMMENT_SPLIT)]
                if LineComment is None:
                    LineComment = InfLineCommentObject()
                LineComment.SetTailComments(TailComments)

            #
            # Find Macro
            #
            Name, Value = MacroParser((SrcLineContent, SrcLineNo),
                                      FileName,
                                      DT.MODEL_EFI_SOURCE_FILE,
                                      self.FileLocalMacros)
            if Name is not None:
                SectionMacros[Name] = Value
                LineComment = None
                HeaderComments = []
                continue

            #
            # Replace with Local section Macro and [Defines] section Macro.
            #
            SrcLineContent = InfExpandMacro(SrcLineContent,
                                         (FileName, SrcLineContent, SrcLineNo),
                                         self.FileLocalMacros,
                                         SectionMacros)

            TokenList = GetSplitValueList(SrcLineContent, DT.TAB_VALUE_SPLIT, 4)
            ValueList[0:len(TokenList)] = TokenList

            #
            # Store section content string after MACRO replaced.
            #
            SectionContent += SrcLineContent + DT.END_OF_LINE

            SourceList.append((ValueList, LineComment,
                               (SrcLineContent, SrcLineNo, FileName)))
            ValueList = []
            LineComment = None
            TailComments = ''
            HeaderComments = []
            continue

        #
        # Current section archs
        #
        ArchList = []
        for Item in self.LastSectionHeaderContent:
            if Item[1] not in ArchList:
                ArchList.append(Item[1])
                InfSectionObject.SetSupArchList(Item[1])

        InfSectionObject.SetAllContent(SectionContent)
        if not InfSectionObject.SetSources(SourceList, Arch = ArchList):
            Logger.Error('InfParser',
                         FORMAT_INVALID,
                         ST.ERR_INF_PARSER_MODULE_SECTION_TYPE_ERROR % ("[Sources]"),
                         File=FileName,
                         Line=Item[3])
