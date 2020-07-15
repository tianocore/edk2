## @file
# This file is used to define a class object to describe a module
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

'''
ModuleObject
'''

##
# Import Modules
#
from Object.POM.CommonObject import CommonPropertiesObject
from Object.POM.CommonObject import IdentificationObject
from Object.POM.CommonObject import CommonHeaderObject
from Object.POM.CommonObject import BinaryHeaderObject
from Object.POM.CommonObject import HelpTextListObject
from Object.POM.CommonObject import GuidVersionObject


##
# BootModeObject
#
class BootModeObject(CommonPropertiesObject, HelpTextListObject):
    def __init__(self):
        self.SupportedBootModes = ''
        CommonPropertiesObject.__init__(self)
        HelpTextListObject.__init__(self)

    def SetSupportedBootModes(self, SupportedBootModes):
        self.SupportedBootModes = SupportedBootModes

    def GetSupportedBootModes(self):
        return self.SupportedBootModes

##
# EventObject
#
class EventObject(CommonPropertiesObject, HelpTextListObject):
    def __init__(self):
        self.EventType = ''
        CommonPropertiesObject.__init__(self)
        HelpTextListObject.__init__(self)

    def SetEventType(self, EventType):
        self.EventType = EventType

    def GetEventType(self):
        return self.EventType

##
# HobObject
#
class HobObject(CommonPropertiesObject, HelpTextListObject):
    def __init__(self):
        self.HobType = ''
        CommonPropertiesObject.__init__(self)
        HelpTextListObject.__init__(self)

    def SetHobType(self, HobType):
        self.HobType = HobType

    def GetHobType(self):
        return self.HobType

##
# SpecObject
#
class SpecObject(object):
    def __init__(self):
        self.Spec = ''
        self.Version = ''

    def SetSpec(self, Spec):
        self.Spec = Spec

    def GetSpec(self):
        return self.Spec

    def SetVersion(self, Version):
        self.Version = Version

    def GetVersion(self):
        return self.Version

## ModuleHeaderObject
#
# This class defined header items used in Module file
#
class ModuleHeaderObject(IdentificationObject, CommonHeaderObject, BinaryHeaderObject):
    def __init__(self):
        self.IsLibrary = False
        self.IsLibraryModList = []
        self.ModuleType = ''
        self.BinaryModule = False
        self.PcdIsDriver = ''
        self.PiSpecificationVersion = ''
        self.UefiSpecificationVersion = ''
        self.UNIFlag = False
        self.ModuleUniFile = ''
        #
        # SpecObject
        #
        self.SpecList = []
        #
        # BootModeObject
        #
        self.BootModeList = []
        #
        # EventObject
        #
        self.EventList = []
        #
        # HobObject
        #
        self.HobList = []
        #
        # LibraryClassObject
        #
        self.LibraryClassList = []
        self.SupArchList = []
        IdentificationObject.__init__(self)
        CommonHeaderObject.__init__(self)
        BinaryHeaderObject.__init__(self)

    def SetIsLibrary(self, IsLibrary):
        self.IsLibrary = IsLibrary

    def GetIsLibrary(self):
        return self.IsLibrary

    def SetIsLibraryModList(self, IsLibraryModList):
        self.IsLibraryModList = IsLibraryModList

    def GetIsLibraryModList(self):
        return self.IsLibraryModList

    def SetModuleType(self, ModuleType):
        self.ModuleType = ModuleType

    def GetModuleType(self):
        return self.ModuleType

    def SetBinaryModule(self, BinaryModule):
        self.BinaryModule = BinaryModule

    def GetBinaryModule(self):
        return self.BinaryModule

    def SetPcdIsDriver(self, PcdIsDriver):
        self.PcdIsDriver = PcdIsDriver

    def GetPcdIsDriver(self):
        return self.PcdIsDriver

    def SetPiSpecificationVersion(self, PiSpecificationVersion):
        self.PiSpecificationVersion = PiSpecificationVersion

    def GetPiSpecificationVersion(self):
        return self.PiSpecificationVersion

    def SetUefiSpecificationVersion(self, UefiSpecificationVersion):
        self.UefiSpecificationVersion = UefiSpecificationVersion

    def GetUefiSpecificationVersion(self):
        return self.UefiSpecificationVersion

    def SetSpecList(self, SpecList):
        self.SpecList = SpecList

    def GetSpecList(self):
        return self.SpecList

    def SetBootModeList(self, BootModeList):
        self.BootModeList = BootModeList

    def GetBootModeList(self):
        return self.BootModeList

    def SetEventList(self, EventList):
        self.EventList = EventList

    def GetEventList(self):
        return self.EventList

    def SetHobList(self, HobList):
        self.HobList = HobList

    def GetHobList(self):
        return self.HobList

    def SetLibraryClassList(self, LibraryClassList):
        self.LibraryClassList = LibraryClassList

    def GetLibraryClassList(self):
        return self.LibraryClassList

    def SetSupArchList(self, SupArchList):
        self.SupArchList = SupArchList

    def GetSupArchList(self):
        return self.SupArchList

    def SetModuleUniFile(self, ModuleUniFile):
        self.ModuleUniFile = ModuleUniFile

    def GetModuleUniFile(self):
        return self.ModuleUniFile
