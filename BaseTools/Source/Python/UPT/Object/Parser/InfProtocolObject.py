## @file
# This file is used to define class objects of INF file [Protocols] section.
# It will consumed by InfParser.
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

'''
InfProtocolObject
'''

from Library.ParserValidate import IsValidCVariableName
from Library.CommentParsing import ParseComment
from Library.ExpressionValidate import IsValidFeatureFlagExp

from Library.Misc import Sdict

from Object.Parser.InfMisc import ErrorInInf

from Library import DataType as DT
from Logger import StringTable as ST

def ParseProtocolComment(CommentsList, InfProtocolItemObj):
    CommentInsList = []
    PreUsage = None
    PreNotify = None
    PreHelpText = ''
    BlockFlag = -1
    Count = 0
    for CommentItem in CommentsList:
        Count = Count + 1
        CommentItemUsage, \
        CommentItemNotify, \
        CommentItemString, \
        CommentItemHelpText = \
                ParseComment(CommentItem,
                             DT.PROTOCOL_USAGE_TOKENS,
                             DT.PROTOCOL_NOTIFY_TOKENS,
                             ['PROTOCOL'],
                             False)

        if CommentItemString:
            pass

        if CommentItemHelpText is None:
            CommentItemHelpText = ''
            if Count == len(CommentsList) and CommentItemUsage == CommentItemNotify == DT.ITEM_UNDEFINED:
                CommentItemHelpText = DT.END_OF_LINE

        if Count == len(CommentsList):
            if BlockFlag == 1 or BlockFlag == 2:
                if CommentItemUsage == CommentItemNotify == DT.ITEM_UNDEFINED:
                    BlockFlag = 4
                else:
                    BlockFlag = 3
            elif BlockFlag == -1:
                BlockFlag = 4

        if BlockFlag == -1 or BlockFlag == 1 or BlockFlag == 2:
            if CommentItemUsage == CommentItemNotify == DT.ITEM_UNDEFINED:
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
        if CommentItemUsage == CommentItemNotify == PreUsage == PreNotify == DT.ITEM_UNDEFINED:
            CommentItemHelpText = PreHelpText + DT.END_OF_LINE + CommentItemHelpText

            PreHelpText = CommentItemHelpText

        if BlockFlag == 4:
            CommentItemIns = InfProtocolItemCommentContent()
            CommentItemIns.SetUsageItem(CommentItemUsage)
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
            CommentItemIns = InfProtocolItemCommentContent()
            CommentItemIns.SetUsageItem(DT.ITEM_UNDEFINED)
            CommentItemIns.SetNotify(DT.ITEM_UNDEFINED)
            if PreHelpText == '' or PreHelpText.endswith(DT.END_OF_LINE):
                PreHelpText += DT.END_OF_LINE
            CommentItemIns.SetHelpStringItem(PreHelpText)
            CommentInsList.append(CommentItemIns)
            #
            # Add Current help string
            #
            CommentItemIns = InfProtocolItemCommentContent()
            CommentItemIns.SetUsageItem(CommentItemUsage)
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

    InfProtocolItemObj.SetCommentList(CommentInsList)

    return InfProtocolItemObj

class InfProtocolItemCommentContent():
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

    def SetUsageItem(self, UsageItem):
        self.UsageItem = UsageItem
    def GetUsageItem(self):
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

class InfProtocolItem():
    def __init__(self):
        self.Name = ''
        self.FeatureFlagExp = ''
        self.SupArchList = []
        self.CommentList = []

    def SetName(self, Name):
        self.Name = Name
    def GetName(self):
        return self.Name

    def SetFeatureFlagExp(self, FeatureFlagExp):
        self.FeatureFlagExp = FeatureFlagExp
    def GetFeatureFlagExp(self):
        return self.FeatureFlagExp

    def SetSupArchList(self, SupArchList):
        self.SupArchList = SupArchList
    def GetSupArchList(self):
        return self.SupArchList

    def SetCommentList(self, CommentList):
        self.CommentList = CommentList
    def GetCommentList(self):
        return self.CommentList

