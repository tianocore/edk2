## @file
# This file is used to parse a xml file of .PKG file
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
#

'''
XmlParserMisc
'''
from Object.POM.CommonObject import TextObject
from Logger.StringTable import ERR_XML_PARSER_REQUIRED_ITEM_MISSING
from Logger.ToolError import PARSER_ERROR
import Logger.Log as Logger

## ConvertVariableName()
# Convert VariableName to be L"string", 
# input of UCS-2 format Hex Array or L"string" (C style.) could be converted successfully,
# others will not.
#
# @param VariableName: string need to be converted
# @retval: the L quoted string converted if success, else None will be returned
#
def ConvertVariableName(VariableName):
    VariableName = VariableName.strip()
    #
    # check for L quoted string 
    #
    if VariableName.startswith('L"') and VariableName.endswith('"'):
        return VariableName
    
    #
    # check for Hex Array, it should be little endian even number of hex numbers
    #
    ValueList = VariableName.split(' ')
    if len(ValueList)%2 == 1:
        return None

    TransferedStr = ''

    Index = 0

    while Index < len(ValueList):
        FirstByte = int(ValueList[Index], 16)
        SecondByte = int(ValueList[Index + 1], 16)
        if SecondByte != 0:
            return None
  
        if FirstByte not in xrange(0x20, 0x7F):
            return None
        TransferedStr += ('%c')%FirstByte
        Index = Index + 2

    return 'L"' + TransferedStr + '"'

## IsRequiredItemListNull
#
# Check if a required XML section item/attribue is NULL
# 
# @param ItemList:     The list of items to be checked
# @param XmlTreeLevel: The error message tree level
# 
def IsRequiredItemListNull(ItemDict, XmlTreeLevel):
    for Key in ItemDict:
        if not ItemDict[Key]:
            Msg = "->".join(Node for Node in XmlTreeLevel)
            ErrorMsg = ERR_XML_PARSER_REQUIRED_ITEM_MISSING % (Key, Msg)
            Logger.Error('\nUPT', PARSER_ERROR, ErrorMsg, RaiseError=True)

            
## Get help text 
#
# @param HelpText
#
def GetHelpTextList(HelpText):
    HelpTextList = []
    for HelT in HelpText:
        HelpTextObj = TextObject()
        HelpTextObj.SetLang(HelT.Lang)
        HelpTextObj.SetString(HelT.HelpText)
        HelpTextList.append(HelpTextObj)
    return HelpTextList
