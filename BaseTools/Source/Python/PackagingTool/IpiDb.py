## @file
# This file is for installed package information database operations
#
# Copyright (c) 2007 ~ 2008, Intel Corporation
# All rights reserved. This program and the accompanying materials
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
import os
import time
import Common.EdkLogger as EdkLogger

from CommonDataClass import DistributionPackageClass

## IpiDb
#
# This class represents the installed package information databse
# Add/Remove/Get installed distribution package information here.
# 
# 
# @param object:      Inherited from object class
# @param DbPath:      A string for the path of the database
#
# @var Conn:          Connection of the database
# @var Cur:           Cursor of the connection
#
class IpiDatabase(object):
    def __init__(self, DbPath):
        Dir = os.path.dirname(DbPath)
        if not os.path.isdir(Dir):
            os.mkdir(Dir)
        self.Conn = sqlite3.connect(DbPath, isolation_level = 'DEFERRED')
        self.Conn.execute("PRAGMA page_size=4096")
        self.Conn.execute("PRAGMA synchronous=OFF")
        self.Cur = self.Conn.cursor()
        self.DpTable = 'DpInfo'
        self.PkgTable = 'PkgInfo'
        self.ModInPkgTable = 'ModInPkgInfo'
        self.StandaloneModTable = 'StandaloneModInfo'
        self.ModDepexTable = 'ModDepexInfo'
        self.DpFileListTable = 'DpFileListInfo'
    
    ## Initialize build database
    #
    #
    def InitDatabase(self):
        EdkLogger.verbose("\nInitialize IPI database started ...")
        
        #
        # Create new table
        #
        SqlCommand = """create table IF NOT EXISTS %s (DpGuid TEXT NOT NULL,
                                                       DpVersion TEXT NOT NULL,
                                                       InstallTime REAL NOT NULL,
                                                       PkgFileName TEXT,
                                                       PRIMARY KEY (DpGuid, DpVersion)
                                                      )""" % self.DpTable
        self.Cur.execute(SqlCommand)
        
        SqlCommand = """create table IF NOT EXISTS %s (FilePath TEXT NOT NULL,
                                                       DpGuid TEXT,
                                                       DpVersion TEXT,
                                                       PRIMARY KEY (FilePath)
                                                      )""" % self.DpFileListTable
        self.Cur.execute(SqlCommand)
        
        SqlCommand = """create table IF NOT EXISTS %s (PackageGuid TEXT NOT NULL,
                                                       PackageVersion TEXT NOT NULL,
                                                       InstallTime REAL NOT NULL,
                                                       DpGuid TEXT,
                                                       DpVersion TEXT,
                                                       InstallPath TEXT NOT NULL,
                                                       PRIMARY KEY (PackageGuid, PackageVersion, InstallPath)
                                                      )""" % self.PkgTable
        self.Cur.execute(SqlCommand)
        
        SqlCommand = """create table IF NOT EXISTS %s (ModuleGuid TEXT NOT NULL,
                                                       ModuleVersion TEXT NOT NULL,
                                                       InstallTime REAL NOT NULL,
                                                       PackageGuid TEXT,
                                                       PackageVersion TEXT,
                                                       InstallPath TEXT NOT NULL,
                                                       PRIMARY KEY (ModuleGuid, ModuleVersion, InstallPath)
                                                      )""" % self.ModInPkgTable
        self.Cur.execute(SqlCommand)
        
        SqlCommand = """create table IF NOT EXISTS %s (ModuleGuid TEXT NOT NULL,
                                                       ModuleVersion TEXT NOT NULL,
                                                       InstallTime REAL NOT NULL,
                                                       DpGuid TEXT,
                                                       DpVersion TEXT,
                                                       InstallPath TEXT NOT NULL,
                                                       PRIMARY KEY (ModuleGuid, ModuleVersion, InstallPath)
                                                      )""" % self.StandaloneModTable
        self.Cur.execute(SqlCommand)
        
        SqlCommand = """create table IF NOT EXISTS %s (ModuleGuid TEXT NOT NULL,
                                                       ModuleVersion TEXT NOT NULL,
                                                       InstallPath TEXT NOT NULL,
                                                       DepexGuid TEXT,
                                                       DepexVersion TEXT
                                                      )""" % self.ModDepexTable
        self.Cur.execute(SqlCommand)
        
        self.Conn.commit()
        
        EdkLogger.verbose("Initialize IPI database ... DONE!")

    ## Add a distribution install information from DpObj
    #
    # @param DpObj:
    #
    def AddDPObject(self, DpObj):
        
        for PkgKey in DpObj.PackageSurfaceArea.keys():
            PkgGuid = PkgKey[0]
            PkgVersion = PkgKey[1]
            PkgInstallPath = PkgKey[2]
            self.AddPackage(PkgGuid, PkgVersion, DpObj.Header.Guid, DpObj.Header.Version, PkgInstallPath)
            PkgObj = DpObj.PackageSurfaceArea[PkgKey]
            for ModKey in PkgObj.Modules.keys():
                ModGuid = ModKey[0]
                ModVersion = ModKey[1]
                ModInstallPath = ModKey[2]
                self.AddModuleInPackage(ModGuid, ModVersion, PkgGuid, PkgVersion, ModInstallPath)
                ModObj = PkgObj.Modules[ModKey]
                for Dep in ModObj.PackageDependencies:
                    DepexGuid = Dep.PackageGuid
                    DepexVersion = Dep.PackageVersion
                    self.AddModuleDepex(ModGuid, ModVersion, ModInstallPath, DepexGuid, DepexVersion)
            for FilePath in PkgObj.FileList:
                self.AddDpFilePathList(DpObj.Header.Guid, DpObj.Header.Version, FilePath)

        for ModKey in DpObj.ModuleSurfaceArea.keys():
            ModGuid = ModKey[0]
            ModVersion = ModKey[1]
            ModInstallPath = ModKey[2]
            self.AddStandaloneModule(ModGuid, ModVersion, DpObj.Header.Guid, DpObj.Header.Version, ModInstallPath)
            ModObj = DpObj.ModuleSurfaceArea[ModKey]
            for Dep in ModObj.PackageDependencies:
                DepexGuid = Dep.PackageGuid
                DepexVersion = Dep.PackageVersion
                self.AddModuleDepex(ModGuid, ModVersion, ModInstallPath, DepexGuid, DepexVersion)
            for FilePath in ModObj.FileList:
                self.AddDpFilePathList(DpObj.Header.Guid, DpObj.Header.Version, FilePath)
                
        self.AddDp(DpObj.Header.Guid, DpObj.Header.Version, DpObj.Header.FileName)    
    ## Add a distribution install information
    #
    # @param Guid:  
    # @param Version:
    # @param PkgFileName:
    #
    def AddDp(self, Guid, Version, PkgFileName = None):
        
        if Version == None or len(Version.strip()) == 0:
            Version = 'N/A'
        
        #
        # Add newly installed DP information to DB.
        #
        if PkgFileName == None or len(PkgFileName.strip()) == 0:
            PkgFileName = 'N/A'
        (Guid, Version, PkgFileName) = (Guid, Version, PkgFileName)
        CurrentTime = time.time()
        SqlCommand = """insert into %s values('%s', '%s', %s, '%s')""" % (self.DpTable, Guid, Version, CurrentTime, PkgFileName)
        self.Cur.execute(SqlCommand)
        self.Conn.commit()
        
    ## Add a file list from DP
    #
    # @param DpGuid:  
    # @param DpVersion:
    # @param Path
    #
    def AddDpFilePathList(self, DpGuid, DpVersion, Path):
        
        SqlCommand = """insert into %s values('%s', '%s', '%s')""" % (self.DpFileListTable, Path, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)
        self.Conn.commit()
            
    ## Add a package install information
    #
    # @param Guid:  
    # @param Version:
    # @param DpGuid:  
    # @param DpVersion:
    # @param Path
    #
    def AddPackage(self, Guid, Version, DpGuid = None, DpVersion = None, Path = ''):
        
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
        SqlCommand = """insert into %s values('%s', '%s', %s, '%s', '%s', '%s')""" % (self.PkgTable, Guid, Version, CurrentTime, DpGuid, DpVersion, Path)
        self.Cur.execute(SqlCommand)
        self.Conn.commit()
        
    ## Add a module that from a package install information
    #
    # @param Guid:  
    # @param Version:
    # @param PkgFileName:
    #
    def AddModuleInPackage(self, Guid, Version, PkgGuid = None, PkgVersion = None, Path = ''):
        
        if Version == None or len(Version.strip()) == 0:
            Version = 'N/A'
        
        if PkgGuid == None or len(PkgGuid.strip()) == 0:
            PkgGuid = 'N/A'
        
        if PkgVersion == None or len(PkgVersion.strip()) == 0:
            PkgVersion = 'N/A'
        
        #
        # Add module from package information to DB.
        #
        CurrentTime = time.time()
        SqlCommand = """insert into %s values('%s', '%s', %s, '%s', '%s', '%s')""" % (self.ModInPkgTable, Guid, Version, CurrentTime, PkgGuid, PkgVersion, Path)
        self.Cur.execute(SqlCommand)
        self.Conn.commit()
    
    ## Add a module that is standalone install information
    #
    # @param Guid:  
    # @param Version:
    # @param PkgFileName:
    #
    def AddStandaloneModule(self, Guid, Version, DpGuid = None, DpVersion = None, Path = ''):
        
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
        SqlCommand = """insert into %s values('%s', '%s', %s, '%s', '%s', '%s')""" % (self.StandaloneModTable, Guid, Version, CurrentTime, DpGuid, DpVersion, Path)
        self.Cur.execute(SqlCommand)
        self.Conn.commit()
    
    ## Add a module depex
    #
    # @param Guid:  
    # @param Version:
    # @param DepexGuid:
    # @param DepexVersion:
    #
    def AddModuleDepex(self, Guid, Version, Path, DepexGuid = None, DepexVersion = None):
                
        if DepexGuid == None or len(DepexGuid.strip()) == 0:
            DepexGuid = 'N/A'
        
        if DepexVersion == None or len(DepexVersion.strip()) == 0:
            DepexVersion = 'N/A'
        
        #
        # Add module depex information to DB.
        #
        
        SqlCommand = """insert into %s values('%s', '%s', '%s', '%s', '%s')""" % (self.ModDepexTable, Guid, Version, Path, DepexGuid, DepexVersion)
        self.Cur.execute(SqlCommand)
        self.Conn.commit()
        
    ## Remove a distribution install information, if no version specified, remove all DPs with this Guid.
    #
    # @param DpObj:  
    #
    def RemoveDpObj(self, DpGuid, DpVersion):
        
        PkgList = self.GetPackageListFromDp(DpGuid, DpVersion)
        
        # delete from ModDepex the standalone module's dependency
        SqlCommand = """delete from ModDepexInfo where ModDepexInfo.ModuleGuid in 
                        (select ModuleGuid from StandaloneModInfo as B where B.DpGuid = '%s' and B.DpVersion = '%s')
                         and ModDepexInfo.ModuleVersion in
                        (select ModuleVersion from StandaloneModInfo as B where B.DpGuid = '%s' and B.DpVersion = '%s')
                         and ModDepexInfo.InstallPath in
                        (select InstallPath from StandaloneModInfo as B where B.DpGuid = '%s' and B.DpVersion = '%s') """ \
                        %(DpGuid, DpVersion, DpGuid, DpVersion, DpGuid, DpVersion)

