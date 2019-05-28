## @file
# Generate AutoGen.h, AutoGen.c and *.depex files
#
# Copyright (c) 2007 - 2019, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2018, Hewlett Packard Enterprise Development, L.P.<BR>
# Copyright (c) 2019, American Megatrends, Inc. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

## Import Modules
#
from __future__ import print_function
from __future__ import absolute_import
import Common.LongFilePathOs as os
import re
import os.path as path
import copy
import uuid

from . import GenC
from . import GenMake
from . import GenDepex
from io import BytesIO

from .StrGather import *
from .BuildEngine import BuildRule
import shutil
from Common.LongFilePathSupport import CopyLongFilePath
from Common.BuildToolError import *
from Common.DataType import *
from Common.Misc import *
from Common.StringUtils import *
import Common.GlobalData as GlobalData
from GenFds.FdfParser import *
from CommonDataClass.CommonClass import SkuInfoClass
from GenPatchPcdTable.GenPatchPcdTable import parsePcdInfoFromMapFile
import Common.VpdInfoFile as VpdInfoFile
from .GenPcdDb import CreatePcdDatabaseCode
from Workspace.MetaFileCommentParser import UsageList
from Workspace.WorkspaceCommon import GetModuleLibInstances
from Common.MultipleWorkspace import MultipleWorkspace as mws
from . import InfSectionParser
import datetime
import hashlib
from .GenVar import VariableMgr, var_info
from collections import OrderedDict
from collections import defaultdict
from Workspace.WorkspaceCommon import OrderedListDict
from Common.ToolDefClassObject import gDefaultToolsDefFile

from Common.caching import cached_property, cached_class_function

## Regular expression for splitting Dependency Expression string into tokens
gDepexTokenPattern = re.compile("(\(|\)|\w+| \S+\.inf)")

## Regular expression for match: PCD(xxxx.yyy)
gPCDAsGuidPattern = re.compile(r"^PCD\(.+\..+\)$")

#
# Regular expression for finding Include Directories, the difference between MSFT and INTEL/GCC/RVCT
# is the former use /I , the Latter used -I to specify include directories
#
gBuildOptIncludePatternMsft = re.compile(r"(?:.*?)/I[ \t]*([^ ]*)", re.MULTILINE | re.DOTALL)
gBuildOptIncludePatternOther = re.compile(r"(?:.*?)-I[ \t]*([^ ]*)", re.MULTILINE | re.DOTALL)

#
# Match name = variable
#
gEfiVarStoreNamePattern = re.compile("\s*name\s*=\s*(\w+)")
#
# The format of guid in efivarstore statement likes following and must be correct:
# guid = {0xA04A27f4, 0xDF00, 0x4D42, {0xB5, 0x52, 0x39, 0x51, 0x13, 0x02, 0x11, 0x3D}}
#
gEfiVarStoreGuidPattern = re.compile("\s*guid\s*=\s*({.*?{.*?}\s*})")

## Mapping Makefile type
gMakeTypeMap = {TAB_COMPILER_MSFT:"nmake", "GCC":"gmake"}


## Build rule configuration file
gDefaultBuildRuleFile = 'build_rule.txt'

## Build rule default version
AutoGenReqBuildRuleVerNum = "0.1"

## default file name for AutoGen
gAutoGenCodeFileName = "AutoGen.c"
gAutoGenHeaderFileName = "AutoGen.h"
gAutoGenStringFileName = "%(module_name)sStrDefs.h"
gAutoGenStringFormFileName = "%(module_name)sStrDefs.hpk"
gAutoGenDepexFileName = "%(module_name)s.depex"
gAutoGenImageDefFileName = "%(module_name)sImgDefs.h"
gAutoGenIdfFileName = "%(module_name)sIdf.hpk"
gInfSpecVersion = "0x00010017"

#
# Template string to generic AsBuilt INF
#
gAsBuiltInfHeaderString = TemplateString("""${header_comments}

# DO NOT EDIT
# FILE auto-generated

[Defines]
  INF_VERSION                = ${module_inf_version}
  BASE_NAME                  = ${module_name}
  FILE_GUID                  = ${module_guid}
  MODULE_TYPE                = ${module_module_type}${BEGIN}
  VERSION_STRING             = ${module_version_string}${END}${BEGIN}
  PCD_IS_DRIVER              = ${pcd_is_driver_string}${END}${BEGIN}
  UEFI_SPECIFICATION_VERSION = ${module_uefi_specification_version}${END}${BEGIN}
  PI_SPECIFICATION_VERSION   = ${module_pi_specification_version}${END}${BEGIN}
  ENTRY_POINT                = ${module_entry_point}${END}${BEGIN}
  UNLOAD_IMAGE               = ${module_unload_image}${END}${BEGIN}
  CONSTRUCTOR                = ${module_constructor}${END}${BEGIN}
  DESTRUCTOR                 = ${module_destructor}${END}${BEGIN}
  SHADOW                     = ${module_shadow}${END}${BEGIN}
  PCI_VENDOR_ID              = ${module_pci_vendor_id}${END}${BEGIN}
  PCI_DEVICE_ID              = ${module_pci_device_id}${END}${BEGIN}
  PCI_CLASS_CODE             = ${module_pci_class_code}${END}${BEGIN}
  PCI_REVISION               = ${module_pci_revision}${END}${BEGIN}
  BUILD_NUMBER               = ${module_build_number}${END}${BEGIN}
  SPEC                       = ${module_spec}${END}${BEGIN}
  UEFI_HII_RESOURCE_SECTION  = ${module_uefi_hii_resource_section}${END}${BEGIN}
  MODULE_UNI_FILE            = ${module_uni_file}${END}

[Packages.${module_arch}]${BEGIN}
  ${package_item}${END}

[Binaries.${module_arch}]${BEGIN}
  ${binary_item}${END}

[PatchPcd.${module_arch}]${BEGIN}
  ${patchablepcd_item}
${END}

[Protocols.${module_arch}]${BEGIN}
  ${protocol_item}
${END}

[Ppis.${module_arch}]${BEGIN}
  ${ppi_item}
${END}

[Guids.${module_arch}]${BEGIN}
  ${guid_item}
${END}

[PcdEx.${module_arch}]${BEGIN}
  ${pcd_item}
${END}

[LibraryClasses.${module_arch}]
## @LIB_INSTANCES${BEGIN}
#  ${libraryclasses_item}${END}

${depexsection_item}

${userextension_tianocore_item}

${tail_comments}

[BuildOptions.${module_arch}]
## @AsBuilt${BEGIN}
##   ${flags_item}${END}
""")
## Split command line option string to list
#
# subprocess.Popen needs the args to be a sequence. Otherwise there's problem
# in non-windows platform to launch command
#
def _SplitOption(OptionString):
    OptionList = []
    LastChar = " "
    OptionStart = 0
    QuotationMark = ""
    for Index in range(0, len(OptionString)):
        CurrentChar = OptionString[Index]
        if CurrentChar in ['"', "'"]:
            if QuotationMark == CurrentChar:
                QuotationMark = ""
            elif QuotationMark == "":
                QuotationMark = CurrentChar
            continue
        elif QuotationMark:
            continue

        if CurrentChar in ["/", "-"] and LastChar in [" ", "\t", "\r", "\n"]:
            if Index > OptionStart:
                OptionList.append(OptionString[OptionStart:Index - 1])
            OptionStart = Index
        LastChar = CurrentChar
    OptionList.append(OptionString[OptionStart:])
    return OptionList

#
# Convert string to C format array
#
def _ConvertStringToByteArray(Value):
    Value = Value.strip()
    if not Value:
        return None
    if Value[0] == '{':
        if not Value.endswith('}'):
            return None
        Value = Value.replace(' ', '').replace('{', '').replace('}', '')
        ValFields = Value.split(',')
        try:
            for Index in range(len(ValFields)):
                ValFields[Index] = str(int(ValFields[Index], 0))
        except ValueError:
            return None
        Value = '{' + ','.join(ValFields) + '}'
        return Value

    Unicode = False
    if Value.startswith('L"'):
        if not Value.endswith('"'):
            return None
        Value = Value[1:]
        Unicode = True
    elif not Value.startswith('"') or not Value.endswith('"'):
        return None

    Value = eval(Value)         # translate escape character
    NewValue = '{'
    for Index in range(0, len(Value)):
        if Unicode:
            NewValue = NewValue + str(ord(Value[Index]) % 0x10000) + ','
        else:
            NewValue = NewValue + str(ord(Value[Index]) % 0x100) + ','
    Value = NewValue + '0}'
    return Value

## Base class for AutoGen
#
#   This class just implements the cache mechanism of AutoGen objects.
#
class AutoGen(object):
    # database to maintain the objects in each child class
    __ObjectCache = {}    # (BuildTarget, ToolChain, ARCH, platform file): AutoGen object

    ## Factory method
    #
    #   @param  Class           class object of real AutoGen class
    #                           (WorkspaceAutoGen, ModuleAutoGen or PlatformAutoGen)
    #   @param  Workspace       Workspace directory or WorkspaceAutoGen object
    #   @param  MetaFile        The path of meta file
    #   @param  Target          Build target
    #   @param  Toolchain       Tool chain name
    #   @param  Arch            Target arch
    #   @param  *args           The specific class related parameters
    #   @param  **kwargs        The specific class related dict parameters
    #
    def __new__(cls, Workspace, MetaFile, Target, Toolchain, Arch, *args, **kwargs):
        # check if the object has been created
        Key = (Target, Toolchain, Arch, MetaFile)
        if Key in cls.__ObjectCache:
            # if it exists, just return it directly
            return cls.__ObjectCache[Key]
            # it didnt exist. create it, cache it, then return it
        RetVal = cls.__ObjectCache[Key] = super(AutoGen, cls).__new__(cls)
        return RetVal


    ## hash() operator
    #
    #  The file path of platform file will be used to represent hash value of this object
    #
    #   @retval int     Hash value of the file path of platform file
    #
    def __hash__(self):
        return hash(self.MetaFile)

    ## str() operator
    #
    #  The file path of platform file will be used to represent this object
    #
    #   @retval string  String of platform file path
    #
    def __str__(self):
        return str(self.MetaFile)

    ## "==" operator
    def __eq__(self, Other):
        return Other and self.MetaFile == Other

