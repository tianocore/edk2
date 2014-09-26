## @file
# This file is used to define class objects for DEC file. It will consumed by 
#DecParser
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

'''
DecObject
'''

## Import modules
#
import os.path

from Library.Misc import Sdict
from Library.DataType import TAB_GUIDS
from Library.DataType import TAB_PPIS
from Library.DataType import TAB_PROTOCOLS
from Library.DataType import TAB_DEC_DEFINES
from Library.DataType import TAB_INCLUDES
from Library.DataType import TAB_LIBRARY_CLASSES
from Library.DataType import TAB_USER_EXTENSIONS
from Library.DataType import TAB_PCDS
from Library.DataType import TAB_ARCH_COMMON

## _DecComments
#
# Base class for all data objects which have head and tail comments
#
class _DecComments:

    ##constructor
    #
    def __init__(self):
        self._HeadComment = []
        self._TailComment = []

    ## GetComments
    #
    def GetComments(self):
        return self._HeadComment, self._TailComment

    ## GetHeadComment
    #    
    def GetHeadComment(self):
        return self._HeadComment

    ## SetHeadComment
    #
    # @param Comment: comment content
    #
    def SetHeadComment(self, Comment):
        self._HeadComment = Comment

    ## GetTailComment
    #    
    def GetTailComment(self):
        return self._TailComment

    ## SetTailComment
    #
    # @param Comment: comment content
    #
    def SetTailComment(self, Comment):
        self._TailComment = Comment

## _DecBaseObject
#
# Base class that hold common info
#
class _DecBaseObject(_DecComments):
    def __init__(self, PkgFullName):
        _DecComments.__init__(self)
        #
        # Key is combined with (Arch, SectionType)
        # Default is common
        #
        self.ValueDict = Sdict()
        self._PkgFullName = PkgFullName
        self._PackagePath, self._FileName = os.path.split(PkgFullName)
        self._SecName = ''

    ## GetSectionName
    #        
    def GetSectionName(self):
        return self._SecName

    ## GetPackagePath
    #        
    def GetPackagePath(self):
        return self._PackagePath

    ## GetPackageFile
    #        
    def GetPackageFile(self):
        return self._FileName

    ## GetPackageFullName
    #        
    def GetPackageFullName(self):
        return self._PkgFullName

    ## AddItem
    # Add sub-item to current object, sub-class should override it if needed
    #
    # @param Item: Sub-item to be added
    # @param Scope: A list store section name and arch info
    #
    def AddItem(self, Item, Scope):
        if not Scope:
            return
        if not Item:
            return
        ArchModule = []
        for Ele in Scope:
            if Ele[1] in self.ValueDict:
                self.ValueDict[Ele[1]].append(Item)
            else:
                self.ValueDict[Ele[1]] = [Item]
            ArchModule.append(Ele[1])
        Item.ArchAndModuleType = ArchModule

    ## _GetItemByArch
    # Helper class used by sub-class
    # @param Arch:  arch
    #
    def _GetItemByArch(self, Arch):
        Arch = Arch.upper()
        if Arch not in self.ValueDict:
            return []
        return self.ValueDict[Arch]

    ## _GetAllItems
    # Get all items, union all arches, items in returned list are unique
    #
    def _GetAllItems(self):
        Retlst = []
        for Arch in self.ValueDict:
            for Item in self.ValueDict[Arch]:
                if Item not in Retlst:
                    Retlst.append(Item)
        return Retlst

## _DecItemBaseObject
#
# Module type and arch the item belongs to 
#
class _DecItemBaseObject(_DecComments):
    def __init__(self):
        _DecComments.__init__(self)
        #
        # Item's arch, if PCD, also include PCD type
        #
        self.ArchAndModuleType = []

    ## GetArchList
    #    
    def GetArchList(self):
        ArchSet = set()
        for Arch in self.ArchAndModuleType:
            ArchSet.add(Arch)
        return list(ArchSet)

