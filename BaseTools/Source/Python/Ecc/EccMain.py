## @file
# This file is used to be the main entrance of ECC tool
#
# Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import Common.LongFilePathOs as os, time, glob, sys
import Common.EdkLogger as EdkLogger
from Ecc import Database
from Ecc import EccGlobalData
from Ecc.MetaDataParser import *
from optparse import OptionParser
from Ecc.Configuration import Configuration
from Ecc.Check import Check
import Common.GlobalData as GlobalData

from Common.StringUtils import NormPath
from Common.BuildVersion import gBUILD_VERSION
from Common import BuildToolError
from Common.Misc import PathClass
from Common.Misc import DirCache
from Ecc.MetaFileWorkspace.MetaFileParser import DscParser
from Ecc.MetaFileWorkspace.MetaFileParser import DecParser
from Ecc.MetaFileWorkspace.MetaFileParser import InfParser
from Ecc.MetaFileWorkspace.MetaFileParser import Fdf
from Ecc.MetaFileWorkspace.MetaFileTable import MetaFileStorage
from Ecc import c
import re, string
from Ecc.Exception import *
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.MultipleWorkspace import MultipleWorkspace as mws

## Ecc
#
# This class is used to define Ecc main entrance
#
# @param object:          Inherited from object class
#
class Ecc(object):
    def __init__(self):
        # Version and Copyright
        self.VersionNumber = ("1.0" + " Build " + gBUILD_VERSION)
        self.Version = "%prog Version " + self.VersionNumber
        self.Copyright = "Copyright (c) 2009 - 2018, Intel Corporation  All rights reserved."

        self.InitDefaultConfigIni()
        self.OutputFile = 'output.txt'
        self.ReportFile = 'Report.csv'
        self.ExceptionFile = 'exception.xml'
        self.IsInit = True
        self.ScanSourceCode = True
        self.ScanMetaData = True
        self.MetaFile = ''
        self.OnlyScan = None

        # Parse the options and args
        self.ParseOption()
        EdkLogger.info(time.strftime("%H:%M:%S, %b.%d %Y ", time.localtime()) + "[00:00]" + "\n")

        WorkspaceDir = os.path.normcase(os.path.normpath(os.environ["WORKSPACE"]))
        os.environ["WORKSPACE"] = WorkspaceDir

        # set multiple workspace
        PackagesPath = os.getenv("PACKAGES_PATH")
        mws.setWs(WorkspaceDir, PackagesPath)

        GlobalData.gWorkspace = WorkspaceDir

        GlobalData.gGlobalDefines["WORKSPACE"]  = WorkspaceDir

        EdkLogger.info("Loading ECC configuration ... done")
        # Generate checkpoints list
        EccGlobalData.gConfig = Configuration(self.ConfigFile)

        # Generate exception list
        EccGlobalData.gException = ExceptionCheck(self.ExceptionFile)

        # Init Ecc database
        EccGlobalData.gDb = Database.Database(Database.DATABASE_PATH)
        EccGlobalData.gDb.InitDatabase(self.IsInit)

        #
        # Get files real name in workspace dir
        #
        GlobalData.gAllFiles = DirCache(GlobalData.gWorkspace)

        # Build ECC database
