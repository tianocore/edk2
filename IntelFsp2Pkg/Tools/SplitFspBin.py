## @ FspTool.py
#
# Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

import os
import sys
import uuid
import copy
import struct
import argparse
from   ctypes import *

"""
This utility supports some operations for Intel FSP image.
It supports:
    - Split a FSP 2.0 compatibale image into individual FSP-T/M/S/C
        and generate the mapping header file.
"""

class c_uint24(Structure):
    """Little-Endian 24-bit Unsigned Integer"""
    _pack_   = 1
    _fields_ = [
        ('Data', (c_uint8 * 3))
        ]

    def __init__(self, val=0):
        self.set_value(val)

    def __str__(self, indent=0):
        return '0x%.6x' % self.value

    def get_value(self):
        return (
            (self.Data[0]      ) +
            (self.Data[1] <<  8) +
            (self.Data[2] << 16)
            )

    def set_value(self, val):
        self.Data[0] = (val      ) & 0xff
        self.Data[1] = (val >>  8) & 0xff
        self.Data[2] = (val >> 16) & 0xff

    value = property(get_value, set_value)

class EFI_FIRMWARE_VOLUME_HEADER(Structure):
    _fields_ = [
        ('ZeroVector',                 ARRAY(c_uint8, 16)),
        ('FileSystemGuid',             ARRAY(c_char, 16)),
        ('FvLength',                   c_uint64),
        ('Signature',                  c_uint32),
        ('Attributes',                 c_uint32),
        ('HeaderLength',               c_uint16),
        ('Checksum',                   c_uint16),
        ('ExtHeaderOffset',            c_uint16),
        ('Reserved',                   c_uint8),
        ('Revision',                   c_uint8)
        ]

class EFI_FIRMWARE_VOLUME_EXT_HEADER(Structure):
    _fields_ = [
        ('FvName',                     ARRAY(c_char, 16)),
        ('ExtHeaderSize',              c_uint32)
        ]

class EFI_FFS_INTEGRITY_CHECK(Structure):
    _fields_ = [
        ('Header',                     c_uint8),
        ('File',                       c_uint8)
        ]

class EFI_FFS_FILE_HEADER(Structure):
    _fields_ = [
        ('Name',                       ARRAY(c_char, 16)),
        ('IntegrityCheck',             EFI_FFS_INTEGRITY_CHECK),
        ('Type',                       c_uint8),
        ('Attributes',                 c_uint8),
        ('Size',                       c_uint24),
        ('State',                      c_uint8)
        ]

class EFI_COMMON_SECTION_HEADER(Structure):
    _fields_ = [
        ('Size',                       c_uint24),
        ('Type',                       c_uint8)
        ]

class FSP_INFORMATION_HEADER(Structure):
     _fields_ = [
        ('Signature',                   c_uint32),
        ('HeaderLength',                c_uint32),
        ('Reserved1',                   ARRAY(c_uint8, 3)),
        ('HeaderRevision',              c_uint8),
        ('ImageRevision',               c_uint32),
        ('ImageId',                     c_uint64),
        ('ImageSize',                   c_uint32),
        ('ImageBase',                   c_uint32),
        ('ImageAttribute',              c_uint32),
        ('CfgRegionOffset',             c_uint32),
        ('CfgRegionSize',               c_uint32),
        ('ApiEntryNum',                 c_uint32),
        ('NemInitEntry',                c_uint32),
        ('FspInitEntry',                c_uint32),
        ('NotifyPhaseEntry',            c_uint32),
        ('FspMemoryInitEntry',          c_uint32),
        ('TempRamExitEntry',            c_uint32),
        ('FspSiliconInitEntry',         c_uint32)
    ]

