## @file
# generate capsule
#
#  Copyright (c) 2007-2013, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import Ffs
from GenFdsGlobalVariable import GenFdsGlobalVariable
import StringIO
from struct import pack
import os
from Common.Misc import SaveFileOnChange

## base class for capsule data
#
#
class CapsuleData:
    ## The constructor
    #
    #   @param  self        The object pointer
    def __init__(self):
        pass
    
    ## generate capsule data
    #
    #   @param  self        The object pointer
    def GenCapsuleSubItem(self):
        pass
        
## FFS class for capsule data
#
#
class CapsuleFfs (CapsuleData):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self) :
        self.Ffs = None
        self.FvName = None

    ## generate FFS capsule data
    #
    #   @param  self        The object pointer
    #   @retval string      Generated file name
    #
    def GenCapsuleSubItem(self):
        FfsFile = self.Ffs.GenFfs()
        return FfsFile

## FV class for capsule data
#
#
class CapsuleFv (CapsuleData):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self) :
        self.Ffs = None
        self.FvName = None
        self.CapsuleName = None

    ## generate FV capsule data
    #
    #   @param  self        The object pointer
    #   @retval string      Generated file name
    #
    def GenCapsuleSubItem(self):
        if self.FvName.find('.fv') == -1:
            if self.FvName.upper() in GenFdsGlobalVariable.FdfParser.Profile.FvDict.keys():
                FvObj = GenFdsGlobalVariable.FdfParser.Profile.FvDict.get(self.FvName.upper())
                FdBuffer = StringIO.StringIO('')
                FvObj.CapsuleName = self.CapsuleName
                FvFile = FvObj.AddToBuffer(FdBuffer)
                FvObj.CapsuleName = None
                FdBuffer.close()
                return FvFile
        else:
            FvFile = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.FvName)
            return FvFile

## FD class for capsule data
#
#
class CapsuleFd (CapsuleData):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self) :
        self.Ffs = None
        self.FdName = None
        self.CapsuleName = None

    ## generate FD capsule data
    #
    #   @param  self        The object pointer
    #   @retval string      Generated file name
    #
    def GenCapsuleSubItem(self):
        if self.FdName.find('.fd') == -1:
            if self.FdName.upper() in GenFdsGlobalVariable.FdfParser.Profile.FdDict.keys():
                FdObj = GenFdsGlobalVariable.FdfParser.Profile.FdDict.get(self.FdName.upper())
                FdFile = FdObj.GenFd()
                return FdFile
        else:
            FdFile = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.FdName)
            return FdFile
        
## AnyFile class for capsule data
#
#
class CapsuleAnyFile (CapsuleData):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self) :
        self.Ffs = None
        self.FileName = None

    ## generate AnyFile capsule data
    #
    #   @param  self        The object pointer
    #   @retval string      Generated file name
    #
    def GenCapsuleSubItem(self):
        return self.FileName
    
## Afile class for capsule data
#
#
class CapsuleAfile (CapsuleData):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self) :
        self.Ffs = None
        self.FileName = None

    ## generate Afile capsule data
    #
    #   @param  self        The object pointer
    #   @retval string      Generated file name
    #
    def GenCapsuleSubItem(self):
        return self.FileName

class CapsulePayload(CapsuleData):
    '''Generate payload file, the header is defined below:
    #pragma pack(1)
    typedef struct {
        UINT32 Version;
        EFI_GUID UpdateImageTypeId;
        UINT8 UpdateImageIndex;
        UINT8 reserved_bytes[3];
        UINT32 UpdateImageSize;
        UINT32 UpdateVendorCodeSize;
        UINT64 UpdateHardwareInstance; //Introduced in v2
    } EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER;
    '''
    def __init__(self):
        self.UiName = None
        self.Version = None
        self.ImageTypeId = None
        self.ImageIndex = None
        self.HardwareInstance = None
        self.ImageFile = None
        self.VendorCodeFile = None

    def GenCapsuleSubItem(self):
        if not self.Version:
            self.Version = 0x00000002
        ImageFileSize = os.path.getsize(self.ImageFile)
        VendorFileSize = 0
        if self.VendorCodeFile:
            VendorFileSize = os.path.getsize(self.VendorCodeFile)

        #
        # Fill structure
        #
        Guid = self.ImageTypeId.split('-')
        Buffer = pack('=ILHHBBBBBBBBBBBBIIQ',
                       int(self.Version,16),
                       int(Guid[0], 16), 
                       int(Guid[1], 16), 
                       int(Guid[2], 16), 
                       int(Guid[3][-4:-2], 16), 
                       int(Guid[3][-2:], 16),  
                       int(Guid[4][-12:-10], 16),
                       int(Guid[4][-10:-8], 16),
                       int(Guid[4][-8:-6], 16),
                       int(Guid[4][-6:-4], 16),
                       int(Guid[4][-4:-2], 16),
                       int(Guid[4][-2:], 16),
                       int(self.ImageIndex, 16),
                       0,
                       0,
                       0,
                       ImageFileSize,
                       VendorFileSize,
                       int(self.HardwareInstance, 16)
                       )
        #
        # Append file content to the structure
        #
        ImageFile = open(self.ImageFile, 'rb')
        Buffer += ImageFile.read()
        ImageFile.close()
        if self.VendorCodeFile:
            VendorFile = open(self.VendorCodeFile, 'rb')
            Buffer += VendorFile.read()
            VendorFile.close()
        return Buffer
