## @file
# This file is used to define class objects of INF file [Guids] section.
# It will consumed by InfParser.
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

'''
InfGuidObject
'''

from Library.ParserValidate import IsValidCVariableName
from Library.CommentParsing import ParseComment
from Library.ExpressionValidate import IsValidFeatureFlagExp

from Library.Misc import Sdict
from Library import DataType as DT
import Logger.Log as Logger
from Logger import ToolError
from Logger import StringTable as ST

class InfGuidItemCommentContent():
    def __init__(self):
        #
        # ## SOMETIMES_CONSUMES ## Variable:L"MemoryTypeInformation"
        # TailString.
        #
        #
        # SOMETIMES_CONSUMES
        #
        self.UsageItem = ''
        #
        # Variable
        #
        self.GuidTypeItem = ''
        #
        # MemoryTypeInformation
        #
        self.VariableNameItem = ''
        #
        # TailString
        #
        self.HelpStringItem = ''

    def SetUsageItem(self, UsageItem):
        self.UsageItem = UsageItem
    def GetUsageItem(self):
        return self.UsageItem

    def SetGuidTypeItem(self, GuidTypeItem):
        self.GuidTypeItem = GuidTypeItem
    def GetGuidTypeItem(self):
        return self.GuidTypeItem

    def SetVariableNameItem(self, VariableNameItem):
        self.VariableNameItem = VariableNameItem
    def GetVariableNameItem(self):
        return self.VariableNameItem

    def SetHelpStringItem(self, HelpStringItem):
        self.HelpStringItem = HelpStringItem
    def GetHelpStringItem(self):
        return self.HelpStringItem

class InfGuidItem():
    def __init__(self):
        self.Name = ''
        self.FeatureFlagExp = ''
        #
        # A list contain instance of InfGuidItemCommentContent
        #
        self.CommentList = []
        self.SupArchList = []

    def SetName(self, Name):
        self.Name = Name
    def GetName(self):
        return self.Name

    def SetFeatureFlagExp(self, FeatureFlagExp):
        self.FeatureFlagExp = FeatureFlagExp
    def GetFeatureFlagExp(self):
        return self.FeatureFlagExp

    def SetCommentList(self, CommentList):
        self.CommentList = CommentList
    def GetCommentList(self):
        return self.CommentList

    def SetSupArchList(self, SupArchList):
        self.SupArchList = SupArchList
    def GetSupArchList(self):
        return self.SupArchList

