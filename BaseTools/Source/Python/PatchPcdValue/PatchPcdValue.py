## @file
# Patch value into the binary file.
#
# Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import Common.LongFilePathOs as os
from Common.LongFilePathSupport import OpenLongFilePath as open
import sys
import re

from optparse import OptionParser
from optparse import make_option
from Common.BuildToolError import *
import Common.EdkLogger as EdkLogger
from Common.BuildVersion import gBUILD_VERSION
import array

# Version and Copyright
__version_number__ = ("0.10" + " " + gBUILD_VERSION)
__version__ = "%prog Version " + __version_number__
__copyright__ = "Copyright (c) 2010, Intel Corporation. All rights reserved."

## PatchBinaryFile method
#
# This method mainly patches the data into binary file.
# 
# @param FileName    File path of the binary file
# @param ValueOffset Offset value 
# @param TypeName    DataType Name
# @param Value       Value String
# @param MaxSize     MaxSize value
#
# @retval 0     File is updated successfully.
# @retval not 0 File is updated failed.
#
def PatchBinaryFile(FileName, ValueOffset, TypeName, ValueString, MaxSize=0):
    #
    # Length of Binary File
    #
    FileHandle = open (FileName, 'rb')
    FileHandle.seek (0, 2)
    FileLength = FileHandle.tell()
    FileHandle.close()
    #
    # Unify string to upper string
    #
    TypeName = TypeName.upper()
    #
    # Get PCD value data length
    #
    ValueLength = 0
    if TypeName == 'BOOLEAN':
        ValueLength = 1
    elif TypeName == 'UINT8':
        ValueLength = 1
    elif TypeName == 'UINT16':
        ValueLength = 2
    elif TypeName == 'UINT32':
        ValueLength = 4
    elif TypeName == 'UINT64':
        ValueLength = 8
    elif TypeName == 'VOID*':
        if MaxSize == 0:
            return OPTION_MISSING, "PcdMaxSize is not specified for VOID* type PCD."
        ValueLength = int(MaxSize)
    else:
        return PARAMETER_INVALID,  "PCD type %s is not valid." %(CommandOptions.PcdTypeName)
    #
    # Check PcdValue is in the input binary file.
    #
    if ValueOffset + ValueLength > FileLength:
        return PARAMETER_INVALID, "PcdOffset + PcdMaxSize(DataType) is larger than the input file size."
    #
    # Read binary file into array
    #
    FileHandle = open (FileName, 'rb')
    ByteArray = array.array('B')
    ByteArray.fromfile(FileHandle, FileLength)
    FileHandle.close()
    OrigByteList = ByteArray.tolist()
    ByteList = ByteArray.tolist()
    #
    # Clear the data in file
    #
    for Index in range(ValueLength):
        ByteList[ValueOffset + Index] = 0
    #
    # Patch value into offset
    #
    SavedStr = ValueString
    ValueString = ValueString.upper()
    ValueNumber = 0
    if TypeName == 'BOOLEAN':
        #
        # Get PCD value for BOOLEAN data type
        #
        try:
            if ValueString == 'TRUE':
                ValueNumber = 1
            elif ValueString == 'FALSE':
                ValueNumber = 0
            elif ValueString.startswith('0X'):
                ValueNumber = int (ValueString, 16)
            else:
                ValueNumber = int (ValueString)
            if ValueNumber != 0:
                ValueNumber = 1
        except:
            return PARAMETER_INVALID, "PCD Value %s is not valid dec or hex string." %(ValueString)
        #
        # Set PCD value into binary data
        #
        ByteList[ValueOffset] = ValueNumber
    elif TypeName in ['UINT8', 'UINT16', 'UINT32', 'UINT64']:
        #
        # Get PCD value for UINT* data type
        #
        try:
            if ValueString.startswith('0X'):
                ValueNumber = int (ValueString, 16)
            else:
                ValueNumber = int (ValueString)
        except:
            return PARAMETER_INVALID, "PCD Value %s is not valid dec or hex string." %(ValueString)
        #
        # Set PCD value into binary data
        #
        for Index in range(ValueLength):
            ByteList[ValueOffset + Index] = ValueNumber % 0x100
            ValueNumber = ValueNumber / 0x100
    elif TypeName == 'VOID*':
        ValueString = SavedStr
        if ValueString.startswith('L"'):
            #
            # Patch Unicode String
            #
            Index = 0
            for ByteString in ValueString[2:-1]:
                #
                # Reserve zero as unicode tail
                #
                if Index + 2 >= ValueLength:
                    break
                #
                # Set string value one by one
                #
                ByteList[ValueOffset + Index] = ord(ByteString)
                Index = Index + 2
        elif ValueString.startswith("{") and ValueString.endswith("}"):
            #
            # Patch {0x1, 0x2, ...} byte by byte
            #
            ValueList = ValueString[1 : len(ValueString) - 1].split(', ')
            Index = 0
            try:
                for ByteString in ValueList:
                    if ByteString.upper().startswith('0X'):
                        ByteValue = int(ByteString, 16)
                    else:
                        ByteValue = int(ByteString)
                    ByteList[ValueOffset + Index] = ByteValue % 0x100
                    Index = Index + 1
                    if Index >= ValueLength:
                        break
            except:
                return PARAMETER_INVALID, "PCD Value %s is not valid dec or hex string array." %(ValueString)
        else:
            #
            # Patch ascii string 
            #
            Index = 0
            for ByteString in ValueString[1:-1]:
                #
                # Reserve zero as string tail
                #
                if Index + 1 >= ValueLength:
                    break
                #
                # Set string value one by one
                #
                ByteList[ValueOffset + Index] = ord(ByteString)
                Index = Index + 1
    #
    # Update new data into input file.
    #
    if ByteList != OrigByteList:
        ByteArray = array.array('B')
        ByteArray.fromlist(ByteList)
        FileHandle = open (FileName, 'wb')
        ByteArray.tofile(FileHandle)
        FileHandle.close()
    return 0, "Patch Value into File %s successfully." %(FileName)

