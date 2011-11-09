## @file
# This file is used to define each component of DEC file
#
# Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
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
import os
from String import *
from DataType import *
from Identification import *
from Dictionary import *
from CommonDataClass.PackageClass import *
from CommonDataClass.CommonClass import PcdClass
from BuildToolError import *
from Table.TableDec import TableDec
import Database
from Parsing import *
import GlobalData

#
# Global variable
#
Section = {TAB_UNKNOWN.upper() : MODEL_UNKNOWN,
           TAB_DEC_DEFINES.upper() : MODEL_META_DATA_HEADER,
           TAB_INCLUDES.upper() : MODEL_EFI_INCLUDE,
           TAB_LIBRARY_CLASSES.upper() : MODEL_EFI_LIBRARY_CLASS,
           TAB_COMPONENTS.upper() : MODEL_META_DATA_COMPONENT,
           TAB_GUIDS.upper() : MODEL_EFI_GUID,
           TAB_PROTOCOLS.upper() : MODEL_EFI_PROTOCOL,
           TAB_PPIS.upper() : MODEL_EFI_PPI,
           TAB_PCDS_FIXED_AT_BUILD_NULL.upper() : MODEL_PCD_FIXED_AT_BUILD,
           TAB_PCDS_PATCHABLE_IN_MODULE_NULL.upper() : MODEL_PCD_PATCHABLE_IN_MODULE,
           TAB_PCDS_FEATURE_FLAG_NULL.upper() : MODEL_PCD_FEATURE_FLAG,
           TAB_PCDS_DYNAMIC_EX_NULL.upper() : MODEL_PCD_DYNAMIC_EX,
           TAB_PCDS_DYNAMIC_NULL.upper() : MODEL_PCD_DYNAMIC,
           TAB_USER_EXTENSIONS.upper() : MODEL_META_DATA_USER_EXTENSION
           }


## DecObject
#
# This class defined basic Dec object which is used by inheriting
# 
# @param object:       Inherited from object class
#
class DecObject(object):
    def __init__(self):
        object.__init__()

