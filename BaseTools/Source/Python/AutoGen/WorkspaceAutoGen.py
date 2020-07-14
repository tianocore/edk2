## @file
# Create makefile for MS nmake and GNU make
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

## Import Modules
#
from __future__ import print_function
from __future__ import absolute_import
import os.path as path
import hashlib
from collections import defaultdict
from GenFds.FdfParser import FdfParser
from Workspace.WorkspaceCommon import GetModuleLibInstances
from AutoGen import GenMake
from AutoGen.AutoGen import AutoGen
from AutoGen.PlatformAutoGen import PlatformAutoGen
from AutoGen.BuildEngine import gDefaultBuildRuleFile
from Common.ToolDefClassObject import gDefaultToolsDefFile
from Common.StringUtils import NormPath
from Common.BuildToolError import *
from Common.DataType import *
from Common.Misc import *
import json

## Regular expression for splitting Dependency Expression string into tokens
gDepexTokenPattern = re.compile("(\(|\)|\w+| \S+\.inf)")

## Regular expression for match: PCD(xxxx.yyy)
gPCDAsGuidPattern = re.compile(r"^PCD\(.+\..+\)$")

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

        self.MergeArch()
        self.ValidateBuildTarget()

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
        #
        # Mark now build in AutoGen Phase
        #
        #
        # Collect Platform Guids to support Guid name in Fdfparser.
        #
        self.CollectPlatformGuids()
        GlobalData.gAutoGenPhase = True
        self.ProcessModuleFromPdf()
        self.ProcessPcdType()
        self.ProcessMixedPcd()
        self.VerifyPcdsFromFDF()
        self.CollectAllPcds()
        for Pa in self.AutoGenObjectList:
            Pa.FillData_LibConstPcd()
        self.GeneratePkgLevelHash()
        #
        # Check PCDs token value conflict in each DEC file.
        #
        self._CheckAllPcdsTokenValueConflict()
        #
        # Check PCD type and definition between DSC and DEC
        #
        self._CheckPcdDefineAndType()

        self.CreateBuildOptionsFile()
        self.CreatePcdTokenNumberFile()
        self.GeneratePlatformLevelHash()

    #
    # Merge Arch
    #
    def MergeArch(self):
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
    def ValidateBuildTarget(self):
        if self.BuildTarget not in self.Platform.BuildTargets:
            EdkLogger.error("build", PARAMETER_INVALID,
                            ExtraData="Build target [%s] is not supported by the platform. [Valid target: %s]"
                                      % (self.BuildTarget, " ".join(self.Platform.BuildTargets)))

    def CollectPlatformGuids(self):
        oriInfList = []
        oriPkgSet = set()
        PlatformPkg = set()
        for Arch in self.ArchList:
            Platform = self.BuildDatabase[self.MetaFile, Arch, self.BuildTarget, self.ToolChain]
            oriInfList = Platform.Modules
            for ModuleFile in oriInfList:
                ModuleData = self.BuildDatabase[ModuleFile, Platform._Arch, Platform._Target, Platform._Toolchain]
                oriPkgSet.update(ModuleData.Packages)
                for Pkg in oriPkgSet:
                    Guids = Pkg.Guids
                    GlobalData.gGuidDict.update(Guids)
            if Platform.Packages:
                PlatformPkg.update(Platform.Packages)
                for Pkg in PlatformPkg:
                    Guids = Pkg.Guids
                    GlobalData.gGuidDict.update(Guids)

    @cached_property
    def FdfProfile(self):
        if not self.FdfFile:
            self.FdfFile = self.Platform.FlashDefinition

        FdfProfile = None
        if self.FdfFile:
            Fdf = FdfParser(self.FdfFile.Path)
            Fdf.ParseFile()
            GlobalData.gFdfParser = Fdf
            if Fdf.CurrentFdName and Fdf.CurrentFdName in Fdf.Profile.FdDict:
                FdDict = Fdf.Profile.FdDict[Fdf.CurrentFdName]
                for FdRegion in FdDict.RegionList:
                    if str(FdRegion.RegionType) == 'FILE' and self.Platform.VpdToolGuid in str(FdRegion.RegionDataList):
                        if int(FdRegion.Offset) % 8 != 0:
                            EdkLogger.error("build", FORMAT_INVALID, 'The VPD Base Address %s must be 8-byte aligned.' % (FdRegion.Offset))
            FdfProfile = Fdf.Profile
        else:
            if self.FdTargetList:
                EdkLogger.info("No flash definition file found. FD [%s] will be ignored." % " ".join(self.FdTargetList))
                self.FdTargetList = []
            if self.FvTargetList:
                EdkLogger.info("No flash definition file found. FV [%s] will be ignored." % " ".join(self.FvTargetList))
                self.FvTargetList = []
            if self.CapTargetList:
                EdkLogger.info("No flash definition file found. Capsule [%s] will be ignored." % " ".join(self.CapTargetList))
                self.CapTargetList = []

        return FdfProfile

    def ProcessModuleFromPdf(self):

        if self.FdfProfile:
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
                        Current_Platform_cache = self.BuildDatabase[self.MetaFile, Arch, self.BuildTarget, self.ToolChain]
                        for Pkey in Current_Platform_cache.Modules:
                            MetaFile_cache[Arch].add(Current_Platform_cache.Modules[Pkey].MetaFile)
                    for Inf in self.FdfProfile.InfDict[key]:
                        ModuleFile = PathClass(NormPath(Inf), GlobalData.gWorkspace, Arch)
                        for Arch in self.ArchList:
                            if ModuleFile in MetaFile_cache[Arch]:
                                break
                        else:
                            ModuleData = self.BuildDatabase[ModuleFile, Arch, self.BuildTarget, self.ToolChain]
                            if not ModuleData.IsBinaryModule:
                                EdkLogger.error('build', PARSER_ERROR, "Module %s NOT found in DSC file; Is it really a binary module?" % ModuleFile)

                else:
                    for Arch in self.ArchList:
                        if Arch == key:
                            Platform = self.BuildDatabase[self.MetaFile, Arch, self.BuildTarget, self.ToolChain]
                            MetaFileList = set()
                            for Pkey in Platform.Modules:
                                MetaFileList.add(Platform.Modules[Pkey].MetaFile)
                            for Inf in self.FdfProfile.InfDict[key]:
                                ModuleFile = PathClass(NormPath(Inf), GlobalData.gWorkspace, Arch)
                                if ModuleFile in MetaFileList:
                                    continue
                                ModuleData = self.BuildDatabase[ModuleFile, Arch, self.BuildTarget, self.ToolChain]
                                if not ModuleData.IsBinaryModule:
                                    EdkLogger.error('build', PARSER_ERROR, "Module %s NOT found in DSC file; Is it really a binary module?" % ModuleFile)



    # parse FDF file to get PCDs in it, if any
    def VerifyPcdsFromFDF(self):

        if self.FdfProfile:
            PcdSet = self.FdfProfile.PcdDict
            self.VerifyPcdDeclearation(PcdSet)

    def ProcessPcdType(self):
        for Arch in self.ArchList:
            Platform = self.BuildDatabase[self.MetaFile, Arch, self.BuildTarget, self.ToolChain]
            Platform.Pcds
            # generate the SourcePcdDict and BinaryPcdDict
            Libs = []
            for BuildData in list(self.BuildDatabase._CACHE_.values()):
                if BuildData.Arch != Arch:
                    continue
                if BuildData.MetaFile.Ext == '.inf' and str(BuildData) in Platform.Modules :
                    Libs.extend(GetModuleLibInstances(BuildData, Platform,
                                     self.BuildDatabase,
                                     Arch,
                                     self.BuildTarget,
                                     self.ToolChain,
                                     self.Platform.MetaFile,
                                     EdkLogger
                                     ))
            for BuildData in list(self.BuildDatabase._CACHE_.values()):
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
                                    if bool(BuildData.LibraryClass):
                                        if BuildData in set(Libs):
                                            ReferenceModules = BuildData.ReferenceModules
                                            for ReferenceModule in ReferenceModules:
                                                if ReferenceModule.MetaFile in Platform.Modules:
                                                    RefPlatformModule = Platform.Modules[str(ReferenceModule.MetaFile)]
                                                    if key in RefPlatformModule.Pcds:
                                                        PcdInReferenceModule = RefPlatformModule.Pcds[key]
                                                        if PcdInReferenceModule.Type:
                                                            BuildData.Pcds[key].Type = PcdInReferenceModule.Type
                                                            BuildData.Pcds[key].Pending = False
                                                            break

    def ProcessMixedPcd(self):
        for Arch in self.ArchList:
            SourcePcdDict = {TAB_PCDS_DYNAMIC_EX:set(), TAB_PCDS_PATCHABLE_IN_MODULE:set(),TAB_PCDS_DYNAMIC:set(),TAB_PCDS_FIXED_AT_BUILD:set()}
            BinaryPcdDict = {TAB_PCDS_DYNAMIC_EX:set(), TAB_PCDS_PATCHABLE_IN_MODULE:set()}
            SourcePcdDict_Keys = SourcePcdDict.keys()
            BinaryPcdDict_Keys = BinaryPcdDict.keys()

            # generate the SourcePcdDict and BinaryPcdDict

            for BuildData in list(self.BuildDatabase._CACHE_.values()):
                if BuildData.Arch != Arch:
                    continue
                if BuildData.MetaFile.Ext == '.inf':
                    for key in BuildData.Pcds:
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

            BuildData = self.BuildDatabase[self.MetaFile, Arch, self.BuildTarget, self.ToolChain]
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

        if self.FdfProfile:
            PcdSet = self.FdfProfile.PcdDict
            # handle the mixed pcd in FDF file
            for key in PcdSet:
                if key in GlobalData.MixedPcd:
                    Value = PcdSet[key]
                    del PcdSet[key]
                    for item in GlobalData.MixedPcd[key]:
                        PcdSet[item] = Value

    #Collect package set information from INF of FDF
    @cached_property
    def PkgSet(self):
        if not self.FdfFile:
            self.FdfFile = self.Platform.FlashDefinition

        if self.FdfFile:
            ModuleList = self.FdfProfile.InfList
        else:
            ModuleList = []
        Pkgs = {}
        for Arch in self.ArchList:
            Platform = self.BuildDatabase[self.MetaFile, Arch, self.BuildTarget, self.ToolChain]
            PkgSet = set()
            for mb in [self.BuildDatabase[m, Arch, self.BuildTarget, self.ToolChain] for m in Platform.Modules]:
                PkgSet.update(mb.Packages)
            for Inf in ModuleList:
                ModuleFile = PathClass(NormPath(Inf), GlobalData.gWorkspace, Arch)
                if ModuleFile in Platform.Modules:
                    continue
                ModuleData = self.BuildDatabase[ModuleFile, Arch, self.BuildTarget, self.ToolChain]
                PkgSet.update(ModuleData.Packages)
            PkgSet.update(Platform.Packages)
            Pkgs[Arch] = list(PkgSet)
        return Pkgs

    def VerifyPcdDeclearation(self,PcdSet):
        for Arch in self.ArchList:
            Platform = self.BuildDatabase[self.MetaFile, Arch, self.BuildTarget, self.ToolChain]
            Pkgs = self.PkgSet[Arch]
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
    def CollectAllPcds(self):

        for Arch in self.ArchList:
            Pa = PlatformAutoGen(self, self.MetaFile, self.BuildTarget, self.ToolChain, Arch)
            #
            # Explicitly collect platform's dynamic PCDs
            #
            Pa.CollectPlatformDynamicPcds()
            Pa.CollectFixedAtBuildPcds()
            self.AutoGenObjectList.append(Pa)
        # We need to calculate the PcdTokenNumber after all Arch Pcds are collected.
        for Arch in self.ArchList:
            #Pcd TokenNumber
            Pa = PlatformAutoGen(self, self.MetaFile, self.BuildTarget, self.ToolChain, Arch)
            self.UpdateModuleDataPipe(Arch,  {"PCD_TNUM":Pa.PcdTokenNumber})

    def UpdateModuleDataPipe(self,arch, attr_dict):
        for (Target, Toolchain, Arch, MetaFile) in AutoGen.Cache():
            if Arch != arch:
                continue
            try:
                AutoGen.Cache()[(Target, Toolchain, Arch, MetaFile)].DataPipe.DataContainer = attr_dict
            except Exception:
                pass
    #
    # Generate Package level hash value
    #
    def GeneratePkgLevelHash(self):
        for Arch in self.ArchList:
            GlobalData.gPackageHash = {}
            if GlobalData.gUseHashCache:
                for Pkg in self.PkgSet[Arch]:
                    self._GenPkgLevelHash(Pkg)


    def CreateBuildOptionsFile(self):
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

    def CreatePcdTokenNumberFile(self):
        #
        # Create PcdToken Number file for Dynamic/DynamicEx Pcd.
        #
        PcdTokenNumber = 'PcdTokenNumber: '
        Pa = self.AutoGenObjectList[0]
        if Pa.PcdTokenNumber:
            if Pa.DynamicPcdList:
                for Pcd in Pa.DynamicPcdList:
                    PcdTokenNumber += TAB_LINE_BREAK
                    PcdTokenNumber += str((Pcd.TokenCName, Pcd.TokenSpaceGuidCName))
                    PcdTokenNumber += ' : '
                    PcdTokenNumber += str(Pa.PcdTokenNumber[Pcd.TokenCName, Pcd.TokenSpaceGuidCName])
        SaveFileOnChange(os.path.join(self.BuildDir, 'PcdTokenNumber'), PcdTokenNumber, False)

    def GeneratePlatformLevelHash(self):
        #
        # Get set of workspace metafiles
        #
        AllWorkSpaceMetaFiles = self._GetMetaFiles(self.BuildTarget, self.ToolChain)
        AllWorkSpaceMetaFileList = sorted(AllWorkSpaceMetaFiles, key=lambda x: str(x))
        #
        # Retrieve latest modified time of all metafiles
        #
        SrcTimeStamp = 0
        for f in AllWorkSpaceMetaFiles:
            if os.stat(f)[8] > SrcTimeStamp:
                SrcTimeStamp = os.stat(f)[8]
        self._SrcTimeStamp = SrcTimeStamp

        if GlobalData.gUseHashCache:
            FileList = []
            m = hashlib.md5()
            for file in AllWorkSpaceMetaFileList:
                if file.endswith('.dec'):
                    continue
                f = open(file, 'rb')
                Content = f.read()
                f.close()
                m.update(Content)
                FileList.append((str(file), hashlib.md5(Content).hexdigest()))

            HashDir = path.join(self.BuildDir, "Hash_Platform")
            HashFile = path.join(HashDir, 'Platform.hash.' + m.hexdigest())
            SaveFileOnChange(HashFile, m.hexdigest(), False)
            HashChainFile = path.join(HashDir, 'Platform.hashchain.' + m.hexdigest())
            GlobalData.gPlatformHashFile = HashChainFile
            try:
                with open(HashChainFile, 'w') as f:
                    json.dump(FileList, f, indent=2)
            except:
                EdkLogger.quiet("[cache warning]: fail to save hashchain file:%s" % HashChainFile)

            if GlobalData.gBinCacheDest:
                # Copy platform hash files to cache destination
                FileDir = path.join(GlobalData.gBinCacheDest, self.OutputDir, self.BuildTarget + "_" + self.ToolChain, "Hash_Platform")
                CacheFileDir = FileDir
                CreateDirectory(CacheFileDir)
                CopyFileOnChange(HashFile, CacheFileDir)
                CopyFileOnChange(HashChainFile, CacheFileDir)

        #
        # Write metafile list to build directory
        #
        AutoGenFilePath = os.path.join(self.BuildDir, 'AutoGen')
        if os.path.exists (AutoGenFilePath):
            os.remove(AutoGenFilePath)
        if not os.path.exists(self.BuildDir):
            os.makedirs(self.BuildDir)
        with open(os.path.join(self.BuildDir, 'AutoGen'), 'w+') as file:
            for f in AllWorkSpaceMetaFileList:
                print(f, file=file)
        return True

    def _GenPkgLevelHash(self, Pkg):
        if Pkg.PackageName in GlobalData.gPackageHash:
            return

        PkgDir = os.path.join(self.BuildDir, Pkg.Arch, "Hash_Pkg", Pkg.PackageName)
        CreateDirectory(PkgDir)
        FileList = []
        m = hashlib.md5()
        # Get .dec file's hash value
        f = open(Pkg.MetaFile.Path, 'rb')
        Content = f.read()
        f.close()
        m.update(Content)
        FileList.append((str(Pkg.MetaFile.Path), hashlib.md5(Content).hexdigest()))
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
                        FileList.append((str(File_Path), hashlib.md5(Content).hexdigest()))
        GlobalData.gPackageHash[Pkg.PackageName] = m.hexdigest()

        HashDir = PkgDir
        HashFile = path.join(HashDir, Pkg.PackageName + '.hash.' + m.hexdigest())
        SaveFileOnChange(HashFile, m.hexdigest(), False)
        HashChainFile = path.join(HashDir, Pkg.PackageName + '.hashchain.' + m.hexdigest())
        GlobalData.gPackageHashFile[(Pkg.PackageName, Pkg.Arch)] = HashChainFile
        try:
            with open(HashChainFile, 'w') as f:
                json.dump(FileList, f, indent=2)
        except:
            EdkLogger.quiet("[cache warning]: fail to save hashchain file:%s" % HashChainFile)

        if GlobalData.gBinCacheDest:
            # Copy Pkg hash files to cache destination dir
            FileDir = path.join(GlobalData.gBinCacheDest, self.OutputDir, self.BuildTarget + "_" + self.ToolChain, Pkg.Arch, "Hash_Pkg", Pkg.PackageName)
            CacheFileDir = FileDir
            CreateDirectory(CacheFileDir)
            CopyFileOnChange(HashFile, CacheFileDir)
            CopyFileOnChange(HashChainFile, CacheFileDir)

    def _GetMetaFiles(self, Target, Toolchain):
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

        for Pa in self.AutoGenObjectList:
            AllWorkSpaceMetaFiles.add(Pa.ToolDefinitionFile)

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

        FdsCommandDict["GenfdsMultiThread"] = GlobalData.gEnableGenfdsMultiThread
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
            Pa.CreateMakeFile(CreateDepsMakeFile)

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
            Pa.CreateCodeFile(CreateDepsCodeFile)

    ## Create AsBuilt INF file the platform
    #
    def CreateAsBuiltInf(self):
        return

