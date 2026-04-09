## @file
# This file is used to define class objects of INF file [Packages] section.
# It will consumed by InfParser.
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

'''
InfPackageObject
'''

from Logger import StringTable as ST
from Logger import ToolError
import Logger.Log as Logger
from Library import GlobalData

from Library.Misc import Sdict
from Library.ParserValidate import IsValidPath
from Library.ExpressionValidate import IsValidFeatureFlagExp

class InfPackageItem():
    def __init__(self,
                 PackageName = '',
                 FeatureFlagExp = '',
                 HelpString = ''):
        self.PackageName    = PackageName
        self.FeatureFlagExp = FeatureFlagExp
        self.HelpString     = HelpString
        self.SupArchList    = []

    def SetPackageName(self, PackageName):
        self.PackageName = PackageName
    def GetPackageName(self):
        return self.PackageName

    def SetFeatureFlagExp(self, FeatureFlagExp):
        self.FeatureFlagExp = FeatureFlagExp
    def GetFeatureFlagExp(self):
        return self.FeatureFlagExp

    def SetHelpString(self, HelpString):
        self.HelpString = HelpString
    def GetHelpString(self):
        return self.HelpString

    def SetSupArchList(self, SupArchList):
        self.SupArchList = SupArchList
    def GetSupArchList(self):
        return self.SupArchList


## INF package section
#
#
#
class InfPackageObject():
    def __init__(self):
        self.Packages = Sdict()
        #
        # Macro defined in this section should be only used in this section.
        #
        self.Macros         = {}

    def SetPackages(self, PackageData, Arch = None):
        IsValidFileFlag = False
        SupArchList     = []
        for ArchItem in Arch:
            #
            # Validate Arch
            #
            if (ArchItem == '' or ArchItem is None):
                ArchItem = 'COMMON'
            SupArchList.append(ArchItem)

        for PackageItem in PackageData:
            PackageItemObj = InfPackageItem()
            HelpStringObj = PackageItem[1]
            CurrentLineOfPackItem = PackageItem[2]
            PackageItem = PackageItem[0]
            if HelpStringObj is not None:
                HelpString = HelpStringObj.HeaderComments + HelpStringObj.TailComments
                PackageItemObj.SetHelpString(HelpString)
            if len(PackageItem) >= 1:
                #
                # Validate file exist/format.
                #
                if IsValidPath(PackageItem[0], ''):
                    IsValidFileFlag = True
                elif IsValidPath(PackageItem[0], GlobalData.gINF_MODULE_DIR):
                    IsValidFileFlag = True
                elif IsValidPath(PackageItem[0], GlobalData.gWORKSPACE):
                    IsValidFileFlag = True
                else:
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID%(PackageItem[0]),
                                 File=CurrentLineOfPackItem[2],
                                 Line=CurrentLineOfPackItem[1],
                                 ExtraData=CurrentLineOfPackItem[0])
                    return False
                if IsValidFileFlag:
                    PackageItemObj.SetPackageName(PackageItem[0])
            if len(PackageItem) == 2:
                #
                # Validate Feature Flag Express
                #
                if PackageItem[1].strip() == '':
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_MISSING,
                                 File=CurrentLineOfPackItem[2],
                                 Line=CurrentLineOfPackItem[1],
                                 ExtraData=CurrentLineOfPackItem[0])
                #
                # Validate FFE
                #
                FeatureFlagRtv = IsValidFeatureFlagExp(PackageItem[1].strip())
                if not FeatureFlagRtv[0]:
                    Logger.Error("InfParser",
                                 ToolError.FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FEATURE_FLAG_EXP_SYNTAX_INVLID%(FeatureFlagRtv[1]),
                                 File=CurrentLineOfPackItem[2],
                                 Line=CurrentLineOfPackItem[1],
                                 ExtraData=CurrentLineOfPackItem[0])

                PackageItemObj.SetFeatureFlagExp(PackageItem[1].strip())

            if len(PackageItem) > 2:
                #
                # Invalid format of Package statement
                #
                Logger.Error("InfParser",
                             ToolError.FORMAT_INVALID,
                             ST.ERR_INF_PARSER_PACKAGE_SECTION_CONTENT_ERROR,
                             File=CurrentLineOfPackItem[2],
                             Line=CurrentLineOfPackItem[1],
                             ExtraData=CurrentLineOfPackItem[0])
            PackageItemObj.SetSupArchList(SupArchList)

            #
            # Determine package file name duplicate. Follow below rule:
            #
            # A package filename must not be duplicated within a [Packages]
            # section. Package filenames may appear in multiple architectural
            # [Packages] sections. A package filename listed in an
            # architectural [Packages] section must not be listed in the common
            # architectural [Packages] section.
            #
            # NOTE: This check will not report error now.
            #
            for Item in self.Packages:
                if Item.GetPackageName() == PackageItemObj.GetPackageName():
                    ItemSupArchList = Item.GetSupArchList()
                    for ItemArch in ItemSupArchList:
                        for PackageItemObjArch in SupArchList:
                            if ItemArch == PackageItemObjArch:
                                #
                                # ST.ERR_INF_PARSER_ITEM_DUPLICATE
                                #
                                pass
                            if ItemArch.upper() == 'COMMON' or PackageItemObjArch.upper() == 'COMMON':
                                #
                                # ST.ERR_INF_PARSER_ITEM_DUPLICATE_COMMON
                                #
                                pass

            if (PackageItemObj) in self.Packages:
                PackageList = self.Packages[PackageItemObj]
                PackageList.append(PackageItemObj)
                self.Packages[PackageItemObj] = PackageList
            else:
                PackageList = []
                PackageList.append(PackageItemObj)
                self.Packages[PackageItemObj] = PackageList

        return True

    def GetPackages(self, Arch = None):
        if Arch is None:
            return self.Packages
