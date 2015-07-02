## @file
# This file is used to create a database used by build tool
#
# Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import sqlite3
import Common.LongFilePathOs as os
import pickle
import uuid

import Common.EdkLogger as EdkLogger
import Common.GlobalData as GlobalData

from Common.String import *
from Common.DataType import *
from Common.Misc import *
from types import *

from CommonDataClass.CommonClass import SkuInfoClass

from MetaDataTable import *
from MetaFileTable import *
from MetaFileParser import *
from BuildClassObject import *
from WorkspaceCommon import GetDeclaredPcd
from Common.Misc import AnalyzeDscPcd
from Common.Misc import ProcessDuplicatedInf
import re
from Common.Parsing import IsValidWord
from Common.VariableAttributes import VariableAttributes
import Common.GlobalData as GlobalData

## Platform build information from DSC file
#
#  This class is used to retrieve information stored in database and convert them
# into PlatformBuildClassObject form for easier use for AutoGen.
#
class DscBuildData(PlatformBuildClassObject):
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

    # dict used to convert part of [Defines] to members of DscBuildData directly
    _PROPERTY_ = {
        #
        # Required Fields
        #
        TAB_DSC_DEFINES_PLATFORM_NAME           :   "_PlatformName",
        TAB_DSC_DEFINES_PLATFORM_GUID           :   "_Guid",
        TAB_DSC_DEFINES_PLATFORM_VERSION        :   "_Version",
        TAB_DSC_DEFINES_DSC_SPECIFICATION       :   "_DscSpecification",
        #TAB_DSC_DEFINES_OUTPUT_DIRECTORY        :   "_OutputDirectory",
        #TAB_DSC_DEFINES_SUPPORTED_ARCHITECTURES :   "_SupArchList",
        #TAB_DSC_DEFINES_BUILD_TARGETS           :   "_BuildTargets",
        TAB_DSC_DEFINES_SKUID_IDENTIFIER        :   "_SkuName",
        #TAB_DSC_DEFINES_FLASH_DEFINITION        :   "_FlashDefinition",
        TAB_DSC_DEFINES_BUILD_NUMBER            :   "_BuildNumber",
        TAB_DSC_DEFINES_MAKEFILE_NAME           :   "_MakefileName",
        TAB_DSC_DEFINES_BS_BASE_ADDRESS         :   "_BsBaseAddress",
        TAB_DSC_DEFINES_RT_BASE_ADDRESS         :   "_RtBaseAddress",
        #TAB_DSC_DEFINES_RFC_LANGUAGES           :   "_RFCLanguages",
        #TAB_DSC_DEFINES_ISO_LANGUAGES           :   "_ISOLanguages",
    }

    # used to compose dummy library class name for those forced library instances
    _NullLibraryNumber = 0

    ## Constructor of DscBuildData
    #
    #  Initialize object of DscBuildData
    #
    #   @param      FilePath        The path of platform description file
    #   @param      RawData         The raw data of DSC file
    #   @param      BuildDataBase   Database used to retrieve module/package information
    #   @param      Arch            The target architecture
    #   @param      Platform        (not used for DscBuildData)
    #   @param      Macros          Macros used for replacement in DSC file
    #
    def __init__(self, FilePath, RawData, BuildDataBase, Arch='COMMON', Target=None, Toolchain=None):
        self.MetaFile = FilePath
        self._RawData = RawData
        self._Bdb = BuildDataBase
        self._Arch = Arch
        self._Target = Target
        self._Toolchain = Toolchain
        self._Clear()
        self._HandleOverridePath()

    ## XXX[key] = value
    def __setitem__(self, key, value):
        self.__dict__[self._PROPERTY_[key]] = value

    ## value = XXX[key]
    def __getitem__(self, key):
        return self.__dict__[self._PROPERTY_[key]]

    ## "in" test support
    def __contains__(self, key):
        return key in self._PROPERTY_

    ## Set all internal used members of DscBuildData to None
    def _Clear(self):
        self._Header            = None
        self._PlatformName      = None
        self._Guid              = None
        self._Version           = None
        self._DscSpecification  = None
        self._OutputDirectory   = None
        self._SupArchList       = None
        self._BuildTargets      = None
        self._SkuName           = None
        self._SkuIdentifier     = None
        self._AvilableSkuIds = None
        self._PcdInfoFlag       = None
        self._VarCheckFlag = None
        self._FlashDefinition   = None
        self._BuildNumber       = None
        self._MakefileName      = None
        self._BsBaseAddress     = None
        self._RtBaseAddress     = None
        self._SkuIds            = None
        self._Modules           = None
        self._LibraryInstances  = None
        self._LibraryClasses    = None
        self._Pcds              = None
        self._DecPcds           = None
        self._BuildOptions      = None
        self._ModuleTypeOptions = None
        self._LoadFixAddress    = None
        self._RFCLanguages      = None
        self._ISOLanguages      = None
        self._VpdToolGuid       = None
        self.__Macros            = None


    ## handle Override Path of Module
    def _HandleOverridePath(self):
        RecordList = self._RawData[MODEL_META_DATA_COMPONENT, self._Arch]
        Macros = self._Macros
        Macros["EDK_SOURCE"] = GlobalData.gEcpSource
        for Record in RecordList:
            ModuleId = Record[5]
            LineNo = Record[6]
            ModuleFile = PathClass(NormPath(Record[0]), GlobalData.gWorkspace, Arch=self._Arch)
            RecordList = self._RawData[MODEL_META_DATA_COMPONENT_SOURCE_OVERRIDE_PATH, self._Arch, None, ModuleId]
            if RecordList != []:
                SourceOverridePath = os.path.join(GlobalData.gWorkspace, NormPath(RecordList[0][0]))

                # Check if the source override path exists
                if not os.path.isdir(SourceOverridePath):
                    EdkLogger.error('build', FILE_NOT_FOUND, Message='Source override path does not exist:', File=self.MetaFile, ExtraData=SourceOverridePath, Line=LineNo)

                #Add to GlobalData Variables
                GlobalData.gOverrideDir[ModuleFile.Key] = SourceOverridePath

    ## Get current effective macros
    def _GetMacros(self):
        if self.__Macros == None:
            self.__Macros = {}
            self.__Macros.update(GlobalData.gPlatformDefines)
            self.__Macros.update(GlobalData.gGlobalDefines)
            self.__Macros.update(GlobalData.gCommandLineDefines)
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

    ## Retrieve all information in [Defines] section
    #
    #   (Retriving all [Defines] information in one-shot is just to save time.)
    #
    def _GetHeaderInfo(self):
        RecordList = self._RawData[MODEL_META_DATA_HEADER, self._Arch]
        for Record in RecordList:
            Name = Record[1]
            # items defined _PROPERTY_ don't need additional processing
            
            # some special items in [Defines] section need special treatment
            if Name == TAB_DSC_DEFINES_OUTPUT_DIRECTORY:
                self._OutputDirectory = NormPath(Record[2], self._Macros)
                if ' ' in self._OutputDirectory:
                    EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "No space is allowed in OUTPUT_DIRECTORY",
                                    File=self.MetaFile, Line=Record[-1],
                                    ExtraData=self._OutputDirectory)
            elif Name == TAB_DSC_DEFINES_FLASH_DEFINITION:
                self._FlashDefinition = PathClass(NormPath(Record[2], self._Macros), GlobalData.gWorkspace)
                ErrorCode, ErrorInfo = self._FlashDefinition.Validate('.fdf')
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, File=self.MetaFile, Line=Record[-1],
                                    ExtraData=ErrorInfo)
            elif Name == TAB_DSC_DEFINES_SUPPORTED_ARCHITECTURES:
                self._SupArchList = GetSplitValueList(Record[2], TAB_VALUE_SPLIT)
            elif Name == TAB_DSC_DEFINES_BUILD_TARGETS:
                self._BuildTargets = GetSplitValueList(Record[2])
            elif Name == TAB_DSC_DEFINES_SKUID_IDENTIFIER:
                if self._SkuName == None:
                    self._SkuName = Record[2]
                self._SkuIdentifier = Record[2]
                self._AvilableSkuIds = Record[2]
            elif Name == TAB_DSC_DEFINES_PCD_INFO_GENERATION:
                self._PcdInfoFlag = Record[2]
            elif Name == TAB_DSC_DEFINES_PCD_VAR_CHECK_GENERATION:
                self._VarCheckFlag = Record[2]
            elif Name == TAB_FIX_LOAD_TOP_MEMORY_ADDRESS:
                try:
                    self._LoadFixAddress = int (Record[2], 0)
                except:
                    EdkLogger.error("build", PARAMETER_INVALID, "FIX_LOAD_TOP_MEMORY_ADDRESS %s is not valid dec or hex string" % (Record[2]))
            elif Name == TAB_DSC_DEFINES_RFC_LANGUAGES:
                if not Record[2] or Record[2][0] != '"' or Record[2][-1] != '"' or len(Record[2]) == 1:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'language code for RFC_LANGUAGES must have double quotes around it, for example: RFC_LANGUAGES = "en-us;zh-hans"',
                                    File=self.MetaFile, Line=Record[-1])
                LanguageCodes = Record[2][1:-1]
                if not LanguageCodes:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'one or more RFC4646 format language code must be provided for RFC_LANGUAGES statement',
                                    File=self.MetaFile, Line=Record[-1])                
                LanguageList = GetSplitValueList(LanguageCodes, TAB_SEMI_COLON_SPLIT)
                # check whether there is empty entries in the list
                if None in LanguageList:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'one or more empty language code is in RFC_LANGUAGES statement',
                                    File=self.MetaFile, Line=Record[-1])                      
                self._RFCLanguages = LanguageList
            elif Name == TAB_DSC_DEFINES_ISO_LANGUAGES:
                if not Record[2] or Record[2][0] != '"' or Record[2][-1] != '"' or len(Record[2]) == 1:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'language code for ISO_LANGUAGES must have double quotes around it, for example: ISO_LANGUAGES = "engchn"',
                                    File=self.MetaFile, Line=Record[-1])
                LanguageCodes = Record[2][1:-1]
                if not LanguageCodes:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'one or more ISO639-2 format language code must be provided for ISO_LANGUAGES statement',
                                    File=self.MetaFile, Line=Record[-1])                    
                if len(LanguageCodes)%3:
                    EdkLogger.error('build', FORMAT_NOT_SUPPORTED, 'bad ISO639-2 format for ISO_LANGUAGES',
                                    File=self.MetaFile, Line=Record[-1])
                LanguageList = []
                for i in range(0, len(LanguageCodes), 3):
                    LanguageList.append(LanguageCodes[i:i+3])
                self._ISOLanguages = LanguageList               
            elif Name == TAB_DSC_DEFINES_VPD_TOOL_GUID:
                #
                # try to convert GUID to a real UUID value to see whether the GUID is format 
                # for VPD_TOOL_GUID is correct.
                #
                try:
                    uuid.UUID(Record[2])
                except:
                    EdkLogger.error("build", FORMAT_INVALID, "Invalid GUID format for VPD_TOOL_GUID", File=self.MetaFile)
                self._VpdToolGuid = Record[2]                   
            elif Name in self:
                self[Name] = Record[2]                 
        # set _Header to non-None in order to avoid database re-querying
        self._Header = 'DUMMY'

    ## Retrieve platform name
    def _GetPlatformName(self):
        if self._PlatformName == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._PlatformName == None:
                EdkLogger.error('build', ATTRIBUTE_NOT_AVAILABLE, "No PLATFORM_NAME", File=self.MetaFile)
        return self._PlatformName

    ## Retrieve file guid
    def _GetFileGuid(self):
        if self._Guid == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._Guid == None:
                EdkLogger.error('build', ATTRIBUTE_NOT_AVAILABLE, "No PLATFORM_GUID", File=self.MetaFile)
        return self._Guid

    ## Retrieve platform version
    def _GetVersion(self):
        if self._Version == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._Version == None:
                EdkLogger.error('build', ATTRIBUTE_NOT_AVAILABLE, "No PLATFORM_VERSION", File=self.MetaFile)
        return self._Version

    ## Retrieve platform description file version
    def _GetDscSpec(self):
        if self._DscSpecification == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._DscSpecification == None:
                EdkLogger.error('build', ATTRIBUTE_NOT_AVAILABLE, "No DSC_SPECIFICATION", File=self.MetaFile)                
        return self._DscSpecification

    ## Retrieve OUTPUT_DIRECTORY
    def _GetOutpuDir(self):
        if self._OutputDirectory == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._OutputDirectory == None:
                self._OutputDirectory = os.path.join("Build", self._PlatformName)
        return self._OutputDirectory

    ## Retrieve SUPPORTED_ARCHITECTURES
    def _GetSupArch(self):
        if self._SupArchList == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._SupArchList == None:
                EdkLogger.error('build', ATTRIBUTE_NOT_AVAILABLE, "No SUPPORTED_ARCHITECTURES", File=self.MetaFile)
        return self._SupArchList

    ## Retrieve BUILD_TARGETS
    def _GetBuildTarget(self):
        if self._BuildTargets == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._BuildTargets == None:
                EdkLogger.error('build', ATTRIBUTE_NOT_AVAILABLE, "No BUILD_TARGETS", File=self.MetaFile)
        return self._BuildTargets
    
    def _GetPcdInfoFlag(self):
        if self._PcdInfoFlag == None or self._PcdInfoFlag.upper() == 'FALSE':
            return False
        elif self._PcdInfoFlag.upper() == 'TRUE':
            return True
        else:
            return False
    def _GetVarCheckFlag(self):  
        if self._VarCheckFlag == None or self._VarCheckFlag.upper() == 'FALSE':
            return False
        elif self._VarCheckFlag.upper() == 'TRUE':
            return True
        else:
            return False
    def _GetAviableSkuIds(self):
        if self._AvilableSkuIds:
            return self._AvilableSkuIds
        return self.SkuIdentifier
    def _GetSkuIdentifier(self):
        if self._SkuName:
            return self._SkuName
        if self._SkuIdentifier == None:
            if self._Header == None:
                self._GetHeaderInfo()
        return self._SkuIdentifier
    ## Retrieve SKUID_IDENTIFIER
    def _GetSkuName(self):
        if self._SkuName == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if (self._SkuName == None or self._SkuName not in self.SkuIds):
                self._SkuName = 'DEFAULT'
        return self._SkuName

    ## Override SKUID_IDENTIFIER
    def _SetSkuName(self, Value):
        self._SkuName = Value
        self._Pcds = None

    def _GetFdfFile(self):
        if self._FlashDefinition == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._FlashDefinition == None:
                self._FlashDefinition = ''
        return self._FlashDefinition

    ## Retrieve FLASH_DEFINITION
    def _GetBuildNumber(self):
        if self._BuildNumber == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._BuildNumber == None:
                self._BuildNumber = ''
        return self._BuildNumber

    ## Retrieve MAKEFILE_NAME
    def _GetMakefileName(self):
        if self._MakefileName == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._MakefileName == None:
                self._MakefileName = ''
        return self._MakefileName

    ## Retrieve BsBaseAddress
    def _GetBsBaseAddress(self):
        if self._BsBaseAddress == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._BsBaseAddress == None:
                self._BsBaseAddress = ''
        return self._BsBaseAddress

    ## Retrieve RtBaseAddress
    def _GetRtBaseAddress(self):
        if self._RtBaseAddress == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._RtBaseAddress == None:
                self._RtBaseAddress = ''
        return self._RtBaseAddress

    ## Retrieve the top address for the load fix address
    def _GetLoadFixAddress(self):
        if self._LoadFixAddress == None:
            if self._Header == None:
                self._GetHeaderInfo()

            if self._LoadFixAddress == None:
                self._LoadFixAddress = self._Macros.get(TAB_FIX_LOAD_TOP_MEMORY_ADDRESS, '0')

            try:
                self._LoadFixAddress = int (self._LoadFixAddress, 0)
            except:
                EdkLogger.error("build", PARAMETER_INVALID, "FIX_LOAD_TOP_MEMORY_ADDRESS %s is not valid dec or hex string" % (self._LoadFixAddress))
         
        #
        # If command line defined, should override the value in DSC file.
        #
        if 'FIX_LOAD_TOP_MEMORY_ADDRESS' in GlobalData.gCommandLineDefines.keys():
            try:
                self._LoadFixAddress = int(GlobalData.gCommandLineDefines['FIX_LOAD_TOP_MEMORY_ADDRESS'], 0)
            except:
                EdkLogger.error("build", PARAMETER_INVALID, "FIX_LOAD_TOP_MEMORY_ADDRESS %s is not valid dec or hex string" % (GlobalData.gCommandLineDefines['FIX_LOAD_TOP_MEMORY_ADDRESS']))
                
        if self._LoadFixAddress < 0:
            EdkLogger.error("build", PARAMETER_INVALID, "FIX_LOAD_TOP_MEMORY_ADDRESS is set to the invalid negative value 0x%x" % (self._LoadFixAddress))
        if self._LoadFixAddress != 0xFFFFFFFFFFFFFFFF and self._LoadFixAddress % 0x1000 != 0:
            EdkLogger.error("build", PARAMETER_INVALID, "FIX_LOAD_TOP_MEMORY_ADDRESS is set to the invalid unaligned 4K value 0x%x" % (self._LoadFixAddress))
            
        return self._LoadFixAddress

    ## Retrieve RFCLanguage filter
    def _GetRFCLanguages(self):
        if self._RFCLanguages == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._RFCLanguages == None:
                self._RFCLanguages = []
        return self._RFCLanguages

    ## Retrieve ISOLanguage filter
    def _GetISOLanguages(self):
        if self._ISOLanguages == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._ISOLanguages == None:
                self._ISOLanguages = []
        return self._ISOLanguages
    ## Retrieve the GUID string for VPD tool
    def _GetVpdToolGuid(self):
        if self._VpdToolGuid == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._VpdToolGuid == None:
                self._VpdToolGuid = ''
        return self._VpdToolGuid
      
    ## Retrieve [SkuIds] section information
    def _GetSkuIds(self):
        if self._SkuIds == None:
            self._SkuIds = sdict()
            RecordList = self._RawData[MODEL_EFI_SKU_ID, self._Arch]
            for Record in RecordList:
                if Record[0] in [None, '']:
                    EdkLogger.error('build', FORMAT_INVALID, 'No Sku ID number',
                                    File=self.MetaFile, Line=Record[-1])
                if Record[1] in [None, '']:
                    EdkLogger.error('build', FORMAT_INVALID, 'No Sku ID name',
                                    File=self.MetaFile, Line=Record[-1])
                self._SkuIds[Record[1]] = Record[0]
            if 'DEFAULT' not in self._SkuIds:
                self._SkuIds['DEFAULT'] = '0'
            if 'COMMON' not in self._SkuIds:
                self._SkuIds['COMMON'] = '0'
        return self._SkuIds

    ## Retrieve [Components] section information
    def _GetModules(self):
        if self._Modules != None:
            return self._Modules

        self._Modules = sdict()
        RecordList = self._RawData[MODEL_META_DATA_COMPONENT, self._Arch]
        Macros = self._Macros
        Macros["EDK_SOURCE"] = GlobalData.gEcpSource
        for Record in RecordList:
            DuplicatedFile = False
            ModuleFile = PathClass(NormPath(Record[0], Macros), GlobalData.gWorkspace, Arch=self._Arch)
            ModuleId = Record[5]
            LineNo = Record[6]

            # check the file validation
            ErrorCode, ErrorInfo = ModuleFile.Validate('.inf')
            if ErrorCode != 0:
                EdkLogger.error('build', ErrorCode, File=self.MetaFile, Line=LineNo,
                                ExtraData=ErrorInfo)
            # Check duplication
            # If arch is COMMON, no duplicate module is checked since all modules in all component sections are selected
            if self._Arch != 'COMMON' and ModuleFile in self._Modules:
                DuplicatedFile = True

            Module = ModuleBuildClassObject()
            Module.MetaFile = ModuleFile

            # get module private library instance
            RecordList = self._RawData[MODEL_EFI_LIBRARY_CLASS, self._Arch, None, ModuleId]
            for Record in RecordList:
                LibraryClass = Record[0]
                LibraryPath = PathClass(NormPath(Record[1], Macros), GlobalData.gWorkspace, Arch=self._Arch)
                LineNo = Record[-1]

                # check the file validation
                ErrorCode, ErrorInfo = LibraryPath.Validate('.inf')
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, File=self.MetaFile, Line=LineNo,
                                    ExtraData=ErrorInfo)

                if LibraryClass == '' or LibraryClass == 'NULL':
                    self._NullLibraryNumber += 1
                    LibraryClass = 'NULL%d' % self._NullLibraryNumber
                    EdkLogger.verbose("Found forced library for %s\n\t%s [%s]" % (ModuleFile, LibraryPath, LibraryClass))
                Module.LibraryClasses[LibraryClass] = LibraryPath
                if LibraryPath not in self.LibraryInstances:
                    self.LibraryInstances.append(LibraryPath)

            # get module private PCD setting
            for Type in [MODEL_PCD_FIXED_AT_BUILD, MODEL_PCD_PATCHABLE_IN_MODULE, \
                         MODEL_PCD_FEATURE_FLAG, MODEL_PCD_DYNAMIC, MODEL_PCD_DYNAMIC_EX]:
                RecordList = self._RawData[Type, self._Arch, None, ModuleId]
                for TokenSpaceGuid, PcdCName, Setting, Dummy1, Dummy2, Dummy3, Dummy4 in RecordList:
                    TokenList = GetSplitValueList(Setting)
                    DefaultValue = TokenList[0]
                    if len(TokenList) > 1:
                        MaxDatumSize = TokenList[1]
                    else:
                        MaxDatumSize = ''
                    TypeString = self._PCD_TYPE_STRING_[Type]
                    Pcd = PcdClassObject(
                            PcdCName,
                            TokenSpaceGuid,
                            TypeString,
                            '',
                            DefaultValue,
                            '',
                            MaxDatumSize,
                            {},
                            False,
                            None
                            )
                    Module.Pcds[PcdCName, TokenSpaceGuid] = Pcd

            # get module private build options
            RecordList = self._RawData[MODEL_META_DATA_BUILD_OPTION, self._Arch, None, ModuleId]
            for ToolChainFamily, ToolChain, Option, Dummy1, Dummy2, Dummy3, Dummy4 in RecordList:
                if (ToolChainFamily, ToolChain) not in Module.BuildOptions:
                    Module.BuildOptions[ToolChainFamily, ToolChain] = Option
                else:
                    OptionString = Module.BuildOptions[ToolChainFamily, ToolChain]
                    Module.BuildOptions[ToolChainFamily, ToolChain] = OptionString + " " + Option

            RecordList = self._RawData[MODEL_META_DATA_HEADER, self._Arch, None, ModuleId]
            if DuplicatedFile and not RecordList:
                EdkLogger.error('build', FILE_DUPLICATED, File=self.MetaFile, ExtraData=str(ModuleFile), Line=LineNo)
            if RecordList:
                if len(RecordList) != 1:
                    EdkLogger.error('build', OPTION_UNKNOWN, 'Only FILE_GUID can be listed in <Defines> section.',
                                    File=self.MetaFile, ExtraData=str(ModuleFile), Line=LineNo)
                ModuleFile = ProcessDuplicatedInf(ModuleFile, RecordList[0][2], GlobalData.gWorkspace)
                ModuleFile.Arch = self._Arch

            self._Modules[ModuleFile] = Module
        return self._Modules

    ## Retrieve all possible library instances used in this platform
    def _GetLibraryInstances(self):
        if self._LibraryInstances == None:
            self._GetLibraryClasses()
        return self._LibraryInstances

    ## Retrieve [LibraryClasses] information
    def _GetLibraryClasses(self):
        if self._LibraryClasses == None:
            self._LibraryInstances = []
            #
            # tdict is a special dict kind of type, used for selecting correct
            # library instance for given library class and module type
            #
            LibraryClassDict = tdict(True, 3)
            # track all library class names
            LibraryClassSet = set()
            RecordList = self._RawData[MODEL_EFI_LIBRARY_CLASS, self._Arch, None, -1]
            Macros = self._Macros
            for Record in RecordList:
                LibraryClass, LibraryInstance, Dummy, Arch, ModuleType, Dummy, LineNo = Record
                if LibraryClass == '' or LibraryClass == 'NULL':
                    self._NullLibraryNumber += 1
                    LibraryClass = 'NULL%d' % self._NullLibraryNumber
                    EdkLogger.verbose("Found forced library for arch=%s\n\t%s [%s]" % (Arch, LibraryInstance, LibraryClass))
                LibraryClassSet.add(LibraryClass)
                LibraryInstance = PathClass(NormPath(LibraryInstance, Macros), GlobalData.gWorkspace, Arch=self._Arch)
                # check the file validation
                ErrorCode, ErrorInfo = LibraryInstance.Validate('.inf')
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, File=self.MetaFile, Line=LineNo,
                                    ExtraData=ErrorInfo)

                if ModuleType != 'COMMON' and ModuleType not in SUP_MODULE_LIST:
                    EdkLogger.error('build', OPTION_UNKNOWN, "Unknown module type [%s]" % ModuleType,
                                    File=self.MetaFile, ExtraData=LibraryInstance, Line=LineNo)
                LibraryClassDict[Arch, ModuleType, LibraryClass] = LibraryInstance
                if LibraryInstance not in self._LibraryInstances:
                    self._LibraryInstances.append(LibraryInstance)

            # resolve the specific library instance for each class and each module type
            self._LibraryClasses = tdict(True)
            for LibraryClass in LibraryClassSet:
                # try all possible module types
                for ModuleType in SUP_MODULE_LIST:
                    LibraryInstance = LibraryClassDict[self._Arch, ModuleType, LibraryClass]
                    if LibraryInstance == None:
                        continue
                    self._LibraryClasses[LibraryClass, ModuleType] = LibraryInstance

            # for Edk style library instances, which are listed in different section
            Macros["EDK_SOURCE"] = GlobalData.gEcpSource
            RecordList = self._RawData[MODEL_EFI_LIBRARY_INSTANCE, self._Arch]
            for Record in RecordList:
                File = PathClass(NormPath(Record[0], Macros), GlobalData.gWorkspace, Arch=self._Arch)
                LineNo = Record[-1]
                # check the file validation
                ErrorCode, ErrorInfo = File.Validate('.inf')
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, File=self.MetaFile, Line=LineNo,
                                    ExtraData=ErrorInfo)
                if File not in self._LibraryInstances:
                    self._LibraryInstances.append(File)
                #
                # we need the module name as the library class name, so we have
                # to parse it here. (self._Bdb[] will trigger a file parse if it
                # hasn't been parsed)
                #
                Library = self._Bdb[File, self._Arch, self._Target, self._Toolchain]
                self._LibraryClasses[Library.BaseName, ':dummy:'] = Library
        return self._LibraryClasses

    def _ValidatePcd(self, PcdCName, TokenSpaceGuid, Setting, PcdType, LineNo):
        if self._DecPcds == None:
            self._DecPcds = GetDeclaredPcd(self, self._Bdb, self._Arch, self._Target, self._Toolchain)
            FdfInfList = []
            if GlobalData.gFdfParser:
                FdfInfList = GlobalData.gFdfParser.Profile.InfList

            PkgSet = set()
            for Inf in FdfInfList:
                ModuleFile = PathClass(NormPath(Inf), GlobalData.gWorkspace, Arch=self._Arch)
                if ModuleFile in self._Modules:
                    continue
                ModuleData = self._Bdb[ModuleFile, self._Arch, self._Target, self._Toolchain]
                PkgSet.update(ModuleData.Packages)
            DecPcds = {}
            for Pkg in PkgSet:
                for Pcd in Pkg.Pcds:
                    DecPcds[Pcd[0], Pcd[1]] = Pkg.Pcds[Pcd]
            self._DecPcds.update(DecPcds)

        if (PcdCName, TokenSpaceGuid) not in self._DecPcds:
            EdkLogger.error('build', PARSER_ERROR,
                            "Pcd (%s.%s) defined in DSC is not declared in DEC files. Arch: ['%s']" % (TokenSpaceGuid, PcdCName, self._Arch),
                            File=self.MetaFile, Line=LineNo)
        ValueList, IsValid, Index = AnalyzeDscPcd(Setting, PcdType, self._DecPcds[PcdCName, TokenSpaceGuid].DatumType)
        if not IsValid and PcdType not in [MODEL_PCD_FEATURE_FLAG, MODEL_PCD_FIXED_AT_BUILD]:
            EdkLogger.error('build', FORMAT_INVALID, "Pcd format incorrect.", File=self.MetaFile, Line=LineNo,
                            ExtraData="%s.%s|%s" % (TokenSpaceGuid, PcdCName, Setting))
        if ValueList[Index] and PcdType not in [MODEL_PCD_FEATURE_FLAG, MODEL_PCD_FIXED_AT_BUILD]:
            try:
                ValueList[Index] = ValueExpression(ValueList[Index], GlobalData.gPlatformPcds)(True)
            except WrnExpression, Value:
                ValueList[Index] = Value.result
            except EvaluationException, Excpt:
                if hasattr(Excpt, 'Pcd'):
                    if Excpt.Pcd in GlobalData.gPlatformOtherPcds:
                        EdkLogger.error('Parser', FORMAT_INVALID, "Cannot use this PCD (%s) in an expression as"
                                        " it must be defined in a [PcdsFixedAtBuild] or [PcdsFeatureFlag] section"
                                        " of the DSC file" % Excpt.Pcd,
                                        File=self.MetaFile, Line=LineNo)
                    else:
                        EdkLogger.error('Parser', FORMAT_INVALID, "PCD (%s) is not defined in DSC file" % Excpt.Pcd,
                                        File=self.MetaFile, Line=LineNo)
                else:
                    EdkLogger.error('Parser', FORMAT_INVALID, "Invalid expression: %s" % str(Excpt),
                                    File=self.MetaFile, Line=LineNo)
            if ValueList[Index] == 'True':
                ValueList[Index] = '1'
            elif ValueList[Index] == 'False':
                ValueList[Index] = '0'
        if ValueList[Index]:
            Valid, ErrStr = CheckPcdDatum(self._DecPcds[PcdCName, TokenSpaceGuid].DatumType, ValueList[Index])
            if not Valid:
                EdkLogger.error('build', FORMAT_INVALID, ErrStr, File=self.MetaFile, Line=LineNo,
                                ExtraData="%s.%s" % (TokenSpaceGuid, PcdCName))
        return ValueList

    ## Retrieve all PCD settings in platform
    def _GetPcds(self):
        if self._Pcds == None:
            self._Pcds = sdict()
            self._Pcds.update(self._GetPcd(MODEL_PCD_FIXED_AT_BUILD))
            self._Pcds.update(self._GetPcd(MODEL_PCD_PATCHABLE_IN_MODULE))
            self._Pcds.update(self._GetPcd(MODEL_PCD_FEATURE_FLAG))
            self._Pcds.update(self._GetDynamicPcd(MODEL_PCD_DYNAMIC_DEFAULT))
            self._Pcds.update(self._GetDynamicHiiPcd(MODEL_PCD_DYNAMIC_HII))
            self._Pcds.update(self._GetDynamicVpdPcd(MODEL_PCD_DYNAMIC_VPD))
            self._Pcds.update(self._GetDynamicPcd(MODEL_PCD_DYNAMIC_EX_DEFAULT))
            self._Pcds.update(self._GetDynamicHiiPcd(MODEL_PCD_DYNAMIC_EX_HII))
            self._Pcds.update(self._GetDynamicVpdPcd(MODEL_PCD_DYNAMIC_EX_VPD))
        return self._Pcds

    ## Retrieve [BuildOptions]
    def _GetBuildOptions(self):
        if self._BuildOptions == None:
            self._BuildOptions = sdict()
            #
            # Retrieve build option for EDKII and EDK style module
            #
            for CodeBase in (EDKII_NAME, EDK_NAME):
                RecordList = self._RawData[MODEL_META_DATA_BUILD_OPTION, self._Arch, CodeBase]
                for ToolChainFamily, ToolChain, Option, Dummy1, Dummy2, Dummy3, Dummy4 in RecordList:
                    CurKey = (ToolChainFamily, ToolChain, CodeBase)
                    #
                    # Only flags can be appended
                    #
                    if CurKey not in self._BuildOptions or not ToolChain.endswith('_FLAGS') or Option.startswith('='):
                        self._BuildOptions[CurKey] = Option
                    else:
                        self._BuildOptions[CurKey] += ' ' + Option
        return self._BuildOptions

    def GetBuildOptionsByModuleType(self, Edk, ModuleType):
        if self._ModuleTypeOptions == None:
            self._ModuleTypeOptions = sdict()
        if (Edk, ModuleType) not in self._ModuleTypeOptions:
            options = sdict()
            self._ModuleTypeOptions[Edk, ModuleType] = options
            DriverType = '%s.%s' % (Edk, ModuleType)
            RecordList = self._RawData[MODEL_META_DATA_BUILD_OPTION, self._Arch, DriverType]
            for ToolChainFamily, ToolChain, Option, Arch, Type, Dummy3, Dummy4 in RecordList:
                if Type == DriverType:
                    Key = (ToolChainFamily, ToolChain, Edk)
                    if Key not in options or not ToolChain.endswith('_FLAGS') or Option.startswith('='):
                        options[Key] = Option
                    else:
                        options[Key] += ' ' + Option
        return self._ModuleTypeOptions[Edk, ModuleType]

    ## Retrieve non-dynamic PCD settings
    #
    #   @param  Type    PCD type
    #
    #   @retval a dict object contains settings of given PCD type
    #
    def _GetPcd(self, Type):
        Pcds = sdict()
        #
        # tdict is a special dict kind of type, used for selecting correct
        # PCD settings for certain ARCH
        #
        
        SkuObj = SkuClass(self.SkuIdentifier,self.SkuIds)
        
        PcdDict = tdict(True, 3)
        PcdSet = set()
        # Find out all possible PCD candidates for self._Arch
        RecordList = self._RawData[Type, self._Arch]
        PcdValueDict = sdict()
        for TokenSpaceGuid, PcdCName, Setting, Arch, SkuName, Dummy3, Dummy4 in RecordList:
            if SkuName in (SkuObj.SystemSkuId,'DEFAULT','COMMON'):
                PcdSet.add((PcdCName, TokenSpaceGuid, SkuName,Dummy4))
                PcdDict[Arch, PcdCName, TokenSpaceGuid,SkuName] = Setting
        
        #handle pcd value override        
        for PcdCName, TokenSpaceGuid, SkuName,Dummy4 in PcdSet:
            Setting = PcdDict[self._Arch, PcdCName, TokenSpaceGuid,SkuName]
            if Setting == None:
                continue
            PcdValue, DatumType, MaxDatumSize = self._ValidatePcd(PcdCName, TokenSpaceGuid, Setting, Type, Dummy4)
            if (PcdCName, TokenSpaceGuid) in PcdValueDict:
                PcdValueDict[PcdCName, TokenSpaceGuid][SkuName] = (PcdValue,DatumType,MaxDatumSize) 
            else:
                PcdValueDict[PcdCName, TokenSpaceGuid] = {SkuName:(PcdValue,DatumType,MaxDatumSize)}       
        
        PcdsKeys = PcdValueDict.keys()
        for PcdCName,TokenSpaceGuid in PcdsKeys:
            
            PcdSetting = PcdValueDict[PcdCName, TokenSpaceGuid]
            PcdValue = None
            DatumType = None
            MaxDatumSize = None
            if 'COMMON' in PcdSetting:
                PcdValue,DatumType,MaxDatumSize = PcdSetting['COMMON']
            if 'DEFAULT' in PcdSetting:
                PcdValue,DatumType,MaxDatumSize = PcdSetting['DEFAULT']
            if SkuObj.SystemSkuId in PcdSetting:
                PcdValue,DatumType,MaxDatumSize = PcdSetting[SkuObj.SystemSkuId]
                
            Pcds[PcdCName, TokenSpaceGuid] = PcdClassObject(
                                                PcdCName,
                                                TokenSpaceGuid,
                                                self._PCD_TYPE_STRING_[Type],
                                                DatumType,
                                                PcdValue,
                                                '',
                                                MaxDatumSize,
                                                {},
                                                False,
                                                None
                                                )
        return Pcds

    ## Retrieve dynamic PCD settings
    #
    #   @param  Type    PCD type
    #
    #   @retval a dict object contains settings of given PCD type
    #
    def _GetDynamicPcd(self, Type):
        
        SkuObj = SkuClass(self.SkuIdentifier,self.SkuIds)
        
        Pcds = sdict()
        #
        # tdict is a special dict kind of type, used for selecting correct
        # PCD settings for certain ARCH and SKU
        #
        PcdDict = tdict(True, 4)
        PcdList = []
        # Find out all possible PCD candidates for self._Arch
        RecordList = self._RawData[Type, self._Arch]
        AvailableSkuIdSet = SkuObj.AvailableSkuIdSet.copy()
        
        AvailableSkuIdSet.update({'DEFAULT':0,'COMMON':0})
        for TokenSpaceGuid, PcdCName, Setting, Arch, SkuName, Dummy3, Dummy4 in RecordList:
            if SkuName not in AvailableSkuIdSet:
                continue
            
            PcdList.append((PcdCName, TokenSpaceGuid, SkuName,Dummy4))
            PcdDict[Arch, SkuName, PcdCName, TokenSpaceGuid] = Setting
        # Remove redundant PCD candidates, per the ARCH and SKU
        for PcdCName, TokenSpaceGuid, SkuName, Dummy4 in PcdList:
            
            Setting = PcdDict[self._Arch, SkuName, PcdCName, TokenSpaceGuid]
            if Setting == None:
                continue
                      
            PcdValue, DatumType, MaxDatumSize = self._ValidatePcd(PcdCName, TokenSpaceGuid, Setting, Type, Dummy4)
            SkuInfo = SkuInfoClass(SkuName, self.SkuIds[SkuName], '', '', '', '', '', PcdValue)
            if (PcdCName,TokenSpaceGuid) in Pcds.keys(): 
                pcdObject = Pcds[PcdCName,TokenSpaceGuid]
                pcdObject.SkuInfoList[SkuName] = SkuInfo
                if MaxDatumSize.strip():
                    CurrentMaxSize = int(MaxDatumSize.strip(),0)
                else:
                    CurrentMaxSize = 0
                if pcdObject.MaxDatumSize:
                    PcdMaxSize = int(pcdObject.MaxDatumSize,0)
                else:
                    PcdMaxSize = 0
                if CurrentMaxSize > PcdMaxSize:
                    pcdObject.MaxDatumSize = str(CurrentMaxSize)
            else:               
                Pcds[PcdCName, TokenSpaceGuid] = PcdClassObject(
                                                    PcdCName,
                                                    TokenSpaceGuid,
                                                    self._PCD_TYPE_STRING_[Type],
                                                    DatumType,
                                                    PcdValue,
                                                    '',
                                                    MaxDatumSize,
                                                    {SkuName : SkuInfo},
                                                    False,
                                                    None
                                                    )
        
        for pcd in Pcds.values():
            pcdDecObject = self._DecPcds[pcd.TokenCName,pcd.TokenSpaceGuidCName]
            if 'DEFAULT' not in pcd.SkuInfoList.keys() and 'COMMON' not in pcd.SkuInfoList.keys():                
                valuefromDec = pcdDecObject.DefaultValue
                SkuInfo = SkuInfoClass('DEFAULT', '0', '', '', '', '', '', valuefromDec)
                pcd.SkuInfoList['DEFAULT'] = SkuInfo
            elif 'DEFAULT' not in pcd.SkuInfoList.keys() and 'COMMON' in pcd.SkuInfoList.keys():
                pcd.SkuInfoList['DEFAULT'] = pcd.SkuInfoList['COMMON']
                del(pcd.SkuInfoList['COMMON'])
            elif 'DEFAULT' in pcd.SkuInfoList.keys() and 'COMMON' in pcd.SkuInfoList.keys():
                del(pcd.SkuInfoList['COMMON'])
            if SkuObj.SkuUsageType == SkuObj.SINGLE:
                if 'DEFAULT' in pcd.SkuInfoList.keys() and SkuObj.SystemSkuId not in pcd.SkuInfoList.keys():
                    pcd.SkuInfoList[SkuObj.SystemSkuId] = pcd.SkuInfoList['DEFAULT']
                del(pcd.SkuInfoList['DEFAULT'])
               
        return Pcds

    def CompareVarAttr(self, Attr1, Attr2):
        if not Attr1 or not Attr2:  # for empty string
            return True
        Attr1s = [attr.strip() for attr in Attr1.split(",")]
        Attr1Set = set(Attr1s)
        Attr2s = [attr.strip() for attr in Attr2.split(",")]
        Attr2Set = set(Attr2s)
        if Attr2Set == Attr1Set:
            return True
        else:
            return False
    ## Retrieve dynamic HII PCD settings
    #
    #   @param  Type    PCD type
    #
    #   @retval a dict object contains settings of given PCD type
    #
    def _GetDynamicHiiPcd(self, Type):
        
        SkuObj = SkuClass(self.SkuIdentifier,self.SkuIds)
        VariableAttrs = {}
        
        Pcds = sdict()
        #
        # tdict is a special dict kind of type, used for selecting correct
        # PCD settings for certain ARCH and SKU
        #
        PcdDict = tdict(True, 4)
        PcdSet = set()
        RecordList = self._RawData[Type, self._Arch]
        # Find out all possible PCD candidates for self._Arch
        AvailableSkuIdSet = SkuObj.AvailableSkuIdSet.copy()
        
        AvailableSkuIdSet.update({'DEFAULT':0,'COMMON':0})
        for TokenSpaceGuid, PcdCName, Setting, Arch, SkuName, Dummy3, Dummy4 in RecordList:
            if SkuName not in AvailableSkuIdSet:
                continue
            PcdSet.add((PcdCName, TokenSpaceGuid, SkuName,Dummy4))
            PcdDict[Arch, SkuName, PcdCName, TokenSpaceGuid] = Setting
        # Remove redundant PCD candidates, per the ARCH and SKU
        for PcdCName, TokenSpaceGuid,SkuName, Dummy4 in PcdSet:
            
            Setting = PcdDict[self._Arch, SkuName, PcdCName, TokenSpaceGuid]
            if Setting == None:
                continue
            VariableName, VariableGuid, VariableOffset, DefaultValue, VarAttribute = self._ValidatePcd(PcdCName, TokenSpaceGuid, Setting, Type, Dummy4)
            
            rt, Msg = VariableAttributes.ValidateVarAttributes(VarAttribute)
            if not rt:
                EdkLogger.error("build", PCD_VARIABLE_ATTRIBUTES_ERROR, "Variable attributes settings for %s is incorrect.\n %s" % (".".join((TokenSpaceGuid, PcdCName)), Msg),
                        ExtraData = "[%s]" % VarAttribute)
            ExceedMax = False
            FormatCorrect = True
            if VariableOffset.isdigit():
                if int(VariableOffset,10) > 0xFFFF:
                    ExceedMax = True
            elif re.match(r'[\t\s]*0[xX][a-fA-F0-9]+$',VariableOffset):
                if int(VariableOffset,16) > 0xFFFF:
                    ExceedMax = True
            # For Offset written in "A.B"
            elif VariableOffset.find('.') > -1:
                VariableOffsetList = VariableOffset.split(".")
                if not (len(VariableOffsetList) == 2
                        and IsValidWord(VariableOffsetList[0])
                        and IsValidWord(VariableOffsetList[1])):
                    FormatCorrect = False
            else:
                FormatCorrect = False
            if not FormatCorrect:
                EdkLogger.error('Build', FORMAT_INVALID, "Invalid syntax or format of the variable offset value is incorrect for %s." % ".".join((TokenSpaceGuid,PcdCName)))
            
            if ExceedMax:
                EdkLogger.error('Build', OPTION_VALUE_INVALID, "The variable offset value must not exceed the maximum value of 0xFFFF (UINT16) for %s." % ".".join((TokenSpaceGuid,PcdCName)))
            if (VariableName, VariableGuid) not in VariableAttrs:
                VariableAttrs[(VariableName, VariableGuid)] = VarAttribute
            else:
                if not self.CompareVarAttr(VariableAttrs[(VariableName, VariableGuid)], VarAttribute):
                    EdkLogger.error('Build', PCD_VARIABLE_ATTRIBUTES_CONFLICT_ERROR, "The variable %s.%s for DynamicHii PCDs has conflicting attributes [%s] and [%s] " % (VariableGuid, VariableName, VarAttribute, VariableAttrs[(VariableName, VariableGuid)]))
            
            SkuInfo = SkuInfoClass(SkuName, self.SkuIds[SkuName], VariableName, VariableGuid, VariableOffset, DefaultValue, VariableAttribute = VarAttribute)
            pcdDecObject = self._DecPcds[PcdCName, TokenSpaceGuid]
            if (PcdCName,TokenSpaceGuid) in Pcds.keys():  
                pcdObject = Pcds[PcdCName,TokenSpaceGuid]
                pcdObject.SkuInfoList[SkuName] = SkuInfo
            else:
                Pcds[PcdCName, TokenSpaceGuid] = PcdClassObject(
                                                PcdCName,
                                                TokenSpaceGuid,
                                                self._PCD_TYPE_STRING_[Type],
                                                '',
                                                DefaultValue,
                                                '',
                                                '',
                                                {SkuName : SkuInfo},
                                                False,
                                                None,
                                                pcdDecObject.validateranges,
                                                pcdDecObject.validlists,
                                                pcdDecObject.expressions
                                                )
                

        for pcd in Pcds.values():
            SkuInfoObj = pcd.SkuInfoList.values()[0]
            pcdDecObject = self._DecPcds[pcd.TokenCName,pcd.TokenSpaceGuidCName]
            # Only fix the value while no value provided in DSC file.
            for sku in pcd.SkuInfoList.values():
                if (sku.HiiDefaultValue == "" or sku.HiiDefaultValue==None):
                    sku.HiiDefaultValue = pcdDecObject.DefaultValue
            if 'DEFAULT' not in pcd.SkuInfoList.keys() and 'COMMON' not in pcd.SkuInfoList.keys():              
                valuefromDec = pcdDecObject.DefaultValue
                SkuInfo = SkuInfoClass('DEFAULT', '0', SkuInfoObj.VariableName, SkuInfoObj.VariableGuid, SkuInfoObj.VariableOffset, valuefromDec)
                pcd.SkuInfoList['DEFAULT'] = SkuInfo
            elif 'DEFAULT' not in pcd.SkuInfoList.keys() and 'COMMON' in pcd.SkuInfoList.keys():
                pcd.SkuInfoList['DEFAULT'] = pcd.SkuInfoList['COMMON']
                del(pcd.SkuInfoList['COMMON'])
            elif 'DEFAULT' in pcd.SkuInfoList.keys() and 'COMMON' in pcd.SkuInfoList.keys():
                del(pcd.SkuInfoList['COMMON'])
                
            if SkuObj.SkuUsageType == SkuObj.SINGLE:
                if 'DEFAULT' in pcd.SkuInfoList.keys() and SkuObj.SystemSkuId not in pcd.SkuInfoList.keys():
                    pcd.SkuInfoList[SkuObj.SystemSkuId] = pcd.SkuInfoList['DEFAULT']
                del(pcd.SkuInfoList['DEFAULT'])
            
            
            if pcd.MaxDatumSize.strip(): 
                MaxSize = int(pcd.MaxDatumSize,0)
            else:
                MaxSize = 0
            if pcdDecObject.DatumType == 'VOID*':
                for (skuname,skuobj) in pcd.SkuInfoList.items():
                    datalen = 0
                    if skuobj.HiiDefaultValue.startswith("L"):
                        datalen = (len(skuobj.HiiDefaultValue)- 3 + 1) * 2
                    elif skuobj.HiiDefaultValue.startswith("{"):
                        datalen = len(skuobj.HiiDefaultValue.split(","))
                    else:
                        datalen = len(skuobj.HiiDefaultValue) -2 + 1 
                    if datalen>MaxSize:
                        MaxSize = datalen
                pcd.MaxDatumSize = str(MaxSize)
        return Pcds

    ## Retrieve dynamic VPD PCD settings
    #
    #   @param  Type    PCD type
    #
    #   @retval a dict object contains settings of given PCD type
    #
    def _GetDynamicVpdPcd(self, Type):
        
        SkuObj = SkuClass(self.SkuIdentifier,self.SkuIds)
        
        Pcds = sdict()
        #
        # tdict is a special dict kind of type, used for selecting correct
        # PCD settings for certain ARCH and SKU
        #
        PcdDict = tdict(True, 4)
        PcdList = []
        # Find out all possible PCD candidates for self._Arch
        RecordList = self._RawData[Type, self._Arch]
        AvailableSkuIdSet = SkuObj.AvailableSkuIdSet.copy()
        
        AvailableSkuIdSet.update({'DEFAULT':0,'COMMON':0})
        for TokenSpaceGuid, PcdCName, Setting, Arch, SkuName, Dummy3, Dummy4 in RecordList:
            if SkuName not in AvailableSkuIdSet:
                continue

            PcdList.append((PcdCName, TokenSpaceGuid,SkuName, Dummy4))
            PcdDict[Arch, SkuName, PcdCName, TokenSpaceGuid] = Setting
        # Remove redundant PCD candidates, per the ARCH and SKU
        for PcdCName, TokenSpaceGuid, SkuName,Dummy4 in PcdList:
            Setting = PcdDict[self._Arch, SkuName, PcdCName, TokenSpaceGuid]
            if Setting == None:
                continue
            #
            # For the VOID* type, it can have optional data of MaxDatumSize and InitialValue
            # For the Integer & Boolean type, the optional data can only be InitialValue.
            # At this point, we put all the data into the PcdClssObject for we don't know the PCD's datumtype
            # until the DEC parser has been called.
            # 
            VpdOffset, MaxDatumSize, InitialValue = self._ValidatePcd(PcdCName, TokenSpaceGuid, Setting, Type, Dummy4)
            SkuInfo = SkuInfoClass(SkuName, self.SkuIds[SkuName], '', '', '', '', VpdOffset, InitialValue)
            if (PcdCName,TokenSpaceGuid) in Pcds.keys():  
                pcdObject = Pcds[PcdCName,TokenSpaceGuid]
                pcdObject.SkuInfoList[SkuName] = SkuInfo
                if MaxDatumSize.strip():
                    CurrentMaxSize = int(MaxDatumSize.strip(),0)
                else:
                    CurrentMaxSize = 0
                if pcdObject.MaxDatumSize:
                    PcdMaxSize = int(pcdObject.MaxDatumSize,0)
                else:
                    PcdMaxSize = 0
                if CurrentMaxSize > PcdMaxSize:
                    pcdObject.MaxDatumSize = str(CurrentMaxSize)
            else:
                Pcds[PcdCName, TokenSpaceGuid] = PcdClassObject(
                                                PcdCName,
                                                TokenSpaceGuid,
                                                self._PCD_TYPE_STRING_[Type],
                                                '',
                                                InitialValue,
                                                '',
                                                MaxDatumSize,
                                                {SkuName : SkuInfo},
                                                False,
                                                None
                                                )
        for pcd in Pcds.values():
            SkuInfoObj = pcd.SkuInfoList.values()[0]
            pcdDecObject = self._DecPcds[pcd.TokenCName,pcd.TokenSpaceGuidCName]
            if 'DEFAULT' not in pcd.SkuInfoList.keys() and 'COMMON' not in pcd.SkuInfoList.keys():
                valuefromDec = pcdDecObject.DefaultValue
                SkuInfo = SkuInfoClass('DEFAULT', '0', '', '', '','',SkuInfoObj.VpdOffset, valuefromDec)
                pcd.SkuInfoList['DEFAULT'] = SkuInfo
            elif 'DEFAULT' not in pcd.SkuInfoList.keys() and 'COMMON' in pcd.SkuInfoList.keys():
                pcd.SkuInfoList['DEFAULT'] = pcd.SkuInfoList['COMMON']
                del(pcd.SkuInfoList['COMMON'])
            elif 'DEFAULT' in pcd.SkuInfoList.keys() and 'COMMON' in pcd.SkuInfoList.keys():
                del(pcd.SkuInfoList['COMMON'])
            if SkuObj.SkuUsageType == SkuObj.SINGLE:
                if 'DEFAULT' in pcd.SkuInfoList.keys() and SkuObj.SystemSkuId not in pcd.SkuInfoList.keys():
                    pcd.SkuInfoList[SkuObj.SystemSkuId] = pcd.SkuInfoList['DEFAULT']
                del(pcd.SkuInfoList['DEFAULT'])
            
        return Pcds

    ## Add external modules
    #
    #   The external modules are mostly those listed in FDF file, which don't
    # need "build".
    #
    #   @param  FilePath    The path of module description file
    #
    def AddModule(self, FilePath):
        FilePath = NormPath(FilePath)
        if FilePath not in self.Modules:
            Module = ModuleBuildClassObject()
            Module.MetaFile = FilePath
            self.Modules.append(Module)

    ## Add external PCDs
    #
    #   The external PCDs are mostly those listed in FDF file to specify address
    # or offset information.
    #
    #   @param  Name    Name of the PCD
    #   @param  Guid    Token space guid of the PCD
    #   @param  Value   Value of the PCD
    #
    def AddPcd(self, Name, Guid, Value):
        if (Name, Guid) not in self.Pcds:
            self.Pcds[Name, Guid] = PcdClassObject(Name, Guid, '', '', '', '', '', {}, False, None)
        self.Pcds[Name, Guid].DefaultValue = Value

    _Macros             = property(_GetMacros)
    Arch                = property(_GetArch, _SetArch)
    Platform            = property(_GetPlatformName)
    PlatformName        = property(_GetPlatformName)
    Guid                = property(_GetFileGuid)
    Version             = property(_GetVersion)
    DscSpecification    = property(_GetDscSpec)
    OutputDirectory     = property(_GetOutpuDir)
    SupArchList         = property(_GetSupArch)
    BuildTargets        = property(_GetBuildTarget)
    SkuName             = property(_GetSkuName, _SetSkuName)
    SkuIdentifier       = property(_GetSkuIdentifier)
    AvilableSkuIds = property(_GetAviableSkuIds)
    PcdInfoFlag         = property(_GetPcdInfoFlag)
    VarCheckFlag = property(_GetVarCheckFlag)
    FlashDefinition     = property(_GetFdfFile)
    BuildNumber         = property(_GetBuildNumber)
    MakefileName        = property(_GetMakefileName)
    BsBaseAddress       = property(_GetBsBaseAddress)
    RtBaseAddress       = property(_GetRtBaseAddress)
    LoadFixAddress      = property(_GetLoadFixAddress)
    RFCLanguages        = property(_GetRFCLanguages)
    ISOLanguages        = property(_GetISOLanguages)
    VpdToolGuid         = property(_GetVpdToolGuid)   
    SkuIds              = property(_GetSkuIds)
    Modules             = property(_GetModules)
    LibraryInstances    = property(_GetLibraryInstances)
    LibraryClasses      = property(_GetLibraryClasses)
    Pcds                = property(_GetPcds)
    BuildOptions        = property(_GetBuildOptions)