##
# SourceFileObject
#
class SourceFileObject(CommonPropertiesObject):
    def __init__(self):
        CommonPropertiesObject.__init__(self)
        self.SourceFile = ''
        self.TagName = ''
        self.ToolCode = ''
        self.Family = ''
        self.FileType = ''

    def SetSourceFile(self, SourceFile):
        self.SourceFile = SourceFile

    def GetSourceFile(self):
        return  self.SourceFile

    def SetTagName(self, TagName):
        self.TagName = TagName

    def GetTagName(self):
        return self.TagName

    def SetToolCode(self, ToolCode):
        self.ToolCode = ToolCode

    def GetToolCode(self):
        return self.ToolCode

    def SetFamily(self, Family):
        self.Family = Family

    def GetFamily(self):
        return self.Family

    def SetFileType(self, FileType):
        self.FileType = FileType

    def GetFileType(self):
        return self.FileType


##
# BinaryFileObject
#
class BinaryFileObject(CommonPropertiesObject):
    def __init__(self):
        self.FileNamList = []
        self.AsBuiltList = []
        CommonPropertiesObject.__init__(self)

    def SetFileNameList(self, FileNamList):
        self.FileNamList = FileNamList

    def GetFileNameList(self):
        return self.FileNamList

    def SetAsBuiltList(self, AsBuiltList):
        self.AsBuiltList = AsBuiltList

    def GetAsBuiltList(self):
        return self.AsBuiltList


##
# AsBuildLibraryClassObject
#
class AsBuildLibraryClassObject(object):
    def __init__(self):
        self.LibGuid = ''
        self.LibVersion = ''
        self.SupArchList = []

    def SetLibGuid(self, LibGuid):
        self.LibGuid = LibGuid
    def GetLibGuid(self):
        return self.LibGuid

    def SetLibVersion(self, LibVersion):
        self.LibVersion = LibVersion
    def GetLibVersion(self):
        return self.LibVersion

    def SetSupArchList(self, SupArchList):
        self.SupArchList = SupArchList
    def GetSupArchList(self):
        return self.SupArchList

