## @file
# This file is used to define a class object to describe a package
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

'''
PackageObject
'''

##
# Import Modules
#
from Object.POM.CommonObject import CommonPropertiesObject
from Object.POM.CommonObject import IdentificationObject
from Object.POM.CommonObject import CommonHeaderObject
from Object.POM.CommonObject import BinaryHeaderObject
from Library.Misc import Sdict

## StandardIncludeFileObject
#
class StandardIncludeFileObject(CommonPropertiesObject):
    def __init__(self):
        CommonPropertiesObject.__init__(self)
        self.IncludeFile = ''

    def SetIncludeFile(self, IncludeFile):
        self.IncludeFile = IncludeFile

    def GetIncludeFile(self):
        return self.IncludeFile

## PackageIncludeFileObject
#
class PackageIncludeFileObject(StandardIncludeFileObject):
    pass

##
# PackageObject
#
class PackageObject(IdentificationObject, CommonHeaderObject, BinaryHeaderObject):
    def __init__(self):
        IdentificationObject.__init__(self)
        CommonHeaderObject.__init__(self)
        BinaryHeaderObject.__init__(self)
        #
        # LibraryClassObject
        #
        self.LibraryClassList = []
        #
        # FileObject
        #
        self.IncludePathList = []
        #
        # StandardIncludeFileObject
        #
        self.StandardIncludeFileList = []
        #
        # PackageIncludeFileObject
        #
        self.PackageIncludeFileList = []
        #
        # Include and Arch List, item is (IncludePath, SupArchList-List of Arch), used during install package
        #
        self.IncludeArchList = []
        #
        # ProtocolObject
        #
        self.ProtocolList = []
        #
        # PpiObject
        #
        self.PpiList = []
        #
        # GuidObject
        #
        self.GuidList = []
        #
        # (PcdObject, PcdErrorObject)
        #
        self.PcdList = []
        #
        # {(PcdTokenSpaceGuidCName, PcdErrroNumber): PcdErrorMessageList}
        #
        self.PcdErrorCommentDict = {}
        #
        # UserExtensionObject
        #
        self.UserExtensionList = []
        #
        # MiscFileObject
        #
        self.MiscFileList = []
        self.ModuleDict = Sdict()
        #
        # ClonedRecordObject
        #
        self.ClonedFromList = []
        #
        # string object
        #
        self.ModuleFileList = []

        self.PcdChecks = []

        self.UNIFlag = False

    def SetLibraryClassList(self, LibraryClassList):
        self.LibraryClassList = LibraryClassList

    def GetLibraryClassList(self):
        return self.LibraryClassList

    def SetIncludePathList(self, IncludePathList):
        self.IncludePathList = IncludePathList

    def GetIncludePathList(self):
        return self.IncludePathList

    def SetIncludeArchList(self, IncludeArchList):
        self.IncludeArchList = IncludeArchList

    def GetIncludeArchList(self):
        return self.IncludeArchList

    def SetStandardIncludeFileList(self, StandardIncludeFileList):
        self.StandardIncludeFileList = StandardIncludeFileList

    def GetStandardIncludeFileList(self):
        return self.StandardIncludeFileList

    def SetPackageIncludeFileList(self, PackageIncludeFileList):
        self.PackageIncludeFileList = PackageIncludeFileList

    def GetPackageIncludeFileList(self):
        return self.PackageIncludeFileList

    def SetProtocolList(self, ProtocolList):
        self.ProtocolList = ProtocolList

    def GetProtocolList(self):
        return self.ProtocolList

    def SetPpiList(self, PpiList):
        self.PpiList = PpiList

    def GetPpiList(self):
        return self.PpiList

    def SetGuidList(self, GuidList):
        self.GuidList = GuidList

    def GetGuidList(self):
        return self.GuidList

    def SetPcdList(self, PcdList):
        self.PcdList = PcdList

    def GetPcdList(self):
        return self.PcdList

    def SetUserExtensionList(self, UserExtensionList):
        self.UserExtensionList = UserExtensionList

    def GetUserExtensionList(self):
        return self.UserExtensionList

    def SetMiscFileList(self, MiscFileList):
        self.MiscFileList = MiscFileList

    def GetMiscFileList(self):
        return self.MiscFileList

    def SetModuleDict(self, ModuleDict):
        self.ModuleDict = ModuleDict

    def GetModuleDict(self):
        return self.ModuleDict

    def SetClonedFromList(self, ClonedFromList):
        self.ClonedFromList = ClonedFromList

    def GetClonedFromList(self):
        return self.ClonedFromList

    def SetModuleFileList(self, ModuleFileList):
        self.ModuleFileList = ModuleFileList

    def GetModuleFileList(self):
        return self.ModuleFileList

