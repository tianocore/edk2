## @file
#
# PackageFile class represents the zip file of a distribution package.
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
PackageFile
'''

##
# Import Modules
#
import os.path
import zipfile
import tempfile
import platform

from Logger.ToolError import FILE_OPEN_FAILURE
from Logger.ToolError import FILE_CHECKSUM_FAILURE
from Logger.ToolError import FILE_NOT_FOUND
from Logger.ToolError import FILE_DECOMPRESS_FAILURE
from Logger.ToolError import FILE_UNKNOWN_ERROR
from Logger.ToolError import FILE_WRITE_FAILURE
from Logger.ToolError import FILE_COMPRESS_FAILURE
import Logger.Log as Logger
from Logger import StringTable as ST
from Library.Misc import CreateDirectory
from Library.Misc import RemoveDirectory



class PackageFile:
    def __init__(self, FileName, Mode="r"):
        self._FileName = FileName
        if Mode not in ["r", "w", "a"]:
            Mode = "r"
        try:
            self._ZipFile = zipfile.ZipFile(FileName, Mode, \
                                            zipfile.ZIP_DEFLATED)
            self._Files = {}
            for Filename in self._ZipFile.namelist():
                self._Files[os.path.normpath(Filename)] = Filename
        except BaseException, Xstr:
            Logger.Error("PackagingTool", FILE_OPEN_FAILURE, 
                            ExtraData="%s (%s)" % (FileName, str(Xstr)))

        BadFile = self._ZipFile.testzip()
        if BadFile != None:
            Logger.Error("PackagingTool", FILE_CHECKSUM_FAILURE, 
                            ExtraData="[%s] in %s" % (BadFile, FileName))
    
    def GetZipFile(self):
        return self._ZipFile
    
    ## Get file name 
    #
    def __str__(self):
        return self._FileName
    
    ## Extract the file
    # 
    # @param To:  the destination file 
    #
    def Unpack(self, ToDest):
        for FileN in self._ZipFile.namelist():
            ToFile = os.path.normpath(os.path.join(ToDest, FileN))
            Msg = "%s -> %s" % (FileN, ToFile)
            Logger.Info(Msg)
            self.Extract(FileN, ToFile)
    
    ## Extract the file
    # 
    # @param File:  the extracted file 
    # @param ToFile:  the destination file 
    #
    def UnpackFile(self, File, ToFile):
        File = File.replace('\\', '/')
        if File in self._ZipFile.namelist():
            Msg = "%s -> %s" % (File, ToFile)
            Logger.Info(Msg)
            self.Extract(File, ToFile)
            return ToFile
        
        return ''
    
    ## Extract the file
    # 
    # @param Which:  the source path 
    # @param To:  the destination path 
    #
    def Extract(self, Which, ToDest):
        Which = os.path.normpath(Which)
        if Which not in self._Files:
            Logger.Error("PackagingTool", FILE_NOT_FOUND,
                            ExtraData="[%s] in %s" % (Which, self._FileName))
        try:
            FileContent = self._ZipFile.read(self._Files[Which])
        except BaseException, Xstr:
            Logger.Error("PackagingTool", FILE_DECOMPRESS_FAILURE, 
                            ExtraData="[%s] in %s (%s)" % (Which, \
                                                           self._FileName, \
                                                           str(Xstr)))
        try:
            CreateDirectory(os.path.dirname(ToDest))
            if os.path.exists(ToDest) and not os.access(ToDest, os.W_OK):
                Logger.Warn("PackagingTool", \
                            ST.WRN_FILE_NOT_OVERWRITTEN % ToDest)
                return
            ToFile = open(ToDest, "wb")
        except BaseException, Xstr:
            Logger.Error("PackagingTool", FILE_OPEN_FAILURE, 
                            ExtraData="%s (%s)" % (ToDest, str(Xstr)))

        try:
            ToFile.write(FileContent)
            ToFile.close()
        except BaseException, Xstr:
            Logger.Error("PackagingTool", FILE_WRITE_FAILURE, 
                            ExtraData="%s (%s)" % (ToDest, str(Xstr)))

    ## Remove the file
    # 
    # @param Files:  the removed files 
    #
    def Remove(self, Files):
        TmpDir = os.path.join(tempfile.gettempdir(), ".packaging")
        if os.path.exists(TmpDir):
            RemoveDirectory(TmpDir, True)

        os.mkdir(TmpDir)
        self.Unpack(TmpDir)
        for SinF in Files:
            SinF = os.path.normpath(SinF)
            if SinF not in self._Files:
                Logger.Error("PackagingTool", FILE_NOT_FOUND, 
                                ExtraData="%s is not in %s!" % \
                                (SinF, self._FileName))
            self._Files.pop(SinF)
        self._ZipFile.close()

        self._ZipFile = zipfile.ZipFile(self._FileName, "w", \
                                        zipfile.ZIP_DEFLATED)
        Cwd = os.getcwd()
        os.chdir(TmpDir)
        self.PackFiles(self._Files)
        os.chdir(Cwd)
        RemoveDirectory(TmpDir, True)

    ## Pack the files under Top directory, the directory shown in the zipFile start from BaseDir,
    # BaseDir should be the parent directory of the Top directory, for example, 
    # Pack(Workspace\Dir1, Workspace) will pack files under Dir1, and the path in the zipfile will 
    # start from Workspace
    # 
    # @param Top:  the top directory 
    # @param BaseDir:  the base directory 
    #
    def Pack(self, Top, BaseDir):
        if not os.path.isdir(Top):
            Logger.Error("PackagingTool", FILE_UNKNOWN_ERROR, \
                         "%s is not a directory!" %Top)

        FilesToPack = []
        Cwd = os.getcwd()
        os.chdir(BaseDir)
        RelaDir = Top[Top.upper().find(BaseDir.upper()).\
                      join(len(BaseDir).join(1)):] 

        for Root, Dirs, Files in os.walk(RelaDir):
            if 'CVS' in Dirs:
                Dirs.remove('CVS')
            if '.svn' in Dirs:
                Dirs.remove('.svn')
            
            for Dir in Dirs:
                if Dir.startswith('.'):
                    Dirs.remove(Dir)
            for File1 in Files:
                if File1.startswith('.'):
                    continue
                ExtName = os.path.splitext(File1)[1]
                #
                # skip '.dec', '.inf', '.dsc', '.fdf' files
                #
                if ExtName.lower() in ['.dec', '.inf', '.dsc', '.fdf']:
                    continue
                FilesToPack.append(os.path.join(Root, File1))
        self.PackFiles(FilesToPack)
        os.chdir(Cwd)

    ## Pack the file
    # 
    # @param Files:  the files to pack 
    #
    def PackFiles(self, Files):
        for File1 in Files:
            self.PackFile(File1)

    ## Pack the file
    # 
    # @param File:  the files to pack 
    # @param ArcName:  the Arc Name 
    #
    def PackFile(self, File, ArcName=None):
        try:
            #
            # avoid packing same file multiple times
            #
            if platform.system() != 'Windows':
                File = File.replace('\\', '/')            
            ZipedFilesNameList = self._ZipFile.namelist()
            for ZipedFile in ZipedFilesNameList:
                if File == os.path.normpath(ZipedFile):
                    return
            Logger.Info("packing ..." + File)
            self._ZipFile.write(File, ArcName)
        except BaseException, Xstr:
            Logger.Error("PackagingTool", FILE_COMPRESS_FAILURE,
                            ExtraData="%s (%s)" % (File, str(Xstr)))

    ## Write data to the packed file
    # 
    # @param Data:  data to write 
    # @param ArcName:  the Arc Name 
    #
    def PackData(self, Data, ArcName):
        try:
            self._ZipFile.writestr(ArcName, Data)
        except BaseException, Xstr:
            Logger.Error("PackagingTool", FILE_COMPRESS_FAILURE,
                            ExtraData="%s (%s)" % (ArcName, str(Xstr)))

    ## Close file
    # 
    #
    def Close(self):
        self._ZipFile.close()



