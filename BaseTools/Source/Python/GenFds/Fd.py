## @file
# process FD generation
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
import Region
import Fv
import Common.LongFilePathOs as os
import StringIO
import sys
from struct import *
from GenFdsGlobalVariable import GenFdsGlobalVariable
from CommonDataClass.FdfClass import FDClassObject
from Common import EdkLogger
from Common.BuildToolError import *
from Common.Misc import SaveFileOnChange
from GenFds import GenFds

## generate FD
#
#
class FD(FDClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        FDClassObject.__init__(self)

    ## GenFd() method
    #
    #   Generate FD
    #
    #   @retval string      Generated FD file name
    #
    def GenFd (self):
        if self.FdUiName.upper() + 'fd' in GenFds.ImageBinDict.keys():
            return GenFds.ImageBinDict[self.FdUiName.upper() + 'fd']

        #
        # Print Information
        #
        GenFdsGlobalVariable.InfLogger("Fd File Name:%s" %self.FdUiName)
        Offset = 0x00
        for item in self.BlockSizeList:
            Offset = Offset + item[0]  * item[1]
        if Offset != self.Size:
            EdkLogger.error("GenFds", GENFDS_ERROR, 'FD %s Size not consistent with block array' % self.FdUiName)
        GenFdsGlobalVariable.VerboseLogger('Following Fv will be add to Fd !!!')
        for FvObj in GenFdsGlobalVariable.FdfParser.Profile.FvDict:
            GenFdsGlobalVariable.VerboseLogger(FvObj)

        GenFdsGlobalVariable.VerboseLogger('################### Gen VTF ####################')
        self.GenVtfFile()

        TempFdBuffer = StringIO.StringIO('')
        PreviousRegionStart = -1
        PreviousRegionSize = 1
        
        for RegionObj in self.RegionList :
            if RegionObj.RegionType == 'CAPSULE':
                continue
            if RegionObj.Offset + RegionObj.Size <= PreviousRegionStart:
                pass
            elif RegionObj.Offset <= PreviousRegionStart or (RegionObj.Offset >=PreviousRegionStart and RegionObj.Offset < PreviousRegionStart + PreviousRegionSize):
                pass
            elif RegionObj.Offset > PreviousRegionStart + PreviousRegionSize:
                GenFdsGlobalVariable.InfLogger('Padding region starting from offset 0x%X, with size 0x%X' %(PreviousRegionStart + PreviousRegionSize, RegionObj.Offset - (PreviousRegionStart + PreviousRegionSize)))
                PadRegion = Region.Region()
                PadRegion.Offset = PreviousRegionStart + PreviousRegionSize
                PadRegion.Size = RegionObj.Offset - PadRegion.Offset
                PadRegion.AddToBuffer(TempFdBuffer, self.BaseAddress, self.BlockSizeList, self.ErasePolarity, GenFds.ImageBinDict, self.vtfRawDict, self.DefineVarDict)
            PreviousRegionStart = RegionObj.Offset
            PreviousRegionSize = RegionObj.Size
            #
            # Call each region's AddToBuffer function
            #
            if PreviousRegionSize > self.Size:
                pass
            GenFdsGlobalVariable.VerboseLogger('Call each region\'s AddToBuffer function')
            RegionObj.AddToBuffer (TempFdBuffer, self.BaseAddress, self.BlockSizeList, self.ErasePolarity, GenFds.ImageBinDict, self.vtfRawDict, self.DefineVarDict)
        
        FdBuffer = StringIO.StringIO('')
        PreviousRegionStart = -1
        PreviousRegionSize = 1
        for RegionObj in self.RegionList :
            if RegionObj.Offset + RegionObj.Size <= PreviousRegionStart:
                EdkLogger.error("GenFds", GENFDS_ERROR,
                                'Region offset 0x%X in wrong order with Region starting from 0x%X, size 0x%X\nRegions in FDF must have offsets appear in ascending order.'\
                                % (RegionObj.Offset, PreviousRegionStart, PreviousRegionSize))
            elif RegionObj.Offset <= PreviousRegionStart or (RegionObj.Offset >=PreviousRegionStart and RegionObj.Offset < PreviousRegionStart + PreviousRegionSize):
                EdkLogger.error("GenFds", GENFDS_ERROR,
                                'Region offset 0x%X overlaps with Region starting from 0x%X, size 0x%X' \
                                % (RegionObj.Offset, PreviousRegionStart, PreviousRegionSize))
            elif RegionObj.Offset > PreviousRegionStart + PreviousRegionSize:
                GenFdsGlobalVariable.InfLogger('Padding region starting from offset 0x%X, with size 0x%X' %(PreviousRegionStart + PreviousRegionSize, RegionObj.Offset - (PreviousRegionStart + PreviousRegionSize)))
                PadRegion = Region.Region()
                PadRegion.Offset = PreviousRegionStart + PreviousRegionSize
                PadRegion.Size = RegionObj.Offset - PadRegion.Offset
                PadRegion.AddToBuffer(FdBuffer, self.BaseAddress, self.BlockSizeList, self.ErasePolarity, GenFds.ImageBinDict, self.vtfRawDict, self.DefineVarDict)
            PreviousRegionStart = RegionObj.Offset
            PreviousRegionSize = RegionObj.Size
            #
            # Verify current region fits within allocated FD section Size
            #
            if PreviousRegionStart + PreviousRegionSize > self.Size:
                EdkLogger.error("GenFds", GENFDS_ERROR,
                                'FD %s size too small to fit region with offset 0x%X and size 0x%X'
                                % (self.FdUiName, PreviousRegionStart, PreviousRegionSize))
            #
            # Call each region's AddToBuffer function
            #
            GenFdsGlobalVariable.VerboseLogger('Call each region\'s AddToBuffer function')
            RegionObj.AddToBuffer (FdBuffer, self.BaseAddress, self.BlockSizeList, self.ErasePolarity, GenFds.ImageBinDict, self.vtfRawDict, self.DefineVarDict)
        #
        # Create a empty Fd file
        #
        GenFdsGlobalVariable.VerboseLogger ('Create an empty Fd file')
        FdFileName = os.path.join(GenFdsGlobalVariable.FvDir,self.FdUiName + '.fd')
        #
        # Write the buffer contents to Fd file
        #
        GenFdsGlobalVariable.VerboseLogger('Write the buffer contents to Fd file')
        SaveFileOnChange(FdFileName, FdBuffer.getvalue())
        FdBuffer.close();
        GenFds.ImageBinDict[self.FdUiName.upper() + 'fd'] = FdFileName
        return FdFileName

    ## generate VTF
    #
    #   @param  self        The object pointer
    #
    def GenVtfFile (self) :
        #
        # Get this Fd's all Fv name
        #
        FvAddDict ={}
        FvList = []
        for RegionObj in self.RegionList:
            if RegionObj.RegionType == 'FV':
                if len(RegionObj.RegionDataList) == 1:
                    RegionData = RegionObj.RegionDataList[0]
                    FvList.append(RegionData.upper())
                    FvAddDict[RegionData.upper()] = (int(self.BaseAddress,16) + \
                                                RegionObj.Offset, RegionObj.Size)
                else:
                    Offset = RegionObj.Offset
                    for RegionData in RegionObj.RegionDataList:
                        FvList.append(RegionData.upper())
                        FvObj = GenFdsGlobalVariable.FdfParser.Profile.FvDict.get(RegionData.upper())
                        if len(FvObj.BlockSizeList) < 1:
                            EdkLogger.error("GenFds", GENFDS_ERROR,
                                            'FV.%s must point out FVs blocksize and Fv BlockNum' \
                                            % FvObj.UiFvName)
                        else:
                            Size = 0
                            for blockStatement in FvObj.BlockSizeList:
                                Size = Size + blockStatement[0] * blockStatement[1]
                            FvAddDict[RegionData.upper()] = (int(self.BaseAddress,16) + \
                                                             Offset, Size)
                            Offset = Offset + Size
        #
        # Check whether this Fd need VTF
        #
        Flag = False
        for VtfObj in GenFdsGlobalVariable.FdfParser.Profile.VtfList:
            compLocList = VtfObj.GetFvList()
            if set(compLocList).issubset(FvList):
                Flag = True
                break
        if Flag == True:
            self.vtfRawDict = VtfObj.GenVtf(FvAddDict)

    ## generate flash map file
    #
    #   @param  self        The object pointer
    #
    def GenFlashMap (self):
        pass