## DecDefineObject
#
# Class to hold define section infomation
#
class DecDefineObject(_DecBaseObject):
    def __init__(self, PkgFullName):
        _DecBaseObject.__init__(self, PkgFullName)
        self._SecName = TAB_DEC_DEFINES.upper()
        self._DecSpec = ''
        self._PkgName = ''
        self._PkgGuid = ''
        self._PkgVersion = ''
        self._PkgUniFile = ''

    ## GetPackageSpecification
    #        
    def GetPackageSpecification(self):
        return self._DecSpec

    def SetPackageSpecification(self, DecSpec):
        self._DecSpec = DecSpec

    ## GetPackageName
    #        
    def GetPackageName(self):
        return self._PkgName

    def SetPackageName(self, PkgName):
        self._PkgName = PkgName

    ## GetPackageGuid
    #        
    def GetPackageGuid(self):
        return self._PkgGuid

    def SetPackageGuid(self, PkgGuid):
        self._PkgGuid = PkgGuid

    ## GetPackageVersion
    #        
    def GetPackageVersion(self):
        return self._PkgVersion

    def SetPackageVersion(self, PkgVersion):
        self._PkgVersion = PkgVersion

    ## GetPackageUniFile
    #        
    def GetPackageUniFile(self):
        return self._PkgUniFile

    def SetPackageUniFile(self, PkgUniFile):
        self._PkgUniFile = PkgUniFile

    ## GetDefines
    #        
    def GetDefines(self):
        return self._GetItemByArch(TAB_ARCH_COMMON)

    ## GetAllDefines
    #        
    def GetAllDefines(self):
        return self._GetAllItems()

## DecDefineItemObject
#
# Each item of define section
#
class DecDefineItemObject(_DecItemBaseObject):
    def __init__(self):
        _DecItemBaseObject.__init__(self)
        self.Key = ''
        self.Value = ''

    ## __hash__
    #            
    def __hash__(self):
        return hash(self.Key + self.Value)

    ## __eq__
    #
    def __eq__(self, Other):
        return id(self) == id(Other)

    ## __str__
    #            
    def __str__(self):
        return str(self.ArchAndModuleType) + '\n' + self.Key + \
            ' = ' + self.Value

## DecIncludeObject
#
# Class to hold include section info
#
class DecIncludeObject(_DecBaseObject):
    def __init__(self, PkgFullName):
        _DecBaseObject.__init__(self, PkgFullName)
        self._SecName = TAB_INCLUDES.upper()

    ## GetIncludes
    #          
    def GetIncludes(self, Arch=TAB_ARCH_COMMON):
        return self._GetItemByArch(Arch)

    ## GetAllIncludes
    #          
    def GetAllIncludes(self):
        return self._GetAllItems()

## DecIncludeItemObject
#
# Item of include section
#
class DecIncludeItemObject(_DecItemBaseObject):
    def __init__(self, File, Root):
        self.File = File
        self.Root = Root
        _DecItemBaseObject.__init__(self)

    ## __hash__
    #          
    def __hash__(self):
        return hash(self.File)

    ## __eq__
    #
    def __eq__(self, Other):
        return id(self) == id(Other)

    ## __str__
    #          
    def __str__(self):
        return self.File

## DecLibraryclassObject
#
# Class to hold library class section info
#
class DecLibraryclassObject(_DecBaseObject):
    def __init__(self, PkgFullName):
        _DecBaseObject.__init__(self, PkgFullName)
        self._PackagePath, self._FileName = os.path.split(PkgFullName)
        self._SecName = TAB_LIBRARY_CLASSES.upper()

    ## GetLibraryclasses
    #           
    def GetLibraryclasses(self, Arch=TAB_ARCH_COMMON):
        return self._GetItemByArch(Arch)

    ## GetAllLibraryclasses
    #           
    def GetAllLibraryclasses(self):
        return self._GetAllItems()

