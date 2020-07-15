## @file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import absolute_import
from .message import *
import re
import os

section_re = re.compile(r'^\[([\w., "]+)\]')

class BaseINIFile(object):
    _objs = {}
    def __new__(cls, *args, **kwargs):
        """Maintain only a single instance of this object
        @return: instance of this class

        """
        if len(args) == 0: return object.__new__(cls)
        filename = args[0]
        parent   = None
        if len(args) > 1:
            parent = args[1]

        key = os.path.normpath(filename)
        if key not in cls._objs.keys():
            cls._objs[key] = object.__new__(cls)

        if parent is not None:
            cls._objs[key].AddParent(parent)

        return cls._objs[key]

    def __init__(self, filename=None, parent=None):
        self._lines    = []
        self._sections = {}
        self._filename = filename
        self._globals  = []
        self._isModify = True

    def AddParent(self, parent):
        if parent is None: return
        if not hasattr(self, "_parents"):
            self._parents = []

        if parent in self._parents:
            ErrorMsg("Duplicate parent is found for INI file %s" % self._filename)
            return
        self._parents.append(parent)

    def GetFilename(self):
        return os.path.normpath(self._filename)

    def IsModified(self):
        return self._isModify

    def Modify(self, modify=True, obj=None):
        if modify == self._isModify: return
        self._isModify = modify
        if modify:
            for parent in self._parents:
                parent.Modify(True, self)

    def _ReadLines(self, filename):
        #
        # try to open file
        #
        if not os.path.exists(filename):
            return False

        try:
            handle = open(filename, 'r')
            self._lines  = handle.readlines()
            handle.close()
        except:
            raise EdkException("Fail to open file %s" % filename)

        return True

    def GetSectionInstance(self, parent, name, isCombined=False):
        return BaseINISection(parent, name, isCombined)

    def GetSectionByName(self, name):
        arr = []
        for key in self._sections.keys():
            if '.private' in key:
                continue
            for item in self._sections[key]:
                if item.GetBaseName().lower().find(name.lower()) != -1:
                    arr.append(item)
        return arr

    def GetSectionObjectsByName(self, name):
        arr = []
        sects = self.GetSectionByName(name)
        for sect in sects:
            for obj in sect.GetObjects():
                arr.append(obj)
        return arr

    def Parse(self):
        if not self._isModify: return True
        if not self._ReadLines(self._filename): return False

        sObjs    = []
        inGlobal = True
        # process line
        for index in range(len(self._lines)):
            templine = self._lines[index].strip()
            # skip comments
            if len(templine) == 0: continue
            if re.match("^\[=*\]", templine) or re.match("^#", templine) or \
               re.match("\*+/", templine):
                continue

            m = section_re.match(templine)
            if m is not None: # found a section
                inGlobal = False
                # Finish the latest section first
                if len(sObjs) != 0:
                    for sObj in sObjs:
                        sObj._end = index - 1
                        if not sObj.Parse():
                            ErrorMsg("Fail to parse section %s" % sObj.GetBaseName(),
                                     self._filename,
                                     sObj._start)

                # start new section
                sname_arr = m.groups()[0].split(',')
                sObjs = []
                for name in sname_arr:
                    sObj = self.GetSectionInstance(self, name, (len(sname_arr) > 1))
                    sObj._start = index
                    sObjs.append(sObj)
                    if name.lower() not in self._sections:
                        self._sections[name.lower()] = [sObj]
                    else:
                        self._sections[name.lower()].append(sObj)
            elif inGlobal:  # not start any section and find global object
                gObj = BaseINIGlobalObject(self)
                gObj._start = index
                gObj.Parse()
                self._globals.append(gObj)

        # Finish the last section
        if len(sObjs) != 0:
            for sObj in sObjs:
                sObj._end = index
                if not sObj.Parse():
                    ErrorMsg("Fail to parse section %s" % sObj.GetBaseName(),
                             self._filename,
                             sObj._start)

        self._isModify = False
        return True

    def Destroy(self, parent):

        # check referenced parent
        if parent is not None:
            assert parent in self._parents, "when destory ini object, can not found parent reference!"
            self._parents.remove(parent)

        if len(self._parents) != 0: return

        for sects in self._sections.values():
            for sect in sects:
                sect.Destroy()

        # dereference from _objs array
        assert self.GetFilename() in self._objs.keys(), "When destroy ini object, can not find obj reference!"
        assert self in self._objs.values(), "When destroy ini object, can not find obj reference!"
        del self._objs[self.GetFilename()]

        # dereference self
        self.Clear()

    def GetDefine(self, name):
        sects = self.GetSectionByName('Defines')
        for sect in sects:
            for obj in sect.GetObjects():
                line = obj.GetLineByOffset(obj._start).split('#')[0].strip()
                arr = line.split('=')
                if arr[0].strip().lower() == name.strip().lower():
                    return arr[1].strip()
        return None

    def Clear(self):
        for sects in self._sections.values():
            for sect in sects:
                del sect
        self._sections.clear()
        for gObj in self._globals:
            del gObj

        del self._globals[:]
        del self._lines[:]

    def Reload(self):
        self.Clear()
        ret = self.Parse()
        if ret:
            self._isModify = False
        return ret

    def AddNewSection(self, sectName):
        if sectName.lower() in self._sections.keys():
            ErrorMsg('Section %s can not be created for conflict with existing section')
            return None

        sectionObj = self.GetSectionInstance(self, sectName)
        sectionObj._start = len(self._lines)
        sectionObj._end   = len(self._lines) + 1
        self._lines.append('[%s]\n' % sectName)
        self._lines.append('\n\n')
        self._sections[sectName.lower()] = sectionObj
        return sectionObj

    def CopySectionsByName(self, oldDscObj, nameStr):
        sects = oldDscObj.GetSectionByName(nameStr)
        for sect in sects:
            sectObj = self.AddNewSection(sect.GetName())
            sectObj.Copy(sect)

    def __str__(self):
        return ''.join(self._lines)

    ## Get file header's comment from basic INI file.
    #  The file comments has two style:
    #  1) #/** @file
    #  2) ## @file
    #
    def GetFileHeader(self):
        desc = []
        lineArr  = self._lines
        inHeader = False
        for num in range(len(self._lines)):
            line = lineArr[num].strip()
            if not inHeader and (line.startswith("#/**") or line.startswith("##")) and \
                line.find("@file") != -1:
                inHeader = True
                continue
            if inHeader and (line.startswith("#**/") or line.startswith('##')):
                inHeader = False
                break
            if inHeader:
                prefixIndex = line.find('#')
                if prefixIndex == -1:
                    desc.append(line)
                else:
                    desc.append(line[prefixIndex + 1:])
        return '<br>\n'.join(desc)

