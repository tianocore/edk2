## @file
# Create rust module autogen obj
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
from __future__ import absolute_import
from AutoGen.AutoGen import AutoGen
from AutoGen.ModuleAutoGenHelper import PlatformInfo
from Workspace.BuildClassObject import ModuleBuildClassObject
from Workspace.TomlBuildData import TomlBuildData

import os.path as path
from Common.Misc import *


def _MakeDir(PathList):
    RetVal = path.join(*PathList)
    CreateDirectory(RetVal)
    return RetVal


class RustModuleAutoGen(AutoGen):
    def __init__(self, Workspace, MetaFile, Target, Toolchain, Arch, *args, **kwargs):
        self.Workspace = Workspace
        self.MetaFile = MetaFile
        self.Target = Target
        self.ToolChain = Toolchain
        self.Arch = Arch
        self._args = args
        self._kwargs = kwargs

        self.DataPipe = self._kwargs.get("DataPipe")
        PInfo = self.DataPipe.Get("P_Info")
        self.WorkspaceDir = PInfo.get("WorkspaceDir")
        self.PlatformInfo = PlatformInfo(
            self.Workspace,
            PInfo.get("ActivePlatform"),
            PInfo.get("Target"),
            PInfo.get("ToolChain"),
            PInfo.get("Arch"),
            self.DataPipe)

        self.MetaFile = MetaFile
        self.SourceDir = self.MetaFile.SubDir
        self.SourceDir = mws.relpath(self.SourceDir, self.WorkspaceDir)

    @property
    def MakeFileDir(self):
        return self.MetaFile.Dir

    @property
    def BuildTarget(self):
        return self.MetaFile.File

    def __repr__(self):
        return "%s [%s]" % (self.MetaFile, self.Arch)

    def __hash__(self):
        return hash((self.MetaFile, self.Arch))

    @property
    def BuildDir(self):
        return _MakeDir((
            self.PlatformInfo.BuildDir,
            self.Arch,
            self.SourceDir,
            self.MetaFile.BaseName
            ))

    # Return the directory to store the intermediate object files of the module
    @property
    def OutputDir(self):
        return _MakeDir((self.BuildDir, "OUTPUT"))

    @property
    def ReferenceModules(self):
        return []

    @property
    def FixedAtBuildPcds(self):
        return []

    @property
    def ConstPcd(self):
        return {}

    @property
    def FixedVoidTypePcds(self):
        return {}

    @property
    def _TomlConfig(self):
        parser = self.Module._RawData
        parser.Start()
        TomlConfig = parser.TomlConfig
        return TomlConfig

    def IsLibrary(self):
        if self._TomlConfig.get('lib') and self._TomlConfig.get('lib').get('name'):
            return True
        else:
            return False
    @property
    def OutPutFilePathName(self):
        if self.IsLibrary():
            return "%s/lib%s.a" % (self.OutputDir, self._TomlConfig.get('lib').get('name'))
        else:
            return "%s/%s.efi" % (self.OutputDir, self._TomlConfig.get('package').get('name'))

    @property
    def Module(self):
        Module = self.Workspace.BuildDatabase[self.MetaFile, self.Arch, self.Target, self.ToolChain]
        return Module
