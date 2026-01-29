## @file
# process OptionROM generation from INF statement
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from . import RuleSimpleFile
from . import RuleComplexFile
from . import Section
import Common.GlobalData as GlobalData

from Common.DataType import *
from Common.StringUtils import *
from .FfsInfStatement import FfsInfStatement
from .GenFdsGlobalVariable import GenFdsGlobalVariable

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
        if self.OverrideAttribs is None:
            self.OverrideAttribs = OverrideAttribs()

        if self.OverrideAttribs.NeedCompress is None:
            self.OverrideAttribs.NeedCompress = self.OptRomDefs.get ('PCI_COMPRESS')
            if self.OverrideAttribs.NeedCompress is not None:
                if self.OverrideAttribs.NeedCompress.upper() not in ('TRUE', 'FALSE'):
                    GenFdsGlobalVariable.ErrorLogger( "Expected TRUE/FALSE for PCI_COMPRESS: %s" %self.InfFileName)
                self.OverrideAttribs.NeedCompress = \
                    self.OverrideAttribs.NeedCompress.upper() == 'TRUE'

        if self.OverrideAttribs.PciVendorId is None:
            self.OverrideAttribs.PciVendorId = self.OptRomDefs.get ('PCI_VENDOR_ID')

        if self.OverrideAttribs.PciClassCode is None:
            self.OverrideAttribs.PciClassCode = self.OptRomDefs.get ('PCI_CLASS_CODE')

        if self.OverrideAttribs.PciDeviceId is None:
            self.OverrideAttribs.PciDeviceId = self.OptRomDefs.get ('PCI_DEVICE_ID')

        if self.OverrideAttribs.PciRevision is None:
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
    def GenFfs(self, IsMakefile=False):
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
        #
        # For the rule only has simpleFile
        #
        if isinstance (Rule, RuleSimpleFile.RuleSimpleFile) :
            EfiOutputList = self.__GenSimpleFileSection__(Rule, IsMakefile=IsMakefile)
            return EfiOutputList
        #
        # For Rule has ComplexFile
        #
        elif isinstance(Rule, RuleComplexFile.RuleComplexFile):
            EfiOutputList = self.__GenComplexFileSection__(Rule, IsMakefile=IsMakefile)
            return EfiOutputList

    ## __GenSimpleFileSection__() method
    #
    #   Get .efi files according to simple rule.
    #
    #   @param  self        The object pointer
    #   @param  Rule        The rule object used to generate section
    #   @retval string      File name of the generated section file
    #
    def __GenSimpleFileSection__(self, Rule, IsMakefile = False):
        #
        # Prepare the parameter of GenSection
        #

        OutputFileList = []
        if Rule.FileName is not None:
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
    def __GenComplexFileSection__(self, Rule, IsMakefile=False):

        OutputFileList = []
        for Sect in Rule.SectionList:
            if Sect.SectionType == BINARY_FILE_TYPE_PE32:
                if Sect.FileName is not None:
                    GenSecInputFile = self.__ExtendMacro__(Sect.FileName)
                    OutputFileList.append(GenSecInputFile)
                else:
                    FileList, IsSect = Section.Section.GetFileList(self, '', Sect.FileExtension)
                    OutputFileList.extend(FileList)

        return OutputFileList

class OverrideAttribs:

    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):

        self.PciVendorId = None
        self.PciClassCode = None
        self.PciDeviceId = None
        self.PciRevision = None
        self.NeedCompress = None