#         self.BuildDatabase()
        self.DetectOnlyScanDirs()

        # Start to check
        self.Check()

        # Show report
        self.GenReport()

        # Close Database
        EccGlobalData.gDb.Close()

    def InitDefaultConfigIni(self):
        paths = map(lambda p: os.path.join(p, 'Ecc', 'config.ini'), sys.path)
        paths = (os.path.realpath('config.ini'),) + tuple(paths)
        for path in paths:
            if os.path.exists(path):
                self.ConfigFile = path
                return
        self.ConfigFile = 'config.ini'


    ## DetectOnlyScan
    #
    # Detect whether only scanned folders have been enabled
    #
    def DetectOnlyScanDirs(self):
        if self.OnlyScan == True:
            OnlyScanDirs = []
            # Use regex here if multiple spaces or TAB exists in ScanOnlyDirList in config.ini file
            for folder in re.finditer(r'\S+', EccGlobalData.gConfig.ScanOnlyDirList):
                OnlyScanDirs.append(folder.group())
            if len(OnlyScanDirs) != 0:
                self.BuildDatabase(OnlyScanDirs)
            else:
                EdkLogger.error("ECC", BuildToolError.OPTION_VALUE_INVALID, ExtraData="Use -f option need to fill specific folders in config.ini file")
        else:
            self.BuildDatabase()


    ## BuildDatabase
    #
    # Build the database for target
    #
    def BuildDatabase(self, SpeciDirs = None):
        # Clean report table
        EccGlobalData.gDb.TblReport.Drop()
        EccGlobalData.gDb.TblReport.Create()

        # Build database
        if self.IsInit:
            if self.ScanMetaData:
                EdkLogger.quiet("Building database for Meta Data File ...")
                self.BuildMetaDataFileDatabase(SpeciDirs)
            if self.ScanSourceCode:
                EdkLogger.quiet("Building database for Meta Data File Done!")
                if SpeciDirs is None:
                    c.CollectSourceCodeDataIntoDB(EccGlobalData.gTarget)
                else:
                    for specificDir in SpeciDirs:
                        c.CollectSourceCodeDataIntoDB(os.path.join(EccGlobalData.gTarget, specificDir))

        EccGlobalData.gIdentifierTableList = GetTableList((MODEL_FILE_C, MODEL_FILE_H), 'Identifier', EccGlobalData.gDb)
        EccGlobalData.gCFileList = GetFileList(MODEL_FILE_C, EccGlobalData.gDb)
        EccGlobalData.gHFileList = GetFileList(MODEL_FILE_H, EccGlobalData.gDb)
        EccGlobalData.gUFileList = GetFileList(MODEL_FILE_UNI, EccGlobalData.gDb)

    ## BuildMetaDataFileDatabase
    #
    # Build the database for meta data files
    #
    def BuildMetaDataFileDatabase(self, SpecificDirs = None):
        ScanFolders = []
        if SpecificDirs is None:
            ScanFolders.append(EccGlobalData.gTarget)
        else:
            for specificDir in SpecificDirs:
                ScanFolders.append(os.path.join(EccGlobalData.gTarget, specificDir))
        EdkLogger.quiet("Building database for meta data files ...")
        Op = open(EccGlobalData.gConfig.MetaDataFileCheckPathOfGenerateFileList, 'w+')
        #SkipDirs = Read from config file
        SkipDirs = EccGlobalData.gConfig.SkipDirList
        SkipDirString = '|'.join(SkipDirs)