class FspFv:
    HeaderFile = """/*
 *
 * Automatically generated file; DO NOT EDIT.
 * FSP mapping file
 *
 */
"""
    FspNameDict = {
        "0" : "-C.Fv",
        "1" : "-T.Fv",
        "2" : "-M.Fv",
        "3" : "-S.Fv",
    }

    def __init__(self, FvBin):
        self.FspFv  = {}
        self.FvList = []
        self.FspBin = FvBin
        hfsp = open (self.FspBin, 'r+b')
        self.FspDat = bytearray(hfsp.read())
        hfsp.close()

    def OutputStruct (self, obj, indent = 0):
        max_key_len = 20
        pstr = ('  ' * indent + '{0:<%d} = {1}\n') % max_key_len
        if indent:
            s = ''
        else:
            s = ('  ' * indent + '<%s>:\n') % obj.__class__.__name__
        for field in obj._fields_:
            key = field[0]
            val = getattr(obj, key)
            rep = ''
            
            if not isinstance(val, c_uint24) and isinstance(val, Structure):                
                s += pstr.format(key, val.__class__.__name__)
                s += self.OutputStruct (val, indent + 1)                
            else:
                if type(val) in (int, long):
                    rep = hex(val)
                elif isinstance(val, str) and (len(val) == 16):
                    rep = str(uuid.UUID(bytes = val))
                elif isinstance(val, c_uint24):                    
                    rep = hex(val.get_value())
                elif 'c_ubyte_Array' in str(type(val)):                
                    rep = str(list(bytearray(val)))
                else:
                    rep = str(val)
                s += pstr.format(key, rep)
        return s
    
    def PrintFv (self):
        print ("FV LIST:")
        idx = 0
        for (fvh, fvhe, offset) in self.FvList:            
            guid = uuid.UUID(bytes = fvhe.FvName)
            print ("FV%d FV GUID:%s  Offset:0x%08X  Length:0x%08X" % (idx, str(guid), offset, fvh.FvLength))
            idx = idx + 1
        print ("\nFSP LIST:")
        for fsp in self.FspFv:
            print "FSP%s contains FV%s" % (fsp, str(self.FspFv[fsp][1]))
            print "\nFSP%s Info Header:" % fsp
            fih = self.FspFv[fsp][0]            

    def AlaignPtr (self, offset, alignment = 8):
        return (offset + alignment - 1) & ~(alignment - 1)

    def GetFspInfoHdr (self, fvh, fvhe, fvoffset):
        if fvhe:
            offset = fvh.ExtHeaderOffset + fvhe.ExtHeaderSize
        else:
            offset = fvh.HeaderLength
        offset = self.AlaignPtr(offset)

        # Now it should be 1st FFS
        ffs = EFI_FFS_FILE_HEADER.from_buffer (self.FspDat, offset)        
        offset += sizeof(ffs)
        offset = self.AlaignPtr(offset)

        # Now it should be 1st Section
        sec = EFI_COMMON_SECTION_HEADER.from_buffer (self.FspDat, offset)
        offset += sizeof(sec)

        # Now it should be FSP_INFO_HEADER
        offset += fvoffset
        fih = FSP_INFORMATION_HEADER.from_buffer (self.FspDat, offset)
        if 'FSPH' != bytearray.fromhex('%08X' % fih.Signature)[::-1]:
            return None

        return fih

    def GetFvHdr (self, offset):
        fvh = EFI_FIRMWARE_VOLUME_HEADER.from_buffer (self.FspDat, offset)
        if '_FVH' != bytearray.fromhex('%08X' % fvh.Signature)[::-1]:
            return None, None
        if fvh.ExtHeaderOffset > 0:
            offset += fvh.ExtHeaderOffset
            fvhe = EFI_FIRMWARE_VOLUME_EXT_HEADER.from_buffer (self.FspDat, offset)
        else:
            fvhe = None
        return fvh, fvhe

    def GetFvData(self, idx):
        (fvh, fvhe, offset) = self.FvList[idx]
        return self.FspDat[offset:offset+fvh.FvLength]

    def CheckFsp (self):
        if len(self.FspFv) == 0:
            return

        fih = None
        for fv in self.FspFv:
            if not fih:
                fih = self.FspFv[fv][0]
            else:
                newfih = self.FspFv[fv][0]
                if (newfih.ImageId != fih.ImageId) or (newfih.ImageRevision != fih.ImageRevision):
                    raise Exception("Inconsistent FSP ImageId or ImageRevision detected !")
        return

    def WriteFsp(self, dir, name):
        if not name:
            name = self.FspBin
        fspname, ext = os.path.splitext(os.path.basename(name))
        for fv in self.FspFv:
            filename = os.path.join(dir, fspname + fv + ext)
            hfsp = open(filename, 'w+b')
            for fvidx in self.FspFv[fv][1]:
                hfsp.write (self.GetFvData(fvidx))
            hfsp.close()

    def WriteMap(self, dir, hfile):
        if not hfile:
            hfile = os.path.splitext(os.path.basename(self.FspBin))[0] + '.h'
        fspname, ext = os.path.splitext(os.path.basename(hfile))
        filename = os.path.join(dir, fspname + ext)
        hfsp   = open(filename, 'w')
        hfsp.write ('%s\n\n' % self.HeaderFile)

        firstfv = True
        for fsp in self.FspFv:
            fih = self.FspFv[fsp][0]
            fvs = self.FspFv[fsp][1]
            if firstfv:
                IdStr = str(bytearray.fromhex('%016X' % fih.ImageId)[::-1])
                hfsp.write("#define  FSP_IMAGE_ID    0x%016X        /* '%s' */\n" % (fih.ImageId, IdStr))
                hfsp.write("#define  FSP_IMAGE_REV   0x%08X \n\n" % fih.ImageRevision)
                firstfv = False
            hfsp.write ('#define  FSP%s_BASE      0x%08X\n'   % (fsp, fih.ImageBase))
            hfsp.write ('#define  FSP%s_OFFSET    0x%08X\n'   % (fsp, self.FvList[fvs[0]][-1]))
            hfsp.write ('#define  FSP%s_LENGTH    0x%08X\n\n' % (fsp, fih.ImageSize))
        hfsp.close()

    def ParseFsp (self):
        self.FspFv  = {}
        flen = 0
        for (fvh, fvhe, offset) in self.FvList:
            fih = self.GetFspInfoHdr (fvh, fvhe, offset)
            if fih:
                if flen != 0:
                    raise Exception("Incorrect FV size in image !")
                ftype = str((fih.ImageAttribute >> 28) & 0xF)
                if ftype not in self.FspNameDict:
                    raise Exception("Unknown Attribute in image !")
                fname = self.FspNameDict[str(ftype)]
                if fname in self.FspFv:
                    raise Exception("Multiple '%s' in image !" % fname)
                self.FspFv[fname] = (copy.deepcopy(fih), [])
                flen = fih.ImageSize
            if flen > 0:
                flen = flen - fvh.FvLength
                if flen < 0:
                    raise Exception("Incorrect FV size in image !")
                self.FspFv[fname][1].append(self.FvList.index((fvh, fvhe, offset)))

    def AddFv(self, offset):
        fvh, fvhe = self.GetFvHdr (offset)
        if fvh is None:
            raise Exception('FV signature is not valid !')
        fvitem = (copy.deepcopy(fvh), copy.deepcopy(fvhe), offset)
        self.FvList.append(fvitem)
        return fvh.FvLength

    def ParseFv(self):
        offset = 0
        while (offset < len(self.FspDat)):
            fv_len  = self.AddFv (offset)
            offset += fv_len

