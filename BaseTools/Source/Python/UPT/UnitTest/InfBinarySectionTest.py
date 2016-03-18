## @file
# This file contain unit test for Test [Binary] section part of InfParser 
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

import os
#import Object.Parser.InfObject as InfObject
from Object.Parser.InfCommonObject import CurrentLine
from Object.Parser.InfCommonObject import InfLineCommentObject
from Object.Parser.InfBinaryObject import InfBinariesObject
import Logger.Log as Logger
import Library.GlobalData as Global
##
# Test Common binary item
#

#-------------start of common binary item test input--------------------------#

#
# Only has 1 element, binary item Type
#
SectionStringsCommonItem1 = \
"""
GUID
"""
#
# Have 2 elements, binary item Type and FileName
#
SectionStringsCommonItem2 = \
"""
GUID | Test/Test.guid
"""

#
# Have 3 elements, Type | FileName | Target | Family | TagName | FeatureFlagExp
#
SectionStringsCommonItem3 = \
"""
GUID | Test/Test.guid | DEBUG
"""

#
# Have 3 elements, Type | FileName | Target 
# Target with MACRO defined in [Define] section
#
SectionStringsCommonItem4 = \
"""
GUID | Test/Test.guid | $(TARGET)
"""

#
# Have 3 elements, Type | FileName | Target 
# FileName with MACRO defined in [Binary] section
#
SectionStringsCommonItem5 = \
"""
DEFINE BINARY_FILE_PATH = Test
GUID | $(BINARY_FILE_PATH)/Test.guid | $(TARGET)
"""

#
# Have 4 elements, Type | FileName | Target | Family
#
SectionStringsCommonItem6 = \
"""
GUID | Test/Test.guid | DEBUG | *
"""

#
# Have 4 elements, Type | FileName | Target | Family
#
SectionStringsCommonItem7 = \
"""
GUID | Test/Test.guid | DEBUG | MSFT
"""

#
# Have 5 elements, Type | FileName | Target | Family | TagName
#
SectionStringsCommonItem8 = \
"""
GUID | Test/Test.guid | DEBUG | MSFT | TEST
"""

#
# Have 6 elements, Type | FileName | Target | Family | TagName | FFE
#
SectionStringsCommonItem9 = \
"""
GUID | Test/Test.guid | DEBUG | MSFT | TEST | TRUE
"""

#
# Have 7 elements, Type | FileName | Target | Family | TagName | FFE | Overflow
# Test wrong format
#
SectionStringsCommonItem10 = \
"""
GUID | Test/Test.guid | DEBUG | MSFT | TEST | TRUE | OVERFLOW
"""

#-------------end of common binary item test input----------------------------#



#-------------start of VER type binary item test input------------------------#

#
# Has 1 element, error format 
#
SectionStringsVerItem1 = \
"""
VER
"""
#
# Have 5 elements, error format(Maximum elements amount is 4)
#
SectionStringsVerItem2 = \
"""
VER | Test/Test.ver | * | TRUE | OverFlow
"""

#
# Have 2 elements, Type | FileName
#
SectionStringsVerItem3 = \
"""
VER | Test/Test.ver
"""

#
# Have 3 elements, Type | FileName | Target
#
SectionStringsVerItem4 = \
"""
VER | Test/Test.ver | DEBUG
"""

#
# Have 4 elements, Type | FileName | Target | FeatureFlagExp
#
SectionStringsVerItem5 = \
"""
VER | Test/Test.ver | DEBUG | TRUE
"""

#
# Exist 2 VER items, both opened.
#
SectionStringsVerItem6 = \
"""
VER | Test/Test.ver | * | TRUE
VER | Test/Test2.ver | * | TRUE
"""


#
# Exist 2 VER items, only 1 opened.
#
SectionStringsVerItem7 = \
"""
VER | Test/Test.ver | * | TRUE
VER | Test/Test2.ver | * | FALSE
"""

#-------------end of VER type binary item test input--------------------------#


#-------------start of UI type binary item test input-------------------------#

#
# Test only one UI section can exist
#
SectionStringsUiItem1 = \
"""
UI | Test/Test.ui | * | TRUE
UI | Test/Test2.ui | * | TRUE
"""

SectionStringsUiItem2 = \
"""
UI | Test/Test.ui | * | TRUE
SEC_UI | Test/Test2.ui | * | TRUE
"""

SectionStringsUiItem3 = \
"""
UI | Test/Test.ui | * | TRUE
UI | Test/Test2.ui | * | FALSE
"""

#
# Has 1 element, error format 
#
SectionStringsUiItem4 = \
"""
UI
"""
#
# Have 5 elements, error format(Maximum elements amount is 4)
#
SectionStringsUiItem5 = \
"""
UI | Test/Test.ui | * | TRUE | OverFlow
"""

