## @file
# This file is used to define class objects of INF file [Binaries] section. 
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
InfBinaryObject
'''

import os

from copy import deepcopy
from Library import DataType as DT
from Library import GlobalData
import Logger.Log as Logger
from Logger import ToolError
from Logger import StringTable as ST
from Library.Misc import Sdict

from Object.Parser.InfCommonObject import InfSectionCommonDef
from Object.Parser.InfCommonObject import CurrentLine
from Library.Misc import ConvPathFromAbsToRel
from Library.ExpressionValidate import IsValidFeatureFlagExp
from Library.Misc import ValidFile
from Library.ParserValidate import IsValidPath


class InfBianryItem():
    def __init__(self):
        self.FileName = ''
        self.Target = ''
        self.FeatureFlagExp = ''
        self.HelpString = ''
        self.Type = ''
        self.SupArchList = []

    def SetFileName(self, FileName):
        self.FileName = FileName
    def GetFileName(self):
        return self.FileName

    def SetTarget(self, Target):
        self.Target = Target
    def GetTarget(self):
        return self.Target

    def SetFeatureFlagExp(self, FeatureFlagExp):
        self.FeatureFlagExp = FeatureFlagExp
    def GetFeatureFlagExp(self):
        return self.FeatureFlagExp

    def SetHelpString(self, HelpString):
        self.HelpString = HelpString
    def GetHelpString(self):
        return self.HelpString

    def SetType(self, Type):
        self.Type = Type
    def GetType(self):
        return self.Type
    def SetSupArchList(self, SupArchList):
        self.SupArchList = SupArchList
    def GetSupArchList(self):
        return self.SupArchList

class InfBianryVerItem(InfBianryItem, CurrentLine):
    def __init__(self):
        InfBianryItem.__init__(self)
        CurrentLine.__init__(self)
        self.VerTypeName = ''

    def SetVerTypeName(self, VerTypeName):
        self.VerTypeName = VerTypeName
    def GetVerTypeName(self):
        return self.VerTypeName

class InfBianryUiItem(InfBianryItem, CurrentLine):
    def __init__(self):
        InfBianryItem.__init__(self)
        CurrentLine.__init__(self)
        self.UiTypeName = ''

    def SetUiTypeName(self, UiTypeName):
        self.UiTypeName = UiTypeName
    def GetVerTypeName(self):
        return self.UiTypeName

class InfBianryCommonItem(InfBianryItem, CurrentLine):
    def __init__(self):
        self.CommonType = ''
        self.TagName = ''
        self.Family = ''
        InfBianryItem.__init__(self)
        CurrentLine.__init__(self)

    def SetCommonType(self, CommonType):
        self.CommonType = CommonType
    def GetCommonType(self):
        return self.CommonType

    def SetTagName(self, TagName):
        self.TagName = TagName
    def GetTagName(self):
        return self.TagName

    def SetFamily(self, Family):
        self.Family = Family
    def GetFamily(self):
        return self.Family

##
#
#
#
class InfBinariesObject(InfSectionCommonDef):
    def __init__(self):
        self.Binaries = Sdict()
        #
        # Macro defined in this section should be only used in this section.
        #
        self.Macros = {}
        InfSectionCommonDef.__init__(self)

    ## CheckVer
    #
    #
    def CheckVer(self, Ver, __SupArchList):
        #
        # Check Ver
        #
        for VerItem in Ver:
            IsValidFileFlag = False
            VerContent = VerItem[0]
            VerComment = VerItem[1]
            VerCurrentLine = VerItem[2]
            GlobalData.gINF_CURRENT_LINE = VerCurrentLine
            InfBianryVerItemObj = None
            #
            # Should not less than 2 elements
            #
            if len(VerContent) < 2:
                Logger.Error("InfParser",
                             ToolError.FORMAT_INVALID,
                             ST.ERR_INF_PARSER_BINARY_ITEM_FORMAT_INVALID % (VerContent[0]),
                             File=VerCurrentLine.GetFileName(),
                             Line=VerCurrentLine.GetLineNo(),
                             ExtraData=VerCurrentLine.GetLineString())
                return False
            if len(VerContent) > 4:
                Logger.Error("InfParser",
                             ToolError.FORMAT_INVALID,
                             ST.ERR_INF_PARSER_BINARY_ITEM_FORMAT_INVALID_MAX % (VerContent[0], 4),
                             File=VerCurrentLine.GetFileName(),
                             Line=VerCurrentLine.GetLineNo(),
                             ExtraData=VerCurrentLine.GetLineString())
                return False
            if len(VerContent) >= 2:
                #
                # Create a Ver Object.
                #
                InfBianryVerItemObj = InfBianryVerItem()

                if VerContent[0] != DT.BINARY_FILE_TYPE_VER:
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_BINARY_VER_TYPE % DT.BINARY_FILE_TYPE_VER,
                                 File=VerCurrentLine.GetFileName(),
                                 Line=VerCurrentLine.GetLineNo(),
                                 ExtraData=VerCurrentLine.GetLineString())

                InfBianryVerItemObj.SetVerTypeName(VerContent[0])
                InfBianryVerItemObj.SetType(VerContent[0])
                #
                # Verify File exist or not
                #
                FullFileName = os.path.normpath(os.path.realpath(os.path.join(GlobalData.gINF_MODULE_DIR,
                                                                              VerContent[1])))
                if not (ValidFile(FullFileName) or ValidFile(VerContent[1])):
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_BINARY_ITEM_FILE_NOT_EXIST % (VerContent[1]),
                                 File=VerCurrentLine.GetFileName(),
                                 Line=VerCurrentLine.GetLineNo(),
                                 ExtraData=VerCurrentLine.GetLineString())
                #
                # Validate file exist/format.
                #
                if IsValidPath(VerContent[1], GlobalData.gINF_MODULE_DIR):
                    IsValidFileFlag = True
                else:
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID % (VerContent[1]),
                                 File=VerCurrentLine.GetFileName(),
                                 Line=VerCurrentLine.GetLineNo(),
                                 ExtraData=VerCurrentLine.GetLineString())
                    return False
                if IsValidFileFlag:
                    VerContent[0] = ConvPathFromAbsToRel(VerContent[0],
                                            GlobalData.gINF_MODULE_DIR)
                    InfBianryVerItemObj.SetFileName(VerContent[1])
            if len(VerContent) >= 3:
                #
                # Add Target information
                #
                InfBianryVerItemObj.SetTarget(VerContent[2])
            if len(VerContent) == 4:
                if VerContent[3].strip() == '':
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_MISSING,
                                 File=VerCurrentLine.GetFileName(),
                                 Line=VerCurrentLine.GetLineNo(),
                                 ExtraData=VerCurrentLine.GetLineString())
                #
                # Validate Feature Flag Express   
                #
                FeatureFlagRtv = IsValidFeatureFlagExp(VerContent[3].\
                                                       strip())
                if not FeatureFlagRtv[0]:
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID % (FeatureFlagRtv[1]),
                                 File=VerCurrentLine.GetFileName(),
                                 Line=VerCurrentLine.GetLineNo(),
                                 ExtraData=VerCurrentLine.GetLineString())
                InfBianryVerItemObj.SetFeatureFlagExp(VerContent[3])

            InfBianryVerItemObj.SetSupArchList(__SupArchList)

            #
            # Determine binary file name duplicate. Follow below rule:
            #
            # A binary filename must not be duplicated within 
            # a [Binaries] section. A binary filename may appear in 
            # multiple architectural [Binaries] sections. A binary 
            # filename listed in an architectural [Binaries] section 
            # must not be listed in the common architectural 
            # [Binaries] section.
            # 
            # NOTE: This check will not report error now.
            # 
            for Item in self.Binaries:
                if Item.GetFileName() == InfBianryVerItemObj.GetFileName():
                    ItemSupArchList = Item.GetSupArchList()
                    for ItemArch in ItemSupArchList:
                        for VerItemObjArch in __SupArchList:
                            if ItemArch == VerItemObjArch:
                                #
                                # ST.ERR_INF_PARSER_ITEM_DUPLICATE
                                #
                                pass
                            if ItemArch.upper() == 'COMMON' or VerItemObjArch.upper() == 'COMMON':
                                #
                                # ERR_INF_PARSER_ITEM_DUPLICATE_COMMON
                                #
                                pass

            if InfBianryVerItemObj != None:
                if self.Binaries.has_key((InfBianryVerItemObj)):
                    BinariesList = self.Binaries[InfBianryVerItemObj]
                    BinariesList.append((InfBianryVerItemObj, VerComment))
                    self.Binaries[InfBianryVerItemObj] = BinariesList
                else:
                    BinariesList = []
                    BinariesList.append((InfBianryVerItemObj, VerComment))
                    self.Binaries[InfBianryVerItemObj] = BinariesList

    ## ParseCommonBinary
    #
    # ParseCommonBinary
    #
    def ParseCommonBinary(self, CommonBinary, __SupArchList):
        #
        # Check common binary definitions
        # Type | FileName | Target | Family | TagName | FeatureFlagExp
        #
        for Item in CommonBinary:
            IsValidFileFlag = False
            ItemContent = Item[0]
            ItemComment = Item[1]
            CurrentLineOfItem = Item[2]
            GlobalData.gINF_CURRENT_LINE = CurrentLineOfItem
            InfBianryCommonItemObj = None
            if len(ItemContent) < 2:
                Logger.Error("InfParser",
                             ToolError.FORMAT_INVALID,
                             ST.ERR_INF_PARSER_BINARY_ITEM_FORMAT_INVALID % (ItemContent[0]),
                             File=CurrentLineOfItem.GetFileName(),
                             Line=CurrentLineOfItem.GetLineNo(),
                             ExtraData=CurrentLineOfItem.GetLineString())
                return False
            if len(ItemContent) > 6:
                Logger.Error("InfParser",
                             ToolError.FORMAT_INVALID,
                             ST.ERR_INF_PARSER_BINARY_ITEM_FORMAT_INVALID_MAX % (ItemContent[0], 6),
                             File=CurrentLineOfItem.GetFileName(),
                             Line=CurrentLineOfItem.GetLineNo(),
                             ExtraData=CurrentLineOfItem.GetLineString())
                return False
            if len(ItemContent) >= 2:
                #
                # Create a Common Object.
                #
                InfBianryCommonItemObj = InfBianryCommonItem()
                #
                # Convert Binary type.
                #
                BinaryFileType = ItemContent[0].strip()
                if BinaryFileType == 'RAW' or BinaryFileType == 'ACPI' or BinaryFileType == 'ASL':
                    BinaryFileType = 'BIN'

                if BinaryFileType not in DT.BINARY_FILE_TYPE_LIST:
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_BINARY_ITEM_INVALID_FILETYPE % \
                                 (DT.BINARY_FILE_TYPE_LIST.__str__()),
                                 File=CurrentLineOfItem.GetFileName(),
                                 Line=CurrentLineOfItem.GetLineNo(),
                                 ExtraData=CurrentLineOfItem.GetLineString())
                    
                if BinaryFileType == 'SUBTYPE_GUID':
                    BinaryFileType = 'FREEFORM'
                    
                if BinaryFileType == 'LIB' or BinaryFileType == 'UEFI_APP':
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_BINARY_ITEM_INVALID_FILETYPE % \
                                 (DT.BINARY_FILE_TYPE_LIST.__str__()),
                                 File=CurrentLineOfItem.GetFileName(),
                                 Line=CurrentLineOfItem.GetLineNo(),
                                 ExtraData=CurrentLineOfItem.GetLineString())

                InfBianryCommonItemObj.SetType(BinaryFileType)
                InfBianryCommonItemObj.SetCommonType(ItemContent[0])
                #
                # Verify File exist or not
                #
                FullFileName = os.path.normpath(os.path.realpath(os.path.join(GlobalData.gINF_MODULE_DIR,
                                                                              ItemContent[1])))
                if not (ValidFile(FullFileName) or ValidFile(ItemContent[1])):
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_BINARY_ITEM_FILE_NOT_EXIST % (ItemContent[1]),
                                 File=CurrentLineOfItem.GetFileName(),
                                 Line=CurrentLineOfItem.GetLineNo(),
                                 ExtraData=CurrentLineOfItem.GetLineString())
                #
                # Validate file exist/format.
                #
                if IsValidPath(ItemContent[1], GlobalData.gINF_MODULE_DIR):
                    IsValidFileFlag = True
                else:
                    Logger.Error("InfParser",
                                ToolError.FORMAT_INVALID,
                                ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID % (ItemContent[1]),
                                File=CurrentLineOfItem.GetFileName(),
                                Line=CurrentLineOfItem.GetLineNo(),
                                ExtraData=CurrentLineOfItem.GetLineString())
                    return False
                if IsValidFileFlag:
                    ItemContent[0] = ConvPathFromAbsToRel(ItemContent[0], GlobalData.gINF_MODULE_DIR)
                    InfBianryCommonItemObj.SetFileName(ItemContent[1])
            if len(ItemContent) >= 3:
                #
                # Add Target information
                #
                InfBianryCommonItemObj.SetTarget(ItemContent[2])
            if len(ItemContent) >= 4:
                #
                # Add Family information
                #
                InfBianryCommonItemObj.SetFamily(ItemContent[3])
            if len(ItemContent) >= 5:
                #
                # TagName entries are build system specific. If there 
                # is content in the entry, the tool must exit 
                # gracefully with an error message that indicates build
                # system specific content cannot be distributed using 
                # the UDP
                #
                if ItemContent[4].strip() != '':
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_TAGNAME_NOT_PERMITTED % (ItemContent[4]),
                                 File=CurrentLineOfItem.GetFileName(),
                                 Line=CurrentLineOfItem.GetLineNo(),
                                 ExtraData=CurrentLineOfItem.GetLineString())
            if len(ItemContent) == 6:
                #
                # Add FeatureFlagExp
                #
                if ItemContent[5].strip() == '':
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_MISSING,
                                 File=CurrentLineOfItem.GetFileName(),
                                 Line=CurrentLineOfItem.GetLineNo(),
                                 ExtraData=CurrentLineOfItem.GetLineString())
                #
                # Validate Feature Flag Express   
                #
                FeatureFlagRtv = IsValidFeatureFlagExp(ItemContent[5].strip())
                if not FeatureFlagRtv[0]:
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID % (FeatureFlagRtv[1]),
                                 File=CurrentLineOfItem.GetFileName(),
                                 Line=CurrentLineOfItem.GetLineNo(),
                                 ExtraData=CurrentLineOfItem.GetLineString())
                InfBianryCommonItemObj.SetFeatureFlagExp(ItemContent[5])

            InfBianryCommonItemObj.SetSupArchList(__SupArchList)

            #
            # Determine binary file name duplicate. Follow below rule:
            #
            # A binary filename must not be duplicated within 
            # a [Binaries] section. A binary filename may appear in 
            # multiple architectural [Binaries] sections. A binary 
            # filename listed in an architectural [Binaries] section 
            # must not be listed in the common architectural 
            # [Binaries] section.
            # 
            # NOTE: This check will not report error now.
            # 
#            for Item in self.Binaries:
#                if Item.GetFileName() == InfBianryCommonItemObj.GetFileName():
#                    ItemSupArchList = Item.GetSupArchList()
#                    for ItemArch in ItemSupArchList:
#                        for ComItemObjArch in __SupArchList:
#                            if ItemArch == ComItemObjArch:
#                                #
#                                # ST.ERR_INF_PARSER_ITEM_DUPLICATE
#                                #
#                                pass
#
#                            if ItemArch.upper() == 'COMMON' or ComItemObjArch.upper() == 'COMMON':
#                                #
#                                # ERR_INF_PARSER_ITEM_DUPLICATE_COMMON
#                                #
#                                pass

            if InfBianryCommonItemObj != None:
                if self.Binaries.has_key((InfBianryCommonItemObj)):
                    BinariesList = self.Binaries[InfBianryCommonItemObj]
                    BinariesList.append((InfBianryCommonItemObj, ItemComment))
                    self.Binaries[InfBianryCommonItemObj] = BinariesList
                else:
                    BinariesList = []
                    BinariesList.append((InfBianryCommonItemObj, ItemComment))
                    self.Binaries[InfBianryCommonItemObj] = BinariesList

    def SetBinary(self, UiInf=None, Ver=None, CommonBinary=None, ArchList=None):

        __SupArchList = []
        for ArchItem in ArchList:
            #
            # Validate Arch
            #            
            if (ArchItem == '' or ArchItem == None):
                ArchItem = 'COMMON'
            __SupArchList.append(ArchItem)

        if UiInf != None:
            if len(UiInf) > 0:
                #
                # Check UI
                #                    
                for UiItem in UiInf:
                    IsValidFileFlag = False
                    InfBianryUiItemObj = None
                    UiContent = UiItem[0]
                    UiComment = UiItem[1]
                    UiCurrentLine = UiItem[2]
                    GlobalData.gINF_CURRENT_LINE = deepcopy(UiItem[2])
                    #
                    # Should not less than 2 elements
                    #
                    if len(UiContent) < 2:
                        Logger.Error("InfParser",
                                     ToolError.FORMAT_INVALID,
                                     ST.ERR_INF_PARSER_BINARY_ITEM_FORMAT_INVALID % (UiContent[0]),
                                     File=UiCurrentLine.GetFileName(),
                                     Line=UiCurrentLine.GetLineNo(),
                                     ExtraData=UiCurrentLine.GetLineString())
                        return False

                    if len(UiContent) > 4:
                        Logger.Error("InfParser",
                                     ToolError.FORMAT_INVALID,
                                     ST.ERR_INF_PARSER_BINARY_ITEM_FORMAT_INVALID_MAX % (UiContent[0], 4),
                                     File=UiCurrentLine.GetFileName(),
                                     Line=UiCurrentLine.GetLineNo(),
                                     ExtraData=UiCurrentLine.GetLineString())
                        return False
                    if len(UiContent) >= 2:
                        #
                        # Create an Ui Object.
                        #
                        InfBianryUiItemObj = InfBianryUiItem()
                        if UiContent[0] != 'UI':
                            Logger.Error("InfParser",
                                         ToolError.FORMAT_INVALID,
                                         ST.ERR_INF_PARSER_BINARY_VER_TYPE % ('UI'),
                                         File=UiCurrentLine.GetFileName(),
                                         Line=UiCurrentLine.GetLineNo(),
                                         ExtraData=UiCurrentLine.GetLineString())
                        InfBianryUiItemObj.SetUiTypeName(UiContent[0])
                        InfBianryUiItemObj.SetType(UiContent[0])
                        #
                        # Verify File exist or not
                        #
                        FullFileName = os.path.normpath(os.path.realpath(os.path.join(GlobalData.gINF_MODULE_DIR,
                                                                                      UiContent[1])))
                        if not (ValidFile(FullFileName) or ValidFile(UiContent[1])):
                            Logger.Error("InfParser",
                                         ToolError.FORMAT_INVALID,
                                         ST.ERR_INF_PARSER_BINARY_ITEM_FILE_NOT_EXIST % (UiContent[1]),
                                         File=UiCurrentLine.GetFileName(),
                                         Line=UiCurrentLine.GetLineNo(),
                                         ExtraData=UiCurrentLine.GetLineString())
                        #
                        # Validate file exist/format.
                        #
                        if IsValidPath(UiContent[1], GlobalData.gINF_MODULE_DIR):
                            IsValidFileFlag = True
                        else:
                            Logger.Error("InfParser",
                                         ToolError.FORMAT_INVALID,
                                         ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID % (UiContent[1]),
                                         File=UiCurrentLine.GetFileName(),
                                         Line=UiCurrentLine.GetLineNo(),
                                         ExtraData=UiCurrentLine.GetLineString())
                            return False
                        if IsValidFileFlag:
                            UiContent[0] = ConvPathFromAbsToRel(UiContent[0], GlobalData.gINF_MODULE_DIR)
                            InfBianryUiItemObj.SetFileName(UiContent[1])
                    if len(UiContent) >= 3:
                        #
                        # Add Target information
                        #
                        InfBianryUiItemObj.SetTarget(UiContent[2])
                    if len(UiContent) == 4:
                        if UiContent[3].strip() == '':
                            Logger.Error("InfParser",
                                         ToolError.FORMAT_INVALID,
                                         ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_MISSING,
                                         File=UiCurrentLine.GetFileName(),
                                         Line=UiCurrentLine.GetLineNo(),
                                         ExtraData=UiCurrentLine.GetLineString())
                        #
                        # Validate Feature Flag Express   
                        #
                        FeatureFlagRtv = IsValidFeatureFlagExp(UiContent[3].strip())
                        if not FeatureFlagRtv[0]:
                            Logger.Error("InfParser",
                                         ToolError.FORMAT_INVALID,
                                         ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID % (FeatureFlagRtv[1]),
                                         File=UiCurrentLine.GetFileName(),
                                         Line=UiCurrentLine.GetLineNo(),
                                         ExtraData=UiCurrentLine.GetLineString())
                        InfBianryUiItemObj.SetFeatureFlagExp(UiContent[3])

                    InfBianryUiItemObj.SetSupArchList(__SupArchList)

                    #
                    # Determine binary file name duplicate. Follow below rule:
                    #
                    # A binary filename must not be duplicated within 
                    # a [Binaries] section. A binary filename may appear in 
                    # multiple architectural [Binaries] sections. A binary 
                    # filename listed in an architectural [Binaries] section 
                    # must not be listed in the common architectural 
                    # [Binaries] section.
                    # 
                    # NOTE: This check will not report error now.
                    # 
#                    for Item in self.Binaries:
#                        if Item.GetFileName() == InfBianryUiItemObj.GetFileName():
#                            ItemSupArchList = Item.GetSupArchList()
#                            for ItemArch in ItemSupArchList:
#                                for UiItemObjArch in __SupArchList:
#                                    if ItemArch == UiItemObjArch:
#                                        #
#                                        # ST.ERR_INF_PARSER_ITEM_DUPLICATE
#                                        #
#                                        pass
#                                    if ItemArch.upper() == 'COMMON' or UiItemObjArch.upper() == 'COMMON':
#                                        #
#                                        # ERR_INF_PARSER_ITEM_DUPLICATE_COMMON
#                                        #
#                                        pass

                    if InfBianryUiItemObj != None:
                        if self.Binaries.has_key((InfBianryUiItemObj)):
                            BinariesList = self.Binaries[InfBianryUiItemObj]
                            BinariesList.append((InfBianryUiItemObj, UiComment))
                            self.Binaries[InfBianryUiItemObj] = BinariesList
                        else:
                            BinariesList = []
                            BinariesList.append((InfBianryUiItemObj, UiComment))
                            self.Binaries[InfBianryUiItemObj] = BinariesList
        if Ver != None and len(Ver) > 0:
            self.CheckVer(Ver, __SupArchList)
        if CommonBinary and len(CommonBinary) > 0:
            self.ParseCommonBinary(CommonBinary, __SupArchList)

        return True

    def GetBinary(self):
        return self.Binaries
