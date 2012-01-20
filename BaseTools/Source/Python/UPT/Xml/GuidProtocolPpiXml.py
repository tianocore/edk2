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
GuidProtocolPpiXml
'''
from Library.String import ConvertNEToNOTEQ
from Library.String import ConvertNOTEQToNE
from Library.String import GetStringOfList
from Library.Xml.XmlRoutines import XmlElement
from Library.Xml.XmlRoutines import XmlAttribute
from Library.Xml.XmlRoutines import XmlNode
from Library.Xml.XmlRoutines import XmlList
from Library.Xml.XmlRoutines import CreateXmlElement

from Object.POM.CommonObject import GuidObject
from Object.POM.CommonObject import ProtocolObject
from Object.POM.CommonObject import PpiObject

from Xml.CommonXml import CommonDefinesXml
from Xml.CommonXml import HelpTextXml

from Xml.XmlParserMisc import GetHelpTextList

##
#GUID/Protocol/Ppi Common
#
class GuidProtocolPpiXml(object):
    def __init__(self, Mode):
        self.UiName = ''
        self.GuidTypes = ''
        self.Notify = ''
        self.CName = ''
        self.GuidValue = ''
        self.CommonDefines = CommonDefinesXml()
        self.HelpText = []
        #
        # Guid/Ppi/Library, internal used for indicate return object for 
        # FromXml
        #
        self.Type = '' 
        #
        # there are slightly different field between package and module
        #
        self.Mode = Mode
        self.GuidType = ''
        self.VariableName = ''
        
    def FromXml(self, Item, Key):
        self.UiName = XmlAttribute(XmlNode(Item, '%s' % Key), 'UiName')
        self.GuidType = XmlAttribute(XmlNode(Item, '%s' % Key), 'GuidType')
        self.Notify = XmlAttribute(XmlNode(Item, '%s' % Key), 'Notify')
        self.CName = XmlElement(Item, '%s/CName' % Key)
        self.GuidValue = XmlElement(Item, '%s/GuidValue' % Key)
        self.VariableName = XmlElement(Item, '%s/VariableName' % Key)
        self.CommonDefines.FromXml(XmlNode(Item, '%s' % Key), Key)
        for HelpTextItem in XmlList(Item, '%s/HelpText' % Key):
            HelpTextObj = HelpTextXml()
            HelpTextObj.FromXml(HelpTextItem, '%s/HelpText' % Key)
            self.HelpText.append(HelpTextObj)
        
        if self.Type == 'Guid':    
            GuidProtocolPpi = GuidObject()
        elif self.Type == 'Protocol':
            GuidProtocolPpi = ProtocolObject()
        else:
            GuidProtocolPpi = PpiObject()
        GuidProtocolPpi.SetHelpTextList(GetHelpTextList(self.HelpText))

        return GuidProtocolPpi

    def ToXml(self, GuidProtocolPpi, Key):
        if self.GuidValue:
            pass
        AttributeList = \
        [['Usage', GetStringOfList(GuidProtocolPpi.GetUsage())], \
         ['UiName', GuidProtocolPpi.GetName()], \
         ['GuidType', GetStringOfList(GuidProtocolPpi.GetGuidTypeList())], \
         ['Notify', str(GuidProtocolPpi.GetNotify()).lower()], \
         ['SupArchList', GetStringOfList(GuidProtocolPpi.GetSupArchList())], \
         ['SupModList', GetStringOfList(GuidProtocolPpi.GetSupModuleList())], \
         ['FeatureFlag', ConvertNEToNOTEQ(GuidProtocolPpi.GetFeatureFlag())]
        ]
        NodeList = [['CName', GuidProtocolPpi.GetCName()], 
                    ['GuidValue', GuidProtocolPpi.GetGuid()],
                    ['VariableName', GuidProtocolPpi.VariableName]
                   ]
        for Item in GuidProtocolPpi.GetHelpTextList():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root

    def __str__(self):
        Str = \
        "UiName = %s Notify = %s GuidTypes = %s CName = %s GuidValue = %s %s" \
        % (self.UiName, self.Notify, self.GuidTypes, self.CName, \
           self.GuidValue, self.CommonDefines)
        for Item in self.HelpText:
            Str = Str + "\n\t" + str(Item)
        return Str
##
#GUID Xml
#
class GuidXml(GuidProtocolPpiXml):
    def __init__(self, Mode):
        GuidProtocolPpiXml.__init__(self, Mode)
        self.Type = 'Guid'
        
    def FromXml(self, Item, Key):         
        GuidProtocolPpi = GuidProtocolPpiXml.FromXml(self, Item, Key)

        if self.Mode == 'Package':
            
            GuidProtocolPpi.SetSupArchList(self.CommonDefines.SupArchList)
            GuidProtocolPpi.SetSupModuleList(self.CommonDefines.SupModList)
            GuidProtocolPpi.SetCName(self.CName)
            GuidProtocolPpi.SetGuid(self.GuidValue)
        else:
            GuidProtocolPpi.SetUsage(self.CommonDefines.Usage)
            if self.GuidType:
                GuidProtocolPpi.SetGuidTypeList([self.GuidType])
            GuidProtocolPpi.SetSupArchList(self.CommonDefines.SupArchList)
            GuidProtocolPpi.SetFeatureFlag(ConvertNOTEQToNE(self.CommonDefines.FeatureFlag))
            GuidProtocolPpi.SetCName(self.CName)
            GuidProtocolPpi.SetVariableName(self.VariableName)
        return GuidProtocolPpi

    def ToXml(self, GuidProtocolPpi, Key):
        if self.Mode == 'Package': 
            AttributeList = \
            [['GuidType', \
              GetStringOfList(GuidProtocolPpi.GetGuidTypeList())], \
              ['SupArchList', \
               GetStringOfList(GuidProtocolPpi.GetSupArchList())], \
               ['SupModList', \
                GetStringOfList(GuidProtocolPpi.GetSupModuleList())], 
            ]
            NodeList = [['CName', GuidProtocolPpi.GetCName()], 
                        ['GuidValue', GuidProtocolPpi.GetGuid()],
                       ]
        else:
            AttributeList = \
            [['Usage', GetStringOfList(GuidProtocolPpi.GetUsage())], \
             ['GuidType', GetStringOfList(GuidProtocolPpi.GetGuidTypeList())],\
              ['SupArchList', \
               GetStringOfList(GuidProtocolPpi.GetSupArchList())], \
               ['FeatureFlag', ConvertNEToNOTEQ(GuidProtocolPpi.GetFeatureFlag())]
            ]
            NodeList = [['CName', GuidProtocolPpi.GetCName()], 
                        ['VariableName', GuidProtocolPpi.GetVariableName()]
                       ]

        for Item in GuidProtocolPpi.GetHelpTextList():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root
##
#Protocol Xml
#
class ProtocolXml(GuidProtocolPpiXml):
    def __init__(self, Mode):
        GuidProtocolPpiXml.__init__(self, Mode)
        self.Type = 'Protocol'
        
    def FromXml(self, Item, Key):
        GuidProtocolPpi = GuidProtocolPpiXml.FromXml(self, Item, Key)
        if self.Mode == 'Package':
            GuidProtocolPpi.SetFeatureFlag(self.CommonDefines.FeatureFlag) 
            GuidProtocolPpi.SetSupArchList(self.CommonDefines.SupArchList)
            GuidProtocolPpi.SetSupModuleList(self.CommonDefines.SupModList)
            GuidProtocolPpi.SetCName(self.CName)
            GuidProtocolPpi.SetGuid(self.GuidValue)
        else:
            GuidProtocolPpi.SetUsage(self.CommonDefines.Usage)
            if self.Notify.upper() == "TRUE":
                GuidProtocolPpi.SetNotify(True)
            elif self.Notify.upper() == "FALSE":
                GuidProtocolPpi.SetNotify(False)
            else:
                GuidProtocolPpi.SetNotify('')
            GuidProtocolPpi.SetSupArchList(self.CommonDefines.SupArchList)
            GuidProtocolPpi.SetFeatureFlag(ConvertNOTEQToNE(self.CommonDefines.FeatureFlag))
            GuidProtocolPpi.SetCName(self.CName)
 
        return GuidProtocolPpi

    def ToXml(self, GuidProtocolPpi, Key):
        if self.Mode == 'Package':        
            AttributeList = \
            [['SupArchList', \
              GetStringOfList(GuidProtocolPpi.GetSupArchList())], \
              ['SupModList', \
               GetStringOfList(GuidProtocolPpi.GetSupModuleList())], \
               ['FeatureFlag', GuidProtocolPpi.GetFeatureFlag()]
            ]
            NodeList = [['CName', GuidProtocolPpi.GetCName()], 
                        ['GuidValue', GuidProtocolPpi.GetGuid()],
                       ]
        else:
            AttributeList = \
            [['Usage', GetStringOfList(GuidProtocolPpi.GetUsage())], \
             ['Notify', str(GuidProtocolPpi.GetNotify()).lower()], \
             ['SupArchList', \
              GetStringOfList(GuidProtocolPpi.GetSupArchList())], \
              ['FeatureFlag', ConvertNEToNOTEQ(GuidProtocolPpi.GetFeatureFlag())]
            ]
            NodeList = [['CName', GuidProtocolPpi.GetCName()], 
                       ]
            
        for Item in GuidProtocolPpi.GetHelpTextList():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        
        return Root
##
#Ppi Xml
#
class PpiXml(GuidProtocolPpiXml):
    def __init__(self, Mode):
        GuidProtocolPpiXml.__init__(self, Mode)
        self.Type = 'Ppi'
        
    def FromXml(self, Item, Key):
        GuidProtocolPpi = GuidProtocolPpiXml.FromXml(self, Item, Key)
        if self.Mode == 'Package':
            GuidProtocolPpi.SetSupArchList(self.CommonDefines.SupArchList)
            GuidProtocolPpi.SetSupModuleList(self.CommonDefines.SupModList)
            GuidProtocolPpi.SetCName(self.CName)
            GuidProtocolPpi.SetGuid(self.GuidValue)
        else:
            GuidProtocolPpi.SetUsage(self.CommonDefines.Usage)
            if self.Notify.upper() == "TRUE":
                GuidProtocolPpi.SetNotify(True)
            elif self.Notify.upper() == "FALSE":
                GuidProtocolPpi.SetNotify(False)
            else:
                GuidProtocolPpi.SetNotify('')
            GuidProtocolPpi.SetSupArchList(self.CommonDefines.SupArchList)
            GuidProtocolPpi.SetFeatureFlag(ConvertNOTEQToNE(self.CommonDefines.FeatureFlag))
            GuidProtocolPpi.SetCName(self.CName)
 
        return GuidProtocolPpi

    def ToXml(self, GuidProtocolPpi, Key):
        if self.Mode == 'Package':
            AttributeList = \
            [['SupArchList', \
              GetStringOfList(GuidProtocolPpi.GetSupArchList())], 
            ]
            NodeList = [['CName', GuidProtocolPpi.GetCName()], 
                        ['GuidValue', GuidProtocolPpi.GetGuid()],
                       ]
        else:
            AttributeList = \
            [['Usage', GetStringOfList(GuidProtocolPpi.GetUsage())], \
             ['Notify', str(GuidProtocolPpi.GetNotify()).lower()], \
             ['SupArchList', \
              GetStringOfList(GuidProtocolPpi.GetSupArchList())], \
              ['FeatureFlag', ConvertNEToNOTEQ(GuidProtocolPpi.GetFeatureFlag())]
            ]
            NodeList = [['CName', GuidProtocolPpi.GetCName()], 
                       ]
        
        for Item in GuidProtocolPpi.GetHelpTextList():
            Tmp = HelpTextXml()
            NodeList.append(Tmp.ToXml(Item))
        Root = CreateXmlElement('%s' % Key, '', NodeList, AttributeList)
        return Root
