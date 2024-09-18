## @file DecPomAlignment.py
# This file contained the adapter for convert INF parser object to POM Object
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
DecPomAlignment
'''
from __future__ import print_function

##
# Import Modules
#
import os.path
from os import sep
import platform

import re
import Logger.Log as Logger
from Logger import StringTable as ST
from Logger.ToolError import UPT_MUL_DEC_ERROR
from Logger.ToolError import FORMAT_INVALID

from Library.Parsing import NormPath
from Library.DataType import ARCH_LIST
from Library.DataType import TAB_GUIDS
from Library.DataType import TAB_PROTOCOLS
from Library.DataType import TAB_PPIS
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_NAME
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_GUID
from Library.DataType import TAB_DEC_DEFINES_PACKAGE_VERSION
from Library.DataType import TAB_DEC_DEFINES_DEC_SPECIFICATION
from Library.DataType import TAB_DEC_DEFINES_PKG_UNI_FILE
from Library.DataType import TAB_ARCH_COMMON
from Library.DataType import TAB_INCLUDES
from Library.DataType import TAB_LIBRARY_CLASSES
from Library.DataType import TAB_PCDS
from Library.DataType import TAB_PCDS_FIXED_AT_BUILD_NULL
from Library.DataType import TAB_PCDS_PATCHABLE_IN_MODULE_NULL
from Library.DataType import TAB_PCDS_FEATURE_FLAG_NULL
from Library.DataType import TAB_PCDS_DYNAMIC_EX_NULL
from Library.DataType import TAB_PCDS_DYNAMIC_NULL
from Library.DataType import TAB_PTR_TYPE_PCD
from Library.DataType import ITEM_UNDEFINED
from Library.DataType import TAB_DEC_BINARY_ABSTRACT
from Library.DataType import TAB_DEC_BINARY_DESCRIPTION
from Library.DataType import TAB_LANGUAGE_EN_US
from Library.DataType import TAB_BINARY_HEADER_IDENTIFIER
from Library.DataType import TAB_BINARY_HEADER_USERID
from Library.DataType import TAB_LANGUAGE_EN_X
from Library.DataType import TAB_LANGUAGE_EN
from Library.DataType import TAB_STR_TOKENCNAME
from Library.DataType import TAB_STR_TOKENPROMPT
from Library.DataType import TAB_STR_TOKENHELP
from Library.DataType import TAB_STR_TOKENERR
from Library.DataType import TAB_HEX_START
from Library.DataType import TAB_SPLIT
import Library.DataType as DT
from Library.CommentParsing import ParseHeaderCommentSection
from Library.CommentParsing import ParseGenericComment
from Library.CommentParsing import ParseDecPcdGenericComment
from Library.CommentParsing import ParseDecPcdTailComment
from Library.Misc import GetFiles
from Library.Misc import Sdict
from Library.Misc import GetRelativePath
from Library.Misc import PathClass
from Library.Misc import ValidateUNIFilePath
from Library.UniClassObject import UniFileClassObject
from Library.UniClassObject import ConvertSpecialUnicodes
from Library.UniClassObject import GetLanguageCode1766
from Library.ParserValidate import IsValidPath
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
from Object.POM.CommonObject import MiscFileObject
from Object.POM.CommonObject import FileObject


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
        self.UniFileClassObject = None
        self.PcdDefaultValueDict = {}

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
        self.SetPackagePath(GetRelativePath(Path, self.WorkspaceDir))
        self.SetCombinePath(GetRelativePath(Filename, self.WorkspaceDir))

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

    ## Generate user extension
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
            #
            # Generate miscellaneous files of DEC file
            #
            if UserId == 'TianoCore' and Identifier == 'ExtraFiles':
                self.GenMiscFiles(Item.UserString)
            UserExtension.SetIdentifier(Identifier)
            UserExtension.SetStatement(Item.UserString)
            UserExtension.SetSupArchList(
                Item.ArchAndModuleType
            )
            self.SetUserExtensionList(
                self.GetUserExtensionList() + [UserExtension]
            )

        # Add Private sections to UserExtension
        if self.DecParser.GetPrivateSections():
            PrivateUserExtension = UserExtensionObject()
            PrivateUserExtension.SetStatement(self.DecParser.GetPrivateSections())
            PrivateUserExtension.SetIdentifier(DT.TAB_PRIVATE)
            PrivateUserExtension.SetUserID(DT.TAB_INTEL)
            self.SetUserExtensionList(self.GetUserExtensionList() + [PrivateUserExtension])

    ## Generate miscellaneous files on DEC file
    #
    #
    def GenMiscFiles(self, Content):
        MiscFileObj = MiscFileObject()
        for Line in Content.splitlines():
            FileName = ''
            if '#' in Line:
                FileName = Line[:Line.find('#')]
            else:
                FileName = Line
            if FileName:
                if IsValidPath(FileName, self.GetRelaPath()):
                    FileObj = FileObject()
                    FileObj.SetURI(FileName)
                    MiscFileObj.SetFileList(MiscFileObj.GetFileList()+[FileObj])
                else:
                    Logger.Error("InfParser",
                                 FORMAT_INVALID,
                                 ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID%(Line),
                                 File=self.GetFileName(),
                                 ExtraData=Line)
        self.SetMiscFileList(self.GetMiscFileList()+[MiscFileObj])

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
                TAB_DEC_DEFINES_PACKAGE_GUID, TAB_DEC_DEFINES_PACKAGE_VERSION, \
                TAB_DEC_DEFINES_DEC_SPECIFICATION, TAB_DEC_DEFINES_PKG_UNI_FILE]
            if Item.Key in SkipItemList:
                continue
            DefinesDict['%s = %s' % (Item.Key, Item.Value)] = TAB_ARCH_COMMON

        self.SetBaseName(DefObj.GetPackageName())
        self.SetVersion(DefObj.GetPackageVersion())
#        self.SetName(DefObj.GetPackageName() + ' Version ' + \
#                     DefObj.GetPackageVersion())
        self.SetName(os.path.splitext(self.GetFileName())[0])
        self.SetGuid(DefObj.GetPackageGuid())
        if DefObj.GetPackageUniFile():
            ValidateUNIFilePath(DefObj.GetPackageUniFile())
            self.UniFileClassObject = \
            UniFileClassObject([PathClass(os.path.join(DefObj.GetPackagePath(), DefObj.GetPackageUniFile()))])
        else:
            self.UniFileClassObject = None

        if DefinesDict:
            UserExtension = UserExtensionObject()
            UserExtension.SetDefinesDict(DefinesDict)
            UserExtension.SetIdentifier('DefineModifiers')
            UserExtension.SetUserID('EDK2')
            self.SetUserExtensionList(
                self.GetUserExtensionList() + [UserExtension]
            )

        #
        # Get File header information
        #
        if self.UniFileClassObject:
            Lang = TAB_LANGUAGE_EN_X
        else:
            Lang = TAB_LANGUAGE_EN_US
        Abstract, Description, Copyright, License = \
            ParseHeaderCommentSection(self.DecParser.GetHeadComment(),
                                      ContainerFile)
        if Abstract:
            self.SetAbstract((Lang, Abstract))
        if Description:
            self.SetDescription((Lang, Description))
        if Copyright:
            self.SetCopyright(('', Copyright))
        if License:
            self.SetLicense(('', License))

        #
        # Get Binary header information
        #
        if self.DecParser.BinaryHeadComment:
            Abstract, Description, Copyright, License = \
                ParseHeaderCommentSection(self.DecParser.BinaryHeadComment,
                                      ContainerFile, True)

            if not Abstract  or not Description or not Copyright or not License:
                Logger.Error('MkPkg',
                             FORMAT_INVALID,
                             ST.ERR_INVALID_BINARYHEADER_FORMAT,
                             ContainerFile)
            else:
                self.SetBinaryHeaderAbstract((Lang, Abstract))
                self.SetBinaryHeaderDescription((Lang, Description))
                self.SetBinaryHeaderCopyright(('', Copyright))
                self.SetBinaryHeaderLicense(('', License))

        BinaryAbstractList = []
        BinaryDescriptionList = []

        #Get Binary header from UNI file
        # Initialize the UniStrDict dictionary, top keys are language codes
        UniStrDict = {}
        if self.UniFileClassObject:
            UniStrDict = self.UniFileClassObject.OrderedStringList
            for Lang in UniStrDict:
                for StringDefClassObject in UniStrDict[Lang]:
                    Lang = GetLanguageCode1766(Lang)
                    if StringDefClassObject.StringName == TAB_DEC_BINARY_ABSTRACT:
                        if (Lang, ConvertSpecialUnicodes(StringDefClassObject.StringValue)) \
                        not in self.GetBinaryHeaderAbstract():
                            BinaryAbstractList.append((Lang, ConvertSpecialUnicodes(StringDefClassObject.StringValue)))
                    if StringDefClassObject.StringName == TAB_DEC_BINARY_DESCRIPTION:
                        if (Lang, ConvertSpecialUnicodes(StringDefClassObject.StringValue)) \
                        not in self.GetBinaryHeaderDescription():
                            BinaryDescriptionList.append((Lang,
                                                          ConvertSpecialUnicodes(StringDefClassObject.StringValue)))
        #Combine Binary header from DEC file and UNI file
        BinaryAbstractList = self.GetBinaryHeaderAbstract() + BinaryAbstractList
        BinaryDescriptionList = self.GetBinaryHeaderDescription() + BinaryDescriptionList
        BinaryCopyrightList = self.GetBinaryHeaderCopyright()
        BinaryLicenseList = self.GetBinaryHeaderLicense()
        #Generate the UserExtensionObject for TianoCore."BinaryHeader"
        if BinaryAbstractList or BinaryDescriptionList or BinaryCopyrightList or BinaryLicenseList:
            BinaryUserExtension = UserExtensionObject()
            BinaryUserExtension.SetBinaryAbstract(BinaryAbstractList)
            BinaryUserExtension.SetBinaryDescription(BinaryDescriptionList)
            BinaryUserExtension.SetBinaryCopyright(BinaryCopyrightList)
            BinaryUserExtension.SetBinaryLicense(BinaryLicenseList)
            BinaryUserExtension.SetIdentifier(TAB_BINARY_HEADER_IDENTIFIER)
            BinaryUserExtension.SetUserID(TAB_BINARY_HEADER_USERID)
            self.SetUserExtensionList(self.GetUserExtensionList() + [BinaryUserExtension])


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
            if platform.system() != 'Windows' and platform.system() != 'Microsoft':
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
            sorted([os.path.normpath(Path) + sep for Path in IncludesDict.keys()])

        #
        # get a non-overlap set of include path, IncludePathList should be
        # sorted, and path should be end with path separator '\'
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
        # to remove the extra path separator '\'
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
                if self.UniFileClassObject:
                    HelpTxt.SetLang(TAB_LANGUAGE_EN_X)
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
                if self.UniFileClassObject:
                    HelpTxt.SetLang(TAB_LANGUAGE_EN_X)
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

        PromptStrList = []
        HelpStrList = []
        PcdErrStrList = []
        # Initialize UniStrDict dictionary, top keys are language codes
        UniStrDict = {}
        StrList = []

        Language = ''
        if self.UniFileClassObject:
            Language = TAB_LANGUAGE_EN_X
        else:
            Language = TAB_LANGUAGE_EN_US

        if self.UniFileClassObject:
            UniStrDict = self.UniFileClassObject.OrderedStringList
            for Lang in UniStrDict:
                for StringDefClassObject in UniStrDict[Lang]:
                    StrList = StringDefClassObject.StringName.split('_')
                    # StringName format is STR_<TOKENSPACECNAME>_<PCDCNAME>_PROMPT
                    if len(StrList) == 4 and StrList[0] == TAB_STR_TOKENCNAME and StrList[3] == TAB_STR_TOKENPROMPT:
                        PromptStrList.append((GetLanguageCode1766(Lang), StringDefClassObject.StringName, \
                                              StringDefClassObject.StringValue))
                    # StringName format is STR_<TOKENSPACECNAME>_<PCDCNAME>_HELP
                    if len(StrList) == 4 and StrList[0] == TAB_STR_TOKENCNAME and StrList[3] == TAB_STR_TOKENHELP:
                        HelpStrList.append((GetLanguageCode1766(Lang), StringDefClassObject.StringName, \
                                            StringDefClassObject.StringValue))
                    # StringName format is STR_<TOKENSPACECNAME>_ERR_##
                    if len(StrList) == 4 and StrList[0] == TAB_STR_TOKENCNAME and StrList[2] == TAB_STR_TOKENERR:
                        PcdErrStrList.append((GetLanguageCode1766(Lang), StringDefClassObject.StringName, \
                                              StringDefClassObject.StringValue))
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
                        Type, Item.GetHeadComment(), Item.GetTailComment(), ''),
                        Language,
                        self.DecParser.GetDefineSectionMacro()
                        )
                PcdDeclaration.SetSupArchList(Item.GetArchListOfType(PcdType))

                #
                # Get PCD error message from PCD error comment section in DEC file
                #
                for PcdErr in PcdDeclaration.GetPcdErrorsList():
                    if (PcdDeclaration.GetTokenSpaceGuidCName(), PcdErr.GetErrorNumber()) \
                        in self.DecParser.PcdErrorCommentDict:
                        Key = (PcdDeclaration.GetTokenSpaceGuidCName(), PcdErr.GetErrorNumber())
                        PcdErr.SetErrorMessageList(PcdErr.GetErrorMessageList() + \
                                                      [(Language, self.DecParser.PcdErrorCommentDict[Key])])

                for Index in range(0, len(PromptStrList)):
                    StrNameList = PromptStrList[Index][1].split('_')
                    if StrNameList[1].lower() == Item.TokenSpaceGuidCName.lower() and \
                    StrNameList[2].lower() == Item.TokenCName.lower():
                        TxtObj = TextObject()
                        TxtObj.SetLang(PromptStrList[Index][0])
                        TxtObj.SetString(PromptStrList[Index][2])
                        for Prompt in PcdDeclaration.GetPromptList():
                            if Prompt.GetLang() == TxtObj.GetLang() and \
                                Prompt.GetString() == TxtObj.GetString():
                                break
                        else:
                            PcdDeclaration.SetPromptList(PcdDeclaration.GetPromptList() + [TxtObj])

                for Index in range(0, len(HelpStrList)):
                    StrNameList = HelpStrList[Index][1].split('_')
                    if StrNameList[1].lower() == Item.TokenSpaceGuidCName.lower() and \
                    StrNameList[2].lower() == Item.TokenCName.lower():
                        TxtObj = TextObject()
                        TxtObj.SetLang(HelpStrList[Index][0])
                        TxtObj.SetString(HelpStrList[Index][2])
                        for HelpStrObj in PcdDeclaration.GetHelpTextList():
                            if HelpStrObj.GetLang() == TxtObj.GetLang() and \
                                HelpStrObj.GetString() == TxtObj.GetString():
                                break
                        else:
                            PcdDeclaration.SetHelpTextList(PcdDeclaration.GetHelpTextList() + [TxtObj])

                #
                # Get PCD error message from UNI file
                #
                for Index in range(0, len(PcdErrStrList)):
                    StrNameList = PcdErrStrList[Index][1].split('_')
                    if StrNameList[1].lower() == Item.TokenSpaceGuidCName.lower() and \
                        StrNameList[2].lower() == TAB_STR_TOKENERR.lower():
                        for PcdErr in PcdDeclaration.GetPcdErrorsList():
                            if PcdErr.GetErrorNumber().lower() == (TAB_HEX_START + StrNameList[3]).lower() and \
                                (PcdErrStrList[Index][0], PcdErrStrList[Index][2]) not in PcdErr.GetErrorMessageList():
                                PcdErr.SetErrorMessageList(PcdErr.GetErrorMessageList() + \
                                                            [(PcdErrStrList[Index][0], PcdErrStrList[Index][2])])

                #
                # Check to prevent missing error message if a Pcd has the error code.
                #
                for PcdErr in PcdDeclaration.GetPcdErrorsList():
                    if PcdErr.GetErrorNumber().strip():
                        if not PcdErr.GetErrorMessageList():
                            Logger.Error('UPT',
                                         FORMAT_INVALID,
                                         ST.ERR_DECPARSE_PCD_UNMATCHED_ERRORCODE % PcdErr.GetErrorNumber(),
                                         ContainerFile,
                                         PcdErr.GetLineNum(),
                                         PcdErr.GetFileLine())

                PcdDeclarations.append(PcdDeclaration)
        self.SetPcdList(self.GetPcdList() + PcdDeclarations)
        self.CheckPcdValue()

    ##
    # Get error message via language
    # @param ErrorMessageList: Error message tuple list the language and its message
    # @param Lang: the language of setting
    # @return: the error message described in the related UNI file
    def GetEnErrorMessage(self, ErrorMessageList):
        if self.FullPath:
            pass
        Lang = TAB_LANGUAGE_EN_US
        for (Language, Message) in ErrorMessageList:
            if Language == Lang:
                return Message
        for (Language, Message) in ErrorMessageList:
            if Language.find(TAB_LANGUAGE_EN) >= 0:
                return Message
        else:
            try:
                return ErrorMessageList[0][1]
            except IndexError:
                return ''
        return ''

    ##
    # Replace the strings for Python eval function.
    # @param ReplaceValue: The string that needs to be replaced.
    # @return: The string was replaced, then eval function is always making out it.
    def ReplaceForEval(self, ReplaceValue, IsRange=False, IsExpr=False):
        if self.FullPath:
            pass
        #
        # deal with "NOT EQ", "NOT LT", "NOT GT", "NOT LE", "NOT GE", "NOT NOT"
        #
        NOTNOT_Pattern = r'[\t\s]*NOT[\t\s]+NOT[\t\s]*'
        NOTGE_Pattern = r'[\t\s]*NOT[\t\s]+GE[\t\s]*'
        NOTLE_Pattern = r'[\t\s]*NOT[\t\s]+LE[\t\s]*'
        NOTGT_Pattern = r'[\t\s]*NOT[\t\s]+GT[\t\s]*'
        NOTLT_Pattern = r'[\t\s]*NOT[\t\s]+LT[\t\s]*'
        NOTEQ_Pattern = r'[\t\s]*NOT[\t\s]+EQ[\t\s]*'
        ReplaceValue = re.compile(NOTNOT_Pattern).sub('', ReplaceValue)
        ReplaceValue = re.compile(NOTLT_Pattern).sub('x >= ', ReplaceValue)
        ReplaceValue = re.compile(NOTGT_Pattern).sub('x <= ', ReplaceValue)
        ReplaceValue = re.compile(NOTLE_Pattern).sub('x > ', ReplaceValue)
        ReplaceValue = re.compile(NOTGE_Pattern).sub('x < ', ReplaceValue)
        ReplaceValue = re.compile(NOTEQ_Pattern).sub('x != ', ReplaceValue)

        if IsRange:
            ReplaceValue = ReplaceValue.replace('EQ', 'x ==')
            ReplaceValue = ReplaceValue.replace('LT', 'x <')
            ReplaceValue = ReplaceValue.replace('LE', 'x <=')
            ReplaceValue = ReplaceValue.replace('GT', 'x >')
            ReplaceValue = ReplaceValue.replace('GE', 'x >=')
            ReplaceValue = ReplaceValue.replace('XOR', 'x ^')
        elif IsExpr:
            ReplaceValue = ReplaceValue.replace('EQ', '==')
            ReplaceValue = ReplaceValue.replace('NE', '!=')
            ReplaceValue = ReplaceValue.replace('LT', '<')
            ReplaceValue = ReplaceValue.replace('LE', '<=')
            ReplaceValue = ReplaceValue.replace('GT', '>')
            ReplaceValue = ReplaceValue.replace('GE', '>=')
            ReplaceValue = ReplaceValue.replace('XOR', '^')

        ReplaceValue = ReplaceValue.replace('AND', 'and')
        ReplaceValue = ReplaceValue.replace('&&', ' and ')
        ReplaceValue = ReplaceValue.replace('xor', '^')
        ReplaceValue = ReplaceValue.replace('OR', 'or')
        ReplaceValue = ReplaceValue.replace('||', ' or ')
        ReplaceValue = ReplaceValue.replace('NOT', 'not')
        if ReplaceValue.find('!') >= 0 and ReplaceValue[ReplaceValue.index('!') + 1] != '=':
            ReplaceValue = ReplaceValue.replace('!', ' not ')
        if '.' in ReplaceValue:
            Pattern = r'[a-zA-Z0-9]{1,}\.[a-zA-Z0-9]{1,}'
            MatchedList = re.findall(Pattern, ReplaceValue)
            for MatchedItem in MatchedList:
                if MatchedItem not in self.PcdDefaultValueDict:
                    Logger.Error("Dec File Parser", FORMAT_INVALID, Message=ST.ERR_DECPARSE_PCD_NODEFINED % MatchedItem,
                                     File=self.FullPath)

                ReplaceValue = ReplaceValue.replace(MatchedItem, self.PcdDefaultValueDict[MatchedItem])

        return ReplaceValue

    ##
    # Check pcd's default value according to the pcd's description
    #
    def CheckPcdValue(self):
        for Pcd in self.GetPcdList():
            self.PcdDefaultValueDict[TAB_SPLIT.join((Pcd.GetTokenSpaceGuidCName(), Pcd.GetCName())).strip()] = \
            Pcd.GetDefaultValue()

        for Pcd in self.GetPcdList():
            ValidationExpressions = []
            PcdGuidName = TAB_SPLIT.join((Pcd.GetTokenSpaceGuidCName(), Pcd.GetCName()))
            Valids = Pcd.GetPcdErrorsList()
            for Valid in Valids:
                Expression = Valid.GetExpression()
                if Expression:
                    #
                    # Delete the 'L' prefix of a quoted string, this operation is for eval()
                    #
                    QUOTED_PATTERN = r'[\t\s]*L?"[^"]*"'
                    QuotedMatchedObj = re.search(QUOTED_PATTERN, Expression)
                    if QuotedMatchedObj:
                        MatchedStr = QuotedMatchedObj.group().strip()
                        if MatchedStr.startswith('L'):
                            Expression = Expression.replace(MatchedStr, MatchedStr[1:].strip())

                    Expression = self.ReplaceForEval(Expression, IsExpr=True)
                    Expression = Expression.replace(PcdGuidName, 'x')
                    Message = self.GetEnErrorMessage(Valid.GetErrorMessageList())
                    ValidationExpressions.append((Expression, Message))

                ValidList = Valid.GetValidValue()
                if ValidList:
                    ValidValue = 'x in %s' % [eval(v) for v in ValidList.split(' ') if v]
                    Message = self.GetEnErrorMessage(Valid.GetErrorMessageList())
                    ValidationExpressions.append((ValidValue, Message))

                ValidValueRange = Valid.GetValidValueRange()
                if ValidValueRange:
                    ValidValueRange = self.ReplaceForEval(ValidValueRange, IsRange=True)
                    if ValidValueRange.find('-') >= 0:
                        ValidValueRange = ValidValueRange.replace('-', '<= x <=')
                    elif not ValidValueRange.startswith('x ') and not ValidValueRange.startswith('not ') \
                        and not ValidValueRange.startswith('not(') and not ValidValueRange.startswith('('):
                        ValidValueRange = 'x %s' % ValidValueRange
                    Message = self.GetEnErrorMessage(Valid.GetErrorMessageList())
                    ValidationExpressions.append((ValidValueRange, Message))

            DefaultValue = self.PcdDefaultValueDict[PcdGuidName.strip()]
            #
            # Delete the 'L' prefix of a quoted string, this operation is for eval()
            #
            QUOTED_PATTERN = r'[\t\s]*L?"[^"]*"'
            QuotedMatchedObj = re.search(QUOTED_PATTERN, DefaultValue)
            if QuotedMatchedObj:
                MatchedStr = QuotedMatchedObj.group().strip()
                if MatchedStr.startswith('L'):
                    DefaultValue = DefaultValue.replace(MatchedStr, MatchedStr[1:].strip())

            try:
                DefaultValue = eval(DefaultValue.replace('TRUE', 'True').replace('true', 'True')
                                        .replace('FALSE', 'False').replace('false', 'False'))
            except BaseException:
                pass

            for (Expression, Msg) in ValidationExpressions:
                try:
                    if not eval(Expression, {'x':DefaultValue}):
                        Logger.Error("Dec File Parser", FORMAT_INVALID, ExtraData='%s, value = %s' %\
                                     (PcdGuidName, DefaultValue), Message=Msg, File=self.FullPath)
                except TypeError:
                    Logger.Error("Dec File Parser", FORMAT_INVALID, ExtraData=PcdGuidName, \
                                    Message=Msg, File=self.FullPath)

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
        print('\nName =', self.GetName())
        print('\nBaseName =', self.GetBaseName())
        print('\nVersion =', self.GetVersion())
        print('\nGuid =', self.GetGuid())

        print('\nStandardIncludes = %d ' \
            % len(self.GetStandardIncludeFileList()), end=' ')
        for Item in self.GetStandardIncludeFileList():
            print(Item.GetFilePath(), '  ', Item.GetSupArchList())
        print('\nPackageIncludes = %d \n' \
            % len(self.GetPackageIncludeFileList()), end=' ')
        for Item in self.GetPackageIncludeFileList():
            print(Item.GetFilePath(), '  ', Item.GetSupArchList())

        print('\nGuids =', self.GetGuidList())
        for Item in self.GetGuidList():
            print(Item.GetCName(), Item.GetGuid(), Item.GetSupArchList())
        print('\nProtocols =', self.GetProtocolList())
        for Item in self.GetProtocolList():
            print(Item.GetCName(), Item.GetGuid(), Item.GetSupArchList())
        print('\nPpis =', self.GetPpiList())
        for Item in self.GetPpiList():
            print(Item.GetCName(), Item.GetGuid(), Item.GetSupArchList())
        print('\nLibraryClasses =', self.GetLibraryClassList())
        for Item in self.GetLibraryClassList():
            print(Item.GetLibraryClass(), Item.GetRecommendedInstance(), \
            Item.GetSupArchList())
        print('\nPcds =', self.GetPcdList())
        for Item in self.GetPcdList():
            print('CName=', Item.GetCName(), 'TokenSpaceGuidCName=', \
                Item.GetTokenSpaceGuidCName(), \
                'DefaultValue=', Item.GetDefaultValue(), \
                'ValidUsage=', Item.GetValidUsage(), \
                'SupArchList', Item.GetSupArchList(), \
                'Token=', Item.GetToken(), 'DatumType=', Item.GetDatumType())

        for Item in self.GetMiscFileList():
            print(Item.GetName())
            for FileObjectItem in Item.GetFileList():
                print(FileObjectItem.GetURI())
        print('****************\n')

## GenPcdDeclaration
#
# @param ContainerFile:   File name of the DEC file
# @param PcdInfo:         Pcd information, of format (TokenGuidCName,
#                         TokenName, Value, DatumType, Token, Type,
#                         GenericComment, TailComment, Arch)
# @param Language: The language of HelpText, Prompt
#
def GenPcdDeclaration(ContainerFile, PcdInfo, Language, MacroReplaceDict):
    HelpStr = ''
    PromptStr = ''
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
        HelpStr, PcdErrList, PromptStr = ParseDecPcdGenericComment(GenericComment,
                                                                   ContainerFile,
                                                                   TokenGuidCName,
                                                                   TokenName,
                                                                   MacroReplaceDict)
        if PcdErrList:
            Pcd.SetPcdErrorsList(PcdErrList)

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
        HelpTxtObj.SetLang(Language)
        HelpTxtObj.SetString(HelpStr)
        Pcd.SetHelpTextList([HelpTxtObj])
    if PromptStr:
        TxtObj = TextObject()
        TxtObj.SetLang(Language)
        TxtObj.SetString(PromptStr)
        Pcd.SetPromptList([TxtObj])

    return Pcd