## Parse command line options
#
# Using standard Python module optparse to parse command line option of this tool.
#
# @retval Options   A optparse.Values object containing the parsed options
# @retval InputFile Path of file to be trimmed
#
def Options():
    OptionList = [
        make_option("-f", "--offset", dest="PcdOffset", action="store", type="int",
                          help="Start offset to the image is used to store PCD value."),
        make_option("-u", "--value", dest="PcdValue", action="store",
                          help="PCD value will be updated into the image."),
        make_option("-t", "--type", dest="PcdTypeName", action="store",
                          help="The name of PCD data type may be one of VOID*,BOOLEAN, UINT8, UINT16, UINT32, UINT64."),
        make_option("-s", "--maxsize", dest="PcdMaxSize", action="store", type="int",
                          help="Max size of data buffer is taken by PCD value.It must be set when PCD type is VOID*."),
        make_option("-v", "--verbose", dest="LogLevel", action="store_const", const=EdkLogger.VERBOSE,
                          help="Run verbosely"),
        make_option("-d", "--debug", dest="LogLevel", type="int",
                          help="Run with debug information"),
        make_option("-q", "--quiet", dest="LogLevel", action="store_const", const=EdkLogger.QUIET,
                          help="Run quietly"),
        make_option("-?", action="help", help="show this help message and exit"),
    ]

    # use clearer usage to override default usage message
    UsageString = "%prog -f Offset -u Value -t Type [-s MaxSize] <input_file>"

    Parser = OptionParser(description=__copyright__, version=__version__, option_list=OptionList, usage=UsageString)
    Parser.set_defaults(LogLevel=EdkLogger.INFO)

    Options, Args = Parser.parse_args()

    # error check
    if len(Args) == 0:
        EdkLogger.error("PatchPcdValue", PARAMETER_INVALID, ExtraData=Parser.get_usage())

    InputFile = Args[len(Args) - 1]
    return Options, InputFile

## Entrance method
#
# This method mainly dispatch specific methods per the command line options.
# If no error found, return zero value so the caller of this tool can know
# if it's executed successfully or not.
#
# @retval 0     Tool was successful
# @retval 1     Tool failed
#
def Main():
    try:
        #
        # Check input parameter
        #
        EdkLogger.Initialize()
        CommandOptions, InputFile = Options()
        if CommandOptions.LogLevel < EdkLogger.DEBUG_9:
            EdkLogger.SetLevel(CommandOptions.LogLevel + 1)
        else:
            EdkLogger.SetLevel(CommandOptions.LogLevel)
        if not os.path.exists (InputFile):
            EdkLogger.error("PatchPcdValue", FILE_NOT_FOUND, ExtraData=InputFile)
            return 1
        if CommandOptions.PcdOffset == None or CommandOptions.PcdValue == None or CommandOptions.PcdTypeName == None:
            EdkLogger.error("PatchPcdValue", OPTION_MISSING, ExtraData="PcdOffset or PcdValue of PcdTypeName is not specified.")
            return 1
        if CommandOptions.PcdTypeName.upper() not in ["BOOLEAN", "UINT8", "UINT16", "UINT32", "UINT64", "VOID*"]:
            EdkLogger.error("PatchPcdValue", PARAMETER_INVALID, ExtraData="PCD type %s is not valid." %(CommandOptions.PcdTypeName))
            return 1
        if CommandOptions.PcdTypeName.upper() == "VOID*" and CommandOptions.PcdMaxSize == None:
            EdkLogger.error("PatchPcdValue", OPTION_MISSING, ExtraData="PcdMaxSize is not specified for VOID* type PCD.")
            return 1
        #
        # Patch value into binary image.
        #
        ReturnValue, ErrorInfo = PatchBinaryFile (InputFile, CommandOptions.PcdOffset, CommandOptions.PcdTypeName, CommandOptions.PcdValue, CommandOptions.PcdMaxSize)
        if ReturnValue != 0:
            EdkLogger.error("PatchPcdValue", ReturnValue, ExtraData=ErrorInfo)
            return 1
        return 0
    except:
        return 1

if __name__ == '__main__':
    r = Main()
    sys.exit(r)
