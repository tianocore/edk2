## @file
# This file is used to be the main entrance of EOT tool
#
# Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
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
import Common.LongFilePathOs as os, time, glob
import Common.EdkLogger as EdkLogger
import EotGlobalData
from optparse import OptionParser
from Common.String import NormPath
from Common import BuildToolError
from Common.Misc import GuidStructureStringToGuidString
from InfParserLite import *
import c
import Database
from FvImage import *
from array import array
from Report import Report
from Common.Misc import ParseConsoleLog
from Common.BuildVersion import gBUILD_VERSION
from Parser import ConvertGuid
from Common.LongFilePathSupport import OpenLongFilePath as open

## Class Eot
#
# This class is used to define Eot main entrance
#
# @param object:          Inherited from object class
#
class Eot(object):
    ## The constructor
    #
    #   @param  self:      The object pointer
    #
    def __init__(self, CommandLineOption=True, IsInit=True, SourceFileList=None, \
                 IncludeDirList=None, DecFileList=None, GuidList=None, LogFile=None,
                 FvFileList="", MapFileList="", Report='Report.html', Dispatch=None):
        # Version and Copyright
        self.VersionNumber = ("0.02" + " " + gBUILD_VERSION)
        self.Version = "%prog Version " + self.VersionNumber
        self.Copyright = "Copyright (c) 2008 - 2010, Intel Corporation  All rights reserved."
        self.Report = Report

        self.IsInit = IsInit
        self.SourceFileList = SourceFileList
        self.IncludeDirList = IncludeDirList
        self.DecFileList = DecFileList
        self.GuidList = GuidList
        self.LogFile = LogFile
        self.FvFileList = FvFileList
        self.MapFileList = MapFileList
        self.Dispatch = Dispatch
        
        # Check workspace environment
        if "EFI_SOURCE" not in os.environ:
            if "EDK_SOURCE" not in os.environ:
                pass
            else:
                EotGlobalData.gEDK_SOURCE = os.path.normpath(os.getenv("EDK_SOURCE"))
        else:
            EotGlobalData.gEFI_SOURCE = os.path.normpath(os.getenv("EFI_SOURCE"))
            EotGlobalData.gEDK_SOURCE = os.path.join(EotGlobalData.gEFI_SOURCE, 'Edk')

        if "WORKSPACE" not in os.environ:
            EdkLogger.error("EOT", BuildToolError.ATTRIBUTE_NOT_AVAILABLE, "Environment variable not found",
                            ExtraData="WORKSPACE")
        else:
            EotGlobalData.gWORKSPACE = os.path.normpath(os.getenv("WORKSPACE"))

        EotGlobalData.gMACRO['WORKSPACE'] = EotGlobalData.gWORKSPACE
        EotGlobalData.gMACRO['EFI_SOURCE'] = EotGlobalData.gEFI_SOURCE
        EotGlobalData.gMACRO['EDK_SOURCE'] = EotGlobalData.gEDK_SOURCE

        # Parse the options and args
        if CommandLineOption:
            self.ParseOption()

        if self.FvFileList:
            for FvFile in GetSplitValueList(self.FvFileList, ' '):
                FvFile = os.path.normpath(FvFile)
                if not os.path.isfile(FvFile):
                    EdkLogger.error("Eot", EdkLogger.EOT_ERROR, "Can not find file %s " % FvFile)
                EotGlobalData.gFV_FILE.append(FvFile)
        else:
            EdkLogger.error("Eot", EdkLogger.EOT_ERROR, "The fv file list of target platform was not specified")

        if self.MapFileList:
            for MapFile in GetSplitValueList(self.MapFileList, ' '):
                MapFile = os.path.normpath(MapFile)
                if not os.path.isfile(MapFile):
                    EdkLogger.error("Eot", EdkLogger.EOT_ERROR, "Can not find file %s " % MapFile)
                EotGlobalData.gMAP_FILE.append(MapFile)
                
        # Generate source file list
        self.GenerateSourceFileList(self.SourceFileList, self.IncludeDirList)

        # Generate guid list of dec file list
        self.ParseDecFile(self.DecFileList)
        
        # Generate guid list from GUID list file
        self.ParseGuidList(self.GuidList)

        # Init Eot database
        EotGlobalData.gDb = Database.Database(Database.DATABASE_PATH)
        EotGlobalData.gDb.InitDatabase(self.IsInit)

        # Build ECC database
        self.BuildDatabase()

        # Parse Ppi/Protocol
        self.ParseExecutionOrder()

        # Merge Identifier tables
        self.GenerateQueryTable()

        # Generate report database
        self.GenerateReportDatabase()

        # Load Fv Info
        self.LoadFvInfo()

        # Load Map Info
        self.LoadMapInfo()

        # Generate Report
        self.GenerateReport()

        # Convert log file
        self.ConvertLogFile(self.LogFile)

        # DONE
        EdkLogger.quiet("EOT FINISHED!")

        # Close Database
        EotGlobalData.gDb.Close()

    ## ParseDecFile() method
    #
    #  parse DEC file and get all GUID names with GUID values as {GuidName : GuidValue}
    #  The Dict is stored in EotGlobalData.gGuidDict
    #
    #  @param self: The object pointer
    #  @param DecFileList: A list of all DEC files
    #
    def ParseDecFile(self, DecFileList):
        if DecFileList:
            path = os.path.normpath(DecFileList)
            lfr = open(path, 'rb')
            for line in lfr:
                path = os.path.normpath(os.path.join(EotGlobalData.gWORKSPACE, line.strip()))
                if os.path.exists(path):
                    dfr = open(path, 'rb')
                    for line in dfr:
                        line = CleanString(line)
                        list = line.split('=')
                        if len(list) == 2:
                            EotGlobalData.gGuidDict[list[0].strip()] = GuidStructureStringToGuidString(list[1].strip())

    
    ## ParseGuidList() method
    #
    #  Parse Guid list and get all GUID names with GUID values as {GuidName : GuidValue}
    #  The Dict is stored in EotGlobalData.gGuidDict
    #
    #  @param self: The object pointer
    #  @param GuidList: A list of all GUID and its value
    #
    def ParseGuidList(self, GuidList):
        Path = os.path.join(EotGlobalData.gWORKSPACE, GuidList)
        if os.path.isfile(Path):
            for Line in open(Path):
                (GuidName, GuidValue) = Line.split()
                EotGlobalData.gGuidDict[GuidName] = GuidValue
            
    ## ConvertLogFile() method
    #
    #  Parse a real running log file to get real dispatch order
    #  The result is saved to old file name + '.new'
    #
    #  @param self: The object pointer
    #  @param LogFile: A real running log file name
    #
    def ConvertLogFile(self, LogFile):
        newline = []
        lfr = None
        lfw = None
        if LogFile:
            lfr = open(LogFile, 'rb')
            lfw = open(LogFile + '.new', 'wb')
            for line in lfr:
                line = line.strip()
                line = line.replace('.efi', '')
                index = line.find("Loading PEIM at ")
                if index > -1:
                    newline.append(line[index + 55 : ])
                    continue
                index = line.find("Loading driver at ")
                if index > -1:
                    newline.append(line[index + 57 : ])
                    continue

        for line in newline:
            lfw.write(line + '\r\n')

        if lfr:
            lfr.close()
        if lfw:
            lfw.close()

    ## GenerateSourceFileList() method
    #
    #  Generate a list of all source files
    #  1. Search the file list one by one
    #  2. Store inf file name with source file names under it like
    #  { INF file name: [source file1, source file2, ...]}
    #  3. Search the include list to find all .h files
    #  4. Store source file list to EotGlobalData.gSOURCE_FILES
    #  5. Store INF file list to EotGlobalData.gINF_FILES
    #
    #  @param self: The object pointer
    #  @param SourceFileList: A list of all source files
    #  @param IncludeFileList: A list of all include files
    #
    def GenerateSourceFileList(self, SourceFileList, IncludeFileList):
        EdkLogger.quiet("Generating source files list ... ")
        mSourceFileList = []
        mInfFileList = []
        mDecFileList = []
        mFileList = {}
        mCurrentInfFile = ''
        mCurrentSourceFileList = []

        if SourceFileList:
            sfl = open(SourceFileList, 'rb')
            for line in sfl:
                line = os.path.normpath(os.path.join(EotGlobalData.gWORKSPACE, line.strip()))
                if line[-2:].upper() == '.C' or  line[-2:].upper() == '.H':
                    if line not in mCurrentSourceFileList:
                        mCurrentSourceFileList.append(line)
                        mSourceFileList.append(line)
                        EotGlobalData.gOP_SOURCE_FILES.write('%s\n' % line)
                if line[-4:].upper() == '.INF':
                    if mCurrentInfFile != '':
                        mFileList[mCurrentInfFile] = mCurrentSourceFileList
                        mCurrentSourceFileList = []
                    mCurrentInfFile = os.path.normpath(os.path.join(EotGlobalData.gWORKSPACE, line))
                    EotGlobalData.gOP_INF.write('%s\n' % mCurrentInfFile)
            if mCurrentInfFile not in mFileList:
                mFileList[mCurrentInfFile] = mCurrentSourceFileList

        # Get all include files from packages
        if IncludeFileList:
            ifl = open(IncludeFileList, 'rb')
            for line in ifl:
                if not line.strip():
                    continue
                newline = os.path.normpath(os.path.join(EotGlobalData.gWORKSPACE, line.strip()))
                for Root, Dirs, Files in os.walk(str(newline)):
                    for File in Files:
                        FullPath = os.path.normpath(os.path.join(Root, File))
                        if FullPath not in mSourceFileList and File[-2:].upper() == '.H':
                            mSourceFileList.append(FullPath)
                            EotGlobalData.gOP_SOURCE_FILES.write('%s\n' % FullPath)
                        if FullPath not in mDecFileList and File.upper().find('.DEC') > -1:
                            mDecFileList.append(FullPath)

        EotGlobalData.gSOURCE_FILES = mSourceFileList
        EotGlobalData.gOP_SOURCE_FILES.close()

        EotGlobalData.gINF_FILES = mFileList
        EotGlobalData.gOP_INF.close()

        EotGlobalData.gDEC_FILES = mDecFileList


    ## GenerateReport() method
    #
    #  Generate final HTML report
    #
    #  @param self: The object pointer
    #
    def GenerateReport(self):
        EdkLogger.quiet("Generating report file ... ")
        Rep = Report(self.Report, EotGlobalData.gFV, self.Dispatch)
        Rep.GenerateReport()

    ## LoadMapInfo() method
    #
    #  Load map files and parse them
    #
    #  @param self: The object pointer
    #
    def LoadMapInfo(self):
        if EotGlobalData.gMAP_FILE != []:
            EdkLogger.quiet("Parsing Map file ... ")
            EotGlobalData.gMap = ParseMapFile(EotGlobalData.gMAP_FILE)

    ## LoadFvInfo() method
    #
    #  Load FV binary files and parse them
    #
    #  @param self: The object pointer
    #
    def LoadFvInfo(self):
        EdkLogger.quiet("Parsing FV file ... ")
        EotGlobalData.gFV = MultipleFv(EotGlobalData.gFV_FILE)
        EotGlobalData.gFV.Dispatch(EotGlobalData.gDb)

        for Protocol in EotGlobalData.gProtocolList:
            EotGlobalData.gOP_UN_MATCHED_IN_LIBRARY_CALLING.write('%s\n' %Protocol)

    ## GenerateReportDatabase() method
    #
    #  Generate data for the information needed by report
    #  1. Update name, macro and value of all found PPI/PROTOCOL GUID
    #  2. Install hard coded PPI/PROTOCOL
    #
    #  @param self: The object pointer
    #
    def GenerateReportDatabase(self):
        EdkLogger.quiet("Generating the cross-reference table of GUID for Ppi/Protocol ... ")

        # Update Protocol/Ppi Guid
        SqlCommand = """select DISTINCT GuidName from Report"""
        RecordSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
        for Record in RecordSet:
            GuidName = Record[0]
            GuidMacro = ''
            GuidMacro2 = ''
            GuidValue = ''

            # Find value for hardcode guid macro
            if GuidName in EotGlobalData.gGuidMacroDict:
                GuidMacro = EotGlobalData.gGuidMacroDict[GuidName][0]
                GuidValue = EotGlobalData.gGuidMacroDict[GuidName][1]
                SqlCommand = """update Report set GuidMacro = '%s', GuidValue = '%s' where GuidName = '%s'""" %(GuidMacro, GuidValue, GuidName)
                EotGlobalData.gDb.TblReport.Exec(SqlCommand)
                continue

            # Find guid value defined in Dec file
            if GuidName in EotGlobalData.gGuidDict:
                GuidValue = EotGlobalData.gGuidDict[GuidName]
                SqlCommand = """update Report set GuidMacro = '%s', GuidValue = '%s' where GuidName = '%s'""" %(GuidMacro, GuidValue, GuidName)
                EotGlobalData.gDb.TblReport.Exec(SqlCommand)
                continue

            # Search defined Macros for guid name
            SqlCommand ="""select DISTINCT Value, Modifier from Query where Name like '%s'""" % GuidName
            GuidMacroSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
            # Ignore NULL result
            if not GuidMacroSet:
                continue
            GuidMacro = GuidMacroSet[0][0].strip()
            if not GuidMacro:
                continue
            # Find Guid value of Guid Macro
            SqlCommand ="""select DISTINCT Value from Query2 where Value like '%%%s%%' and Model = %s""" % (GuidMacro, MODEL_IDENTIFIER_MACRO_DEFINE)
            GuidValueSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
            if GuidValueSet != []:
                GuidValue = GuidValueSet[0][0]
                GuidValue = GuidValue[GuidValue.find(GuidMacro) + len(GuidMacro) :]
                GuidValue = GuidValue.lower().replace('\\', '').replace('\r', '').replace('\n', '').replace('l', '').strip()
                GuidValue = GuidStructureStringToGuidString(GuidValue)
                SqlCommand = """update Report set GuidMacro = '%s', GuidValue = '%s' where GuidName = '%s'""" %(GuidMacro, GuidValue, GuidName)
                EotGlobalData.gDb.TblReport.Exec(SqlCommand)
                continue

        # Update Hard Coded Ppi/Protocol
        SqlCommand = """select DISTINCT GuidValue, ItemType from Report where ModuleID = -2 and ItemMode = 'Produced'"""
        RecordSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
        for Record in RecordSet:
            if Record[1] == 'Ppi':
                EotGlobalData.gPpiList[Record[0].lower()] = -2
            if Record[1] == 'Protocol':
                EotGlobalData.gProtocolList[Record[0].lower()] = -2

    ## GenerateQueryTable() method
    #
    #  Generate two tables improve query performance
    #
    #  @param self: The object pointer
    #
    def GenerateQueryTable(self):
        EdkLogger.quiet("Generating temp query table for analysis ... ")
        for Identifier in EotGlobalData.gIdentifierTableList:
            SqlCommand = """insert into Query (Name, Modifier, Value, Model)
                            select Name, Modifier, Value, Model from %s where (Model = %s or Model = %s)""" \
                            % (Identifier[0], MODEL_IDENTIFIER_VARIABLE, MODEL_IDENTIFIER_ASSIGNMENT_EXPRESSION)
            EotGlobalData.gDb.TblReport.Exec(SqlCommand)
            SqlCommand = """insert into Query2 (Name, Modifier, Value, Model)
                            select Name, Modifier, Value, Model from %s where Model = %s""" \
                            % (Identifier[0], MODEL_IDENTIFIER_MACRO_DEFINE)
            EotGlobalData.gDb.TblReport.Exec(SqlCommand)

    ## ParseExecutionOrder() method
    #
    #  Get final execution order
    #  1. Search all PPI
    #  2. Search all PROTOCOL
    #
    #  @param self: The object pointer
    #
    def ParseExecutionOrder(self):
        EdkLogger.quiet("Searching Ppi/Protocol ... ")
        for Identifier in EotGlobalData.gIdentifierTableList:
            ModuleID, ModuleName, ModuleGuid, SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue, BelongsToFunction, Enabled = \
            -1, '', '', -1, '', '', '', '', '', '', '', '', 0

            SourceFileID = Identifier[0].replace('Identifier', '')
            SourceFileFullPath = Identifier[1]
            Identifier = Identifier[0]

            # Find Ppis
            ItemMode = 'Produced'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.InstallPpi', '->InstallPpi', 'PeiInstallPpi', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchPpi(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode)

            ItemMode = 'Produced'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.ReInstallPpi', '->ReInstallPpi', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchPpi(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 2)

            SearchPpiCallFunction(Identifier, SourceFileID, SourceFileFullPath, ItemMode)

            ItemMode = 'Consumed'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.LocatePpi', '->LocatePpi', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchPpi(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode)

            SearchFunctionCalling(Identifier, SourceFileID, SourceFileFullPath, 'Ppi', ItemMode)

            ItemMode = 'Callback'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.NotifyPpi', '->NotifyPpi', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchPpi(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode)

            # Find Procotols
            ItemMode = 'Produced'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%' or Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.InstallProtocolInterface', '.ReInstallProtocolInterface', '->InstallProtocolInterface', '->ReInstallProtocolInterface', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchProtocols(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 1)

            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.InstallMultipleProtocolInterfaces', '->InstallMultipleProtocolInterfaces', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchProtocols(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 2)

            SearchFunctionCalling(Identifier, SourceFileID, SourceFileFullPath, 'Protocol', ItemMode)

            ItemMode = 'Consumed'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.LocateProtocol', '->LocateProtocol', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchProtocols(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 0)

            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.HandleProtocol', '->HandleProtocol', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchProtocols(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 1)

            SearchFunctionCalling(Identifier, SourceFileID, SourceFileFullPath, 'Protocol', ItemMode)

            ItemMode = 'Callback'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.RegisterProtocolNotify', '->RegisterProtocolNotify', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchProtocols(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 0)

            SearchFunctionCalling(Identifier, SourceFileID, SourceFileFullPath, 'Protocol', ItemMode)

        # Hard Code
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gEfiSecPlatformInformationPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gEfiNtLoadAsDllPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gNtPeiLoadFileGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gPeiNtAutoScanPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gNtFwhPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gPeiNtThunkPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gPeiPlatformTypePpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gPeiFrequencySelectionCpuPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gPeiCachePpiGuid', '', '', '', 0)

        EotGlobalData.gDb.Conn.commit()


    ## BuildDatabase() methoc
    #
    #  Build the database for target
    #
    #  @param self: The object pointer
    #
    def BuildDatabase(self):
        # Clean report table
        EotGlobalData.gDb.TblReport.Drop()
        EotGlobalData.gDb.TblReport.Create()

        # Build database
        if self.IsInit:
            self.BuildMetaDataFileDatabase(EotGlobalData.gINF_FILES)
            EdkLogger.quiet("Building database for source code ...")
            c.CreateCCodeDB(EotGlobalData.gSOURCE_FILES)
            EdkLogger.quiet("Building database for source code done!")

        EotGlobalData.gIdentifierTableList = GetTableList((MODEL_FILE_C, MODEL_FILE_H), 'Identifier', EotGlobalData.gDb)

    ## BuildMetaDataFileDatabase() method
    #
    #  Build the database for meta data files
    #
    #  @param self: The object pointer
    #  @param Inf_Files: A list for all INF files
    #
    def BuildMetaDataFileDatabase(self, Inf_Files):
        EdkLogger.quiet("Building database for meta data files ...")
        for InfFile in Inf_Files:
            EdkLogger.quiet("Parsing %s ..."  % str(InfFile))
            EdkInfParser(InfFile, EotGlobalData.gDb, Inf_Files[InfFile], '')

        EotGlobalData.gDb.Conn.commit()
        EdkLogger.quiet("Building database for meta data files done!")

    ## ParseOption() method
    #
    #  Parse command line options
    #
    #  @param self: The object pointer
    #
    def ParseOption(self):
        (Options, Target) = self.EotOptionParser()

        # Set log level
        self.SetLogLevel(Options)

        if Options.FvFileList:
            self.FvFileList = Options.FvFileList
 
        if Options.MapFileList:
            self.MapFileList = Options.FvMapFileList

        if Options.SourceFileList:
            self.SourceFileList = Options.SourceFileList

        if Options.IncludeDirList:
            self.IncludeDirList = Options.IncludeDirList

        if Options.DecFileList:
            self.DecFileList = Options.DecFileList
        
        if Options.GuidList:
            self.GuidList = Options.GuidList

        if Options.LogFile:
            self.LogFile = Options.LogFile

        if Options.keepdatabase:
            self.IsInit = False

    ## SetLogLevel() method
    #
    #  Set current log level of the tool based on args
    #
    #  @param self: The object pointer
    #  @param Option: The option list including log level setting
    #
    def SetLogLevel(self, Option):
        if Option.verbose != None:
            EdkLogger.SetLevel(EdkLogger.VERBOSE)
        elif Option.quiet != None:
            EdkLogger.SetLevel(EdkLogger.QUIET)
        elif Option.debug != None:
            EdkLogger.SetLevel(Option.debug + 1)
        else:
            EdkLogger.SetLevel(EdkLogger.INFO)

    ## EotOptionParser() method
    #
    #  Using standard Python module optparse to parse command line option of this tool.
    #
    #  @param self: The object pointer
    #
    #  @retval Opt   A optparse.Values object containing the parsed options
    #  @retval Args  Target of build command
    #
    def EotOptionParser(self):
        Parser = OptionParser(description = self.Copyright, version = self.Version, prog = "Eot.exe", usage = "%prog [options]")
        Parser.add_option("-m", "--makefile filename", action="store", type="string", dest='MakeFile',
            help="Specify a makefile for the platform.")
        Parser.add_option("-c", "--dsc filename", action="store", type="string", dest="DscFile",
            help="Specify a dsc file for the platform.")
        Parser.add_option("-f", "--fv filename", action="store", type="string", dest="FvFileList",
            help="Specify fv file list, quoted by \"\".")
        Parser.add_option("-a", "--map filename", action="store", type="string", dest="MapFileList",
            help="Specify map file list, quoted by \"\".")
        Parser.add_option("-s", "--source files", action="store", type="string", dest="SourceFileList",
            help="Specify source file list by a file")
        Parser.add_option("-i", "--include dirs", action="store", type="string", dest="IncludeDirList",
            help="Specify include dir list by a file")
        Parser.add_option("-e", "--dec files", action="store", type="string", dest="DecFileList",
            help="Specify dec file list by a file")
        Parser.add_option("-g", "--guid list", action="store", type="string", dest="GuidList",
            help="Specify guid file list by a file")
        Parser.add_option("-l", "--log filename", action="store", type="string", dest="LogFile",
            help="Specify real execution log file")

        Parser.add_option("-k", "--keepdatabase", action="store_true", type=None, help="The existing Eot database will not be cleaned except report information if this option is specified.")

        Parser.add_option("-q", "--quiet", action="store_true", type=None, help="Disable all messages except FATAL ERRORS.")
        Parser.add_option("-v", "--verbose", action="store_true", type=None, help="Turn on verbose output with informational messages printed, "\
                                                                                   "including library instances selected, final dependency expression, "\
                                                                                   "and warning messages, etc.")
        Parser.add_option("-d", "--debug", action="store", type="int", help="Enable debug messages at specified level.")

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
    EdkLogger.quiet(time.strftime("%H:%M:%S, %b.%d %Y ", time.localtime()) + "[00:00]" + "\n")

    StartTime = time.clock()
    Eot = Eot()
    FinishTime = time.clock()

    BuildDuration = time.strftime("%M:%S", time.gmtime(int(round(FinishTime - StartTime))))
    EdkLogger.quiet("\n%s [%s]" % (time.strftime("%H:%M:%S, %b.%d %Y", time.localtime()), BuildDuration))
