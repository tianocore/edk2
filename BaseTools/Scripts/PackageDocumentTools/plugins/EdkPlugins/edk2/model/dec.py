## @file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from plugins.EdkPlugins.basemodel import ini
import re, os
from plugins.EdkPlugins.basemodel.message import *

class DECFile(ini.BaseINIFile):

    def GetSectionInstance(self, parent, name, isCombined=False):
        return DECSection(parent, name, isCombined)

    def GetComponents(self):
        return self.GetSectionByName('Components')

    def GetPackageRootPath(self):
        return os.path.dirname(self.GetFilename()).strip()

    def GetBaseName(self):
        return self.GetDefine("PACKAGE_NAME").strip()

    def GetVersion(self):
        return self.GetDefine("PACKAGE_VERSION").strip()

    def GetSectionObjectsByName(self, name, arch=None):
        arr = []
        sects = self.GetSectionByName(name)
        for sect in sects:
            # skip unmatched archtecture content
            if not sect.IsArchMatch(arch):
                continue

            for obj in sect.GetObjects():
                arr.append(obj)

        return arr

class DECSection(ini.BaseINISection):
    def GetSectionINIObject(self, parent):
        type = self.GetType()

        if type.lower().find('defines') != -1:
            return DECDefineSectionObject(self)
        if type.lower().find('includes') != -1:
            return DECIncludeObject(self)
        if type.lower().find('pcd') != -1:
            return DECPcdObject(self)
        if type.lower() == 'libraryclasses':
            return DECLibraryClassObject(self)
        if type.lower() == 'guids':
            return DECGuidObject(self)
        if type.lower() == 'ppis':
            return DECPpiObject(self)
        if type.lower() == 'protocols':
            return DECProtocolObject(self)

        return DECSectionObject(self)

    def GetType(self):
        arr = self._name.split('.')
        return arr[0].strip()

    def GetArch(self):
        arr = self._name.split('.')
        if len(arr) == 1:
            return 'common'
        return arr[1]

    def IsArchMatch(self, arch):
        if arch is None or self.GetArch() == 'common':
            return True

        if self.GetArch().lower() != arch.lower():
            return False

        return True

class DECSectionObject(ini.BaseINISectionObject):
    def GetArch(self):
        return self.GetParent().GetArch()

class DECDefineSectionObject(DECSectionObject):
    def __init__(self, parent):
        DECSectionObject.__init__(self, parent)
        self._key = None
        self._value = None

    def Parse(self):
        assert (self._start == self._end), 'The object in define section must be in single line'

        line = self.GetLineByOffset(self._start).strip()

        line = line.split('#')[0]
        arr  = line.split('=')
        if len(arr) != 2:
            ErrorMsg('Invalid define section object',
                   self.GetFilename(),
                   self.GetParent().GetName()
                   )
            return False

        self._key   = arr[0].strip()
        self._value = arr[1].strip()

        return True

    def GetKey(self):
        return self._key

    def GetValue(self):
        return self._value

class DECGuidObject(DECSectionObject):
    _objs = {}

    def __init__(self, parent):
        DECSectionObject.__init__(self, parent)
        self._name = None

    def Parse(self):
        line = self.GetLineByOffset(self._start).strip().split('#')[0]
        self._name = line.split('=')[0].strip()
        self._guid = line.split('=')[1].strip()
        objdict = DECGuidObject._objs
        if self._name not in objdict.keys():
            objdict[self._name] = [self]
        else:
            objdict[self._name].append(self)

        return True

    def GetName(self):
        return self._name

    def GetGuid(self):
        return self._guid

    def Destroy(self):
        objdict = DECGuidObject._objs
        objdict[self._name].remove(self)
        if len(objdict[self._name]) == 0:
            del objdict[self._name]

    @staticmethod
    def GetObjectDict():
        return DECGuidObject._objs