##
# AsBuiltObject
#
class AsBuiltObject(object):
    def __init__(self):
        #
        # list of PcdObject
        #
        self.PatchPcdList = []
        #
        # list of PcdObject
        #
        self.PcdExValueList = []
        #
        # list of GuidVersionObject
        #
        self.LibraryInstancesList = []
        #
        # List of BinaryBuildFlag object
        #
        self.BinaryBuildFlagList = []

    def SetPatchPcdList(self, PatchPcdList):
        self.PatchPcdList = PatchPcdList

    def GetPatchPcdList(self):
        return self.PatchPcdList

    def SetPcdExList(self, PcdExValueList):
        self.PcdExValueList = PcdExValueList

    def GetPcdExList(self):
        return self.PcdExValueList

    def SetLibraryInstancesList(self, LibraryInstancesList):
        self.LibraryInstancesList = LibraryInstancesList

    def GetLibraryInstancesList(self):
        return self.LibraryInstancesList

    def SetBuildFlagsList(self, BinaryBuildFlagList):
        self.BinaryBuildFlagList = BinaryBuildFlagList

    def GetBuildFlagsList(self):
        return self.BinaryBuildFlagList

##
# BinaryBuildFlag, this object will include those fields that are not
# covered by the UPT Spec BinaryFile field
#
class BinaryBuildFlagObject(object):
    def __init__(self):
        self.Target = ''
        self.TagName = ''
        self.Family = ''
        self.AsBuiltOptionFlags = ''

    def SetTarget(self, Target):
        self.Target = Target

    def GetTarget(self):
        return self.Target

    def SetTagName(self, TagName):
        self.TagName = TagName

    def GetTagName(self):
        return self.TagName

    def SetFamily(self, Family):
        self.Family = Family

    def GetFamily(self):
        return self.Family

    def SetAsBuiltOptionFlags(self, AsBuiltOptionFlags):
        self.AsBuiltOptionFlags = AsBuiltOptionFlags
    def GetAsBuiltOptionFlags(self):
        return self.AsBuiltOptionFlags

##
# ExternObject
#
class ExternObject(CommonPropertiesObject):
    def __init__(self):
        self.EntryPoint = ''
        self.UnloadImage = ''
        self.Constructor = ''
        self.Destructor = ''
        self.SupModList = []
        CommonPropertiesObject.__init__(self)

    def SetEntryPoint(self, EntryPoint):
        self.EntryPoint = EntryPoint

    def GetEntryPoint(self):
        return self.EntryPoint

    def SetUnloadImage(self, UnloadImage):
        self.UnloadImage = UnloadImage

    def GetUnloadImage(self):
        return self.UnloadImage

    def SetConstructor(self, Constructor):
        self.Constructor = Constructor

    def GetConstructor(self):
        return self.Constructor

    def SetDestructor(self, Destructor):
        self.Destructor = Destructor

    def GetDestructor(self):
        return self.Destructor

    def SetSupModList(self, SupModList):
        self.SupModList = SupModList
    def GetSupModList(self):
        return self.SupModList

##
# DepexObject
#
class DepexObject(CommonPropertiesObject):
    def __init__(self):
        self.Depex = ''
        self.ModuelType = ''
        CommonPropertiesObject.__init__(self)

    def SetDepex(self, Depex):
        self.Depex = Depex

    def GetDepex(self):
        return self.Depex

    def SetModuleType(self, ModuleType):
        self.ModuelType = ModuleType

    def GetModuleType(self):
        return self.ModuelType

##
# PackageDependencyObject
#
class PackageDependencyObject(GuidVersionObject, CommonPropertiesObject):
    def __init__(self):
        self.Package = ''
        self.PackageFilePath = ''
        GuidVersionObject.__init__(self)
        CommonPropertiesObject.__init__(self)

    def SetPackageFilePath(self, PackageFilePath):
        self.PackageFilePath = PackageFilePath

    def GetPackageFilePath(self):
        return self.PackageFilePath

    def SetPackage(self, Package):
        self.Package = Package

    def GetPackage(self):
        return self.Package

##
# BuildOptionObject
#
class BuildOptionObject(CommonPropertiesObject):
    def __init__(self):
        CommonPropertiesObject.__init__(self)
        self.BuildOption = ''

    def SetBuildOption(self, BuildOption):
        self.BuildOption = BuildOption

    def GetBuildOption(self):
        return self.BuildOption

