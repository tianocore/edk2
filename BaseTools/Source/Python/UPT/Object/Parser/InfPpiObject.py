## @file
# This file is used to define class objects of INF file [Ppis] section. 
# It will consumed by InfParser. 
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

'''
InfPpiObject
'''

from Library.ParserValidate import IsValidCVariableName
from Library.CommentParsing import ParseComment
from Library.ExpressionValidate import IsValidFeatureFlagExp 
                               
from Library.Misc import Sdict
from Library import DataType as DT                
import Logger.Log as Logger
from Logger import ToolError
from Logger import StringTable as ST

def ParsePpiComment(CommentsList, InfPpiItemObj):
    PreNotify = None
    PreUsage = None              
    PreHelpText = ''
    BlockFlag = -1
    CommentInsList = []
    Count = 0
    for CommentItem in CommentsList:
        Count = Count + 1
        CommentItemUsage, \
        CommentItemNotify, \
        CommentItemString, \
        CommentItemHelpText = \
                ParseComment(CommentItem, 
                             DT.ALL_USAGE_TOKENS, 
                             DT.PPI_NOTIFY_TOKENS, 
                             ['PPI'], 
                             False)
        
        #
        # To avoid PyLint error   
        #
        if CommentItemString:
            pass
        
        if CommentItemHelpText == None:
            CommentItemHelpText = ''
            if Count == len(CommentsList) and CommentItemUsage == CommentItemNotify == DT.ITEM_UNDEFINED:
                CommentItemHelpText = DT.END_OF_LINE                    
        #
        # For the Last comment Item, set BlockFlag.
        #
        if Count == len(CommentsList):
            if BlockFlag == 1 or BlockFlag == 2:
                if CommentItemUsage == CommentItemNotify == DT.ITEM_UNDEFINED:
                    BlockFlag = 4
                else:
                    BlockFlag = 3
            elif BlockFlag == -1:
                BlockFlag = 4  
        
        #
        # Comment USAGE and NOTIFY information are "UNDEFINED"
        #
        if BlockFlag == -1 or BlockFlag == 1 or BlockFlag == 2:                                             
            if CommentItemUsage == CommentItemNotify == DT.ITEM_UNDEFINED:
                if BlockFlag == -1:
                    BlockFlag = 1
                elif BlockFlag == 1:
                    BlockFlag = 2
            else:
                if BlockFlag == 1 or BlockFlag == 2:
                    BlockFlag = 3
                #
                # An item have Usage or Notify information and the first time get this information
                # 
                elif BlockFlag == -1:
                    BlockFlag = 4
                                                                          
        #
        # Combine two comment line if they are generic comment
        #   
        if CommentItemUsage == CommentItemNotify == PreUsage == PreNotify == DT.ITEM_UNDEFINED:
            CommentItemHelpText = PreHelpText + DT.END_OF_LINE + CommentItemHelpText
            #
            # Store this information for next line may still need  combine operation.
            #
            PreHelpText = CommentItemHelpText
            
        if BlockFlag == 4:     
            CommentItemIns = InfPpiItemCommentContent()
            CommentItemIns.SetUsage(CommentItemUsage)
            CommentItemIns.SetNotify(CommentItemNotify)
            CommentItemIns.SetHelpStringItem(CommentItemHelpText)
            CommentInsList.append(CommentItemIns)
            
            BlockFlag = -1
            PreUsage = None
            PreNotify = None
            PreHelpText = ''
            
        elif BlockFlag == 3:
            #
            # Add previous help string
            # 
            CommentItemIns = InfPpiItemCommentContent()
            CommentItemIns.SetUsage(DT.ITEM_UNDEFINED)
            CommentItemIns.SetNotify(DT.ITEM_UNDEFINED)
            if PreHelpText == '' or PreHelpText.endswith(DT.END_OF_LINE):
                PreHelpText += DT.END_OF_LINE            
            CommentItemIns.SetHelpStringItem(PreHelpText)
            CommentInsList.append(CommentItemIns)
            #
            # Add Current help string
            #
            CommentItemIns = InfPpiItemCommentContent()
            CommentItemIns.SetUsage(CommentItemUsage)
            CommentItemIns.SetNotify(CommentItemNotify)
            CommentItemIns.SetHelpStringItem(CommentItemHelpText)
            CommentInsList.append(CommentItemIns)
            
            BlockFlag = -1
            PreUsage = None
            PreNotify = None
            PreHelpText = ''
        else:
            PreUsage = CommentItemUsage
            PreNotify = CommentItemNotify
            PreHelpText = CommentItemHelpText
            
    InfPpiItemObj.SetCommentList(CommentInsList)
    
    return InfPpiItemObj

