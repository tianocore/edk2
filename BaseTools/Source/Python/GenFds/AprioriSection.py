## @file
# process APRIORI file data and generate PEI/DXE APRIORI file
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from struct import pack
import Common.LongFilePathOs as os
from io import BytesIO
from .FfsFileStatement import FileStatement
from .GenFdsGlobalVariable import GenFdsGlobalVariable
from Common.StringUtils import NormPath
from Common.Misc import SaveFileOnChange, PathClass
from Common.EdkLogger import error as EdkLoggerError
from Common.BuildToolError import RESOURCE_NOT_AVAILABLE
from Common.DataType import TAB_COMMON

DXE_APRIORI_GUID = "FC510EE7-FFDC-11D4-BD41-0080C73C8881"
PEI_APRIORI_GUID = "1B45CC0A-156A-428A-AF62-49864DA0E6E6"

## process APRIORI file data and generate PEI/DXE APRIORI file
#
#
class AprioriSection (object):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        self.DefineVarDict = {}
        self.FfsList = []
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
    def GenFfs (self, FvName, Dict = {}, IsMakefile = False):
        Buffer = BytesIO()
        if self.AprioriType == "PEI":
            AprioriFileGuid = PEI_APRIORI_GUID
        else:
            AprioriFileGuid = DXE_APRIORI_GUID

        OutputAprFilePath = os.path.join (GenFdsGlobalVariable.WorkSpaceDir, \
                                   GenFdsGlobalVariable.FfsDir,\
                                   AprioriFileGuid + FvName)
        if not os.path.exists(OutputAprFilePath):
            os.makedirs(OutputAprFilePath)

        OutputAprFileName = os.path.join( OutputAprFilePath, \
                                       AprioriFileGuid + FvName + '.Apri' )
        AprFfsFileName = os.path.join (OutputAprFilePath,\
                                    AprioriFileGuid + FvName + '.Ffs')

        Dict.update(self.DefineVarDict)
        InfFileName = None
        for FfsObj in self.FfsList:
            Guid = ""
            if isinstance(FfsObj, FileStatement):
                Guid = FfsObj.NameGuid
            else:
                InfFileName = NormPath(FfsObj.InfFileName)
                Arch = FfsObj.GetCurrentArch()

                if Arch:
                    Dict['$(ARCH)'] = Arch
                InfFileName = GenFdsGlobalVariable.MacroExtend(InfFileName, Dict, Arch)

                if Arch:
                    Inf = GenFdsGlobalVariable.WorkSpace.BuildObject[PathClass(InfFileName, GenFdsGlobalVariable.WorkSpaceDir), Arch, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
                    Guid = Inf.Guid
                else:
                    Inf = GenFdsGlobalVariable.WorkSpace.BuildObject[PathClass(InfFileName, GenFdsGlobalVariable.WorkSpaceDir), TAB_COMMON, GenFdsGlobalVariable.TargetName, GenFdsGlobalVariable.ToolChainTag]
                    Guid = Inf.Guid

                    if not Inf.Module.Binaries:
                        EdkLoggerError("GenFds", RESOURCE_NOT_AVAILABLE,
                                        "INF %s not found in build ARCH %s!" \
                                        % (InfFileName, GenFdsGlobalVariable.ArchList))

            GuidPart = Guid.split('-')
            Buffer.write(pack('I', int(GuidPart[0], 16)))
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
        MakefilePath = None
        if IsMakefile:
            if not InfFileName:
                return None
            MakefilePath = InfFileName, Arch
        GenFdsGlobalVariable.GenerateSection(RawSectionFileName, [OutputAprFileName], 'EFI_SECTION_RAW', IsMakefile=IsMakefile)
        GenFdsGlobalVariable.GenerateFfs(AprFfsFileName, [RawSectionFileName],
                                        'EFI_FV_FILETYPE_FREEFORM', AprioriFileGuid, MakefilePath=MakefilePath)

        return AprFfsFileName