#         p = re.compile(r'.*[\\/](?:%s)[\\/]?.*' % SkipDirString)
        p = re.compile(r'.*[\\/](?:%s^\S)[\\/]?.*' % SkipDirString)
        for scanFolder in ScanFolders:
            for Root, Dirs, Files in os.walk(scanFolder):
                if p.match(Root.upper()):
                    continue
                for Dir in Dirs:
                    Dirname = os.path.join(Root, Dir)
                    if os.path.islink(Dirname):
                        Dirname = os.path.realpath(Dirname)
                        if os.path.isdir(Dirname):
                            # symlinks to directories are treated as directories
                            Dirs.remove(Dir)
                            Dirs.append(Dirname)

                for File in Files:
                    if len(File) > 4 and File[-4:].upper() == ".DEC":
                        Filename = os.path.normpath(os.path.join(Root, File))
                        EdkLogger.quiet("Parsing %s" % Filename)
                        Op.write("%s\r" % Filename)
                        #Dec(Filename, True, True, EccGlobalData.gWorkspace, EccGlobalData.gDb)
                        self.MetaFile = DecParser(Filename, MODEL_FILE_DEC, EccGlobalData.gDb.TblDec)
                        self.MetaFile.Start()
                        continue
                    if len(File) > 4 and File[-4:].upper() == ".DSC":
                        Filename = os.path.normpath(os.path.join(Root, File))
                        EdkLogger.quiet("Parsing %s" % Filename)
                        Op.write("%s\r" % Filename)
                        #Dsc(Filename, True, True, EccGlobalData.gWorkspace, EccGlobalData.gDb)
                        self.MetaFile = DscParser(PathClass(Filename, Root), MODEL_FILE_DSC, MetaFileStorage(EccGlobalData.gDb.TblDsc.Cur, Filename, MODEL_FILE_DSC, True))
                        # always do post-process, in case of macros change
                        self.MetaFile.DoPostProcess()
                        self.MetaFile.Start()
                        self.MetaFile._PostProcess()
                        continue
                    if len(File) > 4 and File[-4:].upper() == ".INF":
                        Filename = os.path.normpath(os.path.join(Root, File))
                        EdkLogger.quiet("Parsing %s" % Filename)
                        Op.write("%s\r" % Filename)
                        #Inf(Filename, True, True, EccGlobalData.gWorkspace, EccGlobalData.gDb)
                        self.MetaFile = InfParser(Filename, MODEL_FILE_INF, EccGlobalData.gDb.TblInf)
                        self.MetaFile.Start()
                        continue
                    if len(File) > 4 and File[-4:].upper() == ".FDF":
                        Filename = os.path.normpath(os.path.join(Root, File))
                        EdkLogger.quiet("Parsing %s" % Filename)
                        Op.write("%s\r" % Filename)
                        Fdf(Filename, True, EccGlobalData.gWorkspace, EccGlobalData.gDb)
                        continue
                    if len(File) > 4 and File[-4:].upper() == ".UNI":
                        Filename = os.path.normpath(os.path.join(Root, File))
                        EdkLogger.quiet("Parsing %s" % Filename)
                        Op.write("%s\r" % Filename)
                        FileID = EccGlobalData.gDb.TblFile.InsertFile(Filename, MODEL_FILE_UNI)
                        EccGlobalData.gDb.TblReport.UpdateBelongsToItemByFile(FileID, File)
                        continue

        Op.close()

        # Commit to database
        EccGlobalData.gDb.Conn.commit()

        EdkLogger.quiet("Building database for meta data files done!")

    ##
    #
    # Check each checkpoint
    #
    def Check(self):
        EdkLogger.quiet("Checking ...")
        EccCheck = Check()
        EccCheck.Check()
        EdkLogger.quiet("Checking  done!")

    ##
    #
    # Generate the scan report
    #
    def GenReport(self):
        EdkLogger.quiet("Generating report ...")
        EccGlobalData.gDb.TblReport.ToCSV(self.ReportFile)
        EdkLogger.quiet("Generating report done!")

    def GetRealPathCase(self, path):
        TmpPath = path.rstrip(os.sep)
        PathParts = TmpPath.split(os.sep)
        if len(PathParts) == 0:
            return path
        if len(PathParts) == 1:
            if PathParts[0].strip().endswith(':'):
                return PathParts[0].upper()
            # Relative dir, list . current dir
            Dirs = os.listdir('.')
            for Dir in Dirs:
                if Dir.upper() == PathParts[0].upper():
                    return Dir

        if PathParts[0].strip().endswith(':'):
            PathParts[0] = PathParts[0].upper()
        ParentDir = PathParts[0]
        RealPath = ParentDir
        if PathParts[0] == '':
            RealPath = os.sep
            ParentDir = os.sep

        PathParts.remove(PathParts[0])    # need to remove the parent
        for Part in PathParts:
            Dirs = os.listdir(ParentDir + os.sep)
            for Dir in Dirs:
                if Dir.upper() == Part.upper():
                    RealPath += os.sep
                    RealPath += Dir
                    break
            ParentDir += os.sep
            ParentDir += Dir

        return RealPath

    ## ParseOption
    #
    # Parse options
    #
    def ParseOption(self):
        (Options, Target) = self.EccOptionParser()

        if Options.Workspace:
            os.environ["WORKSPACE"] = Options.Workspace

        # Check workspace environment
        if "WORKSPACE" not in os.environ:
            EdkLogger.error("ECC", BuildToolError.ATTRIBUTE_NOT_AVAILABLE, "Environment variable not found",
                            ExtraData="WORKSPACE")
        else:
            EccGlobalData.gWorkspace = os.path.normpath(os.getenv("WORKSPACE"))
            if not os.path.exists(EccGlobalData.gWorkspace):
                EdkLogger.error("ECC", BuildToolError.FILE_NOT_FOUND, ExtraData="WORKSPACE = %s" % EccGlobalData.gWorkspace)
            os.environ["WORKSPACE"] = EccGlobalData.gWorkspace
        # Set log level
        self.SetLogLevel(Options)

        # Set other options
        if Options.ConfigFile is not None:
            self.ConfigFile = Options.ConfigFile
        if Options.OutputFile is not None:
            self.OutputFile = Options.OutputFile
        if Options.ReportFile is not None:
            self.ReportFile = Options.ReportFile
        if Options.ExceptionFile is not None:
            self.ExceptionFile = Options.ExceptionFile
        if Options.Target is not None:
            if not os.path.isdir(Options.Target):
                EdkLogger.error("ECC", BuildToolError.OPTION_VALUE_INVALID, ExtraData="Target [%s] does NOT exist" % Options.Target)
            else:
                EccGlobalData.gTarget = self.GetRealPathCase(os.path.normpath(Options.Target))
        else:
            EdkLogger.warn("Ecc", EdkLogger.ECC_ERROR, "The target source tree was not specified, using current WORKSPACE instead!")
            EccGlobalData.gTarget = os.path.normpath(os.getenv("WORKSPACE"))
        if Options.keepdatabase is not None:
            self.IsInit = False
        if Options.metadata is not None and Options.sourcecode is not None:
            EdkLogger.error("ECC", BuildToolError.OPTION_CONFLICT, ExtraData="-m and -s can't be specified at one time")
        if Options.metadata is not None:
            self.ScanSourceCode = False
        if Options.sourcecode is not None:
            self.ScanMetaData = False
        if Options.folders is not None:
            self.OnlyScan = True

    ## SetLogLevel
    #
    # Set current log level of the tool based on args
    #
    # @param Option:  The option list including log level setting
    #
    def SetLogLevel(self, Option):
        if Option.verbose is not None:
            EdkLogger.SetLevel(EdkLogger.VERBOSE)
        elif Option.quiet is not None:
            EdkLogger.SetLevel(EdkLogger.QUIET)
        elif Option.debug is not None:
            EdkLogger.SetLevel(Option.debug + 1)
        else:
            EdkLogger.SetLevel(EdkLogger.INFO)

    ## Parse command line options
    #
    # Using standard Python module optparse to parse command line option of this tool.
    #
    # @retval Opt   A optparse.Values object containing the parsed options
    # @retval Args  Target of build command
    #
    def EccOptionParser(self):
        Parser = OptionParser(description = self.Copyright, version = self.Version, prog = "Ecc.exe", usage = "%prog [options]")
        Parser.add_option("-t", "--target sourcepath", action="store", type="string", dest='Target',
            help="Check all files under the target workspace.")
        Parser.add_option("-c", "--config filename", action="store", type="string", dest="ConfigFile",
            help="Specify a configuration file. Defaultly use config.ini under ECC tool directory.")
        Parser.add_option("-o", "--outfile filename", action="store", type="string", dest="OutputFile",
            help="Specify the name of an output file, if and only if one filename was specified.")
        Parser.add_option("-r", "--reportfile filename", action="store", type="string", dest="ReportFile",
            help="Specify the name of an report file, if and only if one filename was specified.")
        Parser.add_option("-e", "--exceptionfile filename", action="store", type="string", dest="ExceptionFile",
            help="Specify the name of an exception file, if and only if one filename was specified.")
        Parser.add_option("-m", "--metadata", action="store_true", type=None, help="Only scan meta-data files information if this option is specified.")
        Parser.add_option("-s", "--sourcecode", action="store_true", type=None, help="Only scan source code files information if this option is specified.")
        Parser.add_option("-k", "--keepdatabase", action="store_true", type=None, help="The existing Ecc database will not be cleaned except report information if this option is specified.")
        Parser.add_option("-l", "--log filename", action="store", dest="LogFile", help="""If specified, the tool should emit the changes that
                                                                                          were made by the tool after printing the result message.
                                                                                          If filename, the emit to the file, otherwise emit to
                                                                                          standard output. If no modifications were made, then do not
                                                                                          create a log file, or output a log message.""")
        Parser.add_option("-q", "--quiet", action="store_true", type=None, help="Disable all messages except FATAL ERRORS.")
        Parser.add_option("-v", "--verbose", action="store_true", type=None, help="Turn on verbose output with informational messages printed, "\
                                                                                   "including library instances selected, final dependency expression, "\
                                                                                   "and warning messages, etc.")
        Parser.add_option("-d", "--debug", action="store", type="int", help="Enable debug messages at specified level.")
        Parser.add_option("-w", "--workspace", action="store", type="string", dest='Workspace', help="Specify workspace.")
        Parser.add_option("-f", "--folders", action="store_true", type=None, help="Only scanning specified folders which are recorded in config.ini file.")

        (Opt, Args)=Parser.parse_args()

        return (Opt, Args)

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    # Initialize log system
    EdkLogger.Initialize()
    EdkLogger.IsRaiseError = False

    StartTime = time.clock()
    Ecc = Ecc()
    FinishTime = time.clock()

    BuildDuration = time.strftime("%M:%S", time.gmtime(int(round(FinishTime - StartTime))))
    EdkLogger.quiet("\n%s [%s]" % (time.strftime("%H:%M:%S, %b.%d %Y", time.localtime()), BuildDuration))
