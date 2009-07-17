## @file
#
# PackageFile class represents the zip file of a distribution package.
#
# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
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
import os
import sys
import zipfile
import tempfile

from Common import EdkLogger
from Common.Misc import *
from Common.BuildToolError import *

class PackageFile:
    def __init__(self, FileName, Mode="r"):
        self._FileName = FileName
        if Mode not in ["r", "w", "a"]:
            Mode = "r"
        try:
            self._ZipFile = zipfile.ZipFile(FileName, Mode, zipfile.ZIP_DEFLATED)
            self._Files = {}
            for F in self._ZipFile.namelist():
                self._Files[os.path.normpath(F)] = F
        except BaseException, X:
            EdkLogger.error("PackagingTool", FILE_OPEN_FAILURE, 
                            ExtraData="%s (%s)" % (FileName, str(X)))

        BadFile = self._ZipFile.testzip()
        if BadFile != None:
            EdkLogger.error("PackagingTool", FILE_CHECKSUM_FAILURE, 
                            ExtraData="[%s] in %s" % (BadFile, FileName))

    def __str__(self):
        return self._FileName

    def Unpack(self, To):
        for F in self._ZipFile.namelist():
            ToFile = os.path.normpath(os.path.join(To, F))
            print F, "->", ToFile
            self.Extract(F, ToFile)
    
    def UnpackFile(self, File, ToFile):
        File = File.replace('\\', '/')
        if File in self._ZipFile.namelist():
            print File, "->", ToFile
            self.Extract(File, ToFile)
            
            return ToFile
        
        return ''
    
    def Extract(self, Which, To):
        Which = os.path.normpath(Which)
        if Which not in self._Files:
            EdkLogger.error("PackagingTool", FILE_NOT_FOUND,
                            ExtraData="[%s] in %s" % (Which, self._FileName))
        try:
            FileContent = self._ZipFile.read(self._Files[Which])
        except BaseException, X:
            EdkLogger.error("PackagingTool", FILE_DECOMPRESS_FAILURE, 
                            ExtraData="[%s] in %s (%s)" % (Which, self._FileName, str(X)))
        try:
            CreateDirectory(os.path.dirname(To))
            ToFile = open(To, "wb")
        except BaseException, X:
            EdkLogger.error("PackagingTool", FILE_OPEN_FAILURE, 
                            ExtraData="%s (%s)" % (To, str(X)))

        try:
            ToFile.write(FileContent)
            ToFile.close()
        except BaseException, X:
            EdkLogger.error("PackagingTool", FILE_WRITE_FAILURE, 
                            ExtraData="%s (%s)" % (To, str(X)))

    def Remove(self, Files):
        TmpDir = os.path.join(tempfile.gettempdir(), ".packaging")
        if os.path.exists(TmpDir):
            RemoveDirectory(TmpDir, True)

        os.mkdir(TmpDir)
        self.Unpack(TmpDir)
        for F in Files:
            F = os.path.normpath(F)
            if F not in self._Files:
                EdkLogger.error("PackagingTool", FILE_NOT_FOUND, 
                                ExtraData="%s is not in %s!" % (F, self._FileName))
            #os.remove(os.path.join(TmpDir, F))  # no need to really remove file
            self._Files.pop(F)
        self._ZipFile.close()

        self._ZipFile = zipfile.ZipFile(self._FileName, "w", zipfile.ZIP_DEFLATED)
        Cwd = os.getcwd()
        os.chdir(TmpDir)
        self.PackFiles(self._Files)
        os.chdir(Cwd)
        RemoveDirectory(TmpDir, True)

    def Pack(self, Top):
        if not os.path.isdir(Top):
            EdkLogger.error("PackagingTool", FILE_UNKNOWN_ERROR, "%s is not a directory!" %Top)

        FilesToPack = []
        ParentDir = os.path.dirname(Top)
        BaseDir = os.path.basename(Top)
        Cwd = os.getcwd()
        os.chdir(ParentDir)
        for Root, Dirs, Files in os.walk(BaseDir):
            if 'CVS' in Dirs:
                Dirs.remove('CVS')
            if '.svn' in Dirs:
                Dirs.remove('.svn')
            for F in Files:
                FilesToPack.append(os.path.join(Root, F))
        self.PackFiles(FilesToPack)
        os.chdir(Cwd)

    def PackFiles(self, Files):
        for F in Files:
            try:
                print "packing ...", F
                self._ZipFile.write(F)
            except BaseException, X:
                EdkLogger.error("PackagingTool", FILE_COMPRESS_FAILURE, 
                                ExtraData="%s (%s)" % (F, str(X)))

    def PackFile(self, File, ArcName=None):
        try:
            print "packing ...", File
            self._ZipFile.write(File, ArcName)
        except BaseException, X:
            EdkLogger.error("PackagingTool", FILE_COMPRESS_FAILURE,
                            ExtraData="%s (%s)" % (File, str(X)))

    def PackData(self, Data, ArcName):
        try:
            self._ZipFile.writestr(ArcName, Data)
        except BaseException, X:
            EdkLogger.error("PackagingTool", FILE_COMPRESS_FAILURE,
                            ExtraData="%s (%s)" % (ArcName, str(X)))

    def Close(self):
        self._ZipFile.close()

if __name__ == '__main__':
    pass

