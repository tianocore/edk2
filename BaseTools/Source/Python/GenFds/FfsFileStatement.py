## @file
# process FFS generation from FILE statement
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from io import BytesIO
from struct import pack
from CommonDataClass.FdfClass import FileStatementClassObject
from Common import EdkLogger
from Common.BuildToolError import GENFDS_ERROR
from Common.Misc import GuidStructureByteArrayToGuidString, SaveFileOnChange
import Common.LongFilePathOs as os
from .GuidSection import GuidSection
from .FvImageSection import FvImageSection
from .Ffs import FdfFvFileTypeToFileType
from .GenFdsGlobalVariable import GenFdsGlobalVariable

## generate FFS from FILE
#
#
class FileStatement (FileStatementClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        FileStatementClassObject.__init__(self)
        self.CurrentLineNum = None
        self.CurrentLineContent = None
        self.FileName = None
        self.InfFileName = None
        self.SubAlignment = None

    ## GenFfs() method
    #
    #   Generate FFS
    #
    #   @param  self         The object pointer
    #   @param  Dict         dictionary contains macro and value pair
    #   @param  FvChildAddr  Array of the inside FvImage base address
    #   @param  FvParentAddr Parent Fv base address
    #   @retval string       Generated FFS file name
    #
    def GenFfs(self, Dict = {}, FvChildAddr=[], FvParentAddr=None, IsMakefile=False, FvName=None):

        if self.NameGuid and self.NameGuid.startswith('PCD('):
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

        Str = self.NameGuid
        if FvName:
            Str += FvName
        OutputDir = os.path.join(GenFdsGlobalVariable.FfsDir, Str)
        if not os.path.exists(OutputDir):
            os.makedirs(OutputDir)

        Dict.update(self.DefineVarDict)
        SectionAlignments = None
        if self.FvName:
            Buffer = BytesIO()
            if self.FvName.upper() not in GenFdsGlobalVariable.FdfParser.Profile.FvDict:
                EdkLogger.error("GenFds", GENFDS_ERROR, "FV (%s) is NOT described in FDF file!" % (self.FvName))
            Fv = GenFdsGlobalVariable.FdfParser.Profile.FvDict.get(self.FvName.upper())
            FileName = Fv.AddToBuffer(Buffer)
            SectionFiles = [FileName]

        elif self.FdName:
            if self.FdName.upper() not in GenFdsGlobalVariable.FdfParser.Profile.FdDict:
                EdkLogger.error("GenFds", GENFDS_ERROR, "FD (%s) is NOT described in FDF file!" % (self.FdName))
            Fd = GenFdsGlobalVariable.FdfParser.Profile.FdDict.get(self.FdName.upper())
            FileName = Fd.GenFd()
            SectionFiles = [FileName]

        elif self.FileName:
            if hasattr(self, 'FvFileType') and self.FvFileType == 'RAW':
                if isinstance(self.FileName, list) and isinstance(self.SubAlignment, list) and len(self.FileName) == len(self.SubAlignment):
                    FileContent = BytesIO()
                    MaxAlignIndex = 0
                    MaxAlignValue = 1
                    for Index, File in enumerate(self.FileName):
                        try:
                            f = open(File, 'rb')
                        except:
                            GenFdsGlobalVariable.ErrorLogger("Error opening RAW file %s." % (File))
                        Content = f.read()
                        f.close()
                        AlignValue = 1
                        if self.SubAlignment[Index]:
                            AlignValue = GenFdsGlobalVariable.GetAlignment(self.SubAlignment[Index])
                        if AlignValue > MaxAlignValue:
                            MaxAlignIndex = Index
                            MaxAlignValue = AlignValue
                        FileContent.write(Content)
                        if len(FileContent.getvalue()) % AlignValue != 0:
                            Size = AlignValue - len(FileContent.getvalue()) % AlignValue
                            for i in range(0, Size):
                                FileContent.write(pack('B', 0xFF))

                    if FileContent.getvalue() != b'':
                        OutputRAWFile = os.path.join(GenFdsGlobalVariable.FfsDir, self.NameGuid, self.NameGuid + '.raw')
                        SaveFileOnChange(OutputRAWFile, FileContent.getvalue(), True)
                        self.FileName = OutputRAWFile
                        self.SubAlignment = self.SubAlignment[MaxAlignIndex]

                if self.Alignment and self.SubAlignment:
                    if GenFdsGlobalVariable.GetAlignment (self.Alignment) < GenFdsGlobalVariable.GetAlignment (self.SubAlignment):
                        self.Alignment = self.SubAlignment
                elif self.SubAlignment:
                    self.Alignment = self.SubAlignment

            self.FileName = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.FileName)
            #Replace $(SAPCE) with real space
            self.FileName = self.FileName.replace('$(SPACE)', ' ')
            SectionFiles = [GenFdsGlobalVariable.MacroExtend(self.FileName, Dict)]

        else:
            SectionFiles = []
            Index = 0
            SectionAlignments = []
            for section in self.SectionList:
                Index = Index + 1
                SecIndex = '%d' %Index
                # process the inside FvImage from FvSection or GuidSection
                if FvChildAddr != []:
                    if isinstance(section, FvImageSection):
                        section.FvAddr = FvChildAddr.pop(0)
                    elif isinstance(section, GuidSection):
                        section.FvAddr = FvChildAddr
                if FvParentAddr and isinstance(section, GuidSection):
                    section.FvParentAddr = FvParentAddr

                if self.KeepReloc == False:
                    section.KeepReloc = False
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
                                         FdfFvFileTypeToFileType.get(self.FvFileType),
                                         self.NameGuid,
                                         Fixed=self.Fixed,
                                         CheckSum=self.CheckSum,
                                         Align=self.Alignment,
                                         SectionAlign=SectionAlignments
                                        )

        return FfsFileOutput
