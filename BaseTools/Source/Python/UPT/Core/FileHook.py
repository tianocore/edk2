## @file
# This file hooks file and directory creation and removal
#
# Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
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
File hook
'''

import os
import stat
import time
import zipfile
from time import sleep
from Library import GlobalData

__built_in_remove__ = os.remove
__built_in_mkdir__  = os.mkdir
__built_in_rmdir__  = os.rmdir
__built_in_chmod__  = os.chmod
__built_in_open__   = open

_RMFILE      = 0
_MKFILE      = 1
_RMDIR       = 2
_MKDIR       = 3
_CHMOD       = 4

gBACKUPFILE = 'file.backup'
gEXCEPTION_LIST = ['Conf'+os.sep+'DistributionPackageDatabase.db', '.tmp', gBACKUPFILE]

class _PathInfo:
    def __init__(self, action, path, mode=-1):
        self.action = action
        self.path = path
        self.mode = mode

class RecoverMgr:
    def __init__(self, workspace):
        self.rlist = []
        self.zip = None
        self.workspace = os.path.normpath(workspace)
        self.backupfile = gBACKUPFILE
        self.zipfile = os.path.join(self.workspace, gBACKUPFILE)

    def _createzip(self):
        if self.zip:
            return
        self.zip = zipfile.ZipFile(self.zipfile, 'w', zipfile.ZIP_DEFLATED)

    def _save(self, tmp, path):
        if not self._tryhook(path):
            return
        self.rlist.append(_PathInfo(tmp, path))

    def bkrmfile(self, path):
        arc = self._tryhook(path)
        if arc and os.path.isfile(path):
            self._createzip()
            self.zip.write(path, arc.encode('utf_8'))
            sta = os.stat(path)
            oldmode = stat.S_IMODE(sta.st_mode)
            self.rlist.append(_PathInfo(_CHMOD, path, oldmode))
            self.rlist.append(_PathInfo(_RMFILE, path))
        __built_in_remove__(path)

    def bkmkfile(self, path, mode, bufsize):
        if not os.path.exists(path):
            self._save(_MKFILE, path)
        return __built_in_open__(path, mode, bufsize)

    def bkrmdir(self, path):
        if os.path.exists(path):
            sta = os.stat(path)
            oldmode = stat.S_IMODE(sta.st_mode)
            self.rlist.append(_PathInfo(_CHMOD, path, oldmode))
            self._save(_RMDIR, path)
        __built_in_rmdir__(path)

    def bkmkdir(self, path, mode):
        if not os.path.exists(path):
            self._save(_MKDIR, path)
        __built_in_mkdir__(path, mode)

    def bkchmod(self, path, mode):
        if self._tryhook(path) and os.path.exists(path):
            sta = os.stat(path)
            oldmode = stat.S_IMODE(sta.st_mode)
            self.rlist.append(_PathInfo(_CHMOD, path, oldmode))
        __built_in_chmod__(path, mode)

    def rollback(self):
        if self.zip:
            self.zip.close()
            self.zip = None
        index = len(self.rlist) - 1
        while index >= 0:
            item = self.rlist[index]
            exist = os.path.exists(item.path)
            if item.action == _MKFILE and exist:
                #if not os.access(item.path, os.W_OK):
                #    os.chmod(item.path, S_IWUSR)
                __built_in_remove__(item.path)
            elif item.action == _RMFILE and not exist:
                if not self.zip:
                    self.zip = zipfile.ZipFile(self.zipfile, 'r', zipfile.ZIP_DEFLATED)
                arcname = os.path.normpath(item.path)
                arcname = arcname[len(self.workspace)+1:].encode('utf_8')
                if os.sep != "/" and os.sep in arcname:
                    arcname = arcname.replace(os.sep, '/')
                mtime = self.zip.getinfo(arcname).date_time
                content = self.zip.read(arcname)
                filep = __built_in_open__(item.path, "wb")
                filep.write(content)
                filep.close()
                intime = time.mktime(mtime + (0, 0, 0))
                os.utime(item.path, (intime, intime))
            elif item.action == _MKDIR and exist:
                while True:
                    try:
                        __built_in_rmdir__(item.path)
                        break
                    except IOError:
                        # Sleep a short time and try again
                        # The anti-virus software may delay the file removal in this directory
                        sleep(0.1)
            elif item.action == _RMDIR and not exist:
                __built_in_mkdir__(item.path)
            elif item.action == _CHMOD and exist:
                try:
                    __built_in_chmod__(item.path, item.mode)
                except EnvironmentError:
                    pass
            index -= 1
        self.commit()

    def commit(self):
        if self.zip:
            self.zip.close()
            __built_in_remove__(self.zipfile)

    # Check if path needs to be hooked
    def _tryhook(self, path):
        path = os.path.normpath(path)
        works = self.workspace if str(self.workspace).endswith(os.sep) else (self.workspace  + os.sep)
        if not path.startswith(works):
            return ''
        for exceptdir in gEXCEPTION_LIST:
            full = os.path.join(self.workspace, exceptdir)
            if full == path or path.startswith(full + os.sep) or os.path.split(full)[0] == path:
                return ''
        return path[len(self.workspace)+1:]

def _hookrm(path):
    if GlobalData.gRECOVERMGR:
        GlobalData.gRECOVERMGR.bkrmfile(path)
    else:
        __built_in_remove__(path)

def _hookmkdir(path, mode=0777):
    if GlobalData.gRECOVERMGR:
        GlobalData.gRECOVERMGR.bkmkdir(path, mode)
    else:
        __built_in_mkdir__(path, mode)

def _hookrmdir(path):
    if GlobalData.gRECOVERMGR:
        GlobalData.gRECOVERMGR.bkrmdir(path)
    else:
        __built_in_rmdir__(path)

def _hookmkfile(path, mode='r', bufsize=-1):
    if GlobalData.gRECOVERMGR:
        return GlobalData.gRECOVERMGR.bkmkfile(path, mode, bufsize)
    return __built_in_open__(path, mode, bufsize)

def _hookchmod(path, mode):
    if GlobalData.gRECOVERMGR:
        GlobalData.gRECOVERMGR.bkchmod(path, mode)
    else:
        __built_in_chmod__(path, mode)

def SetRecoverMgr(mgr):
    GlobalData.gRECOVERMGR = mgr

os.remove   = _hookrm
os.mkdir    = _hookmkdir
os.rmdir    = _hookrmdir
os.chmod    = _hookchmod
__FileHookOpen__    = _hookmkfile