## Workspace AutoGen class
#
#   This class is used mainly to control the whole platform build for different
# architecture. This class will generate top level makefile.
#
class WorkspaceAutoGen(AutoGen):
    # call super().__init__ then call the worker function with different parameter count
    def __init__(self, Workspace, MetaFile, Target, Toolchain, Arch, *args, **kwargs):
        if not hasattr(self, "_Init"):
            self._InitWorker(Workspace, MetaFile, Target, Toolchain, Arch, *args, **kwargs)
            self._Init = True

    ## Initialize WorkspaceAutoGen
    #
    #   @param  WorkspaceDir            Root directory of workspace
    #   @param  ActivePlatform          Meta-file of active platform
    #   @param  Target                  Build target
    #   @param  Toolchain               Tool chain name
    #   @param  ArchList                List of architecture of current build
    #   @param  MetaFileDb              Database containing meta-files
    #   @param  BuildConfig             Configuration of build
    #   @param  ToolDefinition          Tool chain definitions
    #   @param  FlashDefinitionFile     File of flash definition
    #   @param  Fds                     FD list to be generated
    #   @param  Fvs                     FV list to be generated
    #   @param  Caps                    Capsule list to be generated
    #   @param  SkuId                   SKU id from command line
    #
    def _InitWorker(self, WorkspaceDir, ActivePlatform, Target, Toolchain, ArchList, MetaFileDb,
              BuildConfig, ToolDefinition, FlashDefinitionFile='', Fds=None, Fvs=None, Caps=None, SkuId='', UniFlag=None,
              Progress=None, BuildModule=None):
        self.BuildDatabase  = MetaFileDb
        self.MetaFile       = ActivePlatform
        self.WorkspaceDir   = WorkspaceDir
        self.Platform       = self.BuildDatabase[self.MetaFile, TAB_ARCH_COMMON, Target, Toolchain]
        GlobalData.gActivePlatform = self.Platform
        self.BuildTarget    = Target
        self.ToolChain      = Toolchain
        self.ArchList       = ArchList
        self.SkuId          = SkuId
        self.UniFlag        = UniFlag

        self.TargetTxt      = BuildConfig
        self.ToolDef        = ToolDefinition
        self.FdfFile        = FlashDefinitionFile
        self.FdTargetList   = Fds if Fds else []
        self.FvTargetList   = Fvs if Fvs else []
        self.CapTargetList  = Caps if Caps else []
        self.AutoGenObjectList = []
        self._GuidDict = {}

        # there's many relative directory operations, so ...
        os.chdir(self.WorkspaceDir)

        #
        # Merge Arch
        #
        if not self.ArchList:
            ArchList = set(self.Platform.SupArchList)
        else:
            ArchList = set(self.ArchList) & set(self.Platform.SupArchList)
        if not ArchList:
            EdkLogger.error("build", PARAMETER_INVALID,
                            ExtraData = "Invalid ARCH specified. [Valid ARCH: %s]" % (" ".join(self.Platform.SupArchList)))
        elif self.ArchList and len(ArchList) != len(self.ArchList):
            SkippedArchList = set(self.ArchList).symmetric_difference(set(self.Platform.SupArchList))
            EdkLogger.verbose("\nArch [%s] is ignored because the platform supports [%s] only!"
                              % (" ".join(SkippedArchList), " ".join(self.Platform.SupArchList)))
        self.ArchList = tuple(ArchList)

        # Validate build target
        if self.BuildTarget not in self.Platform.BuildTargets:
            EdkLogger.error("build", PARAMETER_INVALID,
                            ExtraData="Build target [%s] is not supported by the platform. [Valid target: %s]"
                                      % (self.BuildTarget, " ".join(self.Platform.BuildTargets)))


        # parse FDF file to get PCDs in it, if any
        if not self.FdfFile:
            self.FdfFile = self.Platform.FlashDefinition

        EdkLogger.info("")
        if self.ArchList:
            EdkLogger.info('%-16s = %s' % ("Architecture(s)", ' '.join(self.ArchList)))
        EdkLogger.info('%-16s = %s' % ("Build target", self.BuildTarget))
        EdkLogger.info('%-16s = %s' % ("Toolchain", self.ToolChain))

        EdkLogger.info('\n%-24s = %s' % ("Active Platform", self.Platform))
        if BuildModule:
            EdkLogger.info('%-24s = %s' % ("Active Module", BuildModule))

        if self.FdfFile:
            EdkLogger.info('%-24s = %s' % ("Flash Image Definition", self.FdfFile))

        EdkLogger.verbose("\nFLASH_DEFINITION = %s" % self.FdfFile)

        if Progress:
            Progress.Start("\nProcessing meta-data")

        if self.FdfFile:
            #
            # Mark now build in AutoGen Phase
            #
            GlobalData.gAutoGenPhase = True
            Fdf = FdfParser(self.FdfFile.Path)
            Fdf.ParseFile()
            GlobalData.gFdfParser = Fdf
            GlobalData.gAutoGenPhase = False
            PcdSet = Fdf.Profile.PcdDict
            if Fdf.CurrentFdName and Fdf.CurrentFdName in Fdf.Profile.FdDict:
                FdDict = Fdf.Profile.FdDict[Fdf.CurrentFdName]
                for FdRegion in FdDict.RegionList:
                    if str(FdRegion.RegionType) is 'FILE' and self.Platform.VpdToolGuid in str(FdRegion.RegionDataList):
                        if int(FdRegion.Offset) % 8 != 0:
                            EdkLogger.error("build", FORMAT_INVALID, 'The VPD Base Address %s must be 8-byte aligned.' % (FdRegion.Offset))
            ModuleList = Fdf.Profile.InfList
            self.FdfProfile = Fdf.Profile
            for fvname in self.FvTargetList:
                if fvname.upper() not in self.FdfProfile.FvDict:
                    EdkLogger.error("build", OPTION_VALUE_INVALID,
                                    "No such an FV in FDF file: %s" % fvname)

            # In DSC file may use FILE_GUID to override the module, then in the Platform.Modules use FILE_GUIDmodule.inf as key,
            # but the path (self.MetaFile.Path) is the real path
            for key in self.FdfProfile.InfDict:
                if key == 'ArchTBD':
                    MetaFile_cache = defaultdict(set)
                    for Arch in self.ArchList:
                        Current_Platform_cache = self.BuildDatabase[self.MetaFile, Arch, Target, Toolchain]
                        for Pkey in Current_Platform_cache.Modules:
                            MetaFile_cache[Arch].add(Current_Platform_cache.Modules[Pkey].MetaFile)
                    for Inf in self.FdfProfile.InfDict[key]:
                        ModuleFile = PathClass(NormPath(Inf), GlobalData.gWorkspace, Arch)
                        for Arch in self.ArchList:
                            if ModuleFile in MetaFile_cache[Arch]:
                                break
                        else:
                            ModuleData = self.BuildDatabase[ModuleFile, Arch, Target, Toolchain]
                            if not ModuleData.IsBinaryModule:
                                EdkLogger.error('build', PARSER_ERROR, "Module %s NOT found in DSC file; Is it really a binary module?" % ModuleFile)

                else:
                    for Arch in self.ArchList:
                        if Arch == key:
                            Platform = self.BuildDatabase[self.MetaFile, Arch, Target, Toolchain]
                            MetaFileList = set()
                            for Pkey in Platform.Modules:
                                MetaFileList.add(Platform.Modules[Pkey].MetaFile)
                            for Inf in self.FdfProfile.InfDict[key]:
                                ModuleFile = PathClass(NormPath(Inf), GlobalData.gWorkspace, Arch)
                                if ModuleFile in MetaFileList:
                                    continue
                                ModuleData = self.BuildDatabase[ModuleFile, Arch, Target, Toolchain]
                                if not ModuleData.IsBinaryModule:
                                    EdkLogger.error('build', PARSER_ERROR, "Module %s NOT found in DSC file; Is it really a binary module?" % ModuleFile)

        else:
            PcdSet = {}
            ModuleList = []
            self.FdfProfile = None
            if self.FdTargetList:
                EdkLogger.info("No flash definition file found. FD [%s] will be ignored." % " ".join(self.FdTargetList))
                self.FdTargetList = []
            if self.FvTargetList:
                EdkLogger.info("No flash definition file found. FV [%s] will be ignored." % " ".join(self.FvTargetList))
                self.FvTargetList = []
            if self.CapTargetList:
                EdkLogger.info("No flash definition file found. Capsule [%s] will be ignored." % " ".join(self.CapTargetList))
                self.CapTargetList = []

        # apply SKU and inject PCDs from Flash Definition file
        for Arch in self.ArchList:
            Platform = self.BuildDatabase[self.MetaFile, Arch, Target, Toolchain]
            PlatformPcds = Platform.Pcds
            self._GuidDict = Platform._GuidDict
            SourcePcdDict = {TAB_PCDS_DYNAMIC_EX:set(), TAB_PCDS_PATCHABLE_IN_MODULE:set(),TAB_PCDS_DYNAMIC:set(),TAB_PCDS_FIXED_AT_BUILD:set()}
            BinaryPcdDict = {TAB_PCDS_DYNAMIC_EX:set(), TAB_PCDS_PATCHABLE_IN_MODULE:set()}
            SourcePcdDict_Keys = SourcePcdDict.keys()
            BinaryPcdDict_Keys = BinaryPcdDict.keys()

            # generate the SourcePcdDict and BinaryPcdDict
            PGen = PlatformAutoGen(self, self.MetaFile, Target, Toolchain, Arch)
            for BuildData in list(PGen.BuildDatabase._CACHE_.values()):
                if BuildData.Arch != Arch:
                    continue
                if BuildData.MetaFile.Ext == '.inf':
                    for key in BuildData.Pcds:
                        if BuildData.Pcds[key].Pending:
                            if key in Platform.Pcds:
                                PcdInPlatform = Platform.Pcds[key]
                                if PcdInPlatform.Type:
                                    BuildData.Pcds[key].Type = PcdInPlatform.Type
                                    BuildData.Pcds[key].Pending = False

                            if BuildData.MetaFile in Platform.Modules:
                                PlatformModule = Platform.Modules[str(BuildData.MetaFile)]
                                if key in PlatformModule.Pcds:
                                    PcdInPlatform = PlatformModule.Pcds[key]
                                    if PcdInPlatform.Type:
                                        BuildData.Pcds[key].Type = PcdInPlatform.Type
                                        BuildData.Pcds[key].Pending = False
                            else:
                                #Pcd used in Library, Pcd Type from reference module if Pcd Type is Pending
                                if BuildData.Pcds[key].Pending:
                                    MGen = ModuleAutoGen(self, BuildData.MetaFile, Target, Toolchain, Arch, self.MetaFile)
                                    if MGen and MGen.IsLibrary:
                                        if MGen in PGen.LibraryAutoGenList:
                                            ReferenceModules = MGen.ReferenceModules
                                            for ReferenceModule in ReferenceModules:
                                                if ReferenceModule.MetaFile in Platform.Modules:
                                                    RefPlatformModule = Platform.Modules[str(ReferenceModule.MetaFile)]
                                                    if key in RefPlatformModule.Pcds:
                                                        PcdInReferenceModule = RefPlatformModule.Pcds[key]
                                                        if PcdInReferenceModule.Type:
                                                            BuildData.Pcds[key].Type = PcdInReferenceModule.Type
                                                            BuildData.Pcds[key].Pending = False
                                                            break

                        if TAB_PCDS_DYNAMIC_EX in BuildData.Pcds[key].Type:
                            if BuildData.IsBinaryModule:
                                BinaryPcdDict[TAB_PCDS_DYNAMIC_EX].add((BuildData.Pcds[key].TokenCName, BuildData.Pcds[key].TokenSpaceGuidCName))
                            else:
                                SourcePcdDict[TAB_PCDS_DYNAMIC_EX].add((BuildData.Pcds[key].TokenCName, BuildData.Pcds[key].TokenSpaceGuidCName))

                        elif TAB_PCDS_PATCHABLE_IN_MODULE in BuildData.Pcds[key].Type:
                            if BuildData.MetaFile.Ext == '.inf':
                                if BuildData.IsBinaryModule:
                                    BinaryPcdDict[TAB_PCDS_PATCHABLE_IN_MODULE].add((BuildData.Pcds[key].TokenCName, BuildData.Pcds[key].TokenSpaceGuidCName))
                                else:
                                    SourcePcdDict[TAB_PCDS_PATCHABLE_IN_MODULE].add((BuildData.Pcds[key].TokenCName, BuildData.Pcds[key].TokenSpaceGuidCName))

                        elif TAB_PCDS_DYNAMIC in BuildData.Pcds[key].Type:
                            SourcePcdDict[TAB_PCDS_DYNAMIC].add((BuildData.Pcds[key].TokenCName, BuildData.Pcds[key].TokenSpaceGuidCName))
                        elif TAB_PCDS_FIXED_AT_BUILD in BuildData.Pcds[key].Type:
                            SourcePcdDict[TAB_PCDS_FIXED_AT_BUILD].add((BuildData.Pcds[key].TokenCName, BuildData.Pcds[key].TokenSpaceGuidCName))
                else:
                    pass
            #
            # A PCD can only use one type for all source modules
            #
            for i in SourcePcdDict_Keys:
                for j in SourcePcdDict_Keys:
                    if i != j:
                        Intersections = SourcePcdDict[i].intersection(SourcePcdDict[j])
                        if len(Intersections) > 0:
                            EdkLogger.error(
                            'build',
                            FORMAT_INVALID,
                            "Building modules from source INFs, following PCD use %s and %s access method. It must be corrected to use only one access method." % (i, j),
                            ExtraData='\n\t'.join(str(P[1]+'.'+P[0]) for P in Intersections)
                            )

            #
            # intersection the BinaryPCD for Mixed PCD
            #
            for i in BinaryPcdDict_Keys:
                for j in BinaryPcdDict_Keys:
                    if i != j:
                        Intersections = BinaryPcdDict[i].intersection(BinaryPcdDict[j])
                        for item in Intersections:
                            NewPcd1 = (item[0] + '_' + i, item[1])
                            NewPcd2 = (item[0] + '_' + j, item[1])
                            if item not in GlobalData.MixedPcd:
                                GlobalData.MixedPcd[item] = [NewPcd1, NewPcd2]
                            else:
                                if NewPcd1 not in GlobalData.MixedPcd[item]:
                                    GlobalData.MixedPcd[item].append(NewPcd1)
                                if NewPcd2 not in GlobalData.MixedPcd[item]:
                                    GlobalData.MixedPcd[item].append(NewPcd2)

            #
            # intersection the SourcePCD and BinaryPCD for Mixed PCD
            #
            for i in SourcePcdDict_Keys:
                for j in BinaryPcdDict_Keys:
                    if i != j:
                        Intersections = SourcePcdDict[i].intersection(BinaryPcdDict[j])
                        for item in Intersections:
                            NewPcd1 = (item[0] + '_' + i, item[1])
                            NewPcd2 = (item[0] + '_' + j, item[1])
                            if item not in GlobalData.MixedPcd:
                                GlobalData.MixedPcd[item] = [NewPcd1, NewPcd2]
                            else:
                                if NewPcd1 not in GlobalData.MixedPcd[item]:
                                    GlobalData.MixedPcd[item].append(NewPcd1)
                                if NewPcd2 not in GlobalData.MixedPcd[item]:
                                    GlobalData.MixedPcd[item].append(NewPcd2)

            for BuildData in list(PGen.BuildDatabase._CACHE_.values()):
                if BuildData.Arch != Arch:
                    continue
                for key in BuildData.Pcds:
                    for SinglePcd in GlobalData.MixedPcd:
                        if (BuildData.Pcds[key].TokenCName, BuildData.Pcds[key].TokenSpaceGuidCName) == SinglePcd:
                            for item in GlobalData.MixedPcd[SinglePcd]:
                                Pcd_Type = item[0].split('_')[-1]
                                if (Pcd_Type == BuildData.Pcds[key].Type) or (Pcd_Type == TAB_PCDS_DYNAMIC_EX and BuildData.Pcds[key].Type in PCD_DYNAMIC_EX_TYPE_SET) or \
                                   (Pcd_Type == TAB_PCDS_DYNAMIC and BuildData.Pcds[key].Type in PCD_DYNAMIC_TYPE_SET):
                                    Value = BuildData.Pcds[key]
                                    Value.TokenCName = BuildData.Pcds[key].TokenCName + '_' + Pcd_Type
                                    if len(key) == 2:
                                        newkey = (Value.TokenCName, key[1])
                                    elif len(key) == 3:
                                        newkey = (Value.TokenCName, key[1], key[2])
                                    del BuildData.Pcds[key]
                                    BuildData.Pcds[newkey] = Value
                                    break
                            break

            # handle the mixed pcd in FDF file
            for key in PcdSet:
                if key in GlobalData.MixedPcd:
                    Value = PcdSet[key]
                    del PcdSet[key]
                    for item in GlobalData.MixedPcd[key]:
                        PcdSet[item] = Value

            #Collect package set information from INF of FDF
            PkgSet = set()
            for Inf in ModuleList:
                ModuleFile = PathClass(NormPath(Inf), GlobalData.gWorkspace, Arch)
                if ModuleFile in Platform.Modules:
                    continue
                ModuleData = self.BuildDatabase[ModuleFile, Arch, Target, Toolchain]
                PkgSet.update(ModuleData.Packages)
            Pkgs = list(PkgSet) + list(PGen.PackageList)
            DecPcds = set()
            DecPcdsKey = set()
            for Pkg in Pkgs:
                for Pcd in Pkg.Pcds:
                    DecPcds.add((Pcd[0], Pcd[1]))
                    DecPcdsKey.add((Pcd[0], Pcd[1], Pcd[2]))

            Platform.SkuName = self.SkuId
            for Name, Guid,Fileds in PcdSet:
                if (Name, Guid) not in DecPcds:
                    EdkLogger.error(
                        'build',
                        PARSER_ERROR,
                        "PCD (%s.%s) used in FDF is not declared in DEC files." % (Guid, Name),
                        File = self.FdfProfile.PcdFileLineDict[Name, Guid, Fileds][0],
                        Line = self.FdfProfile.PcdFileLineDict[Name, Guid, Fileds][1]
                    )
                else:
                    # Check whether Dynamic or DynamicEx PCD used in FDF file. If used, build break and give a error message.
                    if (Name, Guid, TAB_PCDS_FIXED_AT_BUILD) in DecPcdsKey \
                        or (Name, Guid, TAB_PCDS_PATCHABLE_IN_MODULE) in DecPcdsKey \
                        or (Name, Guid, TAB_PCDS_FEATURE_FLAG) in DecPcdsKey:
                        continue
                    elif (Name, Guid, TAB_PCDS_DYNAMIC) in DecPcdsKey or (Name, Guid, TAB_PCDS_DYNAMIC_EX) in DecPcdsKey:
                        EdkLogger.error(
                                'build',
                                PARSER_ERROR,
                                "Using Dynamic or DynamicEx type of PCD [%s.%s] in FDF file is not allowed." % (Guid, Name),
                                File = self.FdfProfile.PcdFileLineDict[Name, Guid, Fileds][0],
                                Line = self.FdfProfile.PcdFileLineDict[Name, Guid, Fileds][1]
                        )

            Pa = PlatformAutoGen(self, self.MetaFile, Target, Toolchain, Arch)
            #
            # Explicitly collect platform's dynamic PCDs
            #
            Pa.CollectPlatformDynamicPcds()
            Pa.CollectFixedAtBuildPcds()
            self.AutoGenObjectList.append(Pa)

            #
            # Generate Package level hash value
            #
            GlobalData.gPackageHash = {}
            if GlobalData.gUseHashCache:
                for Pkg in Pkgs:
                    self._GenPkgLevelHash(Pkg)

        #
        # Check PCDs token value conflict in each DEC file.
        #
        self._CheckAllPcdsTokenValueConflict()

        #
        # Check PCD type and definition between DSC and DEC
        #
        self._CheckPcdDefineAndType()

        #
        # Create BuildOptions Macro & PCD metafile, also add the Active Platform and FDF file.
        #
        content = 'gCommandLineDefines: '
        content += str(GlobalData.gCommandLineDefines)
        content += TAB_LINE_BREAK
        content += 'BuildOptionPcd: '
        content += str(GlobalData.BuildOptionPcd)
        content += TAB_LINE_BREAK
        content += 'Active Platform: '
        content += str(self.Platform)
        content += TAB_LINE_BREAK
        if self.FdfFile:
            content += 'Flash Image Definition: '
            content += str(self.FdfFile)
            content += TAB_LINE_BREAK
        SaveFileOnChange(os.path.join(self.BuildDir, 'BuildOptions'), content, False)

        #
        # Create PcdToken Number file for Dynamic/DynamicEx Pcd.
        #
        PcdTokenNumber = 'PcdTokenNumber: '
        if Pa.PcdTokenNumber:
            if Pa.DynamicPcdList:
                for Pcd in Pa.DynamicPcdList:
                    PcdTokenNumber += TAB_LINE_BREAK
                    PcdTokenNumber += str((Pcd.TokenCName, Pcd.TokenSpaceGuidCName))
                    PcdTokenNumber += ' : '
                    PcdTokenNumber += str(Pa.PcdTokenNumber[Pcd.TokenCName, Pcd.TokenSpaceGuidCName])
        SaveFileOnChange(os.path.join(self.BuildDir, 'PcdTokenNumber'), PcdTokenNumber, False)

        #
        # Get set of workspace metafiles
        #
        AllWorkSpaceMetaFiles = self._GetMetaFiles(Target, Toolchain, Arch)

        #
        # Retrieve latest modified time of all metafiles
        #
        SrcTimeStamp = 0
        for f in AllWorkSpaceMetaFiles:
            if os.stat(f)[8] > SrcTimeStamp:
                SrcTimeStamp = os.stat(f)[8]
        self._SrcTimeStamp = SrcTimeStamp

        if GlobalData.gUseHashCache:
            m = hashlib.md5()
            for files in AllWorkSpaceMetaFiles:
                if files.endswith('.dec'):
                    continue
                f = open(files, 'rb')
                Content = f.read()
                f.close()
                m.update(Content)
            SaveFileOnChange(os.path.join(self.BuildDir, 'AutoGen.hash'), m.hexdigest(), False)
            GlobalData.gPlatformHash = m.hexdigest()

        #
        # Write metafile list to build directory
        #
        AutoGenFilePath = os.path.join(self.BuildDir, 'AutoGen')
        if os.path.exists (AutoGenFilePath):
            os.remove(AutoGenFilePath)
        if not os.path.exists(self.BuildDir):
            os.makedirs(self.BuildDir)
        with open(os.path.join(self.BuildDir, 'AutoGen'), 'w+') as file:
            for f in AllWorkSpaceMetaFiles:
                print(f, file=file)
        return True

    def _GenPkgLevelHash(self, Pkg):
        if Pkg.PackageName in GlobalData.gPackageHash:
            return

        PkgDir = os.path.join(self.BuildDir, Pkg.Arch, Pkg.PackageName)
        CreateDirectory(PkgDir)
        HashFile = os.path.join(PkgDir, Pkg.PackageName + '.hash')
        m = hashlib.md5()
        # Get .dec file's hash value
        f = open(Pkg.MetaFile.Path, 'rb')
        Content = f.read()
        f.close()
        m.update(Content)
        # Get include files hash value
        if Pkg.Includes:
            for inc in sorted(Pkg.Includes, key=lambda x: str(x)):
                for Root, Dirs, Files in os.walk(str(inc)):
                    for File in sorted(Files):
                        File_Path = os.path.join(Root, File)
                        f = open(File_Path, 'rb')
                        Content = f.read()
                        f.close()
                        m.update(Content)
        SaveFileOnChange(HashFile, m.hexdigest(), False)
        GlobalData.gPackageHash[Pkg.PackageName] = m.hexdigest()

    def _GetMetaFiles(self, Target, Toolchain, Arch):
        AllWorkSpaceMetaFiles = set()
        #
        # add fdf
        #
        if self.FdfFile:
            AllWorkSpaceMetaFiles.add (self.FdfFile.Path)
            for f in GlobalData.gFdfParser.GetAllIncludedFile():
                AllWorkSpaceMetaFiles.add (f.FileName)
        #
        # add dsc
        #
        AllWorkSpaceMetaFiles.add(self.MetaFile.Path)

        #
        # add build_rule.txt & tools_def.txt
        #
        AllWorkSpaceMetaFiles.add(os.path.join(GlobalData.gConfDirectory, gDefaultBuildRuleFile))
        AllWorkSpaceMetaFiles.add(os.path.join(GlobalData.gConfDirectory, gDefaultToolsDefFile))

        # add BuildOption metafile
        #
        AllWorkSpaceMetaFiles.add(os.path.join(self.BuildDir, 'BuildOptions'))

        # add PcdToken Number file for Dynamic/DynamicEx Pcd
        #
        AllWorkSpaceMetaFiles.add(os.path.join(self.BuildDir, 'PcdTokenNumber'))

        for Arch in self.ArchList:
            #
            # add dec
            #
            for Package in PlatformAutoGen(self, self.MetaFile, Target, Toolchain, Arch).PackageList:
                AllWorkSpaceMetaFiles.add(Package.MetaFile.Path)

            #
            # add included dsc
            #
            for filePath in self.BuildDatabase[self.MetaFile, Arch, Target, Toolchain]._RawData.IncludedFiles:
                AllWorkSpaceMetaFiles.add(filePath.Path)

        return AllWorkSpaceMetaFiles

    def _CheckPcdDefineAndType(self):
        PcdTypeSet = {TAB_PCDS_FIXED_AT_BUILD,
            TAB_PCDS_PATCHABLE_IN_MODULE,
            TAB_PCDS_FEATURE_FLAG,
            TAB_PCDS_DYNAMIC,
            TAB_PCDS_DYNAMIC_EX}

        # This dict store PCDs which are not used by any modules with specified arches
        UnusedPcd = OrderedDict()
        for Pa in self.AutoGenObjectList:
            # Key of DSC's Pcds dictionary is PcdCName, TokenSpaceGuid
            for Pcd in Pa.Platform.Pcds:
                PcdType = Pa.Platform.Pcds[Pcd].Type

                # If no PCD type, this PCD comes from FDF
                if not PcdType:
                    continue

                # Try to remove Hii and Vpd suffix
                if PcdType.startswith(TAB_PCDS_DYNAMIC_EX):
                    PcdType = TAB_PCDS_DYNAMIC_EX
                elif PcdType.startswith(TAB_PCDS_DYNAMIC):
                    PcdType = TAB_PCDS_DYNAMIC

                for Package in Pa.PackageList:
                    # Key of DEC's Pcds dictionary is PcdCName, TokenSpaceGuid, PcdType
                    if (Pcd[0], Pcd[1], PcdType) in Package.Pcds:
                        break
                    for Type in PcdTypeSet:
                        if (Pcd[0], Pcd[1], Type) in Package.Pcds:
                            EdkLogger.error(
                                'build',
                                FORMAT_INVALID,
                                "Type [%s] of PCD [%s.%s] in DSC file doesn't match the type [%s] defined in DEC file." \
                                % (Pa.Platform.Pcds[Pcd].Type, Pcd[1], Pcd[0], Type),
                                ExtraData=None
                            )
                            return
                else:
                    UnusedPcd.setdefault(Pcd, []).append(Pa.Arch)

        for Pcd in UnusedPcd:
            EdkLogger.warn(
                'build',
                "The PCD was not specified by any INF module in the platform for the given architecture.\n"
                "\tPCD: [%s.%s]\n\tPlatform: [%s]\n\tArch: %s"
                % (Pcd[1], Pcd[0], os.path.basename(str(self.MetaFile)), str(UnusedPcd[Pcd])),
                ExtraData=None
            )

    def __repr__(self):
        return "%s [%s]" % (self.MetaFile, ", ".join(self.ArchList))

    ## Return the directory to store FV files
    @cached_property
    def FvDir(self):
        return path.join(self.BuildDir, TAB_FV_DIRECTORY)

    ## Return the directory to store all intermediate and final files built
    @cached_property
    def BuildDir(self):
        return self.AutoGenObjectList[0].BuildDir

    ## Return the build output directory platform specifies
    @cached_property
    def OutputDir(self):
        return self.Platform.OutputDirectory

    ## Return platform name
    @cached_property
    def Name(self):
        return self.Platform.PlatformName

    ## Return meta-file GUID
    @cached_property
    def Guid(self):
        return self.Platform.Guid

    ## Return platform version
    @cached_property
    def Version(self):
        return self.Platform.Version

    ## Return paths of tools
    @cached_property
    def ToolDefinition(self):
        return self.AutoGenObjectList[0].ToolDefinition

    ## Return directory of platform makefile
    #
    #   @retval     string  Makefile directory
    #
    @cached_property
    def MakeFileDir(self):
        return self.BuildDir

    ## Return build command string
    #
    #   @retval     string  Build command string
    #
    @cached_property
    def BuildCommand(self):
        # BuildCommand should be all the same. So just get one from platform AutoGen
        return self.AutoGenObjectList[0].BuildCommand

    ## Check the PCDs token value conflict in each DEC file.
    #
    # Will cause build break and raise error message while two PCDs conflict.
    #
    # @return  None
    #
    def _CheckAllPcdsTokenValueConflict(self):
        for Pa in self.AutoGenObjectList:
            for Package in Pa.PackageList:
                PcdList = list(Package.Pcds.values())
                PcdList.sort(key=lambda x: int(x.TokenValue, 0))
                Count = 0
                while (Count < len(PcdList) - 1) :
                    Item = PcdList[Count]
                    ItemNext = PcdList[Count + 1]
                    #
                    # Make sure in the same token space the TokenValue should be unique
                    #
                    if (int(Item.TokenValue, 0) == int(ItemNext.TokenValue, 0)):
                        SameTokenValuePcdList = []
                        SameTokenValuePcdList.append(Item)
                        SameTokenValuePcdList.append(ItemNext)
                        RemainPcdListLength = len(PcdList) - Count - 2
                        for ValueSameCount in range(RemainPcdListLength):
                            if int(PcdList[len(PcdList) - RemainPcdListLength + ValueSameCount].TokenValue, 0) == int(Item.TokenValue, 0):
                                SameTokenValuePcdList.append(PcdList[len(PcdList) - RemainPcdListLength + ValueSameCount])
                            else:
                                break;
                        #
                        # Sort same token value PCD list with TokenGuid and TokenCName
                        #
                        SameTokenValuePcdList.sort(key=lambda x: "%s.%s" % (x.TokenSpaceGuidCName, x.TokenCName))
                        SameTokenValuePcdListCount = 0
                        while (SameTokenValuePcdListCount < len(SameTokenValuePcdList) - 1):
                            Flag = False
                            TemListItem = SameTokenValuePcdList[SameTokenValuePcdListCount]
                            TemListItemNext = SameTokenValuePcdList[SameTokenValuePcdListCount + 1]

                            if (TemListItem.TokenSpaceGuidCName == TemListItemNext.TokenSpaceGuidCName) and (TemListItem.TokenCName != TemListItemNext.TokenCName):
                                for PcdItem in GlobalData.MixedPcd:
                                    if (TemListItem.TokenCName, TemListItem.TokenSpaceGuidCName) in GlobalData.MixedPcd[PcdItem] or \
                                        (TemListItemNext.TokenCName, TemListItemNext.TokenSpaceGuidCName) in GlobalData.MixedPcd[PcdItem]:
                                        Flag = True
                                if not Flag:
                                    EdkLogger.error(
                                                'build',
                                                FORMAT_INVALID,
                                                "The TokenValue [%s] of PCD [%s.%s] is conflict with: [%s.%s] in %s"\
                                                % (TemListItem.TokenValue, TemListItem.TokenSpaceGuidCName, TemListItem.TokenCName, TemListItemNext.TokenSpaceGuidCName, TemListItemNext.TokenCName, Package),
                                                ExtraData=None
                                                )
                            SameTokenValuePcdListCount += 1
                        Count += SameTokenValuePcdListCount
                    Count += 1

                PcdList = list(Package.Pcds.values())
                PcdList.sort(key=lambda x: "%s.%s" % (x.TokenSpaceGuidCName, x.TokenCName))
                Count = 0
                while (Count < len(PcdList) - 1) :
                    Item = PcdList[Count]
                    ItemNext = PcdList[Count + 1]
                    #
                    # Check PCDs with same TokenSpaceGuidCName.TokenCName have same token value as well.
                    #
                    if (Item.TokenSpaceGuidCName == ItemNext.TokenSpaceGuidCName) and (Item.TokenCName == ItemNext.TokenCName) and (int(Item.TokenValue, 0) != int(ItemNext.TokenValue, 0)):
                        EdkLogger.error(
                                    'build',
                                    FORMAT_INVALID,
                                    "The TokenValue [%s] of PCD [%s.%s] in %s defined in two places should be same as well."\
                                    % (Item.TokenValue, Item.TokenSpaceGuidCName, Item.TokenCName, Package),
                                    ExtraData=None
                                    )
                    Count += 1
    ## Generate fds command
    @property
    def GenFdsCommand(self):
        return (GenMake.TopLevelMakefile(self)._TEMPLATE_.Replace(GenMake.TopLevelMakefile(self)._TemplateDict)).strip()

    @property
    def GenFdsCommandDict(self):
        FdsCommandDict = {}
        LogLevel = EdkLogger.GetLevel()
        if LogLevel == EdkLogger.VERBOSE:
            FdsCommandDict["verbose"] = True
        elif LogLevel <= EdkLogger.DEBUG_9:
            FdsCommandDict["debug"] = LogLevel - 1
        elif LogLevel == EdkLogger.QUIET:
            FdsCommandDict["quiet"] = True

        if GlobalData.gEnableGenfdsMultiThread:
            FdsCommandDict["GenfdsMultiThread"] = True
        if GlobalData.gIgnoreSource:
            FdsCommandDict["IgnoreSources"] = True

        FdsCommandDict["OptionPcd"] = []
        for pcd in GlobalData.BuildOptionPcd:
            if pcd[2]:
                pcdname = '.'.join(pcd[0:3])
            else:
                pcdname = '.'.join(pcd[0:2])
            if pcd[3].startswith('{'):
                FdsCommandDict["OptionPcd"].append(pcdname + '=' + 'H' + '"' + pcd[3] + '"')
            else:
                FdsCommandDict["OptionPcd"].append(pcdname + '=' + pcd[3])

        MacroList = []
        # macros passed to GenFds
        MacroDict = {}
        MacroDict.update(GlobalData.gGlobalDefines)
        MacroDict.update(GlobalData.gCommandLineDefines)
        for MacroName in MacroDict:
            if MacroDict[MacroName] != "":
                MacroList.append('"%s=%s"' % (MacroName, MacroDict[MacroName].replace('\\', '\\\\')))
            else:
                MacroList.append('"%s"' % MacroName)
        FdsCommandDict["macro"] = MacroList

        FdsCommandDict["fdf_file"] = [self.FdfFile]
        FdsCommandDict["build_target"] = self.BuildTarget
        FdsCommandDict["toolchain_tag"] = self.ToolChain
        FdsCommandDict["active_platform"] = str(self)

        FdsCommandDict["conf_directory"] = GlobalData.gConfDirectory
        FdsCommandDict["build_architecture_list"] = ','.join(self.ArchList)
        FdsCommandDict["platform_build_directory"] = self.BuildDir

        FdsCommandDict["fd"] = self.FdTargetList
        FdsCommandDict["fv"] = self.FvTargetList
        FdsCommandDict["cap"] = self.CapTargetList
        return FdsCommandDict

    ## Create makefile for the platform and modules in it
    #
    #   @param      CreateDepsMakeFile      Flag indicating if the makefile for
    #                                       modules will be created as well
    #
    def CreateMakeFile(self, CreateDepsMakeFile=False):
        if not CreateDepsMakeFile:
            return
        for Pa in self.AutoGenObjectList:
            Pa.CreateMakeFile(True)

    ## Create autogen code for platform and modules
    #
    #  Since there's no autogen code for platform, this method will do nothing
    #  if CreateModuleCodeFile is set to False.
    #
    #   @param      CreateDepsCodeFile      Flag indicating if creating module's
    #                                       autogen code file or not
    #
    def CreateCodeFile(self, CreateDepsCodeFile=False):
        if not CreateDepsCodeFile:
            return
        for Pa in self.AutoGenObjectList:
            Pa.CreateCodeFile(True)

    ## Create AsBuilt INF file the platform
    #
    def CreateAsBuiltInf(self):
        return