## ParseComment
#
# ParseComment
#
def ParseGuidComment(CommentsList, InfGuidItemObj):
    #
    # Get/Set Usage and HelpString
    #
    if CommentsList is not None and len(CommentsList) != 0 :
        CommentInsList = []
        PreUsage = None
        PreGuidType = None
        PreHelpText = ''
        BlockFlag = -1
        Count = 0
        for CommentItem in CommentsList:
            Count = Count + 1
            CommentItemUsage, \
            CommentItemGuidType, \
            CommentItemVarString, \
            CommentItemHelpText = \
                    ParseComment(CommentItem,
                                 DT.ALL_USAGE_TOKENS,
                                 DT.GUID_TYPE_TOKENS,
                                 [],
                                 True)

            if CommentItemHelpText is None:
                CommentItemHelpText = ''
                if Count == len(CommentsList) and CommentItemUsage == CommentItemGuidType == DT.ITEM_UNDEFINED:
                    CommentItemHelpText = DT.END_OF_LINE

            if Count == len(CommentsList):
                if BlockFlag == 1 or BlockFlag == 2:
                    if CommentItemUsage == CommentItemGuidType == DT.ITEM_UNDEFINED:
                        BlockFlag = 4
                    else:
                        BlockFlag = 3
                if BlockFlag == -1:
                    BlockFlag = 4
            if BlockFlag == -1 or BlockFlag == 1 or BlockFlag == 2:
                if CommentItemUsage == CommentItemGuidType == DT.ITEM_UNDEFINED:
                    if BlockFlag == -1:
                        BlockFlag = 1
                    elif BlockFlag == 1:
                        BlockFlag = 2
                else:
                    if BlockFlag == 1 or BlockFlag == 2:
                        BlockFlag = 3
                    elif BlockFlag == -1:
                        BlockFlag = 4

            #
            # Combine two comment line if they are generic comment
            #
            if CommentItemUsage == CommentItemGuidType == PreUsage == PreGuidType == DT.ITEM_UNDEFINED:
                CommentItemHelpText = PreHelpText + DT.END_OF_LINE + CommentItemHelpText
                PreHelpText = CommentItemHelpText

            if BlockFlag == 4:
                CommentItemIns = InfGuidItemCommentContent()
                CommentItemIns.SetUsageItem(CommentItemUsage)
                CommentItemIns.SetGuidTypeItem(CommentItemGuidType)
                CommentItemIns.SetVariableNameItem(CommentItemVarString)
                if CommentItemHelpText == '' or CommentItemHelpText.endswith(DT.END_OF_LINE):
                    CommentItemHelpText = CommentItemHelpText.strip(DT.END_OF_LINE)
                CommentItemIns.SetHelpStringItem(CommentItemHelpText)
                CommentInsList.append(CommentItemIns)

                BlockFlag = -1
                PreUsage = None
                PreGuidType = None
                PreHelpText = ''

            elif BlockFlag == 3:
                #
                # Add previous help string
                #
                CommentItemIns = InfGuidItemCommentContent()
                CommentItemIns.SetUsageItem(DT.ITEM_UNDEFINED)
                CommentItemIns.SetGuidTypeItem(DT.ITEM_UNDEFINED)
                if PreHelpText == '' or PreHelpText.endswith(DT.END_OF_LINE):
                    PreHelpText = PreHelpText.strip(DT.END_OF_LINE)
                CommentItemIns.SetHelpStringItem(PreHelpText)
                CommentInsList.append(CommentItemIns)
                #
                # Add Current help string
                #
                CommentItemIns = InfGuidItemCommentContent()
                CommentItemIns.SetUsageItem(CommentItemUsage)
                CommentItemIns.SetGuidTypeItem(CommentItemGuidType)
                CommentItemIns.SetVariableNameItem(CommentItemVarString)
                if CommentItemHelpText == '' or CommentItemHelpText.endswith(DT.END_OF_LINE):
                    CommentItemHelpText = CommentItemHelpText.strip(DT.END_OF_LINE)
                CommentItemIns.SetHelpStringItem(CommentItemHelpText)
                CommentInsList.append(CommentItemIns)

                BlockFlag = -1
                PreUsage = None
                PreGuidType = None
                PreHelpText = ''

            else:
                PreUsage = CommentItemUsage
                PreGuidType = CommentItemGuidType
                PreHelpText = CommentItemHelpText

        InfGuidItemObj.SetCommentList(CommentInsList)
    else:
        #
        # Still need to set the USAGE/GUIDTYPE to undefined.
        #
        CommentItemIns = InfGuidItemCommentContent()
        CommentItemIns.SetUsageItem(DT.ITEM_UNDEFINED)
        CommentItemIns.SetGuidTypeItem(DT.ITEM_UNDEFINED)
        InfGuidItemObj.SetCommentList([CommentItemIns])

    return InfGuidItemObj

