## @file
# This file contained the parser for define sections in INF file 
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
InfDefineSectionParser
'''
##
# Import Modules
#
import re

from Library import DataType as DT
from Library import GlobalData
from Library.Parsing import MacroParser
from Library.Misc import GetSplitValueList
from Library.ParserValidate import IsValidArch
from Object.Parser.InfCommonObject import InfLineCommentObject
from Object.Parser.InfDefineObject import InfDefMember
from Parser.InfParserMisc import InfExpandMacro
from Object.Parser.InfMisc import ErrorInInf
from Logger import StringTable as ST
from Parser.InfParserMisc import InfParserSectionRoot

## __GetValidateArchList
#        
#
def GetValidateArchList(LineContent):
    
    TempArch = ''
    ArchList = []
    ValidateAcrhPatten = re.compile(r"^\s*#\s*VALID_ARCHITECTURES\s*=\s*.*$", re.DOTALL)
    
    if ValidateAcrhPatten.match(LineContent):
        TempArch = GetSplitValueList(LineContent, DT.TAB_EQUAL_SPLIT, 1)[1]
                                
        TempArch = GetSplitValueList(TempArch, '(', 1)[0]
                                
        ArchList = re.split('\s+', TempArch)
        NewArchList = []
        for Arch in ArchList:
            if IsValidArch(Arch):
                NewArchList.append(Arch)
        
        ArchList = NewArchList
        
    return ArchList   

class InfDefinSectionParser(InfParserSectionRoot):
    def InfDefineParser(self, SectionString, InfSectionObject, FileName, SectionComment):
        
        if SectionComment:
            pass
        #
        # Parser Defines section content and fill self._ContentList dict.
        #
        StillCommentFalg  = False
        HeaderComments = []
        SectionContent = ''
        ArchList       = []
        _ContentList   = []
        _ValueList     = []
        #
        # Add WORKSPACE to global Marco dict.
        #
        self.FileLocalMacros['WORKSPACE'] = GlobalData.gWORKSPACE
        
        for Line in SectionString:
            LineContent = Line[0]
            LineNo      = Line[1]
            TailComments   = ''
            LineComment    = None
            
            LineInfo       = ['', -1, '']
            LineInfo[0]    = FileName
            LineInfo[1]    = LineNo
            LineInfo[2]    = LineContent
            
            if LineContent.strip() == '':
                continue
            #
            # The first time encountered VALIDATE_ARCHITECHERS will be considered as support arch list.
            #
            if not ArchList:
                ArchList = GetValidateArchList(LineContent)

            #
            # Parser Comment
            #
            if LineContent.strip().startswith(DT.TAB_COMMENT_SPLIT):
                #
                # Last line is comments, and this line go on.
                #
                if StillCommentFalg:
                    HeaderComments.append(Line)
                    SectionContent += LineContent + DT.END_OF_LINE
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
                    SectionContent += LineContent + DT.END_OF_LINE
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
            if LineContent.find(DT.TAB_COMMENT_SPLIT) > -1:
                TailComments = LineContent[LineContent.find(DT.TAB_COMMENT_SPLIT):]
                LineContent = LineContent[:LineContent.find(DT.TAB_COMMENT_SPLIT)]
                if LineComment == None:
                    LineComment = InfLineCommentObject()
                LineComment.SetTailComments(TailComments)
                              
            #
            # Find Macro
            #
            Name, Value = MacroParser((LineContent, LineNo), 
                                      FileName, 
                                      DT.MODEL_META_DATA_HEADER, 
                                      self.FileLocalMacros)
            if Name != None:
                self.FileLocalMacros[Name] = Value
                continue            

            #
            # Replace with [Defines] section Macro
            #
            LineContent = InfExpandMacro(LineContent, 
                                         (FileName, LineContent, LineNo), 
                                         self.FileLocalMacros, 
                                         None, True)
                       
            SectionContent += LineContent + DT.END_OF_LINE
            
            TokenList = GetSplitValueList(LineContent, DT.TAB_EQUAL_SPLIT, 1)
            if len(TokenList) < 2:
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_NO_VALUE,
                           LineInfo=LineInfo)                
            _ValueList[0:len(TokenList)] = TokenList
            if not _ValueList[0]:
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_NO_NAME,
                           LineInfo=LineInfo)
            if not _ValueList[1]:
                ErrorInInf(ST.ERR_INF_PARSER_DEFINE_ITEM_NO_VALUE,
                           LineInfo=LineInfo)   
   
            Name, Value = _ValueList[0], _ValueList[1]            
            
            InfDefMemberObj = InfDefMember(Name, Value)
            if (LineComment != None):
                InfDefMemberObj.Comments.SetHeaderComments(LineComment.GetHeaderComments())
                InfDefMemberObj.Comments.SetTailComments(LineComment.GetTailComments())
                
            InfDefMemberObj.CurrentLine.SetFileName(self.FullPath)
            InfDefMemberObj.CurrentLine.SetLineString(LineContent)
            InfDefMemberObj.CurrentLine.SetLineNo(LineNo)
                       
            _ContentList.append(InfDefMemberObj)
            HeaderComments = []
            TailComments = ''
        
        #
        # Current Define section archs
        #
        if not ArchList:
            ArchList = ['COMMON']
        
        InfSectionObject.SetAllContent(SectionContent)                
        
        InfSectionObject.SetDefines(_ContentList, Arch=ArchList)
        