## @file
#  This file include GenVpd class for fix the Vpd type PCD offset, and PcdEntry for describe
#  and process each entry of vpd type PCD.
#
#  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import absolute_import
import Common.LongFilePathOs as os
from io import BytesIO
from . import StringTable as st
import array
import re
from Common.LongFilePathSupport import OpenLongFilePath as open
from struct import *
from Common.DataType import MAX_SIZE_TYPE, MAX_VAL_TYPE, TAB_STAR
import Common.EdkLogger as EdkLogger
import Common.BuildToolError as BuildToolError

_FORMAT_CHAR = {1: 'B',
                2: 'H',
                4: 'I',
                8: 'Q'
                }

## The VPD PCD data structure for store and process each VPD PCD entry.
#
#  This class contain method to format and pack pcd's value.
#
class PcdEntry:
    def __init__(self, PcdCName, SkuId,PcdOffset, PcdSize, PcdValue, Lineno=None, FileName=None, PcdUnpackValue=None,
                 PcdBinOffset=None, PcdBinSize=None, Alignment=None):
        self.PcdCName       = PcdCName.strip()
        self.SkuId          = SkuId.strip()
        self.PcdOffset      = PcdOffset.strip()
        self.PcdSize        = PcdSize.strip()
        self.PcdValue       = PcdValue.strip()
        self.Lineno         = Lineno.strip()
        self.FileName       = FileName.strip()
        self.PcdUnpackValue = PcdUnpackValue
        self.PcdBinOffset   = PcdBinOffset
        self.PcdBinSize     = PcdBinSize
        self.Alignment       = Alignment

        if self.PcdValue == '' :
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                            "Invalid PCD format(Name: %s File: %s line: %s) , no Value specified!" % (self.PcdCName, self.FileName, self.Lineno))

        if self.PcdOffset == '' :
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                            "Invalid PCD format(Name: %s File: %s Line: %s) , no Offset specified!" % (self.PcdCName, self.FileName, self.Lineno))

        if self.PcdSize == '' :
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                            "Invalid PCD format(Name: %s File: %s Line: %s), no PcdSize specified!" % (self.PcdCName, self.FileName, self.Lineno))

        self._GenOffsetValue ()

    ## Analyze the string value to judge the PCD's datum type equal to Boolean or not.
    #
    #  @param   ValueString      PCD's value
    #  @param   Size             PCD's size
    #
    #  @retval  True   PCD's datum type is Boolean
    #  @retval  False  PCD's datum type is not Boolean.
    #
    def _IsBoolean(self, ValueString, Size):
        if (Size == "1"):
            if ValueString.upper() in ["TRUE", "FALSE"]:
                return True
            elif ValueString in ["0", "1", "0x0", "0x1", "0x00", "0x01"]:
                return True

        return False

    ## Convert the PCD's value from string to integer.
    #
    #  This function will try to convert the Offset value form string to integer
    #  for both hexadecimal and decimal.
    #
    def _GenOffsetValue(self):
        if self.PcdOffset != TAB_STAR:
            try:
                self.PcdBinOffset = int (self.PcdOffset)
            except:
                try:
                    self.PcdBinOffset = int(self.PcdOffset, 16)
                except:
                    EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                                    "Invalid offset value %s for PCD %s (File: %s Line: %s)" % (self.PcdOffset, self.PcdCName, self.FileName, self.Lineno))

    ## Pack Boolean type VPD PCD's value form string to binary type.
    #
    #  @param ValueString     The boolean type string for pack.
    #
    #
    def _PackBooleanValue(self, ValueString):
        if ValueString.upper() == "TRUE" or ValueString in ["1", "0x1", "0x01"]:
            try:
                self.PcdValue = pack(_FORMAT_CHAR[1], 1)
            except:
                EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                                "Invalid size or value for PCD %s to pack(File: %s Line: %s)." % (self.PcdCName, self.FileName, self.Lineno))
        else:
            try:
                self.PcdValue = pack(_FORMAT_CHAR[1], 0)
            except:
                EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                                "Invalid size or value for PCD %s to pack(File: %s Line: %s)." % (self.PcdCName, self.FileName, self.Lineno))

    ## Pack Integer type VPD PCD's value form string to binary type.
    #
    #  @param ValueString     The Integer type string for pack.
    #
    #
    def _PackIntValue(self, IntValue, Size):
        if Size not in _FORMAT_CHAR:
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                            "Invalid size %d for PCD %s in integer datum size(File: %s Line: %s)." % (Size, self.PcdCName, self.FileName, self.Lineno))

        for Type, MaxSize in MAX_SIZE_TYPE.items():
            if Type == 'BOOLEAN':
                continue
            if Size == MaxSize:
                if IntValue < 0:
                    EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                                    "PCD can't be set to negative value %d for PCD %s in %s datum type(File: %s Line: %s)." % (
                                    IntValue, self.PcdCName, Type, self.FileName, self.Lineno))
                elif IntValue > MAX_VAL_TYPE[Type]:
                    EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                                    "Too large PCD value %d for datum type %s for PCD %s(File: %s Line: %s)." % (
                                    IntValue, Type, self.PcdCName, self.FileName, self.Lineno))

        try:
            self.PcdValue = pack(_FORMAT_CHAR[Size], IntValue)
        except:
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                            "Invalid size or value for PCD %s to pack(File: %s Line: %s)." % (self.PcdCName, self.FileName, self.Lineno))

    ## Pack VOID* type VPD PCD's value form string to binary type.
    #
    #  The VOID* type of string divided into 3 sub-type:
    #    1:    L"String"/L'String', Unicode type string.
    #    2:    "String"/'String',  Ascii type string.
    #    3:    {bytearray}, only support byte-array.
    #
    #  @param ValueString     The Integer type string for pack.
    #
    def _PackPtrValue(self, ValueString, Size):
        if ValueString.startswith('L"') or ValueString.startswith("L'"):
            self._PackUnicode(ValueString, Size)
        elif ValueString.startswith('{') and ValueString.endswith('}'):
            self._PackByteArray(ValueString, Size)
        elif (ValueString.startswith('"') and ValueString.endswith('"')) or (ValueString.startswith("'") and ValueString.endswith("'")):
            self._PackString(ValueString, Size)
        else:
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                            "Invalid VOID* type PCD %s value %s (File: %s Line: %s)" % (self.PcdCName, ValueString, self.FileName, self.Lineno))

    ## Pack an Ascii PCD value.
    #
    #  An Ascii string for a PCD should be in format as  ""/''.
    #
    def _PackString(self, ValueString, Size):
        if (Size < 0):
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                            "Invalid parameter Size %s of PCD %s!(File: %s Line: %s)" % (self.PcdBinSize, self.PcdCName, self.FileName, self.Lineno))
        if (ValueString == ""):
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID, "Invalid parameter ValueString %s of PCD %s!(File: %s Line: %s)" % (self.PcdUnpackValue, self.PcdCName, self.FileName, self.Lineno))

        QuotedFlag = True
        if ValueString.startswith("'"):
            QuotedFlag = False

        ValueString = ValueString[1:-1]
        # No null-terminator in 'string'
        if (QuotedFlag and len(ValueString) + 1 > Size) or (not QuotedFlag and len(ValueString) > Size):
            EdkLogger.error("BPDG", BuildToolError.RESOURCE_OVERFLOW,
                            "PCD value string %s is exceed to size %d(File: %s Line: %s)" % (ValueString, Size, self.FileName, self.Lineno))
        try:
            self.PcdValue = pack('%ds' % Size, ValueString.encode('utf-8'))
        except:
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                            "Invalid size or value for PCD %s to pack(File: %s Line: %s)." % (self.PcdCName, self.FileName, self.Lineno))

    ## Pack a byte-array PCD value.
    #
    #  A byte-array for a PCD should be in format as  {0x01, 0x02, ...}.
    #
    def _PackByteArray(self, ValueString, Size):
        if (Size < 0):
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID, "Invalid parameter Size %s of PCD %s!(File: %s Line: %s)" % (self.PcdBinSize, self.PcdCName, self.FileName, self.Lineno))
        if (ValueString == ""):
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID, "Invalid parameter ValueString %s of PCD %s!(File: %s Line: %s)" % (self.PcdUnpackValue, self.PcdCName, self.FileName, self.Lineno))

        ValueString = ValueString.strip()
        ValueString = ValueString.lstrip('{').strip('}')
        ValueList = ValueString.split(',')
        ValueList = [item.strip() for item in ValueList]

        if len(ValueList) > Size:
            EdkLogger.error("BPDG", BuildToolError.RESOURCE_OVERFLOW,
                            "The byte array %s is too large for size %d(File: %s Line: %s)" % (ValueString, Size, self.FileName, self.Lineno))

        ReturnArray = array.array('B')

        for Index in range(len(ValueList)):
            Value = None
            if ValueList[Index].lower().startswith('0x'):
                # translate hex value
                try:
                    Value = int(ValueList[Index], 16)
                except:
                    EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                                    "The value item %s in byte array %s is an invalid HEX value.(File: %s Line: %s)" % \
                                    (ValueList[Index], ValueString, self.FileName, self.Lineno))
            else:
                # translate decimal value
                try:
                    Value = int(ValueList[Index], 10)
                except:
                    EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                                    "The value item %s in byte array %s is an invalid DECIMAL value.(File: %s Line: %s)" % \
                                    (ValueList[Index], ValueString, self.FileName, self.Lineno))

            if Value > 255:
                EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                                "The value item %s in byte array %s do not in range 0 ~ 0xFF(File: %s Line: %s)" % \
                                (ValueList[Index], ValueString, self.FileName, self.Lineno))

            ReturnArray.append(Value)

        for Index in range(len(ValueList), Size):
            ReturnArray.append(0)

        self.PcdValue = ReturnArray.tolist()

    ## Pack a unicode PCD value into byte array.
    #
    #  A unicode string for a PCD should be in format as  L""/L''.
    #
    def _PackUnicode(self, UnicodeString, Size):
        if (Size < 0):
            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID, "Invalid parameter Size %s of PCD %s!(File: %s Line: %s)" % \
                             (self.PcdBinSize, self.PcdCName, self.FileName, self.Lineno))

        QuotedFlag = True
        if UnicodeString.startswith("L'"):
            QuotedFlag = False
        UnicodeString = UnicodeString[2:-1]

        # No null-terminator in L'string'
        if (QuotedFlag and (len(UnicodeString) + 1) * 2 > Size) or (not QuotedFlag and len(UnicodeString) * 2 > Size):
            EdkLogger.error("BPDG", BuildToolError.RESOURCE_OVERFLOW,
                            "The size of unicode string %s is too larger for size %s(File: %s Line: %s)" % \
                            (UnicodeString, Size, self.FileName, self.Lineno))

        ReturnArray = array.array('B')
        for Value in UnicodeString:
            try:
                ReturnArray.append(ord(Value))
                ReturnArray.append(0)
            except:
                EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID,
                                "Invalid unicode character %s in unicode string %s(File: %s Line: %s)" % \
                                (Value, UnicodeString, self.FileName, self.Lineno))

        for Index in range(len(UnicodeString) * 2, Size):
            ReturnArray.append(0)

        self.PcdValue = ReturnArray.tolist()



