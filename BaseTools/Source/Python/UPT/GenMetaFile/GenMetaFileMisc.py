## @file GenMetaFileMisc.py
#
# This file contained the miscellaneous routines for GenMetaFile usage.
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
GenMetaFileMisc
'''

from Library import DataType as DT
from Library import GlobalData
from Parser.DecParser import Dec

# AddExternToDefineSec
#
#  @param SectionDict: string of source file path/name
#  @param Arch:     string of source file family field
#  @param ExternList:  string of source file FeatureFlag field
#   
def AddExternToDefineSec(SectionDict, Arch, ExternList):
    for ArchList, EntryPoint, UnloadImage, Constructor, Destructor, FFE, HelpStringList in ExternList:
        if Arch or ArchList:
            if EntryPoint:
                Statement = '%s = %s' % (DT.TAB_INF_DEFINES_ENTRY_POINT, EntryPoint)
                if FFE:
                    Statement += ' | %s' % FFE
                if len(HelpStringList) > 0:
                    Statement = HelpStringList[0].GetString() + '\n' + Statement
                if len(HelpStringList) > 1:
                    Statement = Statement + HelpStringList[1].GetString()
                SectionDict[Arch] = SectionDict[Arch] + [Statement]
            if UnloadImage:
                Statement = '%s = %s' % (DT.TAB_INF_DEFINES_UNLOAD_IMAGE, UnloadImage)
                if FFE:
                    Statement += ' | %s' % FFE
                
                if len(HelpStringList) > 0:
                    Statement = HelpStringList[0].GetString() + '\n' + Statement
                if len(HelpStringList) > 1:
                    Statement = Statement + HelpStringList[1].GetString()
                SectionDict[Arch] = SectionDict[Arch] + [Statement]
            if Constructor:
                Statement = '%s = %s' % (DT.TAB_INF_DEFINES_CONSTRUCTOR, Constructor)
                if FFE:
                    Statement += ' | %s' % FFE
                    
                if len(HelpStringList) > 0:
                    Statement = HelpStringList[0].GetString() + '\n' + Statement
                if len(HelpStringList) > 1:
                    Statement = Statement + HelpStringList[1].GetString()      
                SectionDict[Arch] = SectionDict[Arch] + [Statement]  
            if Destructor:
                Statement = '%s = %s' % (DT.TAB_INF_DEFINES_DESTRUCTOR, Destructor)
                if FFE:
                    Statement += ' | %s' % FFE
                    
                if len(HelpStringList) > 0:
                    Statement = HelpStringList[0].GetString() + '\n' + Statement
                if len(HelpStringList) > 1:
                    Statement = Statement + HelpStringList[1].GetString()
                SectionDict[Arch] = SectionDict[Arch] + [Statement]
                
## ObtainPcdName
#
# Using TokenSpaceGuidValue and Token to obtain PcdName from DEC file
#
def ObtainPcdName(Packages, TokenSpaceGuidValue, Token):
    for PackageDependency in Packages:
        #
        # Generate generic comment
        #
        Guid = PackageDependency.GetGuid()
        Version = PackageDependency.GetVersion()
          
        #
        # find package path/name
        # 
        for PkgInfo in GlobalData.gWSPKG_LIST:
            if Guid == PkgInfo[1]:
                if (not Version) or (Version == PkgInfo[2]):
                    Path = PkgInfo[3]
                    break
    
        DecFile = Dec(Path)
        DecGuidsDict = DecFile.GetGuidSectionObject().ValueDict
        DecPcdsDict = DecFile.GetPcdSectionObject().ValueDict
        
        TokenSpaceGuidName = ''
        PcdCName = ''
        TokenSpaceGuidNameFound = False
        PcdCNameFound = False
        
        #
        # Get TokenSpaceGuidCName from Guids section 
        #
        for GuidKey in DecGuidsDict:
            GuidList = DecGuidsDict[GuidKey]
            if TokenSpaceGuidNameFound:
                break
            for GuidItem in GuidList:
                if TokenSpaceGuidValue == GuidItem.GuidString:
                    TokenSpaceGuidName = GuidItem.GuidCName
                    TokenSpaceGuidNameFound = True
                    break
    
        #
        # Retrieve PcdCName from Pcds Section
        #
        for PcdKey in DecPcdsDict:
            PcdList = DecPcdsDict[PcdKey]
            if PcdCNameFound:
                break
            for PcdItem in PcdList:
                if TokenSpaceGuidName == PcdItem.TokenSpaceGuidCName and Token == PcdItem.TokenValue:
                    PcdCName = PcdItem.TokenCName
                    PcdCNameFound = True
                    break        
        
    return TokenSpaceGuidName, PcdCName

## _TransferDict
#  transfer dict that using (Statement, SortedArch) as key, 
#  (GenericComment, UsageComment) as value into a dict that using SortedArch as
#  key and NewStatement as value 
#
def TransferDict(OrigDict):
    NewDict = {}

    for Statement, SortedArch in OrigDict:
        Comment = OrigDict[Statement, SortedArch]
        #
        # apply the NComment/1Comment rule
        #
        if Comment.find('\n') != len(Comment) - 1:    
            NewStateMent = Comment + Statement
        else:
            NewStateMent = Statement + ' ' + Comment.rstrip('\n')

        if SortedArch in NewDict:
            NewDict[SortedArch] = NewDict[SortedArch] + [NewStateMent]
        else:
            NewDict[SortedArch] = [NewStateMent]

    return NewDict                
                