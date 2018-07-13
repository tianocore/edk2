## @file
# This file is used to create a database used by build tool
#
# Copyright (c) 2008 - 2017, Intel Corporation. All rights reserved.<BR>
# (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

from Common.String import *
from Common.DataType import *
from Common.Misc import *
from types import *
from MetaFileParser import *

from Workspace.BuildClassObject import ModuleBuildClassObject, LibraryClassObject, PcdClassObject
## Module build information from INF file
#
#  This class is used to retrieve information stored in database and convert them
# into ModuleBuildClassObject form for easier use for AutoGen.
#
class InfBuildData(ModuleBuildClassObject):
    # dict used to convert PCD type in database to string used by build tool
    _PCD_TYPE_STRING_ = {
        MODEL_PCD_FIXED_AT_BUILD        :   "FixedAtBuild",
        MODEL_PCD_PATCHABLE_IN_MODULE   :   "PatchableInModule",
        MODEL_PCD_FEATURE_FLAG          :   "FeatureFlag",
        MODEL_PCD_DYNAMIC               :   "Dynamic",
        MODEL_PCD_DYNAMIC_DEFAULT       :   "Dynamic",
        MODEL_PCD_DYNAMIC_HII           :   "DynamicHii",
        MODEL_PCD_DYNAMIC_VPD           :   "DynamicVpd",
        MODEL_PCD_DYNAMIC_EX            :   "DynamicEx",
        MODEL_PCD_DYNAMIC_EX_DEFAULT    :   "DynamicEx",
        MODEL_PCD_DYNAMIC_EX_HII        :   "DynamicExHii",
        MODEL_PCD_DYNAMIC_EX_VPD        :   "DynamicExVpd",
    }

    # dict used to convert part of [Defines] to members of InfBuildData directly
    _PROPERTY_ = {
        #
        # Required Fields
        #
        TAB_INF_DEFINES_BASE_NAME                   : "_BaseName",
        TAB_INF_DEFINES_FILE_GUID                   : "_Guid",
        TAB_INF_DEFINES_MODULE_TYPE                 : "_ModuleType",
        #
        # Optional Fields
        #
        # TAB_INF_DEFINES_INF_VERSION                 : "_AutoGenVersion",
        TAB_INF_DEFINES_COMPONENT_TYPE              : "_ComponentType",
        TAB_INF_DEFINES_MAKEFILE_NAME               : "_MakefileName",
        # TAB_INF_DEFINES_CUSTOM_MAKEFILE             : "_CustomMakefile",
        TAB_INF_DEFINES_DPX_SOURCE                  :"_DxsFile",
        TAB_INF_DEFINES_VERSION_NUMBER              : "_Version",
        TAB_INF_DEFINES_VERSION_STRING              : "_Version",
        TAB_INF_DEFINES_VERSION                     : "_Version",
        TAB_INF_DEFINES_PCD_IS_DRIVER               : "_PcdIsDriver",
        TAB_INF_DEFINES_SHADOW                      : "_Shadow",

        TAB_COMPONENTS_SOURCE_OVERRIDE_PATH         : "_SourceOverridePath",
    }

    # dict used to convert Component type to Module type
    _MODULE_TYPE_ = {
        "LIBRARY"               :   "BASE",
        "SECURITY_CORE"         :   "SEC",
        "PEI_CORE"              :   "PEI_CORE",
        "COMBINED_PEIM_DRIVER"  :   "PEIM",
        "PIC_PEIM"              :   "PEIM",
        "RELOCATABLE_PEIM"      :   "PEIM",
        "PE32_PEIM"             :   "PEIM",
        "BS_DRIVER"             :   "DXE_DRIVER",
        "RT_DRIVER"             :   "DXE_RUNTIME_DRIVER",
        "SAL_RT_DRIVER"         :   "DXE_SAL_DRIVER",
        "DXE_SMM_DRIVER"        :   "DXE_SMM_DRIVER",
    #    "SMM_DRIVER"            :   "DXE_SMM_DRIVER",
    #    "BS_DRIVER"             :   "DXE_SMM_DRIVER",
    #    "BS_DRIVER"             :   "UEFI_DRIVER",
        "APPLICATION"           :   "UEFI_APPLICATION",
        "LOGO"                  :   "BASE",
    }

    # regular expression for converting XXX_FLAGS in [nmake] section to new type
    _NMAKE_FLAG_PATTERN_ = re.compile("(?:EBC_)?([A-Z]+)_(?:STD_|PROJ_|ARCH_)?FLAGS(?:_DLL|_ASL|_EXE)?", re.UNICODE)
    # dict used to convert old tool name used in [nmake] section to new ones
    _TOOL_CODE_ = {
        "C"         :   "CC",
        "LIB"       :   "SLINK",
        "LINK"      :   "DLINK",
    }


    ## Constructor of DscBuildData
    #
    #  Initialize object of DscBuildData
    #
    #   @param      FilePath        The path of platform description file
    #   @param      RawData         The raw data of DSC file
    #   @param      BuildDataBase   Database used to retrieve module/package information
    #   @param      Arch            The target architecture
    #   @param      Platform        The name of platform employing this module
    #   @param      Macros          Macros used for replacement in DSC file
    #
    def __init__(self, FilePath, RawData, BuildDatabase, Arch='COMMON', Target=None, Toolchain=None):
        self.MetaFile = FilePath
        self._ModuleDir = FilePath.Dir
        self._RawData = RawData
        self._Bdb = BuildDatabase
        self._Arch = Arch
        self._Target = Target
        self._Toolchain = Toolchain
        self._Platform = 'COMMON'
        self._SourceOverridePath = None
        if FilePath.Key in GlobalData.gOverrideDir:
            self._SourceOverridePath = GlobalData.gOverrideDir[FilePath.Key]
        self._Clear()

    ## XXX[key] = value
    def __setitem__(self, key, value):
        self.__dict__[self._PROPERTY_[key]] = value

    ## value = XXX[key]
    def __getitem__(self, key):
        return self.__dict__[self._PROPERTY_[key]]

    ## "in" test support
    def __contains__(self, key):
        return key in self._PROPERTY_

    ## Set all internal used members of InfBuildData to None
    def _Clear(self):
        self._HeaderComments = None
        self._TailComments = None
        self._Header_               = None
        self._AutoGenVersion        = None
        self._BaseName              = None
        self._DxsFile               = None
        self._ModuleType            = None
        self._ComponentType         = None
        self._BuildType             = None
        self._Guid                  = None
        self._Version               = None
        self._PcdIsDriver           = None
        self._BinaryModule          = None
        self._Shadow                = None
        self._MakefileName          = None
        self._CustomMakefile        = None
        self._Specification         = None
        self._LibraryClass          = None
        self._ModuleEntryPointList  = None
        self._ModuleUnloadImageList = None
        self._ConstructorList       = None
        self._DestructorList        = None
        self._Defs                  = None
        self._Binaries              = None
        self._Sources               = None
        self._LibraryClasses        = None
        self._Libraries             = None
        self._Protocols             = None
        self._ProtocolComments      = None
        self._Ppis                  = None
        self._PpiComments           = None
        self._Guids                 = None
        self._GuidsUsedByPcd        = sdict()
        self._GuidComments          = None
        self._Includes              = None
        self._Packages              = None
        self._Pcds                  = None
        self._PcdComments           = None
        self._BuildOptions          = None
        self._Depex                 = None
        self._DepexExpression       = None
        self.__Macros               = None

    ## Get current effective macros
    def _GetMacros(self):
        if self.__Macros == None:
            self.__Macros = {}
            # EDK_GLOBAL defined macros can be applied to EDK module
            if self.AutoGenVersion < 0x00010005:
                self.__Macros.update(GlobalData.gEdkGlobal)
                self.__Macros.update(GlobalData.gGlobalDefines)
        return self.__Macros

    ## Get architecture
    def _GetArch(self):
        return self._Arch

    ## Set architecture
    #
    #   Changing the default ARCH to another may affect all other information
    # because all information in a platform may be ARCH-related. That's
    # why we need to clear all internal used members, in order to cause all
    # information to be re-retrieved.
    #
    #   @param  Value   The value of ARCH
    #
    def _SetArch(self, Value):
        if self._Arch == Value:
            return
        self._Arch = Value
        self._Clear()

    ## Return the name of platform employing this module
    def _GetPlatform(self):
        return self._Platform

    ## Change the name of platform employing this module
    #
    #   Changing the default name of platform to another may affect some information
    # because they may be PLATFORM-related. That's why we need to clear all internal
    # used members, in order to cause all information to be re-retrieved.
    #
    def _SetPlatform(self, Value):
        if self._Platform == Value:
            return
        self._Platform = Value
        self._Clear()
    def _GetHeaderComments(self):
        if not self._HeaderComments:
            self._HeaderComments = []
            RecordList = self._RawData[MODEL_META_DATA_HEADER_COMMENT]
            for Record in RecordList:
                self._HeaderComments.append(Record[0])
        return self._HeaderComments
    def _GetTailComments(self):
        if not self._TailComments:
            self._TailComments = []
            RecordList = self._RawData[MODEL_META_DATA_TAIL_COMMENT]
            for Record in RecordList:
                self._TailComments.append(Record[0])
        return self._TailComments
    ## Retrieve all information in [Defines] section
    #
    #   (Retriving all [Defines] information in one-shot is just to save time.)
    #
    def _GetHeaderInfo(self):
        RecordList = self._RawData[MODEL_META_DATA_HEADER, self._Arch, self._Platform]
        for Record in RecordList:
            Name, Value = Record[1], ReplaceMacro(Record[2], self._Macros, False)
            # items defined _PROPERTY_ don't need additional processing
            if Name in self:
                self[Name] = Value
                if self._Defs == None:
                    self._Defs = sdict()
                self._Defs[Name] = Value
                self._Macros[Name] = Value
            # some special items in [Defines] section need special treatment
            elif Name in ('EFI_SPECIFICATION_VERSION', 'UEFI_SPECIFICATION_VERSION', 'EDK_RELEASE_VERSION', 'PI_SPECIFICATION_VERSION'):
                if Name in ('EFI_SPECIFICATION_VERSION', 'UEFI_SPECIFICATION_VERSION'):
                    Name = 'UEFI_SPECIFICATION_VERSION'
                if self._Specification == None:
                    self._Specification = sdict()
                self._Specification[Name] = GetHexVerValue(Value)
                if self._Specification[Name] == None:
                    EdkLogger.error("build", FORMAT_NOT_SUPPORTED,
                                    "'%s' format is not supported for %s" % (Value, Name),
                                    File=self.MetaFile, Line=Record[-1])
            elif Name == 'LIBRARY_CLASS':
                if self._LibraryClass == None:
                    self._LibraryClass = []
                ValueList = GetSplitValueList(Value)
                LibraryClass = ValueList[0]
                if len(ValueList) > 1:
                    SupModuleList = GetSplitValueList(ValueList[1], ' ')
                else:
                    SupModuleList = SUP_MODULE_LIST
                self._LibraryClass.append(LibraryClassObject(LibraryClass, SupModuleList))
            elif Name == 'ENTRY_POINT':
                if self._ModuleEntryPointList == None:
                    self._ModuleEntryPointList = []
                self._ModuleEntryPointList.append(Value)
            elif Name == 'UNLOAD_IMAGE':
                if self._ModuleUnloadImageList == None:
                    self._ModuleUnloadImageList = []
                if not Value:
                    continue
                self._ModuleUnloadImageList.append(Value)
            elif Name == 'CONSTRUCTOR':
                if self._ConstructorList == None:
                    self._ConstructorList = []
                if not Value:
                    continue
                self._ConstructorList.append(Value)
            elif Name == 'DESTRUCTOR':
                if self._DestructorList == None:
                    self._DestructorList = []
                if not Value:
                    continue
                self._DestructorList.append(Value)
            elif Name == TAB_INF_DEFINES_CUSTOM_MAKEFILE:
                TokenList = GetSplitValueList(Value)
                if self._CustomMakefile == None:
                    self._CustomMakefile = {}
                if len(TokenList) < 2:
                    self._CustomMakefile['MSFT'] = TokenList[0]
                    self._CustomMakefile['GCC'] = TokenList[0]
                else:
                    if TokenList[0] not in ['MSFT', 'GCC']:
                        EdkLogger.error("build", FORMAT_NOT_SUPPORTED,
                                        "No supported family [%s]" % TokenList[0],
                                        File=self.MetaFile, Line=Record[-1])
                    self._CustomMakefile[TokenList[0]] = TokenList[1]
            else:
                if self._Defs == None:
                    self._Defs = sdict()
                self._Defs[Name] = Value
                self._Macros[Name] = Value

        #
        # Retrieve information in sections specific to Edk.x modules
        #
        if self.AutoGenVersion >= 0x00010005:
            if not self._ModuleType:
                EdkLogger.error("build", ATTRIBUTE_NOT_AVAILABLE,
                                "MODULE_TYPE is not given", File=self.MetaFile)
            if self._ModuleType not in SUP_MODULE_LIST:
                RecordList = self._RawData[MODEL_META_DATA_HEADER, self._Arch, self._Platform]
                for Record in RecordList:
                    Name = Record[1]
                    if Name == "MODULE_TYPE":
                        LineNo = Record[6]
                        break
                EdkLogger.error("build", FORMAT_NOT_SUPPORTED,
                                "MODULE_TYPE %s is not supported for EDK II, valid values are:\n %s" % (self._ModuleType, ' '.join(l for l in SUP_MODULE_LIST)),
                                File=self.MetaFile, Line=LineNo)
            if (self._Specification == None) or (not 'PI_SPECIFICATION_VERSION' in self._Specification) or (int(self._Specification['PI_SPECIFICATION_VERSION'], 16) < 0x0001000A):
                if self._ModuleType == SUP_MODULE_SMM_CORE:
                    EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "SMM_CORE module type can't be used in the module with PI_SPECIFICATION_VERSION less than 0x0001000A", File=self.MetaFile)
            if (self._Specification == None) or (not 'PI_SPECIFICATION_VERSION' in self._Specification) or (int(self._Specification['PI_SPECIFICATION_VERSION'], 16) < 0x00010032):
                if self._ModuleType == SUP_MODULE_MM_CORE_STANDALONE:
                    EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "MM_CORE_STANDALONE module type can't be used in the module with PI_SPECIFICATION_VERSION less than 0x00010032", File=self.MetaFile)
                if self._ModuleType == SUP_MODULE_MM_STANDALONE:
                    EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "MM_STANDALONE module type can't be used in the module with PI_SPECIFICATION_VERSION less than 0x00010032", File=self.MetaFile)
            if self._Defs and 'PCI_DEVICE_ID' in self._Defs and 'PCI_VENDOR_ID' in self._Defs \
               and 'PCI_CLASS_CODE' in self._Defs and 'PCI_REVISION' in self._Defs:
                self._BuildType = 'UEFI_OPTIONROM'
                if 'PCI_COMPRESS' in self._Defs:
                    if self._Defs['PCI_COMPRESS'] not in ('TRUE', 'FALSE'):
                        EdkLogger.error("build", FORMAT_INVALID, "Expected TRUE/FALSE for PCI_COMPRESS: %s" % self.MetaFile)

            elif self._Defs and 'UEFI_HII_RESOURCE_SECTION' in self._Defs \
               and self._Defs['UEFI_HII_RESOURCE_SECTION'] == 'TRUE':
                self._BuildType = 'UEFI_HII'
            else:
                self._BuildType = self._ModuleType.upper()

            if self._DxsFile:
                File = PathClass(NormPath(self._DxsFile), self._ModuleDir, Arch=self._Arch)
                # check the file validation
                ErrorCode, ErrorInfo = File.Validate(".dxs", CaseSensitive=False)
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, ExtraData=ErrorInfo,
                                    File=self.MetaFile, Line=LineNo)
                if self.Sources == None:
                    self._Sources = []
                self._Sources.append(File)
        else:
            if not self._ComponentType:
                EdkLogger.error("build", ATTRIBUTE_NOT_AVAILABLE,
                                "COMPONENT_TYPE is not given", File=self.MetaFile)
            self._BuildType = self._ComponentType.upper()
            if self._ComponentType in self._MODULE_TYPE_:
                self._ModuleType = self._MODULE_TYPE_[self._ComponentType]
            if self._ComponentType == 'LIBRARY':
                self._LibraryClass = [LibraryClassObject(self._BaseName, SUP_MODULE_LIST)]
            # make use some [nmake] section macros
            Macros = self._Macros
            Macros["EDK_SOURCE"] = GlobalData.gEcpSource
            Macros['PROCESSOR'] = self._Arch
            RecordList = self._RawData[MODEL_META_DATA_NMAKE, self._Arch, self._Platform]
            for Name, Value, Dummy, Arch, Platform, ID, LineNo in RecordList:
                Value = ReplaceMacro(Value, Macros, True)
                if Name == "IMAGE_ENTRY_POINT":
                    if self._ModuleEntryPointList == None:
                        self._ModuleEntryPointList = []
                    self._ModuleEntryPointList.append(Value)
                elif Name == "DPX_SOURCE":
                    File = PathClass(NormPath(Value), self._ModuleDir, Arch=self._Arch)
                    # check the file validation
                    ErrorCode, ErrorInfo = File.Validate(".dxs", CaseSensitive=False)
                    if ErrorCode != 0:
                        EdkLogger.error('build', ErrorCode, ExtraData=ErrorInfo,
                                        File=self.MetaFile, Line=LineNo)
                    if self.Sources == None:
                        self._Sources = []
                    self._Sources.append(File)
                else:
                    ToolList = self._NMAKE_FLAG_PATTERN_.findall(Name)
                    if len(ToolList) == 0 or len(ToolList) != 1:
                        pass