class BaseINISection(object):
    def __init__(self, parent, name, isCombined=False):
        self._parent     = parent
        self._name       = name
        self._isCombined = isCombined
        self._start      = 0
        self._end        = 0
        self._objs       = []

    def __del__(self):
        for obj in self._objs:
            del obj
        del self._objs[:]

    def GetName(self):
        return self._name

    def GetObjects(self):
        return self._objs

    def GetParent(self):
        return self._parent

    def GetStartLinenumber(self):
        return self._start

    def GetEndLinenumber(self):
        return self._end

    def GetLine(self, linenumber):
        return self._parent._lines[linenumber]

    def GetFilename(self):
        return self._parent.GetFilename()

    def GetSectionINIObject(self, parent):
        return BaseINISectionObject(parent)

    def Parse(self):
        # skip first line in section, it is used by section name
        visit = self._start + 1
        iniObj = None
        while (visit <= self._end):
            line = self.GetLine(visit).strip()
            if re.match("^\[=*\]", line) or re.match("^#", line) or len(line) == 0:
                visit += 1
                continue
            line = line.split('#')[0].strip()
            if iniObj is not None:
                if line.endswith('}'):
                    iniObj._end = visit - self._start
                    if not iniObj.Parse():
                        ErrorMsg("Fail to parse ini object",
                                 self.GetFilename(),
                                 iniObj.GetStartLinenumber())
                    else:
                        self._objs.append(iniObj)
                    iniObj = None
            else:
                iniObj = self.GetSectionINIObject(self)
                iniObj._start = visit - self._start
                if not line.endswith('{'):
                    iniObj._end = visit - self._start
                    if not iniObj.Parse():
                        ErrorMsg("Fail to parse ini object",
                                 self.GetFilename(),
                                 iniObj.GetStartLinenumber())
                    else:
                        self._objs.append(iniObj)
                    iniObj = None
            visit += 1
        return True

    def Destroy(self):
        for obj in self._objs:
            obj.Destroy()

    def GetBaseName(self):
        return self._name

    def AddLine(self, line):
        end = self.GetEndLinenumber()
        self._parent._lines.insert(end, line)
        self._end += 1

    def Copy(self, sectObj):
        index = sectObj.GetStartLinenumber() + 1
        while index < sectObj.GetEndLinenumber():
            line = sectObj.GetLine(index)
            if not line.strip().startswith('#'):
                self.AddLine(line)
            index += 1

    def AddObject(self, obj):
        lines = obj.GenerateLines()
        for line in lines:
            self.AddLine(line)

    def GetComment(self):
        comments = []
        start  = self._start - 1
        bFound = False

        while (start > 0):
            line = self.GetLine(start).strip()
            if len(line) == 0:
                start -= 1
                continue
            if line.startswith('##'):
                bFound = True
                index = line.rfind('#')
                if (index + 1) < len(line):
                    comments.append(line[index + 1:])
                break
            if line.startswith('#'):
                start -= 1
                continue
            break
        if bFound:
            end = start + 1
            while (end < self._start):
                line = self.GetLine(end).strip()
                if len(line) == 0: break
                if not line.startswith('#'): break
                index = line.rfind('#')
                if (index + 1) < len(line):
                    comments.append(line[index + 1:])
                end += 1
        return comments