## DecLibraryclassItemObject
# Item of library class section
#
class DecLibraryclassItemObject(_DecItemBaseObject):
    def __init__(self, Libraryclass, File, Root):
        _DecItemBaseObject.__init__(self)
        self.File = File
        self.Root = Root
        self.Libraryclass = Libraryclass

    ## __hash__
    #        
    def __hash__(self):
        return hash(self.Libraryclass + self.File)

    ## __eq__
    #
    def __eq__(self, Other):
        return id(self) == id(Other)

    ## __str__
    #        
    def __str__(self):
        return self.Libraryclass + '|' + self.File

## DecPcdObject
# Class to hold PCD section
#
class DecPcdObject(_DecBaseObject):
    def __init__(self, PkgFullName):
        _DecBaseObject.__init__(self, PkgFullName)
        self._SecName = TAB_PCDS.upper()

    ## AddItem
    #
    # Diff from base class
    #
    # @param Item: Item
    # @param Scope: Scope
    #
    def AddItem(self, Item, Scope):
        if not Scope:
            return
        if not Item:
            return
        ArchModule = []
        for Type, Arch in Scope:
            if (Type, Arch) in self.ValueDict:
                self.ValueDict[Type, Arch].append(Item)
            else:
                self.ValueDict[Type, Arch] = [Item]
            ArchModule.append([Type, Arch])
        Item.ArchAndModuleType = ArchModule

    ## GetPcds
    #
    # @param PcdType: PcdType
    # @param Arch: Arch
    #    
    def GetPcds(self, PcdType, Arch=TAB_ARCH_COMMON):
        PcdType = PcdType.upper()
        Arch = Arch.upper()
        if (PcdType, Arch) not in self.ValueDict:
            return []
        return self.ValueDict[PcdType, Arch]

    ## GetPcdsByType
    #
    # @param PcdType: PcdType
    #        
    def GetPcdsByType(self, PcdType):
        PcdType = PcdType.upper()
        Retlst = []
        for TypeInDict, Arch in self.ValueDict:
            if TypeInDict != PcdType:
                continue
            for Item in self.ValueDict[PcdType, Arch]:
                if Item not in Retlst:
                    Retlst.append(Item)
        return Retlst

## DecPcdItemObject
#
# Item of PCD section
#
# @param _DecItemBaseObject: _DecItemBaseObject object
#
class DecPcdItemObject(_DecItemBaseObject):
    def __init__(self, Guid, Name, Value, DatumType,
                 Token, MaxDatumSize=''):
        _DecItemBaseObject.__init__(self)
        self.TokenCName = Name
        self.TokenSpaceGuidCName = Guid
        self.DatumType = DatumType
        self.DefaultValue = Value
        self.TokenValue = Token
        self.MaxDatumSize = MaxDatumSize

    ## __hash__
    #  
    def __hash__(self):
        return hash(self.TokenSpaceGuidCName + self.TokenCName)

    ## __eq__
    #
    def __eq__(self, Other):
        return id(self) == id(Other)

    ## GetArchListOfType
    #
    # @param PcdType: PcdType
    #      
    def GetArchListOfType(self, PcdType):
        ItemSet = set()
        PcdType = PcdType.upper()
        for Type, Arch in self.ArchAndModuleType:
            if Type != PcdType:
                continue
            ItemSet.add(Arch)
        return list(ItemSet)

## DecGuidObjectBase
#
# Base class for PPI, Protocol, and GUID.
# Hold same data but has different method for clarification in sub-class
#
# @param _DecBaseObject: Dec Base Object
#
class DecGuidObjectBase(_DecBaseObject):
    def __init__(self, PkgFullName):
        _DecBaseObject.__init__(self, PkgFullName)

    ## GetGuidStyleItems
    #
    # @param Arch: Arch
    #       
    def GetGuidStyleItems(self, Arch=TAB_ARCH_COMMON):
        return self._GetItemByArch(Arch)

    ## GetGuidStyleAllItems
    #       
    def GetGuidStyleAllItems(self):
        return self._GetAllItems()

