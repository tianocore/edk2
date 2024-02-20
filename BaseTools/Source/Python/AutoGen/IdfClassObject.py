## @file
# This file is used to collect all defined strings in Image Definition files
#
# Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

##
# Import Modules
#
from __future__ import absolute_import
import Common.EdkLogger as EdkLogger
from Common.BuildToolError import *
from Common.StringUtils import GetLineNo
from Common.Misc import PathClass
from Common.LongFilePathSupport import LongFilePath
import re
import os
from Common.GlobalData import gIdentifierPattern
from .UniClassObject import StripComments

IMAGE_TOKEN = re.compile(r'IMAGE_TOKEN *\(([A-Z0-9_]+) *\)', re.MULTILINE | re.UNICODE)

#
# Value of different image information block types
#
EFI_HII_IIBT_END               = 0x00
EFI_HII_IIBT_IMAGE_1BIT        = 0x10
EFI_HII_IIBT_IMAGE_1BIT_TRANS  = 0x11
EFI_HII_IIBT_IMAGE_4BIT        = 0x12
EFI_HII_IIBT_IMAGE_4BIT_TRANS  = 0x13
EFI_HII_IIBT_IMAGE_8BIT        = 0x14
EFI_HII_IIBT_IMAGE_8BIT_TRANS  = 0x15
EFI_HII_IIBT_IMAGE_24BIT       = 0x16
EFI_HII_IIBT_IMAGE_24BIT_TRANS = 0x17
EFI_HII_IIBT_IMAGE_JPEG        = 0x18
EFI_HII_IIBT_IMAGE_PNG         = 0x19
EFI_HII_IIBT_DUPLICATE         = 0x20
EFI_HII_IIBT_SKIP2             = 0x21
EFI_HII_IIBT_SKIP1             = 0x22
EFI_HII_IIBT_EXT1              = 0x30
EFI_HII_IIBT_EXT2              = 0x31
EFI_HII_IIBT_EXT4              = 0x32

#
# Value of HII package type
#
EFI_HII_PACKAGE_TYPE_ALL           = 0x00
EFI_HII_PACKAGE_TYPE_GUID          = 0x01
EFI_HII_PACKAGE_FORMS              = 0x02
EFI_HII_PACKAGE_STRINGS            = 0x04
EFI_HII_PACKAGE_FONTS              = 0x05
EFI_HII_PACKAGE_IMAGES             = 0x06
EFI_HII_PACKAGE_SIMPLE_FONTS       = 0x07
EFI_HII_PACKAGE_DEVICE_PATH        = 0x08
EFI_HII_PACKAGE_KEYBOARD_LAYOUT    = 0x09
EFI_HII_PACKAGE_ANIMATIONS         = 0x0A
EFI_HII_PACKAGE_END                = 0xDF
EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN  = 0xE0
EFI_HII_PACKAGE_TYPE_SYSTEM_END    = 0xFF

class IdfFileClassObject(object):
    def __init__(self, FileList = []):
        self.ImageFilesDict = {}
        self.ImageIDList = []
        for File in FileList:
            if File is None:
                EdkLogger.error("Image Definition File Parser", PARSER_ERROR, 'No Image definition file is given.')

            try:
                IdfFile = open(LongFilePath(File.Path), mode='r')
                FileIn = IdfFile.read()
                IdfFile.close()
            except:
                EdkLogger.error("build", FILE_OPEN_FAILURE, ExtraData=File)

            ImageFileList = []
            for Line in FileIn.splitlines():
                Line = Line.strip()
                Line = StripComments(Line)
                if len(Line) == 0:
                    continue

                LineNo = GetLineNo(FileIn, Line, False)
                if not Line.startswith('#image '):
                    EdkLogger.error("Image Definition File Parser", PARSER_ERROR, 'The %s in Line %s of File %s is invalid.' % (Line, LineNo, File.Path))

                if Line.find('#image ') >= 0:
                    LineDetails = Line.split()
                    Len = len(LineDetails)
                    if Len != 3 and Len != 4:
                        EdkLogger.error("Image Definition File Parser", PARSER_ERROR, 'The format is not match #image IMAGE_ID [TRANSPARENT] ImageFileName in Line %s of File %s.' % (LineNo, File.Path))
                    if Len == 4 and LineDetails[2] != 'TRANSPARENT':
                        EdkLogger.error("Image Definition File Parser", PARSER_ERROR, 'Please use the keyword "TRANSPARENT" to describe the transparency setting in Line %s of File %s.' % (LineNo, File.Path))
                    MatchString = gIdentifierPattern.match(LineDetails[1])
                    if MatchString is None:
                        EdkLogger.error('Image Definition  File Parser', FORMAT_INVALID, 'The Image token name %s defined in Idf file %s contains the invalid character.' % (LineDetails[1], File.Path))
                    if LineDetails[1] not in self.ImageIDList:
                        self.ImageIDList.append(LineDetails[1])
                    else:
                        EdkLogger.error("Image Definition File Parser", PARSER_ERROR, 'The %s in Line %s of File %s is already defined.' % (LineDetails[1], LineNo, File.Path))
                    if Len == 4:
                        ImageFile = ImageFileObject(LineDetails[Len-1], LineDetails[1], True)
                    else:
                        ImageFile = ImageFileObject(LineDetails[Len-1], LineDetails[1], False)
                    ImageFileList.append(ImageFile)
            if ImageFileList:
                self.ImageFilesDict[File] = ImageFileList

def SearchImageID(ImageFileObject, FileList):
    if FileList == []:
        return ImageFileObject

    for File in FileList:
        if os.path.isfile(File):
            Lines = open(File, 'r')
            for Line in Lines:
                ImageIdList = IMAGE_TOKEN.findall(Line)
                for ID in ImageIdList:
                    EdkLogger.debug(EdkLogger.DEBUG_5, "Found ImageID identifier: " + ID)
                    ImageFileObject.SetImageIDReferenced(ID)

class ImageFileObject(object):
    def __init__(self, FileName, ImageID, TransParent = False):
        self.FileName = FileName
        self.File = ''
        self.ImageID = ImageID
        self.TransParent = TransParent
        self.Referenced = False

    def SetImageIDReferenced(self, ImageID):
        if ImageID == self.ImageID:
            self.Referenced = True
