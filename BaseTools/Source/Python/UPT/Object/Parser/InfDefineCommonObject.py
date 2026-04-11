## @file
# This file is used to define common class objects of [Defines] section for INF file.
# It will consumed by InfParser
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

'''
InfDefineCommonObject
'''

from Object.Parser.InfCommonObject import InfLineCommentObject

## InfDefineImageExeParamItem
#
class InfDefineImageExeParamItem():
    def __init__(self):
        self.CName  = ''
        self.FeatureFlagExp = ''
        self.Comments = InfLineCommentObject()

    def SetCName(self, CName):
        self.CName = CName
    def GetCName(self):
        return self.CName
    def SetFeatureFlagExp(self, FeatureFlagExp):
        self.FeatureFlagExp = FeatureFlagExp
    def GetFeatureFlagExp(self):
        return self.FeatureFlagExp

## InfDefineEntryPointItem
#
class InfDefineEntryPointItem(InfDefineImageExeParamItem):
    def __init__(self):
        InfDefineImageExeParamItem.__init__(self)

## InfDefineUnloadImageItem
#
class InfDefineUnloadImageItem(InfDefineImageExeParamItem):
    def __init__(self):
        InfDefineImageExeParamItem.__init__(self)

## InfDefineConstructorItem
#
class InfDefineConstructorItem(InfDefineImageExeParamItem):
    def __init__(self):
        InfDefineImageExeParamItem.__init__(self)
        self.SupModList = []

    def SetSupModList(self, SupModList):
        self.SupModList = SupModList
    def GetSupModList(self):
        return self.SupModList

## InfDefineDestructorItem
#
class InfDefineDestructorItem(InfDefineImageExeParamItem):
    def __init__(self):
        InfDefineImageExeParamItem.__init__(self)
        self.SupModList = []

    def SetSupModList(self, SupModList):
        self.SupModList = SupModList
    def GetSupModList(self):
        return self.SupModList

## InfDefineLibraryItem
#
class InfDefineLibraryItem():
    def __init__(self):
        self.LibraryName = ''
        self.Types = []
        self.Comments = InfLineCommentObject()

    def SetLibraryName(self, Name):
        self.LibraryName = Name
    def GetLibraryName(self):
        return self.LibraryName
    def SetTypes(self, Type):
        self.Types = Type
    def GetTypes(self):
        return self.Types