##
#
#
#
class InfProtocolObject():
    def __init__(self):
        self.Protocols = Sdict()
        #
        # Macro defined in this section should be only used in this section.
        #
        self.Macros = {}

    def SetProtocol(self, ProtocolContent, Arch = None,):
        __SupArchList = []
        for ArchItem in Arch:
            #
            # Validate Arch
            #
            if (ArchItem == '' or ArchItem is None):
                ArchItem = 'COMMON'
            __SupArchList.append(ArchItem)

        for Item in ProtocolContent:
            #
            # Get Comment content of this protocol
            #
            CommentsList = None
            if len(Item) == 3:
                CommentsList = Item[1]
            CurrentLineOfItem = Item[2]
            LineInfo = (CurrentLineOfItem[2], CurrentLineOfItem[1], CurrentLineOfItem[0])
            Item = Item[0]
            InfProtocolItemObj = InfProtocolItem()
            if len(Item) >= 1 and len(Item) <= 2:
                #
                # Only CName contained
                #
                if not IsValidCVariableName(Item[0]):
                    ErrorInInf(ST.ERR_INF_PARSER_INVALID_CNAME%(Item[0]),
                               LineInfo=LineInfo)
                if (Item[0] != ''):
                    InfProtocolItemObj.SetName(Item[0])
                else:
                    ErrorInInf(ST.ERR_INF_PARSER_CNAME_MISSING,
                               LineInfo=LineInfo)
            if len(Item) == 2:
                #
                # Contained CName and Feature Flag Express
                # <statements>           ::=  <CName> ["|"
                # <FeatureFlagExpress>]
                # For Protocol Object
                #
                if Item[1].strip() == '':
                    ErrorInInf(ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_MISSING,
                               LineInfo=LineInfo)
                #
                # Validate Feature Flag Express for Item[1]
                #
                FeatureFlagRtv = IsValidFeatureFlagExp(Item[1].strip())
                if not FeatureFlagRtv[0]:
                    ErrorInInf(ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID%(FeatureFlagRtv[1]),
                               LineInfo=LineInfo)
                InfProtocolItemObj.SetFeatureFlagExp(Item[1])

            if len(Item) < 1 or len(Item) > 2:
                #
                # Invalid format of Protocols statement
                #
                ErrorInInf(ST.ERR_INF_PARSER_GUID_PPI_PROTOCOL_SECTION_CONTENT_ERROR,
                           LineInfo=LineInfo)

            #
            # Get/Set Usage and HelpString for Protocol entry
            #
            if CommentsList is not None and len(CommentsList) != 0:
                InfProtocolItemObj = ParseProtocolComment(CommentsList, InfProtocolItemObj)
            else:
                CommentItemIns = InfProtocolItemCommentContent()
                CommentItemIns.SetUsageItem(DT.ITEM_UNDEFINED)
                CommentItemIns.SetNotify(DT.ITEM_UNDEFINED)
                InfProtocolItemObj.SetCommentList([CommentItemIns])

            InfProtocolItemObj.SetSupArchList(__SupArchList)

            #
            # Determine protocol name duplicate. Follow below rule:
            #
            # A protocol must not be duplicated within a [Protocols] section.
            # A protocol may appear in multiple architectural [Protocols]
            # sections. A protocol listed in an architectural [Protocols]
            # section must not be listed in the common architectural
            # [Protocols] section.
            #
            # NOTE: This check will not report error now.
            #
            for Item in self.Protocols:
                if Item.GetName() == InfProtocolItemObj.GetName():
                    ItemSupArchList = Item.GetSupArchList()
                    for ItemArch in ItemSupArchList:
                        for ProtocolItemObjArch in __SupArchList:
                            if ItemArch == ProtocolItemObjArch:
                                #
                                # ST.ERR_INF_PARSER_ITEM_DUPLICATE
                                #
                                pass
                            if ItemArch.upper() == 'COMMON' or ProtocolItemObjArch.upper() == 'COMMON':
                                #
                                # ST.ERR_INF_PARSER_ITEM_DUPLICATE_COMMON
                                #
                                pass

            if (InfProtocolItemObj) in self.Protocols:
                ProcotolList = self.Protocols[InfProtocolItemObj]
                ProcotolList.append(InfProtocolItemObj)
                self.Protocols[InfProtocolItemObj] = ProcotolList
            else:
                ProcotolList = []
                ProcotolList.append(InfProtocolItemObj)
                self.Protocols[InfProtocolItemObj] = ProcotolList

        return True

    def GetProtocol(self):
        return self.Protocols
