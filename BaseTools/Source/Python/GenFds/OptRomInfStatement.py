## @file
# process OptionROM generation from INF statement
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
import RuleSimpleFile
import RuleComplexFile
import Section
import OptionRom
import Common.GlobalData as GlobalData

from Common.DataType import *
from Common.String import *
from FfsInfStatement import FfsInfStatement
from GenFdsGlobalVariable import GenFdsGlobalVariable

## 
#
#
class OptRomInfStatement (FfsInfStatement):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        FfsInfStatement.__init__(self)
        self.OverrideAttribs = None

    ## __GetOptRomParams() method
    #
    #   Parse inf file to get option ROM related parameters
    #
    #   @param  self        The object pointer
    #
    def __GetOptRomParams(self):
        
        if self.OverrideAttribs == None:
            self.OverrideAttribs = OptionRom.OverrideAttribs()

        if self.OverrideAttribs.NeedCompress == None:
            self.OverrideAttribs.NeedCompress = self.OptRomDefs.get ('PCI_COMPRESS')
            if self.OverrideAttribs.NeedCompress is not None:
                if self.OverrideAttribs.NeedCompress.upper() not in ('TRUE', 'FALSE'):
                    GenFdsGlobalVariable.ErrorLogger( "Expected TRUE/FALSE for PCI_COMPRESS: %s" %self.InfFileName)
                self.OverrideAttribs.NeedCompress = \
                    self.OverrideAttribs.NeedCompress.upper() == 'TRUE'

        if self.OverrideAttribs.PciVendorId == None:
            self.OverrideAttribs.PciVendorId = self.OptRomDefs.get ('PCI_VENDOR_ID')
        
        if self.OverrideAttribs.PciClassCode == None:
            self.OverrideAttribs.PciClassCode = self.OptRomDefs.get ('PCI_CLASS_CODE')
            
        if self.OverrideAttribs.PciDeviceId == None:
            self.OverrideAttribs.PciDeviceId = self.OptRomDefs.get ('PCI_DEVICE_ID')
            
        if self.OverrideAttribs.PciRevision == None:
            self.OverrideAttribs.PciRevision = self.OptRomDefs.get ('PCI_REVISION')
        
#        InfObj = GenFdsGlobalVariable.WorkSpace.BuildObject[self.PathClassObj, self.CurrentArch]  
#        RecordList = InfObj._RawData[MODEL_META_DATA_HEADER, InfObj._Arch, InfObj._Platform]
#        for Record in RecordList:
#            Record = ReplaceMacros(Record, GlobalData.gEdkGlobal, False)
#            Name = Record[0]  
    ## GenFfs() method
    #
    #   Generate FFS
    #
    #   @param  self        The object pointer
    #   @retval string      Generated .efi file name
    #
    def GenFfs(self):
        #
        # Parse Inf file get Module related information
        #

        self.__InfParse__()
        self.__GetOptRomParams()
        #
        # Get the rule of how to generate Ffs file
        #
        Rule = self.__GetRule__()
        GenFdsGlobalVariable.VerboseLogger( "Packing binaries from inf file : %s" %self.InfFileName)
        #FileType = Ffs.Ffs.ModuleTypeToFileType[Rule.ModuleType]
        #
        # For the rule only has simpleFile
        #
        if isinstance (Rule, RuleSimpleFile.RuleSimpleFile) :
            EfiOutputList = self.__GenSimpleFileSection__(Rule)
            return EfiOutputList
        #
        # For Rule has ComplexFile
        #
        elif isinstance(Rule, RuleComplexFile.RuleComplexFile):
            EfiOutputList = self.__GenComplexFileSection__(Rule)
            return EfiOutputList

    ## __GenSimpleFileSection__() method
    #
    #   Get .efi files according to simple rule.
    #
    #   @param  self        The object pointer
    #   @param  Rule        The rule object used to generate section
    #   @retval string      File name of the generated section file
    #
    def __GenSimpleFileSection__(self, Rule):
        #
        # Prepare the parameter of GenSection
        #

        OutputFileList = []
        if Rule.FileName != None:
            GenSecInputFile = self.__ExtendMacro__(Rule.FileName)
            OutputFileList.append(GenSecInputFile)
        else:
            OutputFileList, IsSect = Section.Section.GetFileList(self, '', Rule.FileExtension)

        return OutputFileList


    ## __GenComplexFileSection__() method
    #
    #   Get .efi by sections in complex Rule
    #
    #   @param  self        The object pointer
    #   @param  Rule        The rule object used to generate section
    #   @retval string      File name of the generated section file
    #
    def __GenComplexFileSection__(self, Rule):

        OutputFileList = []
        for Sect in Rule.SectionList:
            if Sect.SectionType == 'PE32':
                if Sect.FileName != None:
                    GenSecInputFile = self.__ExtendMacro__(Sect.FileName)
                    OutputFileList.append(GenSecInputFile)
                else:
                    FileList, IsSect = Section.Section.GetFileList(self, '', Sect.FileExtension)
                    OutputFileList.extend(FileList)    
        
        return OutputFileList

    