## AutoGen class for platform
#
#  PlatformAutoGen class will process the original information in platform
#  file in order to generate makefile for platform.
#
class PlatformAutoGen(AutoGen):
    # call super().__init__ then call the worker function with different parameter count
    def __init__(self, Workspace, MetaFile, Target, Toolchain, Arch, *args, **kwargs):
        if not hasattr(self, "_Init"):
            self._InitWorker(Workspace, MetaFile, Target, Toolchain, Arch)
            self._Init = True
    #
    # Used to store all PCDs for both PEI and DXE phase, in order to generate
    # correct PCD database
    #
    _DynaPcdList_ = []
    _NonDynaPcdList_ = []
    _PlatformPcds = {}

    #
    # The priority list while override build option
    #
    PrioList = {"0x11111"  : 16,     #  TARGET_TOOLCHAIN_ARCH_COMMANDTYPE_ATTRIBUTE (Highest)
                "0x01111"  : 15,     #  ******_TOOLCHAIN_ARCH_COMMANDTYPE_ATTRIBUTE
                "0x10111"  : 14,     #  TARGET_*********_ARCH_COMMANDTYPE_ATTRIBUTE
                "0x00111"  : 13,     #  ******_*********_ARCH_COMMANDTYPE_ATTRIBUTE
                "0x11011"  : 12,     #  TARGET_TOOLCHAIN_****_COMMANDTYPE_ATTRIBUTE
                "0x01011"  : 11,     #  ******_TOOLCHAIN_****_COMMANDTYPE_ATTRIBUTE
                "0x10011"  : 10,     #  TARGET_*********_****_COMMANDTYPE_ATTRIBUTE
                "0x00011"  : 9,      #  ******_*********_****_COMMANDTYPE_ATTRIBUTE
                "0x11101"  : 8,      #  TARGET_TOOLCHAIN_ARCH_***********_ATTRIBUTE
                "0x01101"  : 7,      #  ******_TOOLCHAIN_ARCH_***********_ATTRIBUTE
                "0x10101"  : 6,      #  TARGET_*********_ARCH_***********_ATTRIBUTE
                "0x00101"  : 5,      #  ******_*********_ARCH_***********_ATTRIBUTE
                "0x11001"  : 4,      #  TARGET_TOOLCHAIN_****_***********_ATTRIBUTE
                "0x01001"  : 3,      #  ******_TOOLCHAIN_****_***********_ATTRIBUTE
                "0x10001"  : 2,      #  TARGET_*********_****_***********_ATTRIBUTE
                "0x00001"  : 1}      #  ******_*********_****_***********_ATTRIBUTE (Lowest)

    ## Initialize PlatformAutoGen
    #
    #
    #   @param      Workspace       WorkspaceAutoGen object
    #   @param      PlatformFile    Platform file (DSC file)
    #   @param      Target          Build target (DEBUG, RELEASE)
    #   @param      Toolchain       Name of tool chain
    #   @param      Arch            arch of the platform supports
    #
    def _InitWorker(self, Workspace, PlatformFile, Target, Toolchain, Arch):
        EdkLogger.debug(EdkLogger.DEBUG_9, "AutoGen platform [%s] [%s]" % (PlatformFile, Arch))
        GlobalData.gProcessingFile = "%s [%s, %s, %s]" % (PlatformFile, Arch, Toolchain, Target)

        self.MetaFile = PlatformFile
        self.Workspace = Workspace
        self.WorkspaceDir = Workspace.WorkspaceDir
        self.ToolChain = Toolchain
        self.BuildTarget = Target
        self.Arch = Arch
        self.SourceDir = PlatformFile.SubDir
        self.FdTargetList = self.Workspace.FdTargetList
        self.FvTargetList = self.Workspace.FvTargetList
        # get the original module/package/platform objects
        self.BuildDatabase = Workspace.BuildDatabase
        self.DscBuildDataObj = Workspace.Platform

        # flag indicating if the makefile/C-code file has been created or not
        self.IsMakeFileCreated  = False

        self._DynamicPcdList = None    # [(TokenCName1, TokenSpaceGuidCName1), (TokenCName2, TokenSpaceGuidCName2), ...]
        self._NonDynamicPcdList = None # [(TokenCName1, TokenSpaceGuidCName1), (TokenCName2, TokenSpaceGuidCName2), ...]

        self._AsBuildInfList = []
        self._AsBuildModuleList = []

        self.VariableInfo = None

        if GlobalData.gFdfParser is not None:
            self._AsBuildInfList = GlobalData.gFdfParser.Profile.InfList
            for Inf in self._AsBuildInfList:
                InfClass = PathClass(NormPath(Inf), GlobalData.gWorkspace, self.Arch)
                M = self.BuildDatabase[InfClass, self.Arch, self.BuildTarget, self.ToolChain]
                if not M.IsBinaryModule:
                    continue
                self._AsBuildModuleList.append(InfClass)
        # get library/modules for build
        self.LibraryBuildDirectoryList = []
        self.ModuleBuildDirectoryList = []

        return True

    @cached_class_function
    def __repr__(self):
        return "%s [%s]" % (self.MetaFile, self.Arch)

    ## Create autogen code for platform and modules
    #
    #  Since there's no autogen code for platform, this method will do nothing
    #  if CreateModuleCodeFile is set to False.
    #
    #   @param      CreateModuleCodeFile    Flag indicating if creating module's
    #                                       autogen code file or not
    #
    @cached_class_function
    def CreateCodeFile(self, CreateModuleCodeFile=False):
        # only module has code to be created, so do nothing if CreateModuleCodeFile is False
        if not CreateModuleCodeFile:
            return

        for Ma in self.ModuleAutoGenList:
            Ma.CreateCodeFile(True)

    ## Generate Fds Command
    @cached_property
    def GenFdsCommand(self):
        return self.Workspace.GenFdsCommand

    ## Create makefile for the platform and modules in it
    #
    #   @param      CreateModuleMakeFile    Flag indicating if the makefile for
    #                                       modules will be created as well
    #
    def CreateMakeFile(self, CreateModuleMakeFile=False, FfsCommand = {}):
        if CreateModuleMakeFile:
            for Ma in self._MaList:
                key = (Ma.MetaFile.File, self.Arch)
                if key in FfsCommand:
                    Ma.CreateMakeFile(True, FfsCommand[key])
                else:
                    Ma.CreateMakeFile(True)

        # no need to create makefile for the platform more than once
        if self.IsMakeFileCreated:
            return

        # create library/module build dirs for platform
        Makefile = GenMake.PlatformMakefile(self)
        self.LibraryBuildDirectoryList = Makefile.GetLibraryBuildDirectoryList()
        self.ModuleBuildDirectoryList = Makefile.GetModuleBuildDirectoryList()

        self.IsMakeFileCreated = True

    @property
    def AllPcdList(self):
        return self.DynamicPcdList + self.NonDynamicPcdList
    ## Deal with Shared FixedAtBuild Pcds
    #
    def CollectFixedAtBuildPcds(self):
        for LibAuto in self.LibraryAutoGenList:
            FixedAtBuildPcds = {}
            ShareFixedAtBuildPcdsSameValue = {}
            for Module in LibAuto.ReferenceModules:
                for Pcd in set(Module.FixedAtBuildPcds + LibAuto.FixedAtBuildPcds):
                    DefaultValue = Pcd.DefaultValue
                    # Cover the case: DSC component override the Pcd value and the Pcd only used in one Lib
                    if Pcd in Module.LibraryPcdList:
                        Index = Module.LibraryPcdList.index(Pcd)
                        DefaultValue = Module.LibraryPcdList[Index].DefaultValue
                    key = ".".join((Pcd.TokenSpaceGuidCName, Pcd.TokenCName))
                    if key not in FixedAtBuildPcds:
                        ShareFixedAtBuildPcdsSameValue[key] = True
                        FixedAtBuildPcds[key] = DefaultValue
                    else:
                        if FixedAtBuildPcds[key] != DefaultValue:
                            ShareFixedAtBuildPcdsSameValue[key] = False
            for Pcd in LibAuto.FixedAtBuildPcds:
                key = ".".join((Pcd.TokenSpaceGuidCName, Pcd.TokenCName))
                if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) not in self.NonDynamicPcdDict:
                    continue
                else:
                    DscPcd = self.NonDynamicPcdDict[(Pcd.TokenCName, Pcd.TokenSpaceGuidCName)]
                    if DscPcd.Type != TAB_PCDS_FIXED_AT_BUILD:
                        continue
                if key in ShareFixedAtBuildPcdsSameValue and ShareFixedAtBuildPcdsSameValue[key]:
                    LibAuto.ConstPcd[key] = FixedAtBuildPcds[key]

    def CollectVariables(self, DynamicPcdSet):
        VpdRegionSize = 0
        VpdRegionBase = 0
        if self.Workspace.FdfFile:
            FdDict = self.Workspace.FdfProfile.FdDict[GlobalData.gFdfParser.CurrentFdName]
            for FdRegion in FdDict.RegionList:
                for item in FdRegion.RegionDataList:
                    if self.Platform.VpdToolGuid.strip() and self.Platform.VpdToolGuid in item:
                        VpdRegionSize = FdRegion.Size
                        VpdRegionBase = FdRegion.Offset
                        break

        VariableInfo = VariableMgr(self.DscBuildDataObj._GetDefaultStores(), self.DscBuildDataObj.SkuIds)
        VariableInfo.SetVpdRegionMaxSize(VpdRegionSize)
        VariableInfo.SetVpdRegionOffset(VpdRegionBase)
        Index = 0
        for Pcd in DynamicPcdSet:
            pcdname = ".".join((Pcd.TokenSpaceGuidCName, Pcd.TokenCName))
            for SkuName in Pcd.SkuInfoList:
                Sku = Pcd.SkuInfoList[SkuName]
                SkuId = Sku.SkuId
                if SkuId is None or SkuId == '':
                    continue
                if len(Sku.VariableName) > 0:
                    if Sku.VariableAttribute and 'NV' not in Sku.VariableAttribute:
                        continue
                    VariableGuidStructure = Sku.VariableGuidValue
                    VariableGuid = GuidStructureStringToGuidString(VariableGuidStructure)
                    for StorageName in Sku.DefaultStoreDict:
                        VariableInfo.append_variable(var_info(Index, pcdname, StorageName, SkuName, StringToArray(Sku.VariableName), VariableGuid, Sku.VariableOffset, Sku.VariableAttribute, Sku.HiiDefaultValue, Sku.DefaultStoreDict[StorageName] if Pcd.DatumType in TAB_PCD_NUMERIC_TYPES else StringToArray(Sku.DefaultStoreDict[StorageName]), Pcd.DatumType, Pcd.CustomAttribute['DscPosition'], Pcd.CustomAttribute.get('IsStru',False)))
            Index += 1
        return VariableInfo

    def UpdateNVStoreMaxSize(self, OrgVpdFile):
        if self.VariableInfo:
            VpdMapFilePath = os.path.join(self.BuildDir, TAB_FV_DIRECTORY, "%s.map" % self.Platform.VpdToolGuid)
            PcdNvStoreDfBuffer = [item for item in self._DynamicPcdList if item.TokenCName == "PcdNvStoreDefaultValueBuffer" and item.TokenSpaceGuidCName == "gEfiMdeModulePkgTokenSpaceGuid"]

            if PcdNvStoreDfBuffer:
                if os.path.exists(VpdMapFilePath):
                    OrgVpdFile.Read(VpdMapFilePath)
                    PcdItems = OrgVpdFile.GetOffset(PcdNvStoreDfBuffer[0])
                    NvStoreOffset = list(PcdItems.values())[0].strip() if PcdItems else '0'
                else:
                    EdkLogger.error("build", FILE_READ_FAILURE, "Can not find VPD map file %s to fix up VPD offset." % VpdMapFilePath)

                NvStoreOffset = int(NvStoreOffset, 16) if NvStoreOffset.upper().startswith("0X") else int(NvStoreOffset)
                default_skuobj = PcdNvStoreDfBuffer[0].SkuInfoList.get(TAB_DEFAULT)
                maxsize = self.VariableInfo.VpdRegionSize  - NvStoreOffset if self.VariableInfo.VpdRegionSize else len(default_skuobj.DefaultValue.split(","))
                var_data = self.VariableInfo.PatchNVStoreDefaultMaxSize(maxsize)

                if var_data and default_skuobj:
                    default_skuobj.DefaultValue = var_data
                    PcdNvStoreDfBuffer[0].DefaultValue = var_data
                    PcdNvStoreDfBuffer[0].SkuInfoList.clear()
                    PcdNvStoreDfBuffer[0].SkuInfoList[TAB_DEFAULT] = default_skuobj
                    PcdNvStoreDfBuffer[0].MaxDatumSize = str(len(default_skuobj.DefaultValue.split(",")))

        return OrgVpdFile

    ## Collect dynamic PCDs
    #
    #  Gather dynamic PCDs list from each module and their settings from platform
    #  This interface should be invoked explicitly when platform action is created.
    #
    def CollectPlatformDynamicPcds(self):
        for key in self.Platform.Pcds:
            for SinglePcd in GlobalData.MixedPcd:
                if (self.Platform.Pcds[key].TokenCName, self.Platform.Pcds[key].TokenSpaceGuidCName) == SinglePcd:
                    for item in GlobalData.MixedPcd[SinglePcd]:
                        Pcd_Type = item[0].split('_')[-1]
                        if (Pcd_Type == self.Platform.Pcds[key].Type) or (Pcd_Type == TAB_PCDS_DYNAMIC_EX and self.Platform.Pcds[key].Type in PCD_DYNAMIC_EX_TYPE_SET) or \
                           (Pcd_Type == TAB_PCDS_DYNAMIC and self.Platform.Pcds[key].Type in PCD_DYNAMIC_TYPE_SET):
                            Value = self.Platform.Pcds[key]
                            Value.TokenCName = self.Platform.Pcds[key].TokenCName + '_' + Pcd_Type
                            if len(key) == 2:
                                newkey = (Value.TokenCName, key[1])
                            elif len(key) == 3:
                                newkey = (Value.TokenCName, key[1], key[2])
                            del self.Platform.Pcds[key]
                            self.Platform.Pcds[newkey] = Value
                            break
                    break

        # for gathering error information
        NoDatumTypePcdList = set()
        FdfModuleList = []
        for InfName in self._AsBuildInfList:
            InfName = mws.join(self.WorkspaceDir, InfName)
            FdfModuleList.append(os.path.normpath(InfName))
        for M in self._MaList:
#            F is the Module for which M is the module autogen
            for PcdFromModule in M.ModulePcdList + M.LibraryPcdList:
                # make sure that the "VOID*" kind of datum has MaxDatumSize set
                if PcdFromModule.DatumType == TAB_VOID and not PcdFromModule.MaxDatumSize:
                    NoDatumTypePcdList.add("%s.%s [%s]" % (PcdFromModule.TokenSpaceGuidCName, PcdFromModule.TokenCName, M.MetaFile))

                # Check the PCD from Binary INF or Source INF
                if M.IsBinaryModule == True:
                    PcdFromModule.IsFromBinaryInf = True

                # Check the PCD from DSC or not
                PcdFromModule.IsFromDsc = (PcdFromModule.TokenCName, PcdFromModule.TokenSpaceGuidCName) in self.Platform.Pcds

                if PcdFromModule.Type in PCD_DYNAMIC_TYPE_SET or PcdFromModule.Type in PCD_DYNAMIC_EX_TYPE_SET:
                    if M.MetaFile.Path not in FdfModuleList:
                        # If one of the Source built modules listed in the DSC is not listed
                        # in FDF modules, and the INF lists a PCD can only use the PcdsDynamic
                        # access method (it is only listed in the DEC file that declares the
                        # PCD as PcdsDynamic), then build tool will report warning message
                        # notify the PI that they are attempting to build a module that must
                        # be included in a flash image in order to be functional. These Dynamic
                        # PCD will not be added into the Database unless it is used by other
                        # modules that are included in the FDF file.
                        if PcdFromModule.Type in PCD_DYNAMIC_TYPE_SET and \
                            PcdFromModule.IsFromBinaryInf == False:
                            # Print warning message to let the developer make a determine.
                            continue
                        # If one of the Source built modules listed in the DSC is not listed in
                        # FDF modules, and the INF lists a PCD can only use the PcdsDynamicEx
                        # access method (it is only listed in the DEC file that declares the
                        # PCD as PcdsDynamicEx), then DO NOT break the build; DO NOT add the
                        # PCD to the Platform's PCD Database.
                        if PcdFromModule.Type in PCD_DYNAMIC_EX_TYPE_SET:
                            continue
                    #
                    # If a dynamic PCD used by a PEM module/PEI module & DXE module,
                    # it should be stored in Pcd PEI database, If a dynamic only
                    # used by DXE module, it should be stored in DXE PCD database.
                    # The default Phase is DXE
                    #
                    if M.ModuleType in SUP_MODULE_SET_PEI:
                        PcdFromModule.Phase = "PEI"
                    if PcdFromModule not in self._DynaPcdList_:
                        self._DynaPcdList_.append(PcdFromModule)
                    elif PcdFromModule.Phase == 'PEI':
                        # overwrite any the same PCD existing, if Phase is PEI
                        Index = self._DynaPcdList_.index(PcdFromModule)
                        self._DynaPcdList_[Index] = PcdFromModule
                elif PcdFromModule not in self._NonDynaPcdList_:
                    self._NonDynaPcdList_.append(PcdFromModule)
                elif PcdFromModule in self._NonDynaPcdList_ and PcdFromModule.IsFromBinaryInf == True:
                    Index = self._NonDynaPcdList_.index(PcdFromModule)
                    if self._NonDynaPcdList_[Index].IsFromBinaryInf == False:
                        #The PCD from Binary INF will override the same one from source INF
                        self._NonDynaPcdList_.remove (self._NonDynaPcdList_[Index])
                        PcdFromModule.Pending = False
                        self._NonDynaPcdList_.append (PcdFromModule)
        DscModuleSet = {os.path.normpath(ModuleInf.Path) for ModuleInf in self.Platform.Modules}
        # add the PCD from modules that listed in FDF but not in DSC to Database
        for InfName in FdfModuleList:
            if InfName not in DscModuleSet:
                InfClass = PathClass(InfName)
                M = self.BuildDatabase[InfClass, self.Arch, self.BuildTarget, self.ToolChain]
                # If a module INF in FDF but not in current arch's DSC module list, it must be module (either binary or source)
                # for different Arch. PCDs in source module for different Arch is already added before, so skip the source module here.
                # For binary module, if in current arch, we need to list the PCDs into database.
                if not M.IsBinaryModule:
                    continue
                # Override the module PCD setting by platform setting
                ModulePcdList = self.ApplyPcdSetting(M, M.Pcds)
                for PcdFromModule in ModulePcdList:
                    PcdFromModule.IsFromBinaryInf = True
                    PcdFromModule.IsFromDsc = False
                    # Only allow the DynamicEx and Patchable PCD in AsBuild INF
                    if PcdFromModule.Type not in PCD_DYNAMIC_EX_TYPE_SET and PcdFromModule.Type not in TAB_PCDS_PATCHABLE_IN_MODULE:
                        EdkLogger.error("build", AUTOGEN_ERROR, "PCD setting error",
                                        File=self.MetaFile,
                                        ExtraData="\n\tExisted %s PCD %s in:\n\t\t%s\n"
                                        % (PcdFromModule.Type, PcdFromModule.TokenCName, InfName))
                    # make sure that the "VOID*" kind of datum has MaxDatumSize set
                    if PcdFromModule.DatumType == TAB_VOID and not PcdFromModule.MaxDatumSize:
                        NoDatumTypePcdList.add("%s.%s [%s]" % (PcdFromModule.TokenSpaceGuidCName, PcdFromModule.TokenCName, InfName))
                    if M.ModuleType in SUP_MODULE_SET_PEI:
                        PcdFromModule.Phase = "PEI"
                    if PcdFromModule not in self._DynaPcdList_ and PcdFromModule.Type in PCD_DYNAMIC_EX_TYPE_SET:
                        self._DynaPcdList_.append(PcdFromModule)
                    elif PcdFromModule not in self._NonDynaPcdList_ and PcdFromModule.Type in TAB_PCDS_PATCHABLE_IN_MODULE:
                        self._NonDynaPcdList_.append(PcdFromModule)
                    if PcdFromModule in self._DynaPcdList_ and PcdFromModule.Phase == 'PEI' and PcdFromModule.Type in PCD_DYNAMIC_EX_TYPE_SET:
                        # Overwrite the phase of any the same PCD existing, if Phase is PEI.
                        # It is to solve the case that a dynamic PCD used by a PEM module/PEI
                        # module & DXE module at a same time.
                        # Overwrite the type of the PCDs in source INF by the type of AsBuild
                        # INF file as DynamicEx.
                        Index = self._DynaPcdList_.index(PcdFromModule)
                        self._DynaPcdList_[Index].Phase = PcdFromModule.Phase
                        self._DynaPcdList_[Index].Type = PcdFromModule.Type
        for PcdFromModule in self._NonDynaPcdList_:
            # If a PCD is not listed in the DSC file, but binary INF files used by
            # this platform all (that use this PCD) list the PCD in a [PatchPcds]
            # section, AND all source INF files used by this platform the build
            # that use the PCD list the PCD in either a [Pcds] or [PatchPcds]
            # section, then the tools must NOT add the PCD to the Platform's PCD
            # Database; the build must assign the access method for this PCD as
            # PcdsPatchableInModule.
            if PcdFromModule not in self._DynaPcdList_:
                continue
            Index = self._DynaPcdList_.index(PcdFromModule)
            if PcdFromModule.IsFromDsc == False and \
                PcdFromModule.Type in TAB_PCDS_PATCHABLE_IN_MODULE and \
                PcdFromModule.IsFromBinaryInf == True and \
                self._DynaPcdList_[Index].IsFromBinaryInf == False:
                Index = self._DynaPcdList_.index(PcdFromModule)
                self._DynaPcdList_.remove (self._DynaPcdList_[Index])

        # print out error information and break the build, if error found
        if len(NoDatumTypePcdList) > 0:
            NoDatumTypePcdListString = "\n\t\t".join(NoDatumTypePcdList)
            EdkLogger.error("build", AUTOGEN_ERROR, "PCD setting error",
                            File=self.MetaFile,
                            ExtraData="\n\tPCD(s) without MaxDatumSize:\n\t\t%s\n"
                                      % NoDatumTypePcdListString)
        self._NonDynamicPcdList = self._NonDynaPcdList_
        self._DynamicPcdList = self._DynaPcdList_
        #
        # Sort dynamic PCD list to:
        # 1) If PCD's datum type is VOID* and value is unicode string which starts with L, the PCD item should
        #    try to be put header of dynamicd List
        # 2) If PCD is HII type, the PCD item should be put after unicode type PCD
        #
        # The reason of sorting is make sure the unicode string is in double-byte alignment in string table.
        #
        UnicodePcdArray = set()
        HiiPcdArray     = set()
        OtherPcdArray   = set()
        VpdPcdDict      = {}
        VpdFile               = VpdInfoFile.VpdInfoFile()
        NeedProcessVpdMapFile = False

        for pcd in self.Platform.Pcds:
            if pcd not in self._PlatformPcds:
                self._PlatformPcds[pcd] = self.Platform.Pcds[pcd]

        for item in self._PlatformPcds:
            if self._PlatformPcds[item].DatumType and self._PlatformPcds[item].DatumType not in [TAB_UINT8, TAB_UINT16, TAB_UINT32, TAB_UINT64, TAB_VOID, "BOOLEAN"]:
                self._PlatformPcds[item].DatumType = TAB_VOID

        if (self.Workspace.ArchList[-1] == self.Arch):
            for Pcd in self._DynamicPcdList:
                # just pick the a value to determine whether is unicode string type
                Sku = Pcd.SkuInfoList.get(TAB_DEFAULT)
                Sku.VpdOffset = Sku.VpdOffset.strip()

                if Pcd.DatumType not in [TAB_UINT8, TAB_UINT16, TAB_UINT32, TAB_UINT64, TAB_VOID, "BOOLEAN"]:
                    Pcd.DatumType = TAB_VOID

                    # if found PCD which datum value is unicode string the insert to left size of UnicodeIndex
                    # if found HII type PCD then insert to right of UnicodeIndex
                if Pcd.Type in [TAB_PCDS_DYNAMIC_VPD, TAB_PCDS_DYNAMIC_EX_VPD]:
                    VpdPcdDict[(Pcd.TokenCName, Pcd.TokenSpaceGuidCName)] = Pcd

            #Collect DynamicHii PCD values and assign it to DynamicExVpd PCD gEfiMdeModulePkgTokenSpaceGuid.PcdNvStoreDefaultValueBuffer
            PcdNvStoreDfBuffer = VpdPcdDict.get(("PcdNvStoreDefaultValueBuffer", "gEfiMdeModulePkgTokenSpaceGuid"))
            if PcdNvStoreDfBuffer:
                self.VariableInfo = self.CollectVariables(self._DynamicPcdList)
                vardump = self.VariableInfo.dump()
                if vardump:
                    #
                    #According to PCD_DATABASE_INIT in edk2\MdeModulePkg\Include\Guid\PcdDataBaseSignatureGuid.h,
                    #the max size for string PCD should not exceed USHRT_MAX 65535(0xffff).
                    #typedef UINT16 SIZE_INFO;
                    #//SIZE_INFO  SizeTable[];
                    if len(vardump.split(",")) > 0xffff:
                        EdkLogger.error("build", RESOURCE_OVERFLOW, 'The current length of PCD %s value is %d, it exceeds to the max size of String PCD.' %(".".join([PcdNvStoreDfBuffer.TokenSpaceGuidCName,PcdNvStoreDfBuffer.TokenCName]) ,len(vardump.split(","))))
                    PcdNvStoreDfBuffer.DefaultValue = vardump
                    for skuname in PcdNvStoreDfBuffer.SkuInfoList:
                        PcdNvStoreDfBuffer.SkuInfoList[skuname].DefaultValue = vardump
                        PcdNvStoreDfBuffer.MaxDatumSize = str(len(vardump.split(",")))
            else:
                #If the end user define [DefaultStores] and [XXX.Menufacturing] in DSC, but forget to configure PcdNvStoreDefaultValueBuffer to PcdsDynamicVpd
                if [Pcd for Pcd in self._DynamicPcdList if Pcd.UserDefinedDefaultStoresFlag]:
                    EdkLogger.warn("build", "PcdNvStoreDefaultValueBuffer should be defined as PcdsDynamicExVpd in dsc file since the DefaultStores is enabled for this platform.\n%s" %self.Platform.MetaFile.Path)
            PlatformPcds = sorted(self._PlatformPcds.keys())
            #
            # Add VPD type PCD into VpdFile and determine whether the VPD PCD need to be fixed up.
            #
            VpdSkuMap = {}
            for PcdKey in PlatformPcds:
                Pcd = self._PlatformPcds[PcdKey]
                if Pcd.Type in [TAB_PCDS_DYNAMIC_VPD, TAB_PCDS_DYNAMIC_EX_VPD] and \
                   PcdKey in VpdPcdDict:
                    Pcd = VpdPcdDict[PcdKey]
                    SkuValueMap = {}
                    DefaultSku = Pcd.SkuInfoList.get(TAB_DEFAULT)
                    if DefaultSku:
                        PcdValue = DefaultSku.DefaultValue
                        if PcdValue not in SkuValueMap:
                            SkuValueMap[PcdValue] = []
                            VpdFile.Add(Pcd, TAB_DEFAULT, DefaultSku.VpdOffset)
                        SkuValueMap[PcdValue].append(DefaultSku)

                    for (SkuName, Sku) in Pcd.SkuInfoList.items():
                        Sku.VpdOffset = Sku.VpdOffset.strip()
                        PcdValue = Sku.DefaultValue
                        if PcdValue == "":
                            PcdValue  = Pcd.DefaultValue
                        if Sku.VpdOffset != TAB_STAR:
                            if PcdValue.startswith("{"):
                                Alignment = 8
                            elif PcdValue.startswith("L"):
                                Alignment = 2
                            else:
                                Alignment = 1
                            try:
                                VpdOffset = int(Sku.VpdOffset)
                            except:
                                try:
                                    VpdOffset = int(Sku.VpdOffset, 16)
                                except:
                                    EdkLogger.error("build", FORMAT_INVALID, "Invalid offset value %s for PCD %s.%s." % (Sku.VpdOffset, Pcd.TokenSpaceGuidCName, Pcd.TokenCName))
                            if VpdOffset % Alignment != 0:
                                if PcdValue.startswith("{"):
                                    EdkLogger.warn("build", "The offset value of PCD %s.%s is not 8-byte aligned!" %(Pcd.TokenSpaceGuidCName, Pcd.TokenCName), File=self.MetaFile)
                                else:
                                    EdkLogger.error("build", FORMAT_INVALID, 'The offset value of PCD %s.%s should be %s-byte aligned.' % (Pcd.TokenSpaceGuidCName, Pcd.TokenCName, Alignment))
                        if PcdValue not in SkuValueMap:
                            SkuValueMap[PcdValue] = []
                            VpdFile.Add(Pcd, SkuName, Sku.VpdOffset)
                        SkuValueMap[PcdValue].append(Sku)
                        # if the offset of a VPD is *, then it need to be fixed up by third party tool.
                        if not NeedProcessVpdMapFile and Sku.VpdOffset == TAB_STAR:
                            NeedProcessVpdMapFile = True
                            if self.Platform.VpdToolGuid is None or self.Platform.VpdToolGuid == '':
                                EdkLogger.error("Build", FILE_NOT_FOUND, \
                                                "Fail to find third-party BPDG tool to process VPD PCDs. BPDG Guid tool need to be defined in tools_def.txt and VPD_TOOL_GUID need to be provided in DSC file.")

                    VpdSkuMap[PcdKey] = SkuValueMap
            #
            # Fix the PCDs define in VPD PCD section that never referenced by module.
            # An example is PCD for signature usage.
            #
            for DscPcd in PlatformPcds:
                DscPcdEntry = self._PlatformPcds[DscPcd]
                if DscPcdEntry.Type in [TAB_PCDS_DYNAMIC_VPD, TAB_PCDS_DYNAMIC_EX_VPD]:
                    if not (self.Platform.VpdToolGuid is None or self.Platform.VpdToolGuid == ''):
                        FoundFlag = False
                        for VpdPcd in VpdFile._VpdArray:
                            # This PCD has been referenced by module
                            if (VpdPcd.TokenSpaceGuidCName == DscPcdEntry.TokenSpaceGuidCName) and \
                               (VpdPcd.TokenCName == DscPcdEntry.TokenCName):
                                    FoundFlag = True

                        # Not found, it should be signature
                        if not FoundFlag :
                            # just pick the a value to determine whether is unicode string type
                            SkuValueMap = {}
                            SkuObjList = list(DscPcdEntry.SkuInfoList.items())
                            DefaultSku = DscPcdEntry.SkuInfoList.get(TAB_DEFAULT)
                            if DefaultSku:
                                defaultindex = SkuObjList.index((TAB_DEFAULT, DefaultSku))
                                SkuObjList[0], SkuObjList[defaultindex] = SkuObjList[defaultindex], SkuObjList[0]
                            for (SkuName, Sku) in SkuObjList:
                                Sku.VpdOffset = Sku.VpdOffset.strip()

                                # Need to iterate DEC pcd information to get the value & datumtype
                                for eachDec in self.PackageList:
                                    for DecPcd in eachDec.Pcds:
                                        DecPcdEntry = eachDec.Pcds[DecPcd]
                                        if (DecPcdEntry.TokenSpaceGuidCName == DscPcdEntry.TokenSpaceGuidCName) and \
                                           (DecPcdEntry.TokenCName == DscPcdEntry.TokenCName):
                                            # Print warning message to let the developer make a determine.
                                            EdkLogger.warn("build", "Unreferenced vpd pcd used!",
                                                            File=self.MetaFile, \
                                                            ExtraData = "PCD: %s.%s used in the DSC file %s is unreferenced." \
                                                            %(DscPcdEntry.TokenSpaceGuidCName, DscPcdEntry.TokenCName, self.Platform.MetaFile.Path))

                                            DscPcdEntry.DatumType    = DecPcdEntry.DatumType
                                            DscPcdEntry.DefaultValue = DecPcdEntry.DefaultValue
                                            DscPcdEntry.TokenValue = DecPcdEntry.TokenValue
                                            DscPcdEntry.TokenSpaceGuidValue = eachDec.Guids[DecPcdEntry.TokenSpaceGuidCName]
                                            # Only fix the value while no value provided in DSC file.
                                            if not Sku.DefaultValue:
                                                DscPcdEntry.SkuInfoList[list(DscPcdEntry.SkuInfoList.keys())[0]].DefaultValue = DecPcdEntry.DefaultValue

                                if DscPcdEntry not in self._DynamicPcdList:
                                    self._DynamicPcdList.append(DscPcdEntry)
                                Sku.VpdOffset = Sku.VpdOffset.strip()
                                PcdValue = Sku.DefaultValue
                                if PcdValue == "":
                                    PcdValue  = DscPcdEntry.DefaultValue
                                if Sku.VpdOffset != TAB_STAR:
                                    if PcdValue.startswith("{"):
                                        Alignment = 8
                                    elif PcdValue.startswith("L"):
                                        Alignment = 2
                                    else:
                                        Alignment = 1
                                    try:
                                        VpdOffset = int(Sku.VpdOffset)
                                    except:
                                        try:
                                            VpdOffset = int(Sku.VpdOffset, 16)
                                        except:
                                            EdkLogger.error("build", FORMAT_INVALID, "Invalid offset value %s for PCD %s.%s." % (Sku.VpdOffset, DscPcdEntry.TokenSpaceGuidCName, DscPcdEntry.TokenCName))
                                    if VpdOffset % Alignment != 0:
                                        if PcdValue.startswith("{"):
                                            EdkLogger.warn("build", "The offset value of PCD %s.%s is not 8-byte aligned!" %(DscPcdEntry.TokenSpaceGuidCName, DscPcdEntry.TokenCName), File=self.MetaFile)
                                        else:
                                            EdkLogger.error("build", FORMAT_INVALID, 'The offset value of PCD %s.%s should be %s-byte aligned.' % (DscPcdEntry.TokenSpaceGuidCName, DscPcdEntry.TokenCName, Alignment))
                                if PcdValue not in SkuValueMap:
                                    SkuValueMap[PcdValue] = []
                                    VpdFile.Add(DscPcdEntry, SkuName, Sku.VpdOffset)
                                SkuValueMap[PcdValue].append(Sku)
                                if not NeedProcessVpdMapFile and Sku.VpdOffset == TAB_STAR:
                                    NeedProcessVpdMapFile = True
                            if DscPcdEntry.DatumType == TAB_VOID and PcdValue.startswith("L"):
                                UnicodePcdArray.add(DscPcdEntry)
                            elif len(Sku.VariableName) > 0:
                                HiiPcdArray.add(DscPcdEntry)
                            else:
                                OtherPcdArray.add(DscPcdEntry)

                                # if the offset of a VPD is *, then it need to be fixed up by third party tool.
                            VpdSkuMap[DscPcd] = SkuValueMap
            if (self.Platform.FlashDefinition is None or self.Platform.FlashDefinition == '') and \
               VpdFile.GetCount() != 0:
                EdkLogger.error("build", ATTRIBUTE_NOT_AVAILABLE,
                                "Fail to get FLASH_DEFINITION definition in DSC file %s which is required when DSC contains VPD PCD." % str(self.Platform.MetaFile))

            if VpdFile.GetCount() != 0:

                self.FixVpdOffset(VpdFile)

                self.FixVpdOffset(self.UpdateNVStoreMaxSize(VpdFile))
                PcdNvStoreDfBuffer = [item for item in self._DynamicPcdList if item.TokenCName == "PcdNvStoreDefaultValueBuffer" and item.TokenSpaceGuidCName == "gEfiMdeModulePkgTokenSpaceGuid"]
                if PcdNvStoreDfBuffer:
                    PcdName,PcdGuid = PcdNvStoreDfBuffer[0].TokenCName, PcdNvStoreDfBuffer[0].TokenSpaceGuidCName
                    if (PcdName,PcdGuid) in VpdSkuMap:
                        DefaultSku = PcdNvStoreDfBuffer[0].SkuInfoList.get(TAB_DEFAULT)
                        VpdSkuMap[(PcdName,PcdGuid)] = {DefaultSku.DefaultValue:[SkuObj for SkuObj in PcdNvStoreDfBuffer[0].SkuInfoList.values() ]}

                # Process VPD map file generated by third party BPDG tool
                if NeedProcessVpdMapFile:
                    VpdMapFilePath = os.path.join(self.BuildDir, TAB_FV_DIRECTORY, "%s.map" % self.Platform.VpdToolGuid)
                    if os.path.exists(VpdMapFilePath):
                        VpdFile.Read(VpdMapFilePath)

                        # Fixup TAB_STAR offset
                        for pcd in VpdSkuMap:
                            vpdinfo = VpdFile.GetVpdInfo(pcd)
                            if vpdinfo is None:
                            # just pick the a value to determine whether is unicode string type
                                continue
                            for pcdvalue in VpdSkuMap[pcd]:
                                for sku in VpdSkuMap[pcd][pcdvalue]:
                                    for item in vpdinfo:
                                        if item[2] == pcdvalue:
                                            sku.VpdOffset = item[1]
                    else:
                        EdkLogger.error("build", FILE_READ_FAILURE, "Can not find VPD map file %s to fix up VPD offset." % VpdMapFilePath)

            # Delete the DynamicPcdList At the last time enter into this function
            for Pcd in self._DynamicPcdList:
                # just pick the a value to determine whether is unicode string type
                Sku = Pcd.SkuInfoList.get(TAB_DEFAULT)
                Sku.VpdOffset = Sku.VpdOffset.strip()

                if Pcd.DatumType not in [TAB_UINT8, TAB_UINT16, TAB_UINT32, TAB_UINT64, TAB_VOID, "BOOLEAN"]:
                    Pcd.DatumType = TAB_VOID

                PcdValue = Sku.DefaultValue
                if Pcd.DatumType == TAB_VOID and PcdValue.startswith("L"):
                    # if found PCD which datum value is unicode string the insert to left size of UnicodeIndex
                    UnicodePcdArray.add(Pcd)
                elif len(Sku.VariableName) > 0:
                    # if found HII type PCD then insert to right of UnicodeIndex
                    HiiPcdArray.add(Pcd)
                else:
                    OtherPcdArray.add(Pcd)
            del self._DynamicPcdList[:]
        self._DynamicPcdList.extend(list(UnicodePcdArray))
        self._DynamicPcdList.extend(list(HiiPcdArray))
        self._DynamicPcdList.extend(list(OtherPcdArray))
        allskuset = [(SkuName, Sku.SkuId) for pcd in self._DynamicPcdList for (SkuName, Sku) in pcd.SkuInfoList.items()]
        for pcd in self._DynamicPcdList:
            if len(pcd.SkuInfoList) == 1:
                for (SkuName, SkuId) in allskuset:
                    if isinstance(SkuId, str) and eval(SkuId) == 0 or SkuId == 0:
                        continue
                    pcd.SkuInfoList[SkuName] = copy.deepcopy(pcd.SkuInfoList[TAB_DEFAULT])
                    pcd.SkuInfoList[SkuName].SkuId = SkuId
                    pcd.SkuInfoList[SkuName].SkuIdName = SkuName

    def FixVpdOffset(self, VpdFile ):
        FvPath = os.path.join(self.BuildDir, TAB_FV_DIRECTORY)
        if not os.path.exists(FvPath):
            try:
                os.makedirs(FvPath)
            except:
                EdkLogger.error("build", FILE_WRITE_FAILURE, "Fail to create FV folder under %s" % self.BuildDir)

        VpdFilePath = os.path.join(FvPath, "%s.txt" % self.Platform.VpdToolGuid)

        if VpdFile.Write(VpdFilePath):
            # retrieve BPDG tool's path from tool_def.txt according to VPD_TOOL_GUID defined in DSC file.
            BPDGToolName = None
            for ToolDef in self.ToolDefinition.values():
                if TAB_GUID in ToolDef and ToolDef[TAB_GUID] == self.Platform.VpdToolGuid:
                    if "PATH" not in ToolDef:
                        EdkLogger.error("build", ATTRIBUTE_NOT_AVAILABLE, "PATH attribute was not provided for BPDG guid tool %s in tools_def.txt" % self.Platform.VpdToolGuid)
                    BPDGToolName = ToolDef["PATH"]
                    break
            # Call third party GUID BPDG tool.
            if BPDGToolName is not None:
                VpdInfoFile.CallExtenalBPDGTool(BPDGToolName, VpdFilePath)
            else:
                EdkLogger.error("Build", FILE_NOT_FOUND, "Fail to find third-party BPDG tool to process VPD PCDs. BPDG Guid tool need to be defined in tools_def.txt and VPD_TOOL_GUID need to be provided in DSC file.")

    ## Return the platform build data object
    @cached_property
    def Platform(self):
        return self.BuildDatabase[self.MetaFile, self.Arch, self.BuildTarget, self.ToolChain]

    ## Return platform name
    @cached_property
    def Name(self):
        return self.Platform.PlatformName

    ## Return the meta file GUID
    @cached_property
    def Guid(self):
        return self.Platform.Guid

    ## Return the platform version
    @cached_property
    def Version(self):
        return self.Platform.Version

    ## Return the FDF file name
    @cached_property
    def FdfFile(self):
        if self.Workspace.FdfFile:
            RetVal= mws.join(self.WorkspaceDir, self.Workspace.FdfFile)
        else:
            RetVal = ''
        return RetVal

    ## Return the build output directory platform specifies
    @cached_property
    def OutputDir(self):
        return self.Platform.OutputDirectory

    ## Return the directory to store all intermediate and final files built
    @cached_property
    def BuildDir(self):
        if os.path.isabs(self.OutputDir):
            GlobalData.gBuildDirectory = RetVal = path.join(
                                        path.abspath(self.OutputDir),
                                        self.BuildTarget + "_" + self.ToolChain,
                                        )
        else:
            GlobalData.gBuildDirectory = RetVal = path.join(
                                        self.WorkspaceDir,
                                        self.OutputDir,
                                        self.BuildTarget + "_" + self.ToolChain,
                                        )
        return RetVal

    ## Return directory of platform makefile
    #
    #   @retval     string  Makefile directory
    #
    @cached_property
    def MakeFileDir(self):
        return path.join(self.BuildDir, self.Arch)

    ## Return build command string
    #
    #   @retval     string  Build command string
    #
    @cached_property
    def BuildCommand(self):
        RetVal = []
        if "MAKE" in self.ToolDefinition and "PATH" in self.ToolDefinition["MAKE"]:
            RetVal += _SplitOption(self.ToolDefinition["MAKE"]["PATH"])
            if "FLAGS" in self.ToolDefinition["MAKE"]:
                NewOption = self.ToolDefinition["MAKE"]["FLAGS"].strip()
                if NewOption != '':
                    RetVal += _SplitOption(NewOption)
            if "MAKE" in self.EdkIIBuildOption:
                if "FLAGS" in self.EdkIIBuildOption["MAKE"]:
                    Flags = self.EdkIIBuildOption["MAKE"]["FLAGS"]
                    if Flags.startswith('='):
                        RetVal = [RetVal[0]] + [Flags[1:]]
                    else:
                        RetVal.append(Flags)
        return RetVal

    ## Get tool chain definition
    #
    #  Get each tool definition for given tool chain from tools_def.txt and platform
    #
    @cached_property
    def ToolDefinition(self):
        ToolDefinition = self.Workspace.ToolDef.ToolsDefTxtDictionary
        if TAB_TOD_DEFINES_COMMAND_TYPE not in self.Workspace.ToolDef.ToolsDefTxtDatabase:
            EdkLogger.error('build', RESOURCE_NOT_AVAILABLE, "No tools found in configuration",
                            ExtraData="[%s]" % self.MetaFile)
        RetVal = {}
        DllPathList = set()
        for Def in ToolDefinition:
            Target, Tag, Arch, Tool, Attr = Def.split("_")
            if Target != self.BuildTarget or Tag != self.ToolChain or Arch != self.Arch:
                continue

            Value = ToolDefinition[Def]
            # don't record the DLL
            if Attr == "DLL":
                DllPathList.add(Value)
                continue

            if Tool not in RetVal:
                RetVal[Tool] = {}
            RetVal[Tool][Attr] = Value

        ToolsDef = ''
        if GlobalData.gOptions.SilentMode and "MAKE" in RetVal:
            if "FLAGS" not in RetVal["MAKE"]:
                RetVal["MAKE"]["FLAGS"] = ""
            RetVal["MAKE"]["FLAGS"] += " -s"
        MakeFlags = ''
        for Tool in RetVal:
            for Attr in RetVal[Tool]:
                Value = RetVal[Tool][Attr]
                if Tool in self._BuildOptionWithToolDef(RetVal) and Attr in self._BuildOptionWithToolDef(RetVal)[Tool]:
                    # check if override is indicated
                    if self._BuildOptionWithToolDef(RetVal)[Tool][Attr].startswith('='):
                        Value = self._BuildOptionWithToolDef(RetVal)[Tool][Attr][1:]
                    else:
                        if Attr != 'PATH':
                            Value += " " + self._BuildOptionWithToolDef(RetVal)[Tool][Attr]
                        else:
                            Value = self._BuildOptionWithToolDef(RetVal)[Tool][Attr]

                if Attr == "PATH":
                    # Don't put MAKE definition in the file
                    if Tool != "MAKE":
                        ToolsDef += "%s = %s\n" % (Tool, Value)
                elif Attr != "DLL":
                    # Don't put MAKE definition in the file
                    if Tool == "MAKE":
                        if Attr == "FLAGS":
                            MakeFlags = Value
                    else:
                        ToolsDef += "%s_%s = %s\n" % (Tool, Attr, Value)
            ToolsDef += "\n"

        SaveFileOnChange(self.ToolDefinitionFile, ToolsDef, False)
        for DllPath in DllPathList:
            os.environ["PATH"] = DllPath + os.pathsep + os.environ["PATH"]
        os.environ["MAKE_FLAGS"] = MakeFlags

        return RetVal

    ## Return the paths of tools
    @cached_property
    def ToolDefinitionFile(self):
        return os.path.join(self.MakeFileDir, "TOOLS_DEF." + self.Arch)

    ## Retrieve the toolchain family of given toolchain tag. Default to 'MSFT'.
    @cached_property
    def ToolChainFamily(self):
        ToolDefinition = self.Workspace.ToolDef.ToolsDefTxtDatabase
        if TAB_TOD_DEFINES_FAMILY not in ToolDefinition \
           or self.ToolChain not in ToolDefinition[TAB_TOD_DEFINES_FAMILY] \
           or not ToolDefinition[TAB_TOD_DEFINES_FAMILY][self.ToolChain]:
            EdkLogger.verbose("No tool chain family found in configuration for %s. Default to MSFT." \
                               % self.ToolChain)
            RetVal = TAB_COMPILER_MSFT
        else:
            RetVal = ToolDefinition[TAB_TOD_DEFINES_FAMILY][self.ToolChain]
        return RetVal

    @cached_property
    def BuildRuleFamily(self):
        ToolDefinition = self.Workspace.ToolDef.ToolsDefTxtDatabase
        if TAB_TOD_DEFINES_BUILDRULEFAMILY not in ToolDefinition \
           or self.ToolChain not in ToolDefinition[TAB_TOD_DEFINES_BUILDRULEFAMILY] \
           or not ToolDefinition[TAB_TOD_DEFINES_BUILDRULEFAMILY][self.ToolChain]:
            EdkLogger.verbose("No tool chain family found in configuration for %s. Default to MSFT." \
                               % self.ToolChain)
            return TAB_COMPILER_MSFT

        return ToolDefinition[TAB_TOD_DEFINES_BUILDRULEFAMILY][self.ToolChain]

    ## Return the build options specific for all modules in this platform
    @cached_property
    def BuildOption(self):
        return self._ExpandBuildOption(self.Platform.BuildOptions)

    def _BuildOptionWithToolDef(self, ToolDef):
        return self._ExpandBuildOption(self.Platform.BuildOptions, ToolDef=ToolDef)

    ## Return the build options specific for EDK modules in this platform
    @cached_property
    def EdkBuildOption(self):
        return self._ExpandBuildOption(self.Platform.BuildOptions, EDK_NAME)

    ## Return the build options specific for EDKII modules in this platform
    @cached_property
    def EdkIIBuildOption(self):
        return self._ExpandBuildOption(self.Platform.BuildOptions, EDKII_NAME)

    ## Parse build_rule.txt in Conf Directory.
    #
    #   @retval     BuildRule object
    #
    @cached_property
    def BuildRule(self):
        BuildRuleFile = None
        if TAB_TAT_DEFINES_BUILD_RULE_CONF in self.Workspace.TargetTxt.TargetTxtDictionary:
            BuildRuleFile = self.Workspace.TargetTxt.TargetTxtDictionary[TAB_TAT_DEFINES_BUILD_RULE_CONF]
        if not BuildRuleFile:
            BuildRuleFile = gDefaultBuildRuleFile
        RetVal = BuildRule(BuildRuleFile)
        if RetVal._FileVersion == "":
            RetVal._FileVersion = AutoGenReqBuildRuleVerNum
        else:
            if RetVal._FileVersion < AutoGenReqBuildRuleVerNum :
                # If Build Rule's version is less than the version number required by the tools, halting the build.
                EdkLogger.error("build", AUTOGEN_ERROR,
                                ExtraData="The version number [%s] of build_rule.txt is less than the version number required by the AutoGen.(the minimum required version number is [%s])"\
                                 % (RetVal._FileVersion, AutoGenReqBuildRuleVerNum))
        return RetVal

    ## Summarize the packages used by modules in this platform
    @cached_property
    def PackageList(self):
        RetVal = set()
        for La in self.LibraryAutoGenList:
            RetVal.update(La.DependentPackageList)
        for Ma in self.ModuleAutoGenList:
            RetVal.update(Ma.DependentPackageList)
        #Collect package set information from INF of FDF
        for ModuleFile in self._AsBuildModuleList:
            if ModuleFile in self.Platform.Modules:
                continue
            ModuleData = self.BuildDatabase[ModuleFile, self.Arch, self.BuildTarget, self.ToolChain]
            RetVal.update(ModuleData.Packages)
        return list(RetVal)

    @cached_property
    def NonDynamicPcdDict(self):
        return {(Pcd.TokenCName, Pcd.TokenSpaceGuidCName):Pcd for Pcd in self.NonDynamicPcdList}

    ## Get list of non-dynamic PCDs
    @property
    def NonDynamicPcdList(self):
        if not self._NonDynamicPcdList:
            self.CollectPlatformDynamicPcds()
        return self._NonDynamicPcdList

    ## Get list of dynamic PCDs
    @property
    def DynamicPcdList(self):
        if not self._DynamicPcdList:
            self.CollectPlatformDynamicPcds()
        return self._DynamicPcdList

    ## Generate Token Number for all PCD
    @cached_property
    def PcdTokenNumber(self):
        RetVal = OrderedDict()
        TokenNumber = 1
        #
        # Make the Dynamic and DynamicEx PCD use within different TokenNumber area.
        # Such as:
        #
        # Dynamic PCD:
        # TokenNumber 0 ~ 10
        # DynamicEx PCD:
        # TokeNumber 11 ~ 20
        #
        for Pcd in self.DynamicPcdList:
            if Pcd.Phase == "PEI" and Pcd.Type in PCD_DYNAMIC_TYPE_SET:
                EdkLogger.debug(EdkLogger.DEBUG_5, "%s %s (%s) -> %d" % (Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Pcd.Phase, TokenNumber))
                RetVal[Pcd.TokenCName, Pcd.TokenSpaceGuidCName] = TokenNumber
                TokenNumber += 1

        for Pcd in self.DynamicPcdList:
            if Pcd.Phase == "PEI" and Pcd.Type in PCD_DYNAMIC_EX_TYPE_SET:
                EdkLogger.debug(EdkLogger.DEBUG_5, "%s %s (%s) -> %d" % (Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Pcd.Phase, TokenNumber))
                RetVal[Pcd.TokenCName, Pcd.TokenSpaceGuidCName] = TokenNumber
                TokenNumber += 1

        for Pcd in self.DynamicPcdList:
            if Pcd.Phase == "DXE" and Pcd.Type in PCD_DYNAMIC_TYPE_SET:
                EdkLogger.debug(EdkLogger.DEBUG_5, "%s %s (%s) -> %d" % (Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Pcd.Phase, TokenNumber))
                RetVal[Pcd.TokenCName, Pcd.TokenSpaceGuidCName] = TokenNumber
                TokenNumber += 1

        for Pcd in self.DynamicPcdList:
            if Pcd.Phase == "DXE" and Pcd.Type in PCD_DYNAMIC_EX_TYPE_SET:
                EdkLogger.debug(EdkLogger.DEBUG_5, "%s %s (%s) -> %d" % (Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Pcd.Phase, TokenNumber))
                RetVal[Pcd.TokenCName, Pcd.TokenSpaceGuidCName] = TokenNumber
                TokenNumber += 1

        for Pcd in self.NonDynamicPcdList:
            RetVal[Pcd.TokenCName, Pcd.TokenSpaceGuidCName] = TokenNumber
            TokenNumber += 1
        return RetVal

    @cached_property
    def _MaList(self):
        for ModuleFile in self.Platform.Modules:
            Ma = ModuleAutoGen(
                  self.Workspace,
                  ModuleFile,
                  self.BuildTarget,
                  self.ToolChain,
                  self.Arch,
                  self.MetaFile
                  )
            self.Platform.Modules[ModuleFile].M = Ma
        return [x.M for x in self.Platform.Modules.values()]

    ## Summarize ModuleAutoGen objects of all modules to be built for this platform
    @cached_property
    def ModuleAutoGenList(self):
        RetVal = []
        for Ma in self._MaList:
            if Ma not in RetVal:
                RetVal.append(Ma)
        return RetVal

    ## Summarize ModuleAutoGen objects of all libraries to be built for this platform
    @cached_property
    def LibraryAutoGenList(self):
        RetVal = []
        for Ma in self._MaList:
            for La in Ma.LibraryAutoGenList:
                if La not in RetVal:
                    RetVal.append(La)
                if Ma not in La.ReferenceModules:
                    La.ReferenceModules.append(Ma)
        return RetVal

    ## Test if a module is supported by the platform
    #
    #  An error will be raised directly if the module or its arch is not supported
    #  by the platform or current configuration
    #
    def ValidModule(self, Module):
        return Module in self.Platform.Modules or Module in self.Platform.LibraryInstances \
            or Module in self._AsBuildModuleList

    ## Resolve the library classes in a module to library instances
    #
    # This method will not only resolve library classes but also sort the library
    # instances according to the dependency-ship.
    #
    #   @param  Module      The module from which the library classes will be resolved
    #
    #   @retval library_list    List of library instances sorted
    #
    def ApplyLibraryInstance(self, Module):
        # Cover the case that the binary INF file is list in the FDF file but not DSC file, return empty list directly
        if str(Module) not in self.Platform.Modules:
            return []

        return GetModuleLibInstances(Module,
                                     self.Platform,
                                     self.BuildDatabase,
                                     self.Arch,
                                     self.BuildTarget,
                                     self.ToolChain,
                                     self.MetaFile,
                                     EdkLogger)

    ## Override PCD setting (type, value, ...)
    #
    #   @param  ToPcd       The PCD to be overridden
    #   @param  FromPcd     The PCD overriding from
    #
    def _OverridePcd(self, ToPcd, FromPcd, Module="", Msg="", Library=""):
        #
        # in case there's PCDs coming from FDF file, which have no type given.
        # at this point, ToPcd.Type has the type found from dependent
        # package
        #
        TokenCName = ToPcd.TokenCName
        for PcdItem in GlobalData.MixedPcd:
            if (ToPcd.TokenCName, ToPcd.TokenSpaceGuidCName) in GlobalData.MixedPcd[PcdItem]:
                TokenCName = PcdItem[0]
                break
        if FromPcd is not None:
            if ToPcd.Pending and FromPcd.Type:
                ToPcd.Type = FromPcd.Type
            elif ToPcd.Type and FromPcd.Type\
                and ToPcd.Type != FromPcd.Type and ToPcd.Type in FromPcd.Type:
                if ToPcd.Type.strip() == TAB_PCDS_DYNAMIC_EX:
                    ToPcd.Type = FromPcd.Type
            elif ToPcd.Type and FromPcd.Type \
                and ToPcd.Type != FromPcd.Type:
                if Library:
                    Module = str(Module) + " 's library file (" + str(Library) + ")"
                EdkLogger.error("build", OPTION_CONFLICT, "Mismatched PCD type",
                                ExtraData="%s.%s is used as [%s] in module %s, but as [%s] in %s."\
                                          % (ToPcd.TokenSpaceGuidCName, TokenCName,
                                             ToPcd.Type, Module, FromPcd.Type, Msg),
                                          File=self.MetaFile)

            if FromPcd.MaxDatumSize:
                ToPcd.MaxDatumSize = FromPcd.MaxDatumSize
                ToPcd.MaxSizeUserSet = FromPcd.MaxDatumSize
            if FromPcd.DefaultValue:
                ToPcd.DefaultValue = FromPcd.DefaultValue
            if FromPcd.TokenValue:
                ToPcd.TokenValue = FromPcd.TokenValue
            if FromPcd.DatumType:
                ToPcd.DatumType = FromPcd.DatumType
            if FromPcd.SkuInfoList:
                ToPcd.SkuInfoList = FromPcd.SkuInfoList
            if FromPcd.UserDefinedDefaultStoresFlag:
                ToPcd.UserDefinedDefaultStoresFlag = FromPcd.UserDefinedDefaultStoresFlag
            # Add Flexible PCD format parse
            if ToPcd.DefaultValue:
                try:
                    ToPcd.DefaultValue = ValueExpressionEx(ToPcd.DefaultValue, ToPcd.DatumType, self.Workspace._GuidDict)(True)
                except BadExpression as Value:
                    EdkLogger.error('Parser', FORMAT_INVALID, 'PCD [%s.%s] Value "%s", %s' %(ToPcd.TokenSpaceGuidCName, ToPcd.TokenCName, ToPcd.DefaultValue, Value),
                                        File=self.MetaFile)

            # check the validation of datum
            IsValid, Cause = CheckPcdDatum(ToPcd.DatumType, ToPcd.DefaultValue)
            if not IsValid:
                EdkLogger.error('build', FORMAT_INVALID, Cause, File=self.MetaFile,
                                ExtraData="%s.%s" % (ToPcd.TokenSpaceGuidCName, TokenCName))
            ToPcd.validateranges = FromPcd.validateranges
            ToPcd.validlists = FromPcd.validlists
            ToPcd.expressions = FromPcd.expressions
            ToPcd.CustomAttribute = FromPcd.CustomAttribute

        if FromPcd is not None and ToPcd.DatumType == TAB_VOID and not ToPcd.MaxDatumSize:
            EdkLogger.debug(EdkLogger.DEBUG_9, "No MaxDatumSize specified for PCD %s.%s" \
                            % (ToPcd.TokenSpaceGuidCName, TokenCName))
            Value = ToPcd.DefaultValue
            if not Value:
                ToPcd.MaxDatumSize = '1'
            elif Value[0] == 'L':
                ToPcd.MaxDatumSize = str((len(Value) - 2) * 2)
            elif Value[0] == '{':
                ToPcd.MaxDatumSize = str(len(Value.split(',')))
            else:
                ToPcd.MaxDatumSize = str(len(Value) - 1)

        # apply default SKU for dynamic PCDS if specified one is not available
        if (ToPcd.Type in PCD_DYNAMIC_TYPE_SET or ToPcd.Type in PCD_DYNAMIC_EX_TYPE_SET) \
            and not ToPcd.SkuInfoList:
            if self.Platform.SkuName in self.Platform.SkuIds:
                SkuName = self.Platform.SkuName
            else:
                SkuName = TAB_DEFAULT
            ToPcd.SkuInfoList = {
                SkuName : SkuInfoClass(SkuName, self.Platform.SkuIds[SkuName][0], '', '', '', '', '', ToPcd.DefaultValue)
            }

    ## Apply PCD setting defined platform to a module
    #
    #   @param  Module  The module from which the PCD setting will be overridden
    #
    #   @retval PCD_list    The list PCDs with settings from platform
    #
    def ApplyPcdSetting(self, Module, Pcds, Library=""):
        # for each PCD in module
        for Name, Guid in Pcds:
            PcdInModule = Pcds[Name, Guid]
            # find out the PCD setting in platform
            if (Name, Guid) in self.Platform.Pcds:
                PcdInPlatform = self.Platform.Pcds[Name, Guid]
            else:
                PcdInPlatform = None
            # then override the settings if any
            self._OverridePcd(PcdInModule, PcdInPlatform, Module, Msg="DSC PCD sections", Library=Library)
            # resolve the VariableGuid value
            for SkuId in PcdInModule.SkuInfoList:
                Sku = PcdInModule.SkuInfoList[SkuId]
                if Sku.VariableGuid == '': continue
                Sku.VariableGuidValue = GuidValue(Sku.VariableGuid, self.PackageList, self.MetaFile.Path)
                if Sku.VariableGuidValue is None:
                    PackageList = "\n\t".join(str(P) for P in self.PackageList)
                    EdkLogger.error(
                                'build',
                                RESOURCE_NOT_AVAILABLE,
                                "Value of GUID [%s] is not found in" % Sku.VariableGuid,
                                ExtraData=PackageList + "\n\t(used with %s.%s from module %s)" \
                                                        % (Guid, Name, str(Module)),
                                File=self.MetaFile
                                )

        # override PCD settings with module specific setting
        if Module in self.Platform.Modules:
            PlatformModule = self.Platform.Modules[str(Module)]
            for Key  in PlatformModule.Pcds:
                if GlobalData.BuildOptionPcd:
                    for pcd in GlobalData.BuildOptionPcd:
                        (TokenSpaceGuidCName, TokenCName, FieldName, pcdvalue, _) = pcd
                        if (TokenCName, TokenSpaceGuidCName) == Key and FieldName =="":
                            PlatformModule.Pcds[Key].DefaultValue = pcdvalue
                            PlatformModule.Pcds[Key].PcdValueFromComm = pcdvalue
                            break
                Flag = False
                if Key in Pcds:
                    ToPcd = Pcds[Key]
                    Flag = True
                elif Key in GlobalData.MixedPcd:
                    for PcdItem in GlobalData.MixedPcd[Key]:
                        if PcdItem in Pcds:
                            ToPcd = Pcds[PcdItem]
                            Flag = True
                            break
                if Flag:
                    self._OverridePcd(ToPcd, PlatformModule.Pcds[Key], Module, Msg="DSC Components Module scoped PCD section", Library=Library)
        # use PCD value to calculate the MaxDatumSize when it is not specified
        for Name, Guid in Pcds:
            Pcd = Pcds[Name, Guid]
            if Pcd.DatumType == TAB_VOID and not Pcd.MaxDatumSize:
                Pcd.MaxSizeUserSet = None
                Value = Pcd.DefaultValue
                if not Value:
                    Pcd.MaxDatumSize = '1'
                elif Value[0] == 'L':
                    Pcd.MaxDatumSize = str((len(Value) - 2) * 2)
                elif Value[0] == '{':
                    Pcd.MaxDatumSize = str(len(Value.split(',')))
                else:
                    Pcd.MaxDatumSize = str(len(Value) - 1)
        return list(Pcds.values())



    ## Calculate the priority value of the build option
    #
    # @param    Key    Build option definition contain: TARGET_TOOLCHAIN_ARCH_COMMANDTYPE_ATTRIBUTE
    #
    # @retval   Value  Priority value based on the priority list.
    #
    def CalculatePriorityValue(self, Key):
        Target, ToolChain, Arch, CommandType, Attr = Key.split('_')
        PriorityValue = 0x11111
        if Target == TAB_STAR:
            PriorityValue &= 0x01111
        if ToolChain == TAB_STAR:
            PriorityValue &= 0x10111
        if Arch == TAB_STAR:
            PriorityValue &= 0x11011
        if CommandType == TAB_STAR:
            PriorityValue &= 0x11101
        if Attr == TAB_STAR:
            PriorityValue &= 0x11110

        return self.PrioList["0x%0.5x" % PriorityValue]


    ## Expand * in build option key
    #
    #   @param  Options     Options to be expanded
    #   @param  ToolDef     Use specified ToolDef instead of full version.
    #                       This is needed during initialization to prevent
    #                       infinite recursion betweeh BuildOptions,
    #                       ToolDefinition, and this function.
    #
    #   @retval options     Options expanded
    #
    def _ExpandBuildOption(self, Options, ModuleStyle=None, ToolDef=None):
        if not ToolDef:
            ToolDef = self.ToolDefinition
        BuildOptions = {}
        FamilyMatch  = False
        FamilyIsNull = True

        OverrideList = {}
        #
        # Construct a list contain the build options which need override.
        #
        for Key in Options:
            #
            # Key[0] -- tool family
            # Key[1] -- TARGET_TOOLCHAIN_ARCH_COMMANDTYPE_ATTRIBUTE
            #
            if (Key[0] == self.BuildRuleFamily and
                (ModuleStyle is None or len(Key) < 3 or (len(Key) > 2 and Key[2] == ModuleStyle))):
                Target, ToolChain, Arch, CommandType, Attr = Key[1].split('_')
                if (Target == self.BuildTarget or Target == TAB_STAR) and\
                    (ToolChain == self.ToolChain or ToolChain == TAB_STAR) and\
                    (Arch == self.Arch or Arch == TAB_STAR) and\
                    Options[Key].startswith("="):

                    if OverrideList.get(Key[1]) is not None:
                        OverrideList.pop(Key[1])
                    OverrideList[Key[1]] = Options[Key]

        #
        # Use the highest priority value.
        #
        if (len(OverrideList) >= 2):
            KeyList = list(OverrideList.keys())
            for Index in range(len(KeyList)):
                NowKey = KeyList[Index]
                Target1, ToolChain1, Arch1, CommandType1, Attr1 = NowKey.split("_")
                for Index1 in range(len(KeyList) - Index - 1):
                    NextKey = KeyList[Index1 + Index + 1]
                    #
                    # Compare two Key, if one is included by another, choose the higher priority one
                    #
                    Target2, ToolChain2, Arch2, CommandType2, Attr2 = NextKey.split("_")
                    if (Target1 == Target2 or Target1 == TAB_STAR or Target2 == TAB_STAR) and\
                        (ToolChain1 == ToolChain2 or ToolChain1 == TAB_STAR or ToolChain2 == TAB_STAR) and\
                        (Arch1 == Arch2 or Arch1 == TAB_STAR or Arch2 == TAB_STAR) and\
                        (CommandType1 == CommandType2 or CommandType1 == TAB_STAR or CommandType2 == TAB_STAR) and\
                        (Attr1 == Attr2 or Attr1 == TAB_STAR or Attr2 == TAB_STAR):

                        if self.CalculatePriorityValue(NowKey) > self.CalculatePriorityValue(NextKey):
                            if Options.get((self.BuildRuleFamily, NextKey)) is not None:
                                Options.pop((self.BuildRuleFamily, NextKey))
                        else:
                            if Options.get((self.BuildRuleFamily, NowKey)) is not None:
                                Options.pop((self.BuildRuleFamily, NowKey))

        for Key in Options:
            if ModuleStyle is not None and len (Key) > 2:
                # Check Module style is EDK or EDKII.
                # Only append build option for the matched style module.
                if ModuleStyle == EDK_NAME and Key[2] != EDK_NAME:
                    continue
                elif ModuleStyle == EDKII_NAME and Key[2] != EDKII_NAME:
                    continue
            Family = Key[0]
            Target, Tag, Arch, Tool, Attr = Key[1].split("_")
            # if tool chain family doesn't match, skip it
            if Tool in ToolDef and Family != "":
                FamilyIsNull = False
                if ToolDef[Tool].get(TAB_TOD_DEFINES_BUILDRULEFAMILY, "") != "":
                    if Family != ToolDef[Tool][TAB_TOD_DEFINES_BUILDRULEFAMILY]:
                        continue
                elif Family != ToolDef[Tool][TAB_TOD_DEFINES_FAMILY]:
                    continue
                FamilyMatch = True
            # expand any wildcard
            if Target == TAB_STAR or Target == self.BuildTarget:
                if Tag == TAB_STAR or Tag == self.ToolChain:
                    if Arch == TAB_STAR or Arch == self.Arch:
                        if Tool not in BuildOptions:
                            BuildOptions[Tool] = {}
                        if Attr != "FLAGS" or Attr not in BuildOptions[Tool] or Options[Key].startswith('='):
                            BuildOptions[Tool][Attr] = Options[Key]
                        else:
                            # append options for the same tool except PATH
                            if Attr != 'PATH':
                                BuildOptions[Tool][Attr] += " " + Options[Key]
                            else:
                                BuildOptions[Tool][Attr] = Options[Key]
        # Build Option Family has been checked, which need't to be checked again for family.
        if FamilyMatch or FamilyIsNull:
            return BuildOptions

        for Key in Options:
            if ModuleStyle is not None and len (Key) > 2:
                # Check Module style is EDK or EDKII.
                # Only append build option for the matched style module.
                if ModuleStyle == EDK_NAME and Key[2] != EDK_NAME:
                    continue
                elif ModuleStyle == EDKII_NAME and Key[2] != EDKII_NAME:
                    continue
            Family = Key[0]
            Target, Tag, Arch, Tool, Attr = Key[1].split("_")
            # if tool chain family doesn't match, skip it
            if Tool not in ToolDef or Family == "":
                continue
            # option has been added before
            if Family != ToolDef[Tool][TAB_TOD_DEFINES_FAMILY]:
                continue

            # expand any wildcard
            if Target == TAB_STAR or Target == self.BuildTarget:
                if Tag == TAB_STAR or Tag == self.ToolChain:
                    if Arch == TAB_STAR or Arch == self.Arch:
                        if Tool not in BuildOptions:
                            BuildOptions[Tool] = {}
                        if Attr != "FLAGS" or Attr not in BuildOptions[Tool] or Options[Key].startswith('='):
                            BuildOptions[Tool][Attr] = Options[Key]
                        else:
                            # append options for the same tool except PATH
                            if Attr != 'PATH':
                                BuildOptions[Tool][Attr] += " " + Options[Key]
                            else:
                                BuildOptions[Tool][Attr] = Options[Key]
        return BuildOptions

    ## Append build options in platform to a module
    #
    #   @param  Module  The module to which the build options will be appended
    #
    #   @retval options     The options appended with build options in platform
    #
    def ApplyBuildOption(self, Module):
        # Get the different options for the different style module
        PlatformOptions = self.EdkIIBuildOption
        ModuleTypeOptions = self.Platform.GetBuildOptionsByModuleType(EDKII_NAME, Module.ModuleType)
        ModuleTypeOptions = self._ExpandBuildOption(ModuleTypeOptions)
        ModuleOptions = self._ExpandBuildOption(Module.BuildOptions)
        if Module in self.Platform.Modules:
            PlatformModule = self.Platform.Modules[str(Module)]
            PlatformModuleOptions = self._ExpandBuildOption(PlatformModule.BuildOptions)
        else:
            PlatformModuleOptions = {}

        BuildRuleOrder = None
        for Options in [self.ToolDefinition, ModuleOptions, PlatformOptions, ModuleTypeOptions, PlatformModuleOptions]:
            for Tool in Options:
                for Attr in Options[Tool]:
                    if Attr == TAB_TOD_DEFINES_BUILDRULEORDER:
                        BuildRuleOrder = Options[Tool][Attr]

        AllTools = set(list(ModuleOptions.keys()) + list(PlatformOptions.keys()) +
                       list(PlatformModuleOptions.keys()) + list(ModuleTypeOptions.keys()) +
                       list(self.ToolDefinition.keys()))
        BuildOptions = defaultdict(lambda: defaultdict(str))
        for Tool in AllTools:
            for Options in [self.ToolDefinition, ModuleOptions, PlatformOptions, ModuleTypeOptions, PlatformModuleOptions]:
                if Tool not in Options:
                    continue
                for Attr in Options[Tool]:
                    #
                    # Do not generate it in Makefile
                    #
                    if Attr == TAB_TOD_DEFINES_BUILDRULEORDER:
                        continue
                    Value = Options[Tool][Attr]
                    # check if override is indicated
                    if Value.startswith('='):
                        BuildOptions[Tool][Attr] = mws.handleWsMacro(Value[1:])
                    else:
                        if Attr != 'PATH':
                            BuildOptions[Tool][Attr] += " " + mws.handleWsMacro(Value)
                        else:
                            BuildOptions[Tool][Attr] = mws.handleWsMacro(Value)

        return BuildOptions, BuildRuleOrder

