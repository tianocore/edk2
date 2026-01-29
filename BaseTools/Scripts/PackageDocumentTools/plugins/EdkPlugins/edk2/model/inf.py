## @file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from plugins.EdkPlugins.basemodel import ini
import re, os
from plugins.EdkPlugins.basemodel.message import *

class INFFile(ini.BaseINIFile):
    _libobjs = {}

    def GetSectionInstance(self, parent, name, isCombined=False):
        return INFSection(parent, name, isCombined)

    def GetProduceLibraryClass(self):
        obj = self.GetDefine("LIBRARY_CLASS")
        if obj is None: return None

        return obj.split('|')[0].strip()

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

    def GetSourceObjects(self, arch=None, tool=None):
        arr = []
        sects = self.GetSectionByName('sources')
        for sect in sects:
            # skip unmatched archtecture content
            if not sect.IsArchMatch(arch):
                continue

            for obj in sect.GetObjects():
                if not obj.IsMatchFamily(tool):
                    continue
                arr.append(obj)

        return arr

    def Parse(self):
        if not ini.BaseINIFile.Parse(self):
            return False
        classname = self.GetProduceLibraryClass()
        if classname is not None:
            libobjdict = INFFile._libobjs
            if classname in libobjdict:
                if self not in libobjdict[classname]:
                    libobjdict[classname].append(self)
            else:
                libobjdict[classname] = [self]

        return True

    def GetBaseName(self):
        return self.GetDefine("BASE_NAME").strip()

    def GetModuleRootPath(self):
        return os.path.dirname(self.GetFilename())

    def Clear(self):
        classname = self.GetProduceLibraryClass()
        if classname is not None:
            libobjdict = INFFile._libobjs
            libobjdict[classname].remove(self)
            if len(libobjdict[classname]) == 0:
                del libobjdict[classname]
        ini.BaseINIFile.Clear(self)


class INFSection(ini.BaseINISection):
    def GetSectionINIObject(self, parent):
        type = self.GetType()

        if type.lower() == 'libraryclasses':
            return INFLibraryClassObject(self)
        if type.lower() == 'sources':
            return INFSourceObject(self)
        if type.lower().find('pcd') != -1:
            return INFPcdObject(self)
        if type.lower() == 'packages':
            return INFDependentPackageObject(self)
        if type.lower() in ['guids', 'protocols', 'ppis']:
            return INFGuidObject(self)
        if type.lower() == 'defines':
            return INFDefineSectionObject(self)
        return INFSectionObject(self)

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

class INFSectionObject(ini.BaseINISectionObject):
    def GetArch(self):
        return self.GetParent().GetArch()

class INFDefineSectionObject(INFSectionObject):
    def __init__(self, parent):
        INFSectionObject.__init__(self, parent)
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
                   self._start
                   )
            return False

        self._key   = arr[0].strip()
        self._value = arr[1].strip()

        return True

    def GetKey(self):
        return self._key

    def GetValue(self):
        return self._value

class INFLibraryClassObject(INFSectionObject):
    _objs = {}
    def __init__(self, parent):
        INFSectionObject.__init__(self, parent)
        self._classname = None

    def GetClass(self):
        return self._classname

    def Parse(self):
        self._classname = self.GetLineByOffset(self._start).split('#')[0].strip()
        objdict = INFLibraryClassObject._objs
        if self._classname in objdict:
            objdict[self._classname].append(self)
        else:
            objdict[self._classname] = [self]
        return True

    def Destroy(self):
        objdict = INFLibraryClassObject._objs
        objdict[self._classname].remove(self)
        if len(objdict[self._classname]) == 0:
            del objdict[self._classname]

    def GetName(self):
        return self._classname

    @staticmethod
    def GetObjectDict():
        return INFLibraryClassObject._objs

class INFDependentPackageObject(INFSectionObject):
    def GetPath(self):
        return self.GetLineByOffset(self._start).split('#')[0].strip()