## InfGuidObject
#
# InfGuidObject
#
class InfGuidObject():
    def __init__(self):
        self.Guids = Sdict()
        #
        # Macro defined in this section should be only used in this section.
        #
        self.Macros = {}

    def SetGuid(self, GuidList, Arch = None):
        __SupportArchList = []
        for ArchItem in Arch:
            #
            # Validate Arch
            #
            if (ArchItem == '' or ArchItem is None):
                ArchItem = 'COMMON'

            __SupportArchList.append(ArchItem)

        for Item in GuidList:
            #
            # Get Comment content of this protocol
            #
            CommentsList = None
            if len(Item) == 3:
                CommentsList = Item[1]
            CurrentLineOfItem = Item[2]
            Item = Item[0]
            InfGuidItemObj = InfGuidItem()
            if len(Item) >= 1 and len(Item) <= 2:
                #
                # Only GuildName contained
                #
                if not IsValidCVariableName(Item[0]):
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_INVALID_CNAME%(Item[0]),
                                 File=CurrentLineOfItem[2],
                                 Line=CurrentLineOfItem[1],
                                 ExtraData=CurrentLineOfItem[0])
                if (Item[0] != ''):
                    InfGuidItemObj.SetName(Item[0])
                else:
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_CNAME_MISSING,
                                 File=CurrentLineOfItem[2],
                                 Line=CurrentLineOfItem[1],
                                 ExtraData=CurrentLineOfItem[0])
            if len(Item) == 2:
                #
                # Contained CName and Feature Flag Express
                # <statements>           ::=  <CName> ["|" <FeatureFlagExpress>]
                # For GUID entry.
                #
                if Item[1].strip() == '':
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_MISSING,
                                 File=CurrentLineOfItem[2],
                                 Line=CurrentLineOfItem[1],
                                 ExtraData=CurrentLineOfItem[0])
                #
                # Validate Feature Flag Express
                #
                FeatureFlagRtv = IsValidFeatureFlagExp(Item[1].strip())
                if not FeatureFlagRtv[0]:
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID%(FeatureFlagRtv[1]),
                                 File=CurrentLineOfItem[2],
                                 Line=CurrentLineOfItem[1],
                                 ExtraData=CurrentLineOfItem[0])
                InfGuidItemObj.SetFeatureFlagExp(Item[1])
            if len(Item) != 1 and len(Item) != 2:
                #
                # Invalid format of GUID statement
                #
                Logger.Error("InfParser",
                             ToolError.FORMAT_INVALID,
                             ST.ERR_INF_PARSER_GUID_PPI_PROTOCOL_SECTION_CONTENT_ERROR,
                             File=CurrentLineOfItem[2],
                             Line=CurrentLineOfItem[1],
                             ExtraData=CurrentLineOfItem[0])

            InfGuidItemObj = ParseGuidComment(CommentsList, InfGuidItemObj)
            InfGuidItemObj.SetSupArchList(__SupportArchList)

            #
            # Determine GUID name duplicate. Follow below rule:
            #
            # A GUID must not be duplicated within a [Guids] section.
            # A GUID may appear in multiple architectural [Guids]
            # sections. A GUID listed in an architectural [Guids]
            # section must not be listed in the common architectural
            # [Guids] section.
            #
            # NOTE: This check will not report error now.
            #
            for Item in self.Guids:
                if Item.GetName() == InfGuidItemObj.GetName():
                    ItemSupArchList = Item.GetSupArchList()
                    for ItemArch in ItemSupArchList:
                        for GuidItemObjArch in __SupportArchList:
                            if ItemArch == GuidItemObjArch:
                                #
                                # ST.ERR_INF_PARSER_ITEM_DUPLICATE
                                #
                                pass

                            if ItemArch.upper() == 'COMMON' or GuidItemObjArch.upper() == 'COMMON':
                                #
                                # ST.ERR_INF_PARSER_ITEM_DUPLICATE_COMMON
                                #
                                pass

            if (InfGuidItemObj) in self.Guids:
                GuidList = self.Guids[InfGuidItemObj]
                GuidList.append(InfGuidItemObj)
                self.Guids[InfGuidItemObj] = GuidList
            else:
                GuidList = []
                GuidList.append(InfGuidItemObj)
                self.Guids[InfGuidItemObj] = GuidList

        return True

    def GetGuid(self):
        return self.Guids
