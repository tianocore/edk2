## @file
# This file is used to parse exception items found by ECC tool
#
# Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
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
from Xml.XmlRoutines import *
import os.path

# ExceptionXml to parse Exception Node of XML file
class ExceptionXml(object):
    def __init__(self):
        self.KeyWord = ''
        self.ErrorID = ''
        self.FilePath = ''
        
    def FromXml(self, Item, Key):
        self.KeyWord = XmlElement(Item, '%s/KeyWord' % Key)
        self.ErrorID = XmlElement(Item, '%s/ErrorID' % Key)
        self.FilePath = os.path.normpath(XmlElement(Item, '%s/FilePath' % Key))
        
    def __str__(self):
        return 'ErrorID = %s KeyWord = %s FilePath = %s' %(self.ErrorID, self.KeyWord, self.FilePath)

# ExceptionListXml to parse Exception Node List of XML file
class ExceptionListXml(object):
    def __init__(self):
        self.List = []
    
    def FromXmlFile(self, FilePath):
        XmlContent = XmlParseFile(FilePath)
        for Item in XmlList(XmlContent, '/ExceptionList/Exception'):
            Exp = ExceptionXml()
            Exp.FromXml(Item, 'Exception')
            self.List.append(Exp)
    
    def ToList(self):
        RtnList = []
        for Item in self.List:
            #RtnList.append((Item.ErrorID, Item.KeyWord, Item.FilePath))
            RtnList.append((Item.ErrorID, Item.KeyWord))
    
        return RtnList
        
    def __str__(self):
        RtnStr = ''
        if self.List:
            for Item in self.List:
                RtnStr = RtnStr + str(Item) + '\n'
        return RtnStr

# A class to check exception
class ExceptionCheck(object):
    def __init__(self, FilePath = None):
        self.ExceptionList = []
        self.ExceptionListXml = ExceptionListXml()
        self.LoadExceptionListXml(FilePath)

    def LoadExceptionListXml(self, FilePath):
        if FilePath and os.path.isfile(FilePath):
            self.ExceptionListXml.FromXmlFile(FilePath)
            self.ExceptionList = self.ExceptionListXml.ToList()
    
    def IsException(self, ErrorID, KeyWord, FileID=-1):
        if (str(ErrorID), KeyWord) in self.ExceptionList:
            return True
        else:
            return False

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    El = ExceptionCheck('C:\\Hess\\Project\\BuildTool\\src\\Ecc\\exception.xml')
    print El.ExceptionList
