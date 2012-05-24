## @file
# process compress section generation
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
from Ffs import Ffs
import Section
import subprocess
import os
from GenFdsGlobalVariable import GenFdsGlobalVariable
from CommonDataClass.FdfClass import CompressSectionClassObject

## generate compress section
#
#
class CompressSection (CompressSectionClassObject) :

    ## compress types: PI standard and non PI standard
    CompTypeDict = {
        'PI_STD'  : 'PI_STD',
        'PI_NONE' : 'PI_NONE'
    }

    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        CompressSectionClassObject.__init__(self)

    ## GenSection() method
    #
    #   Generate compressed section
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

        if FfsInf != None:
            self.CompType = FfsInf.__ExtendMacro__(self.CompType)
            self.Alignment = FfsInf.__ExtendMacro__(self.Alignment)

        SectFiles = tuple()
        Index = 0
        for Sect in self.SectionList:
            Index = Index + 1
            SecIndex = '%s.%d' %(SecNum, Index)
            ReturnSectList, AlignValue = Sect.GenSection(OutputPath, ModuleName, SecIndex, KeyStringList, FfsInf, Dict)
            if ReturnSectList != []:
                for FileData in ReturnSectList:
                   SectFiles += (FileData,)


        OutputFile = OutputPath + \
                     os.sep     + \
                     ModuleName + \
                     'SEC'      + \
                     SecNum     + \
                     Ffs.SectionSuffix['COMPRESS']
        OutputFile = os.path.normpath(OutputFile)

        GenFdsGlobalVariable.GenerateSection(OutputFile, SectFiles, Section.Section.SectionType['COMPRESS'],
                                             CompressionType=self.CompTypeDict[self.CompType])
        OutputFileList = []
        OutputFileList.append(OutputFile)
        return OutputFileList, self.Alignment


