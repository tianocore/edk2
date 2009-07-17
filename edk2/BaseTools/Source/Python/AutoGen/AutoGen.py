## @file
# Generate AutoGen.h, AutoGen.c and *.depex files
#
# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

## Import Modules
#
import os
import re
import os.path as path
import copy

import GenC
import GenMake
import GenDepex

from StrGather import *
from BuildEngine import BuildRule

from Common.BuildToolError import *
from Common.DataType import *
from Common.Misc import *
from Common.String import *
import Common.GlobalData as GlobalData
from GenFds.FdfParser import *
from CommonDataClass.CommonClass import SkuInfoClass
from Workspace.BuildClassObject import *

## Regular expression for splitting Dependency Expression stirng into tokens
gDepexTokenPattern = re.compile("(\(|\)|\w+| \S+\.inf)")

## Mapping Makefile type
gMakeTypeMap = {"MSFT":"nmake", "GCC":"gmake"}


## Build rule configuration file
gBuildRuleFile = 'Conf/build_rule.txt'

## default file name for AutoGen
gAutoGenCodeFileName = "AutoGen.c"
gAutoGenHeaderFileName = "AutoGen.h"
gAutoGenStringFileName = "%(module_name)sStrDefs.h"
gAutoGenDepexFileName = "%(module_name)s.depex"
gAutoGenSmmDepexFileName = "%(module_name)s.smm"

## Base class for AutoGen
#
#   This class just implements the cache mechanism of AutoGen objects.
#
class AutoGen(object):
    # database to maintain the objects of xxxAutoGen
    _CACHE_ = {}    # (BuildTarget, ToolChain) : {ARCH : {platform file: AutoGen object}}}

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
    def __new__(Class, Workspace, MetaFile, Target, Toolchain, Arch, *args, **kwargs):
        # check if the object has been created
        Key = (Target, Toolchain)
        if Key not in Class._CACHE_ or Arch not in Class._CACHE_[Key] \
           or MetaFile not in Class._CACHE_[Key][Arch]:
            AutoGenObject = super(AutoGen, Class).__new__(Class)
            # call real constructor
            if not AutoGenObject._Init(Workspace, MetaFile, Target, Toolchain, Arch, *args, **kwargs):
                return None
            if Key not in Class._CACHE_:
                Class._CACHE_[Key] = {}
            if Arch not in Class._CACHE_[Key]:
                Class._CACHE_[Key][Arch] = {}
            Class._CACHE_[Key][Arch][MetaFile] = AutoGenObject
        else:
            AutoGenObject = Class._CACHE_[Key][Arch][MetaFile]

        return AutoGenObject

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
    ## Real constructor of WorkspaceAutoGen
    #
    # This method behaves the same as __init__ except that it needs explict invoke
    # (in super class's __new__ method)
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
    #   @param  SkuId                   SKU id from command line
    #
    def _Init(self, WorkspaceDir, ActivePlatform, Target, Toolchain, ArchList, MetaFileDb,
              BuildConfig, ToolDefinition, FlashDefinitionFile='', Fds=[], Fvs=[], SkuId=''):
        self.MetaFile       = ActivePlatform.MetaFile
        self.WorkspaceDir   = WorkspaceDir
        self.Platform       = ActivePlatform
        self.BuildTarget    = Target
        self.ToolChain      = Toolchain
        self.ArchList       = ArchList
        self.SkuId          = SkuId

        self.BuildDatabase  = MetaFileDb
        self.TargetTxt      = BuildConfig
        self.ToolDef        = ToolDefinition
        self.FdfFile        = FlashDefinitionFile
        self.FdTargetList   = Fds
        self.FvTargetList   = Fvs
        self.AutoGenObjectList = []

        # there's many relative directory operations, so ...
        os.chdir(self.WorkspaceDir)

        # parse FDF file to get PCDs in it, if any
        if self.FdfFile != None and self.FdfFile != '':
            Fdf = FdfParser(self.FdfFile.Path)
            Fdf.ParseFile()
            PcdSet = Fdf.Profile.PcdDict
            ModuleList = Fdf.Profile.InfList
        else:
            PcdSet = {}
            ModuleList = []

        # apply SKU and inject PCDs from Flash Definition file
        for Arch in self.ArchList:
            Platform = self.BuildDatabase[self.MetaFile, Arch]
            Platform.SkuName = self.SkuId
            for Name, Guid in PcdSet:
                Platform.AddPcd(Name, Guid, PcdSet[Name, Guid])

            Pa = PlatformAutoGen(self, self.MetaFile, Target, Toolchain, Arch)
            #
            # Explicitly collect platform's dynamic PCDs
            #
            Pa.CollectPlatformDynamicPcds()
            self.AutoGenObjectList.append(Pa)

        self._BuildDir = None
        self._FvDir = None
        self._MakeFileDir = None
        self._BuildCommand = None

        return True

    def __repr__(self):
        return "%s [%s]" % (self.MetaFile, ", ".join(self.ArchList))

    ## Return the directory to store FV files
    def _GetFvDir(self):
        if self._FvDir == None:
            self._FvDir = path.join(self.BuildDir, 'FV')
        return self._FvDir

    ## Return the directory to store all intermediate and final files built
    def _GetBuildDir(self):
        return self.AutoGenObjectList[0].BuildDir

    ## Return the build output directory platform specifies
    def _GetOutputDir(self):
        return self.Platform.OutputDirectory

    ## Return platform name
    def _GetName(self):
        return self.Platform.PlatformName

    ## Return meta-file GUID
    def _GetGuid(self):
        return self.Platform.Guid

    ## Return platform version
    def _GetVersion(self):
        return self.Platform.Version

    ## Return paths of tools
    def _GetToolDefinition(self):
        return self.AutoGenObjectList[0].ToolDefinition

    ## Return directory of platform makefile
    #
    #   @retval     string  Makefile directory
    #
    def _GetMakeFileDir(self):
        if self._MakeFileDir == None:
            self._MakeFileDir = self.BuildDir
        return self._MakeFileDir

    ## Return build command string
    #
    #   @retval     string  Build command string
    #
    def _GetBuildCommand(self):
        if self._BuildCommand == None:
            # BuildCommand should be all the same. So just get one from platform AutoGen
            self._BuildCommand = self.AutoGenObjectList[0].BuildCommand
        return self._BuildCommand

    ## Create makefile for the platform and mdoules in it
    #
    #   @param      CreateDepsMakeFile      Flag indicating if the makefile for
    #                                       modules will be created as well
    #
    def CreateMakeFile(self, CreateDepsMakeFile=False):
        # create makefile for platform
        Makefile = GenMake.TopLevelMakefile(self)
        if Makefile.Generate():
            EdkLogger.debug(EdkLogger.DEBUG_9, "Generated makefile for platform [%s] %s\n" %
                            (self.MetaFile, self.ArchList))
        else:
            EdkLogger.debug(EdkLogger.DEBUG_9, "Skipped the generation of makefile for platform [%s] %s\n" %
                            (self.MetaFile, self.ArchList))

        if CreateDepsMakeFile:
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

    Name                = property(_GetName)
    Guid                = property(_GetGuid)
    Version             = property(_GetVersion)
    OutputDir           = property(_GetOutputDir)

    ToolDefinition      = property(_GetToolDefinition)       # toolcode : tool path

    BuildDir            = property(_GetBuildDir)
    FvDir               = property(_GetFvDir)
    MakeFileDir         = property(_GetMakeFileDir)
    BuildCommand        = property(_GetBuildCommand)

