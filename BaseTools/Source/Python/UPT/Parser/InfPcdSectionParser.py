## @file
# This file contained the parser for [Pcds] sections in INF file 
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
InfPcdSectionParser
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
from Library import GlobalData
from Library.String import SplitPcdEntry
from Parser.InfParserMisc import InfParserSectionRoot

class InfPcdSectionParser(InfParserSectionRoot):
    ## Section PCD related parser
    # 
    # For 5 types of PCD list below, all use this function.
    # 'FixedPcd', 'FeaturePcd', 'PatchPcd', 'Pcd', 'PcdEx'
    #
    # This is a INF independent parser, the validation in this parser only 
    # cover
    # INF spec scope, will not cross DEC/DSC to check pcd value
    #
    def InfPcdParser(self, SectionString, InfSectionObject, FileName):
        KeysList = []
        PcdList   = []
        CommentsList = []          
        ValueList = [] 
        #
        # Current section archs
        #          
        LineIndex = -1
        for Item in self.LastSectionHeaderContent:
            if (Item[0], Item[1], Item[3]) not in KeysList:
                KeysList.append((Item[0], Item[1], Item[3]))
                LineIndex = Item[3]
            
            if (Item[0].upper() == DT.TAB_INF_FIXED_PCD.upper() or \
                Item[0].upper() == DT.TAB_INF_FEATURE_PCD.upper() or \
                Item[0].upper() == DT.TAB_INF_PCD.upper()) and GlobalData.gIS_BINARY_INF:
                Logger.Error('InfParser', FORMAT_INVALID, ST.ERR_ASBUILD_PCD_SECTION_TYPE%("\"" + Item[0] + "\""),
                             File=FileName, Line=LineIndex)                    
                            
        #
        # For Common INF file
        #
        if not GlobalData.gIS_BINARY_INF:   
            #
            # Macro defined in this section 
            #
            SectionMacros = {}        
            for Line in SectionString:
                PcdLineContent = Line[0]
                PcdLineNo      = Line[1]
                if PcdLineContent.strip() == '':
                    CommentsList = []
                    continue                   
                    
                if PcdLineContent.strip().startswith(DT.TAB_COMMENT_SPLIT):
                    CommentsList.append(Line)
                    continue
                else:
                    #
                    # Encounter a PCD entry
                    #
                    if PcdLineContent.find(DT.TAB_COMMENT_SPLIT) > -1:
                        CommentsList.append((
                                PcdLineContent[PcdLineContent.find(DT.TAB_COMMENT_SPLIT):], 
                                PcdLineNo))
                        PcdLineContent = PcdLineContent[:PcdLineContent.find(DT.TAB_COMMENT_SPLIT)] 
                
                if PcdLineContent != '':
                    #
                    # Find Macro
                    #
                    Name, Value = MacroParser((PcdLineContent, PcdLineNo),
                                              FileName,
                                              DT.MODEL_EFI_PCD,
                                              self.FileLocalMacros)
                    if Name != None:
                        SectionMacros[Name] = Value
                        ValueList = []
                        CommentsList = []
                        continue
                    
                    PcdEntryReturn = SplitPcdEntry(PcdLineContent)
                    
                    if not PcdEntryReturn[1]:
                        TokenList = ['']               
                    else:
                        TokenList = PcdEntryReturn[0]
                          
                    ValueList[0:len(TokenList)] = TokenList
                    
                    #
                    # Replace with Local section Macro and [Defines] section Macro.
                    #            
                    ValueList = [InfExpandMacro(Value, (FileName, PcdLineContent, PcdLineNo), 
                                                self.FileLocalMacros, SectionMacros, True)
                                for Value in ValueList]
                                   
                if len(ValueList) >= 1:
                    PcdList.append((ValueList, CommentsList, (PcdLineContent, PcdLineNo, FileName)))
                    ValueList = []
                    CommentsList = []
                continue
        #
        # For Binary INF file
        #
        else:
            for Line in SectionString:
                LineContent = Line[0].strip()
                LineNo      = Line[1]
                
                if LineContent == '':
                    CommentsList = []
                    continue
                
                if LineContent.startswith(DT.TAB_COMMENT_SPLIT):
                    CommentsList.append(LineContent)
                    continue
                #
                # Have comments at tail.
                #
                CommentIndex = LineContent.find(DT.TAB_COMMENT_SPLIT)
                if  CommentIndex > -1:
                    CommentsList.append(LineContent[CommentIndex+1:])
                    LineContent = LineContent[:CommentIndex]
                
                TokenList = GetSplitValueList(LineContent, DT.TAB_VALUE_SPLIT)
                #
                # PatchablePcd
                # TokenSpace.CName | Value | Offset
                #
                if KeysList[0][0].upper() == DT.TAB_INF_PATCH_PCD.upper():
                    if len(TokenList) != 3:
                        Logger.Error('InfParser', 
                                     FORMAT_INVALID, 
                                     ST.ERR_ASBUILD_PATCHPCD_FORMAT_INVALID,
                                     File=FileName,
                                     Line=LineNo,
                                     ExtraData=LineContent)
                #                    
                elif KeysList[0][0].upper() == DT.TAB_INF_PCD_EX.upper():
                    if len(TokenList) != 2:
                        Logger.Error('InfParser', 
                                     FORMAT_INVALID, 
                                     ST.ERR_ASBUILD_PCDEX_FORMAT_INVALID,
                                     File=FileName,
                                     Line=LineNo,
                                     ExtraData=LineContent)                                         
                ValueList[0:len(TokenList)] = TokenList
                if len(ValueList) >= 1:                
                    PcdList.append((ValueList, CommentsList, (LineContent, LineNo, FileName)))                  
                    ValueList = []
                    CommentsList = []
                continue          
                    
        if not InfSectionObject.SetPcds(PcdList, KeysList = KeysList, 
                                        PackageInfo = self.InfPackageSection.GetPackages()):
            Logger.Error('InfParser', 
                         FORMAT_INVALID, 
                         ST.ERR_INF_PARSER_MODULE_SECTION_TYPE_ERROR%("[PCD]"),
                         File=FileName,
                         Line=LineIndex)            
    