## @file DecPomAlignment.py
# This file contained the adapter for convert INF parser object to POM Object
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
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
DecPomAlignment
'''

##
# Import Modules
#
import os.path
from os import sep
import platform

import Logger.Log as Logger
from Logger import StringTable as ST
from Logger.ToolError import UPT_MUL_DEC_ERROR

from Library.Parsing import NormPath
from Library.DataType import ARCH_LIST
from Library.DataType import TAB_GUIDS
from Library.DataType import TAB_PROTOCOLS
from Library.DataType import TAB_PPIS
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_NAME
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_GUID
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_VERSION
from Library.DataType import TAB_DEC_DEFINES_DEC_SPECIFICATION
from Library.DataType import TAB_ARCH_COMMON
from Library.CommentParsing import ParseHeaderCommentSection
from Library.DataType import TAB_INCLUDES
from Library.CommentParsing import ParseGenericComment
from Library.DataType import TAB_LIBRARY_CLASSES
from Library.DataType import TAB_PCDS
from Library.DataType import TAB_PCDS_FIXED_AT_BUILD_NULL
from Library.DataType import TAB_PCDS_PATCHABLE_IN_MODULE_NULL
from Library.DataType import TAB_PCDS_FEATURE_FLAG_NULL
from Library.DataType import TAB_PCDS_DYNAMIC_EX_NULL
from Library.DataType import TAB_PCDS_DYNAMIC_NULL
from Library.DataType import TAB_PTR_TYPE_PCD
from Library.DataType import ITEM_UNDEFINED
from Library.CommentParsing import ParseDecPcdGenericComment
from Library.CommentParsing import ParseDecPcdTailComment
from Library.Misc import GetFiles
from Library.Misc import Sdict
from Parser.DecParser import Dec

from Object.POM.PackageObject import PackageObject
from Object.POM.CommonObject import UserExtensionObject
from Object.POM.CommonObject import IncludeObject
from Object.POM.CommonObject import GuidObject
from Object.POM.CommonObject import ProtocolObject
from Object.POM.CommonObject import PpiObject
from Object.POM.CommonObject import LibraryClassObject
from Object.POM.CommonObject import PcdObject
from Object.POM.CommonObject import TextObject


## DecPomAlignment
#
# Inherited from PackageObject
#
class DecPomAlignment(PackageObject):
    def __init__(self, Filename, WorkspaceDir = None, CheckMulDec = False):
        PackageObject.__init__(self)
        self.UserExtensions = ''
        self.WorkspaceDir = WorkspaceDir
        self.SupArchList = ARCH_LIST
        self.CheckMulDec = CheckMulDec
        self.DecParser = None
        
        #
        # Load Dec file
        #
        self.LoadDecFile(Filename)
        
        #
        # Transfer to Package Object if IsToPackage is True
        #
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
        (Path, Name) = os.path.split(Filename)
        self.SetFullPath(Filename)
        self.SetRelaPath(Path)
        self.SetFileName(Name)
        self.SetPackagePath(Path[Path.upper().find(self.WorkspaceDir.upper()) + len(self.WorkspaceDir) + 1:])
        self.SetCombinePath(Filename[Filename.upper().find(self.WorkspaceDir.upper()) + len(self.WorkspaceDir) + 1:])
        
        self.DecParser = Dec(Filename)
    
    ## Transfer to Package Object
    # 
    # Transfer all contents of a Dec file to a standard Package Object
    #
    def DecToPackage(self):
        #
        # Init global information for the file
        #
        ContainerFile = self.GetFullPath()
        
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
        self.GenGuidProtocolPpis(TAB_GUIDS, ContainerFile)

        #
        # Generate Protocols
        #
        self.GenGuidProtocolPpis(TAB_PROTOCOLS, ContainerFile)

        #
        # Generate Ppis
        #
        self.GenGuidProtocolPpis(TAB_PPIS, ContainerFile)
        
        #
        # Generate LibraryClasses
        #
        self.GenLibraryClasses(ContainerFile)
        
        #
        # Generate Pcds
        #
        self.GenPcds(ContainerFile)
        
        #
        # Generate Module File list, will be used later on to generate 
        # distribution
        #
        self.GenModuleFileList(ContainerFile)
        
        #
        # Generate user extensions
        #
        self.GenUserExtensions()
    
    ## Generate user extention
    #
    #
    def GenUserExtensions(self):
        UEObj = self.DecParser.GetUserExtensionSectionObject()
        UEList = UEObj.GetAllUserExtensions()
        for Item in UEList:
            if not Item.UserString:
                continue
            UserExtension = UserExtensionObject()
            UserId = Item.UserId
            if UserId.startswith('"') and UserId.endswith('"'):
                UserId = UserId[1:-1]
            UserExtension.SetUserID(UserId)
            Identifier = Item.IdString
            if Identifier.startswith('"') and Identifier.endswith('"'):
                Identifier = Identifier[1:-1]
            UserExtension.SetIdentifier(Identifier)
            UserExtension.SetStatement(Item.UserString)
            UserExtension.SetSupArchList(
                Item.ArchAndModuleType
            )
            self.SetUserExtensionList(
                self.GetUserExtensionList() + [UserExtension]
            )
    
    ## Generate Package Header
    #
    # Gen Package Header of Dec as <Key> = <Value>
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenPackageHeader(self, ContainerFile):
        Logger.Debug(2, "Generate PackageHeader ...")
        DefinesDict = {}
        
        #
        # Update all defines item in database
        #
        DefObj = self.DecParser.GetDefineSectionObject()
        for Item in DefObj.GetDefines():
            #
            # put items into Dict except for PackageName, Guid, Version, DEC_SPECIFICATION
            #
            SkipItemList = [TAB_DEC_DEFINES_PACKAGE_NAME, \
                TAB_DEC_DEFINES_PACKAGE_GUID, TAB_DEC_DEFINES_PACKAGE_VERSION, TAB_DEC_DEFINES_DEC_SPECIFICATION]
            if Item.Key in SkipItemList:
                continue
            DefinesDict['%s = %s' % (Item.Key, Item.Value)] = TAB_ARCH_COMMON

        self.SetBaseName(DefObj.GetPackageName())
        self.SetVersion(DefObj.GetPackageVersion())
#        self.SetName(DefObj.GetPackageName() + ' Version ' + \
#                     DefObj.GetPackageVersion())
        self.SetName(os.path.splitext(self.GetFileName())[0])
        self.SetGuid(DefObj.GetPackageGuid())
     
        if DefinesDict:
            UserExtension = UserExtensionObject()
            UserExtension.SetDefinesDict(DefinesDict)
            UserExtension.SetIdentifier('DefineModifiers')
            UserExtension.SetUserID('EDK2')  
            self.SetUserExtensionList(
                self.GetUserExtensionList() + [UserExtension]
            )

        #
        # Get All header comment section information
        #
        Abstract, Description, Copyright, License = \
            ParseHeaderCommentSection(self.DecParser.GetHeadComment(),
                                      ContainerFile)
        self.SetAbstract(Abstract)
        self.SetDescription(Description)
        self.SetCopyright(Copyright)
        self.SetLicense(License)
    
    ## GenIncludes
    #
    # Gen Includes of Dec
    # 
    # @param ContainerFile: The Dec file full path 
    #
    def GenIncludes(self, ContainerFile):
        if ContainerFile:
            pass
        Logger.Debug(2, "Generate %s ..." % TAB_INCLUDES)
        IncludesDict = Sdict()

        IncObj = self.DecParser.GetIncludeSectionObject()
        for Item in IncObj.GetAllIncludes():
            IncludePath = os.path.normpath(Item.File)
            if platform.system() != 'Windows':
                IncludePath = IncludePath.replace('\\', '/')
            if IncludePath in IncludesDict:
                if Item.GetArchList() == [TAB_ARCH_COMMON] or IncludesDict[IncludePath] == [TAB_ARCH_COMMON]:
                    IncludesDict[IncludePath] = [TAB_ARCH_COMMON]
                else:
                    IncludesDict[IncludePath] = IncludesDict[IncludePath] + Item.GetArchList()
            else:
                IncludesDict[IncludePath] = Item.GetArchList()
                        
        # 
        # get the  standardIncludeFileList(industry), packageIncludeFileList
        # (others) for PackageObject  
        # 
        PackagePath = os.path.split(self.GetFullPath())[0]
        IncludePathList = \
            [os.path.normpath(Path) + sep for Path in IncludesDict.keys()]
        IncludePathList.sort()
        
        #
        # get a non-overlap set of include path, IncludePathList should be 
        # sorted, and path should be end with path seperator '\'
        #
        NonOverLapList = []
        for Path1 in IncludePathList:
            for Path2 in NonOverLapList:
                if Path1.startswith(Path2):
                    break
            else:
                NonOverLapList.append(Path1)
        #
        # revert the list so the longest path shown first in list, also need
        # to remove the extra path seperator '\'
        # as this list is used to search the supported Arch info
        #
        for IndexN in range (0, len(IncludePathList)):
            IncludePathList[IndexN] = os.path.normpath(IncludePathList[IndexN])
        IncludePathList.sort()            
        IncludePathList.reverse()
        #
        # save the include path list for later usage
        #
        self.SetIncludePathList(IncludePathList)
        StandardIncludeFileList = []
        PackageIncludeFileList = []
        
        IncludeFileList = []
        for Path in NonOverLapList:
            FileList = GetFiles(os.path.join(PackagePath, Path), ['CVS', '.svn'], False)
            IncludeFileList += [os.path.normpath(os.path.join(Path, File)) for File in FileList]
        for Includefile in IncludeFileList:
            ExtName = os.path.splitext(Includefile)[1]
            if ExtName.upper() == '.DEC' and self.CheckMulDec:
                Logger.Error('MkPkg', 
                             UPT_MUL_DEC_ERROR,
                             ST.ERR_MUL_DEC_ERROR%(os.path.dirname(ContainerFile), 
                                                   os.path.basename(ContainerFile),
                                                   Includefile))

            FileCombinePath = os.path.dirname(Includefile)
            Include = IncludeObject()
            for Path in IncludePathList:
                if FileCombinePath.startswith(Path):
                    SupArchList = IncludesDict[Path]
                    break
            Include.SetFilePath(Includefile)
            Include.SetSupArchList(SupArchList)
            if Includefile.find('IndustryStandard') != -1:
                StandardIncludeFileList.append(Include)
            else:
                PackageIncludeFileList.append(Include)            
    
        self.SetStandardIncludeFileList(StandardIncludeFileList)

        #
        # put include path into the PackageIncludeFileList
        #
        PackagePathList = []
        IncObj = self.DecParser.GetIncludeSectionObject()        
        for Item in IncObj.GetAllIncludes():
            IncludePath = Item.File
            Include = IncludeObject()
            Include.SetFilePath(IncludePath)
            Include.SetSupArchList(Item.GetArchList())
            PackagePathList.append(Include)
        self.SetPackageIncludeFileList(PackagePathList + PackageIncludeFileList)
    
    ## GenPpis
    #
    # Gen Ppis of Dec
    # <CName>=<GuidValue>
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenGuidProtocolPpis(self, Type, ContainerFile):
        if ContainerFile:
            pass
        Logger.Debug(2, "Generate %s ..." % Type)

        Obj = None
        Factory = None
        if Type == TAB_GUIDS:
            Obj = self.DecParser.GetGuidSectionObject()
            def CreateGuidObject():
                Object = GuidObject()
                Object.SetGuidTypeList([])
                Object.SetUsage(None)
                Object.SetName(None)
                return Object
            Factory = CreateGuidObject
        elif Type == TAB_PROTOCOLS:
            Obj = self.DecParser.GetProtocolSectionObject()
            
            def CreateProtocolObject():
                return ProtocolObject()
            Factory = CreateProtocolObject
        elif Type == TAB_PPIS:
            Obj = self.DecParser.GetPpiSectionObject()

            def CreatePpiObject():
                return PpiObject()
            Factory = CreatePpiObject
        else:
            #
            # Should not be here
            #
            return
        
        DeclarationsList = []
        
        #
        # Go through each arch
        #
        for Item in Obj.GetGuidStyleAllItems():
            Name = Item.GuidCName
            Value = Item.GuidString
            HelpTxt = ParseGenericComment(Item.GetHeadComment() + \
                                          Item.GetTailComment())
            
            ListObject = Factory()
            ListObject.SetCName(Name)
            ListObject.SetGuid(Value)
            ListObject.SetSupArchList(Item.GetArchList())
            if HelpTxt:
                ListObject.SetHelpTextList([HelpTxt])
            
            DeclarationsList.append(ListObject)

        #    
        #GuidTypeList is abstracted from help
        #
        if Type == TAB_GUIDS:
            self.SetGuidList(self.GetGuidList() + DeclarationsList)
        elif Type == TAB_PROTOCOLS:
            self.SetProtocolList(self.GetProtocolList() + DeclarationsList)
        elif Type == TAB_PPIS:
            self.SetPpiList(self.GetPpiList() + DeclarationsList)
    
    ## GenLibraryClasses
    #
    # Gen LibraryClasses of Dec
    # <CName>=<GuidValue>
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenLibraryClasses(self, ContainerFile):
        if ContainerFile:
            pass
        Logger.Debug(2, "Generate %s ..." % TAB_LIBRARY_CLASSES)
        LibraryClassDeclarations = []
        
        LibObj = self.DecParser.GetLibraryClassSectionObject()
        for Item in LibObj.GetAllLibraryclasses():
            LibraryClass = LibraryClassObject()
            LibraryClass.SetLibraryClass(Item.Libraryclass)
            LibraryClass.SetSupArchList(Item.GetArchList())
            LibraryClass.SetIncludeHeader(Item.File)
            HelpTxt = ParseGenericComment(Item.GetHeadComment() + \
                                          Item.GetTailComment(), None, '@libraryclass')
            if HelpTxt:
                LibraryClass.SetHelpTextList([HelpTxt])
            LibraryClassDeclarations.append(LibraryClass)
        
        self.SetLibraryClassList(self.GetLibraryClassList() + \
                                 LibraryClassDeclarations)
    
    ## GenPcds
    #
    # Gen Pcds of Dec
    # <TokenSpcCName>.<TokenCName>|<Value>|<DatumType>|<Token>
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenPcds(self, ContainerFile):
        Logger.Debug(2, "Generate %s ..." % TAB_PCDS)
        
        PcdObj = self.DecParser.GetPcdSectionObject()
        #
        # Get all Pcds
        #        
        PcdDeclarations = []
        IterList = [
            (TAB_PCDS_FIXED_AT_BUILD_NULL,      'FixedPcd'),
            (TAB_PCDS_PATCHABLE_IN_MODULE_NULL, 'PatchPcd'), 
            (TAB_PCDS_FEATURE_FLAG_NULL,        'FeaturePcd'),
            (TAB_PCDS_DYNAMIC_EX_NULL,          'PcdEx'), 
            (TAB_PCDS_DYNAMIC_NULL,             'Pcd')]
        #
        # For each PCD type
        #
        for PcdType, Type in IterList:
            #
            # Go through all archs
            #
            # for Arch in self.SupArchList + [TAB_ARCH_COMMON]:
            #
            for Item in PcdObj.GetPcdsByType(PcdType.upper()):
                PcdDeclaration = GenPcdDeclaration(
                        ContainerFile,
                        (Item.TokenSpaceGuidCName, Item.TokenCName,
                        Item.DefaultValue, Item.DatumType, Item.TokenValue,
                        Type, Item.GetHeadComment(), Item.GetTailComment(),
                        '')
                    )
                PcdDeclaration.SetSupArchList(Item.GetArchListOfType(PcdType))
                PcdDeclarations.append(PcdDeclaration)
  
        self.SetPcdList(self.GetPcdList() + PcdDeclarations)

    
    ## GenModuleFileList
    #
    def GenModuleFileList(self, ContainerFile):        
        ModuleFileList = []
        ContainerFileName = os.path.basename(ContainerFile)
        ContainerFilePath = os.path.dirname(ContainerFile)
        for Item in GetFiles(ContainerFilePath, 
                        ['CVS', '.svn'] + self.GetIncludePathList(), False):
            ExtName = os.path.splitext(Item)[1]
            if ExtName.lower() == '.inf':
                ModuleFileList.append(Item)
            elif ExtName.upper() == '.DEC' and self.CheckMulDec:
                if Item == ContainerFileName:
                    continue
                Logger.Error('MkPkg', 
                             UPT_MUL_DEC_ERROR,
                             ST.ERR_MUL_DEC_ERROR%(ContainerFilePath, 
                                                   ContainerFileName, 
                                                   Item))
                
        self.SetModuleFileList(ModuleFileList)
    
    ## Show detailed information of Package
    #
    # Print all members and their values of Package class
    #
    def ShowPackage(self):
        print '\nName =', self.GetName()
        print '\nBaseName =', self.GetBaseName()
        print '\nVersion =', self.GetVersion() 
        print '\nGuid =', self.GetGuid()
        
        print '\nStandardIncludes = %d ' \
            % len(self.GetStandardIncludeFileList()),
        for Item in self.GetStandardIncludeFileList():
            print Item.GetFilePath(), '  ', Item.GetSupArchList()
        print '\nPackageIncludes = %d \n' \
            % len(self.GetPackageIncludeFileList()),
        for Item in self.GetPackageIncludeFileList():
            print Item.GetFilePath(), '  ', Item.GetSupArchList()
             
        print '\nGuids =', self.GetGuidList()
        for Item in self.GetGuidList():
            print Item.GetCName(), Item.GetGuid(), Item.GetSupArchList()
        print '\nProtocols =', self.GetProtocolList()
        for Item in self.GetProtocolList():
            print Item.GetCName(), Item.GetGuid(), Item.GetSupArchList()
        print '\nPpis =', self.GetPpiList()
        for Item in self.GetPpiList():
            print Item.GetCName(), Item.GetGuid(), Item.GetSupArchList()
        print '\nLibraryClasses =', self.GetLibraryClassList()
        for Item in self.GetLibraryClassList():
            print Item.GetLibraryClass(), Item.GetRecommendedInstance(), \
            Item.GetSupArchList()
        print '\nPcds =', self.GetPcdList()
        for Item in self.GetPcdList():
            print 'CName=', Item.GetCName(), 'TokenSpaceGuidCName=', \
                Item.GetTokenSpaceGuidCName(), \
                'DefaultValue=', Item.GetDefaultValue(), \
                'ValidUsage=', Item.GetValidUsage(), \
                'SupArchList', Item.GetSupArchList(), \
                'Token=', Item.GetToken(), 'DatumType=', Item.GetDatumType()
 
        for Item in self.GetMiscFileList():
            print Item.GetName()
            for FileObjectItem in Item.GetFileList():
                print FileObjectItem.GetURI()
        print '****************\n'

## GenPcdDeclaration
#
# @param ContainerFile:   File name of the DEC file
# @param PcdInfo:         Pcd information, of format (TokenGuidCName, 
#                         TokenName, Value, DatumType, Token, Type, 
#                         GenericComment, TailComment, Arch)
# 
def GenPcdDeclaration(ContainerFile, PcdInfo):
    HelpStr = ''
    TailHelpStr = ''
    TokenGuidCName, TokenName, Value, DatumType, Token, Type, \
        GenericComment, TailComment, Arch = PcdInfo
    Pcd = PcdObject()
    Pcd.SetCName(TokenName)
    Pcd.SetToken(Token)
    Pcd.SetTokenSpaceGuidCName(TokenGuidCName)
    Pcd.SetDatumType(DatumType)
    Pcd.SetDefaultValue(Value)
    Pcd.SetValidUsage(Type)
    #
    #  MaxDatumSize is required field for 'VOID*' PCD
    #
    if DatumType == TAB_PTR_TYPE_PCD:
        Pcd.SetMaxDatumSize(ITEM_UNDEFINED)

    SupArchList = [Arch]
    Pcd.SetSupArchList(SupArchList)
    
    if GenericComment:
        HelpStr, PcdErr = ParseDecPcdGenericComment(GenericComment, 
                                                    ContainerFile)
        if PcdErr:
            Pcd.SetPcdErrorsList([PcdErr])

    if TailComment:
        SupModuleList, TailHelpStr = ParseDecPcdTailComment(TailComment, 
                                                        ContainerFile)
        if SupModuleList:
            Pcd.SetSupModuleList(SupModuleList)
    
    if HelpStr and (not HelpStr.endswith('\n')) and TailHelpStr:
        HelpStr += '\n'
    HelpStr += TailHelpStr
    if HelpStr:
        HelpTxtObj = TextObject()   
        HelpTxtObj.SetString(HelpStr)
        Pcd.SetHelpTextList([HelpTxtObj])

    return Pcd