class INFSourceObject(INFSectionObject):
    _objs = {}
    def __init__(self, parent):
        INFSectionObject.__init__(self, parent)

        self.mSourcename  = None
        self.mToolCode    = None
        self.mFamily      = None
        self.mTagName     = None
        self.mFeaturePcd  = None
        self.mFilename    = None

    def GetSourcePath(self):
        return self.mSourcename

    def GetSourceFullPath(self):
        path = os.path.dirname(self.GetFilename())
        path = os.path.join(path, self.GetSourcePath())
        return os.path.normpath(path)

    def GetToolCode(self):
        return self.mToolCode

    def GetFamily(self):
        return self.mFamily

    def GetTagName(self):
        return self.mTagName

    def GetFeaturePcd(self):
        return self.mFeaturePcd

    def Parse(self):
        line = self.GetLineByOffset(self._start).strip().split('#')[0]

        arr = line.split('|')

        self.mSourcename = arr[0].strip()
        if len(arr) >= 2:
            self.mFamily = arr[1].strip()
        if len(arr) >= 3:
            self.mTagName = arr[2].strip()
        if len(arr) >= 4:
            self.mToolCode = arr[3].strip()
        if len(arr) >= 5:
            self.mFeaturePcd = arr[4].strip()

        self.mFilename = os.path.basename(self.GetSourceFullPath())
        objdict = INFSourceObject._objs
        if self.mFilename not in objdict:
            objdict[self.mFilename] = [self]
        else:
            objdict[self.mFilename].append(self)

        return True

    def GetName(self):
        return self.mFilename

    def Destroy(self):
        objdict = INFSourceObject._objs
        objdict[self.mFilename].remove(self)
        if len(objdict[self.mFilename]) == 0:
            del objdict[self.mFilename]

    def IsMatchFamily(self, family):
        if family is None:
            return True
        if self.mFamily is not None:
            if family.strip().lower() == self.mFamily.lower():
                return True
            else:
                return False
        else:
            fname = self.GetSourcePath()
            if fname.endswith('.S') and family.lower() != 'gcc':
                return False
            if fname.endswith('.s') and (self.GetArch().lower() != 'ipf' and self.GetArch().lower() != 'common'):
                return False
            if fname.lower().endswith('.asm') and (family.lower() != 'msft' and family.lower() != 'intel'):
                return False
        return True

    @staticmethod
    def GetObjectDict():
        return INFSourceObject._objs

class INFPcdObject(INFSectionObject):
    _objs = {}

    def __init__(self, parent):
        INFSectionObject.__init__(self, parent)

        self.mPcdType      = None
        self.mDefaultValue = None
        self.mPcdName      = None

    @staticmethod
    def GetObjectDict():
        return INFPcdObject._objs

    def Parse(self):
        line = self.GetLineByOffset(self._start).strip().split('#')[0]

        arr = line.split('|')
        self.mPcdName       = arr[0].strip()

        if len(arr) >= 2:
            self.mDefaultValue = arr[1].strip()

        objdict = INFPcdObject._objs
        if self.GetName() in objdict:
            if self not in objdict[self.GetName()]:
                objdict[self.GetName()].append(self)
        else:
            objdict[self.GetName()] = [self]
        return True

    def GetPcdName(self):
        return self.mPcdName

    def GetPcdType(self):
        return self.GetParent().GetType()

    def GetName(self):
        return self.mPcdName.split('.')[1]

    def Destroy(self):
        objdict = INFPcdObject._objs
        objdict[self.GetName()].remove(self)
        if len(objdict[self.GetName()]) == 0:
            del objdict[self.GetName()]

class INFGuidObject(INFSectionObject):
    def __init__(self, parent):
        INFSectionObject.__init__(self, parent)
        self._name = None

    def Parse(self):
        line = self.GetLineByOffset(self._start).strip().split('#')[0].split("|")[0]
        self._name =  line.strip()
        return True

    def GetName(self):
        return self._name


