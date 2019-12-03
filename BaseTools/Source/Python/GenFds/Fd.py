## @file
# process FD generation
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from . import Region
from . import Fv
import Common.LongFilePathOs as os
from io import BytesIO
import sys
from struct import *
from .GenFdsGlobalVariable import GenFdsGlobalVariable
from CommonDataClass.FdfClass import FDClassObject
from Common import EdkLogger
from Common.BuildToolError import *
from Common.Misc import SaveFileOnChange
from Common.DataType import BINARY_FILE_TYPE_FV

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
    def GenFd (self, Flag = False):
        if self.FdUiName.upper() + 'fd' in GenFdsGlobalVariable.ImageBinDict:
            return GenFdsGlobalVariable.ImageBinDict[self.FdUiName.upper() + 'fd']

        #
        # Print Information
        #
        FdFileName = os.path.join(GenFdsGlobalVariable.FvDir, self.FdUiName + '.fd')
        if not Flag:
            GenFdsGlobalVariable.InfLogger("\nFd File Name:%s (%s)" %(self.FdUiName, FdFileName))

        Offset = 0x00
        for item in self.BlockSizeList:
            Offset = Offset + item[0]  * item[1]
        if Offset != self.Size:
            EdkLogger.error("GenFds", GENFDS_ERROR, 'FD %s Size not consistent with block array' % self.FdUiName)
        GenFdsGlobalVariable.VerboseLogger('Following Fv will be add to Fd !!!')
        for FvObj in GenFdsGlobalVariable.FdfParser.Profile.FvDict:
            GenFdsGlobalVariable.VerboseLogger(FvObj)

        HasCapsuleRegion = False
        for RegionObj in self.RegionList:
            if RegionObj.RegionType == 'CAPSULE':
                HasCapsuleRegion = True
                break
        if HasCapsuleRegion:
            TempFdBuffer = BytesIO()
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
                    if not Flag:
                        GenFdsGlobalVariable.InfLogger('Padding region starting from offset 0x%X, with size 0x%X' %(PreviousRegionStart + PreviousRegionSize, RegionObj.Offset - (PreviousRegionStart + PreviousRegionSize)))
                    PadRegion = Region.Region()
                    PadRegion.Offset = PreviousRegionStart + PreviousRegionSize
                    PadRegion.Size = RegionObj.Offset - PadRegion.Offset
                    if not Flag:
                        PadRegion.AddToBuffer(TempFdBuffer, self.BaseAddress, self.BlockSizeList, self.ErasePolarity, GenFdsGlobalVariable.ImageBinDict, self.DefineVarDict)
                PreviousRegionStart = RegionObj.Offset
                PreviousRegionSize = RegionObj.Size
                #
                # Call each region's AddToBuffer function
                #
                if PreviousRegionSize > self.Size:
                    pass
                GenFdsGlobalVariable.VerboseLogger('Call each region\'s AddToBuffer function')
                RegionObj.AddToBuffer (TempFdBuffer, self.BaseAddress, self.BlockSizeList, self.ErasePolarity, GenFdsGlobalVariable.ImageBinDict, self.DefineVarDict)

        FdBuffer = BytesIO()
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
                if not Flag:
                    GenFdsGlobalVariable.InfLogger('Padding region starting from offset 0x%X, with size 0x%X' %(PreviousRegionStart + PreviousRegionSize, RegionObj.Offset - (PreviousRegionStart + PreviousRegionSize)))
                PadRegion = Region.Region()
                PadRegion.Offset = PreviousRegionStart + PreviousRegionSize
                PadRegion.Size = RegionObj.Offset - PadRegion.Offset
                if not Flag:
                    PadRegion.AddToBuffer(FdBuffer, self.BaseAddress, self.BlockSizeList, self.ErasePolarity, GenFdsGlobalVariable.ImageBinDict, self.DefineVarDict)
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
            RegionObj.AddToBuffer (FdBuffer, self.BaseAddress, self.BlockSizeList, self.ErasePolarity, GenFdsGlobalVariable.ImageBinDict, self.DefineVarDict, Flag=Flag)
        #
        # Write the buffer contents to Fd file
        #
        GenFdsGlobalVariable.VerboseLogger('Write the buffer contents to Fd file')
        if not Flag:
            SaveFileOnChange(FdFileName, FdBuffer.getvalue())
        FdBuffer.close()
        GenFdsGlobalVariable.ImageBinDict[self.FdUiName.upper() + 'fd'] = FdFileName
        return FdFileName

    ## generate flash map file
    #
    #   @param  self        The object pointer
    #
    def GenFlashMap (self):
        pass








