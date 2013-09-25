## @file
# generate capsule
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
from GenFdsGlobalVariable import GenFdsGlobalVariable
from CommonDataClass.FdfClass import CapsuleClassObject
import os
import subprocess
import StringIO
from Common.Misc import SaveFileOnChange
from GenFds import GenFds


T_CHAR_LF = '\n'

## create inf file describes what goes into capsule and call GenFv to generate capsule
#
#
class Capsule (CapsuleClassObject) :
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        CapsuleClassObject.__init__(self)
        # For GenFv
        self.BlockSize = None
        # For GenFv
        self.BlockNum = None
        self.CapsuleName = None

    ## Generate capsule
    #
    #   @param  self        The object pointer
    #   @retval string      Generated Capsule file path
    #
    def GenCapsule(self):
        if self.UiCapsuleName.upper() + 'cap' in GenFds.ImageBinDict.keys():
            return GenFds.ImageBinDict[self.UiCapsuleName.upper() + 'cap']

        GenFdsGlobalVariable.InfLogger( "\nGenerate %s Capsule" %self.UiCapsuleName)
        CapInfFile = self.GenCapInf()
        CapInfFile.writelines("[files]" + T_CHAR_LF)
        CapFileList = []
        for CapsuleDataObj in self.CapsuleDataList :
            CapsuleDataObj.CapsuleName = self.CapsuleName
            FileName = CapsuleDataObj.GenCapsuleSubItem()
            CapsuleDataObj.CapsuleName = None
            CapFileList.append(FileName)
            CapInfFile.writelines("EFI_FILE_NAME = " + \
                                   FileName      + \
                                   T_CHAR_LF)
        SaveFileOnChange(self.CapInfFileName, CapInfFile.getvalue(), False)
        CapInfFile.close()
        #
        # Call GenFv tool to generate capsule
        #
        CapOutputFile = os.path.join(GenFdsGlobalVariable.FvDir, self.UiCapsuleName)
        CapOutputFile = CapOutputFile + '.Cap'
        GenFdsGlobalVariable.GenerateFirmwareVolume(
                                CapOutputFile,
                                [self.CapInfFileName],
                                Capsule=True,
                                FfsList=CapFileList
                                )

        GenFdsGlobalVariable.VerboseLogger( "\nGenerate %s Capsule Successfully" %self.UiCapsuleName)
        GenFdsGlobalVariable.SharpCounter = 0
        GenFds.ImageBinDict[self.UiCapsuleName.upper() + 'cap'] = CapOutputFile
        return CapOutputFile

    ## Generate inf file for capsule
    #
    #   @param  self        The object pointer
    #   @retval file        inf file object
    #
    def GenCapInf(self):
        self.CapInfFileName = os.path.join(GenFdsGlobalVariable.FvDir,
                                   self.UiCapsuleName +  "_Cap" + '.inf')
        CapInfFile = StringIO.StringIO() #open (self.CapInfFileName , 'w+')

        CapInfFile.writelines("[options]" + T_CHAR_LF)

        for Item in self.TokensDict.keys():
            CapInfFile.writelines("EFI_"                    + \
                                  Item                      + \
                                  ' = '                     + \
                                  self.TokensDict.get(Item) + \
                                  T_CHAR_LF)

        return CapInfFile