#                        EdkLogger.warn("build", "Don't know how to do with macro [%s]" % Name,
#                                       File=self.MetaFile, Line=LineNo)
                    else:
                        if self._BuildOptions == None:
                            self._BuildOptions = sdict()

                        if ToolList[0] in self._TOOL_CODE_:
                            Tool = self._TOOL_CODE_[ToolList[0]]
                        else:
                            Tool = ToolList[0]
                        ToolChain = "*_*_*_%s_FLAGS" % Tool
                        ToolChainFamily = 'MSFT'  # Edk.x only support MSFT tool chain
                        # ignore not replaced macros in value
                        ValueList = GetSplitList(' ' + Value, '/D')
                        Dummy = ValueList[0]
                        for Index in range(1, len(ValueList)):
                            if ValueList[Index][-1] == '=' or ValueList[Index] == '':
                                continue
                            Dummy = Dummy + ' /D ' + ValueList[Index]
                        Value = Dummy.strip()
                        if (ToolChainFamily, ToolChain) not in self._BuildOptions:
                            self._BuildOptions[ToolChainFamily, ToolChain] = Value
                        else:
                            OptionString = self._BuildOptions[ToolChainFamily, ToolChain]
                            self._BuildOptions[ToolChainFamily, ToolChain] = OptionString + " " + Value
        # set _Header to non-None in order to avoid database re-querying
        self._Header_ = 'DUMMY'

    ## Retrieve file version
    def _GetInfVersion(self):
        if self._AutoGenVersion == None:
            RecordList = self._RawData[MODEL_META_DATA_HEADER, self._Arch, self._Platform]
            for Record in RecordList:
                if Record[1] == TAB_INF_DEFINES_INF_VERSION:
                    if '.' in Record[2]:
                        ValueList = Record[2].split('.')
                        Major = '%04o' % int(ValueList[0], 0)
                        Minor = '%04o' % int(ValueList[1], 0)
                        self._AutoGenVersion = int('0x' + Major + Minor, 0)
                    else:
                        self._AutoGenVersion = int(Record[2], 0)
                    break
            if self._AutoGenVersion == None:
                self._AutoGenVersion = 0x00010000
        return self._AutoGenVersion

    ## Retrieve BASE_NAME
    def _GetBaseName(self):
        if self._BaseName == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._BaseName == None:
                EdkLogger.error('build', ATTRIBUTE_NOT_AVAILABLE, "No BASE_NAME name", File=self.MetaFile)
        return self._BaseName

    ## Retrieve DxsFile
    def _GetDxsFile(self):
        if self._DxsFile == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._DxsFile == None:
                self._DxsFile = ''
        return self._DxsFile

    ## Retrieve MODULE_TYPE
    def _GetModuleType(self):
        if self._ModuleType == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._ModuleType == None:
                self._ModuleType = 'BASE'
            if self._ModuleType not in SUP_MODULE_LIST:
                self._ModuleType = "USER_DEFINED"
        return self._ModuleType

    ## Retrieve COMPONENT_TYPE
    def _GetComponentType(self):
        if self._ComponentType == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._ComponentType == None:
                self._ComponentType = 'USER_DEFINED'
        return self._ComponentType

    ## Retrieve "BUILD_TYPE"
    def _GetBuildType(self):
        if self._BuildType == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if not self._BuildType:
                self._BuildType = "BASE"
        return self._BuildType

    ## Retrieve file guid
    def _GetFileGuid(self):
        if self._Guid == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._Guid == None:
                self._Guid = '00000000-0000-0000-0000-000000000000'
        return self._Guid

    ## Retrieve module version
    def _GetVersion(self):
        if self._Version == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._Version == None:
                self._Version = '0.0'
        return self._Version

    ## Retrieve PCD_IS_DRIVER
    def _GetPcdIsDriver(self):
        if self._PcdIsDriver == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._PcdIsDriver == None:
                self._PcdIsDriver = ''
        return self._PcdIsDriver

    ## Retrieve SHADOW
    def _GetShadow(self):
        if self._Shadow == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._Shadow != None and self._Shadow.upper() == 'TRUE':
                self._Shadow = True
            else:
                self._Shadow = False
        return self._Shadow

    ## Retrieve CUSTOM_MAKEFILE
    def _GetMakefile(self):
        if self._CustomMakefile == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._CustomMakefile == None:
                self._CustomMakefile = {}
        return self._CustomMakefile

    ## Retrieve EFI_SPECIFICATION_VERSION
    def _GetSpec(self):
        if self._Specification == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._Specification == None:
                self._Specification = {}
        return self._Specification

    ## Retrieve LIBRARY_CLASS
    def _GetLibraryClass(self):
        if self._LibraryClass == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._LibraryClass == None:
                self._LibraryClass = []
        return self._LibraryClass

    ## Retrieve ENTRY_POINT
    def _GetEntryPoint(self):
        if self._ModuleEntryPointList == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._ModuleEntryPointList == None:
                self._ModuleEntryPointList = []
        return self._ModuleEntryPointList

    ## Retrieve UNLOAD_IMAGE
    def _GetUnloadImage(self):
        if self._ModuleUnloadImageList == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._ModuleUnloadImageList == None:
                self._ModuleUnloadImageList = []
        return self._ModuleUnloadImageList

    ## Retrieve CONSTRUCTOR
    def _GetConstructor(self):
        if self._ConstructorList == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._ConstructorList == None:
                self._ConstructorList = []
        return self._ConstructorList

    ## Retrieve DESTRUCTOR
    def _GetDestructor(self):
        if self._DestructorList == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._DestructorList == None:
                self._DestructorList = []
        return self._DestructorList

    ## Retrieve definies other than above ones
    def _GetDefines(self):
        if self._Defs == None:
            if self._Header_ == None:
                self._GetHeaderInfo()
            if self._Defs == None:
                self._Defs = sdict()
        return self._Defs

    ## Retrieve binary files
    def _GetBinaries(self):
        if self._Binaries == None:
            self._Binaries = []
            RecordList = self._RawData[MODEL_EFI_BINARY_FILE, self._Arch, self._Platform]
            Macros = self._Macros
            Macros["EDK_SOURCE"] = GlobalData.gEcpSource
            Macros['PROCESSOR'] = self._Arch
            for Record in RecordList:
                FileType = Record[0]
                LineNo = Record[-1]
                Target = 'COMMON'
                FeatureFlag = []
                if Record[2]:
                    TokenList = GetSplitValueList(Record[2], TAB_VALUE_SPLIT)
                    if TokenList:
                        Target = TokenList[0]
                    if len(TokenList) > 1:
                        FeatureFlag = Record[1:]

                File = PathClass(NormPath(Record[1], Macros), self._ModuleDir, '', FileType, True, self._Arch, '', Target)
                # check the file validation
                ErrorCode, ErrorInfo = File.Validate()
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, ExtraData=ErrorInfo, File=self.MetaFile, Line=LineNo)
                self._Binaries.append(File)
        return self._Binaries

    ## Retrieve binary files with error check.
    def _GetBinaryFiles(self):
        Binaries = self._GetBinaries()
        if GlobalData.gIgnoreSource and Binaries == []:
            ErrorInfo = "The INF file does not contain any Binaries to use in creating the image\n"
            EdkLogger.error('build', RESOURCE_NOT_AVAILABLE, ExtraData=ErrorInfo, File=self.MetaFile)

        return Binaries
    ## Check whether it exists the binaries with current ARCH in AsBuild INF
    def _IsSupportedArch(self):
        if self._GetBinaries() and not self._GetSourceFiles():
            return True
        else:
            return False
    ## Retrieve source files
    def _GetSourceFiles(self):
        # Ignore all source files in a binary build mode
        if GlobalData.gIgnoreSource:
            self._Sources = []
            return self._Sources

        if self._Sources == None:
            self._Sources = []
            RecordList = self._RawData[MODEL_EFI_SOURCE_FILE, self._Arch, self._Platform]
            Macros = self._Macros
            for Record in RecordList:
                LineNo = Record[-1]
                ToolChainFamily = Record[1]
                TagName = Record[2]
                ToolCode = Record[3]
                FeatureFlag = Record[4]
                if self.AutoGenVersion < 0x00010005:
                    Macros["EDK_SOURCE"] = GlobalData.gEcpSource
                    Macros['PROCESSOR'] = self._Arch
                    SourceFile = NormPath(Record[0], Macros)
                    if SourceFile[0] == os.path.sep:
                        SourceFile = mws.join(GlobalData.gWorkspace, SourceFile[1:])
                    # old module source files (Edk)
                    File = PathClass(SourceFile, self._ModuleDir, self._SourceOverridePath,
                                     '', False, self._Arch, ToolChainFamily, '', TagName, ToolCode)
                    # check the file validation
                    ErrorCode, ErrorInfo = File.Validate(CaseSensitive=False)
                    if ErrorCode != 0:
                        if File.Ext.lower() == '.h':
                            EdkLogger.warn('build', 'Include file not found', ExtraData=ErrorInfo,
                                           File=self.MetaFile, Line=LineNo)
                            continue
                        else:
                            EdkLogger.error('build', ErrorCode, ExtraData=File, File=self.MetaFile, Line=LineNo)
                else:
                    File = PathClass(NormPath(Record[0], Macros), self._ModuleDir, '',
                                     '', False, self._Arch, ToolChainFamily, '', TagName, ToolCode)
                    # check the file validation
                    ErrorCode, ErrorInfo = File.Validate()
                    if ErrorCode != 0:
                        EdkLogger.error('build', ErrorCode, ExtraData=ErrorInfo, File=self.MetaFile, Line=LineNo)

                self._Sources.append(File)
        return self._Sources

    ## Retrieve library classes employed by this module
    def _GetLibraryClassUses(self):
        if self._LibraryClasses == None:
            self._LibraryClasses = sdict()
            RecordList = self._RawData[MODEL_EFI_LIBRARY_CLASS, self._Arch, self._Platform]
            for Record in RecordList:
                Lib = Record[0]
                Instance = Record[1]
                if Instance:
                    Instance = NormPath(Instance, self._Macros)
                self._LibraryClasses[Lib] = Instance
        return self._LibraryClasses

    ## Retrieve library names (for Edk.x style of modules)
    def _GetLibraryNames(self):
        if self._Libraries == None:
            self._Libraries = []
            RecordList = self._RawData[MODEL_EFI_LIBRARY_INSTANCE, self._Arch, self._Platform]
            for Record in RecordList:
                LibraryName = ReplaceMacro(Record[0], self._Macros, False)
                # in case of name with '.lib' extension, which is unusual in Edk.x inf
                LibraryName = os.path.splitext(LibraryName)[0]
                if LibraryName not in self._Libraries:
                    self._Libraries.append(LibraryName)
        return self._Libraries

    def _GetProtocolComments(self):
        self._GetProtocols()
        return self._ProtocolComments
    ## Retrieve protocols consumed/produced by this module
    def _GetProtocols(self):
        if self._Protocols == None:
            self._Protocols = sdict()
            self._ProtocolComments = sdict()
            RecordList = self._RawData[MODEL_EFI_PROTOCOL, self._Arch, self._Platform]
            for Record in RecordList:
                CName = Record[0]
                Value = ProtocolValue(CName, self.Packages, self.MetaFile.Path)
                if Value == None:
                    PackageList = "\n\t".join([str(P) for P in self.Packages])
                    EdkLogger.error('build', RESOURCE_NOT_AVAILABLE,
                                    "Value of Protocol [%s] is not found under [Protocols] section in" % CName,
                                    ExtraData=PackageList, File=self.MetaFile, Line=Record[-1])
                self._Protocols[CName] = Value
                CommentRecords = self._RawData[MODEL_META_DATA_COMMENT, self._Arch, self._Platform, Record[5]]
                Comments = []
                for CmtRec in CommentRecords:
                    Comments.append(CmtRec[0])
                self._ProtocolComments[CName] = Comments
        return self._Protocols

    def _GetPpiComments(self):
        self._GetPpis()
        return self._PpiComments
    ## Retrieve PPIs consumed/produced by this module
    def _GetPpis(self):
        if self._Ppis == None:
            self._Ppis = sdict()
            self._PpiComments = sdict()
            RecordList = self._RawData[MODEL_EFI_PPI, self._Arch, self._Platform]
            for Record in RecordList:
                CName = Record[0]
                Value = PpiValue(CName, self.Packages, self.MetaFile.Path)
                if Value == None:
                    PackageList = "\n\t".join([str(P) for P in self.Packages])
                    EdkLogger.error('build', RESOURCE_NOT_AVAILABLE,
                                    "Value of PPI [%s] is not found under [Ppis] section in " % CName,
                                    ExtraData=PackageList, File=self.MetaFile, Line=Record[-1])
                self._Ppis[CName] = Value
                CommentRecords = self._RawData[MODEL_META_DATA_COMMENT, self._Arch, self._Platform, Record[5]]
                Comments = []
                for CmtRec in CommentRecords:
                    Comments.append(CmtRec[0])
                self._PpiComments[CName] = Comments
        return self._Ppis

    def _GetGuidComments(self):
        self._GetGuids()
        return self._GuidComments
    ## Retrieve GUIDs consumed/produced by this module
    def _GetGuids(self):
        if self._Guids == None:
            self._Guids = sdict()
            self._GuidComments = sdict()
            RecordList = self._RawData[MODEL_EFI_GUID, self._Arch, self._Platform]
            for Record in RecordList:
                CName = Record[0]
                Value = GuidValue(CName, self.Packages, self.MetaFile.Path)
                if Value == None:
                    PackageList = "\n\t".join([str(P) for P in self.Packages])
                    EdkLogger.error('build', RESOURCE_NOT_AVAILABLE,
                                    "Value of Guid [%s] is not found under [Guids] section in" % CName,
                                    ExtraData=PackageList, File=self.MetaFile, Line=Record[-1])
                self._Guids[CName] = Value
                CommentRecords = self._RawData[MODEL_META_DATA_COMMENT, self._Arch, self._Platform, Record[5]]
                Comments = []
                for CmtRec in CommentRecords:
                    Comments.append(CmtRec[0])
                self._GuidComments[CName] = Comments
        return self._Guids

    ## Retrieve include paths necessary for this module (for Edk.x style of modules)
    def _GetIncludes(self):
        if self._Includes == None:
            self._Includes = []
            if self._SourceOverridePath:
                self._Includes.append(self._SourceOverridePath)

            Macros = self._Macros
            if 'PROCESSOR' in GlobalData.gEdkGlobal.keys():
                Macros['PROCESSOR'] = GlobalData.gEdkGlobal['PROCESSOR']
            else:
                Macros['PROCESSOR'] = self._Arch
            RecordList = self._RawData[MODEL_EFI_INCLUDE, self._Arch, self._Platform]
            for Record in RecordList:
                if Record[0].find('EDK_SOURCE') > -1:
                    Macros['EDK_SOURCE'] = GlobalData.gEcpSource
                    File = NormPath(Record[0], self._Macros)
                    if File[0] == '.':
                        File = os.path.join(self._ModuleDir, File)
                    else:
                        File = os.path.join(GlobalData.gWorkspace, File)
                    File = RealPath(os.path.normpath(File))
                    if File:
                        self._Includes.append(File)

                    # TRICK: let compiler to choose correct header file
                    Macros['EDK_SOURCE'] = GlobalData.gEdkSource
                    File = NormPath(Record[0], self._Macros)
                    if File[0] == '.':
                        File = os.path.join(self._ModuleDir, File)
                    else:
                        File = os.path.join(GlobalData.gWorkspace, File)
                    File = RealPath(os.path.normpath(File))
                    if File:
                        self._Includes.append(File)
                else:
                    File = NormPath(Record[0], Macros)
                    if File[0] == '.':
                        File = os.path.join(self._ModuleDir, File)
                    else:
                        File = mws.join(GlobalData.gWorkspace, File)
                    File = RealPath(os.path.normpath(File))
                    if File:
                        self._Includes.append(File)
                    if not File and Record[0].find('EFI_SOURCE') > -1:
                        # tricky to regard WorkSpace as EFI_SOURCE
                        Macros['EFI_SOURCE'] = GlobalData.gWorkspace
                        File = NormPath(Record[0], Macros)
                        if File[0] == '.':
                            File = os.path.join(self._ModuleDir, File)
                        else:
                            File = os.path.join(GlobalData.gWorkspace, File)
                        File = RealPath(os.path.normpath(File))
                        if File:
                            self._Includes.append(File)
        return self._Includes

    ## Retrieve packages this module depends on
    def _GetPackages(self):
        if self._Packages == None:
            self._Packages = []
            RecordList = self._RawData[MODEL_META_DATA_PACKAGE, self._Arch, self._Platform]
            Macros = self._Macros
            Macros['EDK_SOURCE'] = GlobalData.gEcpSource
            for Record in RecordList:
                File = PathClass(NormPath(Record[0], Macros), GlobalData.gWorkspace, Arch=self._Arch)
                LineNo = Record[-1]
                # check the file validation
                ErrorCode, ErrorInfo = File.Validate('.dec')
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, ExtraData=ErrorInfo, File=self.MetaFile, Line=LineNo)
                # parse this package now. we need it to get protocol/ppi/guid value
                Package = self._Bdb[File, self._Arch, self._Target, self._Toolchain]
                self._Packages.append(Package)
        return self._Packages

    ## Retrieve PCD comments
    def _GetPcdComments(self):
        self._GetPcds()
        return self._PcdComments
    ## Retrieve PCDs used in this module
    def _GetPcds(self):
        if self._Pcds == None:
            self._Pcds = sdict()
            self._PcdComments = sdict()
            self._Pcds.update(self._GetPcd(MODEL_PCD_FIXED_AT_BUILD))
            self._Pcds.update(self._GetPcd(MODEL_PCD_PATCHABLE_IN_MODULE))
            self._Pcds.update(self._GetPcd(MODEL_PCD_FEATURE_FLAG))
            self._Pcds.update(self._GetPcd(MODEL_PCD_DYNAMIC))
            self._Pcds.update(self._GetPcd(MODEL_PCD_DYNAMIC_EX))
        return self._Pcds

    ## Retrieve build options specific to this module
    def _GetBuildOptions(self):
        if self._BuildOptions == None:
            self._BuildOptions = sdict()
            RecordList = self._RawData[MODEL_META_DATA_BUILD_OPTION, self._Arch, self._Platform]
            for Record in RecordList:
                ToolChainFamily = Record[0]
                ToolChain = Record[1]
                Option = Record[2]
                if (ToolChainFamily, ToolChain) not in self._BuildOptions or Option.startswith('='):
                    self._BuildOptions[ToolChainFamily, ToolChain] = Option
                else:
                    # concatenate the option string if they're for the same tool
                    OptionString = self._BuildOptions[ToolChainFamily, ToolChain]
                    self._BuildOptions[ToolChainFamily, ToolChain] = OptionString + " " + Option
        return self._BuildOptions

    ## Retrieve dependency expression
    def _GetDepex(self):
        if self._Depex == None:
            self._Depex = tdict(False, 2)
            RecordList = self._RawData[MODEL_EFI_DEPEX, self._Arch]

            # If the module has only Binaries and no Sources, then ignore [Depex]
            if self.Sources == None or self.Sources == []:
                if self.Binaries != None and self.Binaries != []:
                    return self._Depex

            # PEIM and DXE drivers must have a valid [Depex] section
            if len(self.LibraryClass) == 0 and len(RecordList) == 0:
                if self.ModuleType == 'DXE_DRIVER' or self.ModuleType == 'PEIM' or self.ModuleType == 'DXE_SMM_DRIVER' or \
                    self.ModuleType == 'DXE_SAL_DRIVER' or self.ModuleType == 'DXE_RUNTIME_DRIVER':
                    EdkLogger.error('build', RESOURCE_NOT_AVAILABLE, "No [Depex] section or no valid expression in [Depex] section for [%s] module" \
                                    % self.ModuleType, File=self.MetaFile)

            if len(RecordList) != 0 and self.ModuleType == 'USER_DEFINED':
                for Record in RecordList:
                    if Record[4] not in ['PEIM', 'DXE_DRIVER', 'DXE_SMM_DRIVER']:
                        EdkLogger.error('build', FORMAT_INVALID,
                                        "'%s' module must specify the type of [Depex] section" % self.ModuleType,
                                        File=self.MetaFile)

            Depex = sdict()
            for Record in RecordList:
                DepexStr = ReplaceMacro(Record[0], self._Macros, False)
                Arch = Record[3]
                ModuleType = Record[4]
                TokenList = DepexStr.split()
                if (Arch, ModuleType) not in Depex:
                    Depex[Arch, ModuleType] = []
                DepexList = Depex[Arch, ModuleType]
                for Token in TokenList:
                    if Token in DEPEX_SUPPORTED_OPCODE:
                        DepexList.append(Token)
                    elif Token.endswith(".inf"):  # module file name
                        ModuleFile = os.path.normpath(Token)
                        Module = self.BuildDatabase[ModuleFile]
                        if Module == None:
                            EdkLogger.error('build', RESOURCE_NOT_AVAILABLE, "Module is not found in active platform",
                                            ExtraData=Token, File=self.MetaFile, Line=Record[-1])
                        DepexList.append(Module.Guid)
                    else:
                        # get the GUID value now
                        Value = ProtocolValue(Token, self.Packages, self.MetaFile.Path)
                        if Value == None:
                            Value = PpiValue(Token, self.Packages, self.MetaFile.Path)
                            if Value == None:
                                Value = GuidValue(Token, self.Packages, self.MetaFile.Path)
                        if Value == None:
                            PackageList = "\n\t".join([str(P) for P in self.Packages])
                            EdkLogger.error('build', RESOURCE_NOT_AVAILABLE,
                                            "Value of [%s] is not found in" % Token,
                                            ExtraData=PackageList, File=self.MetaFile, Line=Record[-1])
                        DepexList.append(Value)
            for Arch, ModuleType in Depex:
                self._Depex[Arch, ModuleType] = Depex[Arch, ModuleType]
        return self._Depex

    ## Retrieve depedency expression
    def _GetDepexExpression(self):
        if self._DepexExpression == None:
            self._DepexExpression = tdict(False, 2)
            RecordList = self._RawData[MODEL_EFI_DEPEX, self._Arch]
            DepexExpression = sdict()
            for Record in RecordList:
                DepexStr = ReplaceMacro(Record[0], self._Macros, False)
                Arch = Record[3]
                ModuleType = Record[4]
                TokenList = DepexStr.split()
                if (Arch, ModuleType) not in DepexExpression:
                    DepexExpression[Arch, ModuleType] = ''
                for Token in TokenList:
                    DepexExpression[Arch, ModuleType] = DepexExpression[Arch, ModuleType] + Token.strip() + ' '
            for Arch, ModuleType in DepexExpression:
                self._DepexExpression[Arch, ModuleType] = DepexExpression[Arch, ModuleType]
        return self._DepexExpression

    def GetGuidsUsedByPcd(self):
        return self._GuidsUsedByPcd
    ## Retrieve PCD for given type
    def _GetPcd(self, Type):
        Pcds = sdict()
        PcdDict = tdict(True, 4)
        PcdList = []
        RecordList = self._RawData[Type, self._Arch, self._Platform]
        for TokenSpaceGuid, PcdCName, Setting, Arch, Platform, Id, LineNo in RecordList:
            PcdDict[Arch, Platform, PcdCName, TokenSpaceGuid] = (Setting, LineNo)
            PcdList.append((PcdCName, TokenSpaceGuid))
            # get the guid value
            if TokenSpaceGuid not in self.Guids:
                Value = GuidValue(TokenSpaceGuid, self.Packages, self.MetaFile.Path)
                if Value == None:
                    PackageList = "\n\t".join([str(P) for P in self.Packages])
                    EdkLogger.error('build', RESOURCE_NOT_AVAILABLE,
                                    "Value of Guid [%s] is not found under [Guids] section in" % TokenSpaceGuid,
                                    ExtraData=PackageList, File=self.MetaFile, Line=LineNo)
                self.Guids[TokenSpaceGuid] = Value
                self._GuidsUsedByPcd[TokenSpaceGuid] = Value
            CommentRecords = self._RawData[MODEL_META_DATA_COMMENT, self._Arch, self._Platform, Id]
            Comments = []
            for CmtRec in CommentRecords:
                Comments.append(CmtRec[0])
            self._PcdComments[TokenSpaceGuid, PcdCName] = Comments

        # resolve PCD type, value, datum info, etc. by getting its definition from package
        _GuidDict = copy.deepcopy(self.Guids)
        for PcdCName, TokenSpaceGuid in PcdList:
            PcdRealName = PcdCName
            Setting, LineNo = PcdDict[self._Arch, self.Platform, PcdCName, TokenSpaceGuid]
            if Setting == None:
                continue
            ValueList = AnalyzePcdData(Setting)
            DefaultValue = ValueList[0]
            Pcd = PcdClassObject(
                    PcdCName,
                    TokenSpaceGuid,
                    '',
                    '',
                    DefaultValue,
                    '',
                    '',
                    {},
                    False,
                    self.Guids[TokenSpaceGuid]
                    )
            if Type == MODEL_PCD_PATCHABLE_IN_MODULE and ValueList[1]:
                # Patch PCD: TokenSpace.PcdCName|Value|Offset
                Pcd.Offset = ValueList[1]

            if (PcdRealName, TokenSpaceGuid) in GlobalData.MixedPcd:
                for Package in self.Packages:
                    for key in Package.Pcds:
                        if (Package.Pcds[key].TokenCName, Package.Pcds[key].TokenSpaceGuidCName) == (PcdRealName, TokenSpaceGuid):
                            for item in GlobalData.MixedPcd[(PcdRealName, TokenSpaceGuid)]:
                                Pcd_Type = item[0].split('_')[-1]
                                if Pcd_Type == Package.Pcds[key].Type:
                                    Value = Package.Pcds[key]
                                    Value.TokenCName = Package.Pcds[key].TokenCName + '_' + Pcd_Type
                                    if len(key) == 2:
                                        newkey = (Value.TokenCName, key[1])
                                    elif len(key) == 3:
                                        newkey = (Value.TokenCName, key[1], key[2])
                                    del Package.Pcds[key]
                                    Package.Pcds[newkey] = Value
                                    break
                                else:
                                    pass
                        else:
                            pass

            # get necessary info from package declaring this PCD
            for Package in self.Packages:
                #
                # 'dynamic' in INF means its type is determined by platform;
                # if platform doesn't give its type, use 'lowest' one in the
                # following order, if any
                #
                #   "FixedAtBuild", "PatchableInModule", "FeatureFlag", "Dynamic", "DynamicEx"
                #
                _GuidDict.update(Package.Guids)
                PcdType = self._PCD_TYPE_STRING_[Type]
                if Type == MODEL_PCD_DYNAMIC:
                    Pcd.Pending = True
                    for T in ["FixedAtBuild", "PatchableInModule", "FeatureFlag", "Dynamic", "DynamicEx"]:
                        if (PcdRealName, TokenSpaceGuid) in GlobalData.MixedPcd:
                            for item in GlobalData.MixedPcd[(PcdRealName, TokenSpaceGuid)]:
                                if str(item[0]).endswith(T) and (item[0], item[1], T) in Package.Pcds:
                                    PcdType = T
                                    PcdCName = item[0]
                                    break
                                else:
                                    pass
                            break
                        else:
                            if (PcdRealName, TokenSpaceGuid, T) in Package.Pcds:
                                PcdType = T
                                break

                else:
                    Pcd.Pending = False
                    if (PcdRealName, TokenSpaceGuid) in GlobalData.MixedPcd:
                        for item in GlobalData.MixedPcd[(PcdRealName, TokenSpaceGuid)]:
                            Pcd_Type = item[0].split('_')[-1]
                            if Pcd_Type == PcdType:
                                PcdCName = item[0]
                                break
                            else:
                                pass
                    else:
                        pass

                if (PcdCName, TokenSpaceGuid, PcdType) in Package.Pcds:
                    PcdInPackage = Package.Pcds[PcdCName, TokenSpaceGuid, PcdType]
                    Pcd.Type = PcdType
                    Pcd.TokenValue = PcdInPackage.TokenValue

                    #
                    # Check whether the token value exist or not.
                    #
                    if Pcd.TokenValue == None or Pcd.TokenValue == "":
                        EdkLogger.error(
                                'build',
                                FORMAT_INVALID,
                                "No TokenValue for PCD [%s.%s] in [%s]!" % (TokenSpaceGuid, PcdRealName, str(Package)),
                                File=self.MetaFile, Line=LineNo,
                                ExtraData=None
                                )
                    #
                    # Check hexadecimal token value length and format.
                    #
                    ReIsValidPcdTokenValue = re.compile(r"^[0][x|X][0]*[0-9a-fA-F]{1,8}$", re.DOTALL)
                    if Pcd.TokenValue.startswith("0x") or Pcd.TokenValue.startswith("0X"):
                        if ReIsValidPcdTokenValue.match(Pcd.TokenValue) == None:
                            EdkLogger.error(
                                    'build',
                                    FORMAT_INVALID,
                                    "The format of TokenValue [%s] of PCD [%s.%s] in [%s] is invalid:" % (Pcd.TokenValue, TokenSpaceGuid, PcdRealName, str(Package)),
                                    File=self.MetaFile, Line=LineNo,
                                    ExtraData=None
                                    )

                    #
                    # Check decimal token value length and format.
                    #
                    else:
                        try:
                            TokenValueInt = int (Pcd.TokenValue, 10)
                            if (TokenValueInt < 0 or TokenValueInt > 4294967295):
                                EdkLogger.error(
                                            'build',
                                            FORMAT_INVALID,
                                            "The format of TokenValue [%s] of PCD [%s.%s] in [%s] is invalid, as a decimal it should between: 0 - 4294967295!" % (Pcd.TokenValue, TokenSpaceGuid, PcdRealName, str(Package)),
                                            File=self.MetaFile, Line=LineNo,
                                            ExtraData=None
                                            )
                        except:
                            EdkLogger.error(
                                        'build',
                                        FORMAT_INVALID,
                                        "The format of TokenValue [%s] of PCD [%s.%s] in [%s] is invalid, it should be hexadecimal or decimal!" % (Pcd.TokenValue, TokenSpaceGuid, PcdRealName, str(Package)),
                                        File=self.MetaFile, Line=LineNo,
                                        ExtraData=None
                                        )

                    Pcd.DatumType = PcdInPackage.DatumType
                    Pcd.MaxDatumSize = PcdInPackage.MaxDatumSize
                    Pcd.InfDefaultValue = Pcd.DefaultValue
                    if Pcd.DefaultValue in [None, '']:
                        Pcd.DefaultValue = PcdInPackage.DefaultValue
                    else:
                        try:
                            Pcd.DefaultValue = ValueExpressionEx(Pcd.DefaultValue, Pcd.DatumType, _GuidDict)(True)
                        except BadExpression, Value:
                            EdkLogger.error('Parser', FORMAT_INVALID, 'PCD [%s.%s] Value "%s", %s' %(TokenSpaceGuid, PcdRealName, Pcd.DefaultValue, Value),
                                            File=self.MetaFile, Line=LineNo)
                    break
            else:
                EdkLogger.error(
                            'build',
                            FORMAT_INVALID,
                            "PCD [%s.%s] in [%s] is not found in dependent packages:" % (TokenSpaceGuid, PcdRealName, self.MetaFile),
                            File=self.MetaFile, Line=LineNo,
                            ExtraData="\t%s" % '\n\t'.join([str(P) for P in self.Packages])
                            )
            Pcds[PcdCName, TokenSpaceGuid] = Pcd

        return Pcds

    ## check whether current module is binary module
    def _IsBinaryModule(self):
        if self.Binaries and not self.Sources:
            return True
        elif GlobalData.gIgnoreSource:
            return True
        else:
            return False

    _Macros = property(_GetMacros)
    Arch = property(_GetArch, _SetArch)
    Platform = property(_GetPlatform, _SetPlatform)

    HeaderComments = property(_GetHeaderComments)
    TailComments = property(_GetTailComments)
    AutoGenVersion          = property(_GetInfVersion)
    BaseName                = property(_GetBaseName)
    ModuleType              = property(_GetModuleType)
    ComponentType           = property(_GetComponentType)
    BuildType               = property(_GetBuildType)
    Guid                    = property(_GetFileGuid)
    Version                 = property(_GetVersion)
    PcdIsDriver             = property(_GetPcdIsDriver)
    Shadow                  = property(_GetShadow)
    CustomMakefile          = property(_GetMakefile)
    Specification           = property(_GetSpec)
    LibraryClass            = property(_GetLibraryClass)
    ModuleEntryPointList    = property(_GetEntryPoint)
    ModuleUnloadImageList   = property(_GetUnloadImage)
    ConstructorList         = property(_GetConstructor)
    DestructorList          = property(_GetDestructor)
    Defines                 = property(_GetDefines)
    DxsFile                 = property(_GetDxsFile)

    Binaries                = property(_GetBinaryFiles)
    Sources                 = property(_GetSourceFiles)
    LibraryClasses          = property(_GetLibraryClassUses)
    Libraries               = property(_GetLibraryNames)
    Protocols               = property(_GetProtocols)
    ProtocolComments        = property(_GetProtocolComments)
    Ppis                    = property(_GetPpis)
    PpiComments             = property(_GetPpiComments)
    Guids                   = property(_GetGuids)
    GuidComments            = property(_GetGuidComments)
    Includes                = property(_GetIncludes)
    Packages                = property(_GetPackages)
    Pcds                    = property(_GetPcds)
    PcdComments             = property(_GetPcdComments)
    BuildOptions            = property(_GetBuildOptions)
    Depex                   = property(_GetDepex)
    DepexExpression         = property(_GetDepexExpression)
    IsBinaryModule          = property(_IsBinaryModule)
    IsSupportedArch         = property(_IsSupportedArch)
