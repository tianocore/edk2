## @file
# Create makefile for MS nmake and GNU make
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
from __future__ import absolute_import
from Workspace.WorkspaceDatabase import BuildDB
from Workspace.WorkspaceCommon import GetModuleLibInstances
import Common.GlobalData as GlobalData
import os
import pickle
from pickle import HIGHEST_PROTOCOL
from Common import EdkLogger

class PCD_DATA():
    def __init__(self,TokenCName,TokenSpaceGuidCName,Type,DatumType,SkuInfoList,DefaultValue,
                 MaxDatumSize,UserDefinedDefaultStoresFlag,validateranges,
                 validlists,expressions,CustomAttribute,TokenValue):
        self.TokenCName = TokenCName
        self.TokenSpaceGuidCName = TokenSpaceGuidCName
        self.Type = Type
        self.DatumType = DatumType
        self.SkuInfoList = SkuInfoList
        self.DefaultValue = DefaultValue
        self.MaxDatumSize = MaxDatumSize
        self.UserDefinedDefaultStoresFlag = UserDefinedDefaultStoresFlag
        self.validateranges = validateranges
        self.validlists = validlists
        self.expressions = expressions
        self.CustomAttribute = CustomAttribute
        self.TokenValue = TokenValue

class DataPipe(object):
    def __init__(self, BuildDir=None):
        self.data_container = {}
        self.BuildDir = BuildDir
        self.dump_file = ""

