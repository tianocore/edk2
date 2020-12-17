## @file
# Create makefile for MS nmake and GNU make
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
#  Copyright (c) 2020, ARM Limited. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

## Import Modules
#
from __future__ import print_function
from __future__ import absolute_import
import os.path as path
import copy
from collections import defaultdict

from .BuildEngine import BuildRule,gDefaultBuildRuleFile,AutoGenReqBuildRuleVerNum
from .GenVar import VariableMgr, var_info
from . import GenMake
from AutoGen.DataPipe import MemoryDataPipe
from AutoGen.ModuleAutoGen import ModuleAutoGen
from AutoGen.AutoGen import AutoGen
from AutoGen.AutoGen import CalculatePriorityValue
from Workspace.WorkspaceCommon import GetModuleLibInstances
from CommonDataClass.CommonClass import SkuInfoClass
from Common.caching import cached_class_function
from Common.Expression import ValueExpressionEx
from Common.StringUtils import StringToArray,NormPath
from Common.BuildToolError import *
from Common.DataType import *
from Common.Misc import *
import Common.VpdInfoFile as VpdInfoFile

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

        # MakeFileName is used to get the Makefile name and as a flag
        # indicating whether the file has been created.
        self.MakeFileName = ""

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

        self.DataPipe = MemoryDataPipe(self.BuildDir)
        self.DataPipe.FillData(self)

        return True
    def FillData_LibConstPcd(self):
        libConstPcd = {}
        for LibAuto in self.LibraryAutoGenList:
            if LibAuto.ConstPcd:
                libConstPcd[(LibAuto.MetaFile.File,LibAuto.MetaFile.Root,LibAuto.Arch,LibAuto.MetaFile.Path)] = LibAuto.ConstPcd
        self.DataPipe.DataContainer = {"LibConstPcd":libConstPcd}
    ## hash() operator of PlatformAutoGen
    #
    #  The platform file path and arch string will be used to represent
    #  hash value of this object
    #
    #   @retval   int Hash value of the platform file path and arch
    #
    @cached_class_function
    def __hash__(self):
        return hash((self.MetaFile, self.Arch,self.ToolChain,self.BuildTarget))
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
            Ma.CreateCodeFile(CreateModuleCodeFile)

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
                    Ma.CreateMakeFile(CreateModuleMakeFile, FfsCommand[key])
                else:
                    Ma.CreateMakeFile(CreateModuleMakeFile)
        self.CreateLibModuelDirs()

    def CreateLibModuelDirs(self):
        # No need to create makefile for the platform more than once.
        if self.MakeFileName:
            return

        # create library/module build dirs for platform
        Makefile = GenMake.PlatformMakefile(self)
        self.LibraryBuildDirectoryList = Makefile.GetLibraryBuildDirectoryList()
        self.ModuleBuildDirectoryList = Makefile.GetModuleBuildDirectoryList()
        self.MakeFileName = Makefile.getMakefileName()

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
        for Pcd in sorted(DynamicPcdSet):
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
                try:
                    OrgVpdFile.Read(VpdMapFilePath)
                    PcdItems = OrgVpdFile.GetOffset(PcdNvStoreDfBuffer[0])
                    NvStoreOffset = list(PcdItems.values())[0].strip() if PcdItems else '0'
                except:
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
        self.CategoryPcds()
        self.SortDynamicPcd()

    def CategoryPcds(self):
        # Category Pcds into DynamicPcds and NonDynamicPcds
        # for gathering error information
        NoDatumTypePcdList = set()
        FdfModuleList = []
        for InfName in self._AsBuildInfList:
            InfName = mws.join(self.WorkspaceDir, InfName)
            FdfModuleList.append(os.path.normpath(InfName))
        for M in self._MbList:
