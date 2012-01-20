## @file
# This file is used to define class objects of INF file [Depex] section. 
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
InfDepexObject
'''

from Library import DataType as DT
from Library import GlobalData
import Logger.Log as Logger
from Logger import ToolError
from Logger import StringTable as ST

from Object.Parser.InfCommonObject import InfSectionCommonDef
from Library.ParserValidate import IsValidArch

class InfDepexContentItem():
    def __init__(self):
        self.SectionType = ''
        self.SectionString = ''

    def SetSectionType(self, SectionType):
        self.SectionType = SectionType
    def GetSectionType(self):
        return self.SectionType

    def SetSectionString(self, SectionString):
        self.SectionString = SectionString
    def GetSectionString(self):
        return self.SectionString


class InfDepexItem():
    def __init__(self):
        self.DepexContent = ''
        self.ModuleType = ''
        self.SupArch = ''
        self.HelpString = ''
        self.FeatureFlagExp = ''
        self.InfDepexContentItemList = []

    def SetFeatureFlagExp(self, FeatureFlagExp):
        self.FeatureFlagExp = FeatureFlagExp
    def GetFeatureFlagExp(self):
        return self.FeatureFlagExp

    def SetSupArch(self, Arch):
        self.SupArch = Arch
    def GetSupArch(self):
        return self.SupArch

    def SetHelpString(self, HelpString):
        self.HelpString = HelpString
    def GetHelpString(self):
        return self.HelpString

    def SetModuleType(self, Type):
        self.ModuleType = Type
    def GetModuleType(self):
        return self.ModuleType

    def SetDepexConent(self, Content):
        self.DepexContent = Content
    def GetDepexContent(self):
        return self.DepexContent

    def SetInfDepexContentItemList(self, InfDepexContentItemList):
        self.InfDepexContentItemList = InfDepexContentItemList
    def GetInfDepexContentItemList(self):
        return self.InfDepexContentItemList

## InfDepexObject
#
#
#
class InfDepexObject(InfSectionCommonDef):
    def __init__(self):
        self.Depex = []
        self.AllContent = ''
        self.SectionContent = ''
        InfSectionCommonDef.__init__(self)

    def SetDepex(self, DepexContent, KeyList=None, CommentList=None):
        for KeyItem in KeyList:
            Arch = KeyItem[0]
            ModuleType = KeyItem[1]
            InfDepexItemIns = InfDepexItem()

            #
            # Validate Arch
            #                 
            if IsValidArch(Arch.strip().upper()):
                InfDepexItemIns.SetSupArch(Arch)
            else:
                Logger.Error("InfParser",
                             ToolError.FORMAT_INVALID,
                             ST.ERR_INF_PARSER_DEFINE_NAME_INVALID % (Arch),
                             File=GlobalData.gINF_MODULE_NAME,
                             Line=KeyItem[2])

            #
            # Validate Module Type
            #
            if ModuleType and ModuleType != 'COMMON':
                if ModuleType in DT.VALID_DEPEX_MODULE_TYPE_LIST:
                    InfDepexItemIns.SetModuleType(ModuleType)
                else:
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_DEPEX_SECTION_MODULE_TYPE_ERROR % (ModuleType),
                                 File=GlobalData.gINF_MODULE_NAME,
                                 Line=KeyItem[2])

            #
            # Parser content in [Depex] section.
            #
            DepexString = ''
            HelpString = ''
            #
            # Get Depex Expression
            #
            for Line in DepexContent:
                LineContent = Line[0].strip()
                if LineContent.find(DT.TAB_COMMENT_SPLIT) > -1:
                    LineContent = LineContent[:LineContent.find(DT.TAB_COMMENT_SPLIT)]
                if LineContent:
                    DepexString = DepexString + LineContent + DT.END_OF_LINE
                continue

            if DepexString.endswith(DT.END_OF_LINE):
                DepexString = DepexString[:-1]

            if not DepexString.strip():
                continue

            #
            # Get Help Text
            #
            for HelpLine in CommentList:
                HelpString = HelpString + HelpLine + DT.END_OF_LINE
            if HelpString.endswith(DT.END_OF_LINE):
                HelpString = HelpString[:-1]

            InfDepexItemIns.SetDepexConent(DepexString)
            InfDepexItemIns.SetHelpString(HelpString)

            self.Depex.append(InfDepexItemIns)

        return True

    def GetDepex(self):
        return self.Depex

    def GetAllContent(self):
        return self.AllContent
