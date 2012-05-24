## @file
# This file contained the parser for [Libraries] sections in INF file 
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
InfLibrarySectionParser
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
from Library import GlobalData
from Parser.InfParserMisc import IsLibInstanceInfo
from Parser.InfAsBuiltProcess import GetLibInstanceInfo
from Parser.InfParserMisc import InfParserSectionRoot

class InfLibrarySectionParser(InfParserSectionRoot):
    ## InfLibraryParser
    #
    #                 
    def InfLibraryParser(self, SectionString, InfSectionObject, FileName):
        #
        # For Common INF file
        #
        if not GlobalData.gIS_BINARY_INF:
            #
            # Macro defined in this section 
            #
            SectionMacros = {}
            ValueList     = []
            LibraryList   = []
            LibStillCommentFalg  = False
            LibHeaderComments    = []
            LibLineComment       = None              
            #
            # Parse section content
            #
            for Line in SectionString:
                LibLineContent = Line[0]
                LibLineNo      = Line[1]
                
                if LibLineContent.strip() == '':
                    continue
                
                #
                # Found Header Comments 
                #
                if LibLineContent.strip().startswith(DT.TAB_COMMENT_SPLIT):
                    #
                    # Last line is comments, and this line go on.
                    #
                    if LibStillCommentFalg:
                        LibHeaderComments.append(Line)
                        continue
                    #
                    # First time encounter comment 
                    #
                    else:
                        #
                        # Clear original data
                        #
                        LibHeaderComments = []
                        LibHeaderComments.append(Line)
                        LibStillCommentFalg = True
                        continue
                else:
                    LibStillCommentFalg = False
                              
                if len(LibHeaderComments) >= 1:
                    LibLineComment = InfLineCommentObject()
                    LineCommentContent = ''
                    for Item in LibHeaderComments:
                        LineCommentContent += Item[0] + DT.END_OF_LINE
                    LibLineComment.SetHeaderComments(LineCommentContent)
                
                #
                # Find Tail comment.
                #
                if LibLineContent.find(DT.TAB_COMMENT_SPLIT) > -1:
                    LibTailComments = LibLineContent[LibLineContent.find(DT.TAB_COMMENT_SPLIT):]
                    LibLineContent = LibLineContent[:LibLineContent.find(DT.TAB_COMMENT_SPLIT)]
                    if LibLineComment == None:
                        LibLineComment = InfLineCommentObject()
                    LibLineComment.SetTailComments(LibTailComments)            
                
                #
                # Find Macro
                #
                Name, Value = MacroParser((LibLineContent, LibLineNo),
                                          FileName,
                                          DT.MODEL_EFI_LIBRARY_CLASS,
                                          self.FileLocalMacros)
                if Name != None:
                    SectionMacros[Name] = Value
                    LibLineComment = None
                    LibHeaderComments = []                
                    continue
                
                TokenList = GetSplitValueList(LibLineContent, DT.TAB_VALUE_SPLIT, 1)
                ValueList[0:len(TokenList)] = TokenList
    
                #
                # Replace with Local section Macro and [Defines] section Macro.
                #            
                ValueList = [InfExpandMacro(Value, (FileName, LibLineContent, LibLineNo), 
                                            self.FileLocalMacros, SectionMacros, True)
                                            for Value in ValueList]
    
                LibraryList.append((ValueList, LibLineComment, 
                                    (LibLineContent, LibLineNo, FileName)))
                ValueList = []
                LibLineComment = None
                LibTailComments = ''
                LibHeaderComments = []
                
                continue
    
            #
            # Current section archs
            #    
            KeyList = []
            for Item in self.LastSectionHeaderContent:
                if (Item[1], Item[2]) not in KeyList:
                    KeyList.append((Item[1], Item[2]))
                                   
            if not InfSectionObject.SetLibraryClasses(LibraryList, KeyList = KeyList):
                Logger.Error('InfParser', 
                             FORMAT_INVALID,
                             ST.ERR_INF_PARSER_MODULE_SECTION_TYPE_ERROR % ("[Library]"),
                             File=FileName, 
                             Line=Item[3])
        #
        # For Binary INF
        #
        else:
            self.InfAsBuiltLibraryParser(SectionString, InfSectionObject, FileName)
                
    def InfAsBuiltLibraryParser(self, SectionString, InfSectionObject, FileName):
        LibraryList = []
        LibInsFlag = False
        for Line in SectionString:
            LineContent = Line[0]
            LineNo      = Line[1]
            
            if LineContent.strip() == '':
                LibInsFlag = False
                continue
            
            if not LineContent.strip().startswith("#"):
                Logger.Error('InfParser', 
                            FORMAT_INVALID,
                            ST.ERR_LIB_CONTATIN_ASBUILD_AND_COMMON, 
                            File=FileName, 
                            Line=LineNo, 
                            ExtraData=LineContent)
            
            if IsLibInstanceInfo(LineContent):
                LibInsFlag = True
                continue
            
            if LibInsFlag:
                LibGuid, LibVer = GetLibInstanceInfo(LineContent, GlobalData.gWORKSPACE, LineNo)
                #
                # If the VERSION_STRING is missing from the INF file, tool should default to "0".
                #
                if LibVer == '':
                    LibVer = '0'
                if LibGuid != '':
                    LibraryList.append((LibGuid, LibVer))
                else:
                    Logger.Error('InfParser', 
                            FORMAT_INVALID,
                            ST.ERR_LIB_INSTANCE_MISS_GUID, 
                            File=FileName, 
                            Line=LineNo, 
                            ExtraData=LineContent)                    
                
        #
        # Current section archs
        #    
        KeyList = []
        Item = ['', '', '']
        for Item in self.LastSectionHeaderContent:
            if (Item[1], Item[2]) not in KeyList:
                KeyList.append((Item[1], Item[2]))
                
        if not InfSectionObject.SetLibraryClasses(LibraryList, KeyList = KeyList):
            Logger.Error('InfParser', 
                         FORMAT_INVALID,
                         ST.ERR_INF_PARSER_MODULE_SECTION_TYPE_ERROR % ("[Library]"),
                         File=FileName, 
                         Line=Item[3])           