#
# extend lists contained in a dictionary with lists stored in another dictionary
# if CopyToDict is not derived from DefaultDict(list) then this may raise exception
#
def ExtendCopyDictionaryLists(CopyToDict, CopyFromDict):
    for Key in CopyFromDict:
        CopyToDict[Key].extend(CopyFromDict[Key])

# Create a directory specified by a set of path elements and return the full path
def _MakeDir(PathList):
    RetVal = path.join(*PathList)
    CreateDirectory(RetVal)
    return RetVal

## ModuleAutoGen class
#
# This class encapsules the AutoGen behaviors for the build tools. In addition to
# the generation of AutoGen.h and AutoGen.c, it will generate *.depex file according
# to the [depex] section in module's inf file.
#
class ModuleAutoGen(AutoGen):
    # call super().__init__ then call the worker function with different parameter count
    def __init__(self, Workspace, MetaFile, Target, Toolchain, Arch, *args, **kwargs):
        if not hasattr(self, "_Init"):
            self._InitWorker(Workspace, MetaFile, Target, Toolchain, Arch, *args)
            self._Init = True

    ## Cache the timestamps of metafiles of every module in a class attribute
    #
    TimeDict = {}

    def __new__(cls, Workspace, MetaFile, Target, Toolchain, Arch, *args, **kwargs):
        # check if this module is employed by active platform
        if not PlatformAutoGen(Workspace, args[0], Target, Toolchain, Arch).ValidModule(MetaFile):
            EdkLogger.verbose("Module [%s] for [%s] is not employed by active platform\n" \
                              % (MetaFile, Arch))
            return None
        return super(ModuleAutoGen, cls).__new__(cls, Workspace, MetaFile, Target, Toolchain, Arch, *args, **kwargs)

    ## Initialize ModuleAutoGen
    #
    #   @param      Workspace           EdkIIWorkspaceBuild object
    #   @param      ModuleFile          The path of module file
    #   @param      Target              Build target (DEBUG, RELEASE)
    #   @param      Toolchain           Name of tool chain
    #   @param      Arch                The arch the module supports
    #   @param      PlatformFile        Platform meta-file
    #
    def _InitWorker(self, Workspace, ModuleFile, Target, Toolchain, Arch, PlatformFile):
        EdkLogger.debug(EdkLogger.DEBUG_9, "AutoGen module [%s] [%s]" % (ModuleFile, Arch))
        GlobalData.gProcessingFile = "%s [%s, %s, %s]" % (ModuleFile, Arch, Toolchain, Target)

        self.Workspace = Workspace
        self.WorkspaceDir = Workspace.WorkspaceDir
        self.MetaFile = ModuleFile
        self.PlatformInfo = PlatformAutoGen(Workspace, PlatformFile, Target, Toolchain, Arch)

        self.SourceDir = self.MetaFile.SubDir
        self.SourceDir = mws.relpath(self.SourceDir, self.WorkspaceDir)

        self.ToolChain = Toolchain
        self.BuildTarget = Target
        self.Arch = Arch
        self.ToolChainFamily = self.PlatformInfo.ToolChainFamily
        self.BuildRuleFamily = self.PlatformInfo.BuildRuleFamily

        self.IsCodeFileCreated = False
        self.IsAsBuiltInfCreated = False
        self.DepexGenerated = False

        self.BuildDatabase = self.Workspace.BuildDatabase
        self.BuildRuleOrder = None
        self.BuildTime      = 0

        self._PcdComments = OrderedListDict()
        self._GuidComments = OrderedListDict()
        self._ProtocolComments = OrderedListDict()
        self._PpiComments = OrderedListDict()
        self._BuildTargets            = None
        self._IntroBuildTargetList    = None
        self._FinalBuildTargetList    = None
        self._FileTypes               = None

        self.AutoGenDepSet = set()
        self.ReferenceModules = []
        self.ConstPcd                  = {}


    def __repr__(self):
        return "%s [%s]" % (self.MetaFile, self.Arch)

    # Get FixedAtBuild Pcds of this Module
    @cached_property
    def FixedAtBuildPcds(self):
        RetVal = []
        for Pcd in self.ModulePcdList:
            if Pcd.Type != TAB_PCDS_FIXED_AT_BUILD:
                continue
            if Pcd not in RetVal:
                RetVal.append(Pcd)
        return RetVal

    @cached_property
    def FixedVoidTypePcds(self):
        RetVal = {}
        for Pcd in self.FixedAtBuildPcds:
            if Pcd.DatumType == TAB_VOID:
                if '{}.{}'.format(Pcd.TokenSpaceGuidCName, Pcd.TokenCName) not in RetVal:
                    RetVal['{}.{}'.format(Pcd.TokenSpaceGuidCName, Pcd.TokenCName)] = Pcd.DefaultValue
        return RetVal

    @property
    def UniqueBaseName(self):
        BaseName = self.Name
        for Module in self.PlatformInfo.ModuleAutoGenList:
            if Module.MetaFile == self.MetaFile:
                continue
            if Module.Name == self.Name:
                if uuid.UUID(Module.Guid) == uuid.UUID(self.Guid):
                    EdkLogger.error("build", FILE_DUPLICATED, 'Modules have same BaseName and FILE_GUID:\n'
                                    '  %s\n  %s' % (Module.MetaFile, self.MetaFile))
                BaseName = '%s_%s' % (self.Name, self.Guid)
        return BaseName

    # Macros could be used in build_rule.txt (also Makefile)
    @cached_property
    def Macros(self):
        return OrderedDict((
            ("WORKSPACE" ,self.WorkspaceDir),
            ("MODULE_NAME" ,self.Name),
            ("MODULE_NAME_GUID" ,self.UniqueBaseName),
            ("MODULE_GUID" ,self.Guid),
            ("MODULE_VERSION" ,self.Version),
            ("MODULE_TYPE" ,self.ModuleType),
            ("MODULE_FILE" ,str(self.MetaFile)),
            ("MODULE_FILE_BASE_NAME" ,self.MetaFile.BaseName),
            ("MODULE_RELATIVE_DIR" ,self.SourceDir),
            ("MODULE_DIR" ,self.SourceDir),
            ("BASE_NAME" ,self.Name),
            ("ARCH" ,self.Arch),
            ("TOOLCHAIN" ,self.ToolChain),
            ("TOOLCHAIN_TAG" ,self.ToolChain),
            ("TOOL_CHAIN_TAG" ,self.ToolChain),
            ("TARGET" ,self.BuildTarget),
            ("BUILD_DIR" ,self.PlatformInfo.BuildDir),
            ("BIN_DIR" ,os.path.join(self.PlatformInfo.BuildDir, self.Arch)),
            ("LIB_DIR" ,os.path.join(self.PlatformInfo.BuildDir, self.Arch)),
            ("MODULE_BUILD_DIR" ,self.BuildDir),
            ("OUTPUT_DIR" ,self.OutputDir),
            ("DEBUG_DIR" ,self.DebugDir),
            ("DEST_DIR_OUTPUT" ,self.OutputDir),
            ("DEST_DIR_DEBUG" ,self.DebugDir),
            ("PLATFORM_NAME" ,self.PlatformInfo.Name),
            ("PLATFORM_GUID" ,self.PlatformInfo.Guid),
            ("PLATFORM_VERSION" ,self.PlatformInfo.Version),
            ("PLATFORM_RELATIVE_DIR" ,self.PlatformInfo.SourceDir),
            ("PLATFORM_DIR" ,mws.join(self.WorkspaceDir, self.PlatformInfo.SourceDir)),
            ("PLATFORM_OUTPUT_DIR" ,self.PlatformInfo.OutputDir),
            ("FFS_OUTPUT_DIR" ,self.FfsOutputDir)
            ))

    ## Return the module build data object
    @cached_property
    def Module(self):
        return self.BuildDatabase[self.MetaFile, self.Arch, self.BuildTarget, self.ToolChain]

    ## Return the module name
    @cached_property
    def Name(self):
        return self.Module.BaseName

    ## Return the module DxsFile if exist
    @cached_property
    def DxsFile(self):
        return self.Module.DxsFile

    ## Return the module meta-file GUID
    @cached_property
    def Guid(self):
        #
        # To build same module more than once, the module path with FILE_GUID overridden has
        # the file name FILE_GUIDmodule.inf, but the relative path (self.MetaFile.File) is the real path
        # in DSC. The overridden GUID can be retrieved from file name
        #
        if os.path.basename(self.MetaFile.File) != os.path.basename(self.MetaFile.Path):
            #
            # Length of GUID is 36
            #
            return os.path.basename(self.MetaFile.Path)[:36]
        return self.Module.Guid

    ## Return the module version
    @cached_property
    def Version(self):
        return self.Module.Version

    ## Return the module type
    @cached_property
    def ModuleType(self):
        return self.Module.ModuleType

    ## Return the component type (for Edk.x style of module)
    @cached_property
    def ComponentType(self):
        return self.Module.ComponentType

    ## Return the build type
    @cached_property
    def BuildType(self):
        return self.Module.BuildType

    ## Return the PCD_IS_DRIVER setting
    @cached_property
    def PcdIsDriver(self):
        return self.Module.PcdIsDriver

    ## Return the autogen version, i.e. module meta-file version
    @cached_property
    def AutoGenVersion(self):
        return self.Module.AutoGenVersion

    ## Check if the module is library or not
    @cached_property
    def IsLibrary(self):
        return bool(self.Module.LibraryClass)

    ## Check if the module is binary module or not
    @cached_property
    def IsBinaryModule(self):
        return self.Module.IsBinaryModule

    ## Return the directory to store intermediate files of the module
    @cached_property
    def BuildDir(self):
        return _MakeDir((
                                    self.PlatformInfo.BuildDir,
                                    self.Arch,
                                    self.SourceDir,
                                    self.MetaFile.BaseName
            ))

    ## Return the directory to store the intermediate object files of the module
    @cached_property
    def OutputDir(self):
        return _MakeDir((self.BuildDir, "OUTPUT"))

    ## Return the directory path to store ffs file
    @cached_property
    def FfsOutputDir(self):
        if GlobalData.gFdfParser:
            return path.join(self.PlatformInfo.BuildDir, TAB_FV_DIRECTORY, "Ffs", self.Guid + self.Name)
        return ''

    ## Return the directory to store auto-gened source files of the module
    @cached_property
    def DebugDir(self):
        return _MakeDir((self.BuildDir, "DEBUG"))

    ## Return the path of custom file
    @cached_property
    def CustomMakefile(self):
        RetVal = {}
        for Type in self.Module.CustomMakefile:
            MakeType = gMakeTypeMap[Type] if Type in gMakeTypeMap else 'nmake'
            File = os.path.join(self.SourceDir, self.Module.CustomMakefile[Type])
            RetVal[MakeType] = File
        return RetVal

    ## Return the directory of the makefile
    #
    #   @retval     string  The directory string of module's makefile
    #
    @cached_property
    def MakeFileDir(self):
        return self.BuildDir

    ## Return build command string
    #
    #   @retval     string  Build command string
    #
    @cached_property
    def BuildCommand(self):
        return self.PlatformInfo.BuildCommand

    ## Get object list of all packages the module and its dependent libraries belong to
    #
    #   @retval     list    The list of package object
    #
    @cached_property
    def DerivedPackageList(self):
        PackageList = []
        for M in [self.Module] + self.DependentLibraryList:
            for Package in M.Packages:
                if Package in PackageList:
                    continue
                PackageList.append(Package)
        return PackageList

    ## Get the depex string
    #
    # @return : a string contain all depex expression.
    def _GetDepexExpresionString(self):
        DepexStr = ''
        DepexList = []
        ## DPX_SOURCE IN Define section.
        if self.Module.DxsFile:
            return DepexStr
        for M in [self.Module] + self.DependentLibraryList:
            Filename = M.MetaFile.Path
            InfObj = InfSectionParser.InfSectionParser(Filename)
            DepexExpressionList = InfObj.GetDepexExpresionList()
            for DepexExpression in DepexExpressionList:
                for key in DepexExpression:
                    Arch, ModuleType = key
                    DepexExpr = [x for x in DepexExpression[key] if not str(x).startswith('#')]
                    # the type of build module is USER_DEFINED.
                    # All different DEPEX section tags would be copied into the As Built INF file
                    # and there would be separate DEPEX section tags
                    if self.ModuleType.upper() == SUP_MODULE_USER_DEFINED:
                        if (Arch.upper() == self.Arch.upper()) and (ModuleType.upper() != TAB_ARCH_COMMON):
                            DepexList.append({(Arch, ModuleType): DepexExpr})
                    else:
                        if Arch.upper() == TAB_ARCH_COMMON or \
                          (Arch.upper() == self.Arch.upper() and \
                          ModuleType.upper() in [TAB_ARCH_COMMON, self.ModuleType.upper()]):
                            DepexList.append({(Arch, ModuleType): DepexExpr})

        #the type of build module is USER_DEFINED.
        if self.ModuleType.upper() == SUP_MODULE_USER_DEFINED:
            for Depex in DepexList:
                for key in Depex:
                    DepexStr += '[Depex.%s.%s]\n' % key
                    DepexStr += '\n'.join('# '+ val for val in Depex[key])
                    DepexStr += '\n\n'
            if not DepexStr:
                return '[Depex.%s]\n' % self.Arch
            return DepexStr

        #the type of build module not is USER_DEFINED.
        Count = 0
        for Depex in DepexList:
            Count += 1
            if DepexStr != '':
                DepexStr += ' AND '
            DepexStr += '('
            for D in Depex.values():
                DepexStr += ' '.join(val for val in D)
            Index = DepexStr.find('END')
            if Index > -1 and Index == len(DepexStr) - 3:
                DepexStr = DepexStr[:-3]
            DepexStr = DepexStr.strip()
            DepexStr += ')'
        if Count == 1:
            DepexStr = DepexStr.lstrip('(').rstrip(')').strip()
        if not DepexStr:
            return '[Depex.%s]\n' % self.Arch
        return '[Depex.%s]\n#  ' % self.Arch + DepexStr

    ## Merge dependency expression
    #
    #   @retval     list    The token list of the dependency expression after parsed
    #
    @cached_property
    def DepexList(self):
        if self.DxsFile or self.IsLibrary or TAB_DEPENDENCY_EXPRESSION_FILE in self.FileTypes:
            return {}

        DepexList = []
        #
        # Append depex from dependent libraries, if not "BEFORE", "AFTER" expression
        #
        for M in [self.Module] + self.DependentLibraryList:
            Inherited = False
            for D in M.Depex[self.Arch, self.ModuleType]:
                if DepexList != []:
                    DepexList.append('AND')
                DepexList.append('(')
                #replace D with value if D is FixedAtBuild PCD
                NewList = []
                for item in D:
                    if '.' not in item:
                        NewList.append(item)
                    else:
                        FixedVoidTypePcds = {}
                        if item in self.FixedVoidTypePcds:
                            FixedVoidTypePcds = self.FixedVoidTypePcds
                        elif M in self.PlatformInfo.LibraryAutoGenList:
                            Index = self.PlatformInfo.LibraryAutoGenList.index(M)
                            FixedVoidTypePcds = self.PlatformInfo.LibraryAutoGenList[Index].FixedVoidTypePcds
                        if item not in FixedVoidTypePcds:
                            EdkLogger.error("build", FORMAT_INVALID, "{} used in [Depex] section should be used as FixedAtBuild type and VOID* datum type in the module.".format(item))
                        else:
                            Value = FixedVoidTypePcds[item]
                            if len(Value.split(',')) != 16:
                                EdkLogger.error("build", FORMAT_INVALID,
                                                "{} used in [Depex] section should be used as FixedAtBuild type and VOID* datum type and 16 bytes in the module.".format(item))
                            NewList.append(Value)
                DepexList.extend(NewList)
                if DepexList[-1] == 'END':  # no need of a END at this time
                    DepexList.pop()
                DepexList.append(')')
                Inherited = True
            if Inherited:
                EdkLogger.verbose("DEPEX[%s] (+%s) = %s" % (self.Name, M.BaseName, DepexList))
            if 'BEFORE' in DepexList or 'AFTER' in DepexList:
                break
            if len(DepexList) > 0:
                EdkLogger.verbose('')
        return {self.ModuleType:DepexList}

    ## Merge dependency expression
    #
    #   @retval     list    The token list of the dependency expression after parsed
    #
    @cached_property
    def DepexExpressionDict(self):
        if self.DxsFile or self.IsLibrary or TAB_DEPENDENCY_EXPRESSION_FILE in self.FileTypes:
            return {}

        DepexExpressionString = ''
        #
        # Append depex from dependent libraries, if not "BEFORE", "AFTER" expresion
        #
        for M in [self.Module] + self.DependentLibraryList:
            Inherited = False
            for D in M.DepexExpression[self.Arch, self.ModuleType]:
                if DepexExpressionString != '':
                    DepexExpressionString += ' AND '
                DepexExpressionString += '('
                DepexExpressionString += D
                DepexExpressionString = DepexExpressionString.rstrip('END').strip()
                DepexExpressionString += ')'
                Inherited = True
            if Inherited:
                EdkLogger.verbose("DEPEX[%s] (+%s) = %s" % (self.Name, M.BaseName, DepexExpressionString))
            if 'BEFORE' in DepexExpressionString or 'AFTER' in DepexExpressionString:
                break
        if len(DepexExpressionString) > 0:
            EdkLogger.verbose('')

        return {self.ModuleType:DepexExpressionString}

    # Get the tiano core user extension, it is contain dependent library.
    # @retval: a list contain tiano core userextension.
    #
    def _GetTianoCoreUserExtensionList(self):
        TianoCoreUserExtentionList = []
        for M in [self.Module] + self.DependentLibraryList:
            Filename = M.MetaFile.Path
            InfObj = InfSectionParser.InfSectionParser(Filename)
            TianoCoreUserExtenList = InfObj.GetUserExtensionTianoCore()
            for TianoCoreUserExtent in TianoCoreUserExtenList:
                for Section in TianoCoreUserExtent:
                    ItemList = Section.split(TAB_SPLIT)
                    Arch = self.Arch
                    if len(ItemList) == 4:
                        Arch = ItemList[3]
                    if Arch.upper() == TAB_ARCH_COMMON or Arch.upper() == self.Arch.upper():
                        TianoCoreList = []
                        TianoCoreList.extend([TAB_SECTION_START + Section + TAB_SECTION_END])
                        TianoCoreList.extend(TianoCoreUserExtent[Section][:])
                        TianoCoreList.append('\n')
                        TianoCoreUserExtentionList.append(TianoCoreList)

        return TianoCoreUserExtentionList

    ## Return the list of specification version required for the module
    #
    #   @retval     list    The list of specification defined in module file
    #
    @cached_property
    def Specification(self):
        return self.Module.Specification

    ## Tool option for the module build
    #
    #   @param      PlatformInfo    The object of PlatformBuildInfo
    #   @retval     dict            The dict containing valid options
    #
    @cached_property
    def BuildOption(self):
        RetVal, self.BuildRuleOrder = self.PlatformInfo.ApplyBuildOption(self.Module)
        if self.BuildRuleOrder:
            self.BuildRuleOrder = ['.%s' % Ext for Ext in self.BuildRuleOrder.split()]
        return RetVal

    ## Get include path list from tool option for the module build
    #
    #   @retval     list            The include path list
    #
    @cached_property
    def BuildOptionIncPathList(self):
        #
        # Regular expression for finding Include Directories, the difference between MSFT and INTEL/GCC/RVCT
        # is the former use /I , the Latter used -I to specify include directories
        #
        if self.PlatformInfo.ToolChainFamily in (TAB_COMPILER_MSFT):
            BuildOptIncludeRegEx = gBuildOptIncludePatternMsft
        elif self.PlatformInfo.ToolChainFamily in ('INTEL', 'GCC', 'RVCT'):
            BuildOptIncludeRegEx = gBuildOptIncludePatternOther
        else:
            #
            # New ToolChainFamily, don't known whether there is option to specify include directories
            #
            return []

        RetVal = []
        for Tool in ('CC', 'PP', 'VFRPP', 'ASLPP', 'ASLCC', 'APP', 'ASM'):
            try:
                FlagOption = self.BuildOption[Tool]['FLAGS']
            except KeyError:
                FlagOption = ''

            if self.ToolChainFamily != 'RVCT':
                IncPathList = [NormPath(Path, self.Macros) for Path in BuildOptIncludeRegEx.findall(FlagOption)]
            else:
                #
                # RVCT may specify a list of directory seperated by commas
                #
                IncPathList = []
                for Path in BuildOptIncludeRegEx.findall(FlagOption):
                    PathList = GetSplitList(Path, TAB_COMMA_SPLIT)
                    IncPathList.extend(NormPath(PathEntry, self.Macros) for PathEntry in PathList)

            #
            # EDK II modules must not reference header files outside of the packages they depend on or
            # within the module's directory tree. Report error if violation.
            #
            if GlobalData.gDisableIncludePathCheck == False:
                for Path in IncPathList:
                    if (Path not in self.IncludePathList) and (CommonPath([Path, self.MetaFile.Dir]) != self.MetaFile.Dir):
                        ErrMsg = "The include directory for the EDK II module in this line is invalid %s specified in %s FLAGS '%s'" % (Path, Tool, FlagOption)
                        EdkLogger.error("build",
                                        PARAMETER_INVALID,
                                        ExtraData=ErrMsg,
                                        File=str(self.MetaFile))
            RetVal += IncPathList
        return RetVal

    ## Return a list of files which can be built from source
    #
    #  What kind of files can be built is determined by build rules in
    #  $(CONF_DIRECTORY)/build_rule.txt and toolchain family.
    #
    @cached_property
    def SourceFileList(self):
        RetVal = []
        ToolChainTagSet = {"", TAB_STAR, self.ToolChain}
        ToolChainFamilySet = {"", TAB_STAR, self.ToolChainFamily, self.BuildRuleFamily}
        for F in self.Module.Sources:
            # match tool chain
            if F.TagName not in ToolChainTagSet:
                EdkLogger.debug(EdkLogger.DEBUG_9, "The toolchain [%s] for processing file [%s] is found, "
                                "but [%s] is currently used" % (F.TagName, str(F), self.ToolChain))
                continue
            # match tool chain family or build rule family
            if F.ToolChainFamily not in ToolChainFamilySet:
                EdkLogger.debug(
                            EdkLogger.DEBUG_0,
                            "The file [%s] must be built by tools of [%s], " \
                            "but current toolchain family is [%s], buildrule family is [%s]" \
                                % (str(F), F.ToolChainFamily, self.ToolChainFamily, self.BuildRuleFamily))
                continue

            # add the file path into search path list for file including
            if F.Dir not in self.IncludePathList:
                self.IncludePathList.insert(0, F.Dir)
            RetVal.append(F)

        self._MatchBuildRuleOrder(RetVal)

        for F in RetVal:
            self._ApplyBuildRule(F, TAB_UNKNOWN_FILE)
        return RetVal

    def _MatchBuildRuleOrder(self, FileList):
        Order_Dict = {}
        self.BuildOption
        for SingleFile in FileList:
            if self.BuildRuleOrder and SingleFile.Ext in self.BuildRuleOrder and SingleFile.Ext in self.BuildRules:
                key = SingleFile.Path.rsplit(SingleFile.Ext,1)[0]
                if key in Order_Dict:
                    Order_Dict[key].append(SingleFile.Ext)
                else:
                    Order_Dict[key] = [SingleFile.Ext]

        RemoveList = []
        for F in Order_Dict:
            if len(Order_Dict[F]) > 1:
                Order_Dict[F].sort(key=lambda i: self.BuildRuleOrder.index(i))
                for Ext in Order_Dict[F][1:]:
                    RemoveList.append(F + Ext)

        for item in RemoveList:
            FileList.remove(item)

        return FileList

    ## Return the list of unicode files
    @cached_property
    def UnicodeFileList(self):
        return self.FileTypes.get(TAB_UNICODE_FILE,[])

    ## Return the list of vfr files
    @cached_property
    def VfrFileList(self):
        return self.FileTypes.get(TAB_VFR_FILE, [])

    ## Return the list of Image Definition files
    @cached_property
    def IdfFileList(self):
        return self.FileTypes.get(TAB_IMAGE_FILE,[])

    ## Return a list of files which can be built from binary
    #
    #  "Build" binary files are just to copy them to build directory.
    #
    #   @retval     list            The list of files which can be built later
    #
    @cached_property
    def BinaryFileList(self):
        RetVal = []
        for F in self.Module.Binaries:
            if F.Target not in [TAB_ARCH_COMMON, TAB_STAR] and F.Target != self.BuildTarget:
                continue
            RetVal.append(F)
            self._ApplyBuildRule(F, F.Type, BinaryFileList=RetVal)
        return RetVal

    @cached_property
    def BuildRules(self):
        RetVal = {}
        BuildRuleDatabase = self.PlatformInfo.BuildRule
        for Type in BuildRuleDatabase.FileTypeList:
            #first try getting build rule by BuildRuleFamily
            RuleObject = BuildRuleDatabase[Type, self.BuildType, self.Arch, self.BuildRuleFamily]
            if not RuleObject:
                # build type is always module type, but ...
                if self.ModuleType != self.BuildType:
                    RuleObject = BuildRuleDatabase[Type, self.ModuleType, self.Arch, self.BuildRuleFamily]
            #second try getting build rule by ToolChainFamily
            if not RuleObject:
                RuleObject = BuildRuleDatabase[Type, self.BuildType, self.Arch, self.ToolChainFamily]
                if not RuleObject:
                    # build type is always module type, but ...
                    if self.ModuleType != self.BuildType:
                        RuleObject = BuildRuleDatabase[Type, self.ModuleType, self.Arch, self.ToolChainFamily]
            if not RuleObject:
                continue
            RuleObject = RuleObject.Instantiate(self.Macros)
            RetVal[Type] = RuleObject
            for Ext in RuleObject.SourceFileExtList:
                RetVal[Ext] = RuleObject
        return RetVal

    def _ApplyBuildRule(self, File, FileType, BinaryFileList=None):
        if self._BuildTargets is None:
            self._IntroBuildTargetList = set()
            self._FinalBuildTargetList = set()
            self._BuildTargets = defaultdict(set)
            self._FileTypes = defaultdict(set)

        if not BinaryFileList:
            BinaryFileList = self.BinaryFileList

        SubDirectory = os.path.join(self.OutputDir, File.SubDir)
        if not os.path.exists(SubDirectory):
            CreateDirectory(SubDirectory)
        LastTarget = None
        RuleChain = set()
        SourceList = [File]
        Index = 0
        #
        # Make sure to get build rule order value
        #
        self.BuildOption

        while Index < len(SourceList):
            Source = SourceList[Index]
            Index = Index + 1

            if Source != File:
                CreateDirectory(Source.Dir)

            if File.IsBinary and File == Source and File in BinaryFileList:
                # Skip all files that are not binary libraries
                if not self.IsLibrary:
                    continue
                RuleObject = self.BuildRules[TAB_DEFAULT_BINARY_FILE]
            elif FileType in self.BuildRules:
                RuleObject = self.BuildRules[FileType]
            elif Source.Ext in self.BuildRules:
                RuleObject = self.BuildRules[Source.Ext]
            else:
                # stop at no more rules
                if LastTarget:
                    self._FinalBuildTargetList.add(LastTarget)
                break

            FileType = RuleObject.SourceFileType
            self._FileTypes[FileType].add(Source)

            # stop at STATIC_LIBRARY for library
            if self.IsLibrary and FileType == TAB_STATIC_LIBRARY:
                if LastTarget:
                    self._FinalBuildTargetList.add(LastTarget)
                break

            Target = RuleObject.Apply(Source, self.BuildRuleOrder)
            if not Target:
                if LastTarget:
                    self._FinalBuildTargetList.add(LastTarget)
                break
            elif not Target.Outputs:
                # Only do build for target with outputs
                self._FinalBuildTargetList.add(Target)

            self._BuildTargets[FileType].add(Target)

            if not Source.IsBinary and Source == File:
                self._IntroBuildTargetList.add(Target)

            # to avoid cyclic rule
            if FileType in RuleChain:
                break

            RuleChain.add(FileType)
            SourceList.extend(Target.Outputs)
            LastTarget = Target
            FileType = TAB_UNKNOWN_FILE

    @cached_property
    def Targets(self):
        if self._BuildTargets is None:
            self._IntroBuildTargetList = set()
            self._FinalBuildTargetList = set()
            self._BuildTargets = defaultdict(set)
            self._FileTypes = defaultdict(set)

        #TRICK: call SourceFileList property to apply build rule for source files
        self.SourceFileList

        #TRICK: call _GetBinaryFileList to apply build rule for binary files
        self.BinaryFileList

        return self._BuildTargets

    @cached_property
    def IntroTargetList(self):
        self.Targets
        return self._IntroBuildTargetList

    @cached_property
    def CodaTargetList(self):
        self.Targets
        return self._FinalBuildTargetList

    @cached_property
    def FileTypes(self):
        self.Targets
        return self._FileTypes

    ## Get the list of package object the module depends on
    #
    #   @retval     list    The package object list
    #
    @cached_property
    def DependentPackageList(self):
        return self.Module.Packages

    ## Return the list of auto-generated code file
    #
    #   @retval     list        The list of auto-generated file
    #
    @cached_property
    def AutoGenFileList(self):
        AutoGenUniIdf = self.BuildType != 'UEFI_HII'
        UniStringBinBuffer = BytesIO()
        IdfGenBinBuffer = BytesIO()
        RetVal = {}
        AutoGenC = TemplateString()
        AutoGenH = TemplateString()
        StringH = TemplateString()
        StringIdf = TemplateString()
        GenC.CreateCode(self, AutoGenC, AutoGenH, StringH, AutoGenUniIdf, UniStringBinBuffer, StringIdf, AutoGenUniIdf, IdfGenBinBuffer)
        #
        # AutoGen.c is generated if there are library classes in inf, or there are object files
        #
        if str(AutoGenC) != "" and (len(self.Module.LibraryClasses) > 0
                                    or TAB_OBJECT_FILE in self.FileTypes):
            AutoFile = PathClass(gAutoGenCodeFileName, self.DebugDir)
            RetVal[AutoFile] = str(AutoGenC)
            self._ApplyBuildRule(AutoFile, TAB_UNKNOWN_FILE)
        if str(AutoGenH) != "":
            AutoFile = PathClass(gAutoGenHeaderFileName, self.DebugDir)
            RetVal[AutoFile] = str(AutoGenH)
            self._ApplyBuildRule(AutoFile, TAB_UNKNOWN_FILE)
        if str(StringH) != "":
            AutoFile = PathClass(gAutoGenStringFileName % {"module_name":self.Name}, self.DebugDir)
            RetVal[AutoFile] = str(StringH)
            self._ApplyBuildRule(AutoFile, TAB_UNKNOWN_FILE)
        if UniStringBinBuffer is not None and UniStringBinBuffer.getvalue() != b"":
            AutoFile = PathClass(gAutoGenStringFormFileName % {"module_name":self.Name}, self.OutputDir)
            RetVal[AutoFile] = UniStringBinBuffer.getvalue()
            AutoFile.IsBinary = True
            self._ApplyBuildRule(AutoFile, TAB_UNKNOWN_FILE)
        if UniStringBinBuffer is not None:
            UniStringBinBuffer.close()
        if str(StringIdf) != "":
            AutoFile = PathClass(gAutoGenImageDefFileName % {"module_name":self.Name}, self.DebugDir)
            RetVal[AutoFile] = str(StringIdf)
            self._ApplyBuildRule(AutoFile, TAB_UNKNOWN_FILE)
        if IdfGenBinBuffer is not None and IdfGenBinBuffer.getvalue() != b"":
            AutoFile = PathClass(gAutoGenIdfFileName % {"module_name":self.Name}, self.OutputDir)
            RetVal[AutoFile] = IdfGenBinBuffer.getvalue()
            AutoFile.IsBinary = True
            self._ApplyBuildRule(AutoFile, TAB_UNKNOWN_FILE)
        if IdfGenBinBuffer is not None:
            IdfGenBinBuffer.close()
        return RetVal

    ## Return the list of library modules explicitly or implicitly used by this module
    @cached_property
    def DependentLibraryList(self):
        # only merge library classes and PCD for non-library module
        if self.IsLibrary:
            return []
        return self.PlatformInfo.ApplyLibraryInstance(self.Module)

    ## Get the list of PCDs from current module
    #
    #   @retval     list                    The list of PCD
    #
    @cached_property
    def ModulePcdList(self):
        # apply PCD settings from platform
        RetVal = self.PlatformInfo.ApplyPcdSetting(self.Module, self.Module.Pcds)
        ExtendCopyDictionaryLists(self._PcdComments, self.Module.PcdComments)
        return RetVal

    ## Get the list of PCDs from dependent libraries
    #
    #   @retval     list                    The list of PCD
    #
    @cached_property
    def LibraryPcdList(self):
        if self.IsLibrary:
            return []
        RetVal = []
        Pcds = set()
        # get PCDs from dependent libraries
        for Library in self.DependentLibraryList:
            PcdsInLibrary = OrderedDict()
            ExtendCopyDictionaryLists(self._PcdComments, Library.PcdComments)
            for Key in Library.Pcds:
                # skip duplicated PCDs
                if Key in self.Module.Pcds or Key in Pcds:
                    continue
                Pcds.add(Key)
                PcdsInLibrary[Key] = copy.copy(Library.Pcds[Key])
            RetVal.extend(self.PlatformInfo.ApplyPcdSetting(self.Module, PcdsInLibrary, Library=Library))
        return RetVal

    ## Get the GUID value mapping
    #
    #   @retval     dict    The mapping between GUID cname and its value
    #
    @cached_property
    def GuidList(self):
        RetVal = OrderedDict(self.Module.Guids)
        for Library in self.DependentLibraryList:
            RetVal.update(Library.Guids)
            ExtendCopyDictionaryLists(self._GuidComments, Library.GuidComments)
        ExtendCopyDictionaryLists(self._GuidComments, self.Module.GuidComments)
        return RetVal

    @cached_property
    def GetGuidsUsedByPcd(self):
        RetVal = OrderedDict(self.Module.GetGuidsUsedByPcd())
        for Library in self.DependentLibraryList:
            RetVal.update(Library.GetGuidsUsedByPcd())
        return RetVal
    ## Get the protocol value mapping
    #
    #   @retval     dict    The mapping between protocol cname and its value
    #
    @cached_property
    def ProtocolList(self):
        RetVal = OrderedDict(self.Module.Protocols)
        for Library in self.DependentLibraryList:
            RetVal.update(Library.Protocols)
            ExtendCopyDictionaryLists(self._ProtocolComments, Library.ProtocolComments)
        ExtendCopyDictionaryLists(self._ProtocolComments, self.Module.ProtocolComments)
        return RetVal

    ## Get the PPI value mapping
    #
    #   @retval     dict    The mapping between PPI cname and its value
    #
    @cached_property
    def PpiList(self):
        RetVal = OrderedDict(self.Module.Ppis)
        for Library in self.DependentLibraryList:
            RetVal.update(Library.Ppis)
            ExtendCopyDictionaryLists(self._PpiComments, Library.PpiComments)
        ExtendCopyDictionaryLists(self._PpiComments, self.Module.PpiComments)
        return RetVal

    ## Get the list of include search path
    #
    #   @retval     list                    The list path
    #
    @cached_property
    def IncludePathList(self):
        RetVal = []
        RetVal.append(self.MetaFile.Dir)
        RetVal.append(self.DebugDir)

        for Package in self.Module.Packages:
            PackageDir = mws.join(self.WorkspaceDir, Package.MetaFile.Dir)
            if PackageDir not in RetVal:
                RetVal.append(PackageDir)
            IncludesList = Package.Includes
            if Package._PrivateIncludes:
                if not self.MetaFile.OriginalPath.Path.startswith(PackageDir):
                    IncludesList = list(set(Package.Includes).difference(set(Package._PrivateIncludes)))
            for Inc in IncludesList:
                if Inc not in RetVal:
                    RetVal.append(str(Inc))
        return RetVal

    @cached_property
    def IncludePathLength(self):
        return sum(len(inc)+1 for inc in self.IncludePathList)

    ## Get HII EX PCDs which maybe used by VFR
    #
    #  efivarstore used by VFR may relate with HII EX PCDs
    #  Get the variable name and GUID from efivarstore and HII EX PCD
    #  List the HII EX PCDs in As Built INF if both name and GUID match.
    #
    #  @retval    list    HII EX PCDs
    #
    def _GetPcdsMaybeUsedByVfr(self):
        if not self.SourceFileList:
            return []

        NameGuids = set()
        for SrcFile in self.SourceFileList:
            if SrcFile.Ext.lower() != '.vfr':
                continue
            Vfri = os.path.join(self.OutputDir, SrcFile.BaseName + '.i')
            if not os.path.exists(Vfri):
                continue
            VfriFile = open(Vfri, 'r')
            Content = VfriFile.read()
            VfriFile.close()
            Pos = Content.find('efivarstore')
            while Pos != -1:
                #
                # Make sure 'efivarstore' is the start of efivarstore statement
                # In case of the value of 'name' (name = efivarstore) is equal to 'efivarstore'
                #
                Index = Pos - 1
                while Index >= 0 and Content[Index] in ' \t\r\n':
                    Index -= 1
                if Index >= 0 and Content[Index] != ';':
                    Pos = Content.find('efivarstore', Pos + len('efivarstore'))
                    continue
                #
                # 'efivarstore' must be followed by name and guid
                #
                Name = gEfiVarStoreNamePattern.search(Content, Pos)
                if not Name:
                    break
                Guid = gEfiVarStoreGuidPattern.search(Content, Pos)
                if not Guid:
                    break
                NameArray = _ConvertStringToByteArray('L"' + Name.group(1) + '"')
                NameGuids.add((NameArray, GuidStructureStringToGuidString(Guid.group(1))))
                Pos = Content.find('efivarstore', Name.end())
        if not NameGuids:
            return []
        HiiExPcds = []
        for Pcd in self.PlatformInfo.Platform.Pcds.values():
            if Pcd.Type != TAB_PCDS_DYNAMIC_EX_HII:
                continue
            for SkuInfo in Pcd.SkuInfoList.values():
                Value = GuidValue(SkuInfo.VariableGuid, self.PlatformInfo.PackageList, self.MetaFile.Path)
                if not Value:
                    continue
                Name = _ConvertStringToByteArray(SkuInfo.VariableName)
                Guid = GuidStructureStringToGuidString(Value)
                if (Name, Guid) in NameGuids and Pcd not in HiiExPcds:
                    HiiExPcds.append(Pcd)
                    break

        return HiiExPcds

    def _GenOffsetBin(self):
        VfrUniBaseName = {}
        for SourceFile in self.Module.Sources:
            if SourceFile.Type.upper() == ".VFR" :
                #
                # search the .map file to find the offset of vfr binary in the PE32+/TE file.
                #
                VfrUniBaseName[SourceFile.BaseName] = (SourceFile.BaseName + "Bin")
            elif SourceFile.Type.upper() == ".UNI" :
                #
                # search the .map file to find the offset of Uni strings binary in the PE32+/TE file.
                #
                VfrUniBaseName["UniOffsetName"] = (self.Name + "Strings")

        if not VfrUniBaseName:
            return None
        MapFileName = os.path.join(self.OutputDir, self.Name + ".map")
        EfiFileName = os.path.join(self.OutputDir, self.Name + ".efi")
        VfrUniOffsetList = GetVariableOffset(MapFileName, EfiFileName, list(VfrUniBaseName.values()))
        if not VfrUniOffsetList:
            return None

        OutputName = '%sOffset.bin' % self.Name
        UniVfrOffsetFileName    =  os.path.join( self.OutputDir, OutputName)

        try:
            fInputfile = open(UniVfrOffsetFileName, "wb+", 0)
        except:
            EdkLogger.error("build", FILE_OPEN_FAILURE, "File open failed for %s" % UniVfrOffsetFileName, None)

        # Use a instance of BytesIO to cache data
        fStringIO = BytesIO()

        for Item in VfrUniOffsetList:
            if (Item[0].find("Strings") != -1):
                #
                # UNI offset in image.
                # GUID + Offset
                # { 0x8913c5e0, 0x33f6, 0x4d86, { 0x9b, 0xf1, 0x43, 0xef, 0x89, 0xfc, 0x6, 0x66 } }
                #
                UniGuid = b'\xe0\xc5\x13\x89\xf63\x86M\x9b\xf1C\xef\x89\xfc\x06f'
                fStringIO.write(UniGuid)
                UniValue = pack ('Q', int (Item[1], 16))
                fStringIO.write (UniValue)
            else:
                #
                # VFR binary offset in image.
                # GUID + Offset
                # { 0xd0bc7cb4, 0x6a47, 0x495f, { 0xaa, 0x11, 0x71, 0x7, 0x46, 0xda, 0x6, 0xa2 } };
                #
                VfrGuid = b'\xb4|\xbc\xd0Gj_I\xaa\x11q\x07F\xda\x06\xa2'
                fStringIO.write(VfrGuid)
                VfrValue = pack ('Q', int (Item[1], 16))
                fStringIO.write (VfrValue)
        #
        # write data into file.
        #
        try :
            fInputfile.write (fStringIO.getvalue())
        except:
            EdkLogger.error("build", FILE_WRITE_FAILURE, "Write data to file %s failed, please check whether the "
                            "file been locked or using by other applications." %UniVfrOffsetFileName, None)

        fStringIO.close ()
        fInputfile.close ()
        return OutputName

    ## Create AsBuilt INF file the module
    #
    def CreateAsBuiltInf(self, IsOnlyCopy = False):
        self.OutputFile = set()
        if IsOnlyCopy and GlobalData.gBinCacheDest:
            self.CopyModuleToCache()
            return

        if self.IsAsBuiltInfCreated:
            return

        # Skip the following code for libraries
        if self.IsLibrary:
            return

        # Skip the following code for modules with no source files
        if not self.SourceFileList:
            return

        # Skip the following code for modules without any binary files
        if self.BinaryFileList:
            return

        ### TODO: How to handles mixed source and binary modules

        # Find all DynamicEx and PatchableInModule PCDs used by this module and dependent libraries
        # Also find all packages that the DynamicEx PCDs depend on
        Pcds = []
        PatchablePcds = []
        Packages = []
        PcdCheckList = []
        PcdTokenSpaceList = []
        for Pcd in self.ModulePcdList + self.LibraryPcdList:
            if Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE:
                PatchablePcds.append(Pcd)
                PcdCheckList.append((Pcd.TokenCName, Pcd.TokenSpaceGuidCName, TAB_PCDS_PATCHABLE_IN_MODULE))
            elif Pcd.Type in PCD_DYNAMIC_EX_TYPE_SET:
                if Pcd not in Pcds:
                    Pcds.append(Pcd)
                    PcdCheckList.append((Pcd.TokenCName, Pcd.TokenSpaceGuidCName, TAB_PCDS_DYNAMIC_EX))
                    PcdCheckList.append((Pcd.TokenCName, Pcd.TokenSpaceGuidCName, TAB_PCDS_DYNAMIC))
                    PcdTokenSpaceList.append(Pcd.TokenSpaceGuidCName)
        GuidList = OrderedDict(self.GuidList)
        for TokenSpace in self.GetGuidsUsedByPcd:
            # If token space is not referred by patch PCD or Ex PCD, remove the GUID from GUID list
            # The GUIDs in GUIDs section should really be the GUIDs in source INF or referred by Ex an patch PCDs
            if TokenSpace not in PcdTokenSpaceList and TokenSpace in GuidList:
                GuidList.pop(TokenSpace)
        CheckList = (GuidList, self.PpiList, self.ProtocolList, PcdCheckList)
        for Package in self.DerivedPackageList:
            if Package in Packages:
                continue
            BeChecked = (Package.Guids, Package.Ppis, Package.Protocols, Package.Pcds)
            Found = False
            for Index in range(len(BeChecked)):
                for Item in CheckList[Index]:
                    if Item in BeChecked[Index]:
                        Packages.append(Package)
                        Found = True
                        break
                if Found:
                    break

        VfrPcds = self._GetPcdsMaybeUsedByVfr()
        for Pkg in self.PlatformInfo.PackageList:
            if Pkg in Packages:
                continue
            for VfrPcd in VfrPcds:
                if ((VfrPcd.TokenCName, VfrPcd.TokenSpaceGuidCName, TAB_PCDS_DYNAMIC_EX) in Pkg.Pcds or
                    (VfrPcd.TokenCName, VfrPcd.TokenSpaceGuidCName, TAB_PCDS_DYNAMIC) in Pkg.Pcds):
                    Packages.append(Pkg)
                    break

        ModuleType = SUP_MODULE_DXE_DRIVER if self.ModuleType == SUP_MODULE_UEFI_DRIVER and self.DepexGenerated else self.ModuleType
        DriverType = self.PcdIsDriver if self.PcdIsDriver else ''
        Guid = self.Guid
        MDefs = self.Module.Defines

        AsBuiltInfDict = {
          'module_name'                       : self.Name,
          'module_guid'                       : Guid,
          'module_module_type'                : ModuleType,
          'module_version_string'             : [MDefs['VERSION_STRING']] if 'VERSION_STRING' in MDefs else [],
          'pcd_is_driver_string'              : [],
          'module_uefi_specification_version' : [],
          'module_pi_specification_version'   : [],
          'module_entry_point'                : self.Module.ModuleEntryPointList,
          'module_unload_image'               : self.Module.ModuleUnloadImageList,
          'module_constructor'                : self.Module.ConstructorList,
          'module_destructor'                 : self.Module.DestructorList,
          'module_shadow'                     : [MDefs['SHADOW']] if 'SHADOW' in MDefs else [],
          'module_pci_vendor_id'              : [MDefs['PCI_VENDOR_ID']] if 'PCI_VENDOR_ID' in MDefs else [],
          'module_pci_device_id'              : [MDefs['PCI_DEVICE_ID']] if 'PCI_DEVICE_ID' in MDefs else [],
          'module_pci_class_code'             : [MDefs['PCI_CLASS_CODE']] if 'PCI_CLASS_CODE' in MDefs else [],
          'module_pci_revision'               : [MDefs['PCI_REVISION']] if 'PCI_REVISION' in MDefs else [],
          'module_build_number'               : [MDefs['BUILD_NUMBER']] if 'BUILD_NUMBER' in MDefs else [],
          'module_spec'                       : [MDefs['SPEC']] if 'SPEC' in MDefs else [],
          'module_uefi_hii_resource_section'  : [MDefs['UEFI_HII_RESOURCE_SECTION']] if 'UEFI_HII_RESOURCE_SECTION' in MDefs else [],
          'module_uni_file'                   : [MDefs['MODULE_UNI_FILE']] if 'MODULE_UNI_FILE' in MDefs else [],
          'module_arch'                       : self.Arch,
          'package_item'                      : [Package.MetaFile.File.replace('\\', '/') for Package in Packages],
          'binary_item'                       : [],
          'patchablepcd_item'                 : [],
          'pcd_item'                          : [],
          'protocol_item'                     : [],
          'ppi_item'                          : [],
          'guid_item'                         : [],
          'flags_item'                        : [],
          'libraryclasses_item'               : []
        }

        if 'MODULE_UNI_FILE' in MDefs:
            UNIFile = os.path.join(self.MetaFile.Dir, MDefs['MODULE_UNI_FILE'])
            if os.path.isfile(UNIFile):
                shutil.copy2(UNIFile, self.OutputDir)

        if self.AutoGenVersion > int(gInfSpecVersion, 0):
            AsBuiltInfDict['module_inf_version'] = '0x%08x' % self.AutoGenVersion
        else:
            AsBuiltInfDict['module_inf_version'] = gInfSpecVersion

        if DriverType:
            AsBuiltInfDict['pcd_is_driver_string'].append(DriverType)

        if 'UEFI_SPECIFICATION_VERSION' in self.Specification:
            AsBuiltInfDict['module_uefi_specification_version'].append(self.Specification['UEFI_SPECIFICATION_VERSION'])
        if 'PI_SPECIFICATION_VERSION' in self.Specification:
            AsBuiltInfDict['module_pi_specification_version'].append(self.Specification['PI_SPECIFICATION_VERSION'])

        OutputDir = self.OutputDir.replace('\\', '/').strip('/')
        DebugDir = self.DebugDir.replace('\\', '/').strip('/')
        for Item in self.CodaTargetList:
            File = Item.Target.Path.replace('\\', '/').strip('/').replace(DebugDir, '').replace(OutputDir, '').strip('/')
            self.OutputFile.add(File)
            if os.path.isabs(File):
                File = File.replace('\\', '/').strip('/').replace(OutputDir, '').strip('/')
            if Item.Target.Ext.lower() == '.aml':
                AsBuiltInfDict['binary_item'].append('ASL|' + File)
            elif Item.Target.Ext.lower() == '.acpi':
                AsBuiltInfDict['binary_item'].append('ACPI|' + File)
            elif Item.Target.Ext.lower() == '.efi':
                AsBuiltInfDict['binary_item'].append('PE32|' + self.Name + '.efi')
            else:
                AsBuiltInfDict['binary_item'].append('BIN|' + File)
        if not self.DepexGenerated:
            DepexFile = os.path.join(self.OutputDir, self.Name + '.depex')
            if os.path.exists(DepexFile):
                self.DepexGenerated = True
        if self.DepexGenerated:
            self.OutputFile.add(self.Name + '.depex')
            if self.ModuleType in [SUP_MODULE_PEIM]:
                AsBuiltInfDict['binary_item'].append('PEI_DEPEX|' + self.Name + '.depex')
            elif self.ModuleType in [SUP_MODULE_DXE_DRIVER, SUP_MODULE_DXE_RUNTIME_DRIVER, SUP_MODULE_DXE_SAL_DRIVER, SUP_MODULE_UEFI_DRIVER]:
                AsBuiltInfDict['binary_item'].append('DXE_DEPEX|' + self.Name + '.depex')
            elif self.ModuleType in [SUP_MODULE_DXE_SMM_DRIVER]:
                AsBuiltInfDict['binary_item'].append('SMM_DEPEX|' + self.Name + '.depex')

        Bin = self._GenOffsetBin()
        if Bin:
            AsBuiltInfDict['binary_item'].append('BIN|%s' % Bin)
            self.OutputFile.add(Bin)

        for Root, Dirs, Files in os.walk(OutputDir):
            for File in Files:
                if File.lower().endswith('.pdb'):
                    AsBuiltInfDict['binary_item'].append('DISPOSABLE|' + File)
                    self.OutputFile.add(File)
        HeaderComments = self.Module.HeaderComments
        StartPos = 0
        for Index in range(len(HeaderComments)):
            if HeaderComments[Index].find('@BinaryHeader') != -1:
                HeaderComments[Index] = HeaderComments[Index].replace('@BinaryHeader', '@file')
                StartPos = Index
                break
        AsBuiltInfDict['header_comments'] = '\n'.join(HeaderComments[StartPos:]).replace(':#', '://')
        AsBuiltInfDict['tail_comments'] = '\n'.join(self.Module.TailComments)

        GenList = [
            (self.ProtocolList, self._ProtocolComments, 'protocol_item'),
            (self.PpiList, self._PpiComments, 'ppi_item'),
            (GuidList, self._GuidComments, 'guid_item')
        ]
        for Item in GenList:
            for CName in Item[0]:
                Comments = '\n  '.join(Item[1][CName]) if CName in Item[1] else ''
                Entry = Comments + '\n  ' + CName if Comments else CName
                AsBuiltInfDict[Item[2]].append(Entry)
        PatchList = parsePcdInfoFromMapFile(
                            os.path.join(self.OutputDir, self.Name + '.map'),
                            os.path.join(self.OutputDir, self.Name + '.efi')
                        )
        if PatchList:
            for Pcd in PatchablePcds:
                TokenCName = Pcd.TokenCName
                for PcdItem in GlobalData.MixedPcd:
                    if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) in GlobalData.MixedPcd[PcdItem]:
                        TokenCName = PcdItem[0]
                        break
                for PatchPcd in PatchList:
                    if TokenCName == PatchPcd[0]:
                        break
                else:
                    continue
                PcdValue = ''
                if Pcd.DatumType == 'BOOLEAN':
                    BoolValue = Pcd.DefaultValue.upper()
                    if BoolValue == 'TRUE':
                        Pcd.DefaultValue = '1'
                    elif BoolValue == 'FALSE':
                        Pcd.DefaultValue = '0'

                if Pcd.DatumType in TAB_PCD_NUMERIC_TYPES:
                    HexFormat = '0x%02x'
                    if Pcd.DatumType == TAB_UINT16:
                        HexFormat = '0x%04x'
                    elif Pcd.DatumType == TAB_UINT32:
                        HexFormat = '0x%08x'
                    elif Pcd.DatumType == TAB_UINT64:
                        HexFormat = '0x%016x'
                    PcdValue = HexFormat % int(Pcd.DefaultValue, 0)
                else:
                    if Pcd.MaxDatumSize is None or Pcd.MaxDatumSize == '':
                        EdkLogger.error("build", AUTOGEN_ERROR,
                                        "Unknown [MaxDatumSize] of PCD [%s.%s]" % (Pcd.TokenSpaceGuidCName, TokenCName)
                                        )
                    ArraySize = int(Pcd.MaxDatumSize, 0)
                    PcdValue = Pcd.DefaultValue
                    if PcdValue[0] != '{':
                        Unicode = False
                        if PcdValue[0] == 'L':
                            Unicode = True
                        PcdValue = PcdValue.lstrip('L')
                        PcdValue = eval(PcdValue)
                        NewValue = '{'
                        for Index in range(0, len(PcdValue)):
                            if Unicode:
                                CharVal = ord(PcdValue[Index])
                                NewValue = NewValue + '0x%02x' % (CharVal & 0x00FF) + ', ' \
                                        + '0x%02x' % (CharVal >> 8) + ', '
                            else:
                                NewValue = NewValue + '0x%02x' % (ord(PcdValue[Index]) % 0x100) + ', '
                        Padding = '0x00, '
                        if Unicode:
                            Padding = Padding * 2
                            ArraySize = ArraySize // 2
                        if ArraySize < (len(PcdValue) + 1):
                            if Pcd.MaxSizeUserSet:
                                EdkLogger.error("build", AUTOGEN_ERROR,
                                            "The maximum size of VOID* type PCD '%s.%s' is less than its actual size occupied." % (Pcd.TokenSpaceGuidCName, TokenCName)
                                            )
                            else:
                                ArraySize = len(PcdValue) + 1
                        if ArraySize > len(PcdValue) + 1:
                            NewValue = NewValue + Padding * (ArraySize - len(PcdValue) - 1)
                        PcdValue = NewValue + Padding.strip().rstrip(',') + '}'
                    elif len(PcdValue.split(',')) <= ArraySize:
                        PcdValue = PcdValue.rstrip('}') + ', 0x00' * (ArraySize - len(PcdValue.split(',')))
                        PcdValue += '}'
                    else:
                        if Pcd.MaxSizeUserSet:
                            EdkLogger.error("build", AUTOGEN_ERROR,
                                        "The maximum size of VOID* type PCD '%s.%s' is less than its actual size occupied." % (Pcd.TokenSpaceGuidCName, TokenCName)
                                        )
                        else:
                            ArraySize = len(PcdValue) + 1
                PcdItem = '%s.%s|%s|0x%X' % \
                    (Pcd.TokenSpaceGuidCName, TokenCName, PcdValue, PatchPcd[1])
                PcdComments = ''
                if (Pcd.TokenSpaceGuidCName, Pcd.TokenCName) in self._PcdComments:
                    PcdComments = '\n  '.join(self._PcdComments[Pcd.TokenSpaceGuidCName, Pcd.TokenCName])
                if PcdComments:
                    PcdItem = PcdComments + '\n  ' + PcdItem
                AsBuiltInfDict['patchablepcd_item'].append(PcdItem)

        for Pcd in Pcds + VfrPcds:
            PcdCommentList = []
            HiiInfo = ''
            TokenCName = Pcd.TokenCName
            for PcdItem in GlobalData.MixedPcd:
                if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) in GlobalData.MixedPcd[PcdItem]:
                    TokenCName = PcdItem[0]
                    break
            if Pcd.Type == TAB_PCDS_DYNAMIC_EX_HII:
                for SkuName in Pcd.SkuInfoList:
                    SkuInfo = Pcd.SkuInfoList[SkuName]
                    HiiInfo = '## %s|%s|%s' % (SkuInfo.VariableName, SkuInfo.VariableGuid, SkuInfo.VariableOffset)
                    break
            if (Pcd.TokenSpaceGuidCName, Pcd.TokenCName) in self._PcdComments:
                PcdCommentList = self._PcdComments[Pcd.TokenSpaceGuidCName, Pcd.TokenCName][:]
            if HiiInfo:
                UsageIndex = -1
                UsageStr = ''
                for Index, Comment in enumerate(PcdCommentList):
                    for Usage in UsageList:
                        if Comment.find(Usage) != -1:
                            UsageStr = Usage
                            UsageIndex = Index
                            break
                if UsageIndex != -1:
                    PcdCommentList[UsageIndex] = '## %s %s %s' % (UsageStr, HiiInfo, PcdCommentList[UsageIndex].replace(UsageStr, ''))
                else:
                    PcdCommentList.append('## UNDEFINED ' + HiiInfo)
            PcdComments = '\n  '.join(PcdCommentList)
            PcdEntry = Pcd.TokenSpaceGuidCName + '.' + TokenCName
            if PcdComments:
                PcdEntry = PcdComments + '\n  ' + PcdEntry
            AsBuiltInfDict['pcd_item'].append(PcdEntry)
        for Item in self.BuildOption:
            if 'FLAGS' in self.BuildOption[Item]:
                AsBuiltInfDict['flags_item'].append('%s:%s_%s_%s_%s_FLAGS = %s' % (self.ToolChainFamily, self.BuildTarget, self.ToolChain, self.Arch, Item, self.BuildOption[Item]['FLAGS'].strip()))

        # Generated LibraryClasses section in comments.
        for Library in self.LibraryAutoGenList:
            AsBuiltInfDict['libraryclasses_item'].append(Library.MetaFile.File.replace('\\', '/'))

        # Generated UserExtensions TianoCore section.
        # All tianocore user extensions are copied.
        UserExtStr = ''
        for TianoCore in self._GetTianoCoreUserExtensionList():
            UserExtStr += '\n'.join(TianoCore)
            ExtensionFile = os.path.join(self.MetaFile.Dir, TianoCore[1])
            if os.path.isfile(ExtensionFile):
                shutil.copy2(ExtensionFile, self.OutputDir)
        AsBuiltInfDict['userextension_tianocore_item'] = UserExtStr

        # Generated depex expression section in comments.
        DepexExpression = self._GetDepexExpresionString()
        AsBuiltInfDict['depexsection_item'] = DepexExpression if DepexExpression else ''

        AsBuiltInf = TemplateString()
        AsBuiltInf.Append(gAsBuiltInfHeaderString.Replace(AsBuiltInfDict))

        SaveFileOnChange(os.path.join(self.OutputDir, self.Name + '.inf'), str(AsBuiltInf), False)

        self.IsAsBuiltInfCreated = True
        if GlobalData.gBinCacheDest:
            self.CopyModuleToCache()

    def CopyModuleToCache(self):
        FileDir = path.join(GlobalData.gBinCacheDest, self.PlatformInfo.OutputDir, self.BuildTarget + "_" + self.ToolChain, self.Arch, self.SourceDir, self.MetaFile.BaseName)
        CreateDirectory (FileDir)
        HashFile = path.join(self.BuildDir, self.Name + '.hash')
        if os.path.exists(HashFile):
            shutil.copy2(HashFile, FileDir)
        if not self.IsLibrary:
            ModuleFile = path.join(self.OutputDir, self.Name + '.inf')
            if os.path.exists(ModuleFile):
                shutil.copy2(ModuleFile, FileDir)
        else:
            OutputDir = self.OutputDir.replace('\\', '/').strip('/')
            DebugDir = self.DebugDir.replace('\\', '/').strip('/')
            for Item in self.CodaTargetList:
                File = Item.Target.Path.replace('\\', '/').strip('/').replace(DebugDir, '').replace(OutputDir, '').strip('/')
                self.OutputFile.add(File)
        if not self.OutputFile:
            Ma = self.BuildDatabase[self.MetaFile, self.Arch, self.BuildTarget, self.ToolChain]
            self.OutputFile = Ma.Binaries
        if self.OutputFile:
            for File in self.OutputFile:
                File = str(File)
                if not os.path.isabs(File):
                    File = os.path.join(self.OutputDir, File)
                if os.path.exists(File):
                    sub_dir = os.path.relpath(File, self.OutputDir)
                    destination_file = os.path.join(FileDir, sub_dir)
                    destination_dir = os.path.dirname(destination_file)
                    CreateDirectory(destination_dir)
                    shutil.copy2(File, destination_dir)

    def AttemptModuleCacheCopy(self):
        # If library or Module is binary do not skip by hash
        if self.IsBinaryModule:
            return False
        # .inc is contains binary information so do not skip by hash as well
        for f_ext in self.SourceFileList:
            if '.inc' in str(f_ext):
                return False
        FileDir = path.join(GlobalData.gBinCacheSource, self.PlatformInfo.OutputDir, self.BuildTarget + "_" + self.ToolChain, self.Arch, self.SourceDir, self.MetaFile.BaseName)
        HashFile = path.join(FileDir, self.Name + '.hash')
        if os.path.exists(HashFile):
            f = open(HashFile, 'r')
            CacheHash = f.read()
            f.close()
            self.GenModuleHash()
            if GlobalData.gModuleHash[self.Arch][self.Name]:
                if CacheHash == GlobalData.gModuleHash[self.Arch][self.Name]:
                    for root, dir, files in os.walk(FileDir):
                        for f in files:
                            if self.Name + '.hash' in f:
                                shutil.copy(HashFile, self.BuildDir)
                            else:
                                File = path.join(root, f)
                                sub_dir = os.path.relpath(File, FileDir)
                                destination_file = os.path.join(self.OutputDir, sub_dir)
                                destination_dir = os.path.dirname(destination_file)
                                CreateDirectory(destination_dir)
                                shutil.copy(File, destination_dir)
                    if self.Name == "PcdPeim" or self.Name == "PcdDxe":
                        CreatePcdDatabaseCode(self, TemplateString(), TemplateString())
                    return True
        return False

    ## Create makefile for the module and its dependent libraries
    #
    #   @param      CreateLibraryMakeFile   Flag indicating if or not the makefiles of
    #                                       dependent libraries will be created
    #
    @cached_class_function
    def CreateMakeFile(self, CreateLibraryMakeFile=True, GenFfsList = []):
        # nest this function inside it's only caller.
        def CreateTimeStamp():
            FileSet = {self.MetaFile.Path}

            for SourceFile in self.Module.Sources:
                FileSet.add (SourceFile.Path)

            for Lib in self.DependentLibraryList:
                FileSet.add (Lib.MetaFile.Path)

            for f in self.AutoGenDepSet:
                FileSet.add (f.Path)

            if os.path.exists (self.TimeStampPath):
                os.remove (self.TimeStampPath)
            with open(self.TimeStampPath, 'w+') as file:
                for f in FileSet:
                    print(f, file=file)

        # Ignore generating makefile when it is a binary module
        if self.IsBinaryModule:
            return

        self.GenFfsList = GenFfsList
        if not self.IsLibrary and CreateLibraryMakeFile:
            for LibraryAutoGen in self.LibraryAutoGenList:
                LibraryAutoGen.CreateMakeFile()

        if self.CanSkip():
            return

        if len(self.CustomMakefile) == 0:
            Makefile = GenMake.ModuleMakefile(self)
        else:
            Makefile = GenMake.CustomMakefile(self)
        if Makefile.Generate():
            EdkLogger.debug(EdkLogger.DEBUG_9, "Generated makefile for module %s [%s]" %
                            (self.Name, self.Arch))
        else:
            EdkLogger.debug(EdkLogger.DEBUG_9, "Skipped the generation of makefile for module %s [%s]" %
                            (self.Name, self.Arch))

        CreateTimeStamp()

    def CopyBinaryFiles(self):
        for File in self.Module.Binaries:
            SrcPath = File.Path
            DstPath = os.path.join(self.OutputDir, os.path.basename(SrcPath))
            CopyLongFilePath(SrcPath, DstPath)
    ## Create autogen code for the module and its dependent libraries
    #
    #   @param      CreateLibraryCodeFile   Flag indicating if or not the code of
    #                                       dependent libraries will be created
    #
    def CreateCodeFile(self, CreateLibraryCodeFile=True):
        if self.IsCodeFileCreated:
            return

        # Need to generate PcdDatabase even PcdDriver is binarymodule
        if self.IsBinaryModule and self.PcdIsDriver != '':
            CreatePcdDatabaseCode(self, TemplateString(), TemplateString())
            return
        if self.IsBinaryModule:
            if self.IsLibrary:
                self.CopyBinaryFiles()
            return

        if not self.IsLibrary and CreateLibraryCodeFile:
            for LibraryAutoGen in self.LibraryAutoGenList:
                LibraryAutoGen.CreateCodeFile()

        if self.CanSkip():
            return

        AutoGenList = []
        IgoredAutoGenList = []

        for File in self.AutoGenFileList:
            if GenC.Generate(File.Path, self.AutoGenFileList[File], File.IsBinary):
                AutoGenList.append(str(File))
            else:
                IgoredAutoGenList.append(str(File))


        for ModuleType in self.DepexList:
            # Ignore empty [depex] section or [depex] section for SUP_MODULE_USER_DEFINED module
            if len(self.DepexList[ModuleType]) == 0 or ModuleType == SUP_MODULE_USER_DEFINED:
                continue

            Dpx = GenDepex.DependencyExpression(self.DepexList[ModuleType], ModuleType, True)
            DpxFile = gAutoGenDepexFileName % {"module_name" : self.Name}

            if len(Dpx.PostfixNotation) != 0:
                self.DepexGenerated = True

            if Dpx.Generate(path.join(self.OutputDir, DpxFile)):
                AutoGenList.append(str(DpxFile))
            else:
                IgoredAutoGenList.append(str(DpxFile))

        if IgoredAutoGenList == []:
            EdkLogger.debug(EdkLogger.DEBUG_9, "Generated [%s] files for module %s [%s]" %
                            (" ".join(AutoGenList), self.Name, self.Arch))
        elif AutoGenList == []:
            EdkLogger.debug(EdkLogger.DEBUG_9, "Skipped the generation of [%s] files for module %s [%s]" %
                            (" ".join(IgoredAutoGenList), self.Name, self.Arch))
        else:
            EdkLogger.debug(EdkLogger.DEBUG_9, "Generated [%s] (skipped %s) files for module %s [%s]" %
                            (" ".join(AutoGenList), " ".join(IgoredAutoGenList), self.Name, self.Arch))

        self.IsCodeFileCreated = True
        return AutoGenList

    ## Summarize the ModuleAutoGen objects of all libraries used by this module
    @cached_property
    def LibraryAutoGenList(self):
        RetVal = []
        for Library in self.DependentLibraryList:
            La = ModuleAutoGen(
                        self.Workspace,
                        Library.MetaFile,
                        self.BuildTarget,
                        self.ToolChain,
                        self.Arch,
                        self.PlatformInfo.MetaFile
                        )
            if La not in RetVal:
                RetVal.append(La)
                for Lib in La.CodaTargetList:
                    self._ApplyBuildRule(Lib.Target, TAB_UNKNOWN_FILE)
        return RetVal

    def GenModuleHash(self):
        # Initialize a dictionary for each arch type
        if self.Arch not in GlobalData.gModuleHash:
            GlobalData.gModuleHash[self.Arch] = {}

        # Early exit if module or library has been hashed and is in memory
        if self.Name in GlobalData.gModuleHash[self.Arch]:
            return GlobalData.gModuleHash[self.Arch][self.Name].encode('utf-8')

        # Initialze hash object
        m = hashlib.md5()

        # Add Platform level hash
        m.update(GlobalData.gPlatformHash.encode('utf-8'))

        # Add Package level hash
        if self.DependentPackageList:
            for Pkg in sorted(self.DependentPackageList, key=lambda x: x.PackageName):
                if Pkg.PackageName in GlobalData.gPackageHash:
                    m.update(GlobalData.gPackageHash[Pkg.PackageName].encode('utf-8'))

        # Add Library hash
        if self.LibraryAutoGenList:
            for Lib in sorted(self.LibraryAutoGenList, key=lambda x: x.Name):
                if Lib.Name not in GlobalData.gModuleHash[self.Arch]:
                    Lib.GenModuleHash()
                m.update(GlobalData.gModuleHash[self.Arch][Lib.Name].encode('utf-8'))

        # Add Module self
        f = open(str(self.MetaFile), 'rb')
        Content = f.read()
        f.close()
        m.update(Content)

        # Add Module's source files
        if self.SourceFileList:
            for File in sorted(self.SourceFileList, key=lambda x: str(x)):
                f = open(str(File), 'rb')
                Content = f.read()
                f.close()
                m.update(Content)

        GlobalData.gModuleHash[self.Arch][self.Name] = m.hexdigest()

        return GlobalData.gModuleHash[self.Arch][self.Name].encode('utf-8')

    ## Decide whether we can skip the ModuleAutoGen process
    def CanSkipbyHash(self):
        # Hashing feature is off
        if not GlobalData.gUseHashCache:
            return False

        # Initialize a dictionary for each arch type
        if self.Arch not in GlobalData.gBuildHashSkipTracking:
            GlobalData.gBuildHashSkipTracking[self.Arch] = dict()

        # If library or Module is binary do not skip by hash
        if self.IsBinaryModule:
            return False

        # .inc is contains binary information so do not skip by hash as well
        for f_ext in self.SourceFileList:
            if '.inc' in str(f_ext):
                return False

        # Use Cache, if exists and if Module has a copy in cache
        if GlobalData.gBinCacheSource and self.AttemptModuleCacheCopy():
            return True

        # Early exit for libraries that haven't yet finished building
        HashFile = path.join(self.BuildDir, self.Name + ".hash")
        if self.IsLibrary and not os.path.exists(HashFile):
            return False

        # Return a Boolean based on if can skip by hash, either from memory or from IO.
        if self.Name not in GlobalData.gBuildHashSkipTracking[self.Arch]:
            # If hashes are the same, SaveFileOnChange() will return False.
            GlobalData.gBuildHashSkipTracking[self.Arch][self.Name] = not SaveFileOnChange(HashFile, self.GenModuleHash(), True)
            return GlobalData.gBuildHashSkipTracking[self.Arch][self.Name]
        else:
            return GlobalData.gBuildHashSkipTracking[self.Arch][self.Name]

    ## Decide whether we can skip the ModuleAutoGen process
    #  If any source file is newer than the module than we cannot skip
    #
    def CanSkip(self):
        if self.MakeFileDir in GlobalData.gSikpAutoGenCache:
            return True
        if not os.path.exists(self.TimeStampPath):
            return False
        #last creation time of the module
        DstTimeStamp = os.stat(self.TimeStampPath)[8]

        SrcTimeStamp = self.Workspace._SrcTimeStamp
        if SrcTimeStamp > DstTimeStamp:
            return False

        with open(self.TimeStampPath,'r') as f:
            for source in f:
                source = source.rstrip('\n')
                if not os.path.exists(source):
                    return False
                if source not in ModuleAutoGen.TimeDict :
                    ModuleAutoGen.TimeDict[source] = os.stat(source)[8]
                if ModuleAutoGen.TimeDict[source] > DstTimeStamp:
                    return False
        GlobalData.gSikpAutoGenCache.add(self.MakeFileDir)
        return True

    @cached_property
    def TimeStampPath(self):
        return os.path.join(self.MakeFileDir, 'AutoGenTimeStamp')