## Dec
#
# This class defined the structure used in Dec object
# 
# @param DecObject:         Inherited from DecObject class
# @param Filename:          Input value for Filename of Dec file, default is None
# @param IsMergeAllArches:  Input value for IsMergeAllArches
#                           True is to merge all arches
#                           Fales is not to merge all arches
#                           default is False
# @param IsToPackage:       Input value for IsToPackage
#                           True is to transfer to PackageObject automatically
#                           False is not to transfer to PackageObject automatically
#                           default is False
# @param WorkspaceDir:      Input value for current workspace directory, default is None
#
# @var Identification:      To store value for Identification, it is a structure as Identification
# @var Defines:             To store value for Defines, it is a structure as DecDefines
# @var UserExtensions:      To store value for UserExtensions
# @var Package:             To store value for Package, it is a structure as PackageClass
# @var WorkspaceDir:        To store value for WorkspaceDir
# @var Contents:            To store value for Contents, it is a structure as DecContents
# @var KeyList:             To store value for KeyList, a list for all Keys used in Dec
#
class Dec(DecObject):
    def __init__(self, Filename=None, IsToDatabase=False, IsToPackage=False, WorkspaceDir=None, Database=None, SupArchList=DataType.ARCH_LIST):
        self.Identification = Identification()
        self.Package = PackageClass()
        self.UserExtensions = ''
        self.WorkspaceDir = WorkspaceDir
        self.SupArchList = SupArchList
        self.IsToDatabase = IsToDatabase

        self.Cur = Database.Cur
        self.TblFile = Database.TblFile
        self.TblDec = Database.TblDec
        self.FileID = -1

        self.KeyList = [
            TAB_INCLUDES, TAB_GUIDS, TAB_PROTOCOLS, TAB_PPIS, TAB_LIBRARY_CLASSES, \
            TAB_PCDS_FIXED_AT_BUILD_NULL, TAB_PCDS_PATCHABLE_IN_MODULE_NULL, TAB_PCDS_FEATURE_FLAG_NULL, \
            TAB_PCDS_DYNAMIC_NULL, TAB_PCDS_DYNAMIC_EX_NULL, TAB_DEC_DEFINES
        ]
        #
        # Upper all KEYs to ignore case sensitive when parsing
        #
        self.KeyList = map(lambda c: c.upper(), self.KeyList)

        #
        # Init RecordSet
        #
        self.RecordSet = {}
        for Key in self.KeyList:
            self.RecordSet[Section[Key]] = []

        #
        # Load Dec file if filename is not None
        #
        if Filename != None:
            self.LoadDecFile(Filename)

        #
        # Transfer to Package Object if IsToPackage is True
        #
        if IsToPackage:
            self.DecToPackage()

    ## Load Dec file
    #
    # Load the file if it exists
    #
    # @param Filename:  Input value for filename of Dec file
    #
    def LoadDecFile(self, Filename):
        #
        # Insert a record for file
        #
        Filename = NormPath(Filename)
        self.Identification.FileFullPath = Filename
        (self.Identification.FileRelativePath, self.Identification.FileName) = os.path.split(Filename)
        self.FileID = self.TblFile.InsertFile(Filename, MODEL_FILE_DEC)

        #
        # Init DecTable
        #
        #self.TblDec.Table = "Dec%s" % self.FileID
        #self.TblDec.Create()

        #
        # Init common datas
        #
        IfDefList, SectionItemList, CurrentSection, ArchList, ThirdList, IncludeFiles = \
        [], [], TAB_UNKNOWN, [], [], []
        LineNo = 0

        #
        # Parse file content
        #
        IsFindBlockComment = False
        ReservedLine = ''
        for Line in open(Filename, 'r'):
            LineNo = LineNo + 1
            #
            # Remove comment block
            #
            if Line.find(TAB_COMMENT_EDK_START) > -1:
                ReservedLine = GetSplitList(Line, TAB_COMMENT_EDK_START, 1)[0]
                IsFindBlockComment = True
            if Line.find(TAB_COMMENT_EDK_END) > -1:
                Line = ReservedLine + GetSplitList(Line, TAB_COMMENT_EDK_END, 1)[1]
                ReservedLine = ''
                IsFindBlockComment = False
            if IsFindBlockComment:
                continue

            #
            # Remove comments at tail and remove spaces again
            #
            Line = CleanString(Line)
            if Line == '':
                continue

            #
            # Find a new section tab
            # First insert previous section items
            # And then parse the content of the new section
            #
            if Line.startswith(TAB_SECTION_START) and Line.endswith(TAB_SECTION_END):
                #
                # Insert items data of previous section
                #
                Model = Section[CurrentSection.upper()]
                InsertSectionItemsIntoDatabase(self.TblDec, self.FileID, Filename, Model, CurrentSection, SectionItemList, ArchList, ThirdList, IfDefList, self.RecordSet)

                #
                # Parse the new section
                #
                SectionItemList = []
                ArchList = []
                ThirdList = []

                CurrentSection = ''
                LineList = GetSplitValueList(Line[len(TAB_SECTION_START):len(Line) - len(TAB_SECTION_END)], TAB_COMMA_SPLIT)
                for Item in LineList:
                    ItemList = GetSplitValueList(Item, TAB_SPLIT)
                    if CurrentSection == '':
                        CurrentSection = ItemList[0]
                    else:
                        if CurrentSection != ItemList[0]:
                            EdkLogger.error("Parser", PARSER_ERROR, "Different section names '%s' and '%s' are found in one section definition, this is not allowed." % (CurrentSection, ItemList[0]), File=Filename, Line=LineNo, RaiseError=EdkLogger.IsRaiseError)
                    if CurrentSection.upper() not in self.KeyList:
                        RaiseParserError(Line, CurrentSection, Filename, '', LineNo)
                    ItemList.append('')
                    ItemList.append('')
                    if len(ItemList) > 5:
                        RaiseParserError(Line, CurrentSection, Filename, '', LineNo)
                    else:
                        if ItemList[1] != '' and ItemList[1].upper() not in ARCH_LIST_FULL:
                            EdkLogger.error("Parser", PARSER_ERROR, "Invalid Arch definition '%s' found" % ItemList[1], File=Filename, Line=LineNo, RaiseError=EdkLogger.IsRaiseError)
                        ArchList.append(ItemList[1].upper())
                        ThirdList.append(ItemList[2])

                continue

            #
            # Not in any defined section
            #
            if CurrentSection == TAB_UNKNOWN:
                ErrorMsg = "%s is not in any defined section" % Line
                EdkLogger.error("Parser", PARSER_ERROR, ErrorMsg, File=Filename, Line=LineNo, RaiseError=EdkLogger.IsRaiseError)

            #
            # Add a section item
            #
            SectionItemList.append([Line, LineNo])
            # End of parse
        #End of For

        #
        # Insert items data of last section
        #
        Model = Section[CurrentSection.upper()]
        InsertSectionItemsIntoDatabase(self.TblDec, self.FileID, Filename, Model, CurrentSection, SectionItemList, ArchList, ThirdList, IfDefList, self.RecordSet)

        #
        # Replace all DEFINE macros with its actual values
        #
        ParseDefineMacro2(self.TblDec, self.RecordSet, GlobalData.gGlobalDefines)

    ## Transfer to Package Object
    # 
    # Transfer all contents of a Dec file to a standard Package Object
    #
    def DecToPackage(self):
        #
        # Init global information for the file
        #
        ContainerFile = self.Identification.FileFullPath

        #
        # Generate Package Header
        #
        self.GenPackageHeader(ContainerFile)

        #
        # Generate Includes
        #
        self.GenIncludes(ContainerFile)

        #
        # Generate Guids
        #
        self.GenGuidProtocolPpis(DataType.TAB_GUIDS, ContainerFile)

        #
        # Generate Protocols
        #
        self.GenGuidProtocolPpis(DataType.TAB_PROTOCOLS, ContainerFile)

        #
        # Generate Ppis
        #
        self.GenGuidProtocolPpis(DataType.TAB_PPIS, ContainerFile)

        #
        # Generate LibraryClasses
        #
        self.GenLibraryClasses(ContainerFile)

        #
        # Generate Pcds
        #
        self.GenPcds(ContainerFile)

    ## Get Package Header
    #
    # Gen Package Header of Dec as <Key> = <Value>
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenPackageHeader(self, ContainerFile):
        EdkLogger.debug(2, "Generate PackageHeader ...")
        #
        # Update all defines item in database
        #
        RecordSet = self.RecordSet[MODEL_META_DATA_HEADER]
        for Record in RecordSet:
            ValueList = GetSplitValueList(Record[0], TAB_EQUAL_SPLIT)
            if len(ValueList) != 2:
                RaiseParserError(Record[0], 'Defines', ContainerFile, '<Key> = <Value>', Record[2])
            ID, Value1, Value2, Arch, LineNo = Record[3], ValueList[0], ValueList[1], Record[1], Record[2]
            SqlCommand = """update %s set Value1 = '%s', Value2 = '%s'
                            where ID = %s""" % (self.TblDec.Table, ConvertToSqlString2(Value1), ConvertToSqlString2(Value2), ID)
            self.TblDec.Exec(SqlCommand)

        #
        # Get detailed information
        #
        for Arch in self.SupArchList:
            PackageHeader = PackageHeaderClass()

            PackageHeader.Name = QueryDefinesItem(self.TblDec, TAB_DEC_DEFINES_PACKAGE_NAME, Arch, self.FileID)[0]
            PackageHeader.Guid = QueryDefinesItem(self.TblDec, TAB_DEC_DEFINES_PACKAGE_GUID, Arch, self.FileID)[0]
            PackageHeader.Version = QueryDefinesItem(self.TblDec, TAB_DEC_DEFINES_PACKAGE_VERSION, Arch, self.FileID)[0]
            PackageHeader.FileName = self.Identification.FileName
            PackageHeader.FullPath = self.Identification.FileFullPath
            PackageHeader.DecSpecification = QueryDefinesItem(self.TblDec, TAB_DEC_DEFINES_DEC_SPECIFICATION, Arch, self.FileID)[0]

            self.Package.Header[Arch] = PackageHeader

    ## GenIncludes
    #
    # Gen Includes of Dec
    # 
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenIncludes(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_INCLUDES)
        Includes = {}
        #
        # Get all Includes
        #
        RecordSet = self.RecordSet[MODEL_EFI_INCLUDE]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    MergeArches(Includes, Record[0], Arch)

        for Key in Includes.keys():
            Include = IncludeClass()
            Include.FilePath = NormPath(Key)
            Include.SupArchList = Includes[Key]
            self.Package.Includes.append(Include)

    ## GenPpis
    #
    # Gen Ppis of Dec
    # <CName>=<GuidValue>
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenGuidProtocolPpis(self, Type, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % Type)
        Lists = {}
        #
        # Get all Items
        #
        RecordSet = self.RecordSet[Section[Type.upper()]]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (Name, Value) = GetGuidsProtocolsPpisOfDec(Record[0], Type, ContainerFile, Record[2])
                    MergeArches(Lists, (Name, Value), Arch)
                    if self.IsToDatabase:
                        SqlCommand = """update %s set Value1 = '%s', Value2 = '%s'
                                        where ID = %s""" % (self.TblDec.Table, ConvertToSqlString2(Name), ConvertToSqlString2(Value), Record[3])
                        self.TblDec.Exec(SqlCommand)

        ListMember = None
        if Type == TAB_GUIDS:
            ListMember = self.Package.GuidDeclarations
        elif Type == TAB_PROTOCOLS:
            ListMember = self.Package.ProtocolDeclarations
        elif Type == TAB_PPIS:
            ListMember = self.Package.PpiDeclarations

        for Key in Lists.keys():
            ListClass = GuidProtocolPpiCommonClass()
            ListClass.CName = Key[0]
            ListClass.Guid = Key[1]
            ListClass.SupArchList = Lists[Key]
            ListMember.append(ListClass)


    ## GenLibraryClasses
    #
    # Gen LibraryClasses of Dec
    # <CName>=<GuidValue>
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenLibraryClasses(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_LIBRARY_CLASSES)
        LibraryClasses = {}
        #
        # Get all Guids
        #
        RecordSet = self.RecordSet[MODEL_EFI_LIBRARY_CLASS]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    List = GetSplitValueList(Record[0], DataType.TAB_VALUE_SPLIT)
                    if len(List) != 2:
                        RaiseParserError(Record[0], 'LibraryClasses', ContainerFile, '<LibraryClassName>|<LibraryClassInstanceFilename>', Record[2])
                    else:
                        CheckFileExist(self.Identification.FileRelativePath, List[1], ContainerFile, 'LibraryClasses', Record[0])
                    MergeArches(LibraryClasses, (List[0], List[1]), Arch)
                    if self.IsToDatabase:
                        SqlCommand = """update %s set Value1 = '%s', Value2 = '%s', Value3 = '%s'
                                        where ID = %s""" % (self.TblDec.Table, ConvertToSqlString2(List[0]), ConvertToSqlString2(List[1]), SUP_MODULE_LIST_STRING, Record[3])
                        self.TblDec.Exec(SqlCommand)


        for Key in LibraryClasses.keys():
            LibraryClass = LibraryClassClass()
            LibraryClass.LibraryClass = Key[0]
            LibraryClass.RecommendedInstance = NormPath(Key[1])
            LibraryClass.SupModuleList = SUP_MODULE_LIST
            LibraryClass.SupArchList = LibraryClasses[Key]
            self.Package.LibraryClassDeclarations.append(LibraryClass)

    ## GenPcds
    #
    # Gen Pcds of Dec
    # <TokenSpcCName>.<TokenCName>|<Value>|<DatumType>|<Token>
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenPcds(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_PCDS)
        Pcds = {}
        PcdToken = {}
        #
        # Get all Guids
        #
        RecordSet1 = self.RecordSet[MODEL_PCD_FIXED_AT_BUILD]
        RecordSet2 = self.RecordSet[MODEL_PCD_PATCHABLE_IN_MODULE]
        RecordSet3 = self.RecordSet[MODEL_PCD_FEATURE_FLAG]
        RecordSet4 = self.RecordSet[MODEL_PCD_DYNAMIC_EX]
        RecordSet5 = self.RecordSet[MODEL_PCD_DYNAMIC]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet1:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (TokenGuidCName, TokenName, Value, DatumType, Token, Type) = GetPcdOfDec(Record[0], TAB_PCDS_FIXED_AT_BUILD, ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenGuidCName, TokenName, Value, DatumType, Token, Type), Arch)
                    PcdToken[Record[3]] = (TokenGuidCName, TokenName)
            for Record in RecordSet2:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (TokenGuidCName, TokenName, Value, DatumType, Token, Type) = GetPcdOfDec(Record[0], TAB_PCDS_PATCHABLE_IN_MODULE, ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenGuidCName, TokenName, Value, DatumType, Token, Type), Arch)
                    PcdToken[Record[3]] = (TokenGuidCName, TokenName)
            for Record in RecordSet3:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (TokenGuidCName, TokenName, Value, DatumType, Token, Type) = GetPcdOfDec(Record[0], TAB_PCDS_FEATURE_FLAG, ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenGuidCName, TokenName, Value, DatumType, Token, Type), Arch)
                    PcdToken[Record[3]] = (TokenGuidCName, TokenName)
            for Record in RecordSet4:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (TokenGuidCName, TokenName, Value, DatumType, Token, Type) = GetPcdOfDec(Record[0], TAB_PCDS_DYNAMIC_EX, ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenGuidCName, TokenName, Value, DatumType, Token, Type), Arch)
                    PcdToken[Record[3]] = (TokenGuidCName, TokenName)
            for Record in RecordSet5:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (TokenGuidCName, TokenName, Value, DatumType, Token, Type) = GetPcdOfDec(Record[0], TAB_PCDS_DYNAMIC, ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenGuidCName, TokenName, Value, DatumType, Token, Type), Arch)
                    PcdToken[Record[3]] = (TokenGuidCName, TokenName)
        #
        # Update to database
        #
        if self.IsToDatabase:
            for Key in PcdToken.keys():
                SqlCommand = """update %s set Value2 = '%s' where ID = %s""" % (self.TblDec.Table, ".".join((PcdToken[Key][0], PcdToken[Key][1])), Key)
                self.TblDec.Exec(SqlCommand)

        for Key in Pcds.keys():
            Pcd = PcdClass()
            Pcd.CName = Key[1]
            Pcd.Token = Key[4]
            Pcd.TokenSpaceGuidCName = Key[0]
            Pcd.DatumType = Key[3]
            Pcd.DefaultValue = Key[2]
            Pcd.ItemType = Key[5]
            Pcd.SupArchList = Pcds[Key]
            self.Package.PcdDeclarations.append(Pcd)

    ## Show detailed information of Package
    #
    # Print all members and their values of Package class
    #
    def ShowPackage(self):
        M = self.Package
        for Arch in M.Header.keys():
            print '\nArch =', Arch
            print 'Filename =', M.Header[Arch].FileName
            print 'FullPath =', M.Header[Arch].FullPath
            print 'BaseName =', M.Header[Arch].Name
            print 'Guid =', M.Header[Arch].Guid
            print 'Version =', M.Header[Arch].Version
            print 'DecSpecification =', M.Header[Arch].DecSpecification
        print '\nIncludes =', M.Includes
        for Item in M.Includes:
            print Item.FilePath, Item.SupArchList
        print '\nGuids =', M.GuidDeclarations
        for Item in M.GuidDeclarations:
            print Item.CName, Item.Guid, Item.SupArchList
        print '\nProtocols =', M.ProtocolDeclarations
        for Item in M.ProtocolDeclarations:
            print Item.CName, Item.Guid, Item.SupArchList
        print '\nPpis =', M.PpiDeclarations
        for Item in M.PpiDeclarations:
            print Item.CName, Item.Guid, Item.SupArchList
        print '\nLibraryClasses =', M.LibraryClassDeclarations
        for Item in M.LibraryClassDeclarations:
            print Item.LibraryClass, Item.RecommendedInstance, Item.SupModuleList, Item.SupArchList
        print '\nPcds =', M.PcdDeclarations
        for Item in M.PcdDeclarations:
            print 'CName=', Item.CName, 'TokenSpaceGuidCName=', Item.TokenSpaceGuidCName, 'DefaultValue=', Item.DefaultValue, 'ItemType=', Item.ItemType, 'Token=', Item.Token, 'DatumType=', Item.DatumType, Item.SupArchList

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    EdkLogger.Initialize()
    EdkLogger.SetLevel(EdkLogger.DEBUG_0)

    W = os.getenv('WORKSPACE')
    F = os.path.join(W, 'Nt32Pkg/Nt32Pkg.dec')

    Db = Database.Database('Dec.db')
    Db.InitDatabase()

    P = Dec(os.path.normpath(F), True, True, W, Db)
    P.ShowPackage()

    Db.Close()
