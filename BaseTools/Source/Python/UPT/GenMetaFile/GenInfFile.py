## @file GenInfFile.py
#
# This file contained the logical of transfer package object to INF files.
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
GenInf
'''
from os import getenv
from Library.String import GetSplitValueList
from Library.Parsing import GenSection
from Library.Parsing import GetWorkspacePackage
from Library.Parsing import ConvertArchForInstall                     
from Library.Misc import SaveFileOnChange
from Library.Misc import IsAllModuleList
from Library.Misc import Sdict
from Library.Misc import ConvertPath
from Library.Misc import ConvertSpec
from Library.CommentGenerating import GenHeaderCommentSection
from Library.CommentGenerating import GenGenericCommentF
from Library.CommentGenerating import _GetHelpStr
from Library import GlobalData
from Logger import StringTable as ST
from Logger import ToolError
import Logger.Log as Logger
from Library import DataType as DT
from GenMetaFile import GenMetaFileMisc

## Transfer Module Object to Inf files
#
# Transfer all contents of a standard Module Object to an Inf file 
# @param ModuleObject: A Module Object  
#
def ModuleToInf(ModuleObject):
    if not GlobalData.gWSPKG_LIST:  
        GlobalData.gWSPKG_LIST = GetWorkspacePackage()
    
    #
    # Init global information for the file
    #
    ContainerFile = ModuleObject.GetFullPath()
    Content = ''
    #
    # generate header comment section
    #        
    Content += GenHeaderCommentSection(ModuleObject.GetAbstract(), 
                                       ModuleObject.GetDescription(), 
                                       ModuleObject.GetCopyright(), 
                                       ModuleObject.GetLicense())
    
    #
    # Judge whether the INF file is an AsBuild INF.
    #
    if ModuleObject.BinaryModule:
        GlobalData.gIS_BINARY_INF = True
    else:
        GlobalData.gIS_BINARY_INF = False
    
    #
    # for each section, maintain a dict, sorted arch will be its key, 
    # statement list will be its data
    # { 'Arch1 Arch2 Arch3': [statement1, statement2],
    #   'Arch1' : [statement1, statement3] 
    #  }
    #
    
    #
    # Gen section contents
    #
    Content += GenDefines(ModuleObject)
    Content += GenBuildOptions(ModuleObject)
    Content += GenLibraryClasses(ModuleObject)
    Content += GenPackages(ModuleObject)
    Content += GenPcdSections(ModuleObject)
    Content += GenSources(ModuleObject)
    Content += GenProtocolPPiSections(ModuleObject.GetProtocolList(), True) 
    Content += GenProtocolPPiSections(ModuleObject.GetPpiList(), False) 
    Content += GenGuidSections(ModuleObject.GetGuidList())    
    Content += GenBinaries(ModuleObject)
    Content += GenDepex(ModuleObject)
    Content += GenUserExtensions(ModuleObject)        

    if ModuleObject.GetEventList() or ModuleObject.GetBootModeList() or ModuleObject.GetHobList():
        Content += '\n\n'
    #
    # generate [Event], [BootMode], [Hob] section
    #
    Content += GenSpecialSections(ModuleObject.GetEventList(), 'Event')    
    Content += GenSpecialSections(ModuleObject.GetBootModeList(), 'BootMode')
    Content += GenSpecialSections(ModuleObject.GetHobList(), 'Hob')

    SaveFileOnChange(ContainerFile, Content, False)
    return ContainerFile

def GenDefines(ModuleObject):
    #
    # generate [Defines] section
    #
    Content = ''          
    NewSectionDict = {}     
    for UserExtension in ModuleObject.GetUserExtensionList():
        DefinesDict = UserExtension.GetDefinesDict()
        if not DefinesDict:
            continue
        
        for Statement in DefinesDict:
            SortedArch = DT.TAB_ARCH_COMMON
            if Statement.strip().startswith(DT.TAB_INF_DEFINES_CUSTOM_MAKEFILE):
                pos = Statement.find(DT.TAB_VALUE_SPLIT)
                if pos == -1:
                    pos = Statement.find(DT.TAB_EQUAL_SPLIT)
                Makefile = ConvertPath(Statement[pos + 1:].strip())
                Statement = Statement[:pos + 1] + ' ' + Makefile
            if SortedArch in NewSectionDict:
                NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [Statement]
            else:
                NewSectionDict[SortedArch] = [Statement]        

    SpecialStatementList = []
    
    #
    # Add INF_VERSION statement firstly
    #
    Statement = 'INF_VERSION = 0x00010017'
    SpecialStatementList.append(Statement)

    BaseName = ModuleObject.GetBaseName()
    if BaseName.startswith('.') or BaseName.startswith('-'):
        BaseName = '_' + BaseName
    Statement = '%s = %s' % (DT.TAB_INF_DEFINES_BASE_NAME, BaseName)
    SpecialStatementList.append(Statement)
    Statement = '%s = %s' % (DT.TAB_INF_DEFINES_FILE_GUID, ModuleObject.GetGuid())
    SpecialStatementList.append(Statement)
    Statement = '%s = %s' % (DT.TAB_INF_DEFINES_VERSION_STRING, ModuleObject.GetVersion())
    SpecialStatementList.append(Statement)
    
    if ModuleObject.GetModuleType():
        Statement = '%s = %s' % (DT.TAB_INF_DEFINES_MODULE_TYPE, ModuleObject.GetModuleType())
        SpecialStatementList.append(Statement)
    if ModuleObject.GetPcdIsDriver():
        Statement = '%s = %s' % (DT.TAB_INF_DEFINES_PCD_IS_DRIVER, ModuleObject.GetPcdIsDriver())
        SpecialStatementList.append(Statement)
    if ModuleObject.GetUefiSpecificationVersion():
        Statement = '%s = %s' % (DT.TAB_INF_DEFINES_UEFI_SPECIFICATION_VERSION, \
                                 ModuleObject.GetUefiSpecificationVersion())
        SpecialStatementList.append(Statement)
    if ModuleObject.GetPiSpecificationVersion():
        Statement = '%s = %s' % (DT.TAB_INF_DEFINES_PI_SPECIFICATION_VERSION, ModuleObject.GetPiSpecificationVersion())
        SpecialStatementList.append(Statement)        
    for LibraryClass in ModuleObject.GetLibraryClassList():
        if LibraryClass.GetUsage() == DT.USAGE_ITEM_PRODUCES or \
           LibraryClass.GetUsage() == DT.USAGE_ITEM_SOMETIMES_PRODUCES:
            Statement = '%s = %s' % (DT.TAB_INF_DEFINES_LIBRARY_CLASS, LibraryClass.GetLibraryClass())
            if LibraryClass.GetSupModuleList():
                Statement += '|' + DT.TAB_SPACE_SPLIT.join(l for l in LibraryClass.GetSupModuleList())
            SpecialStatementList.append(Statement)
    for SpecItem in ModuleObject.GetSpecList():
        Spec, Version = SpecItem
        Spec = ConvertSpec(Spec)
        Statement = '%s %s = %s' % (DT.TAB_INF_DEFINES_SPEC, Spec, Version)
        SpecialStatementList.append(Statement)
        
    ExternList = []
    for Extern in ModuleObject.GetExternList():
        ArchList = Extern.GetSupArchList()
        EntryPoint = Extern.GetEntryPoint()
        UnloadImage = Extern.GetUnloadImage()
        Constructor = Extern.GetConstructor()
        Destructor = Extern.GetDestructor()
        HelpStringList = Extern.GetHelpTextList()
        FFE = Extern.GetFeatureFlag()
        ExternList.append([ArchList, EntryPoint, UnloadImage, Constructor, Destructor, FFE, HelpStringList])
    
    #
    # Add VALID_ARCHITECTURES information
    #
    ValidArchStatement = None
    if ModuleObject.SupArchList:
        ValidArchStatement = '# ' + '\n'
        ValidArchStatement += '# The following information is for reference only and not required by the build tools.\n'
        ValidArchStatement += '# ' + '\n'
        ValidArchStatement += '# VALID_ARCHITECTURES = %s' % (' '.join(ModuleObject.SupArchList)) + '\n'
        ValidArchStatement += '# ' + '\n'
        
    if DT.TAB_ARCH_COMMON not in NewSectionDict:
        NewSectionDict[DT.TAB_ARCH_COMMON] = []
    NewSectionDict[DT.TAB_ARCH_COMMON] = NewSectionDict[DT.TAB_ARCH_COMMON] + SpecialStatementList
    GenMetaFileMisc.AddExternToDefineSec(NewSectionDict, DT.TAB_ARCH_COMMON, ExternList)
    if ValidArchStatement is not None:
        NewSectionDict[DT.TAB_ARCH_COMMON] = NewSectionDict[DT.TAB_ARCH_COMMON] + [ValidArchStatement]
        
    Content += GenSection('Defines', NewSectionDict)
    
    return Content

def GenLibraryClasses(ModuleObject):
    #
    # generate [LibraryClasses] section
    #
    Content = ''
    NewSectionDict = {}
    if not GlobalData.gIS_BINARY_INF:
        for LibraryClass in ModuleObject.GetLibraryClassList():
            if LibraryClass.GetUsage() == DT.USAGE_ITEM_PRODUCES:
                continue
            #
            # Generate generic comment
            #
            HelpTextList = LibraryClass.GetHelpTextList()
            HelpStr = _GetHelpStr(HelpTextList)
            CommentStr = GenGenericCommentF(HelpStr)
            Statement = CommentStr
            Name = LibraryClass.GetLibraryClass()
            FFE = LibraryClass.GetFeatureFlag()
            Statement += Name
            if FFE:
                Statement += '|' + FFE 
            ModuleList = LibraryClass.GetSupModuleList()
            ArchList = LibraryClass.GetSupArchList()
            for Index in xrange(0, len(ArchList)):
                ArchList[Index] = ConvertArchForInstall(ArchList[Index])
            ArchList.sort()
            SortedArch = ' '.join(ArchList)
            
            KeyList = []
            if not ModuleList or IsAllModuleList(ModuleList):
                KeyList = [SortedArch]        
            else:
                ModuleString = DT.TAB_VALUE_SPLIT.join(l for l in ModuleList)
                if not ArchList:
                    SortedArch = DT.TAB_ARCH_COMMON
                    KeyList = [SortedArch + '.' + ModuleString]
                else:
                    KeyList = [Arch + '.' + ModuleString for Arch in ArchList]
                                
            for Key in KeyList:
                if Key in NewSectionDict:
                    NewSectionDict[Key] = NewSectionDict[Key] + [Statement]
                else:
                    NewSectionDict[Key] = [Statement]
        Content += GenSection('LibraryClasses', NewSectionDict)
    else:
        LibraryClassDict = {}
        for BinaryFile in ModuleObject.GetBinaryFileList():
            if not BinaryFile.AsBuiltList:
                continue
            for LibraryItem in BinaryFile.AsBuiltList[0].LibraryInstancesList:
                Statement = '# Guid: ' +  LibraryItem.Guid + ' Version: ' + LibraryItem.Version
                if len(BinaryFile.SupArchList) == 0:
                    if LibraryClassDict.has_key('COMMON'):
                        LibraryClassDict['COMMON'].append(Statement)
                    else:
                        LibraryClassDict['COMMON'] = ['## @LIB_INSTANCES']
                        LibraryClassDict['COMMON'].append(Statement)
                else:
                    for Arch in BinaryFile.SupArchList:
                        if LibraryClassDict.has_key(Arch):
                            LibraryClassDict[Arch].append(Statement)
                        else:
                            LibraryClassDict[Arch] = ['## @LIB_INSTANCES']
                            LibraryClassDict[Arch].append(Statement)
                    
        Content += GenSection('LibraryClasses', LibraryClassDict)
    
    return Content

def GenPackages(ModuleObject):
    Content = ''
    #
    # generate [Packages] section
    #
    NewSectionDict = Sdict()
    WorkspaceDir = getenv('WORKSPACE')
    for PackageDependency in ModuleObject.GetPackageDependencyList():
        #
        # Generate generic comment
        #
        CommentStr = ''
        HelpText = PackageDependency.GetHelpText()
        if HelpText:
            HelpStr = HelpText.GetString()
            CommentStr = GenGenericCommentF(HelpStr)   
        Statement = CommentStr
        Guid = PackageDependency.GetGuid()
        Version = PackageDependency.GetVersion()
        FFE = PackageDependency.GetFeatureFlag()
        #
        # find package path/name
        # 
        for PkgInfo in GlobalData.gWSPKG_LIST:
            if Guid == PkgInfo[1]:
                if (not Version) or (Version == PkgInfo[2]):
                    Path = PkgInfo[3]
                    break
        #
        # get relative path
        #
        RelaPath = Path[Path.upper().find(WorkspaceDir.upper()) + len(WorkspaceDir) + 1:]
        Statement += RelaPath.replace('\\', '/')
        if FFE:
            Statement += '|' + FFE        
        ArchList = PackageDependency.GetSupArchList()
        ArchList.sort()
        SortedArch = ' '.join(ArchList)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]            

    Content += GenSection('Packages', NewSectionDict)
    
    return Content

def GenSources(ModuleObject):
    #
    # generate [Sources] section
    #
    Content = ''
    NewSectionDict = {}
    
    for Source in ModuleObject.GetSourceFileList():    
        SourceFile = Source.GetSourceFile()
        Family = Source.GetFamily()
        FeatureFlag = Source.GetFeatureFlag()
        SupArchList = Source.GetSupArchList()
        SupArchList.sort()
        SortedArch = ' '.join(SupArchList)    

        Statement = GenSourceStatement(ConvertPath(SourceFile), Family, FeatureFlag)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [Statement]
        else:
            NewSectionDict[SortedArch] = [Statement]

    Content += GenSection('Sources', NewSectionDict)
    
    return Content

def GenDepex(ModuleObject):
    #
    # generate [Depex] section
    #
    NewSectionDict = Sdict()
    Content = ''
    for Depex in ModuleObject.GetPeiDepex() + ModuleObject.GetDxeDepex() + ModuleObject.GetSmmDepex():
        HelpTextList = Depex.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)
        CommentStr = GenGenericCommentF(HelpStr)
        SupArchList = Depex.GetSupArchList()
        SupModList = Depex.GetModuleType()
        Expression = Depex.GetDepex()
        Statement = CommentStr + Expression
        
        SupArchList.sort()
        KeyList = []
        if not SupArchList:
            SupArchList.append(DT.TAB_ARCH_COMMON.lower())
        if not SupModList:
            KeyList = SupArchList
        else:
            for ModuleType in SupModList:
                for Arch in SupArchList:
                    KeyList.append(ConvertArchForInstall(Arch) + '.' + ModuleType)
                            
        for Key in KeyList:
            if Key in NewSectionDict:
                NewSectionDict[Key] = NewSectionDict[Key] + [Statement]
            else:
                NewSectionDict[Key] = [Statement]
        
    Content += GenSection('Depex', NewSectionDict, False)
    
    return Content

## GenUserExtensions
#
# GenUserExtensions
#
def GenUserExtensions(ModuleObject):
    NewSectionDict = {}
    for UserExtension in ModuleObject.GetUserExtensionList():
        if UserExtension.GetIdentifier() == 'Depex':
            continue
        Statement = UserExtension.GetStatement()
        if not Statement:
            continue
        
        ArchList = UserExtension.GetSupArchList()
        for Index in xrange(0, len(ArchList)):
            ArchList[Index] = ConvertArchForInstall(ArchList[Index])
        ArchList.sort()
                
        KeyList = []
        CommonPreFix = ''
        if UserExtension.GetUserID():
            CommonPreFix = UserExtension.GetUserID()
            if CommonPreFix.find('.') > -1:
                CommonPreFix = '"' + CommonPreFix + '"'
            if UserExtension.GetIdentifier():
                CommonPreFix += '.' + '"' + UserExtension.GetIdentifier() + '"'
            if ArchList:
                KeyList = [CommonPreFix + '.' + Arch for Arch in ArchList]
            else:
                KeyList = [CommonPreFix]    
        
        for Key in KeyList:
            if Key in NewSectionDict:
                NewSectionDict[Key] = NewSectionDict[Key] + [Statement]
            else:
                NewSectionDict[Key] = [Statement]
    Content = GenSection('UserExtensions', NewSectionDict, False)
    
    return Content
    
# GenSourceStatement
#
#  @param SourceFile: string of source file path/name
#  @param Family:     string of source file family field
#  @param FeatureFlag:  string of source file FeatureFlag field
#  @param TagName:  string of source file TagName field
#  @param ToolCode:  string of source file ToolCode field
#  @param HelpStr:  string of source file HelpStr field
#
#  @retval Statement: The generated statement for source
#
def GenSourceStatement(SourceFile, Family, FeatureFlag, TagName=None, 
                       ToolCode=None, HelpStr=None):
    Statement = ''
    if HelpStr:
        Statement += GenGenericCommentF(HelpStr)   
    #
    # format of SourceFile|Family|TagName|ToolCode|FeatureFlag
    #
    Statement += SourceFile
    
    if TagName == None:
        TagName = ''
    if ToolCode == None:
        ToolCode = ''
    if HelpStr == None:
        HelpStr = ''
    
    if FeatureFlag:
        Statement += '|' + Family + '|' + TagName + '|' + ToolCode + '|' + FeatureFlag
    elif ToolCode:
        Statement += '|' + Family + '|' + TagName + '|' + ToolCode
    elif TagName:
        Statement += '|' + Family + '|' + TagName
    elif Family:
        Statement += '|' + Family
    
    return Statement

# GenBinaryStatement
#
#  @param Key:       (FileName, FileType, FFE, SortedArch)
#  @param Value:     (Target, Family, TagName, Comment)
#
#
def GenBinaryStatement(Key, Value):
    (FileName, FileType, FFE, SortedArch) = Key
    if SortedArch:
        pass
    if Value:
        (Target, Family, TagName, Comment) = Value
    else:
        Target = ''
        Family = ''
        TagName = ''
        Comment = ''
    
    if Comment:
        Statement = GenGenericCommentF(Comment)
    else:
        Statement = ''
    
    Statement += FileType + '|' + FileName

    if FileType in DT.BINARY_FILE_TYPE_UI_LIST + DT.BINARY_FILE_TYPE_VER_LIST:
        if FFE:
            Statement += '|' + Target + '|' + FFE
        elif Target:
            Statement += '|' + Target
    else:
        if FFE:
            Statement += '|' + Target + '|' + Family + '|' + TagName + '|' + FFE
        elif TagName:
            Statement += '|' + Target + '|' + Family + '|' + TagName
        elif Family:
            Statement += '|' + Target + '|' + Family
        elif Target:
            Statement += '|' + Target

    return Statement

## GenGuidSections
# 
#  @param GuidObjList: List of GuidObject
#  @retVal Content: The generated section contents
#
def GenGuidSections(GuidObjList):
    #
    # generate [Guids] section
    #
    Content = '' 
    GuidDict = Sdict()

    for Guid in GuidObjList:
        HelpTextList = Guid.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)

        CName = Guid.GetCName()
        FFE = Guid.GetFeatureFlag()
        Statement = CName
        if FFE:
            Statement += '|' + FFE
        
        Usage = Guid.GetUsage()
        GuidType = Guid.GetGuidTypeList()[0]
        VariableName = Guid.GetVariableName()
        
        #
        # we need to differentiate the generic comment and usage comment
        # as multiple generic comment need to be put at first
        #
        if Usage == DT.ITEM_UNDEFINED and GuidType == DT.ITEM_UNDEFINED:
            # generate list of generic comment
            Comment = GenGenericCommentF(HelpStr)
        else:
            # generate list of other comment
            Comment = HelpStr.replace('\n', ' ')
            Comment = Comment.strip()
            if Comment:
                Comment = ' # ' + Comment
            else:
                Comment = ''
            
            if Usage != DT.ITEM_UNDEFINED and GuidType == DT.ITEM_UNDEFINED:
                Comment = '## ' + Usage + Comment
            elif GuidType == 'Variable':
                Comment = '## ' + Usage + ' ## ' + GuidType + ':' + VariableName + Comment
            else:
                Comment = '## ' + Usage + ' ## ' + GuidType + Comment
            
            if Comment:
                Comment += '\n'
        
        #
        # merge duplicate items
        #
        ArchList = Guid.GetSupArchList()
        ArchList.sort()
        SortedArch = ' '.join(ArchList)
        if (Statement, SortedArch) in GuidDict:
            PreviousComment = GuidDict[Statement, SortedArch]
            Comment = PreviousComment +  Comment     
        GuidDict[Statement, SortedArch] = Comment

    
    NewSectionDict = GenMetaFileMisc.TransferDict(GuidDict) 

    #
    # generate the section contents
    #
    if NewSectionDict:
        Content = GenSection('Guids', NewSectionDict)
    
    return Content

## GenProtocolPPiSections
# 
#  @param ObjList: List of ProtocolObject or Ppi Object
#  @retVal Content: The generated section contents
#
def GenProtocolPPiSections(ObjList, IsProtocol):
    Content = ''
    Dict = Sdict()
    for Object in ObjList:
        HelpTextList = Object.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)

        CName = Object.GetCName()
        FFE = Object.GetFeatureFlag()
        Statement = CName
        if FFE:
            Statement += '|' + FFE
        
        Usage = Object.GetUsage()
        Notify = Object.GetNotify()
        
        #
        # we need to differentiate the generic comment and usage comment
        # as consecutive generic comment need to be put together
        #
        if Usage == DT.ITEM_UNDEFINED and Notify == '':
            # generate list of generic comment
            Comment = GenGenericCommentF(HelpStr)
        else:
            # generate list of other comment
            Comment = HelpStr.replace('\n', ' ')
            Comment = Comment.strip()
            if Comment:
                Comment = ' # ' + Comment
            else:
                Comment = ''
            
            if Usage == DT.ITEM_UNDEFINED and not Comment and Notify == '':
                Comment = ''
            else:
                if Notify:
                    Comment = '## ' + Usage + ' ## ' + 'NOTIFY' + Comment
                else:
                    Comment = '## ' + Usage + Comment
           
            if Comment:
                Comment += '\n'
        
        #
        # merge duplicate items
        #
        ArchList = Object.GetSupArchList()
        ArchList.sort()
        SortedArch = ' '.join(ArchList)
        if (Statement, SortedArch) in Dict:
            PreviousComment = Dict[Statement, SortedArch]
            Comment = PreviousComment + Comment
        Dict[Statement, SortedArch] = Comment
    
    NewSectionDict = GenMetaFileMisc.TransferDict(Dict)         

    #
    # generate the section contents
    #
    if NewSectionDict:
        if IsProtocol:
            Content = GenSection('Protocols', NewSectionDict)
        else:
            Content = GenSection('Ppis', NewSectionDict)
    
    return Content

## GenPcdSections
#
#
def GenPcdSections(ModuleObject):
    Content = ''
    if not GlobalData.gIS_BINARY_INF:
        #
        # for each Pcd Itemtype, maintain a dict so the same type will be grouped 
        # together
        #
        ItemTypeDict = {}
        for Pcd in ModuleObject.GetPcdList():
            HelpTextList = Pcd.GetHelpTextList()
            HelpStr = _GetHelpStr(HelpTextList)
    
            Statement = ''
            CName = Pcd.GetCName()
            TokenSpaceGuidCName = Pcd.GetTokenSpaceGuidCName()
            DefaultValue = Pcd.GetDefaultValue()
            ItemType = Pcd.GetItemType()
            if ItemType in ItemTypeDict:
                Dict = ItemTypeDict[ItemType]
            else:
                Dict = Sdict()
                ItemTypeDict[ItemType] = Dict
                
            FFE = Pcd.GetFeatureFlag()
            Statement += TokenSpaceGuidCName + '.' + CName
            if DefaultValue:
                Statement += '|' + DefaultValue
                if FFE:
                    Statement += '|' + FFE
            elif FFE:
                Statement += '||' + FFE
    
            #
            # Generate comment
            #
            Usage = Pcd.GetValidUsage()
            
            #
            # if FeatureFlag Pcd, then assume all Usage is CONSUMES
            #
            if ItemType == DT.TAB_INF_FEATURE_PCD:
                Usage = DT.USAGE_ITEM_CONSUMES
            if Usage == DT.ITEM_UNDEFINED or (ItemType == DT.TAB_INF_FEATURE_PCD):
                # generate list of generic comment
                Comment = GenGenericCommentF(HelpStr)
            else:
                # generate list of other comment
                Comment = HelpStr.replace('\n', ' ')
                Comment = Comment.strip()
                if Comment:
                    Comment = ' # ' + Comment
                else:
                    Comment = ''
    
                Comment = '## ' + Usage + Comment
               
                if Comment:
                    Comment += '\n'
        
            #
            # Merge duplicate entries
            #
            ArchList = Pcd.GetSupArchList()
            ArchList.sort()
            SortedArch = ' '.join(ArchList)
            if (Statement, SortedArch) in Dict:
                PreviousComment = Dict[Statement, SortedArch]
                Comment = PreviousComment + Comment
            Dict[Statement, SortedArch] = Comment             
                 
        for ItemType in ItemTypeDict:
            #
            # First we need to transfer the Dict to use SortedArch as key
            #
            Dict = ItemTypeDict[ItemType]
            NewSectionDict = GenMetaFileMisc.TransferDict(Dict)   
            
            if NewSectionDict:
                Content += GenSection(ItemType, NewSectionDict)
    #
    # For AsBuild INF files   
    #
    else:
        Content += GenAsBuiltPacthPcdSections(ModuleObject)
        Content += GenAsBuiltPcdExSections(ModuleObject)
    
    return Content

## GenPcdSections
#
#
def GenAsBuiltPacthPcdSections(ModuleObject):
    PatchPcdDict = {}
    for BinaryFile in ModuleObject.GetBinaryFileList():
        if not BinaryFile.AsBuiltList:
            continue            
        for PatchPcd in BinaryFile.AsBuiltList[0].PatchPcdList:      
            TokenSpaceName = ''
            PcdCName = PatchPcd.CName
            PcdValue = PatchPcd.DefaultValue
            PcdOffset = PatchPcd.Offset
            TokenSpaceGuidValue = PatchPcd.TokenSpaceGuidValue
            Token = PatchPcd.Token
            HelpTextList = PatchPcd.HelpTextList
            HelpString = ''
            for HelpStringItem in HelpTextList:
                for HelpLine in GetSplitValueList(HelpStringItem.String, '\n'):
                    HelpString += '# ' + HelpLine + '\n'
                    
            TokenSpaceName, PcdCName = GenMetaFileMisc.ObtainPcdName(ModuleObject.PackageDependencyList, 
                                                                     TokenSpaceGuidValue, 
                                                                     Token)
            if TokenSpaceName == '' or PcdCName == '':    
                Logger.Error("Upt", 
                             ToolError.RESOURCE_NOT_AVAILABLE,
                             ST.ERR_INSTALL_FILE_DEC_FILE_ERROR%(TokenSpaceGuidValue, Token), 
                             File=ModuleObject.GetFullPath())                 
            Statement = HelpString[:-3] + TokenSpaceName + '.' + PcdCName + ' | ' + PcdValue + ' | ' + PcdOffset
            
            if len(BinaryFile.SupArchList) == 0:
                if PatchPcdDict.has_key('COMMON'):
                    PatchPcdDict['COMMON'].append(Statement)
                else:
                    PatchPcdDict['COMMON'] = [Statement]
            else:
                for Arch in BinaryFile.SupArchList:
                    if PatchPcdDict.has_key(Arch):
                        PatchPcdDict[Arch].append(Statement)
                    else:
                        PatchPcdDict[Arch] = [Statement]
    return GenSection('PatchPcd', PatchPcdDict)

## GenPcdSections
#
#
def GenAsBuiltPcdExSections(ModuleObject):
    PcdExDict = {}
    for BinaryFile in ModuleObject.GetBinaryFileList():
        if not BinaryFile.AsBuiltList:
            continue   
        for PcdExItem in BinaryFile.AsBuiltList[0].PcdExValueList:
            TokenSpaceName = ''
            PcdCName = PcdExItem.CName
            PcdValue = PcdExItem.DefaultValue
            TokenSpaceGuidValue = PcdExItem.TokenSpaceGuidValue
            Token = PcdExItem.Token
            HelpTextList = PcdExItem.HelpTextList
            HelpString = ''
            for HelpStringItem in HelpTextList:
                for HelpLine in GetSplitValueList(HelpStringItem.String, '\n'):
                    HelpString += '# ' + HelpLine + '\n'            
            TokenSpaceName, PcdCName = GenMetaFileMisc.ObtainPcdName(ModuleObject.PackageDependencyList, 
                                                                     TokenSpaceGuidValue, Token)
            
            if TokenSpaceName == '' or PcdCName == '':    
                Logger.Error("Upt",
                             ToolError.RESOURCE_NOT_AVAILABLE,
                             ST.ERR_INSTALL_FILE_DEC_FILE_ERROR%(TokenSpaceGuidValue, Token), 
                             File=ModuleObject.GetFullPath()) 
                                      
            Statement = HelpString[:-3] + TokenSpaceName + '.' + PcdCName + ' | ' + PcdValue
            
            if len(BinaryFile.SupArchList) == 0:
                if PcdExDict.has_key('COMMON'):
                    PcdExDict['COMMON'].append(Statement)
                else:
                    PcdExDict['COMMON'] = [Statement]
            else:
                for Arch in BinaryFile.SupArchList:
                    if PcdExDict.has_key(Arch):
                        PcdExDict[Arch].append(Statement)
                    else:
                        PcdExDict[Arch] = [Statement]
    return GenSection('PcdEx', PcdExDict)
               
## GenSpecialSections
#  generate special sections for Event/BootMode/Hob
#
def GenSpecialSections(ObjectList, SectionName):
    #
    # generate section
    #
    Content = ''
    NewSectionDict = {}
    for Obj in ObjectList:
        #
        # Generate comment
        #
        CommentStr = ''
        HelpTextList = Obj.GetHelpTextList()
        HelpStr = _GetHelpStr(HelpTextList)
        CommentStr = GenGenericCommentF(HelpStr)
        
        if SectionName == 'Hob':
            Type = Obj.GetHobType()
        elif SectionName == 'Event':
            Type = Obj.GetEventType()
        elif SectionName == 'BootMode':
            Type = Obj.GetSupportedBootModes()
        else:
            assert(SectionName)
        
        Usage = Obj.GetUsage()
        Statement = ' ' + Type + ' ## ' + Usage
        
        if CommentStr in ['#\n', '#\n#\n']:
            CommentStr = '#\n#\n#\n'
        #
        # the first head comment line should start with '##\n', 
        # if it starts with '#\n', then add one '#'
        # else add '##\n' to meet the format defined in INF spec
        #
        if CommentStr.startswith('#\n'):
            CommentStr = '#' + CommentStr
        elif CommentStr:
            CommentStr = '##\n' + CommentStr

        if CommentStr and not CommentStr.endswith('\n#\n'):
            CommentStr = CommentStr + '#\n' 
        
        NewStateMent = CommentStr + Statement
        SupArch = Obj.GetSupArchList()
        SupArch.sort()
        SortedArch = ' '.join(SupArch)
        if SortedArch in NewSectionDict:
            NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [NewStateMent]
        else:
            NewSectionDict[SortedArch] = [NewStateMent]
    
    SectionContent = GenSection(SectionName, NewSectionDict)
    SectionContent = SectionContent.strip()
    if SectionContent:
        Content = '# ' + ('\n' + '# ').join(GetSplitValueList(SectionContent, '\n'))
        Content = Content.lstrip()
    #
    # add two empty line after the generated section content to differentiate 
    # it between other possible sections
    #
    if Content: 
        Content += '\n#\n#\n'
    return Content

## GenBuildOptions
#
#
def GenBuildOptions(ModuleObject):
    Content = ''
    if not ModuleObject.BinaryModule:
        #
        # generate [BuildOptions] section
        #
        NewSectionDict = {}
        for UserExtension in ModuleObject.GetUserExtensionList():
            BuildOptionDict = UserExtension.GetBuildOptionDict()
            if not BuildOptionDict:
                continue
            for Arch in BuildOptionDict:
                if Arch in NewSectionDict:
                    NewSectionDict[Arch] = NewSectionDict[Arch] + [BuildOptionDict[Arch]]
                else:
                    NewSectionDict[Arch] = [BuildOptionDict[Arch]]
    
        Content = GenSection('BuildOptions', NewSectionDict)
    else:
        BuildOptionDict = {}
        for BinaryFile in ModuleObject.GetBinaryFileList():
            if not BinaryFile.AsBuiltList:
                continue
            for BuilOptionItem in BinaryFile.AsBuiltList[0].BinaryBuildFlagList:
                Statement = '#' +  BuilOptionItem.AsBuiltOptionFlags
                if len(BinaryFile.SupArchList) == 0:
                    if BuildOptionDict.has_key('COMMON'):
                        if Statement not in BuildOptionDict['COMMON']:
                            BuildOptionDict['COMMON'].append(Statement)
                    else:
                        BuildOptionDict['COMMON'] = ['## @AsBuilt']
                        BuildOptionDict['COMMON'].append(Statement)
                else:
                    for Arch in BinaryFile.SupArchList:
                        if BuildOptionDict.has_key(Arch):
                            if Statement not in BuildOptionDict[Arch]:
                                BuildOptionDict[Arch].append(Statement)
                        else:
                            BuildOptionDict[Arch] = ['## @AsBuilt']
                            BuildOptionDict[Arch].append(Statement)
                    
        Content = GenSection('BuildOptions', BuildOptionDict)
    
    return Content

## GenBinaries
#
#
def GenBinaries(ModuleObject):
    NewSectionDict = {}
    BinariesDict = []
    for UserExtension in ModuleObject.GetUserExtensionList():
        BinariesDict = UserExtension.GetBinariesDict()
        if BinariesDict:
            break
        
    for BinaryFile in ModuleObject.GetBinaryFileList():
        FileNameObjList = BinaryFile.GetFileNameList()
        for FileNameObj in FileNameObjList:
            FileName = ConvertPath(FileNameObj.GetFilename())
            FileType = FileNameObj.GetFileType()
            FFE = FileNameObj.GetFeatureFlag()
            ArchList = FileNameObj.GetSupArchList()
            ArchList.sort()
            SortedArch = ' '.join(ArchList)      
            
            Key = (FileName, FileType, FFE, SortedArch)

            if Key in BinariesDict:
                ValueList = BinariesDict[Key]
                for ValueItem in ValueList:
                    Statement = GenBinaryStatement(Key, ValueItem)
                    if SortedArch in NewSectionDict:
                        NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [Statement]
                    else:
                        NewSectionDict[SortedArch] = [Statement]
                #
                # as we already generated statement for this DictKey
                # here set the Valuelist to be empty to avoid generate duplicate entries 
                # as the DictKey may have multiple entries
                #
                BinariesDict[Key] = []
            else:
                Statement = GenBinaryStatement(Key, None)
                if SortedArch in NewSectionDict:
                    NewSectionDict[SortedArch] = NewSectionDict[SortedArch] + [Statement]
                else:
                    NewSectionDict[SortedArch] = [Statement]               
 
    return GenSection('Binaries', NewSectionDict)