## The class implementing the BPDG VPD PCD offset fix process
#
#   The VPD PCD offset fix process includes:
#       1. Parse the input guided.txt file and store it in the data structure;
#       2. Format the input file data to remove unused lines;
#       3. Fixed offset if needed;
#       4. Generate output file, including guided.map and guided.bin file;
#
class GenVPD :
    ## Constructor of DscBuildData
    #
    #  Initialize object of GenVPD
    #   @Param      InputFileName   The filename include the vpd type pcd information
    #   @param      MapFileName     The filename of map file that stores vpd type pcd information.
    #                               This file will be generated by the BPDG tool after fix the offset
    #                               and adjust the offset to make the pcd data aligned.
    #   @param      VpdFileName     The filename of Vpd file that hold vpd pcd information.
    #
    def __init__(self, InputFileName, MapFileName, VpdFileName):
        self.InputFileName           = InputFileName
        self.MapFileName             = MapFileName
        self.VpdFileName             = VpdFileName
        self.FileLinesList           = []
        self.PcdFixedOffsetSizeList  = []
        self.PcdUnknownOffsetList    = []
        try:
            fInputfile = open(InputFileName, "r")
            try:
                self.FileLinesList = fInputfile.readlines()
            except:
                EdkLogger.error("BPDG", BuildToolError.FILE_READ_FAILURE, "File read failed for %s" % InputFileName, None)
            finally:
                fInputfile.close()
        except:
            EdkLogger.error("BPDG", BuildToolError.FILE_OPEN_FAILURE, "File open failed for %s" % InputFileName, None)

    ##
    # Parser the input file which is generated by the build tool. Convert the value of each pcd's
    # from string to its real format. Also remove the useless line in the input file.
    #
    def ParserInputFile (self):
        count = 0
        for line in self.FileLinesList:
            # Strip "\r\n" generated by readlines ().
            line = line.strip()
            line = line.rstrip(os.linesep)

            # Skip the comment line
            if (not line.startswith("#")) and len(line) > 1 :
                #
                # Enhanced for support "|" character in the string.
                #
                ValueList = ['', '', '', '', '']

                ValueRe = re.compile(r'\s*L?\".*\|.*\"\s*$')
                PtrValue = ValueRe.findall(line)

                ValueUpdateFlag = False

                if len(PtrValue) >= 1:
                    line = re.sub(ValueRe, '', line)
                    ValueUpdateFlag = True

                TokenList = line.split('|')
                ValueList[0:len(TokenList)] = TokenList

                if ValueUpdateFlag:
                    ValueList[4] = PtrValue[0]
                self.FileLinesList[count] = ValueList
                # Store the line number
                self.FileLinesList[count].append(str(count + 1))
            elif len(line) <= 1 :
                # Set the blank line to "None"
                self.FileLinesList[count] = None
            else :
                # Set the comment line to "None"
                self.FileLinesList[count] = None
            count += 1

        # The line count contain usage information
        count = 0
        # Delete useless lines
        while (True) :
            try :
                if (self.FileLinesList[count] is None) :
                    del(self.FileLinesList[count])
                else :
                    count += 1
            except :
                break
        #
        # After remove the useless line, if there are no data remain in the file line list,
        # Report warning messages to user's.
        #
        if len(self.FileLinesList) == 0 :
            EdkLogger.warn('BPDG', BuildToolError.RESOURCE_NOT_AVAILABLE,
                           "There are no VPD type pcds defined in DSC file, Please check it.")

        # Process the pcds one by one base on the pcd's value and size
        count = 0
        for line in self.FileLinesList:
            if line is not None :
                PCD = PcdEntry(line[0], line[1], line[2], line[3], line[4], line[5], self.InputFileName)
                # Strip the space char
                PCD.PcdCName     = PCD.PcdCName.strip(' ')
                PCD.SkuId        = PCD.SkuId.strip(' ')
                PCD.PcdOffset    = PCD.PcdOffset.strip(' ')
                PCD.PcdSize      = PCD.PcdSize.strip(' ')
                PCD.PcdValue     = PCD.PcdValue.strip(' ')
                PCD.Lineno       = PCD.Lineno.strip(' ')

                #
                # Store the original pcd value.
                # This information will be useful while generate the output map file.
                #
                PCD.PcdUnpackValue    =  str(PCD.PcdValue)

                #
                # Translate PCD size string to an integer value.
                PackSize = None
                try:
                    PackSize = int(PCD.PcdSize, 10)
                    PCD.PcdBinSize = PackSize
                except:
                    try:
                        PackSize = int(PCD.PcdSize, 16)
                        PCD.PcdBinSize = PackSize
                    except:
                        EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID, "Invalid PCD size value %s at file: %s line: %s" % (PCD.PcdSize, self.InputFileName, PCD.Lineno))

                #
                # If value is Unicode string (e.g. L""), then use 2-byte alignment
                # If value is byte array (e.g. {}), then use 8-byte alignment
                #
                PCD.PcdOccupySize = PCD.PcdBinSize
                if PCD.PcdUnpackValue.startswith("{"):
                    Alignment = 8
                elif PCD.PcdUnpackValue.startswith("L"):
                    Alignment = 2
                else:
                    Alignment = 1

                PCD.Alignment = Alignment
                if PCD.PcdOffset != TAB_STAR:
                    if PCD.PcdOccupySize % Alignment != 0:
                        if PCD.PcdUnpackValue.startswith("{"):
                            EdkLogger.warn("BPDG", "The offset value of PCD %s is not 8-byte aligned!" %(PCD.PcdCName), File=self.InputFileName)
                        else:
                            EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID, 'The offset value of PCD %s should be %s-byte aligned.' % (PCD.PcdCName, Alignment))
                else:
                    if PCD.PcdOccupySize % Alignment != 0:
                        PCD.PcdOccupySize = (PCD.PcdOccupySize // Alignment + 1) * Alignment

                PackSize = PCD.PcdOccupySize
                if PCD._IsBoolean(PCD.PcdValue, PCD.PcdSize):
                    PCD._PackBooleanValue(PCD.PcdValue)
                    self.FileLinesList[count] = PCD
                    count += 1
                    continue
                #
                # Try to translate value to an integer firstly.
                #
                IsInteger = True
                PackValue = None
                try:
                    PackValue = int(PCD.PcdValue)
                except:
                    try:
                        PackValue = int(PCD.PcdValue, 16)
                    except:
                        IsInteger = False

                if IsInteger:
                    PCD._PackIntValue(PackValue, PackSize)
                else:
                    PCD._PackPtrValue(PCD.PcdValue, PackSize)

                self.FileLinesList[count] = PCD
                count += 1
            else :
                continue

    ##
    # This function used to create a clean list only contain useful information and reorganized to make it
    # easy to be sorted
    #
    def FormatFileLine (self) :

        for eachPcd in self.FileLinesList :
            if eachPcd.PcdOffset != TAB_STAR :
                # Use pcd's Offset value as key, and pcd's Value as value
                self.PcdFixedOffsetSizeList.append(eachPcd)
            else :
                # Use pcd's CName as key, and pcd's Size as value
                self.PcdUnknownOffsetList.append(eachPcd)


    ##
    # This function is use to fix the offset value which the not specified in the map file.
    # Usually it use the star (meaning any offset) character in the offset field
    #
    def FixVpdOffset (self):
        # At first, the offset should start at 0
        # Sort fixed offset list in order to find out where has free spaces for the pcd's offset
        # value is TAB_STAR to insert into.

        self.PcdFixedOffsetSizeList.sort(key=lambda x: x.PcdBinOffset)

        #
        # Sort the un-fixed pcd's offset by its size.
        #
        self.PcdUnknownOffsetList.sort(key=lambda x: x.PcdBinSize)

        index =0
        for pcd in self.PcdUnknownOffsetList:
            index += 1
            if pcd.PcdCName == ".".join(("gEfiMdeModulePkgTokenSpaceGuid", "PcdNvStoreDefaultValueBuffer")):
                if index != len(self.PcdUnknownOffsetList):
                    for i in range(len(self.PcdUnknownOffsetList) - index):
                        self.PcdUnknownOffsetList[index+i -1 ], self.PcdUnknownOffsetList[index+i] = self.PcdUnknownOffsetList[index+i], self.PcdUnknownOffsetList[index+i -1]

        #
        # Process all Offset value are TAB_STAR
        #
        if (len(self.PcdFixedOffsetSizeList) == 0) and (len(self.PcdUnknownOffsetList) != 0) :
            # The offset start from 0
            NowOffset = 0
            for Pcd in self.PcdUnknownOffsetList :
                if NowOffset % Pcd.Alignment != 0:
                    NowOffset = (NowOffset// Pcd.Alignment + 1) * Pcd.Alignment
                Pcd.PcdBinOffset = NowOffset
                Pcd.PcdOffset    = str(hex(Pcd.PcdBinOffset))
                NowOffset       += Pcd.PcdOccupySize

            self.PcdFixedOffsetSizeList = self.PcdUnknownOffsetList
            return

        # Check the offset of VPD type pcd's offset start from 0.
        if self.PcdFixedOffsetSizeList[0].PcdBinOffset != 0 :
            EdkLogger.warn("BPDG", "The offset of VPD type pcd should start with 0, please check it.",
                            None)

        # Judge whether the offset in fixed pcd offset list is overlapped or not.
        lenOfList = len(self.PcdFixedOffsetSizeList)
        count     = 0
        while (count < lenOfList - 1) :
            PcdNow  = self.PcdFixedOffsetSizeList[count]
            PcdNext = self.PcdFixedOffsetSizeList[count+1]
            # Two pcd's offset is same
            if PcdNow.PcdBinOffset == PcdNext.PcdBinOffset :
                EdkLogger.error("BPDG", BuildToolError.ATTRIBUTE_GET_FAILURE,
                                "The offset of %s at line: %s is same with %s at line: %s in file %s" % \
                                (PcdNow.PcdCName, PcdNow.Lineno, PcdNext.PcdCName, PcdNext.Lineno, PcdNext.FileName),
                                None)

            # Overlapped
            if PcdNow.PcdBinOffset + PcdNow.PcdOccupySize > PcdNext.PcdBinOffset :
                EdkLogger.error("BPDG", BuildToolError.ATTRIBUTE_GET_FAILURE,
                                "The offset of %s at line: %s is overlapped with %s at line: %s in file %s" % \
                                (PcdNow.PcdCName, PcdNow.Lineno, PcdNext.PcdCName, PcdNext.Lineno, PcdNext.FileName),
                                None)

            # Has free space, raise a warning message
            if PcdNow.PcdBinOffset + PcdNow.PcdOccupySize < PcdNext.PcdBinOffset :
                EdkLogger.warn("BPDG", BuildToolError.ATTRIBUTE_GET_FAILURE,
                               "The offsets have free space of between %s at line: %s and %s at line: %s in file %s" % \
                               (PcdNow.PcdCName, PcdNow.Lineno, PcdNext.PcdCName, PcdNext.Lineno, PcdNext.FileName),
                                None)
            count += 1

        LastOffset              = self.PcdFixedOffsetSizeList[0].PcdBinOffset
        FixOffsetSizeListCount  = 0
        lenOfList               = len(self.PcdFixedOffsetSizeList)
        lenOfUnfixedList        = len(self.PcdUnknownOffsetList)

        ##
        # Insert the un-fixed offset pcd's list into fixed offset pcd's list if has free space between those pcds.
        #
        while (FixOffsetSizeListCount < lenOfList) :

            eachFixedPcd     = self.PcdFixedOffsetSizeList[FixOffsetSizeListCount]
            NowOffset        = eachFixedPcd.PcdBinOffset

            # Has free space
            if LastOffset < NowOffset :
                if lenOfUnfixedList != 0 :
                    countOfUnfixedList = 0
                    while(countOfUnfixedList < lenOfUnfixedList) :
                        eachUnfixedPcd      = self.PcdUnknownOffsetList[countOfUnfixedList]
                        needFixPcdSize      = eachUnfixedPcd.PcdOccupySize
                        # Not been fixed
                        if eachUnfixedPcd.PcdOffset == TAB_STAR :
                            if LastOffset % eachUnfixedPcd.Alignment != 0:
                                LastOffset = (LastOffset // eachUnfixedPcd.Alignment + 1) * eachUnfixedPcd.Alignment
                            # The offset un-fixed pcd can write into this free space
                            if needFixPcdSize <= (NowOffset - LastOffset) :
                                # Change the offset value of un-fixed pcd
                                eachUnfixedPcd.PcdOffset    = str(hex(LastOffset))
                                eachUnfixedPcd.PcdBinOffset = LastOffset
                                # Insert this pcd into fixed offset pcd list.
                                self.PcdFixedOffsetSizeList.insert(FixOffsetSizeListCount, eachUnfixedPcd)

                                # Delete the item's offset that has been fixed and added into fixed offset list
                                self.PcdUnknownOffsetList.pop(countOfUnfixedList)

                                # After item added, should enlarge the length of fixed pcd offset list
                                lenOfList               += 1
                                FixOffsetSizeListCount  += 1

                                # Decrease the un-fixed pcd offset list's length
                                lenOfUnfixedList        -= 1

                                # Modify the last offset value
                                LastOffset              += needFixPcdSize
                            else :
                                # It can not insert into those two pcds, need to check still has other space can store it.
                                LastOffset             = NowOffset + self.PcdFixedOffsetSizeList[FixOffsetSizeListCount].PcdOccupySize
                                FixOffsetSizeListCount += 1
                                break

                # Set the FixOffsetSizeListCount = lenOfList for quit the loop
                else :
                    FixOffsetSizeListCount = lenOfList

            # No free space, smoothly connect with previous pcd.
            elif LastOffset == NowOffset :
                LastOffset = NowOffset + eachFixedPcd.PcdOccupySize
                FixOffsetSizeListCount += 1
            # Usually it will not enter into this thunk, if so, means it overlapped.
            else :
                EdkLogger.error("BPDG", BuildToolError.ATTRIBUTE_NOT_AVAILABLE,
                                "The offset value definition has overlapped at pcd: %s, its offset is: %s, in file: %s line: %s" % \
                                (eachFixedPcd.PcdCName, eachFixedPcd.PcdOffset, eachFixedPcd.InputFileName, eachFixedPcd.Lineno),
                                None)
                FixOffsetSizeListCount += 1

        # Continue to process the un-fixed offset pcd's list, add this time, just append them behind the fixed pcd's offset list.
        lenOfUnfixedList  = len(self.PcdUnknownOffsetList)
        lenOfList         = len(self.PcdFixedOffsetSizeList)
        while (lenOfUnfixedList > 0) :
            # Still has items need to process
            # The last pcd instance
            LastPcd    = self.PcdFixedOffsetSizeList[lenOfList-1]
            NeedFixPcd = self.PcdUnknownOffsetList[0]

            NeedFixPcd.PcdBinOffset = LastPcd.PcdBinOffset + LastPcd.PcdOccupySize
            if NeedFixPcd.PcdBinOffset % NeedFixPcd.Alignment != 0:
                NeedFixPcd.PcdBinOffset = (NeedFixPcd.PcdBinOffset // NeedFixPcd.Alignment + 1) * NeedFixPcd.Alignment

            NeedFixPcd.PcdOffset    = str(hex(NeedFixPcd.PcdBinOffset))

            # Insert this pcd into fixed offset pcd list's tail.
            self.PcdFixedOffsetSizeList.insert(lenOfList, NeedFixPcd)
            # Delete the item's offset that has been fixed and added into fixed offset list
            self.PcdUnknownOffsetList.pop(0)

            lenOfList          += 1
            lenOfUnfixedList   -= 1
    ##
    # Write the final data into output files.
    #
    def GenerateVpdFile (self, MapFileName, BinFileName):
        #Open an VPD file to process

        try:
            fVpdFile = open(BinFileName, "wb")
        except:
            # Open failed
            EdkLogger.error("BPDG", BuildToolError.FILE_OPEN_FAILURE, "File open failed for %s" % self.VpdFileName, None)

        try :
            fMapFile = open(MapFileName, "w")
        except:
            # Open failed
            EdkLogger.error("BPDG", BuildToolError.FILE_OPEN_FAILURE, "File open failed for %s" % self.MapFileName, None)

        # Use a instance of BytesIO to cache data
        fStringIO = BytesIO()

        # Write the header of map file.
        try :
            fMapFile.write (st.MAP_FILE_COMMENT_TEMPLATE + "\n")
        except:
            EdkLogger.error("BPDG", BuildToolError.FILE_WRITE_FAILURE, "Write data to file %s failed, please check whether the file been locked or using by other applications." % self.MapFileName, None)

        for eachPcd in self.PcdFixedOffsetSizeList  :
            # write map file
            try :
                fMapFile.write("%s | %s | %s | %s | %s  \n" % (eachPcd.PcdCName, eachPcd.SkuId, eachPcd.PcdOffset, eachPcd.PcdSize, eachPcd.PcdUnpackValue))
            except:
                EdkLogger.error("BPDG", BuildToolError.FILE_WRITE_FAILURE, "Write data to file %s failed, please check whether the file been locked or using by other applications." % self.MapFileName, None)

            # Write Vpd binary file
            fStringIO.seek (eachPcd.PcdBinOffset)
            if isinstance(eachPcd.PcdValue, list):
                for i in range(len(eachPcd.PcdValue)):
                    Value = eachPcd.PcdValue[i:i + 1]
                    if isinstance(bytes(Value), str):
                        fStringIO.write(chr(Value[0]))
                    else:
                        fStringIO.write(bytes(Value))
            else:
                fStringIO.write (eachPcd.PcdValue)

        try :
            fVpdFile.write (fStringIO.getvalue())
        except:
            EdkLogger.error("BPDG", BuildToolError.FILE_WRITE_FAILURE, "Write data to file %s failed, please check whether the file been locked or using by other applications." % self.VpdFileName, None)

        fStringIO.close ()
        fVpdFile.close ()
        fMapFile.close ()

