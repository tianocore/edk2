## @file
# process APRIORI file data and generate PEI/DXE APRIORI file
#
#  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
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
from struct import *
import os
import StringIO
import FfsFileStatement
from GenFdsGlobalVariable import GenFdsGlobalVariable
from CommonDataClass.FdfClass import AprioriSectionClassObject
from Common.String import *
from Common.Misc import SaveFileOnChange,PathClass
from Common import EdkLogger
from Common.BuildToolError import *

## process APRIORI file data and generate PEI/DXE APRIORI file
#
#
class AprioriSection (AprioriSectionClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        AprioriSectionClassObject.__init__(self)
        self.AprioriType = ""

    ## GenFfs() method
    #
    #   Generate FFS for APRIORI file
    #
    #   @param  self        The object pointer
    #   @param  FvName      for whom apriori file generated
    #   @param  Dict        dictionary contains macro and its value
    #   @retval string      Generated file name
    #
    def GenFfs (self, FvName, Dict = {}):
        DXE_GUID = "FC510EE7-FFDC-11D4-BD41-0080C73C8881"
        PEI_GUID = "1B45CC0A-156A-428A-AF62-49864DA0E6E6"
        Buffer = StringIO.StringIO('')
        AprioriFileGuid = DXE_GUID
        if self.AprioriType == "PEI":
            AprioriFileGuid = PEI_GUID
        OutputAprFilePath = os.path.join (GenFdsGlobalVariable.WorkSpaceDir, \
                                   GenFdsGlobalVariable.FfsDir,\
                                   AprioriFileGuid + FvName)
        if not os.path.exists(OutputAprFilePath) :
            os.makedirs(OutputAprFilePath)

        OutputAprFileName = os.path.join( OutputAprFilePath, \
                                       AprioriFileGuid + FvName + '.Apri' )
        AprFfsFileName = os.path.join (OutputAprFilePath,\
                                    AprioriFileGuid + FvName + '.Ffs')

        Dict.update(self.DefineVarDict)
        for FfsObj in self.FfsList :
            Guid = ""
            if isinstance(FfsObj, FfsFileStatement.FileStatement):
                Guid = FfsObj.NameGuid
            else:
                InfFileName = NormPath(FfsObj.InfFileName)
                Arch = FfsObj.GetCurrentArch()

                if Arch != None:
                    Dict['$(ARCH)'] = Arch
                InfFileName = GenFdsGlobalVariable.MacroExtend(InfFileName, Dict, Arch)

                if Arch != None:
                    Inf = GenFdsGlobalVariable.WorkSpace.BuildObject[PathClass(InfFileName, GenFdsGlobalVariable.WorkSpaceDir), Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
                    Guid = Inf.Guid

                else:
                    Inf = GenFdsGlobalVariable.WorkSpace.BuildObject[PathClass(InfFileName, GenFdsGlobalVariable.WorkSpaceDir), 'COMMON', GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
                    Guid = Inf.Guid

                    self.BinFileList = Inf.Module.Binaries
                    if self.BinFileList == []:
                        EdkLogger.error("GenFds", RESOURCE_NOT_AVAILABLE,
                                        "INF %s not found in build ARCH %s!" \
                                        % (InfFileName, GenFdsGlobalVariable.ArchList))


            GuidPart = Guid.split('-')
            Buffer.write(pack('I', long(GuidPart[0], 16)))
            Buffer.write(pack('H', int(GuidPart[1], 16)))
            Buffer.write(pack('H', int(GuidPart[2], 16)))

            for Num in range(2):
                Char = GuidPart[3][Num*2:Num*2+2]
                Buffer.write(pack('B', int(Char, 16)))

            for Num in range(6):
                Char = GuidPart[4][Num*2:Num*2+2]
                Buffer.write(pack('B', int(Char, 16)))

        SaveFileOnChange(OutputAprFileName, Buffer.getvalue())

        RawSectionFileName = os.path.join( OutputAprFilePath, \
                                       AprioriFileGuid + FvName + '.raw' )
        GenFdsGlobalVariable.GenerateSection(RawSectionFileName, [OutputAprFileName], 'EFI_SECTION_RAW')
        GenFdsGlobalVariable.GenerateFfs(AprFfsFileName, [RawSectionFileName],
                                         'EFI_FV_FILETYPE_FREEFORM', AprioriFileGuid)

        return AprFfsFileName