## AutoGen class for platform
#
#  PlatformAutoGen class will process the original information in platform
#  file in order to generate makefile for platform.
#
class PlatformAutoGen(AutoGen):
    #
    # Used to store all PCDs for both PEI and DXE phase, in order to generate 
    # correct PCD database
    # 
    _DynaPcdList_ = []
    _NonDynaPcdList_ = []

    ## The real constructor of PlatformAutoGen
    #
    #  This method is not supposed to be called by users of PlatformAutoGen. It's
    #  only used by factory method __new__() to do real initialization work for an
    #  object of PlatformAutoGen
    #
    #   @param      Workspace       WorkspaceAutoGen object
    #   @param      PlatformFile    Platform file (DSC file)
    #   @param      Target          Build target (DEBUG, RELEASE)
    #   @param      Toolchain       Name of tool chain
    #   @param      Arch            arch of the platform supports
    #
    def _Init(self, Workspace, PlatformFile, Target, Toolchain, Arch):
        EdkLogger.debug(EdkLogger.DEBUG_9, "AutoGen platform [%s] [%s]" % (PlatformFile, Arch))
        GlobalData.gProcessingFile = "%s [%s, %s, %s]" % (PlatformFile, Arch, Toolchain, Target)

        self.MetaFile = PlatformFile
        self.Workspace = Workspace
        self.WorkspaceDir = Workspace.WorkspaceDir
        self.ToolChain = Toolchain
        self.BuildTarget = Target
        self.Arch = Arch
        self.SourceDir = PlatformFile.SubDir
        self.SourceOverrideDir = None
        self.FdTargetList = self.Workspace.FdTargetList
        self.FvTargetList = self.Workspace.FvTargetList

        # flag indicating if the makefile/C-code file has been created or not
        self.IsMakeFileCreated  = False
        self.IsCodeFileCreated  = False

        self._Platform   = None
        self._Name       = None
        self._Guid       = None
        self._Version    = None

        self._BuildRule = None
        self._SourceDir = None
        self._BuildDir = None
        self._OutputDir = None
        self._FvDir = None
        self._MakeFileDir = None
        self._FdfFile = None

        self._PcdTokenNumber = None    # (TokenCName, TokenSpaceGuidCName) : GeneratedTokenNumber
        self._DynamicPcdList = None    # [(TokenCName1, TokenSpaceGuidCName1), (TokenCName2, TokenSpaceGuidCName2), ...]
        self._NonDynamicPcdList = None # [(TokenCName1, TokenSpaceGuidCName1), (TokenCName2, TokenSpaceGuidCName2), ...]

        self._ToolDefinitions = None
        self._ToolDefFile = None          # toolcode : tool path
        self._ToolChainFamily = None
        self._BuildRuleFamily = None
        self._BuildOption = None          # toolcode : option
        self._PackageList = None
        self._ModuleAutoGenList  = None
        self._LibraryAutoGenList = None
        self._BuildCommand = None

        # get the original module/package/platform objects
        self.BuildDatabase = Workspace.BuildDatabase
        return True

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
    def CreateCodeFile(self, CreateModuleCodeFile=False):
        # only module has code to be greated, so do nothing if CreateModuleCodeFile is False
        if self.IsCodeFileCreated or not CreateModuleCodeFile:
            return

        for Ma in self.ModuleAutoGenList:
            Ma.CreateCodeFile(True)

        # don't do this twice
        self.IsCodeFileCreated = True

    ## Create makefile for the platform and mdoules in it
    #
    #   @param      CreateModuleMakeFile    Flag indicating if the makefile for
    #                                       modules will be created as well
    #
    def CreateMakeFile(self, CreateModuleMakeFile=False):
        if CreateModuleMakeFile:
            for ModuleFile in self.Platform.Modules:
                Ma = ModuleAutoGen(self.Workspace, ModuleFile, self.BuildTarget,
                                   self.ToolChain, self.Arch, self.MetaFile)
                Ma.CreateMakeFile(True)

        # no need to create makefile for the platform more than once
        if self.IsMakeFileCreated:
            return

        # create makefile for platform
        Makefile = GenMake.PlatformMakefile(self)
        if Makefile.Generate():
            EdkLogger.debug(EdkLogger.DEBUG_9, "Generated makefile for platform [%s] [%s]\n" %
                            (self.MetaFile, self.Arch))
        else:
            EdkLogger.debug(EdkLogger.DEBUG_9, "Skipped the generation of makefile for platform [%s] [%s]\n" %
                            (self.MetaFile, self.Arch))
        self.IsMakeFileCreated = True

    ## Collect dynamic PCDs
    #
    #  Gather dynamic PCDs list from each module and their settings from platform
    #  This interface should be invoked explicitly when platform action is created.
    #
    def CollectPlatformDynamicPcds(self):
        # for gathering error information
        NoDatumTypePcdList = set()

        self._GuidValue = {}
        for F in self.Platform.Modules.keys():
            M = ModuleAutoGen(self.Workspace, F, self.BuildTarget, self.ToolChain, self.Arch, self.MetaFile)
            #GuidValue.update(M.Guids)
            for PcdFromModule in M.ModulePcdList+M.LibraryPcdList:
                # make sure that the "VOID*" kind of datum has MaxDatumSize set
                if PcdFromModule.DatumType == "VOID*" and PcdFromModule.MaxDatumSize == None:
                    NoDatumTypePcdList.add("%s.%s [%s]" % (PcdFromModule.TokenSpaceGuidCName, PcdFromModule.TokenCName, F))

                if PcdFromModule.Type in GenC.gDynamicPcd or PcdFromModule.Type in GenC.gDynamicExPcd:
                    #
                    # If a dynamic PCD used by a PEM module/PEI module & DXE module,
                    # it should be stored in Pcd PEI database, If a dynamic only
                    # used by DXE module, it should be stored in DXE PCD database.
                    # The default Phase is DXE
                    #
                    if M.ModuleType in ["PEIM", "PEI_CORE"]:
                        PcdFromModule.Phase = "PEI"
                    if PcdFromModule not in self._DynaPcdList_:
                        self._DynaPcdList_.append(PcdFromModule)
                    elif PcdFromModule.Phase == 'PEI':
                        # overwrite any the same PCD existing, if Phase is PEI
                        Index = self._DynaPcdList_.index(PcdFromModule)
                        self._DynaPcdList_[Index] = PcdFromModule
                elif PcdFromModule not in self._NonDynaPcdList_:
                    self._NonDynaPcdList_.append(PcdFromModule)

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
        UnicodePcdArray = []
        HiiPcdArray     = []
        OtherPcdArray   = []
        for Pcd in self._DynamicPcdList:
            # just pick the a value to determine whether is unicode string type
            Sku      = Pcd.SkuInfoList[Pcd.SkuInfoList.keys()[0]]
            PcdValue = Sku.DefaultValue
            if Pcd.DatumType == 'VOID*' and PcdValue.startswith("L"):
                # if found PCD which datum value is unicode string the insert to left size of UnicodeIndex
                UnicodePcdArray.append(Pcd)
            elif len(Sku.VariableName) > 0:
                # if found HII type PCD then insert to right of UnicodeIndex
                HiiPcdArray.append(Pcd)
            else:
                OtherPcdArray.append(Pcd)
        del self._DynamicPcdList[:]
        self._DynamicPcdList.extend(UnicodePcdArray)
        self._DynamicPcdList.extend(HiiPcdArray)
        self._DynamicPcdList.extend(OtherPcdArray)
            
        
    ## Return the platform build data object
    def _GetPlatform(self):
        if self._Platform == None:
            self._Platform = self.BuildDatabase[self.MetaFile, self.Arch]
        return self._Platform

    ## Return platform name
    def _GetName(self):
        return self.Platform.PlatformName

    ## Return the meta file GUID
    def _GetGuid(self):
        return self.Platform.Guid

    ## Return the platform version
    def _GetVersion(self):
        return self.Platform.Version

    ## Return the FDF file name
    def _GetFdfFile(self):
        if self._FdfFile == None:
            if self.Workspace.FdfFile != "":
                self._FdfFile= path.join(self.WorkspaceDir, self.Workspace.FdfFile)
            else:
                self._FdfFile = ''
        return self._FdfFile

    ## Return the build output directory platform specifies
    def _GetOutputDir(self):
        return self.Platform.OutputDirectory

    ## Return the directory to store all intermediate and final files built
    def _GetBuildDir(self):
        if self._BuildDir == None:
            if os.path.isabs(self.OutputDir):
                self._BuildDir = path.join(
                                            path.abspath(self.OutputDir),
                                            self.BuildTarget + "_" + self.ToolChain,
                                            )
            else:
                self._BuildDir = path.join(
                                            self.WorkspaceDir,
                                            self.OutputDir,
                                            self.BuildTarget + "_" + self.ToolChain,
                                            )
        return self._BuildDir

    ## Return directory of platform makefile
    #
    #   @retval     string  Makefile directory
    #
    def _GetMakeFileDir(self):
        if self._MakeFileDir == None:
            self._MakeFileDir = path.join(self.BuildDir, self.Arch)
        return self._MakeFileDir

    ## Return build command string
    #
    #   @retval     string  Build command string
    #
    def _GetBuildCommand(self):
        if self._BuildCommand == None:
            self._BuildCommand = []
            if "MAKE" in self.ToolDefinition and "PATH" in self.ToolDefinition["MAKE"]:
                self._BuildCommand += SplitOption(self.ToolDefinition["MAKE"]["PATH"])
                if "FLAGS" in self.ToolDefinition["MAKE"]:
                    NewOption = self.ToolDefinition["MAKE"]["FLAGS"].strip()
                    if NewOption != '':
                      self._BuildCommand += SplitOption(NewOption)
        return self._BuildCommand

    ## Get tool chain definition
    #
    #  Get each tool defition for given tool chain from tools_def.txt and platform
    #
    def _GetToolDefinition(self):
        if self._ToolDefinitions == None:
            ToolDefinition = self.Workspace.ToolDef.ToolsDefTxtDictionary
            if TAB_TOD_DEFINES_COMMAND_TYPE not in self.Workspace.ToolDef.ToolsDefTxtDatabase:
                EdkLogger.error('build', RESOURCE_NOT_AVAILABLE, "No tools found in configuration",
                                ExtraData="[%s]" % self.MetaFile)
            self._ToolDefinitions = {}
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

                if Tool not in self._ToolDefinitions:
                    self._ToolDefinitions[Tool] = {}
                self._ToolDefinitions[Tool][Attr] = Value

            ToolsDef = ''
            MakePath = ''
            if GlobalData.gOptions.SilentMode and "MAKE" in self._ToolDefinitions:
                if "FLAGS" not in self._ToolDefinitions["MAKE"]:
                    self._ToolDefinitions["MAKE"]["FLAGS"] = ""
                self._ToolDefinitions["MAKE"]["FLAGS"] += " -s"
            MakeFlags = ''
            for Tool in self._ToolDefinitions:
                for Attr in self._ToolDefinitions[Tool]:
                    Value = self._ToolDefinitions[Tool][Attr]
                    if Tool in self.BuildOption and Attr in self.BuildOption[Tool]:
                        # check if override is indicated
                        if self.BuildOption[Tool][Attr].startswith('='):
                            Value = self.BuildOption[Tool][Attr][1:]
                        else:
                            Value += " " + self.BuildOption[Tool][Attr]

                    if Attr == "PATH":
                        # Don't put MAKE definition in the file
                        if Tool == "MAKE":
                            MakePath = Value
                        else:
                            ToolsDef += "%s = %s\n" % (Tool, Value)
                    elif Attr != "DLL":
                        # Don't put MAKE definition in the file
                        if Tool == "MAKE":
                            if Attr == "FLAGS":
                                MakeFlags = Value
                        else:
                            ToolsDef += "%s_%s = %s\n" % (Tool, Attr, Value)
                ToolsDef += "\n"

            SaveFileOnChange(self.ToolDefinitionFile, ToolsDef)
            for DllPath in DllPathList:
                os.environ["PATH"] = DllPath + os.pathsep + os.environ["PATH"]
            os.environ["MAKE_FLAGS"] = MakeFlags

        return self._ToolDefinitions

    ## Return the paths of tools
    def _GetToolDefFile(self):
        if self._ToolDefFile == None:
            self._ToolDefFile = os.path.join(self.MakeFileDir, "TOOLS_DEF." + self.Arch)
        return self._ToolDefFile

    ## Retrieve the toolchain family of given toolchain tag. Default to 'MSFT'.
    def _GetToolChainFamily(self):
        if self._ToolChainFamily == None:
            ToolDefinition = self.Workspace.ToolDef.ToolsDefTxtDatabase
            if TAB_TOD_DEFINES_FAMILY not in ToolDefinition \
               or self.ToolChain not in ToolDefinition[TAB_TOD_DEFINES_FAMILY] \
               or not ToolDefinition[TAB_TOD_DEFINES_FAMILY][self.ToolChain]:
                EdkLogger.verbose("No tool chain family found in configuration for %s. Default to MSFT." \
                                   % self.ToolChain)
                self._ToolChainFamily = "MSFT"
            else:
                self._ToolChainFamily = ToolDefinition[TAB_TOD_DEFINES_FAMILY][self.ToolChain]
        return self._ToolChainFamily

    def _GetBuildRuleFamily(self):
        if self._BuildRuleFamily == None:
            ToolDefinition = self.Workspace.ToolDef.ToolsDefTxtDatabase
            if TAB_TOD_DEFINES_BUILDRULEFAMILY not in ToolDefinition \
               or self.ToolChain not in ToolDefinition[TAB_TOD_DEFINES_BUILDRULEFAMILY] \
               or not ToolDefinition[TAB_TOD_DEFINES_BUILDRULEFAMILY][self.ToolChain]:
                EdkLogger.verbose("No tool chain family found in configuration for %s. Default to MSFT." \
                                   % self.ToolChain)
                self._BuildRuleFamily = "MSFT"
            else:
                self._BuildRuleFamily = ToolDefinition[TAB_TOD_DEFINES_BUILDRULEFAMILY][self.ToolChain]
        return self._BuildRuleFamily

    ## Return the build options specific to this platform
    def _GetBuildOptions(self):
        if self._BuildOption == None:
            self._BuildOption = self._ExpandBuildOption(self.Platform.BuildOptions)
        return self._BuildOption

    ## Parse build_rule.txt in $(WORKSPACE)/Conf/build_rule.txt
    #
    #   @retval     BuildRule object
    #
    def _GetBuildRule(self):
        if self._BuildRule == None:
            BuildRuleFile = None
            if TAB_TAT_DEFINES_BUILD_RULE_CONF in self.Workspace.TargetTxt.TargetTxtDictionary:
                BuildRuleFile = self.Workspace.TargetTxt.TargetTxtDictionary[TAB_TAT_DEFINES_BUILD_RULE_CONF]
            if BuildRuleFile in [None, '']:
                BuildRuleFile = gBuildRuleFile
            self._BuildRule = BuildRule(BuildRuleFile)
        return self._BuildRule

    ## Summarize the packages used by modules in this platform
    def _GetPackageList(self):
        if self._PackageList == None:
            self._PackageList = set()
            for La in self.LibraryAutoGenList:
                self._PackageList.update(La.DependentPackageList)
            for Ma in self.ModuleAutoGenList:
                self._PackageList.update(Ma.DependentPackageList)
            self._PackageList = list(self._PackageList)
        return self._PackageList

    ## Get list of non-dynamic PCDs
    def _GetNonDynamicPcdList(self):
        return self._NonDynamicPcdList

    ## Get list of dynamic PCDs
    def _GetDynamicPcdList(self):
        return self._DynamicPcdList

    ## Generate Token Number for all PCD
    def _GetPcdTokenNumbers(self):
        if self._PcdTokenNumber == None:
            self._PcdTokenNumber = sdict()
            TokenNumber = 1
            for Pcd in self.DynamicPcdList:
                if Pcd.Phase == "PEI":
                    EdkLogger.debug(EdkLogger.DEBUG_5, "%s %s (%s) -> %d" % (Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Pcd.Phase, TokenNumber))
                    self._PcdTokenNumber[Pcd.TokenCName, Pcd.TokenSpaceGuidCName] = TokenNumber
                    TokenNumber += 1

            for Pcd in self.DynamicPcdList:
                if Pcd.Phase == "DXE":
                    EdkLogger.debug(EdkLogger.DEBUG_5, "%s %s (%s) -> %d" % (Pcd.TokenCName, Pcd.TokenSpaceGuidCName, Pcd.Phase, TokenNumber))
                    self._PcdTokenNumber[Pcd.TokenCName, Pcd.TokenSpaceGuidCName] = TokenNumber
                    TokenNumber += 1

            for Pcd in self.NonDynamicPcdList:
                self._PcdTokenNumber[Pcd.TokenCName, Pcd.TokenSpaceGuidCName] = TokenNumber
                TokenNumber += 1
        return self._PcdTokenNumber

    ## Summarize ModuleAutoGen objects of all modules/libraries to be built for this platform
    def _GetAutoGenObjectList(self):
        self._ModuleAutoGenList = []
        self._LibraryAutoGenList = []
        for ModuleFile in self.Platform.Modules:
            Ma = ModuleAutoGen(
                    self.Workspace,
                    ModuleFile,
                    self.BuildTarget,
                    self.ToolChain,
                    self.Arch,
                    self.MetaFile
                    )
            if Ma not in self._ModuleAutoGenList:
                self._ModuleAutoGenList.append(Ma)
            for La in Ma.LibraryAutoGenList:
                if La not in self._LibraryAutoGenList:
                    self._LibraryAutoGenList.append(La)

    ## Summarize ModuleAutoGen objects of all modules to be built for this platform
    def _GetModuleAutoGenList(self):
        if self._ModuleAutoGenList == None:
            self._GetAutoGenObjectList()
        return self._ModuleAutoGenList

    ## Summarize ModuleAutoGen objects of all libraries to be built for this platform
    def _GetLibraryAutoGenList(self):
        if self._LibraryAutoGenList == None:
            self._GetAutoGenObjectList()
        return self._LibraryAutoGenList

    ## Test if a module is supported by the platform
    #
    #  An error will be raised directly if the module or its arch is not supported
    #  by the platform or current configuration
    #
    def ValidModule(self, Module):
        return Module in self.Platform.Modules or Module in self.Platform.LibraryInstances

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
        ModuleType = Module.ModuleType

        # for overridding library instances with module specific setting
        PlatformModule = self.Platform.Modules[str(Module)]

        # add forced library instance
        for LibraryClass in PlatformModule.LibraryClasses:
            if LibraryClass.startswith("NULL"):
                Module.LibraryClasses[LibraryClass] = PlatformModule.LibraryClasses[LibraryClass]

        # R9 module
        LibraryConsumerList = [Module]
        Constructor         = []
        ConsumedByList      = sdict()
        LibraryInstance     = sdict()

        EdkLogger.verbose("")
        EdkLogger.verbose("Library instances of module [%s] [%s]:" % (str(Module), self.Arch))
        while len(LibraryConsumerList) > 0:
            M = LibraryConsumerList.pop()
            for LibraryClassName in M.LibraryClasses:
                if LibraryClassName not in LibraryInstance:
                    # override library instance for this module
                    if LibraryClassName in PlatformModule.LibraryClasses:
                        LibraryPath = PlatformModule.LibraryClasses[LibraryClassName]
                    else:
                        LibraryPath = self.Platform.LibraryClasses[LibraryClassName, ModuleType]
                    if LibraryPath == None or LibraryPath == "":
                        LibraryPath = M.LibraryClasses[LibraryClassName]
                        if LibraryPath == None or LibraryPath == "":
                            EdkLogger.error("build", RESOURCE_NOT_AVAILABLE,
                                            "Instance of library class [%s] is not found" % LibraryClassName,
                                            File=self.MetaFile,
                                            ExtraData="in [%s] [%s]\n\tconsumed by module [%s]" % (str(M), self.Arch, str(Module)))

                    LibraryModule = self.BuildDatabase[LibraryPath, self.Arch]
                    # for those forced library instance (NULL library), add a fake library class
                    if LibraryClassName.startswith("NULL"):
                        LibraryModule.LibraryClass.append(LibraryClassObject(LibraryClassName, [ModuleType]))
                    elif LibraryModule.LibraryClass == None \
                         or len(LibraryModule.LibraryClass) == 0 \
                         or (ModuleType != 'USER_DEFINED'
                             and ModuleType not in LibraryModule.LibraryClass[0].SupModList):
                        # only USER_DEFINED can link against any library instance despite of its SupModList
                        EdkLogger.error("build", OPTION_MISSING,
                                        "Module type [%s] is not supported by library instance [%s]" \
                                        % (ModuleType, LibraryPath), File=self.MetaFile,
                                        ExtraData="consumed by [%s]" % str(Module))

                    LibraryInstance[LibraryClassName] = LibraryModule
                    LibraryConsumerList.append(LibraryModule)
                    EdkLogger.verbose("\t" + str(LibraryClassName) + " : " + str(LibraryModule))
                else:
                    LibraryModule = LibraryInstance[LibraryClassName]

                if LibraryModule == None:
                    continue

                if LibraryModule.ConstructorList != [] and LibraryModule not in Constructor:
                    Constructor.append(LibraryModule)

                if LibraryModule not in ConsumedByList:
                    ConsumedByList[LibraryModule] = []
                # don't add current module itself to consumer list
                if M != Module:
                    if M in ConsumedByList[LibraryModule]:
                        continue
                    ConsumedByList[LibraryModule].append(M)
        #
        # Initialize the sorted output list to the empty set
        #
        SortedLibraryList = []
        #
        # Q <- Set of all nodes with no incoming edges
        #
        LibraryList = [] #LibraryInstance.values()
        Q = []
        for LibraryClassName in LibraryInstance:
            M = LibraryInstance[LibraryClassName]
            LibraryList.append(M)
            if ConsumedByList[M] == []:
                Q.insert(0, M)

        #
        # start the  DAG algorithm
        #
        while True:
            EdgeRemoved = True
            while Q == [] and EdgeRemoved:
                EdgeRemoved = False
                # for each node Item with a Constructor
                for Item in LibraryList:
                    if Item not in Constructor:
                        continue
                    # for each Node without a constructor with an edge e from Item to Node
                    for Node in ConsumedByList[Item]:
                        if Node in Constructor:
                            continue
                        # remove edge e from the graph if Node has no constructor
                        ConsumedByList[Item].remove(Node)
                        EdgeRemoved = True
                        if ConsumedByList[Item] == []:
                            # insert Item into Q
                            Q.insert(0, Item)
                            break
                    if Q != []:
                        break
            # DAG is done if there's no more incoming edge for all nodes
            if Q == []:
                break

            # remove node from Q
            Node = Q.pop()
            # output Node
            SortedLibraryList.append(Node)

            # for each node Item with an edge e from Node to Item do
            for Item in LibraryList:
                if Node not in ConsumedByList[Item]:
                    continue
                # remove edge e from the graph
                ConsumedByList[Item].remove(Node)

                if ConsumedByList[Item] != []:
                    continue
                # insert Item into Q, if Item has no other incoming edges
                Q.insert(0, Item)

        #
        # if any remaining node Item in the graph has a constructor and an incoming edge, then the graph has a cycle
        #
        for Item in LibraryList:
            if ConsumedByList[Item] != [] and Item in Constructor and len(Constructor) > 1:
                ErrorMessage = "\tconsumed by " + "\n\tconsumed by ".join([str(L) for L in ConsumedByList[Item]])
                EdkLogger.error("build", BUILD_ERROR, 'Library [%s] with constructors has a cycle' % str(Item),
                                ExtraData=ErrorMessage, File=self.MetaFile)
            if Item not in SortedLibraryList:
                SortedLibraryList.append(Item)

        #
        # Build the list of constructor and destructir names
        # The DAG Topo sort produces the destructor order, so the list of constructors must generated in the reverse order
        #
        SortedLibraryList.reverse()
        return SortedLibraryList


    ## Override PCD setting (type, value, ...)
    #
    #   @param  ToPcd       The PCD to be overrided
    #   @param  FromPcd     The PCD overrideing from
    #
    def _OverridePcd(self, ToPcd, FromPcd, Module=""):
        #
        # in case there's PCDs coming from FDF file, which have no type given.
        # at this point, ToPcd.Type has the type found from dependent
        # package
        #
        if FromPcd != None:
            if ToPcd.Pending and FromPcd.Type not in [None, '']:
                ToPcd.Type = FromPcd.Type
            elif ToPcd.Type not in [None, ''] and FromPcd.Type not in [None, ''] \
                and ToPcd.Type != FromPcd.Type:
                EdkLogger.error("build", OPTION_CONFLICT, "Mismatched PCD type",
                                ExtraData="%s.%s is defined as [%s] in module %s, but as [%s] in platform."\
                                          % (ToPcd.TokenSpaceGuidCName, ToPcd.TokenCName,
                                             ToPcd.Type, Module, FromPcd.Type),
                                          File=self.MetaFile)

            if FromPcd.MaxDatumSize not in [None, '']:
                ToPcd.MaxDatumSize = FromPcd.MaxDatumSize
            if FromPcd.DefaultValue not in [None, '']:
                ToPcd.DefaultValue = FromPcd.DefaultValue
            if FromPcd.TokenValue not in [None, '']:
                ToPcd.TokenValue = FromPcd.TokenValue
            if FromPcd.MaxDatumSize not in [None, '']:
                ToPcd.MaxDatumSize = FromPcd.MaxDatumSize
            if FromPcd.DatumType not in [None, '']:
                ToPcd.DatumType = FromPcd.DatumType
            if FromPcd.SkuInfoList not in [None, '', []]:
                ToPcd.SkuInfoList = FromPcd.SkuInfoList

            # check the validation of datum
            IsValid, Cause = CheckPcdDatum(ToPcd.DatumType, ToPcd.DefaultValue)
            if not IsValid:
                EdkLogger.error('build', FORMAT_INVALID, Cause, File=self.MetaFile,
                                ExtraData="%s.%s" % (ToPcd.TokenSpaceGuidCName, ToPcd.TokenCName))

        if ToPcd.DatumType == "VOID*" and ToPcd.MaxDatumSize in ['', None]:
            EdkLogger.debug(EdkLogger.DEBUG_9, "No MaxDatumSize specified for PCD %s.%s" \
                            % (ToPcd.TokenSpaceGuidCName, ToPcd.TokenCName))
            Value = ToPcd.DefaultValue
            if Value in [None, '']:
                ToPcd.MaxDatumSize = 1
            elif Value[0] == 'L':
                ToPcd.MaxDatumSize = str(len(Value) * 2)
            elif Value[0] == '{':
                ToPcd.MaxDatumSize = str(len(Value.split(',')))
            else:
                ToPcd.MaxDatumSize = str(len(Value))

        # apply default SKU for dynamic PCDS if specified one is not available
        if (ToPcd.Type in PCD_DYNAMIC_TYPE_LIST or ToPcd.Type in PCD_DYNAMIC_EX_TYPE_LIST) \
            and ToPcd.SkuInfoList in [None, {}, '']:
            if self.Platform.SkuName in self.Platform.SkuIds:
                SkuName = self.Platform.SkuName
            else:
                SkuName = 'DEFAULT'
            ToPcd.SkuInfoList = {
                SkuName : SkuInfoClass(SkuName, self.Platform.SkuIds[SkuName], '', '', '', '', '', ToPcd.DefaultValue)
            }

    ## Apply PCD setting defined platform to a module
    #
    #   @param  Module  The module from which the PCD setting will be overrided
    #
    #   @retval PCD_list    The list PCDs with settings from platform
    #
    def ApplyPcdSetting(self, Module, Pcds):
        # for each PCD in module
        for Name,Guid in Pcds:
            PcdInModule = Pcds[Name,Guid]
            # find out the PCD setting in platform
            if (Name,Guid) in self.Platform.Pcds:
                PcdInPlatform = self.Platform.Pcds[Name,Guid]
            else:
                PcdInPlatform = None
            # then override the settings if any
            self._OverridePcd(PcdInModule, PcdInPlatform, Module)
            # resolve the VariableGuid value
            for SkuId in PcdInModule.SkuInfoList:
                Sku = PcdInModule.SkuInfoList[SkuId]
                if Sku.VariableGuid == '': continue
                Sku.VariableGuidValue = GuidValue(Sku.VariableGuid, self.PackageList)
                if Sku.VariableGuidValue == None:
                    PackageList = "\n\t".join([str(P) for P in self.PackageList])
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
                if Key in Pcds:
                    self._OverridePcd(Pcds[Key], PlatformModule.Pcds[Key], Module)
        return Pcds.values()

    ## Resolve library names to library modules
    #
    # (for R8.x modules)
    #
    #   @param  Module  The module from which the library names will be resolved
    #
    #   @retval library_list    The list of library modules
    #
    def ResolveLibraryReference(self, Module):
        EdkLogger.verbose("")
        EdkLogger.verbose("Library instances of module [%s] [%s]:" % (str(Module), self.Arch))
        LibraryConsumerList = [Module]

        # "CompilerStub" is a must for R8 modules
        if Module.Libraries:
            Module.Libraries.append("CompilerStub")
        LibraryList = []
        while len(LibraryConsumerList) > 0:
            M = LibraryConsumerList.pop()
            for LibraryName in M.Libraries:
                Library = self.Platform.LibraryClasses[LibraryName, ':dummy:']
                if Library == None:
                    for Key in self.Platform.LibraryClasses.data.keys():
                        if LibraryName.upper() == Key.upper():
                            Library = self.Platform.LibraryClasses[Key, ':dummy:']
                            break
                    if Library == None:
                        EdkLogger.warn("build", "Library [%s] is not found" % LibraryName, File=str(M),
                            ExtraData="\t%s [%s]" % (str(Module), self.Arch))
                        continue

                if Library not in LibraryList:
                    LibraryList.append(Library)
                    LibraryConsumerList.append(Library)
                    EdkLogger.verbose("\t" + LibraryName + " : " + str(Library) + ' ' + str(type(Library)))
        return LibraryList

    ## Expand * in build option key
    #
    #   @param  Options     Options to be expanded
    #
    #   @retval options     Options expanded
    #
    def _ExpandBuildOption(self, Options):
        BuildOptions = {}
        for Key in Options:
            Family = Key[0]
            Target, Tag, Arch, Tool, Attr = Key[1].split("_")
            # if tool chain family doesn't match, skip it
            if Family and Tool in self.ToolDefinition and Family != self.ToolDefinition[Tool]["FAMILY"]:
                continue
            # expand any wildcard
            if Target == "*" or Target == self.BuildTarget:
                if Tag == "*" or Tag == self.ToolChain:
                    if Arch == "*" or Arch == self.Arch:
                        if Tool not in BuildOptions:
                            BuildOptions[Tool] = {}
                        if Attr != "FLAGS" or Attr not in BuildOptions[Tool]:
                            BuildOptions[Tool][Attr] = Options[Key]
                        else:
                            # append options for the same tool
                            BuildOptions[Tool][Attr] += " " + Options[Key]
        return BuildOptions

    ## Append build options in platform to a module
    #
    #   @param  Module  The module to which the build options will be appened
    #
    #   @retval options     The options appended with build options in platform
    #
    def ApplyBuildOption(self, Module):
        PlatformOptions = self.BuildOption
        ModuleOptions = self._ExpandBuildOption(Module.BuildOptions)
        if Module in self.Platform.Modules:
            PlatformModule = self.Platform.Modules[str(Module)]
            PlatformModuleOptions = self._ExpandBuildOption(PlatformModule.BuildOptions)
        else:
            PlatformModuleOptions = {}

        AllTools = set(ModuleOptions.keys() + PlatformOptions.keys() + PlatformModuleOptions.keys() + self.ToolDefinition.keys())
        BuildOptions = {}
        for Tool in AllTools:
            if Tool not in BuildOptions:
                BuildOptions[Tool] = {}

            for Options in [self.ToolDefinition, ModuleOptions, PlatformOptions, PlatformModuleOptions]:
                if Tool not in Options:
                    continue
                for Attr in Options[Tool]:
                    Value = Options[Tool][Attr]
                    if Attr not in BuildOptions[Tool]:
                        BuildOptions[Tool][Attr] = ""
                    # check if override is indicated
                    if Value.startswith('='):
                        BuildOptions[Tool][Attr] = Value[1:]
                    else:
                        BuildOptions[Tool][Attr] += " " + Value
        return BuildOptions

    Platform            = property(_GetPlatform)
    Name                = property(_GetName)
    Guid                = property(_GetGuid)
    Version             = property(_GetVersion)

    OutputDir           = property(_GetOutputDir)
    BuildDir            = property(_GetBuildDir)
    MakeFileDir         = property(_GetMakeFileDir)
    FdfFile             = property(_GetFdfFile)

    PcdTokenNumber      = property(_GetPcdTokenNumbers)    # (TokenCName, TokenSpaceGuidCName) : GeneratedTokenNumber
    DynamicPcdList      = property(_GetDynamicPcdList)    # [(TokenCName1, TokenSpaceGuidCName1), (TokenCName2, TokenSpaceGuidCName2), ...]
    NonDynamicPcdList   = property(_GetNonDynamicPcdList)    # [(TokenCName1, TokenSpaceGuidCName1), (TokenCName2, TokenSpaceGuidCName2), ...]
    PackageList         = property(_GetPackageList)

    ToolDefinition      = property(_GetToolDefinition)    # toolcode : tool path
    ToolDefinitionFile  = property(_GetToolDefFile)    # toolcode : lib path
    ToolChainFamily     = property(_GetToolChainFamily)
    BuildRuleFamily     = property(_GetBuildRuleFamily)
    BuildOption         = property(_GetBuildOptions)    # toolcode : option

    BuildCommand        = property(_GetBuildCommand)
    BuildRule           = property(_GetBuildRule)
    ModuleAutoGenList   = property(_GetModuleAutoGenList)
    LibraryAutoGenList  = property(_GetLibraryAutoGenList)

