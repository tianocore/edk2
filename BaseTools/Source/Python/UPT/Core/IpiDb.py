## @file
# This file is for installed package information database operations
#
# Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

'''
IpiDb
'''

##
# Import Modules
#
import sqlite3
import os.path
import time

import Logger.Log as Logger
from Logger import StringTable as ST
from Logger.ToolError import UPT_ALREADY_RUNNING_ERROR
from Logger.ToolError import UPT_DB_UPDATE_ERROR
import platform as pf

## IpiDb
#
# This class represents the installed package information database
# Add/Remove/Get installed distribution package information here.
# 
# 
# @param object:      Inherited from object class
# @param DbPath:      A string for the path of the database
#
#
class IpiDatabase(object):
    def __init__(self, DbPath, Workspace):
        Dir = os.path.dirname(DbPath)
        if not os.path.isdir(Dir):
            os.mkdir(Dir)
        self.Conn = sqlite3.connect(DbPath, isolation_level='DEFERRED')
        self.Conn.execute("PRAGMA page_size=4096")
        self.Conn.execute("PRAGMA synchronous=OFF")
        self.Cur = self.Conn.cursor()
        self.DpTable = 'DpInfo'
        self.PkgTable = 'PkgInfo'
        self.ModInPkgTable = 'ModInPkgInfo'
        self.StandaloneModTable = 'StandaloneModInfo'
        self.ModDepexTable = 'ModDepexInfo'
        self.DpFileListTable = 'DpFileListInfo'
        self.DummyTable = 'Dummy'
        self.Workspace = os.path.normpath(Workspace)

    ## Initialize build database
    #
    #
    def InitDatabase(self, SkipLock = False):
        Logger.Verbose(ST.MSG_INIT_IPI_START)
        if not SkipLock:
            try:
                #
                # Create a dummy table, if already existed,
                # then UPT is already running
                #
                SqlCommand = """
                create table %s (
                Dummy TEXT NOT NULL,
                PRIMARY KEY (Dummy)                                               
                )""" % self.DummyTable
                self.Cur.execute(SqlCommand)
                self.Conn.commit()
            except sqlite3.OperationalError:
                Logger.Error("UPT", 
                             UPT_ALREADY_RUNNING_ERROR, 
                             ST.ERR_UPT_ALREADY_RUNNING_ERROR
                             )
        
        #
        # Create new table
        #
        SqlCommand = """
        create table IF NOT EXISTS %s (
        DpGuid TEXT NOT NULL,DpVersion TEXT NOT NULL,
        InstallTime REAL NOT NULL,
        NewPkgFileName TEXT NOT NULL,
        PkgFileName TEXT NOT NULL,                                               
        RePackage TEXT NOT NULL,
        PRIMARY KEY (DpGuid, DpVersion)                                               
        )""" % self.DpTable
        self.Cur.execute(SqlCommand)
        
        SqlCommand = """
        create table IF NOT EXISTS %s (
        FilePath TEXT NOT NULL,
        DpGuid TEXT,
        DpVersion TEXT,
        Md5Sum TEXT,
        PRIMARY KEY (FilePath)
        )""" % self.DpFileListTable
        self.Cur.execute(SqlCommand)
        
        SqlCommand = """
        create table IF NOT EXISTS %s (
        PackageGuid TEXT NOT NULL,
        PackageVersion TEXT NOT NULL,
        InstallTime REAL NOT NULL,
        DpGuid TEXT,
        DpVersion TEXT,
        InstallPath TEXT NOT NULL,
        PRIMARY KEY (PackageGuid, PackageVersion, InstallPath)
        )""" % self.PkgTable
        self.Cur.execute(SqlCommand)
                
        SqlCommand = """
        create table IF NOT EXISTS %s (
        ModuleGuid TEXT NOT NULL,
        ModuleVersion TEXT NOT NULL,
        ModuleName TEXT NOT NULL,
        InstallTime REAL NOT NULL,
        PackageGuid TEXT,
        PackageVersion TEXT,
        InstallPath TEXT NOT NULL,
        PRIMARY KEY (ModuleGuid, ModuleVersion, ModuleName, InstallPath)
        )""" % self.ModInPkgTable
        self.Cur.execute(SqlCommand)
        
        SqlCommand = """
        create table IF NOT EXISTS %s (
        ModuleGuid TEXT NOT NULL,
        ModuleVersion TEXT NOT NULL,
        ModuleName TEXT NOT NULL,
        InstallTime REAL NOT NULL,
        DpGuid TEXT,
        DpVersion TEXT,
        InstallPath TEXT NOT NULL,
        PRIMARY KEY (ModuleGuid, ModuleVersion, ModuleName, InstallPath)
        )""" % self.StandaloneModTable
        self.Cur.execute(SqlCommand)
        
        SqlCommand = """
        create table IF NOT EXISTS %s (
        ModuleGuid TEXT NOT NULL,
        ModuleVersion TEXT NOT NULL,
        ModuleName TEXT NOT NULL,
        InstallPath TEXT NOT NULL,
        DepexGuid TEXT,
        DepexVersion TEXT
        )""" % self.ModDepexTable
        self.Cur.execute(SqlCommand)
        
        self.Conn.commit()
        
        Logger.Verbose(ST.MSG_INIT_IPI_FINISH)

    def RollBack(self):
        self.Conn.rollback()

    def Commit(self):
        self.Conn.commit()

    ## Add a distribution install information from DpObj
    #
    # @param DpObj:
    # @param NewDpPkgFileName: New DpPkg File Name
    # @param DpPkgFileName: DpPkg File Name
    # @param RePackage: A RePackage
    #
    def AddDPObject(self, DpObj, NewDpPkgFileName, DpPkgFileName, RePackage):
        try:
            for PkgKey in DpObj.PackageSurfaceArea.keys():
                PkgGuid = PkgKey[0]
                PkgVersion = PkgKey[1]
                PkgInstallPath = PkgKey[2]
                self._AddPackage(PkgGuid, PkgVersion, DpObj.Header.GetGuid(), \
                                 DpObj.Header.GetVersion(), PkgInstallPath)
                PkgObj = DpObj.PackageSurfaceArea[PkgKey]
                for ModKey in PkgObj.GetModuleDict().keys():
                    ModGuid = ModKey[0]
                    ModVersion = ModKey[1]
                    ModName = ModKey[2]
                    ModInstallPath = ModKey[3]
                    ModInstallPath = \
                    os.path.normpath(os.path.join(PkgInstallPath, ModInstallPath))
                    self._AddModuleInPackage(ModGuid, ModVersion, ModName, PkgGuid, \
                                             PkgVersion, ModInstallPath)
                    ModObj = PkgObj.GetModuleDict()[ModKey]
                    for Dep in ModObj.GetPackageDependencyList():
                        DepexGuid = Dep.GetGuid()
                        DepexVersion = Dep.GetVersion()
                        self._AddModuleDepex(ModGuid, ModVersion, ModName, ModInstallPath, \
                                             DepexGuid, DepexVersion)
                for (FilePath, Md5Sum) in PkgObj.FileList:
                    self._AddDpFilePathList(DpObj.Header.GetGuid(), \
                                            DpObj.Header.GetVersion(), FilePath, \
                                            Md5Sum)
    
            for ModKey in DpObj.ModuleSurfaceArea.keys():
                ModGuid = ModKey[0]
                ModVersion = ModKey[1]
                ModName = ModKey[2]
                ModInstallPath = ModKey[3]
                self._AddStandaloneModule(ModGuid, ModVersion, ModName, \
                                          DpObj.Header.GetGuid(), \
                                          DpObj.Header.GetVersion(), \
                                          ModInstallPath)
                ModObj = DpObj.ModuleSurfaceArea[ModKey]
                for Dep in ModObj.GetPackageDependencyList():
                    DepexGuid = Dep.GetGuid()
                    DepexVersion = Dep.GetVersion()
                    self._AddModuleDepex(ModGuid, ModVersion, ModName, ModInstallPath, \
                                         DepexGuid, DepexVersion)
                for (Path, Md5Sum) in ModObj.FileList:
                    self._AddDpFilePathList(DpObj.Header.GetGuid(), \
                                            DpObj.Header.GetVersion(), \
                                            Path, Md5Sum)
    
            #
            # add tool/misc files
            #
            for (Path, Md5Sum) in DpObj.FileList:
                self._AddDpFilePathList(DpObj.Header.GetGuid(), \
                                        DpObj.Header.GetVersion(), Path, Md5Sum)
                                    
            self._AddDp(DpObj.Header.GetGuid(), DpObj.Header.GetVersion(), \
                        NewDpPkgFileName, DpPkgFileName, RePackage)
    
        except sqlite3.IntegrityError, DetailMsg:
            Logger.Error("UPT",
                         UPT_DB_UPDATE_ERROR,
                         ST.ERR_UPT_DB_UPDATE_ERROR,
                         ExtraData = DetailMsg
                         )

    ## Add a distribution install information
    #
    # @param Guid         Guid of the distribution package  
    # @param Version      Version of the distribution package  
    # @param NewDpFileName the saved filename of distribution package file
    # @param DistributionFileName the filename of distribution package file
    #
    def _AddDp(self, Guid, Version, NewDpFileName, DistributionFileName, \
               RePackage):
        
        if Version == None or len(Version.strip()) == 0:
            Version = 'N/A'
        
        #
        # Add newly installed DP information to DB.
        #
        if NewDpFileName == None or len(NewDpFileName.strip()) == 0:
            PkgFileName = 'N/A'
        else:
            PkgFileName = NewDpFileName
        CurrentTime = time.time()
        SqlCommand = \
        """insert into %s values('%s', '%s', %s, '%s', '%s', '%s')""" % \
        (self.DpTable, Guid, Version, CurrentTime, PkgFileName, \
         DistributionFileName, str(RePackage).upper())
        self.Cur.execute(SqlCommand)

        
    ## Add a file list from DP
    #
    # @param DpGuid: A DpGuid 
    # @param DpVersion: A DpVersion
    # @param Path: A Path
    # @param Path: A Md5Sum
    #
    def _AddDpFilePathList(self, DpGuid, DpVersion, Path, Md5Sum):
        Path = os.path.normpath(Path)
        if pf.system() == 'Windows':
            if Path.startswith(self.Workspace):
                Path = Path[len(self.Workspace):]
        else:
            if Path.startswith(self.Workspace + os.sep):
                Path = Path[len(self.Workspace)+1:]
        SqlCommand = """insert into %s values('%s', '%s', '%s', '%s')""" % \
        (self.DpFileListTable, Path, DpGuid, DpVersion, Md5Sum)

        self.Cur.execute(SqlCommand)
            
    ## Add a package install information
    #
    # @param Guid: A package guid 
    # @param Version: A package version
    # @param DpGuid: A DpGuid 
    # @param DpVersion: A DpVersion
    # @param Path: A Path
    #
    def _AddPackage(self, Guid, Version, DpGuid=None, DpVersion=None, Path=''):
        
        if Version == None or len(Version.strip()) == 0:
            Version = 'N/A'
        
        if DpGuid == None or len(DpGuid.strip()) == 0:
            DpGuid = 'N/A'
        
        if DpVersion == None or len(DpVersion.strip()) == 0:
            DpVersion = 'N/A'
        
        #
        # Add newly installed package information to DB.
        #
        CurrentTime = time.time()
        SqlCommand = \
        """insert into %s values('%s', '%s', %s, '%s', '%s', '%s')""" % \
        (self.PkgTable, Guid, Version, CurrentTime, DpGuid, DpVersion, Path)
        self.Cur.execute(SqlCommand)
        
    ## Add a module that from a package install information
    #
    # @param Guid:    Module Guid 
    # @param Version: Module version
    # @param Name:    Module Name
    # @param PkgGuid: Package Guid
    # @param PkgVersion: Package version
    # @param Path:    Package relative path that module installs
    #
    def _AddModuleInPackage(self, Guid, Version, Name, PkgGuid=None, \
                            PkgVersion=None, Path=''):
        
        if Version == None or len(Version.strip()) == 0:
            Version = 'N/A'
        
        if PkgGuid == None or len(PkgGuid.strip()) == 0:
            PkgGuid = 'N/A'
        
        if PkgVersion == None or len(PkgVersion.strip()) == 0:
            PkgVersion = 'N/A'
            
        if os.name == 'posix':
            Path = Path.replace('\\', os.sep)
        else:
            Path = Path.replace('/', os.sep)
        
        #
        # Add module from package information to DB.
        #
        CurrentTime = time.time()
        SqlCommand = \
        """insert into %s values('%s', '%s', '%s', %s, '%s', '%s', '%s')""" % \
        (self.ModInPkgTable, Guid, Version, Name, CurrentTime, PkgGuid, PkgVersion, \
         Path)
        self.Cur.execute(SqlCommand)
    
    ## Add a module that is standalone install information
    #
    # @param Guid: a module Guid
    # @param Version: a module Version
    # @param Name: a module name
    # @param DpGuid: a DpGuid
    # @param DpVersion: a DpVersion
    # @param Path: path
    #
    def _AddStandaloneModule(self, Guid, Version, Name, DpGuid=None, \
                             DpVersion=None, Path=''):
        
        if Version == None or len(Version.strip()) == 0:
            Version = 'N/A'
        
        if DpGuid == None or len(DpGuid.strip()) == 0:
            DpGuid = 'N/A'
        
        if DpVersion == None or len(DpVersion.strip()) == 0:
            DpVersion = 'N/A'
        
        #
        # Add module standalone information to DB.
        #
        CurrentTime = time.time()
        SqlCommand = \
        """insert into %s values('%s', '%s', '%s', %s, '%s', '%s', '%s')""" % \
        (self.StandaloneModTable, Guid, Version, Name, CurrentTime, DpGuid, \
         DpVersion, Path)
        self.Cur.execute(SqlCommand)
    
    ## Add a module depex
    #
    # @param Guid: a module Guid
    # @param Version: a module Version
    # @param Name: a module name
    # @param DepexGuid: a module DepexGuid
    # @param DepexVersion: a module DepexVersion
    #
    def _AddModuleDepex(self, Guid, Version, Name, Path, DepexGuid=None, \
                        DepexVersion=None):
                
        if DepexGuid == None or len(DepexGuid.strip()) == 0:
            DepexGuid = 'N/A'
        
        if DepexVersion == None or len(DepexVersion.strip()) == 0:
            DepexVersion = 'N/A'
            
        if os.name == 'posix':
            Path = Path.replace('\\', os.sep)
        else:
            Path = Path.replace('/', os.sep)
        
        #
        # Add module depex information to DB.
        #
        SqlCommand = """insert into %s values('%s', '%s', '%s', '%s', '%s', '%s')"""\
         % (self.ModDepexTable, Guid, Version, Name, Path, DepexGuid, DepexVersion)
        self.Cur.execute(SqlCommand)
        
    ## Remove a distribution install information, if no version specified, 
    # remove all DPs with this Guid.
    #
    # @param DpGuid: guid of dpex 
    # @param DpVersion: version of dpex
    #
    def RemoveDpObj(self, DpGuid, DpVersion):
        
        PkgList = self.GetPackageListFromDp(DpGuid, DpVersion)
        #
        # delete from ModDepex the standalone module's dependency
        #
        SqlCommand = \
        """delete from ModDepexInfo where ModDepexInfo.ModuleGuid in 
        (select ModuleGuid from StandaloneModInfo as B where B.DpGuid = '%s' 
        and B.DpVersion = '%s')
        and ModDepexInfo.ModuleVersion in
        (select ModuleVersion from StandaloneModInfo as B 
        where B.DpGuid = '%s' and B.DpVersion = '%s')
        and ModDepexInfo.ModuleName in
        (select ModuleName from StandaloneModInfo as B 
        where B.DpGuid = '%s' and B.DpVersion = '%s')
        and ModDepexInfo.InstallPath in
        (select InstallPath from StandaloneModInfo as B 
        where B.DpGuid = '%s' and B.DpVersion = '%s') """ % \
        (DpGuid, DpVersion, DpGuid, DpVersion, DpGuid, DpVersion, DpGuid, DpVersion)

        self.Cur.execute(SqlCommand)
        #
        # delete from ModDepex the from pkg module's dependency
        #
        for Pkg in PkgList:

            SqlCommand = \
            """delete from ModDepexInfo where ModDepexInfo.ModuleGuid in 
            (select ModuleGuid from ModInPkgInfo 
            where ModInPkgInfo.PackageGuid ='%s' and 
            ModInPkgInfo.PackageVersion = '%s')
            and ModDepexInfo.ModuleVersion in
            (select ModuleVersion from ModInPkgInfo 
            where ModInPkgInfo.PackageGuid ='%s' and 
            ModInPkgInfo.PackageVersion = '%s')
            and ModDepexInfo.ModuleName in
            (select ModuleName from ModInPkgInfo 
            where ModInPkgInfo.PackageGuid ='%s' and 
            ModInPkgInfo.PackageVersion = '%s')
            and ModDepexInfo.InstallPath in
            (select InstallPath from ModInPkgInfo where 
            ModInPkgInfo.PackageGuid ='%s' 
            and ModInPkgInfo.PackageVersion = '%s')""" \
                            % (Pkg[0], Pkg[1], Pkg[0], Pkg[1], Pkg[0], Pkg[1],Pkg[0], Pkg[1])
            
            self.Cur.execute(SqlCommand)
        #
        # delete the standalone module
        #
        SqlCommand = \
        """delete from %s where DpGuid ='%s' and DpVersion = '%s'""" % \
        (self.StandaloneModTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)
        #
        # delete the from pkg module
        #
        for Pkg in PkgList:
            SqlCommand = \
            """delete from %s where %s.PackageGuid ='%s' 
            and %s.PackageVersion = '%s'""" % \
            (self.ModInPkgTable, self.ModInPkgTable, Pkg[0], \
             self.ModInPkgTable, Pkg[1])
            self.Cur.execute(SqlCommand)
        #
        # delete packages
        #
        SqlCommand = \
        """delete from %s where DpGuid ='%s' and DpVersion = '%s'""" % \
        (self.PkgTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)
        #
        # delete file list from DP
        #
        SqlCommand = \
        """delete from %s where DpGuid ='%s' and DpVersion = '%s'""" % \
        (self.DpFileListTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)
        #    
        # delete DP
        #
        SqlCommand = \
        """delete from %s where DpGuid ='%s' and DpVersion = '%s'""" % \
        (self.DpTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)
        
        #self.Conn.commit()
        
    ## Get a list of distribution install information.
    #
    # @param Guid: distribution package guid  
    # @param Version: distribution package version 
    #
    def GetDp(self, Guid, Version):
        
        if Version == None or len(Version.strip()) == 0:
            Version = 'N/A'
            Logger.Verbose(ST.MSG_GET_DP_INSTALL_LIST)
            (DpGuid, DpVersion) = (Guid, Version)
            SqlCommand = """select * from %s where DpGuid ='%s'""" % \
            (self.DpTable, DpGuid)
            self.Cur.execute(SqlCommand)
        
        else:
            Logger.Verbose(ST.MSG_GET_DP_INSTALL_INFO_START)
            (DpGuid, DpVersion) = (Guid, Version)
            SqlCommand = \
            """select * from %s where DpGuid ='%s' and DpVersion = '%s'""" % \
            (self.DpTable, DpGuid, DpVersion)
            self.Cur.execute(SqlCommand)

        DpList = []
        for DpInfo in self.Cur:
            DpGuid = DpInfo[0]
            DpVersion = DpInfo[1]
            InstallTime = DpInfo[2]
            PkgFileName = DpInfo[3]
            DpList.append((DpGuid, DpVersion, InstallTime, PkgFileName))
            
        Logger.Verbose(ST.MSG_GET_DP_INSTALL_INFO_FINISH)    
        return DpList
    
    ## Get a list of distribution install dirs
    #
    # @param Guid: distribution package guid 
    # @param Version: distribution package version 
    #
    def GetDpInstallDirList(self, Guid, Version):
        SqlCommand = """select InstallPath from PkgInfo where DpGuid = '%s' and DpVersion = '%s'""" % (Guid, Version)
        self.Cur.execute(SqlCommand)
        DirList = []
        for Result in self.Cur:
            if Result[0] not in DirList:
                DirList.append(Result[0])

        SqlCommand = """select InstallPath from StandaloneModInfo where DpGuid = '%s' and DpVersion = '%s'""" % \
                     (Guid, Version)
        self.Cur.execute(SqlCommand)
        for Result in self.Cur:
            if Result[0] not in DirList:
                DirList.append(Result[0])

        return DirList


    ## Get a list of distribution install file path information.
    #
    # @param Guid: distribution package guid 
    # @param Version: distribution package version 
    #
    def GetDpFileList(self, Guid, Version):
        
        (DpGuid, DpVersion) = (Guid, Version)
        SqlCommand = \
        """select * from %s where DpGuid ='%s' and DpVersion = '%s'""" % \
        (self.DpFileListTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)

        PathList = []
        for Result in self.Cur:
            Path = Result[0]
            Md5Sum = Result[3]
            PathList.append((os.path.join(self.Workspace, Path), Md5Sum))
        
        return PathList

    ## Get files' repackage attribute if present that are installed into current workspace
    #
    # @retval FileDict:  a Dict of file, key is file path, value is (DpGuid, DpVersion, NewDpFileName, RePackage)
    #
    def GetRePkgDict(self):
        SqlCommand = """select * from %s """ % (self.DpTable)
        self.Cur.execute(SqlCommand)
        
        DpInfoList = []
        for Result in self.Cur:
            DpInfoList.append(Result)

        FileDict = {}        
        for Result in DpInfoList:
            DpGuid = Result[0]
            DpVersion = Result[1]
            NewDpFileName = Result[3]
            RePackage = Result[5]
            if RePackage == 'TRUE':
                RePackage = True
            else:
                RePackage = False
            for FileInfo in self.GetDpFileList(DpGuid, DpVersion):
                PathInfo = FileInfo[0]
                FileDict[PathInfo] = DpGuid, DpVersion, NewDpFileName, RePackage
                
        return FileDict
    
    ## Get (Guid, Version) from distribution file name information.
    #
    # @param DistributionFile: Distribution File  
    #
    def GetDpByName(self, DistributionFile):
        SqlCommand = """select * from %s where NewPkgFileName like '%s'""" % \
        (self.DpTable, '%' + DistributionFile)
        self.Cur.execute(SqlCommand)

        for Result in self.Cur:
            DpGuid = Result[0]
            DpVersion = Result[1]
            NewDpFileName = Result[3]
   
            return (DpGuid, DpVersion, NewDpFileName)
        else:
            return (None, None, None)
        
    ## Get a list of package information.
    #
    # @param Guid: package guid  
    # @param Version: package version
    #
    def GetPackage(self, Guid, Version, DpGuid='', DpVersion=''):
        
        if DpVersion == '' or DpGuid == '':

            (PackageGuid, PackageVersion) = (Guid, Version)
            SqlCommand = """select * from %s where PackageGuid ='%s' 
            and PackageVersion = '%s'""" % (self.PkgTable, PackageGuid, \
                                            PackageVersion)
            self.Cur.execute(SqlCommand)
        
        elif Version == None or len(Version.strip()) == 0:
            
            SqlCommand = """select * from %s where PackageGuid ='%s'""" % \
            (self.PkgTable, Guid)
            self.Cur.execute(SqlCommand)
        else:
            (PackageGuid, PackageVersion) = (Guid, Version)
            SqlCommand = """select * from %s where PackageGuid ='%s' and 
            PackageVersion = '%s'
                            and DpGuid = '%s' and DpVersion = '%s'""" % \
                            (self.PkgTable, PackageGuid, PackageVersion, \
                             DpGuid, DpVersion)
            self.Cur.execute(SqlCommand)

        PkgList = []
        for PkgInfo in self.Cur:
            PkgGuid = PkgInfo[0]
            PkgVersion = PkgInfo[1]
            InstallTime = PkgInfo[2]
            InstallPath = PkgInfo[5]
            PkgList.append((PkgGuid, PkgVersion, InstallTime, DpGuid, \
                            DpVersion, InstallPath))
        
        return PkgList
 
       
    ## Get a list of module in package information.
    #
    # @param Guid: A module guid
    # @param Version: A module version
    #
    def GetModInPackage(self, Guid, Version, Name, Path, PkgGuid='', PkgVersion=''):
        (ModuleGuid, ModuleVersion, ModuleName, InstallPath) = (Guid, Version, Name, Path)
        if PkgVersion == '' or PkgGuid == '':
            SqlCommand = """select * from %s where ModuleGuid ='%s' and 
            ModuleVersion = '%s' and InstallPath = '%s' 
            and ModuleName = '%s'""" % (self.ModInPkgTable, ModuleGuid, \
                                       ModuleVersion, InstallPath, ModuleName)
            self.Cur.execute(SqlCommand)
        else:
            SqlCommand = """select * from %s where ModuleGuid ='%s' and 
            ModuleVersion = '%s' and InstallPath = '%s' 
            and ModuleName = '%s' and PackageGuid ='%s' 
            and PackageVersion = '%s'
                            """ % (self.ModInPkgTable, ModuleGuid, \
                                   ModuleVersion, InstallPath, ModuleName, PkgGuid, PkgVersion)
            self.Cur.execute(SqlCommand)

        ModList = []
        for ModInfo in self.Cur:
            ModGuid = ModInfo[0]
            ModVersion = ModInfo[1]
            InstallTime = ModInfo[2]
            InstallPath = ModInfo[5]
            ModList.append((ModGuid, ModVersion, InstallTime, PkgGuid, \
                            PkgVersion, InstallPath))
        
        return ModList
    
    ## Get a list of module standalone.
    #
    # @param Guid: A module guid 
    # @param Version: A module version 
    #
    def GetStandaloneModule(self, Guid, Version, Name, Path, DpGuid='', DpVersion=''):
        (ModuleGuid, ModuleVersion, ModuleName, InstallPath) = (Guid, Version, Name, Path)
        if DpGuid == '':
            SqlCommand = """select * from %s where ModuleGuid ='%s' and 
            ModuleVersion = '%s' and InstallPath = '%s' 
            and ModuleName = '%s'""" % (self.StandaloneModTable, ModuleGuid, \
                                       ModuleVersion, InstallPath, ModuleName)
            self.Cur.execute(SqlCommand)
        
        else:
            SqlCommand = """select * from %s where ModuleGuid ='%s' and 
            ModuleVersion = '%s' and InstallPath = '%s' and ModuleName = '%s' and DpGuid ='%s' and DpVersion = '%s' 
                            """ % (self.StandaloneModTable, ModuleGuid, \
                                   ModuleVersion, ModuleName, InstallPath, DpGuid, DpVersion)
            self.Cur.execute(SqlCommand)

        ModList = []
        for ModInfo in self.Cur:
            ModGuid = ModInfo[0]
            ModVersion = ModInfo[1]
            InstallTime = ModInfo[2]
            InstallPath = ModInfo[5]
            ModList.append((ModGuid, ModVersion, InstallTime, DpGuid, \
                            DpVersion, InstallPath))
        
        return ModList
    
    ## Get a list of module information that comes from DP.
    #
    # @param DpGuid: A Distrabution Guid 
    # @param DpVersion: A Distrabution version  
    #
    def GetSModInsPathListFromDp(self, DpGuid, DpVersion):

        PathList = []
        SqlCommand = """select InstallPath from %s where DpGuid ='%s' 
        and DpVersion = '%s'
                        """ % (self.StandaloneModTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)

        for Result in self.Cur:
            InstallPath = Result[0]
            PathList.append(InstallPath)
        
        return PathList
    
    ## Get a list of package information.
    #
    # @param DpGuid: A Distrabution Guid 
    # @param DpVersion: A Distrabution version 
    #
    def GetPackageListFromDp(self, DpGuid, DpVersion):

        SqlCommand = """select * from %s where DpGuid ='%s' and 
        DpVersion = '%s' """ % (self.PkgTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)

        PkgList = []
        for PkgInfo in self.Cur:
            PkgGuid = PkgInfo[0]
            PkgVersion = PkgInfo[1]
            InstallPath = PkgInfo[5]
            PkgList.append((PkgGuid, PkgVersion, InstallPath))
        
        return PkgList
    
    ## Get a list of modules that depends on package information from a DP.
    #
    # @param DpGuid: A Distrabution Guid 
    # @param DpVersion: A Distrabution version 
    #
    def GetDpDependentModuleList(self, DpGuid, DpVersion):
        
        ModList = []
        PkgList = self.GetPackageListFromDp(DpGuid, DpVersion)
        if len(PkgList) > 0:
            return ModList
        
        for Pkg in PkgList:
            #
            # get all in-package modules that depends on current 
            # Pkg (Guid match, Version match or NA) but not belong to 
            # current Pkg
            #
            SqlCommand = """select t1.ModuleGuid, t1.ModuleVersion, 
            t1.InstallPath from %s as t1, %s as t2 where 
            t1.ModuleGuid = t2.ModuleGuid and 
            t1.ModuleVersion = t2.ModuleVersion and t2.DepexGuid ='%s' 
            and (t2.DepexVersion = '%s' or t2.DepexVersion = 'N/A') and
            t1.PackageGuid != '%s' and t1.PackageVersion != '%s'
                        """ % (self.ModInPkgTable, \
                               self.ModDepexTable, Pkg[0], Pkg[1], Pkg[0], \
                               Pkg[1])
            self.Cur.execute(SqlCommand)
            for ModInfo in self.Cur:
                ModGuid = ModInfo[0]
                ModVersion = ModInfo[1]
                InstallPath = ModInfo[2]
                ModList.append((ModGuid, ModVersion, InstallPath))

            #
            # get all modules from standalone modules that depends on current 
            #Pkg (Guid match, Version match or NA) but not in current dp
            #
            SqlCommand = \
            """select t1.ModuleGuid, t1.ModuleVersion, t1.InstallPath 
            from %s as t1, %s as t2 where t1.ModuleGuid = t2.ModuleGuid and 
            t1.ModuleVersion = t2.ModuleVersion and t2.DepexGuid ='%s' 
            and (t2.DepexVersion = '%s' or t2.DepexVersion = 'N/A') and
                            t1.DpGuid != '%s' and t1.DpVersion != '%s'
                        """ % \
                        (self.StandaloneModTable, self.ModDepexTable, Pkg[0], \
                         Pkg[1], DpGuid, DpVersion)
            self.Cur.execute(SqlCommand)
            for ModInfo in self.Cur:
                ModGuid = ModInfo[0]
                ModVersion = ModInfo[1]
                InstallPath = ModInfo[2]
                ModList.append((ModGuid, ModVersion, InstallPath))
        
        
        return ModList

    ## Get Dp's list of modules.
    #
    # @param DpGuid: A Distrabution Guid 
    # @param DpVersion: A Distrabution version 
    #
    def GetDpModuleList(self, DpGuid, DpVersion):      
        ModList = []
        #
        # get Dp module list from the DpFileList table
        #
        SqlCommand = """select FilePath 
                        from %s
                        where DpGuid = '%s' and DpVersion = '%s' and 
                        FilePath like '%%.inf'
                    """ % (self.DpFileListTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)
        for ModuleInfo in self.Cur:
            FilePath = ModuleInfo[0]
            ModList.append(os.path.join(self.Workspace, FilePath))
            
        return ModList        

        
    ## Get a module depex
    #
    # @param DpGuid: A module Guid 
    # @param DpVersion: A module version 
    # @param Path:
    #
    def GetModuleDepex(self, Guid, Version, Path):
                
        #
        # Get module depex information to DB.
        #
        SqlCommand = """select * from %s where ModuleGuid ='%s' and 
        ModuleVersion = '%s' and InstallPath ='%s'
                            """ % (self.ModDepexTable, Guid, Version, Path)
        self.Cur.execute(SqlCommand)

        
        DepexList = []
        for DepInfo in self.Cur:
            DepexGuid = DepInfo[3]
            DepexVersion = DepInfo[4]
            DepexList.append((DepexGuid, DepexVersion))
        
        return DepexList
 
    ## Inventory the distribution installed to current workspace
    #
    # Inventory the distribution installed to current workspace
    #   
    def InventoryDistInstalled(self):
        SqlCommand = """select * from %s """ % (self.DpTable)
        self.Cur.execute(SqlCommand)
        
        DpInfoList = []
        for Result in self.Cur:
            DpGuid = Result[0]
            DpVersion = Result[1]
            DpAliasName = Result[3]
            DpFileName = Result[4]            
            DpInfoList.append((DpGuid, DpVersion, DpFileName, DpAliasName))
        
        return DpInfoList     

    ## Close entire database
    #
    # Close the connection and cursor
    #
    def CloseDb(self):
        #
        # drop the dummy table
        #
        SqlCommand = """
        drop table IF EXISTS %s 
        """ % self.DummyTable
        self.Cur.execute(SqlCommand)
        self.Conn.commit()
        
        self.Cur.close()
        self.Conn.close()

    ## Convert To Sql String
    #
    # 1. Replace "'" with "''" in each item of StringList
    # 
    # @param StringList:  A list for strings to be converted
    #
    def __ConvertToSqlString(self, StringList):
        if self.DpTable:
            pass
        return map(lambda s: s.replace("'", "''") , StringList)



    