class InfPpiItemCommentContent():
    def __init__(self):
        #
        # ## SOMETIMES_CONSUMES ## HelpString 
        #
        self.UsageItem = ''
        #
        # Help String
        #
        self.HelpStringItem = ''
        self.Notify = ''
        self.CommentList = []
                
    def SetUsage(self, UsageItem):
        self.UsageItem = UsageItem
    def GetUsage(self):
        return self.UsageItem
    
    def SetNotify(self, Notify):
        if Notify != DT.ITEM_UNDEFINED:
            self.Notify = 'true'
    def GetNotify(self):
        return self.Notify
          
    def SetHelpStringItem(self, HelpStringItem):
        self.HelpStringItem = HelpStringItem
    def GetHelpStringItem(self):
        return self.HelpStringItem
    
class InfPpiItem():
    def __init__(self):
        self.Name             = ''
        self.FeatureFlagExp   = ''    
        self.SupArchList      = []
        self.CommentList      = []
        
    def SetName(self, Name):
        self.Name = Name
    def GetName(self):
        return self.Name

    def SetSupArchList(self, SupArchList):
        self.SupArchList = SupArchList
    def GetSupArchList(self):
        return self.SupArchList        

    def SetCommentList(self, CommentList):
        self.CommentList = CommentList
    def GetCommentList(self):
        return self.CommentList

    def SetFeatureFlagExp(self, FeatureFlagExp):
        self.FeatureFlagExp = FeatureFlagExp
    def GetFeatureFlagExp(self):
        return self.FeatureFlagExp