def GenFspHdr (fspfile, outdir, hfile, show):
    fsp_fv = FspFv(fspfile)
    fsp_fv.ParseFv()
    fsp_fv.ParseFsp()
    fsp_fv.CheckFsp()
    if show:
        fsp_fv.PrintFv()
    fsp_fv.WriteMap(outdir, hfile)

def SplitFspBin (fspfile, outdir, nametemplate, show):
    fsp_fv = FspFv(fspfile)
    fsp_fv.ParseFv()
    fsp_fv.ParseFsp()
    if show:
        fsp_fv.PrintFv()
    fsp_fv.WriteFsp(outdir, nametemplate)

def main ():
    parser = argparse.ArgumentParser()
    subparsers    = parser.add_subparsers(title='commands')

    parser_split  = subparsers.add_parser('split',  help='split a FSP into multiple components')
    parser_split.set_defaults(which='split')
    parser_split.add_argument('-f',  '--fspbin' , dest='FspBinary', type=str, help='FSP binary file path', required = True)
    parser_split.add_argument('-o',  '--outdir' , dest='OutputDir', type=str, help='Output directory path',   default = '.')
    parser_split.add_argument('-n',  '--nametpl', dest='NameTemplate', type=str, help='Output name template', default = '')
    parser_split.add_argument('-p', action='store_true', help='Print FSP FV information', default = False)

    parser_genhdr = subparsers.add_parser('genhdr',  help='generate a header file for FSP binary')
    parser_genhdr.set_defaults(which='genhdr')
    parser_genhdr.add_argument('-f',  '--fspbin' , dest='FspBinary', type=str, help='FSP binary file path', required = True)
    parser_genhdr.add_argument('-o',  '--outdir' , dest='OutputDir', type=str, help='Output directory path',   default = '.')
    parser_genhdr.add_argument('-n',  '--hfile',   dest='HFileName', type=str, help='Output header file name', default = '')
    parser_genhdr.add_argument('-p', action='store_true', help='Print FSP FV information', default = False)

    args = parser.parse_args()
    if args.which in ['split', 'genhdr']:
        if not os.path.exists(args.FspBinary):
            raise Exception ("Could not locate FSP binary file '%s' !" % args.FspBinary)
        if not os.path.exists(args.OutputDir):
            raise Exception ("Invalid output directory '%s' !" % args.OutputDir)

    if args.which == 'split':
        SplitFspBin (args.FspBinary, args.OutputDir, args.NameTemplate, args.p)
    elif args.which == 'genhdr':
        GenFspHdr   (args.FspBinary, args.OutputDir, args.HFileName, args.p)
    else:
        pass

    print 'Done!'
    return 0

if __name__ == '__main__':
    sys.exit(main())