class BaseINIGlobalObject(object):
    def __init__(self, parent):
        self._start = 0
        self._end   = 0

    def Parse(self):
        return True

    def __str__(self):
        return parent._lines[self._start]

    def __del__(self):
        pass

class BaseINISectionObject(object):
    def __init__(self, parent):
        self._start  = 0
        self._end    = 0
        self._parent = parent

    def __del__(self):
        self._parent = None

    def GetParent(self):
        return self._parent

    def GetFilename(self):
        return self.GetParent().GetFilename()

    def GetPackageName(self):
        return self.GetFilename()

    def GetFileObj(self):
        return self.GetParent().GetParent()

    def GetStartLinenumber(self):
        return self.GetParent()._start + self._start

    def GetLineByOffset(self, offset):
        sect_start = self._parent.GetStartLinenumber()
        linenumber = sect_start + offset
        return self._parent.GetLine(linenumber)

    def GetLinenumberByOffset(self, offset):
        return offset + self._parent.GetStartLinenumber()

    def Parse(self):
        return True

    def Destroy(self):
        pass

    def __str__(self):
        return self.GetLineByOffset(self._start).strip()

    def GenerateLines(self):
        return ['default setion object string\n']

    def GetComment(self):
        comments = []
        start  = self.GetStartLinenumber() - 1
        bFound = False

        while (start > 0):
            line = self.GetParent().GetLine(start).strip()
            if len(line) == 0:
                start -= 1
                continue
            if line.startswith('##'):
                bFound = True
                index = line.rfind('#')
                if (index + 1) < len(line):
                    comments.append(line[index + 1:])
                break
            if line.startswith('#'):
                start -= 1
                continue
            break
        if bFound:
            end = start + 1
            while (end <= self.GetStartLinenumber() - 1):
                line = self.GetParent().GetLine(end).strip()
                if len(line) == 0: break
                if not line.startswith('#'): break
                index = line.rfind('#')
                if (index + 1) < len(line):
                    comments.append(line[index + 1:])
                end += 1
        return comments