## DecGuidItemObject
#
# Item of GUID, PPI and Protocol section
#
# @param _DecItemBaseObject: Dec Item Base Object
#
class DecGuidItemObject(_DecItemBaseObject):
    def __init__(self, CName, GuidCValue, GuidString):
        _DecItemBaseObject.__init__(self)
        self.GuidCName = CName
        self.GuidCValue = GuidCValue
        self.GuidString = GuidString

    ## __hash__
    #      
    def __hash__(self):
        return hash(self.GuidCName)

    ## __eq__
    #
    def __eq__(self, Other):
        return id(self) == id(Other)

    ## __str__
    #      
    def __str__(self):
        return self.GuidCName + ' = ' + self.GuidCValue

## DecGuidObject
#
# Class for GUID section
#
# @param DecGuidObjectBase: Dec Guid Object Base
#
class DecGuidObject(DecGuidObjectBase):
    def __init__(self, PkgFullName):
        DecGuidObjectBase.__init__(self, PkgFullName)
        self._SecName = TAB_GUIDS.upper()

    ## GetGuids
    #          
    # @param Arch: Arch
    #
    def GetGuids(self, Arch=TAB_ARCH_COMMON):
        return self._GetItemByArch(Arch)

    ## GetAllGuids
    #          
    def GetAllGuids(self):
        return self._GetAllItems()

## DecPpiObject
#
# Class for PPI seciont
#
# @param DecGuidObjectBase: Dec Guid Object Base
#
class DecPpiObject(DecGuidObjectBase):
    def __init__(self, PkgFullName):
        DecGuidObjectBase.__init__(self, PkgFullName)
        self._SecName = TAB_PPIS.upper()

    ## GetPpis
    #          
    # @param Arch: Arch
    #    
    def GetPpis(self, Arch=TAB_ARCH_COMMON):
        return self._GetItemByArch(Arch)

    ## GetAllPpis
    #           
    def GetAllPpis(self):
        return self._GetAllItems()

## DecProtocolObject
#
# Class for protocol section
#
# @param DecGuidObjectBase: Dec Guid Object Base
#
class DecProtocolObject(DecGuidObjectBase):
    def __init__(self, PkgFullName):
        DecGuidObjectBase.__init__(self, PkgFullName)
        self._SecName = TAB_PROTOCOLS.upper()

    ## GetProtocols
    #          
    # @param Arch: Arch
    #        
    def GetProtocols(self, Arch=TAB_ARCH_COMMON):
        return self._GetItemByArch(Arch)

    ## GetAllProtocols
    #        
    def GetAllProtocols(self):
        return self._GetAllItems()

## DecUserExtensionObject
#
# Class for user extension section
#
# @param _DecBaseObject: Dec Guid Object Base
#
class DecUserExtensionObject(_DecBaseObject):
    def __init__(self, PkgFullName):
        _DecBaseObject.__init__(self, PkgFullName)
        self._SecName = TAB_USER_EXTENSIONS.upper()
        self.ItemList = []

    ## GetProtocols
    #          
    # @param Item: Item
    # @param Scope: Scope
    #            
    def AddItem(self, Item, Scope):
        if not Scope:
            pass
        if not Item:
            return
        self.ItemList.append(Item)

    ## GetAllUserExtensions
    #       
    def GetAllUserExtensions(self):
        return self.ItemList


## DecUserExtensionItemObject
# Item for user extension section
#
# @param _DecItemBaseObject: Dec Item Base Object
#
class DecUserExtensionItemObject(_DecItemBaseObject):
    def __init__(self):
        _DecItemBaseObject.__init__(self)
        self.UserString = ''
        self.UserId = ''
        self.IdString = ''