##
# ModuleObject
#
class ModuleObject(ModuleHeaderObject):
    def __init__(self):
        #
        # {Arch : ModuleHeaderObject}
        #
        self.HeaderDict = {}
        #
        # LibraryClassObject
        #
        self.LibraryClassList = []
        #
        # SourceFileObject
        #
        self.SourceFileList = []
        #
        # BinaryFileObject
        #
        self.BinaryFileList = []
        #
        # PackageDependencyObject
        #
        self.PackageDependencyList = []
        #
        # DepexObject
        #
        self.PeiDepex = []
        #
        # DepexObject
        #
        self.DxeDepex = []
        #
        # DepexObject
        #
        self.SmmDepex = []
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
        # PcdObject
        #
        self.PcdList = []
        #
        # ExternObject
        #
        self.ExternList = []
        #
        # BuildOptionObject
        #
        self.BuildOptionList = []
        #
        # UserExtensionObject
        #
        self.UserExtensionList = []
        #
        # MiscFileObject
        #
        self.MiscFileList = []
        #
        # ClonedFromObject
        #
        self.ClonedFrom = None

        ModuleHeaderObject.__init__(self)

    def SetHeaderDict(self, HeaderDict):
        self.HeaderDict = HeaderDict

    def GetHeaderDict(self):
        return self.HeaderDict

    def SetLibraryClassList(self, LibraryClassList):
        self.LibraryClassList = LibraryClassList

    def GetLibraryClassList(self):
        return self.LibraryClassList

    def SetSourceFileList(self, SourceFileList):
        self.SourceFileList = SourceFileList

    def GetSourceFileList(self):
        return self.SourceFileList

    def SetBinaryFileList(self, BinaryFileList):
        self.BinaryFileList = BinaryFileList

    def GetBinaryFileList(self):
        return self.BinaryFileList

    def SetPackageDependencyList(self, PackageDependencyList):
        self.PackageDependencyList = PackageDependencyList

    def GetPackageDependencyList(self):
        return self.PackageDependencyList

    def SetPeiDepex(self, PeiDepex):
        self.PeiDepex = PeiDepex

    def GetPeiDepex(self):
        return self.PeiDepex

    def SetDxeDepex(self, DxeDepex):
        self.DxeDepex = DxeDepex

    def GetDxeDepex(self):
        return self.DxeDepex

    def SetSmmDepex(self, SmmDepex):
        self.SmmDepex = SmmDepex

    def GetSmmDepex(self):
        return self.SmmDepex

    def SetPpiList(self, PpiList):
        self.PpiList = PpiList

    def GetPpiList(self):
        return self.PpiList

    def SetProtocolList(self, ProtocolList):
        self.ProtocolList = ProtocolList

    def GetProtocolList(self):
        return self.ProtocolList

    def SetPcdList(self, PcdList):
        self.PcdList = PcdList

    def GetPcdList(self):
        return self.PcdList

    def SetGuidList(self, GuidList):
        self.GuidList = GuidList

    def GetGuidList(self):
        return self.GuidList

    def SetExternList(self, ExternList):
        self.ExternList = ExternList

    def GetExternList(self):
        return self.ExternList

    def SetBuildOptionList(self, BuildOptionList):
        self.BuildOptionList = BuildOptionList

    def GetBuildOptionList(self):
        return self.BuildOptionList

    def SetUserExtensionList(self, UserExtensionList):
        self.UserExtensionList = UserExtensionList

    def GetUserExtensionList(self):
        return self.UserExtensionList

    def SetMiscFileList(self, MiscFileList):
        self.MiscFileList = MiscFileList

    def GetMiscFileList(self):
        return self.MiscFileList

    def SetClonedFrom(self, ClonedFrom):
        self.ClonedFrom = ClonedFrom

    def GetClonedFrom(self):
        return self.ClonedFrom