#            F is the Module for which M is the module autogen
            ModPcdList = self.ApplyPcdSetting(M, M.ModulePcdList)
            LibPcdList = []
            for lib in M.LibraryPcdList:
                LibPcdList.extend(self.ApplyPcdSetting(M, M.LibraryPcdList[lib], lib))
            for PcdFromModule in ModPcdList + LibPcdList:

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
        self._NonDynamicPcdList = sorted(self._NonDynaPcdList_)
        self._DynamicPcdList = self._DynaPcdList_

    def SortDynamicPcd(self):
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
                    try:
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
                    except:
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
        self._DynamicPcdList.sort()
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
        RetVal = OrderedDict()
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
                RetVal[Tool] = OrderedDict()
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
                            Def = '_'.join([self.BuildTarget, self.ToolChain, self.Arch, Tool, Attr])
                            self.Workspace.ToolDef.ToolsDefTxtDictionary[Def] = Value
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

        tool_def_file = os.path.join(self.MakeFileDir, "TOOLS_DEF." + self.Arch)
        SaveFileOnChange(tool_def_file, ToolsDef, False)
        for DllPath in DllPathList:
            os.environ["PATH"] = DllPath + os.pathsep + os.environ["PATH"]
        os.environ["MAKE_FLAGS"] = MakeFlags

        return RetVal

    ## Return the paths of tools
    @cached_property
    def ToolDefinitionFile(self):
        tool_def_file = os.path.join(self.MakeFileDir, "TOOLS_DEF." + self.Arch)
        if not os.path.exists(tool_def_file):
            self.ToolDefinition
        return tool_def_file

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
        for Mb in self._MbList:
            RetVal.update(Mb.Packages)
            for lb in Mb.LibInstances:
                RetVal.update(lb.Packages)
        #Collect package set information from INF of FDF
        for ModuleFile in self._AsBuildModuleList:
            if ModuleFile in self.Platform.Modules:
                continue
            ModuleData = self.BuildDatabase[ModuleFile, self.Arch, self.BuildTarget, self.ToolChain]
            RetVal.update(ModuleData.Packages)
        RetVal.update(self.Platform.Packages)
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
            RetVal[Pcd.TokenCName, Pcd.TokenSpaceGuidCName] = 0
        return RetVal

    @cached_property
    def _MbList(self):
        ModuleList = []
        for m in self.Platform.Modules:
            component = self.Platform.Modules[m]
            module = self.BuildDatabase[m, self.Arch, self.BuildTarget, self.ToolChain]
            module.Guid = component.Guid
            ModuleList.append(module)
        return ModuleList

    @cached_property
    def _MaList(self):
        for ModuleFile in self.Platform.Modules:
            Ma = ModuleAutoGen(
                  self.Workspace,
                  ModuleFile,
                  self.BuildTarget,
                  self.ToolChain,
                  self.Arch,
                  self.MetaFile,
                  self.DataPipe
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
    @cached_property
    def GetAllModuleInfo(self,WithoutPcd=True):
        ModuleLibs = set()
        for m in self.Platform.Modules:
            module_obj = self.BuildDatabase[m,self.Arch,self.BuildTarget,self.ToolChain]
            if not bool(module_obj.LibraryClass):
                Libs = GetModuleLibInstances(module_obj, self.Platform, self.BuildDatabase, self.Arch,self.BuildTarget,self.ToolChain,self.MetaFile,EdkLogger)
            else:
                Libs = []
            ModuleLibs.update( set([(l.MetaFile.File,l.MetaFile.Root,l.MetaFile.Path,l.MetaFile.BaseName,l.MetaFile.OriginalPath,l.Arch,True) for l in Libs]))
            if WithoutPcd and module_obj.PcdIsDriver:
                continue
            ModuleLibs.add((m.File,m.Root,m.Path,m.BaseName,m.OriginalPath,module_obj.Arch,bool(module_obj.LibraryClass)))

        return ModuleLibs

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
                    ToPcd.DefaultValue = ValueExpressionEx(ToPcd.DefaultValue, ToPcd.DatumType, self.Platform._GuidDict)(True)
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


    def GetGlobalBuildOptions(self,Module):
        ModuleTypeOptions = self.Platform.GetBuildOptionsByModuleType(EDKII_NAME, Module.ModuleType)
        ModuleTypeOptions = self._ExpandBuildOption(ModuleTypeOptions)

        if Module in self.Platform.Modules:
            PlatformModule = self.Platform.Modules[str(Module)]
            PlatformModuleOptions = self._ExpandBuildOption(PlatformModule.BuildOptions)
        else:
            PlatformModuleOptions = {}

        return ModuleTypeOptions,PlatformModuleOptions
    def ModuleGuid(self,Module):
        if os.path.basename(Module.MetaFile.File) != os.path.basename(Module.MetaFile.Path):
            #
            # Length of GUID is 36
            #
            return os.path.basename(Module.MetaFile.Path)[:36]
        return Module.Guid
    @cached_property
    def UniqueBaseName(self):
        retVal ={}
        ModuleNameDict = {}
        UniqueName = {}
        for Module in self._MbList:
            unique_base_name = '%s_%s' % (Module.BaseName,self.ModuleGuid(Module))
            if unique_base_name not in ModuleNameDict:
                ModuleNameDict[unique_base_name] = []
            ModuleNameDict[unique_base_name].append(Module.MetaFile)
            if Module.BaseName not in UniqueName:
                UniqueName[Module.BaseName] = set()
            UniqueName[Module.BaseName].add((self.ModuleGuid(Module),Module.MetaFile))
        for module_paths in ModuleNameDict.values():
            if len(set(module_paths))>1:
                samemodules = list(set(module_paths))
                EdkLogger.error("build", FILE_DUPLICATED, 'Modules have same BaseName and FILE_GUID:\n'
                                    '  %s\n  %s' % (samemodules[0], samemodules[1]))
        for name in UniqueName:
            Guid_Path = UniqueName[name]
            if len(Guid_Path) > 1:
                for guid,mpath in Guid_Path:
                    retVal[(name,mpath)] = '%s_%s' % (name,guid)
        return retVal
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

                        if CalculatePriorityValue(NowKey) > CalculatePriorityValue(NextKey):
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