##
#
#
#
class InfPpiObject():
    def __init__(self):
        self.Ppis = Sdict()
        #
        # Macro defined in this section should be only used in this section.
        #
        self.Macros = {}
    
    def SetPpi(self, PpiList, Arch = None):
        __SupArchList = []
        for ArchItem in Arch:
            #
            # Validate Arch
            #            
            if (ArchItem == '' or ArchItem == None):
                ArchItem = 'COMMON'   
            __SupArchList.append(ArchItem)
            
        for Item in PpiList:
            #
            # Get Comment content of this protocol
            #
            CommentsList = None
            if len(Item) == 3:
                CommentsList = Item[1]
            CurrentLineOfItem = Item[2]
            Item = Item[0]
            InfPpiItemObj = InfPpiItem()                  
            if len(Item) >= 1 and len(Item) <= 2:
                #
                # Only CName contained
                #
                if not IsValidCVariableName(Item[0]):
                    Logger.Error("InfParser", 
                                 ToolError.FORMAT_INVALID, 
                                 ST.ERR_INF_PARSER_INVALID_CNAME%(Item[0]),
                                 File=CurrentLineOfItem[2], 
                                 Line=CurrentLineOfItem[1], 
                                 ExtraData=CurrentLineOfItem[0])
                if (Item[0] != ''):
                    InfPpiItemObj.SetName(Item[0])
                else:
                    Logger.Error("InfParser", 
                                 ToolError.FORMAT_INVALID, 
                                 ST.ERR_INF_PARSER_CNAME_MISSING,
                                 File=CurrentLineOfItem[2], 
                                 Line=CurrentLineOfItem[1], 
                                 ExtraData=CurrentLineOfItem[0])
            #
            # Have FeatureFlag information
            #
            if len(Item) == 2:
                #
                # Contained CName and Feature Flag Express
                # <statements>           ::=  <CName> ["|" <FeatureFlagExpress>]
                # Item[1] should not be empty  
                #
                if Item[1].strip() == '':
                    Logger.Error("InfParser", 
                                 ToolError.FORMAT_INVALID, 
                                 ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_MISSING,
                                 File=CurrentLineOfItem[2], 
                                 Line=CurrentLineOfItem[1], 
                                 ExtraData=CurrentLineOfItem[0])
                #
                # Validate Feature Flag Express for PPI entry
                # Item[1] contain FFE information
                #
                FeatureFlagRtv = IsValidFeatureFlagExp(Item[1].strip())
                if not FeatureFlagRtv[0]:
                    Logger.Error("InfParser", 
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID%(FeatureFlagRtv[1]),
                                 File=CurrentLineOfItem[2], 
                                 Line=CurrentLineOfItem[1], 
                                 ExtraData=CurrentLineOfItem[0])
                InfPpiItemObj.SetFeatureFlagExp(Item[1])
            if len(Item) != 1 and len(Item) != 2:
                #
                # Invalid format of Ppi statement 
                #
                Logger.Error("InfParser", 
                             ToolError.FORMAT_INVALID,
                             ST.ERR_INF_PARSER_GUID_PPI_PROTOCOL_SECTION_CONTENT_ERROR,
                             File=CurrentLineOfItem[2], 
                             Line=CurrentLineOfItem[1], 
                             ExtraData=CurrentLineOfItem[0])
            
            #
            # Get/Set Usage and HelpString for PPI entry
            #
            if CommentsList != None and len(CommentsList) != 0:
                InfPpiItemObj = ParsePpiComment(CommentsList, InfPpiItemObj)
            else:
                CommentItemIns = InfPpiItemCommentContent()
                CommentItemIns.SetUsage(DT.ITEM_UNDEFINED)
                CommentItemIns.SetNotify(DT.ITEM_UNDEFINED)
                InfPpiItemObj.SetCommentList([CommentItemIns])
            
            InfPpiItemObj.SetSupArchList(__SupArchList)

            #
            # Determine PPI name duplicate. Follow below rule:
            #
            # A PPI must not be duplicated within a [Ppis] section. 
            # A PPI may appear in multiple architectural [Ppis] 
            # sections. A PPI listed in an architectural [Ppis] 
            # section must not be listed in the common architectural 
            # [Ppis] section.
            # 
            # NOTE: This check will not report error now.
            # 
            for Item in self.Ppis:
                if Item.GetName() == InfPpiItemObj.GetName():
                    ItemSupArchList = Item.GetSupArchList()
                    for ItemArch in ItemSupArchList:
                        for PpiItemObjArch in __SupArchList:
                            if ItemArch == PpiItemObjArch:
                                #
                                # ST.ERR_INF_PARSER_ITEM_DUPLICATE
                                #
                                pass
                            if ItemArch.upper() == 'COMMON' or PpiItemObjArch.upper() == 'COMMON':
                                #
                                # ST.ERR_INF_PARSER_ITEM_DUPLICATE_COMMON
                                # 
                                pass
            
            if self.Ppis.has_key((InfPpiItemObj)):           
                PpiList = self.Ppis[InfPpiItemObj]
                PpiList.append(InfPpiItemObj)
                self.Ppis[InfPpiItemObj] = PpiList
            else:
                PpiList = []
                PpiList.append(InfPpiItemObj)
                self.Ppis[InfPpiItemObj] = PpiList
                
        return True        
        
    
    def GetPpi(self):
        return self.Ppis