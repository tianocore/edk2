## @file
# This file is used to define class objects of INF file [LibraryClasses] section. 
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
InfLibraryClassesObject
'''

from Logger import StringTable as ST
from Logger import ToolError
import Logger.Log as Logger
from Library import GlobalData

from Library.Misc import Sdict
from Object.Parser.InfCommonObject import CurrentLine
from Library.ExpressionValidate import IsValidFeatureFlagExp
from Library.ParserValidate import IsValidLibName

## GetArchModuleType
#
# Get Arch List and ModuleType List
#
def GetArchModuleType(KeyList):
    __SupArchList = []
    __SupModuleList = []

    for (ArchItem, ModuleItem) in KeyList:
        #
        # Validate Arch
        #            
        if (ArchItem == '' or ArchItem == None):
            ArchItem = 'COMMON'

        if (ModuleItem == '' or ModuleItem == None):
            ModuleItem = 'COMMON'

        if ArchItem not in __SupArchList:
            __SupArchList.append(ArchItem)

        List = ModuleItem.split('|')
        for Entry in List:
            if Entry not in __SupModuleList:
                __SupModuleList.append(Entry)

    return (__SupArchList, __SupModuleList)


class InfLibraryClassItem():
    def __init__(self, LibName='', FeatureFlagExp='', HelpString=None):
        self.LibName = LibName
        self.FeatureFlagExp = FeatureFlagExp
        self.HelpString = HelpString
        self.CurrentLine = CurrentLine()
        self.SupArchList = []
        self.SupModuleList = []
        self.FileGuid = ''
        self.Version = ''

    def SetLibName(self, LibName):
        self.LibName = LibName
    def GetLibName(self):
        return self.LibName

    def SetHelpString(self, HelpString):
        self.HelpString = HelpString
    def GetHelpString(self):
        return self.HelpString

    def SetFeatureFlagExp(self, FeatureFlagExp):
        self.FeatureFlagExp = FeatureFlagExp
    def GetFeatureFlagExp(self):
        return self.FeatureFlagExp

    def SetSupArchList(self, SupArchList):
        self.SupArchList = SupArchList
    def GetSupArchList(self):
        return self.SupArchList

    def SetSupModuleList(self, SupModuleList):
        self.SupModuleList = SupModuleList
    def GetSupModuleList(self):
        return self.SupModuleList

    #
    # As Build related information
    #
    def SetFileGuid(self, FileGuid):
        self.FileGuid = FileGuid
    def GetFileGuid(self):
        return self.FileGuid

    def SetVersion(self, Version):
        self.Version = Version
    def GetVersion(self):
        return self.Version

## INF LibraryClass Section
#
#
#
class InfLibraryClassObject():
    def __init__(self):
        self.LibraryClasses = Sdict()
        #
        # Macro defined in this section should be only used in this section.
        #
        self.Macros = {}

    ##SetLibraryClasses
    #
    # 
    # @param HelpString:     It can be a common comment or contain a recommend
    #                        instance.
    #
    def SetLibraryClasses(self, LibContent, KeyList=None):
        #
        # Validate Arch
        #
        (__SupArchList, __SupModuleList) = GetArchModuleType(KeyList)

        for LibItem in LibContent:
            LibItemObj = InfLibraryClassItem()
            if not GlobalData.gIS_BINARY_INF:
                HelpStringObj = LibItem[1]
                LibItemObj.CurrentLine.SetFileName(LibItem[2][2])
                LibItemObj.CurrentLine.SetLineNo(LibItem[2][1])
                LibItemObj.CurrentLine.SetLineString(LibItem[2][0])
                LibItem = LibItem[0]
                if HelpStringObj != None:
                    LibItemObj.SetHelpString(HelpStringObj)
                if len(LibItem) >= 1:
                    if LibItem[0].strip() != '':
                        if IsValidLibName(LibItem[0].strip()):
                            if LibItem[0].strip() != 'NULL':
                                LibItemObj.SetLibName(LibItem[0])
                            else:
                                Logger.Error("InfParser",
                                         ToolError.FORMAT_INVALID,
                                         ST.ERR_INF_PARSER_DEFINE_LIB_NAME_INVALID,
                                         File=GlobalData.gINF_MODULE_NAME,
                                         Line=LibItemObj.CurrentLine.GetLineNo(),
                                         ExtraData=LibItemObj.CurrentLine.GetLineString())
                        else:
                            Logger.Error("InfParser",
                                         ToolError.FORMAT_INVALID,
                                         ST.ERR_INF_PARSER_DEFINE_FROMAT_INVALID % (LibItem[0]),
                                         File=GlobalData.gINF_MODULE_NAME,
                                         Line=LibItemObj.CurrentLine.GetLineNo(),
                                         ExtraData=LibItemObj.CurrentLine.GetLineString())
                    else:
                        Logger.Error("InfParser",
                                     ToolError.FORMAT_INVALID,
                                     ST.ERR_INF_PARSER_LIBRARY_SECTION_LIBNAME_MISSING,
                                     File=GlobalData.gINF_MODULE_NAME,
                                     Line=LibItemObj.CurrentLine.GetLineNo(),
                                     ExtraData=LibItemObj.CurrentLine.GetLineString())
                if len(LibItem) == 2:
                    if LibItem[1].strip() == '':
                        Logger.Error("InfParser",
                                     ToolError.FORMAT_INVALID,
                                     ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_MISSING,
                                     File=GlobalData.gINF_MODULE_NAME,
                                     Line=LibItemObj.CurrentLine.GetLineNo(),
                                     ExtraData=LibItemObj.CurrentLine.GetLineString())
                    #
                    # Validate FFE    
                    #
                    FeatureFlagRtv = IsValidFeatureFlagExp(LibItem[1].strip())
                    if not FeatureFlagRtv[0]:
                        Logger.Error("InfParser",
                                     ToolError.FORMAT_INVALID,
                                     ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID % (FeatureFlagRtv[1]),
                                     File=GlobalData.gINF_MODULE_NAME,
                                     Line=LibItemObj.CurrentLine.GetLineNo(),
                                     ExtraData=LibItemObj.CurrentLine.GetLineString())
                    LibItemObj.SetFeatureFlagExp(LibItem[1].strip())

                #
                # Invalid strings
                #
                if len(LibItem) < 1 or len(LibItem) > 2:
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_LIBRARY_SECTION_CONTENT_ERROR,
                                 File=GlobalData.gINF_MODULE_NAME,
                                 Line=LibItemObj.CurrentLine.GetLineNo(),
                                 ExtraData=LibItemObj.CurrentLine.GetLineString())

                LibItemObj.SetSupArchList(__SupArchList)
                LibItemObj.SetSupModuleList(__SupModuleList)

                #
                # Determine Library class duplicate. Follow below rule:
                #
                # A library class keyword must not be duplicated within a 
                # [LibraryClasses] section. Library class keywords may appear in 
                # multiple architectural and module type [LibraryClasses] sections. 
                # A library class keyword listed in an architectural or module type 
                # [LibraryClasses] section must not be listed in the common 
                # architectural or module type [LibraryClasses] section.
                # 
                # NOTE: This check will not report error now. But keep code for future enhancement.
                # 
#                for Item in self.LibraryClasses:
#                    if Item.GetLibName() == LibItemObj.GetLibName():
#                        ItemSupArchList = Item.GetSupArchList()
#                        ItemSupModuleList = Item.GetSupModuleList()
#                        for ItemArch in ItemSupArchList:
#                            for ItemModule in ItemSupModuleList:
#                                for LibItemObjArch in __SupArchList:
#                                    for LibItemObjModule in __SupModuleList:
#                                        if ItemArch == LibItemObjArch and LibItemObjModule == ItemModule:
#                                            #
#                                            # ERR_INF_PARSER_ITEM_DUPLICATE
#                                            #
#                                            pass
#                                        if (ItemArch.upper() == 'COMMON' or LibItemObjArch.upper() == 'COMMON') \
#                                           and LibItemObjModule == ItemModule:
#                                            #
#                                            # ERR_INF_PARSER_ITEM_DUPLICATE_COMMON
#                                            #
#                                            pass
            else:
                #
                # Assume the file GUID is well formatted.
                #
                LibItemObj.SetFileGuid(LibItem[0])
                LibItemObj.SetVersion(LibItem[1])

            if self.LibraryClasses.has_key((LibItemObj)):
                LibraryList = self.LibraryClasses[LibItemObj]
                LibraryList.append(LibItemObj)
                self.LibraryClasses[LibItemObj] = LibraryList
            else:
                LibraryList = []
                LibraryList.append(LibItemObj)
                self.LibraryClasses[LibItemObj] = LibraryList

        return True

    def GetLibraryClasses(self):
        return self.LibraryClasses