## Platform build information from DEC file
#
#  This class is used to retrieve information stored in database and convert them
# into PackageBuildClassObject form for easier use for AutoGen.
#
class DecBuildData(PackageBuildClassObject):
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

    # dict used to convert part of [Defines] to members of DecBuildData directly
    _PROPERTY_ = {
        #
        # Required Fields
        #
        TAB_DEC_DEFINES_PACKAGE_NAME                : "_PackageName",
        TAB_DEC_DEFINES_PACKAGE_GUID                : "_Guid",
        TAB_DEC_DEFINES_PACKAGE_VERSION             : "_Version",
        TAB_DEC_DEFINES_PKG_UNI_FILE                : "_PkgUniFile",
    }


    ## Constructor of DecBuildData
    #
    #  Initialize object of DecBuildData
    #
    #   @param      FilePath        The path of package description file
    #   @param      RawData         The raw data of DEC file
    #   @param      BuildDataBase   Database used to retrieve module information
    #   @param      Arch            The target architecture
    #   @param      Platform        (not used for DecBuildData)
    #   @param      Macros          Macros used for replacement in DSC file
    #
    def __init__(self, File, RawData, BuildDataBase, Arch='COMMON', Target=None, Toolchain=None):
        self.MetaFile = File
        self._PackageDir = File.Dir
        self._RawData = RawData
        self._Bdb = BuildDataBase
        self._Arch = Arch
        self._Target = Target
        self._Toolchain = Toolchain
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

    ## Set all internal used members of DecBuildData to None
    def _Clear(self):
        self._Header            = None
        self._PackageName       = None
        self._Guid              = None
        self._Version           = None
        self._PkgUniFile        = None
        self._Protocols         = None
        self._Ppis              = None
        self._Guids             = None
        self._Includes          = None
        self._LibraryClasses    = None
        self._Pcds              = None
        self.__Macros           = None

    ## Get current effective macros
    def _GetMacros(self):
        if self.__Macros == None:
            self.__Macros = {}
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

    ## Retrieve all information in [Defines] section
    #
    #   (Retriving all [Defines] information in one-shot is just to save time.)
    #
    def _GetHeaderInfo(self):
        RecordList = self._RawData[MODEL_META_DATA_HEADER, self._Arch]
        for Record in RecordList:
            Name = Record[1]
            if Name in self:
                self[Name] = Record[2]
        self._Header = 'DUMMY'

    ## Retrieve package name
    def _GetPackageName(self):
        if self._PackageName == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._PackageName == None:
                EdkLogger.error("build", ATTRIBUTE_NOT_AVAILABLE, "No PACKAGE_NAME", File=self.MetaFile)
        return self._PackageName

    ## Retrieve file guid
    def _GetFileGuid(self):
        if self._Guid == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._Guid == None:
                EdkLogger.error("build", ATTRIBUTE_NOT_AVAILABLE, "No PACKAGE_GUID", File=self.MetaFile)
        return self._Guid

    ## Retrieve package version
    def _GetVersion(self):
        if self._Version == None:
            if self._Header == None:
                self._GetHeaderInfo()
            if self._Version == None:
                self._Version = ''
        return self._Version

    ## Retrieve protocol definitions (name/value pairs)
    def _GetProtocol(self):
        if self._Protocols == None:
            #
            # tdict is a special kind of dict, used for selecting correct
            # protocol defition for given ARCH
            #
            ProtocolDict = tdict(True)
            NameList = []
            # find out all protocol definitions for specific and 'common' arch
            RecordList = self._RawData[MODEL_EFI_PROTOCOL, self._Arch]
            for Name, Guid, Dummy, Arch, ID, LineNo in RecordList:
                if Name not in NameList:
                    NameList.append(Name)
                ProtocolDict[Arch, Name] = Guid
            # use sdict to keep the order
            self._Protocols = sdict()
            for Name in NameList:
                #
                # limit the ARCH to self._Arch, if no self._Arch found, tdict
                # will automatically turn to 'common' ARCH for trying
                #
                self._Protocols[Name] = ProtocolDict[self._Arch, Name]
        return self._Protocols

    ## Retrieve PPI definitions (name/value pairs)
    def _GetPpi(self):
        if self._Ppis == None:
            #
            # tdict is a special kind of dict, used for selecting correct
            # PPI defition for given ARCH
            #
            PpiDict = tdict(True)
            NameList = []
            # find out all PPI definitions for specific arch and 'common' arch
            RecordList = self._RawData[MODEL_EFI_PPI, self._Arch]
            for Name, Guid, Dummy, Arch, ID, LineNo in RecordList:
                if Name not in NameList:
                    NameList.append(Name)
                PpiDict[Arch, Name] = Guid
            # use sdict to keep the order
            self._Ppis = sdict()
            for Name in NameList:
                #
                # limit the ARCH to self._Arch, if no self._Arch found, tdict
                # will automatically turn to 'common' ARCH for trying
                #
                self._Ppis[Name] = PpiDict[self._Arch, Name]
        return self._Ppis

    ## Retrieve GUID definitions (name/value pairs)
    def _GetGuid(self):
        if self._Guids == None:
            #
            # tdict is a special kind of dict, used for selecting correct
            # GUID defition for given ARCH
            #
            GuidDict = tdict(True)
            NameList = []
            # find out all protocol definitions for specific and 'common' arch
            RecordList = self._RawData[MODEL_EFI_GUID, self._Arch]
            for Name, Guid, Dummy, Arch, ID, LineNo in RecordList:
                if Name not in NameList:
                    NameList.append(Name)
                GuidDict[Arch, Name] = Guid
            # use sdict to keep the order
            self._Guids = sdict()
            for Name in NameList:
                #
                # limit the ARCH to self._Arch, if no self._Arch found, tdict
                # will automatically turn to 'common' ARCH for trying
                #
                self._Guids[Name] = GuidDict[self._Arch, Name]
        return self._Guids

    ## Retrieve public include paths declared in this package
    def _GetInclude(self):
        if self._Includes == None:
            self._Includes = []
            RecordList = self._RawData[MODEL_EFI_INCLUDE, self._Arch]
            Macros = self._Macros
            Macros["EDK_SOURCE"] = GlobalData.gEcpSource
            for Record in RecordList:
                File = PathClass(NormPath(Record[0], Macros), self._PackageDir, Arch=self._Arch)
                LineNo = Record[-1]
                # validate the path
                ErrorCode, ErrorInfo = File.Validate()
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, ExtraData=ErrorInfo, File=self.MetaFile, Line=LineNo)

                # avoid duplicate include path
                if File not in self._Includes:
                    self._Includes.append(File)
        return self._Includes

    ## Retrieve library class declarations (not used in build at present)
    def _GetLibraryClass(self):
        if self._LibraryClasses == None:
            #
            # tdict is a special kind of dict, used for selecting correct
            # library class declaration for given ARCH
            #
            LibraryClassDict = tdict(True)
            LibraryClassSet = set()
            RecordList = self._RawData[MODEL_EFI_LIBRARY_CLASS, self._Arch]
            Macros = self._Macros
            for LibraryClass, File, Dummy, Arch, ID, LineNo in RecordList:
                File = PathClass(NormPath(File, Macros), self._PackageDir, Arch=self._Arch)
                # check the file validation
                ErrorCode, ErrorInfo = File.Validate()
                if ErrorCode != 0:
                    EdkLogger.error('build', ErrorCode, ExtraData=ErrorInfo, File=self.MetaFile, Line=LineNo)
                LibraryClassSet.add(LibraryClass)
                LibraryClassDict[Arch, LibraryClass] = File
            self._LibraryClasses = sdict()
            for LibraryClass in LibraryClassSet:
                self._LibraryClasses[LibraryClass] = LibraryClassDict[self._Arch, LibraryClass]
        return self._LibraryClasses

    ## Retrieve PCD declarations
    def _GetPcds(self):
        if self._Pcds == None:
            self._Pcds = sdict()
            self._Pcds.update(self._GetPcd(MODEL_PCD_FIXED_AT_BUILD))
            self._Pcds.update(self._GetPcd(MODEL_PCD_PATCHABLE_IN_MODULE))
            self._Pcds.update(self._GetPcd(MODEL_PCD_FEATURE_FLAG))
            self._Pcds.update(self._GetPcd(MODEL_PCD_DYNAMIC))
            self._Pcds.update(self._GetPcd(MODEL_PCD_DYNAMIC_EX))
        return self._Pcds

    ## Retrieve PCD declarations for given type
    def _GetPcd(self, Type):
        Pcds = sdict()
        #
        # tdict is a special kind of dict, used for selecting correct
        # PCD declaration for given ARCH
        #
        PcdDict = tdict(True, 3)
        # for summarizing PCD
        PcdSet = set()
        # find out all PCDs of the 'type'
        RecordList = self._RawData[Type, self._Arch]
        for TokenSpaceGuid, PcdCName, Setting, Arch, Dummy1, Dummy2 in RecordList:
            PcdDict[Arch, PcdCName, TokenSpaceGuid] = Setting
            PcdSet.add((PcdCName, TokenSpaceGuid))

        for PcdCName, TokenSpaceGuid in PcdSet:
            #
            # limit the ARCH to self._Arch, if no self._Arch found, tdict
            # will automatically turn to 'common' ARCH and try again
            #
            Setting = PcdDict[self._Arch, PcdCName, TokenSpaceGuid]
            if Setting == None:
                continue

            DefaultValue, DatumType, TokenNumber = AnalyzePcdData(Setting)
                                       
            validateranges, validlists, expressions = self._RawData.GetValidExpression(TokenSpaceGuid, PcdCName)                          
            Pcds[PcdCName, TokenSpaceGuid, self._PCD_TYPE_STRING_[Type]] = PcdClassObject(
                                                                            PcdCName,
                                                                            TokenSpaceGuid,
                                                                            self._PCD_TYPE_STRING_[Type],
                                                                            DatumType,
                                                                            DefaultValue,
                                                                            TokenNumber,
                                                                            '',
                                                                            {},
                                                                            False,
                                                                            None,
                                                                            list(validateranges),
                                                                            list(validlists),
                                                                            list(expressions)
                                                                            )
        return Pcds


    _Macros         = property(_GetMacros)
    Arch            = property(_GetArch, _SetArch)
    PackageName     = property(_GetPackageName)
    Guid            = property(_GetFileGuid)
    Version         = property(_GetVersion)

    Protocols       = property(_GetProtocol)
    Ppis            = property(_GetPpi)
    Guids           = property(_GetGuid)
    Includes        = property(_GetInclude)
    LibraryClasses  = property(_GetLibraryClass)
    Pcds            = property(_GetPcds)

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
        #TAB_INF_DEFINES_INF_VERSION                 : "_AutoGenVersion",
        TAB_INF_DEFINES_COMPONENT_TYPE              : "_ComponentType",
        TAB_INF_DEFINES_MAKEFILE_NAME               : "_MakefileName",
        #TAB_INF_DEFINES_CUSTOM_MAKEFILE             : "_CustomMakefile",
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
        self._ProtocolComments = None
        self._Ppis                  = None
        self._PpiComments = None
        self._Guids                 = None
        self._GuidsUsedByPcd = sdict()
        self._GuidComments = None
        self._Includes              = None
        self._Packages              = None
        self._Pcds                  = None
        self._PcdComments = None
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
                                "MODULE_TYPE %s is not supported for EDK II, valid values are:\n %s" % (self._ModuleType,' '.join(l for l in SUP_MODULE_LIST)), 
                                File=self.MetaFile, Line=LineNo)
            if (self._Specification == None) or (not 'PI_SPECIFICATION_VERSION' in self._Specification) or (int(self._Specification['PI_SPECIFICATION_VERSION'], 16) < 0x0001000A):
                if self._ModuleType == SUP_MODULE_SMM_CORE:
                    EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "SMM_CORE module type can't be used in the module with PI_SPECIFICATION_VERSION less than 0x0001000A", File=self.MetaFile)                
            if self._Defs and 'PCI_DEVICE_ID' in self._Defs and 'PCI_VENDOR_ID' in self._Defs \
               and 'PCI_CLASS_CODE' in self._Defs:
                self._BuildType = 'UEFI_OPTIONROM'
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
            for Name,Value,Dummy,Arch,Platform,ID,LineNo in RecordList:
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
                        ToolChainFamily = 'MSFT'    # Edk.x only support MSFT tool chain
                        #ignore not replaced macros in value
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
        #Ignore all source files in a binary build mode
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
                    # old module source files (Edk)
                    File = PathClass(NormPath(Record[0], Macros), self._ModuleDir, self._SourceOverridePath,
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
                Value = ProtocolValue(CName, self.Packages)
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
                Value = PpiValue(CName, self.Packages)
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
                Value = GuidValue(CName, self.Packages)
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

                    #TRICK: let compiler to choose correct header file
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
                    elif Token.endswith(".inf"):    # module file name
                        ModuleFile = os.path.normpath(Token)
                        Module = self.BuildDatabase[ModuleFile]
                        if Module == None:
                            EdkLogger.error('build', RESOURCE_NOT_AVAILABLE, "Module is not found in active platform",
                                            ExtraData=Token, File=self.MetaFile, Line=Record[-1])
                        DepexList.append(Module.Guid)
                    else:
                        # get the GUID value now
                        Value = ProtocolValue(Token, self.Packages)
                        if Value == None:
                            Value = PpiValue(Token, self.Packages)
                            if Value == None:
                                Value = GuidValue(Token, self.Packages)
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
                Value = GuidValue(TokenSpaceGuid, self.Packages)
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
        for PcdCName, TokenSpaceGuid in PcdList:
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

            # get necessary info from package declaring this PCD
            for Package in self.Packages:
                #
                # 'dynamic' in INF means its type is determined by platform;
                # if platform doesn't give its type, use 'lowest' one in the
                # following order, if any
                #
                #   "FixedAtBuild", "PatchableInModule", "FeatureFlag", "Dynamic", "DynamicEx"
                #
                PcdType = self._PCD_TYPE_STRING_[Type]
                if Type == MODEL_PCD_DYNAMIC:
                    Pcd.Pending = True
                    for T in ["FixedAtBuild", "PatchableInModule", "FeatureFlag", "Dynamic", "DynamicEx"]:
                        if (PcdCName, TokenSpaceGuid, T) in Package.Pcds:
                            PcdType = T
                            break
                else:
                    Pcd.Pending = False

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
                                "No TokenValue for PCD [%s.%s] in [%s]!" % (TokenSpaceGuid, PcdCName, str(Package)),
                                File =self.MetaFile, Line=LineNo,
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
                                    "The format of TokenValue [%s] of PCD [%s.%s] in [%s] is invalid:" % (Pcd.TokenValue, TokenSpaceGuid, PcdCName, str(Package)),
                                    File =self.MetaFile, Line=LineNo,
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
                                            "The format of TokenValue [%s] of PCD [%s.%s] in [%s] is invalid, as a decimal it should between: 0 - 4294967295!"% (Pcd.TokenValue, TokenSpaceGuid, PcdCName, str(Package)),
                                            File =self.MetaFile, Line=LineNo,
                                            ExtraData=None
                                            )                                
                        except:
                            EdkLogger.error(
                                        'build',
                                        FORMAT_INVALID,
                                        "The format of TokenValue [%s] of PCD [%s.%s] in [%s] is invalid, it should be hexadecimal or decimal!"% (Pcd.TokenValue, TokenSpaceGuid, PcdCName, str(Package)),
                                        File =self.MetaFile, Line=LineNo,
                                        ExtraData=None
                                        )
                    
                    Pcd.DatumType = PcdInPackage.DatumType
                    Pcd.MaxDatumSize = PcdInPackage.MaxDatumSize
                    Pcd.InfDefaultValue = Pcd.DefaultValue
                    if Pcd.DefaultValue in [None, '']:
                        Pcd.DefaultValue = PcdInPackage.DefaultValue
                    break
            else:
                EdkLogger.error(
                            'build',
                            FORMAT_INVALID,
                            "PCD [%s.%s] in [%s] is not found in dependent packages:" % (TokenSpaceGuid, PcdCName, self.MetaFile),
                            File =self.MetaFile, Line=LineNo,
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
    ProtocolComments = property(_GetProtocolComments)
    Ppis                    = property(_GetPpis)
    PpiComments = property(_GetPpiComments)
    Guids                   = property(_GetGuids)
    GuidComments = property(_GetGuidComments)
    Includes                = property(_GetIncludes)
    Packages                = property(_GetPackages)
    Pcds                    = property(_GetPcds)
    PcdComments = property(_GetPcdComments)
    BuildOptions            = property(_GetBuildOptions)
    Depex                   = property(_GetDepex)
    DepexExpression         = property(_GetDepexExpression)
    IsBinaryModule = property(_IsBinaryModule)
    IsSupportedArch = property(_IsSupportedArch)

## Database
#
#   This class defined the build database for all modules, packages and platform.
# It will call corresponding parser for the given file if it cannot find it in
# the database.
#
# @param DbPath             Path of database file
# @param GlobalMacros       Global macros used for replacement during file parsing
# @prarm RenewDb=False      Create new database file if it's already there
#
class WorkspaceDatabase(object):


    #
    # internal class used for call corresponding file parser and caching the result
    # to avoid unnecessary re-parsing
    #
    class BuildObjectFactory(object):

        _FILE_TYPE_ = {
            ".inf"  : MODEL_FILE_INF,
            ".dec"  : MODEL_FILE_DEC,
            ".dsc"  : MODEL_FILE_DSC,
        }

        # file parser
        _FILE_PARSER_ = {
            MODEL_FILE_INF  :   InfParser,
            MODEL_FILE_DEC  :   DecParser,
            MODEL_FILE_DSC  :   DscParser,
        }

        # convert to xxxBuildData object
        _GENERATOR_ = {
            MODEL_FILE_INF  :   InfBuildData,
            MODEL_FILE_DEC  :   DecBuildData,
            MODEL_FILE_DSC  :   DscBuildData,
        }

        _CACHE_ = {}    # (FilePath, Arch)  : <object>

        # constructor
        def __init__(self, WorkspaceDb):
            self.WorkspaceDb = WorkspaceDb

        # key = (FilePath, Arch=None)
        def __contains__(self, Key):
            FilePath = Key[0]
            if len(Key) > 1:
                Arch = Key[1]
            else:
                Arch = None
            return (FilePath, Arch) in self._CACHE_

        # key = (FilePath, Arch=None, Target=None, Toochain=None)
        def __getitem__(self, Key):
            FilePath = Key[0]
            KeyLength = len(Key)
            if KeyLength > 1:
                Arch = Key[1]
            else:
                Arch = None
            if KeyLength > 2:
                Target = Key[2]
            else:
                Target = None
            if KeyLength > 3:
                Toolchain = Key[3]
            else:
                Toolchain = None

            # if it's generated before, just return the cached one
            Key = (FilePath, Arch, Target, Toolchain)
            if Key in self._CACHE_:
                return self._CACHE_[Key]

            # check file type
            Ext = FilePath.Type
            if Ext not in self._FILE_TYPE_:
                return None
            FileType = self._FILE_TYPE_[Ext]
            if FileType not in self._GENERATOR_:
                return None

            # get the parser ready for this file
            MetaFile = self._FILE_PARSER_[FileType](
                                FilePath, 
                                FileType, 
                                MetaFileStorage(self.WorkspaceDb.Cur, FilePath, FileType)
                                )
            # alwasy do post-process, in case of macros change
            MetaFile.DoPostProcess()
            # object the build is based on
            BuildObject = self._GENERATOR_[FileType](
                                    FilePath,
                                    MetaFile,
                                    self,
                                    Arch,
                                    Target,
                                    Toolchain
                                    )
            self._CACHE_[Key] = BuildObject
            return BuildObject

    # placeholder for file format conversion
    class TransformObjectFactory:
        def __init__(self, WorkspaceDb):
            self.WorkspaceDb = WorkspaceDb

        # key = FilePath, Arch
        def __getitem__(self, Key):
            pass

    ## Constructor of WorkspaceDatabase
    #
    # @param DbPath             Path of database file
    # @param GlobalMacros       Global macros used for replacement during file parsing
    # @prarm RenewDb=False      Create new database file if it's already there
    #
    def __init__(self, DbPath, RenewDb=False):
        self._DbClosedFlag = False
        if not DbPath:
            DbPath = os.path.normpath(os.path.join(GlobalData.gWorkspace, 'Conf', GlobalData.gDatabasePath))

        # don't create necessary path for db in memory
        if DbPath != ':memory:':
            DbDir = os.path.split(DbPath)[0]
            if not os.path.exists(DbDir):
                os.makedirs(DbDir)

            # remove db file in case inconsistency between db and file in file system
            if self._CheckWhetherDbNeedRenew(RenewDb, DbPath):
                os.remove(DbPath)
        
        # create db with optimized parameters
        self.Conn = sqlite3.connect(DbPath, isolation_level='DEFERRED')
        self.Conn.execute("PRAGMA synchronous=OFF")
        self.Conn.execute("PRAGMA temp_store=MEMORY")
        self.Conn.execute("PRAGMA count_changes=OFF")
        self.Conn.execute("PRAGMA cache_size=8192")
        #self.Conn.execute("PRAGMA page_size=8192")

        # to avoid non-ascii character conversion issue
        self.Conn.text_factory = str
        self.Cur = self.Conn.cursor()

        # create table for internal uses
        self.TblDataModel = TableDataModel(self.Cur)
        self.TblFile = TableFile(self.Cur)
        self.Platform = None

        # conversion object for build or file format conversion purpose
        self.BuildObject = WorkspaceDatabase.BuildObjectFactory(self)
        self.TransformObject = WorkspaceDatabase.TransformObjectFactory(self)

    ## Check whether workspace database need to be renew.
    #  The renew reason maybe:
    #  1) If user force to renew;
    #  2) If user do not force renew, and
    #     a) If the time of last modified python source is newer than database file;
    #     b) If the time of last modified frozen executable file is newer than database file;
    #
    #  @param force     User force renew database
    #  @param DbPath    The absolute path of workspace database file
    #
    #  @return Bool value for whether need renew workspace databse
    #
    def _CheckWhetherDbNeedRenew (self, force, DbPath):
        # if database does not exist, we need do nothing
        if not os.path.exists(DbPath): return False
            
        # if user force to renew database, then not check whether database is out of date
        if force: return True
        
        #    
        # Check the time of last modified source file or build.exe
        # if is newer than time of database, then database need to be re-created.
        #
        timeOfToolModified = 0
        if hasattr(sys, "frozen"):
            exePath             = os.path.abspath(sys.executable)
            timeOfToolModified  = os.stat(exePath).st_mtime
        else:
            curPath  = os.path.dirname(__file__) # curPath is the path of WorkspaceDatabase.py
            rootPath = os.path.split(curPath)[0] # rootPath is root path of python source, such as /BaseTools/Source/Python
            if rootPath == "" or rootPath == None:
                EdkLogger.verbose("\nFail to find the root path of build.exe or python sources, so can not \
determine whether database file is out of date!\n")
        
            # walk the root path of source or build's binary to get the time last modified.
        
            for root, dirs, files in os.walk (rootPath):
                for dir in dirs:
                    # bypass source control folder 
                    if dir.lower() in [".svn", "_svn", "cvs"]:
                        dirs.remove(dir)
                        
                for file in files:
                    ext = os.path.splitext(file)[1]
                    if ext.lower() == ".py":            # only check .py files
                        fd = os.stat(os.path.join(root, file))
                        if timeOfToolModified < fd.st_mtime:
                            timeOfToolModified = fd.st_mtime
        if timeOfToolModified > os.stat(DbPath).st_mtime:
            EdkLogger.verbose("\nWorkspace database is out of data!")
            return True
            
        return False
            
    ## Initialize build database
    def InitDatabase(self):
        EdkLogger.verbose("\nInitialize build database started ...")

        #
        # Create new tables
        #
        self.TblDataModel.Create(False)
        self.TblFile.Create(False)

        #
        # Initialize table DataModel
        #
        self.TblDataModel.InitTable()
        EdkLogger.verbose("Initialize build database ... DONE!")

    ## Query a table
    #
    # @param Table:  The instance of the table to be queried
    #
    def QueryTable(self, Table):
        Table.Query()

    def __del__(self):
        self.Close()

    ## Close entire database
    #
    # Commit all first
    # Close the connection and cursor
    #
    def Close(self):
        if not self._DbClosedFlag:
            self.Conn.commit()
            self.Cur.close()
            self.Conn.close()
            self._DbClosedFlag = True

    ## Summarize all packages in the database
    def GetPackageList(self, Platform, Arch, TargetName, ToolChainTag):
        self.Platform = Platform
        PackageList =[]
        Pa = self.BuildObject[self.Platform, 'COMMON']
        #
        # Get Package related to Modules
        #
        for Module in Pa.Modules:
            ModuleObj = self.BuildObject[Module, Arch, TargetName, ToolChainTag]
            for Package in ModuleObj.Packages:
                if Package not in PackageList:
                    PackageList.append(Package)
        #
        # Get Packages related to Libraries
        #
        for Lib in Pa.LibraryInstances:
            LibObj = self.BuildObject[Lib, Arch, TargetName, ToolChainTag]
            for Package in LibObj.Packages:
                if Package not in PackageList:
                    PackageList.append(Package)            
        
        return PackageList

    ## Summarize all platforms in the database
    def _GetPlatformList(self):
        PlatformList = []
        for PlatformFile in self.TblFile.GetFileList(MODEL_FILE_DSC):
            try:
                Platform = self.BuildObject[PathClass(PlatformFile), 'COMMON']
            except:
                Platform = None
            if Platform != None:
                PlatformList.append(Platform)
        return PlatformList

    PlatformList = property(_GetPlatformList)

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    pass