#        SqlCommand = """delete from %s where %s.DpGuid ='%s' and %s.DpVersion = '%s' and 
#                        %s.ModuleGuid = %s.ModuleGuid and %s.ModuleVersion = %s.ModuleVersion and 
#                        %s.InstallPath = %s.InstallPath""" \
#                        % (self.ModDepexTable, self.StandaloneModTable, DpGuid, self.StandaloneModTable, DpVersion, self.ModDepexTable, self.StandaloneModTable, self.ModDepexTable, self.StandaloneModTable, self.ModDepexTable, self.StandaloneModTable)
#        print SqlCommand
        self.Cur.execute(SqlCommand)
        
        # delete from ModDepex the from pkg module's dependency
        for Pkg in PkgList:
#            SqlCommand = """delete from %s where %s.PackageGuid ='%s' and %s.PackageVersion = '%s' and 
#                        %s.ModuleGuid = %s.ModuleGuid and %s.ModuleVersion = %s.ModuleVersion and 
#                        %s.InstallPath = %s.InstallPath""" \
#                        % (self.ModDepexTable, self.ModInPkgTable, Pkg[0], self.ModInPkgTable, Pkg[1], self.ModDepexTable, self.ModInPkgTable, self.ModDepexTable, self.ModInPkgTable, self.ModDepexTable, self.ModInPkgTable)
            SqlCommand = """delete from ModDepexInfo where ModDepexInfo.ModuleGuid in 
                            (select ModuleGuid from ModInPkgInfo where ModInPkgInfo.PackageGuid ='%s' and ModInPkgInfo.PackageVersion = '%s')
                            and ModDepexInfo.ModuleVersion in
                            (select ModuleVersion from ModInPkgInfo where ModInPkgInfo.PackageGuid ='%s' and ModInPkgInfo.PackageVersion = '%s')
                            and ModDepexInfo.InstallPath in
                            (select InstallPath from ModInPkgInfo where ModInPkgInfo.PackageGuid ='%s' and ModInPkgInfo.PackageVersion = '%s')""" \
                            % (Pkg[0], Pkg[1],Pkg[0], Pkg[1],Pkg[0], Pkg[1])
            
            self.Cur.execute(SqlCommand)
        
        # delete the standalone module
        SqlCommand = """delete from %s where DpGuid ='%s' and DpVersion = '%s'""" % (self.StandaloneModTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)
        
        # delete the from pkg module
        for Pkg in PkgList:
            SqlCommand = """delete from %s where %s.PackageGuid ='%s' and %s.PackageVersion = '%s'""" \
                        % (self.ModInPkgTable, self.ModInPkgTable, Pkg[0], self.ModInPkgTable, Pkg[1])
            self.Cur.execute(SqlCommand)
        
        # delete packages
        SqlCommand = """delete from %s where DpGuid ='%s' and DpVersion = '%s'""" % (self.PkgTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)
        
        # delete file list from DP
        SqlCommand = """delete from %s where DpGuid ='%s' and DpVersion = '%s'""" % (self.DpFileListTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)
            
        # delete DP
        SqlCommand = """delete from %s where DpGuid ='%s' and DpVersion = '%s'""" % (self.DpTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)
        
        self.Conn.commit()
        
    ## Get a list of distribution install information.
    #
    # @param Guid:  
    # @param Version:  
    #
    def GetDp(self, Guid, Version):
        
        if Version == None or len(Version.strip()) == 0:
            Version = 'N/A'
            EdkLogger.verbose("\nGetting list of DP install information started ...")
            (DpGuid, DpVersion) = (Guid, Version)
            SqlCommand = """select * from %s where DpGuid ='%s'""" % (self.DpTable, DpGuid)
            self.Cur.execute(SqlCommand)
        
        else:
            EdkLogger.verbose("\nGetting DP install information started ...")
            (DpGuid, DpVersion) = (Guid, Version)
            SqlCommand = """select * from %s where DpGuid ='%s' and DpVersion = '%s'""" % (self.DpTable, DpGuid, DpVersion)
            self.Cur.execute(SqlCommand)

        DpList = []
        for DpInfo in self.Cur:
            DpGuid = DpInfo[0]
            DpVersion = DpInfo[1]
            InstallTime = DpInfo[2]
            PkgFileName = DpInfo[3]
            DpList.append((DpGuid, DpVersion, InstallTime, PkgFileName))
        
        EdkLogger.verbose("Getting DP install information ... DONE!")
        return DpList
    
    ## Get a list of distribution install file path information.
    #
    # @param Guid:  
    # @param Version:  
    #
    def GetDpFileList(self, Guid, Version):
        
        (DpGuid, DpVersion) = (Guid, Version)
        SqlCommand = """select * from %s where DpGuid ='%s' and DpVersion = '%s'""" % (self.DpFileListTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)

        PathList = []
        for Result in self.Cur:
            Path = Result[0]
            PathList.append(Path)
        
        return PathList
    
    ## Get a list of package information.
    #
    # @param Guid:  
    # @param Version:  
    #
    def GetPackage(self, Guid, Version, DpGuid = '', DpVersion = ''):
        
        if DpVersion == '' or DpGuid == '':

            (PackageGuid, PackageVersion) = (Guid, Version)
            SqlCommand = """select * from %s where PackageGuid ='%s' and PackageVersion = '%s'""" % (self.PkgTable, PackageGuid, PackageVersion)
            self.Cur.execute(SqlCommand)
        
        elif Version == None or len(Version.strip()) == 0:
            
            SqlCommand = """select * from %s where PackageGuid ='%s'""" % (self.PkgTable, Guid)
            self.Cur.execute(SqlCommand)
        else:
            (PackageGuid, PackageVersion) = (Guid, Version)
            SqlCommand = """select * from %s where PackageGuid ='%s' and PackageVersion = '%s'
                            and DpGuid = '%s' and DpVersion = '%s'""" % (self.PkgTable, PackageGuid, PackageVersion, DpGuid, DpVersion)
            self.Cur.execute(SqlCommand)

        PkgList = []
        for PkgInfo in self.Cur:
            PkgGuid = PkgInfo[0]
            PkgVersion = PkgInfo[1]
            InstallTime = PkgInfo[2]
            InstallPath = PkgInfo[5]
            PkgList.append((PkgGuid, PkgVersion, InstallTime, DpGuid, DpVersion, InstallPath))
        
        return PkgList
    
    ## Get a list of module in package information.
    #
    # @param Guid:  
    # @param Version:  
    #
    def GetModInPackage(self, Guid, Version, PkgGuid = '', PkgVersion = ''):
        
        if PkgVersion == '' or PkgGuid == '':

            (ModuleGuid, ModuleVersion) = (Guid, Version)
            SqlCommand = """select * from %s where ModuleGuid ='%s' and ModuleVersion = '%s'""" % (self.ModInPkgTable, ModuleGuid, ModuleVersion)
            self.Cur.execute(SqlCommand)
        
        else:
            (ModuleGuid, ModuleVersion) = (Guid, Version)
            SqlCommand = """select * from %s where ModuleGuid ='%s' and ModuleVersion = '%s' and PackageGuid ='%s' and PackageVersion = '%s'
                            """ % (self.ModInPkgTable, ModuleGuid, ModuleVersion, PkgGuid, PkgVersion)
            self.Cur.execute(SqlCommand)

        ModList = []
        for ModInfo in self.Cur:
            ModGuid = ModInfo[0]
            ModVersion = ModInfo[1]
            InstallTime = ModInfo[2]
            InstallPath = ModInfo[5]
            ModList.append((ModGuid, ModVersion, InstallTime, PkgGuid, PkgVersion, InstallPath))
        
        return ModList
    
    ## Get a list of module standalone.
    #
    # @param Guid:  
    # @param Version:  
    #
    def GetStandaloneModule(self, Guid, Version, DpGuid = '', DpVersion = ''):
        
        if DpGuid == '':

            (ModuleGuid, ModuleVersion) = (Guid, Version)
            SqlCommand = """select * from %s where ModuleGuid ='%s' and ModuleVersion = '%s'""" % (self.StandaloneModTable, ModuleGuid, ModuleVersion)
            self.Cur.execute(SqlCommand)
        
        else:
            (ModuleGuid, ModuleVersion) = (Guid, Version)
            SqlCommand = """select * from %s where ModuleGuid ='%s' and ModuleVersion = '%s' and DpGuid ='%s' and DpVersion = '%s'
                            """ % (self.StandaloneModTable, ModuleGuid, ModuleVersion, DpGuid, DpVersion)
            self.Cur.execute(SqlCommand)

        ModList = []
        for ModInfo in self.Cur:
            ModGuid = ModInfo[0]
            ModVersion = ModInfo[1]
            InstallTime = ModInfo[2]
            InstallPath = ModInfo[5]
            ModList.append((ModGuid, ModVersion, InstallTime, DpGuid, DpVersion, InstallPath))
        
        return ModList
    
    ## Get a list of module information that comes from DP.
    #
    # @param DpGuid:  
    # @param DpVersion:  
    #
    def GetStandaloneModuleInstallPathListFromDp(self, DpGuid, DpVersion):

        PathList = []
        SqlCommand = """select t1.InstallPath from %s t1 where t1.DpGuid ='%s' and t1.DpVersion = '%s'
                        """ % (self.StandaloneModTable, DpGuid, DpVersion)
        self.Cur.execute(SqlCommand)

        for Result in self.Cur:
            InstallPath = Result[0]
            PathList.append(InstallPath)
        
        return PathList
    
    ## Get a list of package information.
    #
    # @param DpGuid:  
    # @param DpVersion:  
    #
    def GetPackageListFromDp(self, DpGuid, DpVersion):
        

        SqlCommand = """select * from %s where DpGuid ='%s' and DpVersion = '%s'
                        """ % (self.PkgTable, DpGuid, DpVersion)
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
    # @param DpGuid:  
    # @param DpVersion:  
    #
    def GetDpDependentModuleList(self, DpGuid, DpVersion):
        
        ModList = []
        PkgList = self.GetPackageListFromDp(DpGuid, DpVersion)
        if len(PkgList) > 0:
            return ModList
        
        for Pkg in PkgList:
            SqlCommand = """select t1.ModuleGuid, t1.ModuleVersion, t1.InstallPath 
                            from %s as t1, %s as t2, where t1.ModuleGuid = t2.ModuleGuid and 
                            t1.ModuleVersion = t2.ModuleVersion and t2.DepexGuid ='%s' and (t2.DepexVersion = '%s' or t2.DepexVersion = 'N/A') and
                            t1.PackageGuid != '%s' and t1.PackageVersion != '%s'
                        """ % (self.ModInPkgTable, self.ModDepexTable, Pkg[0], Pkg[1], Pkg[0], Pkg[1])
            self.Cur.execute(SqlCommand)
            for ModInfo in self.Cur:
                ModGuid = ModInfo[0]
                ModVersion = ModInfo[1]
                InstallPath = ModInfo[2]
                ModList.append((ModGuid, ModVersion, InstallPath))

            SqlCommand = """select t1.ModuleGuid, t1.ModuleVersion, t1.InstallPath 
                            from %s as t1, %s as t2, where t1.ModuleGuid = t2.ModuleGuid and 
                            t1.ModuleVersion = t2.ModuleVersion and t2.DepexGuid ='%s' and (t2.DepexVersion = '%s' or t2.DepexVersion = 'N/A') and
                            t1.DpGuid != '%s' and t1.DpVersion != '%s'
                        """ % (self.StandaloneModTable, self.ModDepexTable, Pkg[0], Pkg[1], DpGuid, DpVersion)
            self.Cur.execute(SqlCommand)
            for ModInfo in self.Cur:
                ModGuid = ModInfo[0]
                ModVersion = ModInfo[1]
                InstallPath = ModInfo[2]
                ModList.append((ModGuid, ModVersion, InstallPath))
        
        
        return ModList
    
    ## Get a module depex
    #
    # @param Guid:  
    # @param Version:
    # @param Path:
    #
    def GetModuleDepex(self, Guid, Version, Path):
                
        #
        # Get module depex information to DB.
        #
        
        SqlCommand = """select * from %s where ModuleGuid ='%s' and ModuleVersion = '%s' and InstallPath ='%s'
                            """ % (self.ModDepexTable, Guid, Version, Path)
        self.Cur.execute(SqlCommand)
        self.Conn.commit()
        
        DepexList = []
        for DepInfo in self.Cur:
            DepexGuid = DepInfo[3]
            DepexVersion = DepInfo[4]
            DepexList.append((DepexGuid, DepexVersion))
        
        return DepexList
    
    ## Close entire database
    #
    # Close the connection and cursor
    #
    def CloseDb(self):
        
        self.Cur.close()
        self.Conn.close()

    ## Convert To Sql String
    #
    # 1. Replace "'" with "''" in each item of StringList
    # 
    # @param StringList:  A list for strings to be converted
    #
    def __ConvertToSqlString(self, StringList):
        return map(lambda s: s.replace("'", "''") , StringList)
##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    EdkLogger.Initialize()
    EdkLogger.SetLevel(EdkLogger.DEBUG_0)
    DATABASE_PATH = "C://MyWork//Conf//.cache//XML.db"
    Db = IpiDatabase(DATABASE_PATH)
    Db.InitDatabase()

    