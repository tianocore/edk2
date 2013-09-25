## @file
# This file contained the parser for [Depex] sections in INF file 
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
'''
InfDepexSectionParser
'''
##
# Import Modules
#
import re
import Logger.Log as Logger
from Logger import StringTable as ST
from Logger.ToolError import FORMAT_INVALID
from Parser.InfParserMisc import InfExpandMacro
from Library import DataType as DT
from Library.Misc import GetSplitValueList
from Parser.InfParserMisc import InfParserSectionRoot

class InfDepexSectionParser(InfParserSectionRoot):
    ## InfDepexParser
    #
    # For now, only separate Depex String and comments. 
    # Have two types of section header.
    # 1. [Depex.Arch.ModuleType, ...]
    # 2. [Depex.Arch|FFE, ...]
    #
    def InfDepexParser(self, SectionString, InfSectionObject, FileName):
        DepexContent = []
        DepexComment = []
        ValueList    = []
        #
        # Parse section content
        #
        for Line in SectionString:
            LineContent = Line[0]
            LineNo      = Line[1]
                                            
            #
            # Found comment
            #
            if LineContent.strip().startswith(DT.TAB_COMMENT_SPLIT):
                DepexComment.append((LineContent, LineNo))
                continue
            #
            # Replace with [Defines] section Macro
            #
            LineContent = InfExpandMacro(LineContent, 
                                         (FileName, LineContent, Line[1]), 
                                         self.FileLocalMacros, 
                                         None, True)
            
            CommentCount = LineContent.find(DT.TAB_COMMENT_SPLIT)
            
            if CommentCount > -1:
                DepexComment.append((LineContent[CommentCount:], LineNo))          
                LineContent = LineContent[:CommentCount-1]
                
        
            CommentCount = -1
            DepexContent.append((LineContent, LineNo))
               
            TokenList = GetSplitValueList(LineContent, DT.TAB_COMMENT_SPLIT)
            ValueList[0:len(TokenList)] = TokenList
             
        #
        # Current section archs
        #    
        KeyList = []
        LastItem = ''
        for Item in self.LastSectionHeaderContent:
            LastItem = Item
            if (Item[1], Item[2], Item[3]) not in KeyList:
                KeyList.append((Item[1], Item[2], Item[3]))        
        
        NewCommentList = []
        FormatCommentLn = -1
        ReFormatComment = re.compile(r"""#(?:\s*)\[(.*?)\](?:.*)""", re.DOTALL)
        for CommentItem in DepexComment:
            CommentContent = CommentItem[0]
            if ReFormatComment.match(CommentContent) != None:
                FormatCommentLn = CommentItem[1] + 1
                continue
            
            if CommentItem[1] != FormatCommentLn:
                NewCommentList.append(CommentContent)
            else:
                FormatCommentLn = CommentItem[1] + 1
                       
        if not InfSectionObject.SetDepex(DepexContent, KeyList = KeyList, CommentList = NewCommentList):
            Logger.Error('InfParser', 
                         FORMAT_INVALID,
                         ST.ERR_INF_PARSER_MODULE_SECTION_TYPE_ERROR%("[Depex]"),
                         File=FileName, 
                         Line=LastItem[3])      