## ModuleAutoGen class
#
# This class encapsules the AutoGen behaviors for the build tools. In addition to
# the generation of AutoGen.h and AutoGen.c, it will generate *.depex file according
# to the [depex] section in module's inf file.
#
class ModuleAutoGen(AutoGen):
    ## The real constructor of ModuleAutoGen
    #
    #  This method is not supposed to be called by users of ModuleAutoGen. It's
    #  only used by factory method __new__() to do real initialization work for an
    #  object of ModuleAutoGen
    #
    #   @param      Workspace           EdkIIWorkspaceBuild object
    #   @param      ModuleFile          The path of module file
    #   @param      Target              Build target (DEBUG, RELEASE)
    #   @param      Toolchain           Name of tool chain
    #   @param      Arch                The arch the module supports
    #   @param      PlatformFile        Platform meta-file
    #
    def _Init(self, Workspace, ModuleFile, Target, Toolchain, Arch, PlatformFile):
        EdkLogger.debug(EdkLogger.DEBUG_9, "AutoGen module [%s] [%s]" % (ModuleFile, Arch))
        GlobalData.gProcessingFile = "%s [%s, %s, %s]" % (ModuleFile, Arch, Toolchain, Target)

        self.Workspace = Workspace
        self.WorkspaceDir = Workspace.WorkspaceDir

        self.MetaFile = ModuleFile
        self.PlatformInfo = PlatformAutoGen(Workspace, PlatformFile, Target, Toolchain, Arch)
        # check if this module is employed by active platform
        if not self.PlatformInfo.ValidModule(self.MetaFile):
            EdkLogger.verbose("Module [%s] for [%s] is not employed by active platform\n" \
                              % (self.MetaFile, Arch))
            return False

        self.SourceDir = self.MetaFile.SubDir
        self.SourceOverrideDir = None
        # use overrided path defined in DSC file
        if self.MetaFile.Key in GlobalData.gOverrideDir:
            self.SourceOverrideDir = GlobalData.gOverrideDir[self.MetaFile.Key]

        self.ToolChain = Toolchain
        self.BuildTarget = Target
        self.Arch = Arch
        self.ToolChainFamily = self.PlatformInfo.ToolChainFamily
        self.BuildRuleFamily = self.PlatformInfo.BuildRuleFamily

        self.IsMakeFileCreated = False
        self.IsCodeFileCreated = False

        self.BuildDatabase = self.Workspace.BuildDatabase

        self._Module          = None
        self._Name            = None
        self._Guid            = None
        self._Version         = None
        self._ModuleType      = None
        self._ComponentType   = None
        self._PcdIsDriver     = None
        self._AutoGenVersion  = None
        self._LibraryFlag     = None
        self._CustomMakefile  = None
        self._Macro           = None

        self._BuildDir        = None
        self._OutputDir       = None
        self._DebugDir        = None
        self._MakeFileDir     = None

        self._IncludePathList = None
        self._AutoGenFileList = None
        self._UnicodeFileList = None
        self._SourceFileList  = None
        self._ObjectFileList  = None
        self._BinaryFileList  = None

        self._DependentPackageList    = None
        self._DependentLibraryList    = None
        self._LibraryAutoGenList      = None
        self._DerivedPackageList      = None
        self._ModulePcdList           = None
        self._LibraryPcdList          = None
        self._GuidList                = None
        self._ProtocolList            = None
        self._PpiList                 = None
        self._DepexList               = None
        self._BuildOption             = None
        self._BuildTargets            = None
        self._IntroBuildTargetList    = None
        self._FinalBuildTargetList    = None
        self._FileTypes               = None
        self._BuildRules              = None

        return True

    def __repr__(self):
        return "%s [%s]" % (self.MetaFile, self.Arch)

    # Macros could be used in build_rule.txt (also Makefile)
    def _GetMacros(self):
        if self._Macro == None:
            self._Macro = sdict()
            self._Macro["WORKSPACE"             ] = self.WorkspaceDir
            self._Macro["MODULE_NAME"           ] = self.Name
            self._Macro["MODULE_GUID"           ] = self.Guid
            self._Macro["MODULE_VERSION"        ] = self.Version
            self._Macro["MODULE_TYPE"           ] = self.ModuleType
            self._Macro["MODULE_FILE"           ] = str(self.MetaFile)
            self._Macro["MODULE_FILE_BASE_NAME" ] = self.MetaFile.BaseName
            self._Macro["MODULE_RELATIVE_DIR"   ] = self.SourceDir
            self._Macro["MODULE_DIR"            ] = self.SourceDir

            self._Macro["BASE_NAME"             ] = self.Name

            self._Macro["ARCH"                  ] = self.Arch
            self._Macro["TOOLCHAIN"             ] = self.ToolChain
            self._Macro["TOOLCHAIN_TAG"         ] = self.ToolChain
            self._Macro["TARGET"                ] = self.BuildTarget

            self._Macro["BUILD_DIR"             ] = self.PlatformInfo.BuildDir
            self._Macro["BIN_DIR"               ] = os.path.join(self.PlatformInfo.BuildDir, self.Arch)
            self._Macro["LIB_DIR"               ] = os.path.join(self.PlatformInfo.BuildDir, self.Arch)
            self._Macro["MODULE_BUILD_DIR"      ] = self.BuildDir
            self._Macro["OUTPUT_DIR"            ] = self.OutputDir
            self._Macro["DEBUG_DIR"             ] = self.DebugDir
        return self._Macro

    ## Return the module build data object
    def _GetModule(self):
        if self._Module == None:
            self._Module = self.Workspace.BuildDatabase[self.MetaFile, self.Arch]
        return self._Module

    ## Return the module name
    def _GetBaseName(self):
        return self.Module.BaseName

    ## Return the module SourceOverridePath
    def _GetSourceOverridePath(self):
        return self.Module.SourceOverridePath

    ## Return the module meta-file GUID
    def _GetGuid(self):
        return self.Module.Guid

    ## Return the module version
    def _GetVersion(self):
        return self.Module.Version

    ## Return the module type
    def _GetModuleType(self):
        return self.Module.ModuleType

    ## Return the component type (for R8.x style of module)
    def _GetComponentType(self):
        return self.Module.ComponentType

    ## Return the build type
    def _GetBuildType(self):
        return self.Module.BuildType

    ## Return the PCD_IS_DRIVER setting
    def _GetPcdIsDriver(self):
        return self.Module.PcdIsDriver

    ## Return the autogen version, i.e. module meta-file version
    def _GetAutoGenVersion(self):
        return self.Module.AutoGenVersion

    ## Check if the module is library or not
    def _IsLibrary(self):
        if self._LibraryFlag == None:
            if self.Module.LibraryClass != None and self.Module.LibraryClass != []:
                self._LibraryFlag = True
            else:
                self._LibraryFlag = False
        return self._LibraryFlag

    ## Return the directory to store intermediate files of the module
    def _GetBuildDir(self):
        if self._BuildDir == None:
            self._BuildDir = path.join(
                                    self.PlatformInfo.BuildDir,
                                    self.Arch,
                                    self.SourceDir,
                                    self.MetaFile.BaseName
                                    )
            CreateDirectory(self._BuildDir)
        return self._BuildDir

    ## Return the directory to store the intermediate object files of the mdoule
    def _GetOutputDir(self):
        if self._OutputDir == None:
            self._OutputDir = path.join(self.BuildDir, "OUTPUT")
            CreateDirectory(self._OutputDir)
        return self._OutputDir

    ## Return the directory to store auto-gened source files of the mdoule
    def _GetDebugDir(self):
        if self._DebugDir == None:
            self._DebugDir = path.join(self.BuildDir, "DEBUG")
            CreateDirectory(self._DebugDir)
        return self._DebugDir

    ## Return the path of custom file
    def _GetCustomMakefile(self):
        if self._CustomMakefile == None:
            self._CustomMakefile = {}
            for Type in self.Module.CustomMakefile:
                if Type in gMakeTypeMap:
                    MakeType = gMakeTypeMap[Type]
                else:
                    MakeType = 'nmake'
                if self.SourceOverrideDir != None:
                    File = os.path.join(self.SourceOverrideDir, self.Module.CustomMakefile[Type])
                    if not os.path.exists(File):
                        File = os.path.join(self.SourceDir, self.Module.CustomMakefile[Type])
                else:
                    File = os.path.join(self.SourceDir, self.Module.CustomMakefile[Type])
                self._CustomMakefile[MakeType] = File
        return self._CustomMakefile

    ## Return the directory of the makefile
    #
    #   @retval     string  The directory string of module's makefile
    #
    def _GetMakeFileDir(self):
        return self.BuildDir

    ## Return build command string
    #
    #   @retval     string  Build command string
    #
    def _GetBuildCommand(self):
        return self.PlatformInfo.BuildCommand

    ## Get object list of all packages the module and its dependent libraries belong to
    #
    #   @retval     list    The list of package object
    #
    def _GetDerivedPackageList(self):
        PackageList = []
        for M in [self.Module] + self.DependentLibraryList:
            for Package in M.Packages:
                if Package in PackageList:
                    continue
                PackageList.append(Package)
        return PackageList

    ## Merge dependency expression
    #
    #   @retval     list    The token list of the dependency expression after parsed
    #
    def _GetDepexTokenList(self):
        if self._DepexList == None:
            self._DepexList = {}
            if self.IsLibrary or TAB_DEPENDENCY_EXPRESSION_FILE in self.FileTypes:
                return self._DepexList

            if self.ModuleType == "DXE_SMM_DRIVER":
                self._DepexList["DXE_DRIVER"] = []
                self._DepexList["SMM_DRIVER"] = []
            else:
                self._DepexList[self.ModuleType] = []

            for ModuleType in self._DepexList:
                DepexList = self._DepexList[ModuleType]
                #
                # Append depex from dependent libraries, if not "BEFORE", "AFTER" expresion
                #
                for M in [self.Module] + self.DependentLibraryList:
                    Inherited = False
                    for D in M.Depex[self.Arch, ModuleType]:
                        if DepexList != []:
                            DepexList.append('AND')
                        DepexList.append('(')
                        DepexList.extend(D)
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
        return self._DepexList

    ## Return the list of specification version required for the module
    #
    #   @retval     list    The list of specification defined in module file
    #
    def _GetSpecification(self):
        return self.Module.Specification

    ## Tool option for the module build
    #
    #   @param      PlatformInfo    The object of PlatformBuildInfo
    #   @retval     dict            The dict containing valid options
    #
    def _GetModuleBuildOption(self):
        if self._BuildOption == None:
            self._BuildOption = self.PlatformInfo.ApplyBuildOption(self.Module)
        return self._BuildOption

    ## Return a list of files which can be built from source
    #
    #  What kind of files can be built is determined by build rules in
    #  $(WORKSPACE)/Conf/build_rule.txt and toolchain family.
    #
    def _GetSourceFileList(self):
        if self._SourceFileList == None:
            self._SourceFileList = []
            for F in self.Module.Sources:
                # match tool chain
                if F.TagName != "" and F.TagName != self.ToolChain:
                    EdkLogger.debug(EdkLogger.DEBUG_9, "The toolchain [%s] for processing file [%s] is found, "
                                    "but [%s] is needed" % (F.TagName, str(F), self.ToolChain))
                    continue
                # match tool chain family
                if F.ToolChainFamily != "" and F.ToolChainFamily != self.ToolChainFamily:
                    EdkLogger.debug(
                                EdkLogger.DEBUG_0,
                                "The file [%s] must be built by tools of [%s], " \
                                "but current toolchain family is [%s]" \
                                    % (str(F), F.ToolChainFamily, self.ToolChainFamily))
                    continue

                # add the file path into search path list for file including
                if F.Dir not in self.IncludePathList and self.AutoGenVersion >= 0x00010005:
                    self.IncludePathList.insert(0, F.Dir)
                self._SourceFileList.append(F)
                self._ApplyBuildRule(F, TAB_UNKNOWN_FILE)
        return self._SourceFileList

    ## Return the list of unicode files
    def _GetUnicodeFileList(self):
        if self._UnicodeFileList == None:
            if TAB_UNICODE_FILE in self.FileTypes:
                self._UnicodeFileList = self.FileTypes[TAB_UNICODE_FILE]
            else:
                self._UnicodeFileList = []
        return self._UnicodeFileList

    ## Return a list of files which can be built from binary
    #
    #  "Build" binary files are just to copy them to build directory.
    #
    #   @retval     list            The list of files which can be built later
    #
    def _GetBinaryFiles(self):
        if self._BinaryFileList == None:
            self._BinaryFileList = []
            for F in self.Module.Binaries:
                if F.Target not in ['COMMON', '*'] and F.Target != self.BuildTarget:
                    continue
                self._BinaryFileList.append(F)
                self._ApplyBuildRule(F, F.Type)
        return self._BinaryFileList

    def _GetBuildRules(self):
        if self._BuildRules == None:
            BuildRules = {}
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
                BuildRules[Type] = RuleObject
                for Ext in RuleObject.SourceFileExtList:
                    BuildRules[Ext] = RuleObject
            self._BuildRules = BuildRules
        return self._BuildRules

    def _ApplyBuildRule(self, File, FileType):
        if self._BuildTargets == None:
            self._IntroBuildTargetList = set()
            self._FinalBuildTargetList = set()
            self._BuildTargets = {}
            self._FileTypes = {}

        LastTarget = None
        RuleChain = []
        SourceList = [File]
        Index = 0
        while Index < len(SourceList):
            Source = SourceList[Index]
            Index = Index + 1

            if Source != File:
                CreateDirectory(Source.Dir)

            if FileType in self.BuildRules:
                RuleObject = self.BuildRules[FileType]
            elif Source.Ext in self.BuildRules:
                RuleObject = self.BuildRules[Source.Ext]
            elif File.IsBinary and File == Source:
                RuleObject = self.BuildRules[TAB_DEFAULT_BINARY_FILE]
            else:
                # stop at no more rules
                if LastTarget:
                    self._FinalBuildTargetList.add(LastTarget)
                break

            FileType = RuleObject.SourceFileType
            if FileType not in self._FileTypes:
                self._FileTypes[FileType] = set()
            self._FileTypes[FileType].add(Source)

            # stop at STATIC_LIBRARY for library
            if self.IsLibrary and FileType == TAB_STATIC_LIBRARY:
                self._FinalBuildTargetList.add(LastTarget)
                break

            Target = RuleObject.Apply(Source)
            if not Target:
                if LastTarget:
                    self._FinalBuildTargetList.add(LastTarget)
                break
            elif not Target.Outputs:
                # Only do build for target with outputs
                self._FinalBuildTargetList.add(Target)

            if FileType not in self._BuildTargets:
                self._BuildTargets[FileType] = set()
            self._BuildTargets[FileType].add(Target)

            if not Source.IsBinary and Source == File:
                self._IntroBuildTargetList.add(Target)

            # to avoid cyclic rule
            if FileType in RuleChain:
                break

            RuleChain.append(FileType)
            SourceList.extend(Target.Outputs)
            LastTarget = Target
            FileType = TAB_UNKNOWN_FILE

    def _GetTargets(self):
        if self._BuildTargets == None:
            self._IntroBuildTargetList = set()
            self._FinalBuildTargetList = set()
            self._BuildTargets = {}
            self._FileTypes = {}

        #TRICK: call _GetSourceFileList to apply build rule for binary files
        if self.SourceFileList:
            pass

        #TRICK: call _GetBinaryFileList to apply build rule for binary files
        if self.BinaryFileList:
            pass

        return self._BuildTargets

    def _GetIntroTargetList(self):
        self._GetTargets()
        return self._IntroBuildTargetList

    def _GetFinalTargetList(self):
        self._GetTargets()
        return self._FinalBuildTargetList

    def _GetFileTypes(self):
        self._GetTargets()
        return self._FileTypes

    ## Get the list of package object the module depends on
    #
    #   @retval     list    The package object list
    #
    def _GetDependentPackageList(self):
        return self.Module.Packages

    ## Return the list of auto-generated code file
    #
    #   @retval     list        The list of auto-generated file
    #
    def _GetAutoGenFileList(self):
        if self._AutoGenFileList == None:
            self._AutoGenFileList = {}
            AutoGenC = TemplateString()
            AutoGenH = TemplateString()
            StringH = TemplateString()
            GenC.CreateCode(self, AutoGenC, AutoGenH, StringH)
            if str(AutoGenC) != "" and TAB_C_CODE_FILE in self.FileTypes:
                AutoFile = PathClass(gAutoGenCodeFileName, self.DebugDir)
                self._AutoGenFileList[AutoFile] = str(AutoGenC)
                self._ApplyBuildRule(AutoFile, TAB_UNKNOWN_FILE)
            if str(AutoGenH) != "":
                AutoFile = PathClass(gAutoGenHeaderFileName, self.DebugDir)
                self._AutoGenFileList[AutoFile] = str(AutoGenH)
                self._ApplyBuildRule(AutoFile, TAB_UNKNOWN_FILE)
            if str(StringH) != "":
                AutoFile = PathClass(gAutoGenStringFileName % {"module_name":self.Name}, self.DebugDir)
                self._AutoGenFileList[AutoFile] = str(StringH)
                self._ApplyBuildRule(AutoFile, TAB_UNKNOWN_FILE)
        return self._AutoGenFileList

    ## Return the list of library modules explicitly or implicityly used by this module
    def _GetLibraryList(self):
        if self._DependentLibraryList == None:
            # only merge library classes and PCD for non-library module
            if self.IsLibrary:
                self._DependentLibraryList = []
            else:
                if self.AutoGenVersion < 0x00010005:
                    self._DependentLibraryList = self.PlatformInfo.ResolveLibraryReference(self.Module)
                else:
                    self._DependentLibraryList = self.PlatformInfo.ApplyLibraryInstance(self.Module)
        return self._DependentLibraryList

    ## Get the list of PCDs from current module
    #
    #   @retval     list                    The list of PCD
    #
    def _GetModulePcdList(self):
        if self._ModulePcdList == None:
            # apply PCD settings from platform
            self._ModulePcdList = self.PlatformInfo.ApplyPcdSetting(self.Module, self.Module.Pcds)
        return self._ModulePcdList

    ## Get the list of PCDs from dependent libraries
    #
    #   @retval     list                    The list of PCD
    #
    def _GetLibraryPcdList(self):
        if self._LibraryPcdList == None:
            Pcds = {}
            if not self.IsLibrary:
                # get PCDs from dependent libraries
                for Library in self.DependentLibraryList:
                    for Key in Library.Pcds:
                        # skip duplicated PCDs
                        if Key in self.Module.Pcds or Key in Pcds:
                            continue
                        Pcds[Key] = copy.copy(Library.Pcds[Key])
                # apply PCD settings from platform
                self._LibraryPcdList = self.PlatformInfo.ApplyPcdSetting(self.Module, Pcds)
            else:
                self._LibraryPcdList = []
        return self._LibraryPcdList

    ## Get the GUID value mapping
    #
    #   @retval     dict    The mapping between GUID cname and its value
    #
    def _GetGuidList(self):
        if self._GuidList == None:
            self._GuidList = self.Module.Guids
            for Library in self.DependentLibraryList:
                self._GuidList.update(Library.Guids)
        return self._GuidList

    ## Get the protocol value mapping
    #
    #   @retval     dict    The mapping between protocol cname and its value
    #
    def _GetProtocolList(self):
        if self._ProtocolList == None:
            self._ProtocolList = self.Module.Protocols
            for Library in self.DependentLibraryList:
                self._ProtocolList.update(Library.Protocols)
        return self._ProtocolList

    ## Get the PPI value mapping
    #
    #   @retval     dict    The mapping between PPI cname and its value
    #
    def _GetPpiList(self):
        if self._PpiList == None:
            self._PpiList = self.Module.Ppis
            for Library in self.DependentLibraryList:
                self._PpiList.update(Library.Ppis)
        return self._PpiList

    ## Get the list of include search path
    #
    #   @retval     list                    The list path
    #
    def _GetIncludePathList(self):
        if self._IncludePathList == None:
            self._IncludePathList = []
            if self.AutoGenVersion < 0x00010005:
                for Inc in self.Module.Includes:
                    if Inc not in self._IncludePathList:
                        self._IncludePathList.append(Inc)
                    # for r8 modules
                    Inc = path.join(Inc, self.Arch.capitalize())
                    if os.path.exists(Inc) and Inc not in self._IncludePathList:
                        self._IncludePathList.append(Inc)
                # r8 module needs to put DEBUG_DIR at the end of search path and not to use SOURCE_DIR all the time
                self._IncludePathList.append(self.DebugDir)
            else:
                self._IncludePathList.append(self.MetaFile.Dir)
                self._IncludePathList.append(self.DebugDir)

            for Package in self.Module.Packages:
                PackageDir = path.join(self.WorkspaceDir, Package.MetaFile.Dir)
                if PackageDir not in self._IncludePathList:
                    self._IncludePathList.append(PackageDir)
                for Inc in Package.Includes:
                    if Inc not in self._IncludePathList:
                        self._IncludePathList.append(str(Inc))
        return self._IncludePathList

    ## Create makefile for the module and its dependent libraries
    #
    #   @param      CreateLibraryMakeFile   Flag indicating if or not the makefiles of
    #                                       dependent libraries will be created
    #
    def CreateMakeFile(self, CreateLibraryMakeFile=True):
        if self.IsMakeFileCreated:
            return

        if not self.IsLibrary and CreateLibraryMakeFile:
            for LibraryAutoGen in self.LibraryAutoGenList:
                LibraryAutoGen.CreateMakeFile()

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

        self.IsMakeFileCreated = True

    ## Create autogen code for the module and its dependent libraries
    #
    #   @param      CreateLibraryCodeFile   Flag indicating if or not the code of
    #                                       dependent libraries will be created
    #
    def CreateCodeFile(self, CreateLibraryCodeFile=True):
        if self.IsCodeFileCreated:
            return

        if not self.IsLibrary and CreateLibraryCodeFile:
            for LibraryAutoGen in self.LibraryAutoGenList:
                LibraryAutoGen.CreateCodeFile()

        AutoGenList = []
        IgoredAutoGenList = []

        for File in self.AutoGenFileList:
            if GenC.Generate(File.Path, self.AutoGenFileList[File]):
                #Ignore R8 AutoGen.c
                if self.AutoGenVersion < 0x00010005 and File.Name == 'AutoGen.c':
                        continue

                AutoGenList.append(str(File))
            else:
                IgoredAutoGenList.append(str(File))

        for ModuleType in self.DepexList:
            if len(self.DepexList[ModuleType]) == 0:
                continue
            Dpx = GenDepex.DependencyExpression(self.DepexList[ModuleType], ModuleType, True)
            if ModuleType == 'SMM_DRIVER':
                DpxFile = gAutoGenSmmDepexFileName % {"module_name" : self.Name}
            else:
                DpxFile = gAutoGenDepexFileName % {"module_name" : self.Name}

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
    def _GetLibraryAutoGenList(self):
        if self._LibraryAutoGenList == None:
            self._LibraryAutoGenList = []
            for Library in self.DependentLibraryList:
                La = ModuleAutoGen(
                        self.Workspace,
                        Library.MetaFile,
                        self.BuildTarget,
                        self.ToolChain,
                        self.Arch,
                        self.PlatformInfo.MetaFile
                        )
                if La not in self._LibraryAutoGenList:
                    self._LibraryAutoGenList.append(La)
                    for Lib in La.CodaTargetList:
                        self._ApplyBuildRule(Lib.Target, TAB_UNKNOWN_FILE)
        return self._LibraryAutoGenList

    ## Return build command string
    #
    #   @retval     string  Build command string
    #
    def _GetBuildCommand(self):
        return self.PlatformInfo.BuildCommand


    Module          = property(_GetModule)
    Name            = property(_GetBaseName)
    Guid            = property(_GetGuid)
    Version         = property(_GetVersion)
    ModuleType      = property(_GetModuleType)
    ComponentType   = property(_GetComponentType)
    BuildType       = property(_GetBuildType)
    PcdIsDriver     = property(_GetPcdIsDriver)
    AutoGenVersion  = property(_GetAutoGenVersion)
    Macros          = property(_GetMacros)
    Specification   = property(_GetSpecification)

    IsLibrary       = property(_IsLibrary)

    BuildDir        = property(_GetBuildDir)
    OutputDir       = property(_GetOutputDir)
    DebugDir        = property(_GetDebugDir)
    MakeFileDir     = property(_GetMakeFileDir)
    CustomMakefile  = property(_GetCustomMakefile)

    IncludePathList = property(_GetIncludePathList)
    AutoGenFileList = property(_GetAutoGenFileList)
    UnicodeFileList = property(_GetUnicodeFileList)
    SourceFileList  = property(_GetSourceFileList)
    BinaryFileList  = property(_GetBinaryFiles) # FileType : [File List]
    Targets         = property(_GetTargets)
    IntroTargetList = property(_GetIntroTargetList)
    CodaTargetList  = property(_GetFinalTargetList)
    FileTypes       = property(_GetFileTypes)
    BuildRules      = property(_GetBuildRules)

    DependentPackageList    = property(_GetDependentPackageList)
    DependentLibraryList    = property(_GetLibraryList)
    LibraryAutoGenList      = property(_GetLibraryAutoGenList)
    DerivedPackageList      = property(_GetDerivedPackageList)

    ModulePcdList           = property(_GetModulePcdList)
    LibraryPcdList          = property(_GetLibraryPcdList)
    GuidList                = property(_GetGuidList)
    ProtocolList            = property(_GetProtocolList)
    PpiList                 = property(_GetPpiList)
    DepexList               = property(_GetDepexTokenList)
    BuildOption             = property(_GetModuleBuildOption)
    BuildCommand            = property(_GetBuildCommand)

# This acts like the main() function for the script, unless it is 'import'ed into another script.
if __name__ == '__main__':
    pass

