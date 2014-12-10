## @file
# process GUIDed section generation
#
#  Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
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
import Section
import subprocess
from Ffs import Ffs
import Common.LongFilePathOs as os
from GenFdsGlobalVariable import GenFdsGlobalVariable
from CommonDataClass.FdfClass import GuidSectionClassObject
from Common import ToolDefClassObject
import sys
from Common import EdkLogger
from Common.BuildToolError import *
from FvImageSection import FvImageSection
from Common.LongFilePathSupport import OpenLongFilePath as open

## generate GUIDed section
#
#
class GuidSection(GuidSectionClassObject) :

    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        GuidSectionClassObject.__init__(self)

    ## GenSection() method
    #
    #   Generate GUIDed section
    #
    #   @param  self        The object pointer
    #   @param  OutputPath  Where to place output file
    #   @param  ModuleName  Which module this section belongs to
    #   @param  SecNum      Index of section
    #   @param  KeyStringList  Filter for inputs of section generation
    #   @param  FfsInf      FfsInfStatement object that contains this section data
    #   @param  Dict        dictionary contains macro and its value
    #   @retval tuple       (Generated file name, section alignment)
    #
    def GenSection(self, OutputPath, ModuleName, SecNum, KeyStringList, FfsInf = None, Dict = {}):
        #
        # Generate all section
        #
        self.KeyStringList = KeyStringList
        self.CurrentArchList = GenFdsGlobalVariable.ArchList
        if FfsInf != None:
            self.Alignment = FfsInf.__ExtendMacro__(self.Alignment)
            self.NameGuid = FfsInf.__ExtendMacro__(self.NameGuid)
            self.SectionType = FfsInf.__ExtendMacro__(self.SectionType)
            self.CurrentArchList = [FfsInf.CurrentArch]

        SectFile  = tuple()
        SectAlign = []
        Index = 0
        MaxAlign = None
        if self.FvAddr != []:
            FvAddrIsSet = True
        else:
            FvAddrIsSet = False
        
        if self.ProcessRequired in ("TRUE", "1"):
            if self.FvAddr != []:
                #no use FvAddr when the image is processed.
                self.FvAddr = []
            if self.FvParentAddr != None:
                #no use Parent Addr when the image is processed.
                self.FvParentAddr = None

        for Sect in self.SectionList:
            Index = Index + 1
            SecIndex = '%s.%d' %(SecNum,Index)
            # set base address for inside FvImage
            if isinstance(Sect, FvImageSection):
                if self.FvAddr != []:
                    Sect.FvAddr = self.FvAddr.pop(0)
                self.IncludeFvSection = True
            elif isinstance(Sect, GuidSection):
                Sect.FvAddr = self.FvAddr
                Sect.FvParentAddr = self.FvParentAddr
            ReturnSectList, align = Sect.GenSection(OutputPath, ModuleName, SecIndex, KeyStringList,FfsInf, Dict)
            if isinstance(Sect, GuidSection):
                if Sect.IncludeFvSection:
                    self.IncludeFvSection = Sect.IncludeFvSection

            if align != None:
                if MaxAlign == None:
                    MaxAlign = align
                if GenFdsGlobalVariable.GetAlignment (align) > GenFdsGlobalVariable.GetAlignment (MaxAlign):
                    MaxAlign = align
            if ReturnSectList != []:
                if align == None:
                    align = "1"
                for file in ReturnSectList:
                    SectFile += (file,)
                    SectAlign.append(align)

        if MaxAlign != None:
            if self.Alignment == None:
                self.Alignment = MaxAlign
            else:
                if GenFdsGlobalVariable.GetAlignment (MaxAlign) > GenFdsGlobalVariable.GetAlignment (self.Alignment):
                    self.Alignment = MaxAlign

        OutputFile = OutputPath + \
                     os.sep     + \
                     ModuleName + \
                     'SEC'      + \
                     SecNum     + \
                     Ffs.SectionSuffix['GUIDED']
        OutputFile = os.path.normpath(OutputFile)

        ExternalTool = None
        ExternalOption = None
        if self.NameGuid != None:
            ExternalTool, ExternalOption = self.__FindExtendTool__()

        #
        # If not have GUID , call default
        # GENCRC32 section
        #
        if self.NameGuid == None :
            GenFdsGlobalVariable.VerboseLogger( "Use GenSection function Generate CRC32 Section")
            GenFdsGlobalVariable.GenerateSection(OutputFile, SectFile, Section.Section.SectionType[self.SectionType], InputAlign=SectAlign)
            OutputFileList = []
            OutputFileList.append(OutputFile)
            return OutputFileList, self.Alignment
        #or GUID not in External Tool List
        elif ExternalTool == None:
            EdkLogger.error("GenFds", GENFDS_ERROR, "No tool found with GUID %s" % self.NameGuid)
        else:
            DummyFile = OutputFile+".dummy"
            #
            # Call GenSection with DUMMY section type.
            #
            GenFdsGlobalVariable.GenerateSection(DummyFile, SectFile, InputAlign=SectAlign)
            #
            # Use external tool process the Output
            #
            TempFile = OutputPath + \
                       os.sep     + \
                       ModuleName + \
                       'SEC'      + \
                       SecNum     + \
                       '.tmp'
            TempFile = os.path.normpath(TempFile)
            #
            # Remove temp file if its time stamp is older than dummy file
            # Just in case the external tool fails at this time but succeeded before
            # Error should be reported if the external tool does not generate a new output based on new input
            #
            if os.path.exists(TempFile) and os.path.exists(DummyFile) and os.path.getmtime(TempFile) < os.path.getmtime(DummyFile):
                os.remove(TempFile)

            FirstCall = False
            CmdOption = '-e'
            if ExternalOption != None:
                CmdOption = CmdOption + ' ' + ExternalOption
            if self.ProcessRequired not in ("TRUE", "1") and self.IncludeFvSection and not FvAddrIsSet and self.FvParentAddr != None:
                #FirstCall is only set for the encapsulated flash FV image without process required attribute.
                FirstCall = True
            #
            # Call external tool
            #
            ReturnValue = [1]
            if FirstCall:
                #first try to call the guided tool with -z option and CmdOption for the no process required guided tool.
                GenFdsGlobalVariable.GuidTool(TempFile, [DummyFile], ExternalTool, '-z' + ' ' + CmdOption, ReturnValue)

            #
            # when no call or first call failed, ReturnValue are not 1.
            # Call the guided tool with CmdOption
            #
            if ReturnValue[0] != 0:
                FirstCall = False
                ReturnValue[0] = 0
                GenFdsGlobalVariable.GuidTool(TempFile, [DummyFile], ExternalTool, CmdOption)
            #
            # There is external tool which does not follow standard rule which return nonzero if tool fails
            # The output file has to be checked
            #
            if not os.path.exists(TempFile):
                EdkLogger.error("GenFds", COMMAND_FAILURE, 'Fail to call %s, no output file was generated' % ExternalTool)

            FileHandleIn = open(DummyFile,'rb')
            FileHandleIn.seek(0,2)
            InputFileSize = FileHandleIn.tell()
            
            FileHandleOut = open(TempFile,'rb')
            FileHandleOut.seek(0,2)
            TempFileSize = FileHandleOut.tell()

            Attribute = []
            HeaderLength = None
            if self.ExtraHeaderSize != -1:
                HeaderLength = str(self.ExtraHeaderSize)

            if self.ProcessRequired == "NONE" and HeaderLength == None:
                if TempFileSize > InputFileSize:
                    FileHandleIn.seek(0)
                    BufferIn  = FileHandleIn.read()
                    FileHandleOut.seek(0)
                    BufferOut = FileHandleOut.read()
                    if BufferIn == BufferOut[TempFileSize - InputFileSize:]:
                        HeaderLength = str(TempFileSize - InputFileSize)
                #auto sec guided attribute with process required
                if HeaderLength == None:
                    Attribute.append('PROCESSING_REQUIRED')

            FileHandleIn.close()
            FileHandleOut.close()
            
            if FirstCall and 'PROCESSING_REQUIRED' in Attribute:
                # Guided data by -z option on first call is the process required data. Call the guided tool with the real option.
                GenFdsGlobalVariable.GuidTool(TempFile, [DummyFile], ExternalTool, CmdOption)
            
            #
            # Call Gensection Add Section Header
            #
            if self.ProcessRequired in ("TRUE", "1"):
                if 'PROCESSING_REQUIRED' not in Attribute:
                    Attribute.append('PROCESSING_REQUIRED')
  
            if self.AuthStatusValid in ("TRUE", "1"):
                Attribute.append('AUTH_STATUS_VALID')
            GenFdsGlobalVariable.GenerateSection(OutputFile, [TempFile], Section.Section.SectionType['GUIDED'],
                                                 Guid=self.NameGuid, GuidAttr=Attribute, GuidHdrLen=HeaderLength)
            OutputFileList = []
            OutputFileList.append(OutputFile)
            if 'PROCESSING_REQUIRED' in Attribute:
                # reset guided section alignment to none for the processed required guided data
                self.Alignment = None
                self.IncludeFvSection = False
                self.ProcessRequired = "TRUE"
            return OutputFileList, self.Alignment

    ## __FindExtendTool()
    #
    #    Find location of tools to process section data
    #
    #   @param  self        The object pointer
    #
    def __FindExtendTool__(self):
        # if user not specify filter, try to deduce it from global data.
        if self.KeyStringList == None or self.KeyStringList == []:
            Target = GenFdsGlobalVariable.TargetName
            ToolChain = GenFdsGlobalVariable.ToolChainTag
            ToolDb = ToolDefClassObject.ToolDefDict(GenFdsGlobalVariable.ConfDir).ToolsDefTxtDatabase
            if ToolChain not in ToolDb['TOOL_CHAIN_TAG']:
                EdkLogger.error("GenFds", GENFDS_ERROR, "Can not find external tool because tool tag %s is not defined in tools_def.txt!" % ToolChain)
            self.KeyStringList = [Target+'_'+ToolChain+'_'+self.CurrentArchList[0]]
            for Arch in self.CurrentArchList:
                if Target + '_' + ToolChain + '_' + Arch not in self.KeyStringList:
                    self.KeyStringList.append(Target + '_' + ToolChain + '_' + Arch)

        ToolDefinition = ToolDefClassObject.ToolDefDict(GenFdsGlobalVariable.ConfDir).ToolsDefTxtDictionary
        ToolPathTmp = None
        ToolOption = None
        for ToolDef in ToolDefinition.items():
            if self.NameGuid == ToolDef[1]:
                KeyList = ToolDef[0].split('_')
                Key = KeyList[0] + \
                      '_'        + \
                      KeyList[1] + \
                      '_'        + \
                      KeyList[2]
                if Key in self.KeyStringList and KeyList[4] == 'GUID':

                    ToolPath = ToolDefinition.get( Key        + \
                                                   '_'        + \
                                                   KeyList[3] + \
                                                   '_'        + \
                                                   'PATH')

                    ToolOption = ToolDefinition.get( Key        + \
                                                    '_'        + \
                                                    KeyList[3] + \
                                                    '_'        + \
                                                    'FLAGS')
                    if ToolPathTmp == None:
                        ToolPathTmp = ToolPath
                    else:
                        if ToolPathTmp != ToolPath:
                            EdkLogger.error("GenFds", GENFDS_ERROR, "Don't know which tool to use, %s or %s ?" % (ToolPathTmp, ToolPath))
                            
                    
        return ToolPathTmp, ToolOption



