## @file
# Define a dictionary structure
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
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
import EdkLogger
from DataType import *
from Common.LongFilePathSupport import OpenLongFilePath as open

## Convert a text file to a dictionary
#
# Convert a text file to a dictionary of (name:value) pairs.
#
# @retval 0  Convert successful
# @retval 1  Open file failed
#
def ConvertTextFileToDictionary(FileName, Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter):
    try:
        F = open(FileName,'r')
        Keys = []
        for Line in F:
            if Line.startswith(CommentCharacter):
                continue
            LineList = Line.split(KeySplitCharacter,1)
            if len(LineList) >= 2:
                Key = LineList[0].split()
            if len(Key) == 1 and Key[0][0] != CommentCharacter and Key[0] not in Keys:
                if ValueSplitFlag:
                    Dictionary[Key[0]] = LineList[1].replace('\\','/').split(ValueSplitCharacter)
                else:
                    Dictionary[Key[0]] = LineList[1].strip().replace('\\','/')
                Keys += [Key[0]]
        F.close()
        return 0
    except:
        EdkLogger.info('Open file failed')
        return 1

## Print the dictionary
#
# Print all items of dictionary one by one
#
# @param Dict:  The dictionary to be printed
#
def printDict(Dict):
    if Dict != None:
        KeyList = Dict.keys()
        for Key in KeyList:
            if Dict[Key] != '':
                print Key + ' = ' + str(Dict[Key])

## Print the dictionary
#
# Print the items of dictionary which matched with input key
#
# @param list:  The dictionary to be printed
# @param key:   The key of the item to be printed
#
def printList(Key, List):
    if type(List) == type([]):
        if len(List) > 0:
            if Key.find(TAB_SPLIT) != -1:
                print "\n" + Key
                for Item in List:
                    print Item