class DECPpiObject(DECSectionObject):
    _objs = {}
    def __init__(self, parent):
        DECSectionObject.__init__(self, parent)
        self._name = None

    def Parse(self):
        line = self.GetLineByOffset(self._start).strip().split('#')[0]
        self._name = line.split('=')[0].strip()
        self._guid = line.split('=')[1].strip()
        objdict = DECPpiObject._objs
        if self._name not in objdict.keys():
            objdict[self._name] = [self]
        else:
            objdict[self._name].append(self)

        return True

    def GetName(self):
        return self._name

    def GetGuid(self):
        return self._guid

    def Destroy(self):
        objdict = DECPpiObject._objs
        objdict[self._name].remove(self)
        if len(objdict[self._name]) == 0:
            del objdict[self._name]

    @staticmethod
    def GetObjectDict():
        return DECPpiObject._objs

class DECProtocolObject(DECSectionObject):
    _objs = {}

    def __init__(self, parent):
        DECSectionObject.__init__(self, parent)
        self._name = None

    def Parse(self):
        line = self.GetLineByOffset(self._start).strip().split('#')[0]
        self._name = line.split('=')[0].strip()
        self._guid = line.split('=')[1].strip()
        objdict = DECProtocolObject._objs
        if self._name not in objdict.keys():
            objdict[self._name] = [self]
        else:
            objdict[self._name].append(self)

        return True

    def GetName(self):
        return self._name

    def GetGuid(self):
        return self._guid

    def Destroy(self):
        objdict = DECProtocolObject._objs
        objdict[self._name].remove(self)
        if len(objdict[self._name]) == 0:
            del objdict[self._name]


    @staticmethod
    def GetObjectDict():
        return DECProtocolObject._objs

class DECLibraryClassObject(DECSectionObject):
    _objs = {}

    def __init__(self, parent):
        DECSectionObject.__init__(self, parent)
        self.mClassName = None
        self.mHeaderFile = None

    def Parse(self):
        line = self.GetLineByOffset(self._start).strip().split('#')[0]
        self.mClassName, self.mHeaderFile = line.split('|')
        objdict = DECLibraryClassObject._objs
        if self.mClassName not in objdict.keys():
            objdict[self.mClassName] = [self]
        else:
            objdict[self.mClassName].append(self)
        return True

    def GetClassName(self):
        return self.mClassName

    def GetName(self):
        return self.mClassName

    def GetHeaderFile(self):
        return self.mHeaderFile

    def Destroy(self):
        objdict = DECLibraryClassObject._objs
        objdict[self.mClassName].remove(self)
        if len(objdict[self.mClassName]) == 0:
            del objdict[self.mClassName]

    @staticmethod
    def GetObjectDict():
        return DECLibraryClassObject._objs

class DECIncludeObject(DECSectionObject):
    def __init__(self, parent):
        DECSectionObject.__init__(self, parent)

    def GetPath(self):
        return self.GetLineByOffset(self._start).split('#')[0].strip()

class DECPcdObject(DECSectionObject):
    _objs = {}

    def __init__(self, parent):
        DECSectionObject.__init__(self, parent)
        self.mPcdName           = None
        self.mPcdDefaultValue   = None
        self.mPcdDataType       = None
        self.mPcdToken          = None

    def Parse(self):
        line = self.GetLineByOffset(self._start).strip().split('#')[0]
        (self.mPcdName, self.mPcdDefaultValue, self.mPcdDataType, self.mPcdToken) = line.split('|')
        objdict = DECPcdObject._objs
        if self.mPcdName not in objdict.keys():
            objdict[self.mPcdName] = [self]
        else:
            objdict[self.mPcdName].append(self)

        return True

    def Destroy(self):
        objdict = DECPcdObject._objs
        objdict[self.mPcdName].remove(self)
        if len(objdict[self.mPcdName]) == 0:
            del objdict[self.mPcdName]

    def GetPcdType(self):
        return self.GetParent().GetType()

    def GetPcdName(self):
        return self.mPcdName

    def GetPcdValue(self):
        return self.mPcdDefaultValue

    def GetPcdDataType(self):
        return self.mPcdDataType

    def GetPcdToken(self):
        return self.mPcdToken

    def GetName(self):
        return self.GetPcdName().split('.')[1]

    @staticmethod
    def GetObjectDict():
        return DECPcdObject._objs