class MemoryDataPipe(DataPipe):

    def Get(self,key):
        return self.data_container.get(key)

    def dump(self,file_path):
        self.dump_file = file_path
        with open(file_path,'wb') as fd:
            pickle.dump(self.data_container,fd,pickle.HIGHEST_PROTOCOL)

    def load(self,file_path):
        with open(file_path,'rb') as fd:
            self.data_container = pickle.load(fd)

    @property
    def DataContainer(self):
        return self.data_container
    @DataContainer.setter
    def DataContainer(self,data):
        self.data_container.update(data)

    def FillData(self,PlatformInfo):
        #Platform Pcds
        self.DataContainer = {
            "PLA_PCD" : [PCD_DATA(
            pcd.TokenCName,pcd.TokenSpaceGuidCName,pcd.Type,
            pcd.DatumType,pcd.SkuInfoList,pcd.DefaultValue,
            pcd.MaxDatumSize,pcd.UserDefinedDefaultStoresFlag,pcd.validateranges,
                 pcd.validlists,pcd.expressions,pcd.CustomAttribute,pcd.TokenValue)
            for pcd in PlatformInfo.Platform.Pcds.values()]
            }

        #Platform Module Pcds
        ModulePcds = {}
        for m in PlatformInfo.Platform.Modules:
            module = PlatformInfo.Platform.Modules[m]
            m_pcds =  module.Pcds
            if m_pcds:
                ModulePcds[module.Guid] = [PCD_DATA(
            pcd.TokenCName,pcd.TokenSpaceGuidCName,pcd.Type,
            pcd.DatumType,pcd.SkuInfoList,pcd.DefaultValue,
            pcd.MaxDatumSize,pcd.UserDefinedDefaultStoresFlag,pcd.validateranges,
                 pcd.validlists,pcd.expressions,pcd.CustomAttribute,pcd.TokenValue)
            for pcd in PlatformInfo.Platform.Modules[m].Pcds.values()]


        self.DataContainer = {"MOL_PCDS":ModulePcds}

        #Module's Library Instance
        ModuleLibs = {}
        libModules = {}
        for m in PlatformInfo.Platform.Modules:
            module_obj = BuildDB.BuildObject[m,PlatformInfo.Arch,PlatformInfo.BuildTarget,PlatformInfo.ToolChain]
            Libs = GetModuleLibInstances(module_obj, PlatformInfo.Platform, BuildDB.BuildObject, PlatformInfo.Arch,PlatformInfo.BuildTarget,PlatformInfo.ToolChain,PlatformInfo.MetaFile,EdkLogger)
            for lib in Libs:
                try:
                    libModules[(lib.MetaFile.File,lib.MetaFile.Root,lib.Arch,lib.MetaFile.Path)].append((m.File,m.Root,module_obj.Arch,m.Path))
                except:
                    libModules[(lib.MetaFile.File,lib.MetaFile.Root,lib.Arch,lib.MetaFile.Path)] = [(m.File,m.Root,module_obj.Arch,m.Path)]
            ModuleLibs[(m.File,m.Root,module_obj.Arch,m.Path)] = [(l.MetaFile.File,l.MetaFile.Root,l.Arch,l.MetaFile.Path) for l in Libs]
        self.DataContainer = {"DEPS":ModuleLibs}
        self.DataContainer = {"REFS":libModules}

        #Platform BuildOptions

        platform_build_opt =  PlatformInfo.EdkIIBuildOption

        ToolDefinition = PlatformInfo.ToolDefinition
        module_build_opt = {}
        for m in PlatformInfo.Platform.Modules:
            ModuleTypeOptions, PlatformModuleOptions = PlatformInfo.GetGlobalBuildOptions(BuildDB.BuildObject[m,PlatformInfo.Arch,PlatformInfo.BuildTarget,PlatformInfo.ToolChain])
            if ModuleTypeOptions or PlatformModuleOptions:
                module_build_opt.update({(m.File,m.Root): {"ModuleTypeOptions":ModuleTypeOptions, "PlatformModuleOptions":PlatformModuleOptions}})

        self.DataContainer = {"PLA_BO":platform_build_opt,
                              "TOOLDEF":ToolDefinition,
                              "MOL_BO":module_build_opt
                              }



        #Platform Info
        PInfo = {
            "WorkspaceDir":PlatformInfo.Workspace.WorkspaceDir,
            "Target":PlatformInfo.BuildTarget,
            "ToolChain":PlatformInfo.Workspace.ToolChain,
            "BuildRuleFile":PlatformInfo.BuildRule,
            "Arch": PlatformInfo.Arch,
            "ArchList":PlatformInfo.Workspace.ArchList,
            "ActivePlatform":PlatformInfo.MetaFile
            }
        self.DataContainer = {'P_Info':PInfo}

        self.DataContainer = {'M_Name':PlatformInfo.UniqueBaseName}

        self.DataContainer = {"ToolChainFamily": PlatformInfo.ToolChainFamily}

        self.DataContainer = {"BuildRuleFamily": PlatformInfo.BuildRuleFamily}

        self.DataContainer = {"MixedPcd":GlobalData.MixedPcd}

        self.DataContainer = {"BuildOptPcd":GlobalData.BuildOptionPcd}

        self.DataContainer = {"BuildCommand": PlatformInfo.BuildCommand}

        self.DataContainer = {"AsBuildModuleList": PlatformInfo._AsBuildModuleList}

        self.DataContainer = {"G_defines": GlobalData.gGlobalDefines}

        self.DataContainer = {"CL_defines": GlobalData.gCommandLineDefines}

        self.DataContainer = {"gCommandMaxLength": GlobalData.gCommandMaxLength}

        self.DataContainer = {"Env_Var": {k:v for k, v in os.environ.items()}}

        self.DataContainer = {"PackageList": [(dec.MetaFile,dec.Arch) for dec in PlatformInfo.PackageList]}

        self.DataContainer = {"GuidDict": PlatformInfo.Platform._GuidDict}

        self.DataContainer = {"DatabasePath":GlobalData.gDatabasePath}

        self.DataContainer = {"FdfParser": True if GlobalData.gFdfParser else False}

        self.DataContainer = {"LogLevel": EdkLogger.GetLevel()}

        self.DataContainer = {"UseHashCache":GlobalData.gUseHashCache}

        self.DataContainer = {"BinCacheSource":GlobalData.gBinCacheSource}

        self.DataContainer = {"BinCacheDest":GlobalData.gBinCacheDest}

        self.DataContainer = {"EnableGenfdsMultiThread":GlobalData.gEnableGenfdsMultiThread}
