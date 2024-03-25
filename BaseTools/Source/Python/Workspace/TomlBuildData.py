## @file
# This file is used to create a database used by build tool
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import absolute_import
from Common.caching import cached_property
from .MetaFileParser import *
from collections import OrderedDict
from Workspace.BuildClassObject import TomlBuildClassObject, LibraryClassObject

## Rust Module build information from toml file
#
#  This class is used to retrieve information stored in the database and
#  convert it into the ModuleBuildClassObject form for easier use in AutoGen.
#
class TomlBuildData(TomlBuildClassObject):
    ## Constructor of TomlBuildData
    #
    #  Initialize object of TomlBuildData
    #
    #   @param      FilePath        The path of platform description file
    #   @param      RawData         The raw data of DSC file
    #   @param      BuildDataBase   Database used to retrieve module/package information
    #   @param      Arch            The target architecture
    #   @param      Target          The target for this module
    #   @param      Toolchain       The toolchain for this module
    #
    def __init__(self, FilePath, RawData, BuildDatabase, Arch=TAB_ARCH_COMMON, Target=None, Toolchain=None):
        self.MetaFile = FilePath
        self._RawData = RawData
        self._Bdb = BuildDatabase
        self._Arch = Arch
        self._Target = Target
        self._Toolchain = Toolchain
        self._ToolChainFamily = None
        self.LibraryClass = []
        self.WorkspaceDir = os.getenv("WORKSPACE") if os.getenv("WORKSPACE") else ""

        # FixMe: Why need LibraryClass? I think it should be removed.
        self.LibraryClass.append(LibraryClassObject("", SUP_MODULE_LIST))

        self.ConstructorList = []
        self.LibraryClasses = []
        self.ReferenceModules = set()
        self._PcdsName = set()
        self._Packages = []
        self._Pcds = set()

    def SetReferenceModule(self,Module):
        self.ReferenceModules.add(Module)
        return self

    @property
    def Packages(self):
        return self._Packages

    @property
    def PcdsName(self):
        return self._PcdsName

    @property
    def Arch(self):
        return self._Arch

    @property
    def Pcds(self):
        return self._Pcds

    @property
    def Guids(self):
        return OrderedDict()

    @cached_property
    def GuidComments(self):
        return OrderedDict()

    @cached_property
    def Protocols(self):
        return OrderedDict()

    @cached_property
    def ProtocolComments(self):
        return OrderedDict()

    @cached_property
    def Ppis(self):
        return OrderedDict()

    @cached_property
    def PpiComments(self):
        return OrderedDict()

    @cached_property
    def DestructorList(self):
        return []

    @cached_property
    def ConstructorList(self):
        return []

    def GetGuidsUsedByPcd(self):
        return OrderedDict()
