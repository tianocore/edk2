## @file
# This file is used to define class objects of INF file miscellaneous.
# Include BootMode/HOB/Event and others. It will consumed by InfParser.
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

'''
InfMisc
'''

import Logger.Log as Logger
from Logger import ToolError

from Library import DataType as DT
from Object.Parser.InfCommonObject import InfSectionCommonDef
from Library.Misc import Sdict

##
# BootModeObject
#
class InfBootModeObject():
    def __init__(self):
        self.SupportedBootModes = ''
        self.HelpString = ''
        self.Usage = ''

    def SetSupportedBootModes(self, SupportedBootModes):
        self.SupportedBootModes = SupportedBootModes
    def GetSupportedBootModes(self):
        return self.SupportedBootModes

    def SetHelpString(self, HelpString):
        self.HelpString = HelpString
    def GetHelpString(self):
        return self.HelpString

    def SetUsage(self, Usage):
        self.Usage = Usage
    def GetUsage(self):
        return self.Usage
##
# EventObject
#
class InfEventObject():
    def __init__(self):
        self.EventType = ''
        self.HelpString = ''
        self.Usage = ''

    def SetEventType(self, EventType):
        self.EventType = EventType

    def GetEventType(self):
        return self.EventType

    def SetHelpString(self, HelpString):
        self.HelpString = HelpString
    def GetHelpString(self):
        return self.HelpString

    def SetUsage(self, Usage):
        self.Usage = Usage
    def GetUsage(self):
        return self.Usage
##
# HobObject
#
class InfHobObject():
    def __init__(self):
        self.HobType = ''
        self.Usage = ''
        self.SupArchList = []
        self.HelpString = ''

    def SetHobType(self, HobType):
        self.HobType = HobType

    def GetHobType(self):
        return self.HobType

    def SetUsage(self, Usage):
        self.Usage = Usage
    def GetUsage(self):
        return self.Usage

    def SetSupArchList(self, ArchList):
        self.SupArchList = ArchList
    def GetSupArchList(self):
        return self.SupArchList

    def SetHelpString(self, HelpString):
        self.HelpString = HelpString
    def GetHelpString(self):
        return self.HelpString

##
# InfSpecialCommentObject
#
class InfSpecialCommentObject(InfSectionCommonDef):
    def __init__(self):
        self.SpecialComments = Sdict()
        InfSectionCommonDef.__init__(self)

    def SetSpecialComments(self, SepcialSectionList = None, Type = ''):
        if Type == DT.TYPE_HOB_SECTION or \
           Type == DT.TYPE_EVENT_SECTION or \
           Type == DT.TYPE_BOOTMODE_SECTION:
            for Item in SepcialSectionList:
                if Type in self.SpecialComments:
                    ObjList = self.SpecialComments[Type]
                    ObjList.append(Item)
                    self.SpecialComments[Type] = ObjList
                else:
                    ObjList = []
                    ObjList.append(Item)
                    self.SpecialComments[Type] = ObjList

        return True

    def GetSpecialComments(self):
        return self.SpecialComments



## ErrorInInf
#
# An encapsulate of Error for INF parser.
#
def ErrorInInf(Message=None, ErrorCode=None, LineInfo=None, RaiseError=True):
    if ErrorCode is None:
        ErrorCode = ToolError.FORMAT_INVALID
    if LineInfo is None:
        LineInfo = ['', -1, '']
    Logger.Error("InfParser",
                 ErrorCode,
                 Message=Message,
                 File=LineInfo[0],
                 Line=LineInfo[1],
                 ExtraData=LineInfo[2],
                 RaiseError=RaiseError)