#
# Have 2 elements, Type | FileName
#
SectionStringsUiItem6 = \
"""
UI | Test/Test.ui
"""

#
# Have 3 elements, Type | FileName | Target
#
SectionStringsUiItem7 = \
"""
UI | Test/Test.ui | DEBUG
"""

#
# Have 4 elements, Type | FileName | Target | FeatureFlagExp
#
SectionStringsUiItem8 = \
"""
UI | Test/Test.ui | DEBUG | TRUE
"""
#---------------end of UI type binary item test input-------------------------#


gFileName = "BinarySectionTest.inf"

##
# Construct SectionString for call section parser usage.
#
def StringToSectionString(String):
    Lines = String.split('\n')
    LineNo = 0
    SectionString = []
    for Line in Lines:
        if Line.strip() == '':
            continue
        SectionString.append((Line, LineNo, ''))
        LineNo = LineNo + 1
        
    return SectionString

def PrepareTest(String):
    SectionString = StringToSectionString(String)
    ItemList = []
    for Item in SectionString:
        ValueList = Item[0].split('|')
        for count in range(len(ValueList)):
            ValueList[count] = ValueList[count].strip()
        if len(ValueList) >= 2:
            #
            # Create a temp file for test.
            #
            FileName = os.path.normpath(os.path.realpath(ValueList[1].strip()))
            try:
                TempFile  = open (FileName, "w")    
                TempFile.close()
            except:
                print "File Create Error"
        CurrentLine = CurrentLine()
        CurrentLine.SetFileName("Test")
        CurrentLine.SetLineString(Item[0])
        CurrentLine.SetLineNo(Item[1])
        InfLineCommentObject = InfLineCommentObject()
        
        ItemList.append((ValueList, InfLineCommentObject, CurrentLine))
                
    return ItemList

if __name__ == '__main__':
    Logger.Initialize()
    
    InfBinariesInstance = InfBinariesObject()
    ArchList = ['COMMON']
    Global.gINF_MODULE_DIR = os.getcwd()
    
    AllPassedFlag = True
    
    #
    # For All Ui test
    #
    UiStringList = [ 
                    SectionStringsUiItem1,
                    SectionStringsUiItem2,
                    SectionStringsUiItem3,
                    SectionStringsUiItem4,
                    SectionStringsUiItem5,
                    SectionStringsUiItem6,
                    SectionStringsUiItem7,
                    SectionStringsUiItem8 
                    ]
    
    for Item in UiStringList:        
        Ui = PrepareTest(Item)
        if Item == SectionStringsUiItem4 or Item == SectionStringsUiItem5:
            try:
                InfBinariesInstance.SetBinary(Ui = Ui, ArchList = ArchList)
            except Logger.FatalError:
                pass
        else:
            try:		
                InfBinariesInstance.SetBinary(Ui = Ui, ArchList = ArchList)
            except:
                AllPassedFlag = False   				
 
    #
    # For All Ver Test
    #
    VerStringList = [
                     SectionStringsVerItem1,
                     SectionStringsVerItem2,
                     SectionStringsVerItem3,
                     SectionStringsVerItem4,
                     SectionStringsVerItem5,
                     SectionStringsVerItem6,
                     SectionStringsVerItem7
                     ]
    for Item in VerStringList:        
        Ver = PrepareTest(Item)
        if Item == SectionStringsVerItem1 or \
           Item == SectionStringsVerItem2:
            
            try:
                InfBinariesInstance.SetBinary(Ver = Ver, ArchList = ArchList)
            except:
                pass
                    
        else:
            try:
                InfBinariesInstance.SetBinary(Ver = Ver, ArchList = ArchList)
            except:
                AllPassedFlag = False   
    
    #
    # For All Common Test
    #    
    CommonStringList = [
                     SectionStringsCommonItem1,
                     SectionStringsCommonItem2,
                     SectionStringsCommonItem3,
                     SectionStringsCommonItem4,
                     SectionStringsCommonItem5,
                     SectionStringsCommonItem6,
                     SectionStringsCommonItem7,
                     SectionStringsCommonItem8,
                     SectionStringsCommonItem9,
                     SectionStringsCommonItem10
                     ]

    for Item in CommonStringList:        
        CommonBin = PrepareTest(Item)
        if Item == SectionStringsCommonItem10 or \
           Item == SectionStringsCommonItem1:
            
            try:
                InfBinariesInstance.SetBinary(CommonBinary = CommonBin, ArchList = ArchList)
            except:
                pass
                    
        else:
            try:
                InfBinariesInstance.SetBinary(Ver = Ver, ArchList = ArchList)
            except:
                print "Test Failed!"
                AllPassedFlag = False
    
    if AllPassedFlag :
        print 'All tests passed...'
    else:
        print 'Some unit test failed!'

