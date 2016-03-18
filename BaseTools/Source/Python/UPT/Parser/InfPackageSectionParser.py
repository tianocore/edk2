## @file
# This file contained the parser for [Packages] sections in INF file 
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
InfPackageSectionParser
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

class InfPackageSectionParser(InfParserSectionRoot):
    ## InfPackageParser
    #
    #                       
    def InfPackageParser(self, SectionString, InfSectionObject, FileName):
        #
        # Macro defined in this section 
        #
        SectionMacros = {}
        ValueList     = []
        PackageList   = []
        StillCommentFalg  = False
        HeaderComments    = []
        LineComment       = None                  
        #
        # Parse section content
        #
        for Line in SectionString:
            PkgLineContent = Line[0]
            PkgLineNo      = Line[1]  
            
            if PkgLineContent.strip() == '':
                continue
            
            #
            # Find Header Comments 
            #
            if PkgLineContent.strip().startswith(DT.TAB_COMMENT_SPLIT):
                #
                # Last line is comments, and this line go on.
                #
                if StillCommentFalg:
                    HeaderComments.append(Line)
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
            if PkgLineContent.find(DT.TAB_COMMENT_SPLIT) > -1:
                TailComments = PkgLineContent[PkgLineContent.find(DT.TAB_COMMENT_SPLIT):]
                PkgLineContent = PkgLineContent[:PkgLineContent.find(DT.TAB_COMMENT_SPLIT)]
                if LineComment == None:
                    LineComment = InfLineCommentObject()
                LineComment.SetTailComments(TailComments)                   
            #
            # Find Macro
            #
            Name, Value = MacroParser((PkgLineContent, PkgLineNo),
                                      FileName,
                                      DT.MODEL_META_DATA_PACKAGE,
                                      self.FileLocalMacros)
            if Name != None:
                SectionMacros[Name] = Value
                LineComment = None
                HeaderComments = []                
                continue

            TokenList = GetSplitValueList(PkgLineContent, DT.TAB_VALUE_SPLIT, 1)
            ValueList[0:len(TokenList)] = TokenList
            
            #
            # Replace with Local section Macro and [Defines] section Macro.
            #            
            ValueList = [InfExpandMacro(Value, (FileName, PkgLineContent, PkgLineNo), 
                                        self.FileLocalMacros, SectionMacros, True)
                                        for Value in ValueList]
            
            PackageList.append((ValueList, LineComment, 
                                (PkgLineContent, PkgLineNo, FileName)))
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
        
        if not InfSectionObject.SetPackages(PackageList, Arch = ArchList):
            Logger.Error('InfParser', 
                         FORMAT_INVALID, 
                         ST.ERR_INF_PARSER_MODULE_SECTION_TYPE_ERROR\
                         %("[Packages]"),
                         File=FileName,
                         Line=Item[3])         