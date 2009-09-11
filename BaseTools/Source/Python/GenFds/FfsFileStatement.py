## @file
# process FFS generation from FILE statement
#
#  Copyright (c) 2007, Intel Corporation
#
#  All rights reserved. This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import Ffs
import Rule
from GenFdsGlobalVariable import GenFdsGlobalVariable
import os
import StringIO
import subprocess
from CommonDataClass.FdfClass import FileStatementClassObject
from Common import EdkLogger
from Common.BuildToolError import *
from Common.Misc import GuidStructureByteArrayToGuidString

## generate FFS from FILE
#
#
class FileStatement (FileStatementClassObject) :
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        FileStatementClassObject.__init__(self)

    ## GenFfs() method
    #
    #   Generate FFS
    #
    #   @param  self        The object pointer
    #   @param  Dict        dictionary contains macro and value pair
    #   @retval string      Generated FFS file name
    #
    def GenFfs(self, Dict = {}):
        
        if self.NameGuid != None and self.NameGuid.startswith('PCD('):
            PcdValue = GenFdsGlobalVariable.GetPcdValue(self.NameGuid)
            if len(PcdValue) == 0:
                EdkLogger.error("GenFds", GENFDS_ERROR, '%s NOT defined.' \
                            % (self.NameGuid))
            if PcdValue.startswith('{'):
                PcdValue = GuidStructureByteArrayToGuidString(PcdValue)
            RegistryGuidStr = PcdValue
            if len(RegistryGuidStr) == 0:
                EdkLogger.error("GenFds", GENFDS_ERROR, 'GUID value for %s in wrong format.' \
                            % (self.NameGuid))
            self.NameGuid = RegistryGuidStr
        
        OutputDir = os.path.join(GenFdsGlobalVariable.FfsDir, self.NameGuid)
        if not os.path.exists(OutputDir):
             os.makedirs(OutputDir)

        Dict.update(self.DefineVarDict)
        SectionAlignments = None
        if self.FvName != None :
            Buffer = StringIO.StringIO('')
            if self.FvName.upper() not in GenFdsGlobalVariable.FdfParser.Profile.FvDict.keys():
                EdkLogger.error("GenFds", GENFDS_ERROR, "FV (%s) is NOT described in FDF file!" % (self.FvName))
            Fv = GenFdsGlobalVariable.FdfParser.Profile.FvDict.get(self.FvName.upper())
            FileName = Fv.AddToBuffer(Buffer)
            SectionFiles = [FileName]

        elif self.FdName != None:
            if self.FdName.upper() not in GenFdsGlobalVariable.FdfParser.Profile.FdDict.keys():
                EdkLogger.error("GenFds", GENFDS_ERROR, "FD (%s) is NOT described in FDF file!" % (self.FdName))
            Fd = GenFdsGlobalVariable.FdfParser.Profile.FdDict.get(self.FdName.upper())
            FileName = Fd.GenFd()
            SectionFiles = [FileName]

        elif self.FileName != None:
            self.FileName = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.FileName)
            SectionFiles = [GenFdsGlobalVariable.MacroExtend(self.FileName, Dict)]

        else:
            SectionFiles = []
            Index = 0
            SectionAlignments = []
            for section in self.SectionList :
                Index = Index + 1
                SecIndex = '%d' %Index
                sectList, align = section.GenSection(OutputDir, self.NameGuid, SecIndex, self.KeyStringList, None, Dict)
                if sectList != []:
                    for sect in sectList:
                        SectionFiles.append(sect)
                        SectionAlignments.append(align)

        #
        # Prepare the parameter
        #
        FfsFileOutput = os.path.join(OutputDir, self.NameGuid + '.ffs')
        GenFdsGlobalVariable.GenerateFfs(FfsFileOutput, SectionFiles,
                                         Ffs.Ffs.FdfFvFileTypeToFileType.get(self.FvFileType),
                                         self.NameGuid,
                                         Fixed=self.Fixed,
                                         CheckSum=self.CheckSum,
                                         Align=self.Alignment,
                                         SectionAlign=SectionAlignments
                                        )

        return FfsFileOutput



