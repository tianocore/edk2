## @file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from plugins.EdkPlugins.basemodel import ini
import re, os
from plugins.EdkPlugins.basemodel.message import *

class DSCFile(ini.BaseINIFile):
    def GetSectionInstance(self, parent, name, isCombined=False):
        return DSCSection(parent, name, isCombined)

    def GetComponents(self):
        return self.GetSectionObjectsByName('Components')

class DSCSection(ini.BaseINISection):
    def GetSectionINIObject(self, parent):
        type = self.GetType()

        if type.lower() == 'components':
            return DSCComponentObject(self)
        if type.lower() == 'libraryclasses':
            return DSCLibraryClassObject(self)
        if type.lower() == 'defines':
            return ini.BaseINISectionObject(self)
        if type.lower() == 'pcdsfeatureflag' or \
           type.lower() == 'pcdsfixedatbuild' or \
           type.lower() == 'pcdspatchableinmodule' or\
           type.lower() == 'pcdsdynamicdefault' or \
           type.lower() == 'pcdsdynamicex' or \
           type.lower() == 'pcdsdynamichii' or \
           type.lower() == 'pcdsdynamicvpd':
            return DSCPcdObject(self)

        return DSCSectionObject(self)

    def GetType(self):
        arr = self._name.split('.')
        return arr[0].strip()

    def GetArch(self):
        arr = self._name.split('.')
        if len(arr) == 1:
            return 'common'
        return arr[1]

    def GetModuleType(self):
        arr = self._name.split('.')
        if len(arr) < 3:
            return 'common'
        return arr[2]

class DSCSectionObject(ini.BaseINISectionObject):
    def GetArch(self):
        return self.GetParent().GetArch()

class DSCPcdObject(DSCSectionObject):

    def __init__(self, parent):
        ini.BaseINISectionObject.__init__(self, parent)
        self._name = None

    def Parse(self):
        line = self.GetLineByOffset(self._start).strip().split('#')[0]
        self._name   = line.split('|')[0]
        self._value  = line.split('|')[1]
        return True

    def GetPcdName(self):
        return self._name

    def GetPcdType(self):
        return self.GetParent().GetType()

    def GetPcdValue(self):
        return self._value

class DSCLibraryClassObject(DSCSectionObject):
    def __init__(self, parent):
        ini.BaseINISectionObject.__init__(self, parent)

    def GetClass(self):
        line = self.GetLineByOffset(self._start)
        return line.split('#')[0].split('|')[0].strip()

    def GetInstance(self):
        line = self.GetLineByOffset(self._start)
        return line.split('#')[0].split('|')[1].strip()

    def GetArch(self):
        return self.GetParent().GetArch()

    def GetModuleType(self):
        return self.GetParent().GetModuleType()

class DSCComponentObject(DSCSectionObject):

    def __init__(self, parent):
        ini.BaseINISectionObject.__init__(self, parent)
        self._OveridePcds      = {}
        self._OverideLibraries = {}
        self._Filename         = ''

    def __del__(self):
        self._OverideLibraries.clear()
        self._OverideLibraries.clear()
        ini.BaseINISectionObject.__del__(self)

    def AddOverideLib(self, libclass, libinstPath):
        if libclass not in self._OverideLibraries.keys():
            self._OverideLibraries[libclass] = libinstPath

    def AddOveridePcd(self, name, type, value=None):
        if type not in self._OveridePcds.keys():
            self._OveridePcds[type] = []
        self._OveridePcds[type].append((name, value))

    def GetOverideLibs(self):
        return self._OverideLibraries

    def GetArch(self):
        return self.GetParent().GetArch()

    def GetOveridePcds(self):
        return self._OveridePcds

    def GetFilename(self):
        return self.GetLineByOffset(self._start).split('#')[0].split('{')[0].strip()

    def SetFilename(self, fName):
        self._Filename = fName

    def Parse(self):
        if (self._start < self._end):
            #
            # The first line is inf path and could be ignored
            # The end line is '}' and could be ignored
            #
            curr = self._start + 1
            end  = self._end - 1
            OverideName = ''
            while (curr <= end):
                line = self.GetLineByOffset(curr).strip()
                if len(line) > 0 and line[0] != '#':
                    line = line.split('#')[0].strip()
                    if line[0] == '<':
                        OverideName = line[1:len(line)-1]
                    elif OverideName.lower() == 'libraryclasses':
                        arr = line.split('|')
                        self._OverideLibraries[arr[0].strip()] = arr[1].strip()
                    elif OverideName.lower() == 'pcds':
                        ErrorMsg('EDES does not support PCD overide',
                                 self.GetFileName(),
                                 self.GetParent().GetLinenumberByOffset(curr))
                curr = curr + 1
        return True

    def GenerateLines(self):
        lines = []
        hasLib = False
        hasPcd = False
        if len(self._OverideLibraries) != 0:
            hasLib = True
        if len(self._OveridePcds) != 0:
            hasPcd = True

        if hasLib or hasPcd:
            lines.append(('  %s {\n' % self._Filename))
        else:
            lines.append(('  %s \n' % self._Filename))
            return lines

        if hasLib:
            lines.append('    <LibraryClasses>\n')
            for libKey in self._OverideLibraries.keys():
                lines.append('      %s|%s\n' % (libKey, self._OverideLibraries[libKey]))

        if hasPcd:
            for key in self._OveridePcds.keys():
                lines.append('    <%s>\n' % key)

                for name, value in self._OveridePcds[key]:
                    if value is not None:
                        lines.append('      %s|%s\n' % (name, value))
                    else:
                        lines.append('      %s\n' % name)

        if hasLib or hasPcd:
            lines.append('  }\n')

        return lines

