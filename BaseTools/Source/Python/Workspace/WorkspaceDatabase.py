## @file
# This file is used to create a database used by build tool
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from Common.StringUtils import *
from Common.DataType import *
from Common.Misc import *
from types import *

from .MetaDataTable import *
from .MetaFileTable import *
from .MetaFileParser import *

from Workspace.DecBuildData import DecBuildData
from Workspace.DscBuildData import DscBuildData
from Workspace.InfBuildData import InfBuildData

## Database
#
#   This class defined the build database for all modules, packages and platform.
# It will call corresponding parser for the given file if it cannot find it in
# the database.
#
# @param DbPath             Path of database file
# @param GlobalMacros       Global macros used for replacement during file parsing
# @param RenewDb=False      Create new database file if it's already there
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
        def GetCache(self):
            return self._CACHE_

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

        # key = (FilePath, Arch=None, Target=None, Toolchain=None)
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
            BuildObject = self.CreateBuildObject(FilePath, Arch, Target, Toolchain)
            self._CACHE_[Key] = BuildObject
            return BuildObject
        def CreateBuildObject(self,FilePath, Arch, Target, Toolchain):
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
                                Arch,
                                MetaFileStorage(self.WorkspaceDb, FilePath, FileType)
                                )
            # always do post-process, in case of macros change
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
    # @param RenewDb=False      Create new database file if it's already there
    #
    def __init__(self):
        self.DB = dict()
        # create table for internal uses
        self.TblDataModel = DataClass.MODEL_LIST
        self.TblFile = []
        self.Platform = None

        # conversion object for build or file format conversion purpose
        self.BuildObject = WorkspaceDatabase.BuildObjectFactory(self)
        self.TransformObject = WorkspaceDatabase.TransformObjectFactory(self)


    ## Summarize all packages in the database
    def GetPackageList(self, Platform, Arch, TargetName, ToolChainTag):
        self.Platform = Platform
        PackageList = []
        Pa = self.BuildObject[self.Platform, Arch, TargetName, ToolChainTag]
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
        for Package in Pa.Packages:
            if Package in PackageList:
                continue
            PackageList.append(Package)

        return PackageList

    def MapPlatform(self, Dscfile):
        Platform = self.BuildObject[PathClass(Dscfile), TAB_COMMON]
        if Platform is None:
            EdkLogger.error('build', PARSER_ERROR, "Failed to parser DSC file: %s" % Dscfile)
        return Platform

BuildDB = WorkspaceDatabase()
##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    pass

